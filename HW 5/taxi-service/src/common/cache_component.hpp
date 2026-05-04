#pragma once

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#include <string>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <optional>
#include <atomic>

namespace taxi_service {

// Simple in-memory cache implementation with TTL
template <typename Key, typename Value>
class InMemoryCache {
public:
    struct CacheEntry {
        Value value;
        std::chrono::system_clock::time_point expiry_time;
        
        CacheEntry() = default;
        
        CacheEntry(const Value& v, std::chrono::seconds ttl)
            : value(v), expiry_time(std::chrono::system_clock::now() + ttl) {}
    };

    InMemoryCache(size_t max_size, std::chrono::seconds default_ttl)
        : max_size_(max_size), default_ttl_(default_ttl) {}

    std::optional<Value> Get(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return std::nullopt;
        }
        
        // Check if entry has expired
        if (std::chrono::system_clock::now() > it->second.expiry_time) {
            cache_.erase(it);
            return std::nullopt;
        }
        
        return it->second.value;
    }

    void Put(const Key& key, const Value& value, std::optional<std::chrono::seconds> ttl = std::nullopt) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto actual_ttl = ttl.value_or(default_ttl_);
        cache_[key] = CacheEntry(value, actual_ttl);
        
        // Evict oldest entries if cache is too large
        while (cache_.size() > max_size_) {
            EvictOldest();
        }
    }

    void Invalidate(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.erase(key);
    }

    void InvalidatePattern(const std::string& pattern) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.begin();
        while (it != cache_.end()) {
            if (it->first.find(pattern) != std::string::npos) {
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    size_t Size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
    }

private:
    void EvictOldest() {
        if (cache_.empty()) return;
        
        auto oldest_it = cache_.begin();
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.expiry_time < oldest_it->second.expiry_time) {
                oldest_it = it;
            }
        }
        cache_.erase(oldest_it);
    }
    
    void RecordEviction() {
        // This method should be called when an eviction occurs
        // The actual tracking is done in the CacheStats
    }

    mutable std::mutex mutex_;
    std::unordered_map<Key, CacheEntry> cache_;
    size_t max_size_;
    std::chrono::seconds default_ttl_;
};

// Cache statistics with atomic counters for thread safety
struct CacheStats {
    std::atomic<uint64_t> hits{0};
    std::atomic<uint64_t> misses{0};
    std::atomic<uint64_t> evictions{0};
    
    double HitRate() const {
        uint64_t total_hits = hits.load();
        uint64_t total_misses = misses.load();
        if (total_hits + total_misses == 0) return 0.0;
        return static_cast<double>(total_hits) / (total_hits + total_misses) * 100.0;
    }
    
    void RecordHit() { hits++; }
    void RecordMiss() { misses++; }
    void RecordEviction() { evictions++; }
    
    uint64_t GetHits() const { return hits.load(); }
    uint64_t GetMisses() const { return misses.load(); }
    uint64_t GetEvictions() const { return evictions.load(); }
};

