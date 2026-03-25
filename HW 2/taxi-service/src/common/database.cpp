#include "database.hpp"

#include <userver/crypto/hash.hpp>

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace taxi_service {

namespace {

std::chrono::system_clock::time_point ParseSqliteDateTime(const char* raw) {
    if (!raw || raw[0] == '\0') {
        return std::chrono::system_clock::now();
    }
    std::tm tm{};
    std::istringstream ss(raw);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        return std::chrono::system_clock::now();
    }

    std::time_t t = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(t);
}

std::string ColText(sqlite3_stmt* stmt, int col) {
    const auto* raw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
    return raw ? raw : "";
}

}  // namespace

User Database::RowToUser(sqlite3_stmt* stmt) {
    User u;
    u.id            = sqlite3_column_int64(stmt, 0);
    u.login         = ColText(stmt, 1);
    u.email         = ColText(stmt, 2);
    u.first_name    = ColText(stmt, 3);
    u.last_name     = ColText(stmt, 4);
    u.phone         = ColText(stmt, 5);
    u.password_hash = ColText(stmt, 6);
    u.created_at    = ParseSqliteDateTime(sqlite3_column_text(stmt, 7)
                          ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7))
                          : nullptr);
    return u;
}

Driver Database::RowToDriver(sqlite3_stmt* stmt) {
    Driver d;
    d.id             = sqlite3_column_int64(stmt, 0);
    d.user_id        = sqlite3_column_int64(stmt, 1);
    d.license_number = ColText(stmt, 2);
    d.car_model      = ColText(stmt, 3);
    d.car_plate      = ColText(stmt, 4);
    d.is_available   = sqlite3_column_int(stmt, 5) != 0;
    d.created_at     = ParseSqliteDateTime(sqlite3_column_text(stmt, 6)
                           ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6))
                           : nullptr);
    return d;
}

Order Database::RowToOrder(sqlite3_stmt* stmt) {
    Order o;
    o.id                  = sqlite3_column_int64(stmt, 0);
    o.user_id             = sqlite3_column_int64(stmt, 1);

    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
        o.driver_id = sqlite3_column_int64(stmt, 2);
    }

    o.pickup_address      = ColText(stmt, 3);
    o.destination_address = ColText(stmt, 4);
    o.status              = ColText(stmt, 5);

    if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
        o.price = sqlite3_column_double(stmt, 6);
    }

    o.created_at = ParseSqliteDateTime(sqlite3_column_text(stmt, 7)
                       ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7))
                       : nullptr);

    if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
        o.completed_at = ParseSqliteDateTime(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)));
    }

    return o;
}

Database::Database(const std::string& db_path) {
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "unknown error";
        sqlite3_close(db_);
        throw std::runtime_error("Cannot open database '" + db_path + "': " + msg);
    }

    char* err = nullptr;
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &err);
    if (err) {
        std::cerr << "[taxi_db] WAL pragma warning: " << err << '\n';
        sqlite3_free(err);
    }

    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);

    sqlite3_busy_timeout(db_, 5000);

    InitSchema();
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void Database::InitSchema() {
    std::cerr << "[taxi_db] Initialising schema...\n";

    const char* sql = R"SQL(
        CREATE TABLE IF NOT EXISTS users (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            login         TEXT    UNIQUE NOT NULL,
            email         TEXT    UNIQUE NOT NULL,
            first_name    TEXT    NOT NULL,
            last_name     TEXT    NOT NULL,
            phone         TEXT    NOT NULL,
            password_hash TEXT    NOT NULL,
            created_at    DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS drivers (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id        INTEGER NOT NULL UNIQUE,
            license_number TEXT    NOT NULL,
            car_model      TEXT    NOT NULL,
            car_plate      TEXT    NOT NULL,
            is_available   INTEGER NOT NULL DEFAULT 1,
            created_at     DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS orders (
            id                   INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id              INTEGER NOT NULL,
            driver_id            INTEGER,
            pickup_address       TEXT    NOT NULL,
            destination_address  TEXT    NOT NULL,
            status               TEXT    NOT NULL DEFAULT 'active',
            price                REAL,
            created_at           DATETIME DEFAULT CURRENT_TIMESTAMP,
            completed_at         DATETIME,
            FOREIGN KEY (user_id)   REFERENCES users(id),
            FOREIGN KEY (driver_id) REFERENCES drivers(id)
        );

        CREATE INDEX IF NOT EXISTS idx_users_login       ON users(login);
        CREATE INDEX IF NOT EXISTS idx_drivers_user_id   ON drivers(user_id);
        CREATE INDEX IF NOT EXISTS idx_orders_user_id    ON orders(user_id);
        CREATE INDEX IF NOT EXISTS idx_orders_driver_id  ON orders(driver_id);
        CREATE INDEX IF NOT EXISTS idx_orders_status     ON orders(status);
    )SQL";

    if (ExecuteSQL(sql)) {
        std::cerr << "[taxi_db] Schema ready.\n";
    } else {
        std::cerr << "[taxi_db] Schema initialisation failed!\n";
    }
}

bool Database::ExecuteSQL(const std::string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "[taxi_db] SQL error: " << (err ? err : "?") << '\n';
        sqlite3_free(err);
        return false;
    }
    return true;
}

