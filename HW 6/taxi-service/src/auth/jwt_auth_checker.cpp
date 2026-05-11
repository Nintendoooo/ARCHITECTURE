#include "jwt_auth_checker.hpp"

#include <jwt-cpp/jwt.h>

#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace taxi_service::auth {

namespace {

constexpr std::string_view kBearerPrefix  = "Bearer ";
constexpr std::string_view kSecretKey     = "secret";
constexpr std::string_view kIssuer        = "taxi-service";
constexpr std::string_view kClaimUserId   = "user_id";
constexpr std::string_view kClaimRole     = "role";

}  // namespace

JwtChecker::JwtChecker(const std::string& secret) : secret_(secret) {}

JwtChecker::AuthCheckResult JwtChecker::CheckAuth(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {

  const std::string_view auth_header =
      request.GetHeader(userver::http::headers::kAuthorization);

  if (auth_header.empty()) {
    LOG_WARNING() << "[taxi-auth] Missing Authorization header";
    return AuthCheckResult{AuthCheckResult::Status::kTokenNotFound,
                           "Authorization header is required"};
  }

  if (!auth_header.starts_with(kBearerPrefix)) {
    LOG_WARNING() << "[taxi-auth] Authorization header does not start with Bearer";
    return AuthCheckResult{AuthCheckResult::Status::kInvalidToken,
                           "Expected 'Bearer <token>' in Authorization header"};
  }

  const std::string token{auth_header.substr(kBearerPrefix.length())};

  try {
    auto decoded = ::jwt::decode<::jwt::traits::kazuho_picojson>(token);

    auto verifier = ::jwt::verify<::jwt::traits::kazuho_picojson>()
        .allow_algorithm(::jwt::algorithm::hs256{secret_})
        .with_issuer(std::string{kIssuer});

    verifier.verify(decoded);

    if (!decoded.has_payload_claim(std::string{kClaimUserId})) {
      LOG_WARNING() << "[taxi-auth] Token is missing 'user_id' claim";
      return AuthCheckResult{AuthCheckResult::Status::kInvalidToken,
                             "Token payload must contain 'user_id'"};
    }

    const std::string user_id =
        decoded.get_payload_claim(std::string{kClaimUserId}).as_string();
    context.SetData(std::string{kClaimUserId}, user_id);

    if (decoded.has_payload_claim(std::string{kClaimRole})) {
      const std::string role =
          decoded.get_payload_claim(std::string{kClaimRole}).as_string();
      context.SetData(std::string{kClaimRole}, role);
      LOG_DEBUG() << "[taxi-auth] Authenticated user_id=" << user_id
                  << " role=" << role;
    } else {
      context.SetData(std::string{kClaimRole}, std::string{"passenger"});
      LOG_DEBUG() << "[taxi-auth] Authenticated user_id=" << user_id
                  << " role=passenger (default)";
    }

    return {};  

  } catch (const ::jwt::error::token_verification_exception& exc) {
    LOG_WARNING() << "[taxi-auth] Token verification failed: " << exc.what();
    return AuthCheckResult{
        AuthCheckResult::Status::kInvalidToken,
        "Token verification failed: " + std::string{exc.what()}};
  } catch (const std::exception& exc) {
    LOG_ERROR() << "[taxi-auth] Unexpected error during token processing: "
                << exc.what();
    return AuthCheckResult{
        AuthCheckResult::Status::kForbidden,
        "Token processing error: " + std::string{exc.what()}};
  }
}

TaxiJwtAuthComponent::TaxiJwtAuthComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : LoggableComponentBase(config, context) {
  const auto secret = config[kSecretKey].As<std::string>();
  checker_ = std::make_shared<JwtChecker>(secret);
  LOG_INFO() << "[taxi-auth] TaxiJwtAuthComponent initialised";
}

JwtCheckerPtr TaxiJwtAuthComponent::GetChecker() const { return checker_; }

userver::yaml_config::Schema TaxiJwtAuthComponent::GetStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Taxi service JWT authentication component
additionalProperties: false
properties:
    secret:
        type: string
        description: HMAC-SHA256 secret key used to sign and verify JWT tokens
)");
}

}  // namespace taxi_service::auth
