-- ============================================================================
-- Taxi Service Database Schema for PostgreSQL
-- Variant 16: Система заказа такси (Taxi Service)
-- ============================================================================

-- Drop existing tables if they exist (for clean re-initialization)
DROP TABLE IF EXISTS orders CASCADE;
DROP TABLE IF EXISTS drivers CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- ============================================================================
-- Table: users
-- Description: Stores user accounts with authentication credentials
-- ============================================================================
CREATE TABLE users (
    id BIGSERIAL PRIMARY KEY,
    login VARCHAR(255) NOT NULL UNIQUE,
    email VARCHAR(255) NOT NULL UNIQUE,
    first_name VARCHAR(255) NOT NULL,
    last_name VARCHAR(255) NOT NULL,
    phone VARCHAR(50) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,

    -- Constraints
    CONSTRAINT chk_email_format CHECK (email LIKE '%@%'),
    CONSTRAINT chk_login_not_empty CHECK (LENGTH(TRIM(login)) > 0),
    CONSTRAINT chk_first_name_not_empty CHECK (LENGTH(TRIM(first_name)) > 0),
    CONSTRAINT chk_last_name_not_empty CHECK (LENGTH(TRIM(last_name)) > 0),
    CONSTRAINT chk_phone_format CHECK (phone LIKE '+%')
);

-- ============================================================================
-- Table: drivers
-- Description: Stores driver profiles linked to user accounts
-- ============================================================================
CREATE TABLE drivers (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL UNIQUE,
    license_number VARCHAR(255) NOT NULL,
    car_model VARCHAR(255) NOT NULL,
    car_plate VARCHAR(50) NOT NULL,
    is_available BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,

    -- Foreign key
    CONSTRAINT fk_drivers_user FOREIGN KEY (user_id)
        REFERENCES users(id) ON DELETE CASCADE
);

-- ============================================================================
-- Table: orders
-- Description: Stores taxi ride orders with status tracking
-- ============================================================================
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

    -- Foreign keys
    CONSTRAINT fk_orders_user FOREIGN KEY (user_id)
        REFERENCES users(id),
    CONSTRAINT fk_orders_driver FOREIGN KEY (driver_id)
        REFERENCES drivers(id),

    -- Constraints
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
-- Creating redundant indexes wastes disk space and slows down INSERT/UPDATE/DELETE operations.

-- Composite index on users first_name and last_name for name search
-- Rationale: Used in "Search user by name mask" API (WHERE first_name || ' ' || last_name LIKE $1)
-- Expected usage: User search by partial name match
CREATE INDEX idx_users_name_search ON users(first_name, last_name);

-- Additional index for case-insensitive name search using text_pattern_ops
-- Rationale: Optimizes LIKE/ILIKE queries for pattern matching
-- This is essential for pattern matching queries like '%smith%'
CREATE INDEX idx_users_name_pattern ON users(LOWER(first_name || ' ' || last_name) text_pattern_ops);

-- Index on orders.user_id for user order history lookups
-- Rationale: Used in "Get user order history" API (WHERE user_id = $1)
-- Expected usage: Every time user views their ride history
-- Note: This is a foreign key, so indexing improves JOIN performance
CREATE INDEX idx_orders_user_id ON orders(user_id);

-- Index on orders.driver_id for driver order lookups
-- Rationale: Used when looking up orders assigned to a specific driver
-- Expected usage: Driver views their assigned orders
CREATE INDEX idx_orders_driver_id ON orders(driver_id);

-- Index on orders.status for active orders query
-- Rationale: Used in "Get active orders" API (WHERE status = 'active')
-- Expected usage: Drivers browsing available orders to accept
CREATE INDEX idx_orders_status ON orders(status);

-- Composite index on orders for active orders sorted by date
-- Rationale: Optimizes the most common query: get active orders ordered by creation time
-- Expected usage: Primary order listing query for drivers
CREATE INDEX idx_orders_status_created ON orders(status, created_at ASC);

-- Composite index on orders for user order history with date ordering
-- Rationale: Optimizes user order history query with date sorting
-- Expected usage: User viewing their ride history (newest first)
CREATE INDEX idx_orders_user_created ON orders(user_id, created_at DESC);

-- ============================================================================
-- COMMENTS
-- ============================================================================

COMMENT ON TABLE users IS 'User accounts with authentication credentials for the taxi service';
COMMENT ON TABLE drivers IS 'Driver profiles linked to user accounts, including vehicle information';
COMMENT ON TABLE orders IS 'Taxi ride orders with status tracking (active → accepted → completed/cancelled)';

COMMENT ON COLUMN users.password_hash IS 'SHA-256 hash of user password';
COMMENT ON COLUMN users.phone IS 'Phone number in international format (starts with +)';
COMMENT ON COLUMN drivers.is_available IS 'Flag indicating if driver is available to accept new orders';
COMMENT ON COLUMN drivers.license_number IS 'Driver license number';
COMMENT ON COLUMN drivers.car_plate IS 'Vehicle license plate number';
COMMENT ON COLUMN orders.status IS 'Order status: active (new), accepted (driver assigned), completed, cancelled';
COMMENT ON COLUMN orders.price IS 'Ride price in currency units (set when order is completed)';
COMMENT ON COLUMN orders.completed_at IS 'Timestamp when the ride was completed';

-- ============================================================================
-- STATISTICS
-- ============================================================================

-- Increase statistics target for frequently queried columns
-- This helps PostgreSQL query planner make better decisions
ALTER TABLE users ALTER COLUMN login SET STATISTICS 1000;
ALTER TABLE users ALTER COLUMN email SET STATISTICS 1000;
ALTER TABLE drivers ALTER COLUMN user_id SET STATISTICS 1000;
ALTER TABLE orders ALTER COLUMN user_id SET STATISTICS 1000;
ALTER TABLE orders ALTER COLUMN driver_id SET STATISTICS 1000;
ALTER TABLE orders ALTER COLUMN status SET STATISTICS 1000;
ALTER TABLE orders ALTER COLUMN created_at SET STATISTICS 1000;

-- ============================================================================
-- END OF SCHEMA
-- ============================================================================
