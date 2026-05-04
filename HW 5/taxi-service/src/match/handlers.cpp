#include "handlers.hpp"

#include <string>

#include <userver/components/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_status.hpp>
#include "../common/mongo_component.hpp"
#include "../common/metrics_handler.hpp"

namespace taxi_service::match {

namespace {

std::string ErrorJson(const std::string& message) {
    return userver::formats::json::ToString(
        userver::formats::json::MakeObject("error", message));
}

std::string ExtractUserId(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context,
    std::string& out_error) {

    try {
        return context.GetData<std::string>("user_id");
    } catch (const std::exception&) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        out_error = "Authentication required";
        return {};
    }
}

std::string ExtractRole(userver::server::request::RequestContext& context) {
    try {
        return context.GetData<std::string>("role");
    } catch (const std::exception&) {
        return "passenger";
    }
}

}  // namespace

ListActiveOrdersHandler::ListActiveOrdersHandler(
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

std::string ListActiveOrdersHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {

    std::string auth_error;
    const std::string caller_id = ExtractUserId(request, context, auth_error);
    if (caller_id.empty()) {
        return ErrorJson(auth_error);
    }

    // ── Rate limiting check ───────────────────────────────────────────────────
    if (rate_limiter_) {
        const auto driver_record = db_->FindDriverByUserId(caller_id);
        if (driver_record) {
            if (!rate_limiter_->CheckActiveOrders(*driver_record->id)) {
                request.SetResponseStatus(userver::server::http::HttpStatus::kTooManyRequests);
                auto& response = request.GetHttpResponse();
                RateLimitConfig config{100, std::chrono::seconds(60), 20};
                SetRateLimitHeaders(response, rate_limiter_->GetRateLimitInfo("active_orders:" + *driver_record->id, config));
                response.SetHeader(std::string{"Retry-After"}, std::string{"60"});
                return ErrorJson("Too many active orders requests");
            }
        }
    }

    const std::string status_param = request.GetArg("status");
    if (status_param.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Query parameter 'status' is required (expected: active)");
    }
    if (status_param != "active") {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson(
            "Unsupported status filter '" + status_param +
            "'. This endpoint only supports status=active");
    }

    // ── Check cache first ────────────────────────────────────────────────────
    std::string cache_key = "order:active";
    if (cache_) {
        auto cached_orders = cache_->GetOrderCache()->Get(cache_key);
        if (cached_orders) {
            cache_->GetOrderCacheStats().RecordHit();
            LOG_DEBUG() << "[match] ListActiveOrders cache hit";
            return *cached_orders;
        }
        cache_->GetOrderCacheStats().RecordMiss();
    }

    const auto orders = db_->GetActiveOrders();

    userver::formats::json::ValueBuilder arr(
        userver::formats::json::Type::kArray);
    for (const auto& o : orders) {
        arr.PushBack(o.ToJson());
    }

    auto orders_json = userver::formats::json::ToString(arr.ExtractValue());
    
    // Cache the result with short TTL (10 seconds)
    if (cache_) {
        cache_->GetOrderCache()->Put(cache_key, orders_json, std::chrono::seconds(10));
    }

    LOG_DEBUG() << "[match] ListActiveOrders returned " << orders.size()
                << " order(s) (caller_id=" << caller_id
                << " role=" << ExtractRole(context) << ")";

    return orders_json;
}

AcceptOrderHandler::AcceptOrderHandler(
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

std::string AcceptOrderHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {

    std::string auth_error;
    const std::string caller_id = ExtractUserId(request, context, auth_error);
    if (caller_id.empty()) {
        return ErrorJson(auth_error);
    }

    const std::string role = ExtractRole(context);
    if (role != "driver") {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return ErrorJson("Only registered drivers can accept orders");
    }

    const auto driver_record = db_->FindDriverByUserId(caller_id);
    if (!driver_record) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return ErrorJson("No driver profile found for the authenticated user");
    }

    const std::string order_id = request.GetPathArg("order_id");
    if (order_id.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Path parameter 'order_id' must not be empty");
    }

    const auto existing = db_->GetOrderById(order_id);
    if (!existing) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
        return ErrorJson("Order with id=" + order_id + " not found");
    }

    const auto accepted = db_->AcceptOrder(order_id, *driver_record->id);
    if (!accepted) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson(
            "Order cannot be accepted: current status is '" +
            existing->status + "' (must be 'active')");
    }

    // Invalidate caches
    if (cache_) {
        cache_->GetOrderCache()->Invalidate("order:active");
        cache_->GetOrderCache()->Invalidate("order:history:" + existing->user_id);
    }

    LOG_INFO() << "[match] Accepted order id=" << order_id
               << " driver_id=" << *driver_record->id
               << " user_id=" << existing->user_id;

    return userver::formats::json::ToString(accepted->ToJson());
}

void AppendMatchHandlers(userver::components::ComponentList& component_list) {
    component_list
        .Append<auth::TaxiJwtAuthComponent>()
        .Append<MetricsHandler>()
        .Append<ListActiveOrdersHandler>()
        .Append<AcceptOrderHandler>();
}

}  // namespace taxi_service::match