std::optional<int64_t> Database::GetLastInsertId() {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT last_insert_rowid()", -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    std::optional<int64_t> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::string Database::HashPassword(const std::string& password) const {
    return userver::crypto::hash::Sha256(password);
}

bool Database::VerifyPassword(const std::string& password,
                               const std::string& hash) const {
    return HashPassword(password) == hash;
}

std::optional<User> Database::CreateUser(const CreateUserRequest& request) {
    const char* sql =
        "INSERT INTO users (login, email, first_name, last_name, phone, password_hash) "
        "VALUES (?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    const std::string hash = HashPassword(request.password);
    sqlite3_bind_text(stmt, 1, request.login.c_str(),      -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, request.email.c_str(),      -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, request.first_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, request.last_name.c_str(),  -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, request.phone.c_str(),      -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, hash.c_str(),               -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "[taxi_db] CreateUser failed: " << sqlite3_errmsg(db_) << '\n';
        return std::nullopt;
    }

    auto new_id = GetLastInsertId();
    return new_id ? FindUserById(*new_id) : std::nullopt;
}

std::optional<User> Database::FindUserByLogin(const std::string& login) {
    const char* sql =
        "SELECT id, login, email, first_name, last_name, phone, password_hash, created_at "
        "FROM users WHERE login = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);

    std::optional<User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = RowToUser(stmt);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<User> Database::FindUserById(int64_t id) {
    const char* sql =
        "SELECT id, login, email, first_name, last_name, phone, password_hash, created_at "
        "FROM users WHERE id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, id);

    std::optional<User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = RowToUser(stmt);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<User> Database::SearchUsersByNameMask(const std::string& mask) {
    const std::string pattern = "%" + mask + "%";
    const char* sql =
        "SELECT id, login, email, first_name, last_name, phone, password_hash, created_at "
        "FROM users "
        "WHERE first_name || ' ' || last_name LIKE ? "
        "ORDER BY last_name, first_name";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }
    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_STATIC);

    std::vector<User> users;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        users.push_back(RowToUser(stmt));
    }
    sqlite3_finalize(stmt);
    return users;
}

std::optional<User> Database::AuthenticateUser(const std::string& login,
                                                const std::string& password) {
    auto user = FindUserByLogin(login);
    if (!user || !VerifyPassword(password, user->password_hash)) {
        return std::nullopt;
    }
    return user;
}

std::optional<Driver> Database::CreateDriver(int64_t user_id,
                                              const CreateDriverRequest& request) {
    const char* sql =
        "INSERT INTO drivers (user_id, license_number, car_model, car_plate) "
        "VALUES (?, ?, ?, ?)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int64(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, request.license_number.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, request.car_model.c_str(),      -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, request.car_plate.c_str(),      -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "[taxi_db] CreateDriver failed: " << sqlite3_errmsg(db_) << '\n';
        return std::nullopt;
    }

    auto new_id = GetLastInsertId();
    return new_id ? FindDriverById(*new_id) : std::nullopt;
}

