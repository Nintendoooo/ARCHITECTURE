#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <sqlite3.h>

#include "models.hpp"

namespace taxi_service {

class Database {
public:
    explicit Database(const std::string& db_path = "taxi.db");
    ~Database();

    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    std::optional<User> CreateUser(const CreateUserRequest& request);

    std::optional<User> FindUserByLogin(const std::string& login);

    std::optional<User> FindUserById(int64_t id);

    std::vector<User> SearchUsersByNameMask(const std::string& mask);

    std::optional<User> AuthenticateUser(const std::string& login,
                                         const std::string& password);

    std::optional<Driver> CreateDriver(int64_t user_id,
                                       const CreateDriverRequest& request);

    std::optional<Driver> FindDriverById(int64_t driver_id);

    std::optional<Driver> FindDriverByUserId(int64_t user_id);

    bool SetDriverAvailability(int64_t driver_id, bool available);

    std::optional<Order> CreateOrder(int64_t user_id,
                                     const CreateOrderRequest& request);

    std::optional<Order> GetOrderById(int64_t order_id);

    std::vector<Order> GetOrdersByUserId(int64_t user_id);

    std::vector<Order> GetActiveOrders();

    std::optional<Order> AcceptOrder(int64_t order_id, int64_t driver_id);

    std::optional<Order> CompleteOrder(int64_t order_id);

    std::optional<Order> CancelOrder(int64_t order_id);

    void InitSchema();

private:
    sqlite3* db_{nullptr};

    std::string HashPassword(const std::string& password) const;
    bool        VerifyPassword(const std::string& password,
                               const std::string& hash) const;

    bool                  ExecuteSQL(const std::string& sql);
    std::optional<int64_t> GetLastInsertId();


    static User   RowToUser(sqlite3_stmt* stmt);
    static Driver RowToDriver(sqlite3_stmt* stmt);
    static Order  RowToOrder(sqlite3_stmt* stmt);
};

}  // namespace taxi_service
