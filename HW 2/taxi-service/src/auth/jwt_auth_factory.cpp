#include "jwt_auth_factory.hpp"

namespace taxi_service::auth {

TaxiJwtAuthCheckerFactory::TaxiJwtAuthCheckerFactory(
    const userver::components::ComponentContext& context)
    : auth_component_(context.FindComponent<TaxiJwtAuthComponent>()) {}

userver::server::handlers::auth::AuthCheckerBasePtr
TaxiJwtAuthCheckerFactory::MakeAuthChecker(
    const userver::server::handlers::auth::HandlerAuthConfig& /*config*/) const {
  return auth_component_.GetChecker();
}

}  // namespace taxi_service::auth