std::optional<Driver> Database::FindDriverById(int64_t driver_id) {
    const char* sql =
        "SELECT id, user_id, license_number, car_model, car_plate, is_available, created_at "
        "FROM drivers WHERE id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, driver_id);

    std::optional<Driver> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = RowToDriver(stmt);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<Driver> Database::FindDriverByUserId(int64_t user_id) {
    const char* sql =
        "SELECT id, user_id, license_number, car_model, car_plate, is_available, created_at "
        "FROM drivers WHERE user_id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, user_id);

    std::optional<Driver> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = RowToDriver(stmt);
    }
    sqlite3_finalize(stmt);
    return result;
}

bool Database::SetDriverAvailability(int64_t driver_id, bool available) {
    const char* sql = "UPDATE drivers SET is_available = ? WHERE id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, available ? 1 : 0);
    sqlite3_bind_int64(stmt, 2, driver_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::optional<Order> Database::CreateOrder(int64_t user_id,
                                            const CreateOrderRequest& request) {
    const char* sql =
        "INSERT INTO orders (user_id, pickup_address, destination_address, status) "
        "VALUES (?, ?, ?, 'active')";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int64(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, request.pickup_address.c_str(),      -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, request.destination_address.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "[taxi_db] CreateOrder failed: " << sqlite3_errmsg(db_) << '\n';
        return std::nullopt;
    }

    auto new_id = GetLastInsertId();
    return new_id ? GetOrderById(*new_id) : std::nullopt;
}

std::optional<Order> Database::GetOrderById(int64_t order_id) {
    const char* sql =
        "SELECT id, user_id, driver_id, pickup_address, destination_address, "
        "       status, price, created_at, completed_at "
        "FROM orders WHERE id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, order_id);

    std::optional<Order> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = RowToOrder(stmt);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Order> Database::GetOrdersByUserId(int64_t user_id) {
    const char* sql =
        "SELECT id, user_id, driver_id, pickup_address, destination_address, "
        "       status, price, created_at, completed_at "
        "FROM orders WHERE user_id = ? "
        "ORDER BY created_at DESC";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }
    sqlite3_bind_int64(stmt, 1, user_id);

    std::vector<Order> orders;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        orders.push_back(RowToOrder(stmt));
    }
    sqlite3_finalize(stmt);
    return orders;
}

std::vector<Order> Database::GetActiveOrders() {
    const char* sql =
        "SELECT id, user_id, driver_id, pickup_address, destination_address, "
        "       status, price, created_at, completed_at "
        "FROM orders WHERE status = 'active' "
        "ORDER BY created_at ASC";  

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }

    std::vector<Order> orders;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        orders.push_back(RowToOrder(stmt));
    }
    sqlite3_finalize(stmt);
    return orders;
}

std::optional<Order> Database::AcceptOrder(int64_t order_id, int64_t driver_id) {

    const char* sql =
        "UPDATE orders "
        "SET driver_id = ?, status = 'accepted' "
        "WHERE id = ? AND status = 'active'";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, driver_id);
    sqlite3_bind_int64(stmt, 2, order_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE || sqlite3_changes(db_) == 0) {

        return std::nullopt;
    }

    return GetOrderById(order_id);
}

std::optional<Order> Database::CompleteOrder(int64_t order_id) {

    const char* sql =
        "UPDATE orders "
        "SET status = 'completed', completed_at = CURRENT_TIMESTAMP "
        "WHERE id = ? AND status = 'accepted'";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, order_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE || sqlite3_changes(db_) == 0) {
        return std::nullopt;
    }

    return GetOrderById(order_id);
}

std::optional<Order> Database::CancelOrder(int64_t order_id) {

    const char* sql =
        "UPDATE orders SET status = 'cancelled' "
        "WHERE id = ? AND status = 'active'";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, order_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE || sqlite3_changes(db_) == 0) {
        return std::nullopt;
    }

    return GetOrderById(order_id);
}

}  // namespace taxi_service
