#include "handlers.hpp"

#include <string>

#include <jwt-cpp/jwt.h>

#include <userver/components/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_status.hpp>

#include "../common/mongo_component.hpp"
#include "../common/metrics_handler.hpp"

namespace taxi_service::input {

namespace {

constexpr std::string_view kJwtSecret  = "taxi-secret-key-change-in-production";
constexpr std::string_view kJwtIssuer  = "taxi-service";
constexpr int              kTokenTtlH  = 24;

std::string ErrorJson(const std::string& message) {
    return userver::formats::json::ToString(
        userver::formats::json::MakeObject("error", message));
}

}  // namespace

CreateUserHandler::CreateUserHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    auto& mongo_component = context.FindComponent<taxi::MongoComponent>("mongo-db");
    db_ = mongo_component.GetDatabase();
    
    try {
        cache_ = &context.FindComponent<CacheComponent>("cache");
    } catch (const std::exception&) {
        cache_ = nullptr;
    }
    
    try {
        rate_limiter_ = &context.FindComponent<RateLimiterComponent>("rate-limiter");
    } catch (const std::exception&) {
        rate_limiter_ = nullptr;
    }
}

std::string CreateUserHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {

    // Rate limiting check
    if (rate_limiter_) {
        auto client_ip = request.GetRemoteAddress().PrimaryAddressString();
        if (!rate_limiter_->CheckUserRegistration(client_ip)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kTooManyRequests);
            auto& response = request.GetHttpResponse();
            RateLimitConfig config{10, std::chrono::seconds(60), 5};
            SetRateLimitHeaders(response, rate_limiter_->GetRateLimitInfo("user_reg:" + client_ip, config));
            response.SetHeader(std::string{"Retry-After"}, std::string{"60"});
            return ErrorJson("Too many registration attempts");
        }
    }

    userver::formats::json::Value body;
    try {
        body = userver::formats::json::FromString(request.RequestBody());
    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Request body is not valid JSON");
    }

    const std::vector<std::string> required{
        "login", "email", "first_name", "last_name", "phone", "password"};
    for (const auto& field : required) {
        if (!body.HasMember(field) || body[field].As<std::string>().empty()) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Missing or empty required field: " + field);
        }
    }

    const auto email = body["email"].As<std::string>();
    if (email.find('@') == std::string::npos ||
        email.find('.') == std::string::npos ||
        email.find(' ') != std::string::npos) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Invalid email format");
    }

    const auto phone = body["phone"].As<std::string>();
    if (phone.empty() || phone[0] != '+') {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Phone number must start with '+'");
    }

    const auto req = CreateUserRequest::FromJson(body);
    const auto user = db_->CreateUser(req);
    if (!user) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
        return ErrorJson("A user with this login or email already exists");
    }

    LOG_INFO() << "[input] Created user id=" << *user->id
               << " login=" << user->login;

    // Invalidate cache for this user
    if (cache_) {
        cache_->GetUserCache()->Invalidate("user:login:" + req.login);
    }

    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    return userver::formats::json::ToString(user->ToJson());
}

LoginHandler::LoginHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    auto& mongo_component = context.FindComponent<taxi::MongoComponent>("mongo-db");
    db_ = mongo_component.GetDatabase();
    
    try {
        cache_ = &context.FindComponent<CacheComponent>("cache");
    } catch (const std::exception&) {
        cache_ = nullptr;
    }
}

std::string LoginHandler::IssueToken(const std::string& user_id,
                                     const std::string& role) const {
    const auto now     = std::chrono::system_clock::now();
    const auto expires = now + std::chrono::hours(kTokenTtlH);

    return ::jwt::create<::jwt::traits::kazuho_picojson>()
        .set_issuer(std::string{kJwtIssuer})
        .set_type("JWT")
        .set_payload_claim("user_id", ::jwt::claim(user_id))
        .set_payload_claim("role", ::jwt::claim(role))
        .set_issued_at(now)
        .set_expires_at(expires)
        .sign(::jwt::algorithm::hs256{std::string{kJwtSecret}});
}

std::string LoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {

    userver::formats::json::Value body;
    try {
        body = userver::formats::json::FromString(request.RequestBody());
    } catch (const std::exception&) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Request body is not valid JSON");
    }

    if (!body.HasMember("login") || body["login"].As<std::string>().empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Field 'login' is required");
    }
    if (!body.HasMember("password") || body["password"].As<std::string>().empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Field 'password' is required");
    }

    const auto req = LoginRequest::FromJson(body);

    const auto user = db_->AuthenticateUser(req.login, req.password);
    if (!user) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        return ErrorJson("Invalid login or password");
    }

    const auto driver = db_->FindDriverByUserId(*user->id);
    const std::string role = driver.has_value() ? "driver" : "passenger";

    AuthToken token;
    token.token      = IssueToken(*user->id, role);
    token.user_id    = *user->id;
    token.expires_at = std::chrono::system_clock::now() +
                       std::chrono::hours(kTokenTtlH);

    LOG_INFO() << "[input] Login successful user_id=" << *user->id
               << " role=" << role;

    return userver::formats::json::ToString(token.ToJson());
}

