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

namespace taxi_service::order {

namespace {

/// Build a minimal {"error": "<message>"} JSON response body.
std::string ErrorJson(const std::string& message) {
    return userver::formats::json::ToString(
        userver::formats::json::MakeObject("error", message));
}

/// Extract the authenticated user_id from the request context.
/// Returns 0 and sets the response to 401 if the claim is absent.
int64_t ExtractUserId(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context,
    std::string& out_error) {

    try {
        return context.GetData<int64_t>("user_id");
    } catch (const std::exception&) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        out_error = "Authentication required";
        return 0;
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
    : HttpHandlerBase(config, context),
      db_(std::make_shared<Database>("/app/data/taxi.db")) {}

std::string CreateOrderHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {

    // ── Authenticate ──────────────────────────────────────────────────────────
    std::string auth_error;
    const int64_t user_id = ExtractUserId(request, context, auth_error);
    if (user_id == 0) {
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
    : HttpHandlerBase(config, context),
      db_(std::make_shared<Database>("/app/data/taxi.db")) {}

std::string GetUserOrdersHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {

    // ── Authenticate ──────────────────────────────────────────────────────────
    std::string auth_error;
    const int64_t caller_id = ExtractUserId(request, context, auth_error);
    if (caller_id == 0) {
        return ErrorJson(auth_error);
    }

    // ── Parse path parameter ──────────────────────────────────────────────────
    int64_t target_user_id{0};
    try {
        target_user_id = std::stoll(request.GetPathArg("user_id"));
    } catch (const std::exception&) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Path parameter 'user_id' must be a valid integer");
    }

    if (target_user_id <= 0) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Path parameter 'user_id' must be a positive integer");
    }

    // ── Access control ────────────────────────────────────────────────────────
    // Passengers may only view their own history.
    // Drivers are allowed to view any user's history (e.g. for support).
    const std::string role = ExtractRole(context);
    if (role != "driver" && caller_id != target_user_id) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return ErrorJson("You are not allowed to view another user's trip history");
    }

    // ── Fetch orders ──────────────────────────────────────────────────────────
    const auto orders = db_->GetOrdersByUserId(target_user_id);

    userver::formats::json::ValueBuilder arr(
        userver::formats::json::Type::kArray);
    for (const auto& o : orders) {
        arr.PushBack(o.ToJson());
    }

    LOG_DEBUG() << "[order] GetUserOrders user_id=" << target_user_id
                << " returned " << orders.size() << " record(s)"
                << " (caller_id=" << caller_id << " role=" << role << ")";

    return userver::formats::json::ToString(arr.ExtractValue());
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
    : HttpHandlerBase(config, context),
      db_(std::make_shared<Database>("/app/data/taxi.db")) {}

std::string CompleteOrderHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {

    // ── Authenticate ──────────────────────────────────────────────────────────
    std::string auth_error;
    const int64_t caller_id = ExtractUserId(request, context, auth_error);
    if (caller_id == 0) {
        return ErrorJson(auth_error);
    }

    // ── Role check: only drivers may complete orders ──────────────────────────
    const std::string role = ExtractRole(context);
    if (role != "driver") {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return ErrorJson("Only drivers can complete orders");
    }

    // ── Parse path parameter ──────────────────────────────────────────────────
    int64_t order_id{0};
    try {
        order_id = std::stoll(request.GetPathArg("order_id"));
    } catch (const std::exception&) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Path parameter 'order_id' must be a valid integer");
    }

    if (order_id <= 0) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Path parameter 'order_id' must be a positive integer");
    }

    // ── Fetch the order to verify it exists and check assignment ──────────────
    const auto existing = db_->GetOrderById(order_id);
    if (!existing) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
        return ErrorJson("Order with id=" + std::to_string(order_id) + " not found");
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
        .Append<CreateOrderHandler>()
        .Append<GetUserOrdersHandler>()
        .Append<CompleteOrderHandler>();
}

}  // namespace taxi_service::order
