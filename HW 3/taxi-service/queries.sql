-- ============================================================================
-- Taxi Service SQL Queries
-- Variant 16: Система заказа такси (Taxi Service)
-- All queries for the 9 required API operations + auxiliary queries
-- ============================================================================

-- ============================================================================
-- USER OPERATIONS
-- ============================================================================

-- ----------------------------------------------------------------------------
-- Q1. Create new user (Создание нового пользователя)
-- API: POST /api/v1/users
-- Parameters: $1=login, $2=email, $3=first_name, $4=last_name, $5=phone, $6=password_hash
-- Returns: created user record
-- ----------------------------------------------------------------------------
INSERT INTO users (login, email, first_name, last_name, phone, password_hash)
VALUES ($1, $2, $3, $4, $5, $6)
RETURNING id, login, email, first_name, last_name, phone, password_hash, created_at;

-- Example:
-- INSERT INTO users (login, email, first_name, last_name, phone, password_hash)
-- VALUES ('john.doe', 'john@example.com', 'John', 'Doe', '+79001234567', 'hashed_password_here')
-- RETURNING id, login, email, first_name, last_name, phone, password_hash, created_at;


-- ----------------------------------------------------------------------------
-- Q2. Find user by login (Поиск пользователя по логину)
-- API: GET /api/v1/users/by-login?login=...
-- Parameters: $1=login
-- Returns: user details
-- Index used: users_login_key (UNIQUE index on login)
-- ----------------------------------------------------------------------------
SELECT id, login, email, first_name, last_name, phone, password_hash, created_at
FROM users
WHERE login = $1;

-- Example:
-- SELECT id, login, email, first_name, last_name, phone, password_hash, created_at
-- FROM users
-- WHERE login = 'alice.smith';


-- ----------------------------------------------------------------------------
-- Q3. Search users by name mask (Поиск пользователя по маске имя и фамилии)
-- API: GET /api/v1/users/search?mask=...
-- Parameters: $1=name_mask (e.g., '%alice%', '%smith%')
-- Returns: list of matching users
-- Index used: idx_users_name_pattern (text_pattern_ops on LOWER(first_name || ' ' || last_name))
-- Note: Uses LOWER() + LIKE for case-insensitive search
-- ----------------------------------------------------------------------------
SELECT id, login, email, first_name, last_name, phone, password_hash, created_at
FROM users
WHERE LOWER(first_name || ' ' || last_name) LIKE LOWER($1)
ORDER BY last_name, first_name;

-- Example:
-- SELECT id, login, email, first_name, last_name, phone, password_hash, created_at
-- FROM users
-- WHERE LOWER(first_name || ' ' || last_name) LIKE LOWER('%smith%')
-- ORDER BY last_name, first_name;


-- ----------------------------------------------------------------------------
-- Auxiliary: Authenticate user (for login endpoint)
-- API: POST /api/v1/auth/login
-- Parameters: $1=login
-- Returns: user record (password verification done in application code)
-- Index used: users_login_key (UNIQUE index on login)
-- ----------------------------------------------------------------------------
SELECT id, login, email, first_name, last_name, phone, password_hash, created_at
FROM users
WHERE login = $1;

-- Example:
-- SELECT id, login, email, first_name, last_name, phone, password_hash, created_at
-- FROM users
-- WHERE login = 'alice.smith';


-- ----------------------------------------------------------------------------
-- Auxiliary: Find user by ID
-- Parameters: $1=user_id
-- Returns: user details
-- Index used: users_pkey (PRIMARY KEY)
-- ----------------------------------------------------------------------------
SELECT id, login, email, first_name, last_name, phone, password_hash, created_at
FROM users
WHERE id = $1;


-- ============================================================================
-- DRIVER OPERATIONS
-- ============================================================================

-- ----------------------------------------------------------------------------
-- Q4. Register driver (Регистрация водителя)
-- API: POST /api/v1/drivers
-- Parameters: $1=user_id, $2=license_number, $3=car_model, $4=car_plate
-- Returns: created driver record
-- Note: Requires authenticated user; user_id from JWT token
-- ----------------------------------------------------------------------------
INSERT INTO drivers (user_id, license_number, car_model, car_plate)
VALUES ($1, $2, $3, $4)
RETURNING id, user_id, license_number, car_model, car_plate, is_available, created_at;

-- Example:
-- INSERT INTO drivers (user_id, license_number, car_model, car_plate)
-- VALUES (1, 'DL-77-001234', 'Toyota Camry', 'A001AA77')
-- RETURNING id, user_id, license_number, car_model, car_plate, is_available, created_at;


-- ----------------------------------------------------------------------------
-- Auxiliary: Find driver by ID
-- Parameters: $1=driver_id
-- Returns: driver details
-- Index used: drivers_pkey (PRIMARY KEY)
-- ----------------------------------------------------------------------------
SELECT id, user_id, license_number, car_model, car_plate, is_available, created_at
FROM drivers
WHERE id = $1;


-- ----------------------------------------------------------------------------
-- Auxiliary: Find driver by user_id
-- Parameters: $1=user_id
-- Returns: driver details
-- Index used: drivers_user_id_key (UNIQUE index on user_id)
-- ----------------------------------------------------------------------------
SELECT id, user_id, license_number, car_model, car_plate, is_available, created_at
FROM drivers
WHERE user_id = $1;


-- ----------------------------------------------------------------------------
-- Auxiliary: Set driver availability
-- Parameters: $1=is_available (boolean), $2=driver_id
-- Returns: nothing (UPDATE)
-- Index used: drivers_pkey (PRIMARY KEY)
-- ----------------------------------------------------------------------------
UPDATE drivers
SET is_available = $1
WHERE id = $2;


