#include "handlers.hpp"

#include <memory>

#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "../auth/jwt_auth_checker.hpp"
#include "../common/mongo_component.hpp"
#include "../common/cache_component.hpp"
#include "../common/rate_limiter.hpp"

int main(int argc, char* argv[]) {
    userver::server::handlers::auth::RegisterAuthCheckerFactory(
        "jwt-auth", std::make_unique<taxi_service::auth::TaxiJwtAuthCheckerFactory>());

    auto component_list =
        userver::components::MinimalServerComponentList()
            .Append<userver::server::handlers::Ping>()
            .Append<userver::components::TestsuiteSupport>()
            .Append<userver::clients::dns::Component>()
            .Append<taxi::MongoComponent>("mongo-db")
            .Append<taxi_service::CacheComponent>("cache")
            .Append<taxi_service::RateLimiterComponent>("rate-limiter");

    taxi_service::input::AppendInputHandlers(component_list);

    return userver::utils::DaemonMain(argc, argv, component_list);
}
