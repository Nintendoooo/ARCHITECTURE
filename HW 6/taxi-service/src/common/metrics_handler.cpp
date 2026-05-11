#include "metrics_handler.hpp"
#include "cache_component.hpp"
#include "rate_limiter.hpp"

#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/http/content_type.hpp>

namespace taxi_service {

MetricsHandler::MetricsHandler(const userver::components::ComponentConfig& config,
                               const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    
    // Store references to components during construction
    try {
        cache_component_ = &context.FindComponent<CacheComponent>("cache");
    } catch (const std::exception&) {
        cache_component_ = nullptr;
    }
    
    try {
        rate_limiter_component_ = &context.FindComponent<RateLimiterComponent>("rate-limiter");
    } catch (const std::exception&) {
        rate_limiter_component_ = nullptr;
    }
}

std::string MetricsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    
    auto& response = request.GetHttpResponse();
    response.SetHeader(std::string("Content-Type"), std::string("text/plain; version=0.0.4; charset=utf-8"));
    
    std::string metrics;
    
    // Get cache component metrics
    if (cache_component_) {
        metrics += cache_component_->ExportMetrics();
        metrics += "\n";
    } else {
        metrics += "# Cache component not available\n\n";
    }
    
    // Get rate limiter component metrics
    if (rate_limiter_component_) {
        metrics += rate_limiter_component_->ExportMetrics();
        metrics += "\n";
    } else {
        metrics += "# Rate limiter component not available\n\n";
    }
    
    return metrics;
}

}  // namespace taxi_service
