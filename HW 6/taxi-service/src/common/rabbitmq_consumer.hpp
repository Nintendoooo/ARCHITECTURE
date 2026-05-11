#pragma once

#include <string>
#include <vector>
#include <memory>

#include <userver/rabbitmq/consumer_component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/parse.hpp>

namespace taxi_service {

class TaxiEventConsumer final : public userver::urabbitmq::ConsumerComponentBase {
public:
    static constexpr std::string_view kName{"taxi-event-consumer"};

    TaxiEventConsumer(const userver::components::ComponentConfig& config,
                      const userver::components::ComponentContext& context)
        : userver::urabbitmq::ConsumerComponentBase{config, context} {
        GetLogger()->info("TaxiEventConsumer initialized");
    }

    // Get all consumed messages (for testing)
    std::vector<std::string> GetConsumedMessages() {
        auto storage = storage_.Lock();
        auto messages = *storage;
        return messages;
    }

    // Get consumed messages count
    size_t GetConsumedMessagesCount() {
        auto storage = storage_.Lock();
        return storage->size();
    }

    // Clear consumed messages (for testing)
    void ClearConsumedMessages() {
        auto storage = storage_.Lock();
        storage->clear();
    }

protected:
    void Process(std::string message) override {
        try {
            // Parse JSON message
            auto json = userver::formats::json::FromString(message);
            
            // Store message for testing
            {
                auto storage = storage_.Lock();
                storage->push_back(message);
            }
            
            // Log event
            std::string event_type = "Unknown";
            if (json.HasMember("event_type")) {
                event_type = json["event_type"].As<std::string>();
            }
            
            GetLogger()->info("Consumed event: type='{}', message_count={}", 
                            event_type, GetConsumedMessagesCount());
            
            // Process specific event types
            ProcessEvent(json);
            
        } catch (const std::exception& e) {
            GetLogger()->error("Error processing message: {}", e.what());
            throw;  // Return message to queue for retry
        }
    }

private:
    void ProcessEvent(const userver::formats::json::Value& event) {
        if (!event.HasMember("event_type")) {
            return;
        }
        
        std::string event_type = event["event_type"].As<std::string>();
        
        if (event_type == "UserCreated") {
            ProcessUserCreated(event);
        } else if (event_type == "DriverRegistered") {
            ProcessDriverRegistered(event);
        } else if (event_type == "OrderCreated") {
            ProcessOrderCreated(event);
        } else if (event_type == "OrderAccepted") {
            ProcessOrderAccepted(event);
        } else if (event_type == "OrderCompleted") {
            ProcessOrderCompleted(event);
        } else {
            GetLogger()->warn("Unknown event type: {}", event_type);
        }
    }

    void ProcessUserCreated(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string user_id = data["user_id"].As<std::string>();
            std::string login = data["login"].As<std::string>();
            
            GetLogger()->info("Processing UserCreated: user_id='{}', login='{}'", user_id, login);
            
            // Here you would implement actual business logic
            // For example: send welcome notification, update analytics, etc.
        } catch (const std::exception& e) {
            GetLogger()->error("Error processing UserCreated: {}", e.what());
            throw;
        }
    }

    void ProcessDriverRegistered(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string driver_id = data["driver_id"].As<std::string>();
            std::string user_id = data["user_id"].As<std::string>();
            std::string car_plate = data["car_plate"].As<std::string>();
            
            GetLogger()->info("Processing DriverRegistered: driver_id='{}', user_id='{}', car_plate='{}'", 
                            driver_id, user_id, car_plate);
            
            // Here you would implement actual business logic
            // For example: update analytics, notify services, etc.
        } catch (const std::exception& e) {
            GetLogger()->error("Error processing DriverRegistered: {}", e.what());
            throw;
        }
    }

    void ProcessOrderCreated(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string order_id = data["order_id"].As<std::string>();
            std::string user_id = data["user_id"].As<std::string>();
            std::string pickup_address = data["pickup_address"].As<std::string>();
            
            GetLogger()->info("Processing OrderCreated: order_id='{}', user_id='{}', pickup_address='{}'", 
                            order_id, user_id, pickup_address);
            
            // Here you would implement actual business logic
            // For example: send notification, update analytics, trigger matching, etc.
        } catch (const std::exception& e) {
            GetLogger()->error("Error processing OrderCreated: {}", e.what());
            throw;
        }
    }

    void ProcessOrderAccepted(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string order_id = data["order_id"].As<std::string>();
            std::string driver_id = data["driver_id"].As<std::string>();
            
            GetLogger()->info("Processing OrderAccepted: order_id='{}', driver_id='{}'", 
                            order_id, driver_id);
            
            // Here you would implement actual business logic
            // For example: send notification to passenger, update analytics, etc.
        } catch (const std::exception& e) {
            GetLogger()->error("Error processing OrderAccepted: {}", e.what());
            throw;
        }
    }

    void ProcessOrderCompleted(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string order_id = data["order_id"].As<std::string>();
            std::string driver_id = data["driver_id"].As<std::string>();
            std::string user_id = data["user_id"].As<std::string>();
            
            GetLogger()->info("Processing OrderCompleted: order_id='{}', driver_id='{}', user_id='{}'", 
                            order_id, driver_id, user_id);
            
            // Here you would implement actual business logic
            // For example: update analytics, billing, audit log, etc.
        } catch (const std::exception& e) {
            GetLogger()->error("Error processing OrderCompleted: {}", e.what());
            throw;
        }
    }

    userver::concurrent::Variable<std::vector<std::string>> storage_;
};

}  // namespace taxi_service
