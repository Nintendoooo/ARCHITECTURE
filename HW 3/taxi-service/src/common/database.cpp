#include "database.hpp"

#include <iostream>
#include <userver/crypto/hash.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/parameter_store.hpp>
#include <userver/storages/postgres/exceptions.hpp>

namespace taxi_service {

Database::Database(userver::storages::postgres::ClusterPtr pg_cluster)
    : pg_cluster_(std::move(pg_cluster)) {
    // No initialization needed - schema is managed by init-db.sql
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

        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO users (login, email, first_name, last_name, phone, password_hash) "
            "VALUES ($1, $2, $3, $4, $5, $6) "
            "RETURNING id, login, email, first_name, last_name, phone, password_hash, created_at",
            request.login,
            request.email,
            request.first_name,
            request.last_name,
            request.phone,
            password_hash
        );

        if (result.IsEmpty()) {
            std::cerr << "[taxi_db] CreateUser result is empty for login: " << request.login << std::endl;
            return std::nullopt;
        }

        auto row = result.AsSingleRow<User>(userver::storages::postgres::kRowTag);
        std::cerr << "[taxi_db] User created successfully: " << request.login << std::endl;
        return row;

    } catch (const userver::storages::postgres::UniqueViolation& e) {
        std::cerr << "[taxi_db] CreateUser UniqueViolation for login: " << request.login
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
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, login, email, first_name, last_name, phone, password_hash, created_at "
            "FROM users WHERE login = $1",
            login
        );

        if (result.IsEmpty()) {
            return std::nullopt;
        }

        return result.AsSingleRow<User>(userver::storages::postgres::kRowTag);

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] FindUserByLogin exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<User> Database::FindUserById(int64_t id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, login, email, first_name, last_name, phone, password_hash, created_at "
            "FROM users WHERE id = $1",
            id
        );

        if (result.IsEmpty()) {
            return std::nullopt;
        }

        return result.AsSingleRow<User>(userver::storages::postgres::kRowTag);

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] FindUserById exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<User> Database::SearchUsersByNameMask(const std::string& mask) {
    try {
        // Add wildcards for LIKE pattern matching
        std::string search_pattern = "%" + mask + "%";

        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, login, email, first_name, last_name, phone, password_hash, created_at "
            "FROM users "
            "WHERE LOWER(first_name || ' ' || last_name) LIKE LOWER($1) "
            "ORDER BY last_name, first_name",
            search_pattern
        );

        return result.AsContainer<std::vector<User>>(userver::storages::postgres::kRowTag);

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] SearchUsersByNameMask exception: " << e.what() << std::endl;
        return {};
    }
}

std::optional<User> Database::AuthenticateUser(const std::string& login,
                                                const std::string& password) {
    auto user = FindUserByLogin(login);
    if (!user || !VerifyPassword(password, user->password_hash)) {
        return std::nullopt;
    }
    return user;
}

// ============================================================================
// DRIVER OPERATIONS
// ============================================================================

std::optional<Driver> Database::CreateDriver(int64_t user_id,
                                              const CreateDriverRequest& request) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO drivers (user_id, license_number, car_model, car_plate) "
            "VALUES ($1, $2, $3, $4) "
            "RETURNING id, user_id, license_number, car_model, car_plate, is_available, created_at",
            user_id,
            request.license_number,
            request.car_model,
            request.car_plate
        );

        if (result.IsEmpty()) {
            std::cerr << "[taxi_db] CreateDriver result is empty for user_id: " << user_id << std::endl;
            return std::nullopt;
        }

        auto row = result.AsSingleRow<Driver>(userver::storages::postgres::kRowTag);
        std::cerr << "[taxi_db] Driver created successfully for user_id: " << user_id << std::endl;
        return row;

    } catch (const userver::storages::postgres::UniqueViolation& e) {
        std::cerr << "[taxi_db] CreateDriver UniqueViolation for user_id: " << user_id
                  << ", error: " << e.what() << std::endl;
        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] CreateDriver exception for user_id: " << user_id
                  << ", error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Driver> Database::FindDriverById(int64_t driver_id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, user_id, license_number, car_model, car_plate, is_available, created_at "
            "FROM drivers WHERE id = $1",
            driver_id
        );

        if (result.IsEmpty()) {
            return std::nullopt;
        }

        return result.AsSingleRow<Driver>(userver::storages::postgres::kRowTag);

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] FindDriverById exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Driver> Database::FindDriverByUserId(int64_t user_id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, user_id, license_number, car_model, car_plate, is_available, created_at "
            "FROM drivers WHERE user_id = $1",
            user_id
        );

        if (result.IsEmpty()) {
            return std::nullopt;
        }

        return result.AsSingleRow<Driver>(userver::storages::postgres::kRowTag);

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] FindDriverByUserId exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool Database::SetDriverAvailability(int64_t driver_id, bool available) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "UPDATE drivers SET is_available = $1 WHERE id = $2",
            available,
            driver_id
        );

        return result.RowsAffected() > 0;

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] SetDriverAvailability exception: " << e.what() << std::endl;
        return false;
    }
}

