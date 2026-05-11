#pragma once

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/components/component_context.hpp>

#include <string>

namespace taxi_service {

// Forward declarations
class CacheComponent;
class RateLimiterComponent;

class MetricsHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "metrics";

    MetricsHandler(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    CacheComponent* cache_component_;
    RateLimiterComponent* rate_limiter_component_;
};

}  // namespace taxi_service
