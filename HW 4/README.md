# Отчёт по домашнему заданию №4: Миграция на MongoDB

**Вариант:** №16 — Система заказа такси (Uber)

---

## Что было сделано

Выполнена миграция REST API сервиса заказа такси с PostgreSQL на MongoDB 7.0. Спроектированы коллекции с валидацией схемы, реализованы все операции с использованием MongoDB C++ Driver, проведено тестирование всех API endpoints.

### Изменения относительно ДЗ №3

| Компонент | ДЗ №3 (PostgreSQL) | ДЗ №4 (MongoDB) |
|---|---|---|
| СУБД | PostgreSQL 16 | MongoDB 7.0 |
| Драйвер | `userver::storages::postgres` | `mongocxx` |
| ID сущностей | BIGSERIAL (int64_t) | ObjectId (строка 24 символа) |
| Инициализация | `init-db.sql` | `init-mongo.js` |
| Тестовые данные | `data.sql` | `data.js` |
| Запросы | SQL (pqxx) | BSON (mongocxx) |
| Индексы | DDL в schema.sql | MongoDB shell в init-mongo.js |

---

## Архитектура решения

Микросервисная архитектура с 3 сервисами + MongoDB + Nginx:

```
Клиент/Водитель → Nginx :8080 → Input Service  :8081
                              → Order Service  :8082
                              → Match Service  :8083
                                     ↓
                              MongoDB :27017
                              (taxi_db)
```

- **Input Service** (порт 8081) — регистрация пользователей и водителей, аутентификация, поиск
- **Order Service** (порт 8082) — создание заказов, история поездок, завершение поездки
- **Match Service** (порт 8083) — просмотр активных заказов, принятие заказа водителем
- **MongoDB** (порт 27017) — документная БД с валидацией схемы
- **Nginx** (порт 8080) — API Gateway, единая точка входа

---

## Схема базы данных

### Коллекции MongoDB

```
┌──────────────────────┐       ┌─────────────────────┐     ┌──────────────────────┐
│      users           │       │     drivers         │     │       orders         │
├──────────────────────┤       ├─────────────────────┤     ├──────────────────────┤
│ _id         ObjectId │       │ _id         ObjectId│     │ _id         ObjectId │
│ login       string   │       │ user_id     string  │     │ user_id     string   │──► users._id
│ email       string   │       │ license_number      │     │ driver_id   string   │──► drivers._id
│ first_name  string   │       │ car_model           │     │ passenger   object   │   (embedded)
│ last_name   string   │       │ car_plate           │     │ pickup_address       │ 
│ phone       string   │       │ is_available bool   │     │ destination_address  │
│ password_hash        │       │ created_at          │     │ status         string│
│ role         string  │       │ updated_at          │     │ price         double │
│ created_at   datetime│       └─────────────────────┘     │ created_at   datetime│
│ updated_at   datetime│                                   │ completed_at datetime│
└──────────────────────┘                                   └──────────────────────┘
```

### Валидация схемы (JSON Schema)

Каждая коллекция использует `$jsonSchema` валидацию:

**users** — пользователи системы
- `_id` — ObjectId (автогенерируется MongoDB)
- `login` — строка, уникальный индекс
- `email` — строка, уникальный индекс
- `first_name`, `last_name` — строка, обязательное поле
- `phone` — строка, формат `+...`
- `password_hash` — строка (SHA-256)
- `role` — строка: `passenger` | `driver`
- `created_at`, `updated_at` — datetime

**drivers** — профили водителей
- `user_id` — string (ссылка на users._id)
- `license_number`, `car_model`, `car_plate` — строки
- `is_available` — boolean (по умолчанию true)
- `created_at`, `updated_at` — datetime

**orders** — заказы поездок
- `user_id` — string (ссылка на users._id)
- `driver_id` — string (nullable, ссылка на drivers._id)
- `passenger` — встроенный объект с данными пользователя
- `pickup_address`, `destination_address` — строки
- `status` — строка: `active` | `accepted` | `completed` | `cancelled`
- `price` — double
- `created_at`, `completed_at` — datetime

### Индексы MongoDB

| Индекс | Коллекция | Поля | Назначение |
|---|---|---|---|
| `login_unique` | users | `login` | Уникальный поиск по логину |
| `email_unique` | users | `email` | Уникальный поиск по email |
| `user_id_unique` | drivers | `user_id` | Связь 1:1 с users |
| `status_idx` | orders | `status` | Фильтрация по статусу |
| `user_id_idx` | orders | `user_id` | История поездок |
| `driver_id_idx` | orders | `driver_id` | Заказы водителя |

---

## Реализованные API endpoints

Все 9 требуемых операций протестированы и работают:

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

**Важно:** ID в URL теперь являются строками (MongoDB ObjectId), например:
- `/api/orders/679f1a2b3c4d5e6f7a8b9c0d/accept`
- `/api/users/679f1a2b3c4d5e6f7a8b9c0d/orders`