// ============================================================================
// ORDER OPERATIONS
// ============================================================================

std::optional<Order> Database::CreateOrder(int64_t user_id,
                                            const CreateOrderRequest& request) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO orders (user_id, pickup_address, destination_address, status) "
            "VALUES ($1, $2, $3, 'active') "
            "RETURNING id, user_id, driver_id, pickup_address, destination_address, "
            "status, price::DOUBLE PRECISION, created_at, completed_at",
            user_id,
            request.pickup_address,
            request.destination_address
        );

        if (result.IsEmpty()) {
            std::cerr << "[taxi_db] CreateOrder result is empty for user_id: " << user_id << std::endl;
            return std::nullopt;
        }

        auto row = result.AsSingleRow<Order>(userver::storages::postgres::kRowTag);
        std::cerr << "[taxi_db] Order created successfully for user_id: " << user_id << std::endl;
        return row;

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] CreateOrder exception for user_id: " << user_id
                  << ", error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Order> Database::GetOrderById(int64_t order_id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, user_id, driver_id, pickup_address, destination_address, "
            "status, price::DOUBLE PRECISION, created_at, completed_at "
            "FROM orders WHERE id = $1",
            order_id
        );

        if (result.IsEmpty()) {
            return std::nullopt;
        }

        return result.AsSingleRow<Order>(userver::storages::postgres::kRowTag);

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] GetOrderById exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<Order> Database::GetOrdersByUserId(int64_t user_id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, user_id, driver_id, pickup_address, destination_address, "
            "status, price::DOUBLE PRECISION, created_at, completed_at "
            "FROM orders WHERE user_id = $1 "
            "ORDER BY created_at DESC",
            user_id
        );

        return result.AsContainer<std::vector<Order>>(userver::storages::postgres::kRowTag);

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] GetOrdersByUserId exception: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Order> Database::GetActiveOrders() {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, user_id, driver_id, pickup_address, destination_address, "
            "status, price::DOUBLE PRECISION, created_at, completed_at "
            "FROM orders WHERE status = 'active' "
            "ORDER BY created_at ASC"
        );

        return result.AsContainer<std::vector<Order>>(userver::storages::postgres::kRowTag);

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] GetActiveOrders exception: " << e.what() << std::endl;
        return {};
    }
}

std::optional<Order> Database::AcceptOrder(int64_t order_id, int64_t driver_id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "UPDATE orders "
            "SET driver_id = $1, status = 'accepted' "
            "WHERE id = $2 AND status = 'active' "
            "RETURNING id, user_id, driver_id, pickup_address, destination_address, "
            "status, price::DOUBLE PRECISION, created_at, completed_at",
            driver_id,
            order_id
        );

        if (result.IsEmpty()) {
            return std::nullopt;
        }

        return result.AsSingleRow<Order>(userver::storages::postgres::kRowTag);

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] AcceptOrder exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Order> Database::CompleteOrder(int64_t order_id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "UPDATE orders "
            "SET status = 'completed', completed_at = CURRENT_TIMESTAMP "
            "WHERE id = $1 AND status = 'accepted' "
            "RETURNING id, user_id, driver_id, pickup_address, destination_address, "
            "status, price::DOUBLE PRECISION, created_at, completed_at",
            order_id
        );

        if (result.IsEmpty()) {
            return std::nullopt;
        }

        return result.AsSingleRow<Order>(userver::storages::postgres::kRowTag);

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] CompleteOrder exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Order> Database::CancelOrder(int64_t order_id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "UPDATE orders SET status = 'cancelled' "
            "WHERE id = $1 AND status = 'active' "
            "RETURNING id, user_id, driver_id, pickup_address, destination_address, "
            "status, price::DOUBLE PRECISION, created_at, completed_at",
            order_id
        );

        if (result.IsEmpty()) {
            return std::nullopt;
        }

        return result.AsSingleRow<Order>(userver::storages::postgres::kRowTag);

    } catch (const std::exception& e) {
        std::cerr << "[taxi_db] CancelOrder exception: " << e.what() << std::endl;
        return std::nullopt;
    }
}

}  // namespace taxi_service
