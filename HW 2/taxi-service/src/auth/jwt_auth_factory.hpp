#pragma once

#include <userver/server/handlers/auth/auth_checker_factory.hpp>

#include "jwt_auth_checker.hpp"

namespace taxi_service::auth {

class TaxiJwtAuthCheckerFactory final
    : public userver::server::handlers::auth::AuthCheckerFactoryBase {
 public:
  static constexpr const char* kAuthType = "taxi-jwt";

  explicit TaxiJwtAuthCheckerFactory(
      const userver::components::ComponentContext& context);

  userver::server::handlers::auth::AuthCheckerBasePtr MakeAuthChecker(
      const userver::server::handlers::auth::HandlerAuthConfig& config)
      const override;

 private:
  TaxiJwtAuthComponent& auth_component_;
};

}  // namespace taxi_service::auth
