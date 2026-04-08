#include "handlers.hpp"

#include <string>

#include <userver/components/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/component.hpp>

namespace taxi_service::match {

namespace {

std::string ErrorJson(const std::string& message) {
    return userver::formats::json::ToString(
        userver::formats::json::MakeObject("error", message));
}

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
    : HttpHandlerBase(config, context),
      db_(std::make_shared<Database>(
          context.FindComponent<userver::components::Postgres>("postgres-db")
              .GetCluster())) {}

std::string ListActiveOrdersHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {

    std::string auth_error;
    const int64_t caller_id = ExtractUserId(request, context, auth_error);
    if (caller_id == 0) {
        return ErrorJson(auth_error);
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

    const auto orders = db_->GetActiveOrders();

    userver::formats::json::ValueBuilder arr(
        userver::formats::json::Type::kArray);
    for (const auto& o : orders) {
        arr.PushBack(o.ToJson());
    }

    LOG_DEBUG() << "[match] ListActiveOrders returned " << orders.size()
                << " order(s) (caller_id=" << caller_id
                << " role=" << ExtractRole(context) << ")";

    return userver::formats::json::ToString(arr.ExtractValue());
}

AcceptOrderHandler::AcceptOrderHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      db_(std::make_shared<Database>(
          context.FindComponent<userver::components::Postgres>("postgres-db")
              .GetCluster())) {}

std::string AcceptOrderHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {

    std::string auth_error;
    const int64_t caller_id = ExtractUserId(request, context, auth_error);
    if (caller_id == 0) {
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

    const auto existing = db_->GetOrderById(order_id);
    if (!existing) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
        return ErrorJson("Order with id=" + std::to_string(order_id) + " not found");
    }

    const auto accepted = db_->AcceptOrder(order_id, *driver_record->id);
    if (!accepted) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson(
            "Order cannot be accepted: current status is '" +
            existing->status + "' (must be 'active')");
    }

    LOG_INFO() << "[match] Accepted order id=" << order_id
               << " driver_id=" << *driver_record->id
               << " user_id=" << existing->user_id;

    return userver::formats::json::ToString(accepted->ToJson());
}

void AppendMatchHandlers(userver::components::ComponentList& component_list) {
    component_list
        .Append<auth::TaxiJwtAuthComponent>()
        .Append<ListActiveOrdersHandler>()
        .Append<AcceptOrderHandler>();
}

}  // namespace taxi_service::match
