#include "models.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/utils/datetime.hpp>

namespace taxi_service {

userver::formats::json::Value User::ToJson() const {
    userver::formats::json::ValueBuilder b;
    if (id) {
        b["id"] = *id;
    }
    b["login"]      = login;
    b["email"]      = email;
    b["first_name"] = first_name;
    b["last_name"]  = last_name;
    b["phone"]      = phone;
    b["created_at"] = userver::utils::datetime::Timestring(created_at);
    return b.ExtractValue();
}

User User::FromJson(const userver::formats::json::Value& json) {
    User u;
    if (json.HasMember("id")) {
        u.id = json["id"].As<std::string>();
    }
    u.login      = json["login"].As<std::string>();
    u.email      = json["email"].As<std::string>();
    u.first_name = json["first_name"].As<std::string>();
    u.last_name  = json["last_name"].As<std::string>();
    u.phone      = json["phone"].As<std::string>();
    if (json.HasMember("password")) {
        u.password_hash = json["password"].As<std::string>();
    }
    return u;
}

userver::formats::json::Value Driver::ToJson() const {
    userver::formats::json::ValueBuilder b;
    if (id) {
        b["id"] = *id;
    }
    b["user_id"]        = user_id;
    b["license_number"] = license_number;
    b["car_model"]      = car_model;
    b["car_plate"]      = car_plate;
    b["is_available"]   = is_available;
    b["created_at"]     = userver::utils::datetime::Timestring(created_at);
    return b.ExtractValue();
}

Driver Driver::FromJson(const userver::formats::json::Value& json) {
    Driver d;
    if (json.HasMember("id")) {
        d.id = json["id"].As<std::string>();
    }
    if (json.HasMember("user_id")) {
        d.user_id = json["user_id"].As<std::string>();
    }
    d.license_number = json["license_number"].As<std::string>();
    d.car_model      = json["car_model"].As<std::string>();
    d.car_plate      = json["car_plate"].As<std::string>();
    d.is_available   = json.HasMember("is_available")
                           ? json["is_available"].As<bool>()
                           : true;
    return d;
}

userver::formats::json::Value Order::ToJson() const {
    userver::formats::json::ValueBuilder b;
    if (id) {
        b["id"] = *id;
    }
    b["user_id"] = user_id;

    if (driver_id) {
        b["driver_id"] = *driver_id;
    } else {
        b["driver_id"] = userver::formats::json::ValueBuilder{}.ExtractValue();  // null
    }

    b["pickup_address"]      = pickup_address;
    b["destination_address"] = destination_address;
    b["status"]              = status;

    if (price) {
        b["price"] = *price;
    } else {
        b["price"] = userver::formats::json::ValueBuilder{}.ExtractValue();  // null
    }

    b["created_at"] = userver::utils::datetime::Timestring(created_at);

    if (completed_at) {
        b["completed_at"] = userver::utils::datetime::Timestring(*completed_at);
    } else {
        b["completed_at"] = userver::formats::json::ValueBuilder{}.ExtractValue();  // null
    }

    return b.ExtractValue();
}

Order Order::FromJson(const userver::formats::json::Value& json) {
    Order o;
    if (json.HasMember("id")) {
        o.id = json["id"].As<std::string>();
    }
    if (json.HasMember("user_id")) {
        o.user_id = json["user_id"].As<std::string>();
    }
    if (json.HasMember("driver_id") && !json["driver_id"].IsNull()) {
        o.driver_id = json["driver_id"].As<std::string>();
    }
    o.pickup_address      = json["pickup_address"].As<std::string>();
    o.destination_address = json["destination_address"].As<std::string>();
    o.status = json.HasMember("status") ? json["status"].As<std::string>() : "active";
    if (json.HasMember("price") && !json["price"].IsNull()) {
        o.price = json["price"].As<double>();
    }
    return o;
}

userver::formats::json::Value AuthToken::ToJson() const {
    userver::formats::json::ValueBuilder b;
    b["token"]      = token;
    b["user_id"]    = user_id;
    b["expires_at"] = userver::utils::datetime::Timestring(expires_at);
    return b.ExtractValue();
}

LoginRequest LoginRequest::FromJson(const userver::formats::json::Value& json) {
    if (!json.HasMember("login")) {
        throw std::runtime_error("Missing required field: login");
    }
    if (!json.HasMember("password")) {
        throw std::runtime_error("Missing required field: password");
    }
    LoginRequest r;
    r.login    = json["login"].As<std::string>();
    r.password = json["password"].As<std::string>();
    return r;
}

CreateUserRequest CreateUserRequest::FromJson(const userver::formats::json::Value& json) {
    for (const auto& field : {"login", "email", "first_name", "last_name", "phone", "password"}) {
        if (!json.HasMember(field)) {
            throw std::runtime_error(std::string("Missing required field: ") + field);
        }
    }
    CreateUserRequest r;
    r.login      = json["login"].As<std::string>();
    r.email      = json["email"].As<std::string>();
    r.first_name = json["first_name"].As<std::string>();
    r.last_name  = json["last_name"].As<std::string>();
    r.phone      = json["phone"].As<std::string>();
    r.password   = json["password"].As<std::string>();
    return r;
}

CreateDriverRequest CreateDriverRequest::FromJson(const userver::formats::json::Value& json) {
    for (const auto& field : {"license_number", "car_model", "car_plate"}) {
        if (!json.HasMember(field)) {
            throw std::runtime_error(std::string("Missing required field: ") + field);
        }
    }
    CreateDriverRequest r;
    r.license_number = json["license_number"].As<std::string>();
    r.car_model      = json["car_model"].As<std::string>();
    r.car_plate      = json["car_plate"].As<std::string>();
    return r;
}

CreateOrderRequest CreateOrderRequest::FromJson(const userver::formats::json::Value& json) {
    for (const auto& field : {"pickup_address", "destination_address"}) {
        if (!json.HasMember(field)) {
            throw std::runtime_error(std::string("Missing required field: ") + field);
        }
    }
    CreateOrderRequest r;
    r.pickup_address      = json["pickup_address"].As<std::string>();
    r.destination_address = json["destination_address"].As<std::string>();
    return r;
}

}  // namespace taxi_service
