-- ============================================================================
-- Taxi Service Database Initialization Script
-- This script is executed automatically when PostgreSQL container starts
-- Combines schema.sql and sample data for initial setup
-- Mounted into PostgreSQL container at /docker-entrypoint-initdb.d/
-- ============================================================================

-- ============================================================================
-- SCHEMA CREATION
-- ============================================================================

-- Drop existing tables if they exist (for clean re-initialization)
DROP TABLE IF EXISTS orders CASCADE;
DROP TABLE IF EXISTS drivers CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- Create users table
CREATE TABLE users (
    id BIGSERIAL PRIMARY KEY,
    login VARCHAR(255) NOT NULL UNIQUE,
    email VARCHAR(255) NOT NULL UNIQUE,
    first_name VARCHAR(255) NOT NULL,
    last_name VARCHAR(255) NOT NULL,
    phone VARCHAR(50) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT chk_email_format CHECK (email LIKE '%@%'),
    CONSTRAINT chk_login_not_empty CHECK (LENGTH(TRIM(login)) > 0),
    CONSTRAINT chk_first_name_not_empty CHECK (LENGTH(TRIM(first_name)) > 0),
    CONSTRAINT chk_last_name_not_empty CHECK (LENGTH(TRIM(last_name)) > 0),
    CONSTRAINT chk_phone_format CHECK (phone LIKE '+%')
);

-- Create drivers table
CREATE TABLE drivers (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL UNIQUE,
    license_number VARCHAR(255) NOT NULL,
    car_model VARCHAR(255) NOT NULL,
    car_plate VARCHAR(50) NOT NULL,
    is_available BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT fk_drivers_user FOREIGN KEY (user_id)
        REFERENCES users(id) ON DELETE CASCADE
);

-- Create orders table
CREATE TABLE orders (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL,
    driver_id BIGINT,
    pickup_address VARCHAR(500) NOT NULL,
    destination_address VARCHAR(500) NOT NULL,
    status VARCHAR(50) NOT NULL DEFAULT 'active',
    price DECIMAL(10,2),
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    completed_at TIMESTAMP,

    CONSTRAINT fk_orders_user FOREIGN KEY (user_id)
        REFERENCES users(id),
    CONSTRAINT fk_orders_driver FOREIGN KEY (driver_id)
        REFERENCES drivers(id),
    CONSTRAINT chk_order_status CHECK (status IN ('active', 'accepted', 'completed', 'cancelled')),
    CONSTRAINT chk_pickup_not_empty CHECK (LENGTH(TRIM(pickup_address)) > 0),
    CONSTRAINT chk_destination_not_empty CHECK (LENGTH(TRIM(destination_address)) > 0),
    CONSTRAINT chk_addresses_different CHECK (TRIM(pickup_address) != TRIM(destination_address))
);

-- ============================================================================
-- INDEXES
-- ============================================================================

-- NOTE: PostgreSQL automatically creates indexes for PRIMARY KEY and UNIQUE constraints.
-- Therefore, we do NOT create explicit indexes for:
-- - users.id (covered by PRIMARY KEY)
-- - users.login (covered by UNIQUE constraint)
-- - users.email (covered by UNIQUE constraint)
-- - drivers.id (covered by PRIMARY KEY)
-- - drivers.user_id (covered by UNIQUE constraint)
-- - orders.id (covered by PRIMARY KEY)

CREATE INDEX idx_users_name_search ON users(first_name, last_name);
CREATE INDEX idx_users_name_pattern ON users(LOWER(first_name || ' ' || last_name) text_pattern_ops);
CREATE INDEX idx_orders_user_id ON orders(user_id);
CREATE INDEX idx_orders_driver_id ON orders(driver_id);
CREATE INDEX idx_orders_status ON orders(status);
CREATE INDEX idx_orders_status_created ON orders(status, created_at ASC);
CREATE INDEX idx_orders_user_created ON orders(user_id, created_at DESC);

-- ============================================================================
-- COMMENTS
-- ============================================================================

COMMENT ON TABLE users IS 'User accounts with authentication credentials for the taxi service';
COMMENT ON TABLE drivers IS 'Driver profiles linked to user accounts, including vehicle information';
COMMENT ON TABLE orders IS 'Taxi ride orders with status tracking (active → accepted → completed/cancelled)';

-- ============================================================================
-- STATISTICS
-- ============================================================================

ALTER TABLE users ALTER COLUMN login SET STATISTICS 1000;
ALTER TABLE users ALTER COLUMN email SET STATISTICS 1000;
ALTER TABLE drivers ALTER COLUMN user_id SET STATISTICS 1000;
ALTER TABLE orders ALTER COLUMN user_id SET STATISTICS 1000;
ALTER TABLE orders ALTER COLUMN driver_id SET STATISTICS 1000;
ALTER TABLE orders ALTER COLUMN status SET STATISTICS 1000;
ALTER TABLE orders ALTER COLUMN created_at SET STATISTICS 1000;

-- ============================================================================
-- INITIAL DATA FOR DEVELOPMENT
-- ============================================================================
-- NOTE: This data is inserted here for convenience during development.
-- For additional test data, use the data.sql file.

-- Insert sample user (matches openapi.yaml examples)
-- Password: "securePass123" (SHA-256 hash)
INSERT INTO users (login, email, first_name, last_name, phone, password_hash) VALUES
('ivan_petrov', 'ivan@example.com', 'Ivan', 'Petrov', '+79001234567', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879')
ON CONFLICT (login) DO NOTHING;

-- Insert a sample driver
INSERT INTO drivers (user_id, license_number, car_model, car_plate) VALUES
(1, 'DL-77-999999', 'Toyota Camry', 'X999XX77')
ON CONFLICT (user_id) DO NOTHING;

-- Insert a sample active order
INSERT INTO orders (user_id, pickup_address, destination_address, status) VALUES
(1, 'Москва, ул. Тверская, д. 1', 'Москва, Красная площадь, д. 1', 'active')
ON CONFLICT DO NOTHING;

-- ============================================================================
-- INITIALIZATION COMPLETE
-- ============================================================================
