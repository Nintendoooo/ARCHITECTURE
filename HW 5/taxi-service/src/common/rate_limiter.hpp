#pragma once

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#include <string>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <atomic>

namespace taxi_service {

// Token Bucket implementation for rate limiting
class TokenBucket {
public:
    TokenBucket(size_t capacity, size_t refill_rate, std::chrono::seconds window)
        : capacity_(capacity)
        , refill_rate_(refill_rate)
        , window_(window)
        , tokens_(capacity)
        , last_refill_(std::chrono::system_clock::now()) {}

    bool TryConsume(size_t tokens = 1) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        Refill();
        
        if (tokens_ >= tokens) {
            tokens_ -= tokens;
            return true;
        }
        
        return false;
    }

    size_t GetAvailableTokens() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return tokens_;
    }

    std::chrono::system_clock::time_point GetLastRefill() const {
        return last_refill_;
    }

    std::chrono::seconds GetWindow() const {
        return window_;
    }

    size_t GetCapacity() const {
        return capacity_;
    }

private:
    void Refill() {
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_refill_);
        
        if (elapsed.count() > 0) {
            // Calculate tokens to add based on elapsed time
            size_t tokens_to_add = (elapsed.count() * refill_rate_) / window_.count();
            
            tokens_ = std::min(capacity_, tokens_ + tokens_to_add);
            last_refill_ = now;
        }
    }

    mutable std::mutex mutex_;
    size_t capacity_;
    size_t refill_rate_;
    std::chrono::seconds window_;
    size_t tokens_;
    std::chrono::system_clock::time_point last_refill_;
};

// Rate limiter configuration
struct RateLimitConfig {
    size_t limit;           // Maximum requests per window
    std::chrono::seconds window;  // Time window
    size_t burst;           // Burst capacity
};

