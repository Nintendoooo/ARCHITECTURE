/// @file handlers.cpp
/// @brief Order Service handler implementations.
///
/// Handlers:
///   - CreateOrderHandler    POST /api/orders
///   - GetUserOrdersHandler  GET  /api/users/{user_id}/orders
///   - CompleteOrderHandler  POST /api/orders/{order_id}/complete

#include "handlers.hpp"

#include <string>

#include <userver/components/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_status.hpp>

#include "../common/mongo_component.hpp"
#include "../common/metrics_handler.hpp"

namespace taxi_service::order {

namespace {

/// Build a minimal {"error": "<message>"} JSON response body.
std::string ErrorJson(const std::string& message) {
    return userver::formats::json::ToString(
        userver::formats::json::MakeObject("error", message));
}

/// Extract the authenticated user_id from the request context.
/// Returns empty string and sets the response to 401 if the claim is absent.
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

/// Extract the role string from the request context.
/// Falls back to "passenger" if the claim is absent.
std::string ExtractRole(userver::server::request::RequestContext& context) {
    try {
        return context.GetData<std::string>("role");
    } catch (const std::exception&) {
        return "passenger";
    }
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
//  CreateOrderHandler  —  POST /api/orders
// ─────────────────────────────────────────────────────────────────────────────

CreateOrderHandler::CreateOrderHandler(
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

std::string CreateOrderHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {

    // ── Rate limiting check ───────────────────────────────────────────────────
    if (rate_limiter_) {
        std::string user_id;
        try {
            user_id = context.GetData<std::string>("user_id");
        } catch (const std::exception&) {
            // Will fail authentication later
        }
        
        if (!user_id.empty() && !rate_limiter_->CheckOrderCreation(user_id)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kTooManyRequests);
            auto& response = request.GetHttpResponse();
            RateLimitConfig config{20, std::chrono::seconds(60), 10};
            SetRateLimitHeaders(response, rate_limiter_->GetRateLimitInfo("order_create:" + user_id, config));
            response.SetHeader(std::string{"Retry-After"}, std::string{"60"});
            return ErrorJson("Too many order creation attempts");
        }
    }

    // ── Authenticate ──────────────────────────────────────────────────────────
    std::string auth_error;
    const std::string user_id = ExtractUserId(request, context, auth_error);
    if (user_id.empty()) {
        return ErrorJson(auth_error);
    }

    // ── Parse body ────────────────────────────────────────────────────────────
    userver::formats::json::Value body;
    try {
        body = userver::formats::json::FromString(request.RequestBody());
    } catch (const std::exception&) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Request body is not valid JSON");
    }

    // ── Validate required fields ──────────────────────────────────────────────
    const std::vector<std::string> required{"pickup_address", "destination_address"};
    for (const auto& field : required) {
        if (!body.HasMember(field) || body[field].As<std::string>().empty()) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Missing or empty required field: " + field);
        }
    }

    // ── Guard: pickup and destination must differ ─────────────────────────────
    const auto pickup = body["pickup_address"].As<std::string>();
    const auto dest   = body["destination_address"].As<std::string>();
    if (pickup == dest) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("pickup_address and destination_address must be different");
    }

    // ── Persist ───────────────────────────────────────────────────────────────
    const auto req   = CreateOrderRequest::FromJson(body);
    const auto order = db_->CreateOrder(user_id, req);
    if (!order) {
        // Unexpected DB failure
        request.SetResponseStatus(
            userver::server::http::HttpStatus::kInternalServerError);
        return ErrorJson("Failed to create order — please try again");
    }

    LOG_INFO() << "[order] Created order id=" << *order->id
               << " user_id=" << user_id
               << " pickup='" << order->pickup_address << "'"
               << " dest='"   << order->destination_address << "'";

    // Invalidate cache for user's order history
    if (cache_) {
        cache_->GetOrderCache()->Invalidate("order:history:" + user_id);
    }

    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    return userver::formats::json::ToString(order->ToJson());
}

// ─────────────────────────────────────────────────────────────────────────────
//  GetUserOrdersHandler  —  GET /api/users/{user_id}/orders
//
//  Returns the full trip history for the given passenger, newest first.
//  A passenger may only view their own history; drivers may view any user's
//  history (useful for dispatch tooling).
// ─────────────────────────────────────────────────────────────────────────────

