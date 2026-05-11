#include "jwt_auth_factory.hpp"

namespace taxi_service::auth {

userver::server::handlers::auth::AuthCheckerBasePtr
TaxiJwtAuthCheckerFactory::operator()(
    const userver::components::ComponentContext& context,
    const userver::server::handlers::auth::HandlerAuthConfig&,
    const userver::server::handlers::auth::AuthCheckerSettings&) const {
  auto& auth_component = context.FindComponent<TaxiJwtAuthComponent>();
  return auth_component.GetChecker();
}

}  // namespace taxi_service::auth