-- ============================================================================
-- ORDER OPERATIONS
-- ============================================================================

-- ----------------------------------------------------------------------------
-- Q5. Create order (Создание заказа поездки)
-- API: POST /api/v1/orders
-- Parameters: $1=user_id, $2=pickup_address, $3=destination_address
-- Returns: created order record
-- Note: Requires authenticated user; user_id from JWT token
-- ----------------------------------------------------------------------------
INSERT INTO orders (user_id, pickup_address, destination_address, status)
VALUES ($1, $2, $3, 'active')
RETURNING id, user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at;

-- Example:
-- INSERT INTO orders (user_id, pickup_address, destination_address, status)
-- VALUES (1, 'Москва, ул. Тверская, д. 1', 'Москва, Красная площадь, д. 1', 'active')
-- RETURNING id, user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at;


-- ----------------------------------------------------------------------------
-- Q6. Get active orders (Получение активных заказов)
-- API: GET /api/v1/orders/active
-- Parameters: none
-- Returns: list of orders with status='active', ordered by creation time (oldest first)
-- Index used: idx_orders_status_created (composite on status, created_at ASC)
-- ----------------------------------------------------------------------------
SELECT id, user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at
FROM orders
WHERE status = 'active'
ORDER BY created_at ASC;

-- Example:
-- SELECT id, user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at
-- FROM orders
-- WHERE status = 'active'
-- ORDER BY created_at ASC;


-- ----------------------------------------------------------------------------
-- Q7. Accept order by driver (Принятие заказа водителем)
-- API: PUT /api/v1/orders/{orderId}/accept
-- Parameters: $1=driver_id, $2=order_id
-- Returns: updated order record (or empty if order was not active)
-- Index used: orders_pkey (PRIMARY KEY) + status check
-- Note: Atomic operation — only updates if status is 'active'
-- ----------------------------------------------------------------------------
UPDATE orders
SET driver_id = $1, status = 'accepted'
WHERE id = $2 AND status = 'active'
RETURNING id, user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at;

-- Example:
-- UPDATE orders
-- SET driver_id = 1, status = 'accepted'
-- WHERE id = 5 AND status = 'active'
-- RETURNING id, user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at;


-- ----------------------------------------------------------------------------
-- Q8. Get user order history (Получение истории поездок пользователя)
-- API: GET /api/v1/orders/history?user_id=...
-- Parameters: $1=user_id
-- Returns: list of all orders for the user, ordered by creation time (newest first)
-- Index used: idx_orders_user_created (composite on user_id, created_at DESC)
-- ----------------------------------------------------------------------------
SELECT id, user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at
FROM orders
WHERE user_id = $1
ORDER BY created_at DESC;

-- Example:
-- SELECT id, user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at
-- FROM orders
-- WHERE user_id = 1
-- ORDER BY created_at DESC;


-- ----------------------------------------------------------------------------
-- Q9. Complete order (Завершение поездки)
-- API: PUT /api/v1/orders/{orderId}/complete
-- Parameters: $1=order_id
-- Returns: updated order record (or empty if order was not accepted)
-- Index used: orders_pkey (PRIMARY KEY) + status check
-- Note: Atomic operation — only updates if status is 'accepted'
-- ----------------------------------------------------------------------------
UPDATE orders
SET status = 'completed', completed_at = CURRENT_TIMESTAMP
WHERE id = $1 AND status = 'accepted'
RETURNING id, user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at;

-- Example:
-- UPDATE orders
-- SET status = 'completed', completed_at = CURRENT_TIMESTAMP
-- WHERE id = 5 AND status = 'accepted'
-- RETURNING id, user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at;


-- ----------------------------------------------------------------------------
-- Auxiliary: Get order by ID
-- Parameters: $1=order_id
-- Returns: order details
-- Index used: orders_pkey (PRIMARY KEY)
-- ----------------------------------------------------------------------------
SELECT id, user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at
FROM orders
WHERE id = $1;


-- ----------------------------------------------------------------------------
-- Auxiliary: Cancel order
-- Parameters: $1=order_id
-- Returns: updated order record (or empty if order was not active)
-- Index used: orders_pkey (PRIMARY KEY) + status check
-- Note: Only active orders can be cancelled
-- ----------------------------------------------------------------------------
UPDATE orders
SET status = 'cancelled'
WHERE id = $1 AND status = 'active'
RETURNING id, user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at;


-- ============================================================================
-- PERFORMANCE TESTING QUERIES
-- ============================================================================

-- Test Q2: Check if UNIQUE index is used for user login search
EXPLAIN ANALYZE
SELECT id, login, email, first_name, last_name, phone, password_hash, created_at
FROM users
WHERE login = 'alice.smith';

-- Test Q3: Check if pattern index is used for name mask search
EXPLAIN ANALYZE
SELECT id, login, email, first_name, last_name, phone, password_hash, created_at
FROM users
WHERE LOWER(first_name || ' ' || last_name) LIKE LOWER('%smith%')
ORDER BY last_name, first_name;

-- Test Q6: Check if composite index is used for active orders
EXPLAIN ANALYZE
SELECT id, user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at
FROM orders
WHERE status = 'active'
ORDER BY created_at ASC;

-- Test Q8: Check if composite index is used for user order history
EXPLAIN ANALYZE
SELECT id, user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at
FROM orders
WHERE user_id = 1
ORDER BY created_at DESC;

-- ============================================================================
-- END OF QUERIES
-- ============================================================================