GetUserOrdersHandler::GetUserOrdersHandler(
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

std::string GetUserOrdersHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {

    // ── Authenticate ──────────────────────────────────────────────────────────
    std::string auth_error;
    const std::string caller_id = ExtractUserId(request, context, auth_error);
    if (caller_id.empty()) {
        return ErrorJson(auth_error);
    }

    // ── Parse path parameter ──────────────────────────────────────────────────
    std::string target_user_id;
    target_user_id = request.GetPathArg("user_id");
    if (target_user_id.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Path parameter 'user_id' must not be empty");
    }

    // ── Access control ────────────────────────────────────────────────────────
    // Passengers may only view their own history.
    // Drivers are allowed to view any user's history (e.g. for support).
    const std::string role = ExtractRole(context);
    if (role != "driver" && caller_id != target_user_id) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return ErrorJson("You are not allowed to view another user's trip history");
    }

    // ── Check cache first ────────────────────────────────────────────────────
    std::string cache_key = "order:history:" + target_user_id;
    if (cache_) {
        auto cached_orders = cache_->GetOrderCache()->Get(cache_key);
        if (cached_orders) {
            cache_->GetOrderCacheStats().RecordHit();
            LOG_DEBUG() << "[order] GetUserOrders cache hit for user_id=" << target_user_id;
            return *cached_orders;
        }
        cache_->GetOrderCacheStats().RecordMiss();
    }

    // ── Fetch orders ──────────────────────────────────────────────────────────
    const auto orders = db_->GetOrdersByUserId(target_user_id);

    userver::formats::json::ValueBuilder arr(
        userver::formats::json::Type::kArray);
    for (const auto& o : orders) {
        arr.PushBack(o.ToJson());
    }

    auto orders_json = userver::formats::json::ToString(arr.ExtractValue());
    
    // Cache the result
    if (cache_) {
        cache_->GetOrderCache()->Put(cache_key, orders_json, std::chrono::seconds(30));
    }

    LOG_DEBUG() << "[order] GetUserOrders user_id=" << target_user_id
                << " returned " << orders.size() << " record(s)"
                << " (caller_id=" << caller_id << " role=" << role << ")";

    return orders_json;
}

// ─────────────────────────────────────────────────────────────────────────────
//  CompleteOrderHandler  —  POST /api/orders/{order_id}/complete
//
//  Transitions an order from 'accepted' → 'completed'.
//  Only the driver who accepted the order may call this endpoint.
// ─────────────────────────────────────────────────────────────────────────────

CompleteOrderHandler::CompleteOrderHandler(
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

std::string CompleteOrderHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {

    // ── Authenticate ──────────────────────────────────────────────────────────
    std::string auth_error;
    const std::string caller_id = ExtractUserId(request, context, auth_error);
    if (caller_id.empty()) {
        return ErrorJson(auth_error);
    }

    // ── Role check: only drivers may complete orders ──────────────────────────
    const std::string role = ExtractRole(context);
    if (role != "driver") {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return ErrorJson("Only drivers can complete orders");
    }

    // ── Parse path parameter ──────────────────────────────────────────────────
    const std::string order_id = request.GetPathArg("order_id");
    if (order_id.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Path parameter 'order_id' must not be empty");
    }

    // ── Fetch the order to verify it exists and check assignment ──────────────
    const auto existing = db_->GetOrderById(order_id);
    if (!existing) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
        return ErrorJson("Order with id=" + order_id + " not found");
    }

    // ── Verify the caller is the assigned driver ──────────────────────────────
    // Look up the driver record for the authenticated user.
    const auto driver_record = db_->FindDriverByUserId(caller_id);
    if (!driver_record) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return ErrorJson("No driver profile found for the authenticated user");
    }

    if (!existing->driver_id.has_value() ||
        *existing->driver_id != *driver_record->id) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return ErrorJson("You are not the driver assigned to this order");
    }

    // ── Attempt status transition: accepted → completed ───────────────────────
    const auto completed = db_->CompleteOrder(order_id);
    if (!completed) {
        // CompleteOrder returns nullopt when the order is not in 'accepted' status
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson(
            "Order cannot be completed: current status is '" +
            existing->status + "' (must be 'accepted')");
    }

    // Invalidate caches
    if (cache_) {
        cache_->GetOrderCache()->Invalidate("order:history:" + existing->user_id);
        cache_->GetOrderCache()->Invalidate("order:active");
    }

    LOG_INFO() << "[order] Completed order id=" << order_id
               << " driver_id=" << *driver_record->id
               << " user_id=" << existing->user_id;

    return userver::formats::json::ToString(completed->ToJson());
}

// ─────────────────────────────────────────────────────────────────────────────
//  AppendOrderHandlers
// ─────────────────────────────────────────────────────────────────────────────

void AppendOrderHandlers(userver::components::ComponentList& component_list) {
    component_list
        .Append<auth::TaxiJwtAuthComponent>()
        .Append<MetricsHandler>()
        .Append<CreateOrderHandler>()
        .Append<GetUserOrdersHandler>()
        .Append<CompleteOrderHandler>();
}

}  // namespace taxi_service::order
