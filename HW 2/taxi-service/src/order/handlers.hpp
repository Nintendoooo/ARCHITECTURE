#pragma once

#include <memory>
#include <string>

#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "../auth/jwt_auth_checker.hpp"
#include "../auth/jwt_auth_factory.hpp"
#include "../common/database.hpp"
#include "../common/models.hpp"

namespace taxi_service::order {

// ─────────────────────────────────────────────────────────────────────────────
//  CreateOrderHandler
//
//  POST /api/orders
//  Creates a new ride order for the authenticated passenger.
//  The order is placed in 'active' status with no driver assigned yet.
//  Requires Bearer JWT authentication (user_id extracted from token context).
//  Returns 201 with the created Order object.
//  Errors: 400 (missing/empty fields), 401 (unauthenticated).
// ─────────────────────────────────────────────────────────────────────────────

class CreateOrderHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-create-order";

  CreateOrderHandler(const userver::components::ComponentConfig& config,
                     const userver::components::ComponentContext& context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  std::shared_ptr<Database> db_;
};

// ─────────────────────────────────────────────────────────────────────────────
//  GetUserOrdersHandler
//
//  GET /api/users/{user_id}/orders
//  Returns the full trip history for the specified passenger, newest first.
//  Requires Bearer JWT authentication.
//  Access control: a passenger may only view their own history (user_id in
//  path must match the authenticated user_id from the token).
//  Errors: 401 (unauthenticated), 403 (accessing another user's history).
// ─────────────────────────────────────────────────────────────────────────────

class GetUserOrdersHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-get-user-orders";

  GetUserOrdersHandler(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  std::shared_ptr<Database> db_;
};

// ─────────────────────────────────────────────────────────────────────────────
//  CompleteOrderHandler
//
//  POST /api/orders/{order_id}/complete
//  Transitions an order from 'accepted' → 'completed' and records the
//  completion timestamp.
//  Only the driver who accepted the order may complete it.
//  Requires Bearer JWT authentication with role = "driver".
//  Errors: 400 (order not in 'accepted' status), 401 (unauthenticated),
//          403 (caller is not the assigned driver), 404 (order not found).
// ─────────────────────────────────────────────────────────────────────────────

class CompleteOrderHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-complete-order";

  CompleteOrderHandler(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  std::shared_ptr<Database> db_;
};

// ─────────────────────────────────────────────────────────────────────────────
//  AppendOrderHandlers
//
//  Registers all Order Service handlers and auth components into the given
//  ComponentList. Called from main.cpp.
// ─────────────────────────────────────────────────────────────────────────────

void AppendOrderHandlers(userver::components::ComponentList& component_list);

}  // namespace taxi_service::order