// Rate limiter component
class RateLimiterComponent : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "rate-limiter";

    RateLimiterComponent(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context)
        : ComponentBase(config, context) {
        
        // Configure rate limits from config
        user_registration_limit_ = {
            config["user_registration_limit"].As<size_t>(10),
            std::chrono::seconds(config["user_registration_window"].As<int>(60)),
            config["user_registration_burst"].As<size_t>(5)
        };
        
        driver_registration_limit_ = {
            config["driver_registration_limit"].As<size_t>(10),
            std::chrono::seconds(config["driver_registration_window"].As<int>(60)),
            config["driver_registration_burst"].As<size_t>(5)
        };
        
        order_creation_limit_ = {
            config["order_creation_limit"].As<size_t>(20),
            std::chrono::seconds(config["order_creation_window"].As<int>(60)),
            config["order_creation_burst"].As<size_t>(10)
        };
        
        active_orders_limit_ = {
            config["active_orders_limit"].As<size_t>(100),
            std::chrono::seconds(config["active_orders_window"].As<int>(60)),
            config["active_orders_burst"].As<size_t>(20)
        };
        
        general_limit_ = {
            config["general_limit"].As<size_t>(1000),
            std::chrono::seconds(config["general_window"].As<int>(60)),
            config["general_burst"].As<size_t>(100)
        };
        
        // Start cleanup task
        cleanup_interval_ = std::chrono::seconds(config["cleanup_interval"].As<int>(300));
    }

    // Check rate limit for user registration
    bool CheckUserRegistration(const std::string& client_ip) {
        return CheckRateLimit("user_reg:" + client_ip, user_registration_limit_);
    }

    // Check rate limit for driver registration
    bool CheckDriverRegistration(const std::string& client_ip) {
        return CheckRateLimit("driver_reg:" + client_ip, driver_registration_limit_);
    }

    // Check rate limit for order creation
    bool CheckOrderCreation(const std::string& user_id) {
        return CheckRateLimit("order_create:" + user_id, order_creation_limit_);
    }

    // Check rate limit for active orders
    bool CheckActiveOrders(const std::string& driver_id) {
        return CheckRateLimit("active_orders:" + driver_id, active_orders_limit_);
    }

    // Check general rate limit
    bool CheckGeneral(const std::string& client_ip) {
        return CheckRateLimit("general:" + client_ip, general_limit_);
    }

    // Get rate limit info for headers
    struct RateLimitInfo {
        size_t limit;
        size_t remaining;
        std::chrono::system_clock::time_point reset;
    };

    RateLimitInfo GetRateLimitInfo(const std::string& key, const RateLimitConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = buckets_.find(key);
        if (it == buckets_.end()) {
            return {config.limit, config.limit, std::chrono::system_clock::now() + config.window};
        }
        
        auto available = it->second->GetAvailableTokens();
        auto last_refill = it->second->GetLastRefill();
        auto reset = last_refill + config.window;
        
        return {config.limit, available, reset};
    }

    // Cleanup old buckets
    void Cleanup() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto it = buckets_.begin();
        
        while (it != buckets_.end()) {
            auto last_refill = it->second->GetLastRefill();
            auto window = it->second->GetWindow();
            
            // Remove buckets that haven't been used for 2x their window
            if (now - last_refill > window * 2) {
                it = buckets_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Get statistics
    struct RateLimiterStats {
        std::atomic<uint64_t> total_requests{0};
        std::atomic<uint64_t> rate_limited_requests{0};
        std::atomic<uint64_t> allowed_requests{0};
        std::atomic<size_t> active_buckets{0};
        
        uint64_t GetTotalRequests() const { return total_requests.load(); }
        uint64_t GetRateLimitedRequests() const { return rate_limited_requests.load(); }
        uint64_t GetAllowedRequests() const { return allowed_requests.load(); }
        size_t GetActiveBuckets() const { return active_buckets.load(); }
    };

    const RateLimiterStats& GetStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_.active_buckets = buckets_.size();
        return stats_;
    }
    
    // Export metrics in Prometheus format
    std::string ExportMetrics() const {
        std::string metrics;
        const auto& stats = GetStats();
        
        metrics += "# HELP rate_limit_requests_total Total number of rate limit checks\n";
        metrics += "# TYPE rate_limit_requests_total counter\n";
        metrics += "rate_limit_requests_total " + std::to_string(stats.GetTotalRequests()) + "\n\n";
        
        metrics += "# HELP rate_limit_allowed_requests_total Number of requests allowed by rate limiter\n";
        metrics += "# TYPE rate_limit_allowed_requests_total counter\n";
        metrics += "rate_limit_allowed_requests_total " + std::to_string(stats.GetAllowedRequests()) + "\n\n";
        
        metrics += "# HELP rate_limit_denied_requests_total Number of requests denied by rate limiter\n";
        metrics += "# TYPE rate_limit_denied_requests_total counter\n";
        metrics += "rate_limit_denied_requests_total " + std::to_string(stats.GetRateLimitedRequests()) + "\n\n";
        
        metrics += "# HELP rate_limit_active_buckets Current number of active rate limit buckets\n";
        metrics += "# TYPE rate_limit_active_buckets gauge\n";
        metrics += "rate_limit_active_buckets " + std::to_string(stats.GetActiveBuckets()) + "\n";
        
        return metrics;
    }

private:
    bool CheckRateLimit(const std::string& key, const RateLimitConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        stats_.total_requests++;
        
        auto it = buckets_.find(key);
        if (it == buckets_.end()) {
            // Create new bucket
            buckets_[key] = std::make_unique<TokenBucket>(
                config.burst, config.limit, config.window);
            it = buckets_.find(key);
        }
        
        bool allowed = it->second->TryConsume(1);
        
        if (allowed) {
            stats_.allowed_requests++;
        } else {
            stats_.rate_limited_requests++;
        }
        
        return allowed;
    }

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<TokenBucket>> buckets_;
    
    RateLimitConfig user_registration_limit_;
    RateLimitConfig driver_registration_limit_;
    RateLimitConfig order_creation_limit_;
    RateLimitConfig active_orders_limit_;
    RateLimitConfig general_limit_;
    
    std::chrono::seconds cleanup_interval_;
    
    mutable RateLimiterStats stats_;
};

// Helper to set rate limit headers
inline void SetRateLimitHeaders(
    userver::server::http::HttpResponse& response,
    const RateLimiterComponent::RateLimitInfo& info) {
    
    response.SetHeader(std::string{"X-RateLimit-Limit"}, std::to_string(info.limit));
    response.SetHeader(std::string{"X-RateLimit-Remaining"}, std::to_string(info.remaining));
    
    auto reset_time = std::chrono::system_clock::to_time_t(info.reset);
    response.SetHeader(std::string{"X-RateLimit-Reset"}, std::to_string(reset_time));
}

}  // namespace taxi_service
