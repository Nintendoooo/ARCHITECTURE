#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace taxi_service {

/// @brief User model for MongoDB
struct User {
    std::optional<std::string> id;  // MongoDB ObjectId as string
    std::string login;          
    std::string email;          
    std::string first_name;
    std::string last_name;
    std::string phone;
    std::string password_hash;
    std::string role;  // "passenger" or "driver"
    std::chrono::system_clock::time_point created_at;

    userver::formats::json::Value ToJson() const;
    static User FromJson(const userver::formats::json::Value& json);
};

/// @brief Driver model for MongoDB
struct Driver {
    std::optional<std::string> id;  // MongoDB ObjectId as string
    std::string user_id;  // MongoDB ObjectId as string (reference to User)
    std::string license_number;  
    std::string car_model;
    std::string car_plate;       
    bool is_available{true};     
    std::chrono::system_clock::time_point created_at;

    userver::formats::json::Value ToJson() const;
    static Driver FromJson(const userver::formats::json::Value& json);
};

/// @brief Order model for MongoDB
struct Order {
    std::optional<std::string> id;  // MongoDB ObjectId as string
    std::string user_id;  // MongoDB ObjectId as string (reference to User)
    std::optional<std::string> driver_id;  // MongoDB ObjectId as string (reference to Driver)
    std::string passenger_name;
    std::string passenger_phone;
    std::string pickup_address;
    std::string destination_address;
    std::string status;  // "active", "accepted", "completed", "cancelled"
    std::optional<double> price;
    std::chrono::system_clock::time_point created_at;
    std::optional<std::chrono::system_clock::time_point> completed_at;

    userver::formats::json::Value ToJson() const;
    static Order FromJson(const userver::formats::json::Value& json);
};

struct AuthToken {
    std::string token;
    std::string user_id;
    std::chrono::system_clock::time_point expires_at;

    userver::formats::json::Value ToJson() const;
};

struct LoginRequest {
    std::string login;
    std::string password;

    static LoginRequest FromJson(const userver::formats::json::Value& json);
};

struct CreateUserRequest {
    std::string login;
    std::string email;
    std::string first_name;
    std::string last_name;
    std::string phone;
    std::string password;

    static CreateUserRequest FromJson(const userver::formats::json::Value& json);
};

struct CreateDriverRequest {
    std::string license_number;
    std::string car_model;
    std::string car_plate;

    static CreateDriverRequest FromJson(const userver::formats::json::Value& json);
};

struct CreateOrderRequest {
    std::string pickup_address;
    std::string destination_address;

    static CreateOrderRequest FromJson(const userver::formats::json::Value& json);
};

}  // namespace taxi_service