// Cache component for Userver
class CacheComponent : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "cache";

    CacheComponent(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context)
        : ComponentBase(config, context) {
        
        // Configure caches based on config
        auto user_cache_size = config["user_cache_size"].As<size_t>(10000);
        auto user_cache_ttl = std::chrono::seconds(config["user_cache_ttl"].As<int>(300));
        
        auto driver_cache_size = config["driver_cache_size"].As<size_t>(5000);
        auto driver_cache_ttl = std::chrono::seconds(config["driver_cache_ttl"].As<int>(300));
        
        auto order_cache_size = config["order_cache_size"].As<size_t>(1000);
        auto order_cache_ttl = std::chrono::seconds(config["order_cache_ttl"].As<int>(30));
        
        user_cache_ = std::make_shared<InMemoryCache<std::string, std::string>>(
            user_cache_size, user_cache_ttl);
        
        driver_cache_ = std::make_shared<InMemoryCache<std::string, std::string>>(
            driver_cache_size, driver_cache_ttl);
        
        order_cache_ = std::make_shared<InMemoryCache<std::string, std::string>>(
            order_cache_size, order_cache_ttl);
    }

    std::shared_ptr<InMemoryCache<std::string, std::string>> GetUserCache() const {
        return user_cache_;
    }

    std::shared_ptr<InMemoryCache<std::string, std::string>> GetDriverCache() const {
        return driver_cache_;
    }

    std::shared_ptr<InMemoryCache<std::string, std::string>> GetOrderCache() const {
        return order_cache_;
    }

    CacheStats& GetUserCacheStats() { return user_cache_stats_; }
    CacheStats& GetDriverCacheStats() { return driver_cache_stats_; }
    CacheStats& GetOrderCacheStats() { return order_cache_stats_; }
    
    // Export metrics in Prometheus format
    std::string ExportMetrics() const {
        std::string metrics;
        
        // User cache metrics
        metrics += "# HELP cache_hits_total Total number of cache hits\n";
        metrics += "# TYPE cache_hits_total counter\n";
        metrics += "cache_hits_total{cache=\"user\"} " + std::to_string(user_cache_stats_.GetHits()) + "\n";
        metrics += "cache_hits_total{cache=\"driver\"} " + std::to_string(driver_cache_stats_.GetHits()) + "\n";
        metrics += "cache_hits_total{cache=\"order\"} " + std::to_string(order_cache_stats_.GetHits()) + "\n\n";
        
        metrics += "# HELP cache_misses_total Total number of cache misses\n";
        metrics += "# TYPE cache_misses_total counter\n";
        metrics += "cache_misses_total{cache=\"user\"} " + std::to_string(user_cache_stats_.GetMisses()) + "\n";
        metrics += "cache_misses_total{cache=\"driver\"} " + std::to_string(driver_cache_stats_.GetMisses()) + "\n";
        metrics += "cache_misses_total{cache=\"order\"} " + std::to_string(order_cache_stats_.GetMisses()) + "\n\n";
        
        metrics += "# HELP cache_evictions_total Total number of cache evictions\n";
        metrics += "# TYPE cache_evictions_total counter\n";
        metrics += "cache_evictions_total{cache=\"user\"} " + std::to_string(user_cache_stats_.GetEvictions()) + "\n";
        metrics += "cache_evictions_total{cache=\"driver\"} " + std::to_string(driver_cache_stats_.GetEvictions()) + "\n";
        metrics += "cache_evictions_total{cache=\"order\"} " + std::to_string(order_cache_stats_.GetEvictions()) + "\n\n";
        
        metrics += "# HELP cache_hit_rate Cache hit rate percentage\n";
        metrics += "# TYPE cache_hit_rate gauge\n";
        metrics += "cache_hit_rate{cache=\"user\"} " + std::to_string(user_cache_stats_.HitRate()) + "\n";
        metrics += "cache_hit_rate{cache=\"driver\"} " + std::to_string(driver_cache_stats_.HitRate()) + "\n";
        metrics += "cache_hit_rate{cache=\"order\"} " + std::to_string(order_cache_stats_.HitRate()) + "\n\n";
        
        metrics += "# HELP cache_size Current cache size\n";
        metrics += "# TYPE cache_size gauge\n";
        metrics += "cache_size{cache=\"user\"} " + std::to_string(user_cache_->Size()) + "\n";
        metrics += "cache_size{cache=\"driver\"} " + std::to_string(driver_cache_->Size()) + "\n";
        metrics += "cache_size{cache=\"order\"} " + std::to_string(order_cache_->Size()) + "\n";
        
        return metrics;
    }

private:
    std::shared_ptr<InMemoryCache<std::string, std::string>> user_cache_;
    std::shared_ptr<InMemoryCache<std::string, std::string>> driver_cache_;
    std::shared_ptr<InMemoryCache<std::string, std::string>> order_cache_;
    
    CacheStats user_cache_stats_;
    CacheStats driver_cache_stats_;
    CacheStats order_cache_stats_;
};

}  // namespace taxi_service
