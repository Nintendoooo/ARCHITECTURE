#pragma once

#include <userver/server/handlers/auth/auth_checker_factory.hpp>

#include "jwt_auth_checker.hpp"

namespace taxi_service::auth {

class TaxiJwtAuthCheckerFactory final
    : public userver::server::handlers::auth::AuthCheckerFactoryBase {
 public:
      userver::server::handlers::auth::AuthCheckerBasePtr operator()(
      const userver::components::ComponentContext& context,
      const userver::server::handlers::auth::HandlerAuthConfig&,
      const userver::server::handlers::auth::AuthCheckerSettings&) const override;
};

}  // namespace taxi_service::auth