FindUsersHandler::FindUsersHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    auto& mongo_component = context.FindComponent<taxi::MongoComponent>("mongo-db");
    db_ = mongo_component.GetDatabase();
    
    try {
        cache_ = &context.FindComponent<CacheComponent>("cache");
    } catch (const std::exception&) {
        cache_ = nullptr;
    }
}

std::string FindUsersHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {

    const auto login_param = request.GetArg("login");
    const auto name_param  = request.GetArg("name");

    if (!login_param.empty()) {
        // Check cache first
        std::string cache_key = "user:login:" + login_param;
        if (cache_) {
            auto cached_user = cache_->GetUserCache()->Get(cache_key);
            if (cached_user) {
                cache_->GetUserCacheStats().RecordHit();
                LOG_DEBUG() << "[input] FindUsers cache hit for login=" << login_param;
                return *cached_user;
            }
            cache_->GetUserCacheStats().RecordMiss();
        }

        const auto user = db_->FindUserByLogin(login_param);
        if (!user) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
            return ErrorJson("User with login '" + login_param + "' not found");
        }
        
        // Cache the result
        if (cache_) {
            auto user_json = userver::formats::json::ToString(user->ToJson());
            cache_->GetUserCache()->Put(cache_key, user_json, std::chrono::seconds(300));
        }
        
        LOG_DEBUG() << "[input] FindUsers by login=" << login_param
                    << " found id=" << *user->id;
        return userver::formats::json::ToString(user->ToJson());
    }

    if (!name_param.empty()) {
        const auto users = db_->SearchUsersByNameMask(name_param);

        userver::formats::json::ValueBuilder arr(
            userver::formats::json::Type::kArray);
        for (const auto& u : users) {
            arr.PushBack(u.ToJson());
        }

        LOG_DEBUG() << "[input] FindUsers by name mask='" << name_param
                    << "' found " << users.size() << " result(s)";
        return userver::formats::json::ToString(arr.ExtractValue());
    }

    request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
    return ErrorJson(
        "Provide either '?login=<exact>' or '?name=<mask>' query parameter");
}

RegisterDriverHandler::RegisterDriverHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    auto& mongo_component = context.FindComponent<taxi::MongoComponent>("mongo-db");
    db_ = mongo_component.GetDatabase();
    
    try {
        cache_ = &context.FindComponent<CacheComponent>("cache");
    } catch (const std::exception&) {
        cache_ = nullptr;
    }
    
    try {
        rate_limiter_ = &context.FindComponent<RateLimiterComponent>("rate-limiter");
    } catch (const std::exception&) {
        rate_limiter_ = nullptr;
    }
}

std::string RegisterDriverHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {

    // Rate limiting check
    if (rate_limiter_) {
        auto client_ip = request.GetRemoteAddress().PrimaryAddressString();
        if (!rate_limiter_->CheckDriverRegistration(client_ip)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kTooManyRequests);
            auto& response = request.GetHttpResponse();
            RateLimitConfig config{10, std::chrono::seconds(60), 5};
            SetRateLimitHeaders(response, rate_limiter_->GetRateLimitInfo("driver_reg:" + client_ip, config));
            response.SetHeader(std::string{"Retry-After"}, std::string{"60"});
            return ErrorJson("Too many registration attempts");
        }
    }

    std::string user_id;
    try {
        user_id = context.GetData<std::string>("user_id");
    } catch (const std::exception&) {

        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        return ErrorJson("Authentication required");
    }

    const auto user = db_->FindUserById(user_id);
    if (!user) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        return ErrorJson("Authenticated user account not found");
    }

    userver::formats::json::Value body;
    try {
        body = userver::formats::json::FromString(request.RequestBody());
    } catch (const std::exception&) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Request body is not valid JSON");
    }

    const std::vector<std::string> required{"license_number", "car_model", "car_plate"};
    for (const auto& field : required) {
        if (!body.HasMember(field) || body[field].As<std::string>().empty()) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Missing or empty required field: " + field);
        }
    }

    const auto req    = CreateDriverRequest::FromJson(body);
    const auto driver = db_->CreateDriver(user_id, req);
    if (!driver) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
        return ErrorJson("This user is already registered as a driver");
    }

    LOG_INFO() << "[input] Registered driver id=" << *driver->id
               << " user_id=" << user_id
               << " plate=" << driver->car_plate;

    // Invalidate cache for this driver
    if (cache_) {
        cache_->GetDriverCache()->Invalidate("driver:user_id:" + user_id);
    }

    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    return userver::formats::json::ToString(driver->ToJson());
}

void AppendInputHandlers(userver::components::ComponentList& component_list) {
    component_list
        .Append<auth::TaxiJwtAuthComponent>()
        .Append<MetricsHandler>()
        .Append<CreateUserHandler>()
        .Append<LoginHandler>()
        .Append<FindUsersHandler>()
        .Append<RegisterDriverHandler>();
}

}  // namespace taxi_service::input
