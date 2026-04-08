/// @file main.cpp
/// @brief Entry point for the Order Service (taxi-service-order).
///
/// Responsibilities:
///   - Ride order creation    (POST /api/orders)
///   - Trip history retrieval (GET  /api/users/{user_id}/orders)
///   - Trip completion        (POST /api/orders/{order_id}/complete)
///
/// The service listens on port 8082 (configured in static_config_order.yaml).
/// All three endpoints require a valid Bearer JWT token issued by the
/// Input Service login endpoint.

#include "handlers.hpp"

#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
    // Register the JWT auth checker factory globally before building the
    // component list. This makes the "taxi-jwt" auth type available to handlers.
    userver::server::handlers::auth::RegisterAuthCheckerFactory<
        taxi_service::auth::TaxiJwtAuthCheckerFactory>();

    // Start with the minimal userver component set (server, logging,
    // dynamic-config, etc.) and add a /ping liveness probe.
    auto component_list =
        userver::components::MinimalServerComponentList()
            .Append<userver::server::handlers::Ping>()
            .Append<userver::components::TestsuiteSupport>()
            .Append<userver::clients::dns::Component>()
            .Append<userver::components::Postgres>("postgres-db");

    // Register all Order Service handlers plus the JWT auth components.
    taxi_service::order::AppendOrderHandlers(component_list);

    return userver::utils::DaemonMain(argc, argv, component_list);
}
