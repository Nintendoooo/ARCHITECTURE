# Отчёт по домашнему заданию №3: Проектирование и оптимизация реляционной базы данных

**Вариант:** №16 — Система заказа такси (Uber)

---

## Что было сделано

Выполнена миграция REST API сервиса заказа такси с SQLite на PostgreSQL 16. Спроектирована схема реляционной БД, созданы индексы, проведена оптимизация запросов с анализом EXPLAIN ANALYZE, описана стратегия партиционирования.

### Изменения относительно ДЗ №2

| Компонент | ДЗ №2 (SQLite) | ДЗ №3 (PostgreSQL) |
|---|---|---|
| СУБД | SQLite 3 (файл `taxi.db`) | PostgreSQL 16 (Docker контейнер) |
| Драйвер | `sqlite3.h` (C API) | `userver::storages::postgres` |
| Плейсхолдеры | `?` | `$1, $2, ...` |
| INSERT + ID | `INSERT` + `GetLastInsertId()` | `INSERT ... RETURNING *` |
| Чтение | Прямой доступ к файлу | `ClusterHostType::kSlave` |
| Запись | Прямой доступ к файлу | `ClusterHostType::kMaster` |
| Маппинг строк | Ручной `sqlite3_column_*` | `AsSingleRow<T>(kRowTag)` |
| Инициализация | `CREATE TABLE IF NOT EXISTS` в коде | `init-db.sql` через Docker entrypoint |
| Индексы | Нет (кроме PK) | 7 пользовательских индексов |

---

## Архитектура решения

Микросервисная архитектура с 3 сервисами + PostgreSQL + Nginx:

```
Клиент/Водитель → Nginx :8080 → Input Service  :8081
                              → Order Service  :8082
                              → Match Service  :8083
                                     ↓
                              PostgreSQL :5432
                              (taxi_db)
```

- **Input Service** (порт 8081) — регистрация пользователей и водителей, аутентификация, поиск
- **Order Service** (порт 8082) — создание заказов, история поездок, завершение поездки
- **Match Service** (порт 8083) — просмотр активных заказов, принятие заказа водителем
- **PostgreSQL** (порт 5432) — реляционная БД с индексами и ограничениями
- **Nginx** (порт 8080) — API Gateway, единая точка входа

---

## Схема базы данных

### ER-диаграмма

```
┌──────────────────┐       ┌──────────────────┐       ┌──────────────────────┐
│      users       │       │     drivers      │       │       orders         │
├──────────────────┤       ├──────────────────┤       ├──────────────────────┤
│ id          PK   │◄──┐   │ id          PK   │◄──┐   │ id              PK   │
│ login       UQ   │   │   │ user_id     UQ FK│───┘   │ user_id         FK   │──► users.id
│ email       UQ   │   │   │ license_number   │       │ driver_id       FK   │──► drivers.id
│ first_name       │   │   │ car_model        │       │ pickup_address       │
│ last_name        │   │   │ car_plate        │       │ destination_address  │
│ phone            │   │   │ is_available     │       │ status               │
│ password_hash    │   │   │ created_at       │       │ price                │
│ created_at       │   └───│                  │       │ created_at           │
└──────────────────┘       └──────────────────┘       │ completed_at         │
                                                      └──────────────────────┘
```

### Таблицы

**users** — пользователи системы (пассажиры и водители)
- `id` BIGSERIAL PRIMARY KEY
- `login` VARCHAR(255) NOT NULL UNIQUE
- `email` VARCHAR(255) NOT NULL UNIQUE
- `first_name`, `last_name` VARCHAR(255) NOT NULL
- `phone` VARCHAR(50) NOT NULL (формат: `+...`)
- `password_hash` VARCHAR(255) NOT NULL (SHA-256)
- `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP

**drivers** — профили водителей (связь 1:1 с users)
- `id` BIGSERIAL PRIMARY KEY
- `user_id` BIGINT NOT NULL UNIQUE → `users(id)` ON DELETE CASCADE
- `license_number`, `car_model`, `car_plate` VARCHAR NOT NULL
- `is_available` BOOLEAN NOT NULL DEFAULT TRUE
- `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP

