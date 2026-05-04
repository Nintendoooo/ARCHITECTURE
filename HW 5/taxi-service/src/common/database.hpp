#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/oid.hpp>

#include "models.hpp"

namespace taxi_service {

/// @brief MongoDB-based Database class for Taxi Service
class Database {
public:
    explicit Database(const std::string& uri, const std::string& database);
    ~Database() = default;

    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    // User operations
    std::optional<User> CreateUser(const CreateUserRequest& request);
    std::optional<User> FindUserByLogin(const std::string& login);
    std::optional<User> FindUserById(const std::string& id);
    std::vector<User> SearchUsersByNameMask(const std::string& mask);
    std::optional<User> AuthenticateUser(const std::string& login,
                                         const std::string& password);

    // Driver operations
    std::optional<Driver> CreateDriver(const std::string& user_id,
                                       const CreateDriverRequest& request);
    std::optional<Driver> FindDriverById(const std::string& driver_id);
    std::optional<Driver> FindDriverByUserId(const std::string& user_id);
    std::vector<Driver> FindAvailableDrivers();
    bool SetDriverAvailability(const std::string& driver_id, bool available);

    // Order operations
    std::optional<Order> CreateOrder(const std::string& user_id,
                                     const CreateOrderRequest& request);
    std::optional<Order> GetOrderById(const std::string& order_id);
    std::vector<Order> GetOrdersByUserId(const std::string& user_id);
    std::vector<Order> GetActiveOrders();
    std::optional<Order> AcceptOrder(const std::string& order_id, const std::string& driver_id);
    std::optional<Order> CompleteOrder(const std::string& order_id);
    std::optional<Order> CancelOrder(const std::string& order_id);

private:
    mongocxx::instance instance_;
    mongocxx::client client_;
    mongocxx::database db_;
    
    std::string HashPassword(const std::string& password) const;
    bool        VerifyPassword(const std::string& password,
                               const std::string& hash) const;
};

}  // namespace taxi_service