---

## Стек технологий

- **C++20** — основной код
- **Yandex uServer v2.6** — асинхронный веб-фреймворк
- **MongoDB 7.0** — документная СУБД
- **mongocxx** — MongoDB C++ драйвер
- **JWT-CPP v0.7.0** — аутентификация (HS256)
- **Docker + Docker Compose** — контейнеризация
- **Nginx** — reverse proxy с regex routing для ObjectId

---

## Структура файлов

```
taxi-service/
├── schema_design.md        # Проектирование документной модели (обоснование embedded vs references)
├── init-mongo.js           # Инициализация БД (создание коллекций, индексы, валидация)
├── data.js                 # Тестовые данные (8 users, 10 drivers, 20 orders)
├── queries.js              # Примеры запросов к MongoDB
├── validation.js           # Тесты валидации схемы
├── CMakeLists.txt          # Сборка с mongocxx
├── Dockerfile              # Сборка сервисов
├── docker-compose.yaml     # MongoDB + 3 сервиса + Nginx
├── nginx.conf              # Reverse proxy с regex для ObjectId
├── openapi.yaml            # OpenAPI 3.0 спецификация
├── configs/
│   ├── static_config_mongo.yaml    # Общая конфигурация MongoDB
│   ├── static_config_input.yaml    # Конфиг Input Service
│   ├── static_config_order.yaml    # Конфиг Order Service
│   └── static_config_match.yaml    # Конфиг Match Service
├── src/
│   ├── common/
│   │   ├── database.hpp    # Database class с MongoClient
│   │   ├── database.cpp    # MongoDB реализация всех операций
│   │   ├── models.hpp      # Структуры User, Driver, Order
│   │   ├── models.cpp      # JSON сериализация/десериализация
│   │   └── mongo_component.hpp # MongoDB компонент для userver
│   ├── auth/
│   │   ├── jwt_auth_checker.hpp/cpp   # JWT middleware
│   │   ├── jwt_auth_factory.hpp/cpp   # Фабрика аутентификации
│   │   └── jwt_auth_checker.cpp
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
cd "ARCHITECTURE/HW 4/taxi-service"
docker-compose build
docker-compose up -d
```

Дождитесь, пока все контейнеры станут `healthy` (~2-3 минуты на первую сборку):

```bash
docker-compose ps
```

### Загрузка тестовых данных

```bash
docker exec -i taxi-mongodb mongosh -u taxi_user -p taxi_pass --authenticationDatabase admin taxi_db < data.js
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

Для полной очистки (включая данные MongoDB):

```bash
docker-compose down -v
```

---

## Запуск MongoDB скриптов

### queries.js — примеры запросов

```bash
docker exec -i taxi-mongodb mongosh -u taxi_user -p taxi_pass --authenticationDatabase taxi_db < queries.js
```

Включает: Create, Read, Update, Delete операции + агрегации ($match, $group, $sort, $limit)

### validation.js — тесты валидации схемы

```bash
docker exec -i taxi-mongodb mongosh -u taxi_user -p taxi_pass --authenticationDatabase taxi_db < validation.js
```

Тесты проверяют отклонение невалидных данных (email, phone, role, required fields, price constraints)

### data.js — тестовые данные

```bash
docker exec -i taxi-mongodb mongosh -u taxi_user -p taxi_pass --authenticationDatabase admin taxi_db < data.js
```

Загружает 8 пользователей, 10 водителей, 20 заказов

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

# Принятие заказа (используйте ObjectId из предыдущего запроса)
curl -X POST "http://localhost:8080/api/orders/679f1a2b3c4d5e6f7a8b9c0d/accept" \
  -H "Authorization: Bearer $DRIVER_TOKEN"

# Завершение поездки
curl -X POST "http://localhost:8080/api/orders/679f1a2b3c4d5e6f7a8b9c0d/complete" \
  -H "Authorization: Bearer $DRIVER_TOKEN"
```

### 7. История поездок

```bash
curl -X GET "http://localhost:8080/api/users/679f1a2b3c4d5e6f7a8b9c0d/orders" \
  -H "Authorization: Bearer $TOKEN"
```

---

## Тестовые пользователи

После загрузки `data.js` доступны:

### Пассажиры
- `ivan_petrov` — Иван Петров, +79001234567
- `anna_smirnova` — Анна Смирнова, +79007654321
- `alex_kuznetsov` — Алексей Кузнецов, +79005557777
- `olga_ivanova` — Ольга Иванова, +79008889999
- `petr_sokolov` — Петр Соколов, +79001112233

### Водители
- `mike_driver` — Михаил Сидоров, Toyota Camry, А777АА77
- `sergey_driver` — Сергей Волков, Kia Optima, А888АА88
- `dmitry_driver` — Дмитрий Новиков, Hyundai Sonata, А999АА99