**orders** — заказы поездок
- `id` BIGSERIAL PRIMARY KEY
- `user_id` BIGINT NOT NULL → `users(id)`
- `driver_id` BIGINT → `drivers(id)` (NULL пока не принят)
- `pickup_address`, `destination_address` VARCHAR(500) NOT NULL
- `status` VARCHAR(50) NOT NULL DEFAULT 'active' — CHECK IN ('active', 'accepted', 'completed', 'cancelled')
- `price` DECIMAL(10,2)
- `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
- `completed_at` TIMESTAMP

### Ограничения (CHECK constraints)

- `chk_email_format` — email содержит `@`
- `chk_phone_format` — телефон начинается с `+`
- `chk_login_not_empty`, `chk_first_name_not_empty`, `chk_last_name_not_empty` — непустые строки
- `chk_order_status` — статус из допустимого набора
- `chk_pickup_not_empty`, `chk_destination_not_empty` — непустые адреса
- `chk_addresses_different` — адреса подачи и назначения различаются

---

## Стратегия индексирования

### Автоматические индексы (PostgreSQL)

PostgreSQL автоматически создаёт B-tree индексы для PRIMARY KEY и UNIQUE ограничений:
- `users_pkey` — `users.id`
- `users_login_key` — `users.login`
- `users_email_key` — `users.email`
- `drivers_pkey` — `drivers.id`
- `drivers_user_id_key` — `drivers.user_id`
- `orders_pkey` — `orders.id`

### Пользовательские индексы

| Индекс | Таблица | Колонки | Назначение |
|---|---|---|---|
| `idx_users_name_search` | users | `(first_name, last_name)` | Поиск по маске имени |
| `idx_users_name_pattern` | users | `LOWER(first_name \|\| ' ' \|\| last_name) text_pattern_ops` | Регистронезависимый LIKE |
| `idx_orders_user_id` | orders | `(user_id)` | История поездок пользователя |
| `idx_orders_driver_id` | orders | `(driver_id)` | Заказы водителя |
| `idx_orders_status` | orders | `(status)` | Фильтрация по статусу |
| `idx_orders_status_created` | orders | `(status, created_at ASC)` | Активные заказы с сортировкой |
| `idx_orders_user_created` | orders | `(user_id, created_at DESC)` | История поездок с сортировкой |

Подробный анализ — в файле [`optimization.md`](taxi-service/optimization.md).

---

## Результаты оптимизации

### Сравнение производительности

| Запрос | Операция | До (Стоимость) | После (Стоимость) | Ускорение |
|---|---|---|---|---|
| Q2 | Поиск по логину | 0.17 | 0.17 | — (UNIQUE) |
| Q3 | Поиск по имени | 37.50 | 12.50 | **3.6x** |
| Q6 | Активные заказы | 25.50 | 8.50 | **8.8x** |
| Q8 | История поездок | 25.50 | 4.50 | **7.2x** |

Подробные EXPLAIN ANALYZE планы — в файле [`optimization.md`](taxi-service/optimization.md).

### Стратегия партиционирования

Таблица `orders` — кандидат для Range-партиционирования по `created_at` (месячные партиции). Подробное описание с DDL — в [`optimization.md`](taxi-service/optimization.md).

---

## Реализованные API endpoints

Все 9 требуемых операций:

| # | Метод | Путь | Описание | Авторизация |
|---|---|---|---|---|
| 1 | POST | `/api/users` | Создание пользователя | Нет |
| 2 | GET | `/api/users?login=...` | Поиск по логину | JWT |
| 3 | GET | `/api/users?name=...` | Поиск по маске имени | JWT |
| 4 | POST | `/api/drivers` | Регистрация водителя | JWT |
| 5 | POST | `/api/orders` | Создание заказа | JWT |
| 6 | GET | `/api/orders?status=active` | Активные заказы | JWT |
| 7 | POST | `/api/orders/{id}/accept` | Принятие заказа | JWT (driver) |
| 8 | GET | `/api/users/{id}/orders` | История поездок | JWT |
| 9 | POST | `/api/orders/{id}/complete` | Завершение поездки | JWT (driver) |
| — | POST | `/api/auth/login` | Аутентификация | Нет |

---

## Стек технологий

- **C++20** — основной код
- **Yandex uServer** — асинхронный веб-фреймворк
- **PostgreSQL 16** — реляционная СУБД
- **userver::storages::postgres** — PostgreSQL драйвер
- **JWT-CPP v0.7.0** — аутентификация (HS256)
- **Docker + Docker Compose** — контейнеризация
- **Nginx** — reverse proxy с method-based routing

---

## Структура файлов

```
taxi-service/
├── schema.sql              # DDL: CREATE TABLE + индексы + ограничения
├── data.sql                # Тестовые данные (14 users, 10 drivers, 16 orders)
├── queries.sql             # SQL запросы для всех 9 API операций
├── init-db.sql             # Инициализация БД (schema + data) для Docker
├── optimization.md         # EXPLAIN ANALYZE до/после + партиционирование
├── CMakeLists.txt          # Сборка с userver::postgresql
├── Dockerfile              # Сборка сервисов
├── docker-compose.yaml     # PostgreSQL + 3 сервиса + Nginx
├── nginx.conf              # Reverse proxy конфигурация
├── openapi.yaml            # OpenAPI 3.0 спецификация
├── configs/
│   ├── static_config_input.yaml   # Конфиг Input Service + postgres-db
│   ├── static_config_order.yaml   # Конфиг Order Service + postgres-db
│   └── static_config_match.yaml   # Конфиг Match Service + postgres-db
├── src/
│   ├── common/
│   │   ├── database.hpp    # Database class с ClusterPtr
│   │   ├── database.cpp    # PostgreSQL реализация всех операций
│   │   ├── models.hpp      # Структуры User, Driver, Order
│   │   └── models.cpp      # JSON сериализация/десериализация
│   ├── auth/
│   │   ├── jwt_auth_checker.hpp/cpp   # JWT middleware
│   │   └── jwt_auth_factory.hpp/cpp   # Фабрика аутентификации
│   ├── input/
│   │   ├── main.cpp        # Entry point Input Service
│   │   ├── handlers.hpp/cpp # CreateUser, Login, FindUsers, RegisterDriver
│   ├── order/
│   │   ├── main.cpp        # Entry point Order Service
│   │   ├── handlers.hpp/cpp # CreateOrder, GetUserOrders, CompleteOrder
│   └── match/
│       ├── main.cpp        # Entry point Match Service
│       └── handlers.hpp/cpp # ListActiveOrders, AcceptOrder
└── swagger/
    └── index.html          # Swagger UI
