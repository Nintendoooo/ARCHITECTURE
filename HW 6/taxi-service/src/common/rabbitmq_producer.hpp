#pragma once

#include <string>
#include <memory>
#include <chrono>

#include <userver/utest/using_namespace_userver.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/rabbitmq.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/utils/uuid4.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "models.hpp"

namespace taxi_service {

class TaxiEventProducer final : public components::LoggableComponentBase {
public:
    static constexpr std::string_view kName{"taxi-event-producer"};

    TaxiEventProducer(const components::ComponentConfig& config,
                      const components::ComponentContext& context)
        : components::LoggableComponentBase{config, context},
          client_{context.FindComponent<components::RabbitMQ>(
                      config["rabbit_name"].As<std::string>())
                      .GetClient()} {
        const auto setup_deadline = engine::Deadline::FromDuration(std::chrono::seconds{2});

        auto admin_channel = client_->GetAdminChannel(setup_deadline);
        
        // Declare exchange
        admin_channel.DeclareExchange(exchange_, urabbitmq::Exchange::Type::kFanOut, setup_deadline);
        
        // Declare queue
        admin_channel.DeclareQueue(queue_, setup_deadline);
        
        // Bind queue to exchange
        admin_channel.BindQueue(exchange_, queue_, routing_key_, setup_deadline);
    }

    ~TaxiEventProducer() override {
        try {
            auto admin_channel = client_->GetAdminChannel(
                engine::Deadline::FromDuration(std::chrono::seconds{1}));
            const auto teardown_deadline = engine::Deadline::FromDuration(std::chrono::seconds{2});
            
            admin_channel.RemoveQueue(queue_, teardown_deadline);
            admin_channel.RemoveExchange(exchange_, teardown_deadline);
        } catch (const std::exception& e) {
            // Cleanup error - log if needed
        }
    }

    // Publish UserCreated event
    void PublishUserCreated(const User& user) {
        formats::json::ValueBuilder builder{formats::json::Type::kObject};
        builder["event_id"] = utils::generators::GenerateUuid();
        builder["event_type"] = "UserCreated";
        builder["timestamp"] = std::chrono::system_clock::now();
        builder["version"] = "1.0";
        
        auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
        data["user_id"] = user.id;
        data["login"] = user.login;
        data["email"] = user.email;
        data["first_name"] = user.first_name;
        data["last_name"] = user.last_name;
        data["phone"] = user.phone;
        data["created_at"] = user.created_at;
        
        builder["data"] = data.ExtractValue();
        
        PublishEvent(builder.ExtractValue());
    }

    // Publish DriverRegistered event
    void PublishDriverRegistered(const Driver& driver) {
        formats::json::ValueBuilder builder{formats::json::Type::kObject};
        builder["event_id"] = utils::generators::GenerateUuid();
        builder["event_type"] = "DriverRegistered";
        builder["timestamp"] = std::chrono::system_clock::now();
        builder["version"] = "1.0";
        
        auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
        data["driver_id"] = driver.id;
        data["user_id"] = driver.user_id;
        data["license_number"] = driver.license_number;
        data["car_model"] = driver.car_model;
        data["car_plate"] = driver.car_plate;
        data["created_at"] = driver.created_at;
        
        builder["data"] = data.ExtractValue();
        
        PublishEvent(builder.ExtractValue());
    }

    // Publish OrderCreated event
    void PublishOrderCreated(const Order& order) {
        formats::json::ValueBuilder builder{formats::json::Type::kObject};
        builder["event_id"] = utils::generators::GenerateUuid();
        builder["event_type"] = "OrderCreated";
        builder["timestamp"] = std::chrono::system_clock::now();
        builder["version"] = "1.0";
        
        auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
        data["order_id"] = order.id;
        data["user_id"] = order.user_id;
        data["pickup_address"] = order.pickup_address;
        data["destination_address"] = order.destination_address;
        data["status"] = order.status;
        data["created_at"] = order.created_at;
        
        builder["data"] = data.ExtractValue();
        
        PublishEvent(builder.ExtractValue());
    }

    // Publish OrderAccepted event
    void PublishOrderAccepted(const Order& order, const std::string& driver_id) {
        formats::json::ValueBuilder builder{formats::json::Type::kObject};
        builder["event_id"] = utils::generators::GenerateUuid();
        builder["event_type"] = "OrderAccepted";
        builder["timestamp"] = std::chrono::system_clock::now();
        builder["version"] = "1.0";
        
        auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
        data["order_id"] = order.id;
        data["driver_id"] = driver_id;
        data["user_id"] = order.user_id;
        data["pickup_address"] = order.pickup_address;
        data["destination_address"] = order.destination_address;
        data["accepted_at"] = std::chrono::system_clock::now();
        
        builder["data"] = data.ExtractValue();
        
        PublishEvent(builder.ExtractValue());
    }

    // Publish OrderCompleted event
    void PublishOrderCompleted(const Order& order) {
        formats::json::ValueBuilder builder{formats::json::Type::kObject};
        builder["event_id"] = utils::generators::GenerateUuid();
        builder["event_type"] = "OrderCompleted";
        builder["timestamp"] = std::chrono::system_clock::now();
        builder["version"] = "1.0";
        
        auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
        data["order_id"] = order.id;
        data["driver_id"] = order.driver_id;
        data["user_id"] = order.user_id;
        data["pickup_address"] = order.pickup_address;
        data["destination_address"] = order.destination_address;
        data["completed_at"] = std::chrono::system_clock::now();
        
        builder["data"] = data.ExtractValue();
        
        PublishEvent(builder.ExtractValue());
    }

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Taxi Event Producer component for RabbitMQ
additionalProperties: false
properties:
    rabbit_name:
        type: string
        description: name of RabbitMQ client component
        )");
    }

private:
    void PublishEvent(const formats::json::Value& event) {
        try {
            const std::string message = formats::json::ToString(event);
            
            client_->PublishReliable(
                exchange_,
                routing_key_,
                message,
                urabbitmq::MessageType::kTransient,
                engine::Deadline::FromDuration(std::chrono::seconds{2})
            );
        } catch (const std::exception& e) {
            throw;
        }
    }

    const urabbitmq::Exchange exchange_{"taxi-events"};
    const urabbitmq::Queue queue_{"taxi-events-queue"};
    const std::string routing_key_ = "taxi-routing-key";

    std::shared_ptr<urabbitmq::Client> client_;
};

}  // namespace taxi_service