**Пароль для всех:** `securePass123`

---

## Реализация CRUD операций

В файле [`queries.js`](taxi-service/queries.js) реализованы все требуемые операции:

### CREATE (вставка документов)
| Операция | Запрос | Описание |
|----------|--------|----------|
| Q1 | `db.users.insertOne()` | Создание пассажира |
| Q2 | `db.drivers.insertOne()` | Создание профиля водителя |
| Q3 | `db.orders.insertOne()` | Создание заказа |

### READ (поиск документов)
| Операция | Запрос | Описание |
|----------|--------|----------|
| Q4 | `db.users.findOne({login: ...})` | Поиск пользователя по логину |
| Q5 | `db.users.find({$or: [...]})` | Поиск по маске имени ($or + $regex) |
| Q6 | `db.users.findOne({_id: ...})` | Поиск по ID |
| Q7 | `db.drivers.findOne({user_id: ...})` | Поиск водителя по user_id |
| Q8 | `db.drivers.find({is_available: true})` | Доступные водители |
| Q9 | `db.orders.find({status: "active"})` | Активные заказы |
| Q11 | `db.orders.find({user_id: ...})` | Заказы пользователя |
| Q12 | `db.orders.find({driver_id: ...})` | Заказы водителя |

### UPDATE (обновление документов)
| Операция | Запрос | Описание |
|----------|--------|----------|
| Q13 | `db.users.updateOne({...}, {$set: {...}})` | Обновление телефона пользователя |
| Q14 | `db.drivers.updateOne({...}, {$set: {...}})` | Изменение доступности водителя |
| Q15 | `db.orders.updateOne({...}, {$set: {...}})` | Назначение водителя на заказ |
| Q16 | `db.orders.updateOne({...}, {$set: {...}})` | Завершение заказа |
| Q17 | `db.orders.updateOne({...}, {$set: {...}})` | Отмена заказа |
| Q18 | `db.drivers.updateOne({...}, {$inc: {...}})` | Инкремент рейтинга ($inc) |

### DELETE (удаление документов)
| Операция | Запрос | Описание |
|----------|--------|----------|
| Q19 | `db.orders.deleteOne({...})` | Удаление заказа |
| Q20 | `db.drivers.deleteOne({...})` | Удаление водителя |
| Q21 | `db.users.deleteOne({...})` | Удаление пользователя |

### Использованные операторы MongoDB
- `$eq` — неявно в запросах `{field: value}`
- `$ne` — `{price: {$ne: null}}`, `{driver_id: {$ne: null}}`
- `$or` — комбинация условий поиска
- `$regex` — поиск по маске `{first_name: {$regex: "Иван", $options: "i"}}`
- `$currentDate` — автоматическое обновление timestamp
- `$inc` — инкремент числовых полей

### Дополнительно: агрегации
- Q22: Группировка заказов по статусу
- Q23: Средняя цена завершённых заказов
- Q24: Топ водителей по количеству выполненных заказов

---

## Чеклист требований ДЗ №4

- [x] **Проектирование коллекций** — 3 коллекции с JSON Schema валидацией
- [x] **Документная модель** — `schema_design.md` с обоснованием embedded vs references
- [x] **Создание базы данных** — MongoDB 7.0 в Docker, `init-mongo.js` для инициализации
- [x] **Тестовые данные** — `data.js` с 8 пользователями, 10 водителями, 20 заказами
- [x] **Создание индексов** — 6 индексов (уникальные, для поиска, для фильтрации)
- [x] **CRUD операции** — `queries.js` с Create/Read/Update/Delete + операторы $eq, $ne, $or, $regex, $inc, $currentDate
- [x] **Валидация схемы** — `validation.js` с тестами
- [x] **Подключение API к БД** — миграция с PostgreSQL на MongoDB в коде сервисов
- [x] **Dockerfile и docker-compose.yaml** — полная контейнеризация с MongoDB
- [x] **Тестирование endpoints** — все 9 API операций проверены через curl
- [x] **README.md** — описание миграции, инструкции, примеры API, CRUD операции

---

## Устранение неполадок

### Сервисы не запускаются

```bash
# Проверить логи
docker-compose logs mongodb
docker-compose logs input-service
docker-compose logs order-service
docker-compose logs match-service
```

### Проблемы с подключением к MongoDB

```bash
# Проверить, что MongoDB запущена
docker exec taxi-mongodb mongosh -u taxi_user -p taxi_pass --authenticationDatabase taxi_db --eval "db.version()"
```

---

## Документация API

Полная OpenAPI 3.0 спецификация: [`taxi-service/openapi.yaml`](taxi-service/openapi.yaml)

**Swagger UI** доступен по адресу `http://localhost:8080/swagger/`
