#include "database.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <userver/crypto/hash.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/exception/exception.hpp>

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;

namespace taxi_service {

// Constructor - initialize MongoDB connection
Database::Database(const std::string& uri, const std::string& database)
    : client_(mongocxx::uri{uri}) {
    
    // Extract database name from URI if not provided
    std::string db_name = database;
    if (db_name.empty()) {
        mongocxx::uri parsed_uri(uri);
        db_name = parsed_uri.database();
        if (db_name.empty()) {
            db_name = "taxi_db";
        }
    }
    db_ = client_[db_name];
    
    std::cerr << "[Database] Connected to MongoDB, uri: " << uri
              << ", database: " << db_name << std::endl;
}

std::string Database::HashPassword(const std::string& password) const {
    return userver::crypto::hash::Sha256(password);
}

bool Database::VerifyPassword(const std::string& password,
                               const std::string& hash) const {
    return HashPassword(password) == hash;
}

// ============================================================================
// USER OPERATIONS
// ============================================================================

std::optional<User> Database::CreateUser(const CreateUserRequest& request) {
    try {
        std::string password_hash = HashPassword(request.password);
        auto now = std::chrono::system_clock::now();

        auto doc = document{}
            << "login" << request.login
            << "email" << request.email
            << "first_name" << request.first_name
            << "last_name" << request.last_name
            << "phone" << request.phone
            << "password_hash" << password_hash
            << "role" << "passenger"
            << "created_at" << bsoncxx::types::b_date{now}
            << bsoncxx::builder::stream::finalize;

        auto result = db_["users"].insert_one(doc.view());
        
        if (!result) {
            std::cerr << "[taxi_db] CreateUser failed for login: " << request.login << std::endl;
            return std::nullopt;
        }

        auto inserted_id = result->inserted_id().get_oid().value.to_string();
        
        User user;
        user.id = inserted_id;
        user.login = request.login;
        user.email = request.email;
        user.first_name = request.first_name;
        user.last_name = request.last_name;
        user.phone = request.phone;
        user.password_hash = password_hash;
        user.role = "passenger";
        user.created_at = now;

        std::cerr << "[taxi_db] User created successfully: " << request.login << std::endl;
        return user;

    } catch (const mongocxx::exception& e) {
        std::cerr << "[taxi_db] CreateUser MongoDB exception for login: " << request.login
                  << ", error: " << e.what() << std::endl;
        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] CreateUser exception for login: " << request.login
                  << ", error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<User> Database::FindUserByLogin(const std::string& login) {
    try {
        auto filter = document{} << "login" << login << bsoncxx::builder::stream::finalize;
        auto result = db_["users"].find_one(filter.view());

        if (!result) {
            return std::nullopt;
        }

        auto view = result->view();
        User user;
        
        if (view["_id"]) {
            user.id = view["_id"].get_oid().value.to_string();
        }
        if (view["login"]) user.login = view["login"].get_string().value.data();
        if (view["email"]) user.email = view["email"].get_string().value.data();
        if (view["first_name"]) user.first_name = view["first_name"].get_string().value.data();
        if (view["last_name"]) user.last_name = view["last_name"].get_string().value.data();
        if (view["phone"]) user.phone = view["phone"].get_string().value.data();
        if (view["password_hash"]) user.password_hash = view["password_hash"].get_string().value.data();
        if (view["role"]) user.role = view["role"].get_string().value.data();
        if (view["created_at"]) {
            auto ms = view["created_at"].get_date().value.count();
            user.created_at = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
        }

        return user;

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] FindUserByLogin exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<User> Database::FindUserById(const std::string& id) {
    try {
        bsoncxx::oid oid(id);
        auto filter = document{} << "_id" << oid << bsoncxx::builder::stream::finalize;
        auto result = db_["users"].find_one(filter.view());

        if (!result) {
            return std::nullopt;
        }

        auto view = result->view();
        User user;
        
        if (view["_id"]) {
            user.id = view["_id"].get_oid().value.to_string();
        }
        if (view["login"]) user.login = view["login"].get_string().value.data();
        if (view["email"]) user.email = view["email"].get_string().value.data();
        if (view["first_name"]) user.first_name = view["first_name"].get_string().value.data();
        if (view["last_name"]) user.last_name = view["last_name"].get_string().value.data();
        if (view["phone"]) user.phone = view["phone"].get_string().value.data();
        if (view["password_hash"]) user.password_hash = view["password_hash"].get_string().value.data();
        if (view["role"]) user.role = view["role"].get_string().value.data();
        if (view["created_at"]) {
            auto ms = view["created_at"].get_date().value.count();
            user.created_at = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
        }

        return user;

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] FindUserById exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<User> Database::SearchUsersByNameMask(const std::string& mask) {
    try {
        auto filter = document{}
            << "$or" << bsoncxx::builder::stream::open_array
                << bsoncxx::builder::stream::open_document
                    << "first_name" << bsoncxx::builder::stream::open_document
                    << "$regex" << mask << "$options" << "i"
                    << bsoncxx::builder::stream::close_document
                << bsoncxx::builder::stream::close_document
                << bsoncxx::builder::stream::open_document
                    << "last_name" << bsoncxx::builder::stream::open_document
                    << "$regex" << mask << "$options" << "i"
                    << bsoncxx::builder::stream::close_document
                << bsoncxx::builder::stream::close_document
            << bsoncxx::builder::stream::close_array
            << bsoncxx::builder::stream::finalize;

        std::vector<User> results;
        auto cursor = db_["users"].find(filter.view());

        for (auto&& doc : cursor) {
            User user;
            if (doc["_id"]) {
                user.id = doc["_id"].get_oid().value.to_string();
            }
            if (doc["login"]) user.login = doc["login"].get_string().value.data();
            if (doc["email"]) user.email = doc["email"].get_string().value.data();
            if (doc["first_name"]) user.first_name = doc["first_name"].get_string().value.data();
            if (doc["last_name"]) user.last_name = doc["last_name"].get_string().value.data();
            if (doc["phone"]) user.phone = doc["phone"].get_string().value.data();
            if (doc["password_hash"]) user.password_hash = doc["password_hash"].get_string().value.data();
            if (doc["role"]) user.role = doc["role"].get_string().value.data();
            if (doc["created_at"]) {
                auto ms = doc["created_at"].get_date().value.count();
                user.created_at = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
            }
            results.push_back(user);
        }

        return results;

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] SearchUsersByNameMask exception: " << e.what() << std::endl;
        return {};
    }
}

std::optional<User> Database::AuthenticateUser(const std::string& login,
                                                const std::string& password) {
    auto user_opt = FindUserByLogin(login);
    if (!user_opt) {
        return std::nullopt;
    }

    auto& user = user_opt.value();
    if (!VerifyPassword(password, user.password_hash)) {
        return std::nullopt;
    }

    return user;
}

// ============================================================================
// DRIVER OPERATIONS
// ============================================================================

std::optional<Driver> Database::CreateDriver(const std::string& user_id,
                                              const CreateDriverRequest& request) {
    try {
        auto now = std::chrono::system_clock::now();
        bsoncxx::oid user_oid(user_id);

        auto doc = document{}
            << "user_id" << user_oid
            << "license_number" << request.license_number
            << "car_model" << request.car_model
            << "car_plate" << request.car_plate
            << "is_available" << true
            << "created_at" << bsoncxx::types::b_date{now}
            << bsoncxx::builder::stream::finalize;

        auto result = db_["drivers"].insert_one(doc.view());
        
        if (!result) {
            std::cerr << "[taxi_db] CreateDriver failed for user_id: " << user_id << std::endl;
            return std::nullopt;
        }

        auto inserted_id = result->inserted_id().get_oid().value.to_string();
        
        Driver driver;
        driver.id = inserted_id;
        driver.user_id = user_id;
        driver.license_number = request.license_number;
        driver.car_model = request.car_model;
        driver.car_plate = request.car_plate;
        driver.is_available = true;
        driver.created_at = now;

        // Update user role to driver
        auto user_filter = document{} << "_id" << user_oid << bsoncxx::builder::stream::finalize;
        auto user_update = document{}
            << "$set" << open_document
                << "role" << "driver"
                << "updated_at" << bsoncxx::types::b_date{now}
            << close_document
            << bsoncxx::builder::stream::finalize;
        db_["users"].update_one(user_filter.view(), user_update.view());

        std::cerr << "[taxi_db] Driver created successfully for user_id: " << user_id << std::endl;
        return driver;

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] CreateDriver exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Driver> Database::FindDriverById(const std::string& driver_id) {
    try {
        bsoncxx::oid oid(driver_id);
        auto filter = document{} << "_id" << oid << bsoncxx::builder::stream::finalize;
        auto result = db_["drivers"].find_one(filter.view());

        if (!result) {
            return std::nullopt;
        }

        auto view = result->view();
        Driver driver;
        
        if (view["_id"]) {
            driver.id = view["_id"].get_oid().value.to_string();
        }
        if (view["user_id"]) driver.user_id = view["user_id"].get_oid().value.to_string();
        if (view["license_number"]) driver.license_number = view["license_number"].get_string().value.data();
        if (view["car_model"]) driver.car_model = view["car_model"].get_string().value.data();
        if (view["car_plate"]) driver.car_plate = view["car_plate"].get_string().value.data();
        if (view["is_available"]) driver.is_available = view["is_available"].get_bool().value;
        if (view["created_at"]) {
            auto ms = view["created_at"].get_date().value.count();
            driver.created_at = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
        }

        return driver;

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] FindDriverById exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Driver> Database::FindDriverByUserId(const std::string& user_id) {
    try {
        bsoncxx::oid oid(user_id);
        auto filter = document{} << "user_id" << oid << bsoncxx::builder::stream::finalize;
        auto result = db_["drivers"].find_one(filter.view());

        if (!result) {
            return std::nullopt;
        }

        auto view = result->view();
        Driver driver;
        
        if (view["_id"]) {
            driver.id = view["_id"].get_oid().value.to_string();
        }
        if (view["user_id"]) driver.user_id = view["user_id"].get_oid().value.to_string();
        if (view["license_number"]) driver.license_number = view["license_number"].get_string().value.data();
        if (view["car_model"]) driver.car_model = view["car_model"].get_string().value.data();
        if (view["car_plate"]) driver.car_plate = view["car_plate"].get_string().value.data();
        if (view["is_available"]) driver.is_available = view["is_available"].get_bool().value;
        if (view["created_at"]) {
            auto ms = view["created_at"].get_date().value.count();
            driver.created_at = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
        }

        return driver;

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] FindDriverByUserId exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<Driver> Database::FindAvailableDrivers() {
    try {
        auto filter = document{} << "is_available" << true << bsoncxx::builder::stream::finalize;
        
        std::vector<Driver> results;
        auto cursor = db_["drivers"].find(filter.view());

        for (auto&& doc : cursor) {
            Driver driver;
            if (doc["_id"]) {
                driver.id = doc["_id"].get_oid().value.to_string();
            }
            if (doc["user_id"]) driver.user_id = doc["user_id"].get_oid().value.to_string();
            if (doc["license_number"]) driver.license_number = doc["license_number"].get_string().value.data();
            if (doc["car_model"]) driver.car_model = doc["car_model"].get_string().value.data();
            if (doc["car_plate"]) driver.car_plate = doc["car_plate"].get_string().value.data();
            if (doc["is_available"]) driver.is_available = doc["is_available"].get_bool().value;
            if (doc["created_at"]) {
                auto ms = doc["created_at"].get_date().value.count();
                driver.created_at = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
            }
            results.push_back(driver);
        }

        return results;

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] FindAvailableDrivers exception: " << e.what() << std::endl;
        return {};
    }
}

bool Database::SetDriverAvailability(const std::string& driver_id, bool available) {
    try {
        bsoncxx::oid oid(driver_id);
        auto now = std::chrono::system_clock::now();
        
        auto filter = document{} << "_id" << oid << bsoncxx::builder::stream::finalize;
        auto update = document{}
            << "$set" << open_document
                << "is_available" << available
                << "updated_at" << bsoncxx::types::b_date{now}
            << close_document
            << bsoncxx::builder::stream::finalize;

        auto result = db_["drivers"].update_one(filter.view(), update.view());
        return result && result->modified_count() > 0;

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] SetDriverAvailability exception: " << e.what() << std::endl;
        return false;
    }
}

// ============================================================================
// ORDER OPERATIONS
// ============================================================================

std::optional<Order> Database::CreateOrder(const std::string& user_id,
                                            const CreateOrderRequest& request) {
    try {
        auto now = std::chrono::system_clock::now();
        bsoncxx::oid user_oid(user_id);

        // Get user info for passenger data
        auto user_filter = document{} << "_id" << user_oid << bsoncxx::builder::stream::finalize;
        auto user_result = db_["users"].find_one(user_filter.view());
        
        std::string passenger_name;
        std::string passenger_phone;
        if (user_result) {
            auto user_view = user_result->view();
            if (user_view["first_name"] && user_view["last_name"]) {
                passenger_name = std::string(user_view["first_name"].get_string().value.data()) + " " +
                                 std::string(user_view["last_name"].get_string().value.data());
            }
            if (user_view["phone"]) {
                passenger_phone = user_view["phone"].get_string().value.data();
            }
        }

        auto doc = document{}
            << "user_id" << user_oid
            << "driver_id" << bsoncxx::types::b_null{}
            << "passenger" << open_document
                << "name" << passenger_name
                << "phone" << passenger_phone
            << close_document
            << "pickup_address" << request.pickup_address
            << "destination_address" << request.destination_address
            << "status" << "active"
            << "price" << bsoncxx::types::b_null{}
            << "created_at" << bsoncxx::types::b_date{now}
            << "completed_at" << bsoncxx::types::b_null{}
            << bsoncxx::builder::stream::finalize;

        auto result = db_["orders"].insert_one(doc.view());
        
        if (!result) {
            std::cerr << "[taxi_db] CreateOrder failed for user_id: " << user_id << std::endl;
            return std::nullopt;
        }

        auto inserted_id = result->inserted_id().get_oid().value.to_string();
        
        Order order;
        order.id = inserted_id;
        order.user_id = user_id;
        order.passenger_name = passenger_name;
        order.passenger_phone = passenger_phone;
        order.pickup_address = request.pickup_address;
        order.destination_address = request.destination_address;
        order.status = "active";
        order.created_at = now;

        std::cerr << "[taxi_db] Order created successfully: " << inserted_id << std::endl;
        return order;

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] CreateOrder exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Order> Database::GetOrderById(const std::string& order_id) {
    try {
        bsoncxx::oid oid(order_id);
        auto filter = document{} << "_id" << oid << bsoncxx::builder::stream::finalize;
        auto result = db_["orders"].find_one(filter.view());

        if (!result) {
            return std::nullopt;
        }

        auto view = result->view();
        Order order;
        
        if (view["_id"]) {
            order.id = view["_id"].get_oid().value.to_string();
        }
        if (view["user_id"]) order.user_id = view["user_id"].get_oid().value.to_string();
        if (view["driver_id"] && view["driver_id"].type() != bsoncxx::type::k_null) {
            order.driver_id = view["driver_id"].get_oid().value.to_string();
        }
        if (view["passenger"]) {
            auto passenger = view["passenger"].get_document().value;
            if (passenger["name"]) order.passenger_name = passenger["name"].get_string().value.data();
            if (passenger["phone"]) order.passenger_phone = passenger["phone"].get_string().value.data();
        }
        if (view["pickup_address"]) order.pickup_address = view["pickup_address"].get_string().value.data();
        if (view["destination_address"]) order.destination_address = view["destination_address"].get_string().value.data();
        if (view["status"]) order.status = view["status"].get_string().value.data();
        if (view["price"] && view["price"].type() != bsoncxx::type::k_null) {
            order.price = view["price"].get_double().value;
        }
        if (view["created_at"]) {
            auto ms = view["created_at"].get_date().value.count();
            order.created_at = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
        }
        if (view["completed_at"] && view["completed_at"].type() != bsoncxx::type::k_null) {
            auto ms = view["completed_at"].get_date().value.count();
            order.completed_at = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
        }

        return order;

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] GetOrderById exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<Order> Database::GetOrdersByUserId(const std::string& user_id) {
    try {
        bsoncxx::oid oid(user_id);
        auto filter = document{} 
            << "user_id" << oid 
            << bsoncxx::builder::stream::finalize;
        
        std::vector<Order> results;
        auto cursor = db_["orders"].find(filter.view());

        for (auto&& doc : cursor) {
            Order order;
            if (doc["_id"]) {
                order.id = doc["_id"].get_oid().value.to_string();
            }
            if (doc["user_id"]) order.user_id = doc["user_id"].get_oid().value.to_string();
            if (doc["driver_id"] && doc["driver_id"].type() != bsoncxx::type::k_null) {
                order.driver_id = doc["driver_id"].get_oid().value.to_string();
            }
            if (doc["passenger"]) {
                auto passenger = doc["passenger"].get_document().value;
                if (passenger["name"]) order.passenger_name = passenger["name"].get_string().value.data();
                if (passenger["phone"]) order.passenger_phone = passenger["phone"].get_string().value.data();
            }
            if (doc["pickup_address"]) order.pickup_address = doc["pickup_address"].get_string().value.data();
            if (doc["destination_address"]) order.destination_address = doc["destination_address"].get_string().value.data();
            if (doc["status"]) order.status = doc["status"].get_string().value.data();
            if (doc["price"] && doc["price"].type() != bsoncxx::type::k_null) {
                order.price = doc["price"].get_double().value;
            }
            if (doc["created_at"]) {
                auto ms = doc["created_at"].get_date().value.count();
                order.created_at = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
            }
            if (doc["completed_at"] && doc["completed_at"].type() != bsoncxx::type::k_null) {
                auto ms = doc["completed_at"].get_date().value.count();
                order.completed_at = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
            }
            results.push_back(order);
        }

        return results;

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] GetOrdersByUserId exception: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Order> Database::GetActiveOrders() {
    try {
        auto filter = document{} 
            << "status" << "active"
            << bsoncxx::builder::stream::finalize;
        
        std::vector<Order> results;
        auto cursor = db_["orders"].find(filter.view());

        for (auto&& doc : cursor) {
            Order order;
            if (doc["_id"]) {
                order.id = doc["_id"].get_oid().value.to_string();
            }
            if (doc["user_id"]) order.user_id = doc["user_id"].get_oid().value.to_string();
            if (doc["driver_id"] && doc["driver_id"].type() != bsoncxx::type::k_null) {
                order.driver_id = doc["driver_id"].get_oid().value.to_string();
            }
            if (doc["passenger"]) {
                auto passenger = doc["passenger"].get_document().value;
                if (passenger["name"]) order.passenger_name = passenger["name"].get_string().value.data();
                if (passenger["phone"]) order.passenger_phone = passenger["phone"].get_string().value.data();
            }
            if (doc["pickup_address"]) order.pickup_address = doc["pickup_address"].get_string().value.data();
            if (doc["destination_address"]) order.destination_address = doc["destination_address"].get_string().value.data();
            if (doc["status"]) order.status = doc["status"].get_string().value.data();
            if (doc["price"] && doc["price"].type() != bsoncxx::type::k_null) {
                order.price = doc["price"].get_double().value;
            }
            if (doc["created_at"]) {
                auto ms = doc["created_at"].get_date().value.count();
                order.created_at = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
            }
            if (doc["completed_at"] && doc["completed_at"].type() != bsoncxx::type::k_null) {
                auto ms = doc["completed_at"].get_date().value.count();
                order.completed_at = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
            }
            results.push_back(order);
        }

        return results;

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] GetActiveOrders exception: " << e.what() << std::endl;
        return {};
    }
}

std::optional<Order> Database::AcceptOrder(const std::string& order_id, 
                                            const std::string& driver_id) {
    try {
        bsoncxx::oid order_oid(order_id);
        bsoncxx::oid driver_oid(driver_id);
        
        auto filter = document{} << "_id" << order_oid << bsoncxx::builder::stream::finalize;
        auto update = document{}
            << "$set" << open_document
                << "driver_id" << driver_oid
                << "status" << "accepted"
            << close_document
            << bsoncxx::builder::stream::finalize;

        auto result = db_["orders"].update_one(filter.view(), update.view());
        
        if (!result || result->modified_count() == 0) {
            return std::nullopt;
        }

        // Set driver as unavailable
        SetDriverAvailability(driver_id, false);

        return GetOrderById(order_id);

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] AcceptOrder exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Order> Database::CompleteOrder(const std::string& order_id) {
    try {
        bsoncxx::oid oid(order_id);
        auto now = std::chrono::system_clock::now();
        
        // First get the order to find the driver
        auto order_opt = GetOrderById(order_id);
        if (!order_opt) {
            return std::nullopt;
        }
        
        // Generate a random price for the completed order
        double price = 300.0 + (rand() % 500);  // Random price between 300-800
        
        auto filter = document{} << "_id" << oid << bsoncxx::builder::stream::finalize;
        auto update = document{}
            << "$set" << open_document
                << "status" << "completed"
                << "price" << price
                << "completed_at" << bsoncxx::types::b_date{now}
            << close_document
            << bsoncxx::builder::stream::finalize;

        auto result = db_["orders"].update_one(filter.view(), update.view());
        
        if (!result || result->modified_count() == 0) {
            return std::nullopt;
        }

        // Make driver available again
        if (order_opt->driver_id) {
            SetDriverAvailability(*order_opt->driver_id, true);
        }

        return GetOrderById(order_id);

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] CompleteOrder exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Order> Database::CancelOrder(const std::string& order_id) {
    try {
        bsoncxx::oid oid(order_id);
        
        // First get the order to find the driver
        auto order_opt = GetOrderById(order_id);
        if (!order_opt) {
            return std::nullopt;
        }
        
        auto filter = document{} << "_id" << oid << bsoncxx::builder::stream::finalize;
        auto update = document{}
            << "$set" << open_document
                << "status" << "cancelled"
            << close_document
            << bsoncxx::builder::stream::finalize;

        auto result = db_["orders"].update_one(filter.view(), update.view());
        
        if (!result || result->modified_count() == 0) {
            return std::nullopt;
        }

        // Make driver available again if order was accepted
        if (order_opt->driver_id) {
            SetDriverAvailability(*order_opt->driver_id, true);
        }

        return GetOrderById(order_id);

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] CancelOrder exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

}  // namespace taxi_service