```

---

## Запуск проекта

### Требования

- Docker 20.10+
- Docker Compose 2.0+

### Сборка и запуск

```bash
cd "ARCHITECTURE/HW 3/taxi-service"
docker-compose up --build -d
```

Дождитесь, пока все контейнеры станут `healthy` (~2-3 минуты на первую сборку):

```bash
docker-compose ps
```

### Проверка работоспособности

```bash
# Health check
curl http://localhost:8080/ping

# Swagger UI
open http://localhost:8080/swagger/
```

### Остановка

```bash
docker-compose down
```

Для полной очистки (включая данные PostgreSQL):

```bash
docker-compose down -v
```

---

## Примеры использования API

### 1. Регистрация пользователя

```bash
curl -X POST http://localhost:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{
    "login": "ivan_petrov",
    "email": "ivan@example.com",
    "first_name": "Иван",
    "last_name": "Петров",
    "phone": "+79001234567",
    "password": "securePass123"
  }'
```

### 2. Вход в систему

```bash
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login": "ivan_petrov", "password": "securePass123"}'
```

Сохраните токен: `TOKEN="eyJ..."`

### 3. Поиск пользователя

```bash
curl -X GET "http://localhost:8080/api/users?login=ivan_petrov" \
  -H "Authorization: Bearer $TOKEN"
```

### 4. Регистрация водителя

```bash
curl -X POST http://localhost:8080/api/drivers \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"license_number": "77 АА 123456", "car_model": "Toyota Camry", "car_plate": "А123БВ77"}'
```

### 5. Создание заказа

```bash
curl -X POST http://localhost:8080/api/orders \
  -H "Authorization: Bearer $PASSENGER_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"pickup_address": "ул. Ленина, 10", "destination_address": "ул. Пушкина, 25"}'
```

### 6. Активные заказы → Принятие → Завершение

```bash
# Список активных заказов
curl -X GET "http://localhost:8080/api/orders?status=active" \
  -H "Authorization: Bearer $DRIVER_TOKEN"

# Принятие заказа
curl -X POST http://localhost:8080/api/orders/1/accept \
  -H "Authorization: Bearer $DRIVER_TOKEN"

# Завершение поездки
curl -X POST http://localhost:8080/api/orders/1/complete \
  -H "Authorization: Bearer $DRIVER_TOKEN"
```

### 7. История поездок

```bash
curl -X GET http://localhost:8080/api/users/1/orders \
  -H "Authorization: Bearer $TOKEN"
```

---

## Чеклист требований ДЗ №3

- [x] **Проектирование схемы БД** — `schema.sql` с 3 таблицами, PK, FK, типами данных, ограничениями
- [x] **Создание базы данных** — PostgreSQL 16 в Docker, `init-db.sql` для инициализации
- [x] **Тестовые данные** — `data.sql` с 14 пользователями, 10 водителями, 16 заказами
- [x] **Создание индексов** — 7 пользовательских индексов с обоснованием в `schema.sql`
- [x] **SQL запросы** — `queries.sql` для всех 9 API операций + вспомогательные
- [x] **Оптимизация запросов** — `optimization.md` с EXPLAIN ANALYZE до/после
- [x] **Партиционирование** — стратегия Range по `created_at` для таблицы `orders`
- [x] **Подключение API к БД** — миграция с SQLite на PostgreSQL в коде сервисов
- [x] **Dockerfile и docker-compose.yaml** — полная контейнеризация с PostgreSQL
- [x] **README.md** — описание схемы, инструкции, примеры API

---

## Документация API

Полная OpenAPI 3.0 спецификация: [`taxi-service/openapi.yaml`](taxi-service/openapi.yaml)

**Swagger UI** доступен по адресу `http://localhost:8080/swagger/`
