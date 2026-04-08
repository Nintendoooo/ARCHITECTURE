#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include "models.hpp"

namespace taxi_service {

class Database {
public:
    explicit Database(userver::storages::postgres::ClusterPtr pg_cluster);
    ~Database() = default;

    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    // User operations
    std::optional<User> CreateUser(const CreateUserRequest& request);
    std::optional<User> FindUserByLogin(const std::string& login);
    std::optional<User> FindUserById(int64_t id);
    std::vector<User> SearchUsersByNameMask(const std::string& mask);
    std::optional<User> AuthenticateUser(const std::string& login,
                                         const std::string& password);

    // Driver operations
    std::optional<Driver> CreateDriver(int64_t user_id,
                                       const CreateDriverRequest& request);
    std::optional<Driver> FindDriverById(int64_t driver_id);
    std::optional<Driver> FindDriverByUserId(int64_t user_id);
    bool SetDriverAvailability(int64_t driver_id, bool available);

    // Order operations
    std::optional<Order> CreateOrder(int64_t user_id,
                                     const CreateOrderRequest& request);
    std::optional<Order> GetOrderById(int64_t order_id);
    std::vector<Order> GetOrdersByUserId(int64_t user_id);
    std::vector<Order> GetActiveOrders();
    std::optional<Order> AcceptOrder(int64_t order_id, int64_t driver_id);
    std::optional<Order> CompleteOrder(int64_t order_id);
    std::optional<Order> CancelOrder(int64_t order_id);

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;

    std::string HashPassword(const std::string& password) const;
    bool        VerifyPassword(const std::string& password,
                               const std::string& hash) const;
};

}  // namespace taxi_service
