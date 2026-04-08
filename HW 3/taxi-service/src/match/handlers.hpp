#pragma once

#include <memory>
#include <string>

#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "../auth/jwt_auth_checker.hpp"
#include "../auth/jwt_auth_factory.hpp"
#include "../common/database.hpp"
#include "../common/models.hpp"

namespace taxi_service::match {

class ListActiveOrdersHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-list-active-orders";

  ListActiveOrdersHandler(const userver::components::ComponentConfig& config,
                          const userver::components::ComponentContext& context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  std::shared_ptr<Database> db_;
};

class AcceptOrderHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-accept-order";

  AcceptOrderHandler(const userver::components::ComponentConfig& config,
                     const userver::components::ComponentContext& context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  std::shared_ptr<Database> db_;
};

void AppendMatchHandlers(userver::components::ComponentList& component_list);

}  // namespace taxi_service::match
