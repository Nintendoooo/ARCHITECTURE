# Каталог событий Taxi Service

## 1. UserCreated

### Описание
Событие создания нового пользователя (пассажира или водителя) в системе.

### Команда (HTTP)
```
POST /api/users
Content-Type: application/json

{
  "login": "john_doe",
  "email": "john@example.com",
  "first_name": "John",
  "last_name": "Doe",
  "phone": "+79001234567",
  "password": "secret123",
  "role": "passenger"
}
```

### Производитель (сервис + файл + класс)
- **Сервис:** Input Service
- **Файл:** `src/input/handlers.cpp`
- **Класс:** `CreateUserHandler`
- **Метод:** `PublishUserCreated(const User& user)`

### Потребители (список с описанием действия)
1. **Notification Service** — отправляет приветственное письмо/SMS
2. **Analytics Service** — обновляет статистику по новым пользователям
3. **Audit Service** — логирует создание пользователя

### Структура Payload (полный JSON с примерами)
```json
{
  "event_id": "550e8400-e29b-41d4-a716-446655440000",
  "event_type": "UserCreated",
  "timestamp": "2024-05-03T11:10:00Z",
  "version": "1.0",
  "data": {
    "user_id": "507f1f77bcf86cd799439011",
    "login": "john_doe",
    "email": "john@example.com",
    "first_name": "John",
    "last_name": "Doe",
    "phone": "+79001234567",
    "created_at": "2024-05-03T11:10:00Z"
  }
}
```

### Поля Payload (таблица: поле, тип, описание)

| Поле | Тип | Описание |
|------|-----|---------|
| event_id | string (UUID) | Уникальный идентификатор события |
| event_type | string | Тип события: "UserCreated" |
| timestamp | ISO-8601 | Время создания события |
| version | string | Версия схемы события: "1.0" |
| data.user_id | string (ObjectId) | Уникальный идентификатор пользователя |
| data.login | string | Логин пользователя |
| data.email | string | Email пользователя |
| data.first_name | string | Имя пользователя |
| data.last_name | string | Фамилия пользователя |
| data.phone | string | Номер телефона |
| data.created_at | ISO-8601 | Время создания пользователя |

### Гарантии доставки
**at-least-once** через `PublishReliable` + manual ACK в consumer

### Обработка ошибок
- Retry через NACK: сообщение возвращается в очередь при ошибке обработки
- Dead Letter Queue: `taxi-events-dlq` для сообщений, которые не удалось обработать после 4 попыток

### Пример кода

**Producer:**
```cpp
void PublishUserCreated(const User& user) {
    formats::json::ValueBuilder builder{formats::json::Type::kObject};
    builder["event_id"] = utils::generators::GenerateUuid();
    builder["event_type"] = "UserCreated";
    builder["timestamp"] = std::chrono::system_clock::now();
    builder["version"] = "1.0";
    
    auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
    data["user_id"] = user.id;
    data["login"] = user.login;
    data["email"] = user.email;
    data["first_name"] = user.first_name;
    data["last_name"] = user.last_name;
    data["phone"] = user.phone;
    data["created_at"] = user.created_at;
    
    builder["data"] = data.ExtractValue();
    PublishEvent(builder.ExtractValue());
}
```

**Consumer:**
```cpp
void ProcessUserCreated(const userver::formats::json::Value& event) {
    auto data = event["data"];
    std::string user_id = data["user_id"].As<std::string>();
    std::string login = data["login"].As<std::string>();
    
    GetLogger()->info("Processing UserCreated: user_id='{}', login='{}'", user_id, login);
    // Отправить приветственное письмо, обновить аналитику, и т.д.
}
```

---

## 2. DriverRegistered

### Описание
Событие регистрации нового водителя в системе с информацией об автомобиле.

### Команда (HTTP)
```
POST /api/drivers
Content-Type: application/json
Authorization: Bearer <JWT_TOKEN>

{
  "license_number": "AB123456",
  "car_model": "Toyota Camry",
  "car_plate": "A001AA77"
}
```

### Производитель (сервис + файл + класс)
- **Сервис:** Input Service
- **Файл:** `src/input/handlers.cpp`
- **Класс:** `RegisterDriverHandler`
- **Метод:** `PublishDriverRegistered(const Driver& driver)`

### Потребители (список с описанием действия)
1. **Analytics Service** — обновляет статистику по водителям
2. **Audit Service** — логирует регистрацию водителя

### Структура Payload (полный JSON с примерами)
```json
{
  "event_id": "550e8400-e29b-41d4-a716-446655440001",
  "event_type": "DriverRegistered",
  "timestamp": "2024-05-03T11:15:00Z",
  "version": "1.0",
  "data": {
    "driver_id": "507f1f77bcf86cd799439012",
    "user_id": "507f1f77bcf86cd799439011",
    "license_number": "AB123456",
    "car_model": "Toyota Camry",
    "car_plate": "A001AA77",
    "created_at": "2024-05-03T11:15:00Z"
  }
}
```

### Поля Payload (таблица: поле, тип, описание)

| Поле | Тип | Описание |
|------|-----|---------|
| event_id | string (UUID) | Уникальный идентификатор события |
| event_type | string | Тип события: "DriverRegistered" |
| timestamp | ISO-8601 | Время создания события |
| version | string | Версия схемы события: "1.0" |
| data.driver_id | string (ObjectId) | Уникальный идентификатор водителя |
| data.user_id | string (ObjectId) | Идентификатор пользователя (водителя) |
| data.license_number | string | Номер водительского удостоверения |
| data.car_model | string | Модель автомобиля |
| data.car_plate | string | Номерной знак автомобиля |
| data.created_at | ISO-8601 | Время регистрации водителя |

### Гарантии доставки
**at-least-once** через `PublishReliable` + manual ACK в consumer

### Обработка ошибок
- Retry через NACK: сообщение возвращается в очередь при ошибке обработки
- Dead Letter Queue: `taxi-events-dlq` для сообщений, которые не удалось обработать после 4 попыток

### Пример кода

**Producer:**
```cpp
void PublishDriverRegistered(const Driver& driver) {
    formats::json::ValueBuilder builder{formats::json::Type::kObject};
    builder["event_id"] = utils::generators::GenerateUuid();
    builder["event_type"] = "DriverRegistered";
    builder["timestamp"] = std::chrono::system_clock::now();
    builder["version"] = "1.0";
    
    auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
    data["driver_id"] = driver.id;
    data["user_id"] = driver.user_id;
    data["license_number"] = driver.license_number;
    data["car_model"] = driver.car_model;
    data["car_plate"] = driver.car_plate;
    data["created_at"] = driver.created_at;
    
    builder["data"] = data.ExtractValue();
    PublishEvent(builder.ExtractValue());
}
```

**Consumer:**
```cpp
void ProcessDriverRegistered(const userver::formats::json::Value& event) {
    auto data = event["data"];
    std::string driver_id = data["driver_id"].As<std::string>();
    std::string car_plate = data["car_plate"].As<std::string>();
    
    GetLogger()->info("Processing DriverRegistered: driver_id='{}', car_plate='{}'", 
                    driver_id, car_plate);
    // Обновить аналитику, логировать, и т.д.
}
```

---

## 3. OrderCreated

### Описание
Событие создания нового заказа поездки пассажиром.

### Команда (HTTP)
```
POST /api/orders
Content-Type: application/json
Authorization: Bearer <JWT_TOKEN>

{
  "passenger_name": "John Doe",
  "passenger_phone": "+79001234567",
  "pickup_address": "Москва, ул. Тверская, 1",
  "destination_address": "Москва, ул. Арбат, 10"
}
```

### Производитель (сервис + файл + класс)
- **Сервис:** Order Service
- **Файл:** `src/order/handlers.cpp`
- **Класс:** `CreateOrderHandler`
- **Метод:** `PublishOrderCreated(const Order& order)`

### Потребители (список с описанием действия)
1. **Notification Service** — отправляет уведомление пассажиру о создании заказа
2. **Analytics Service** — обновляет статистику по заказам
3. **Matching Service** — начинает поиск подходящего водителя

### Структура Payload (полный JSON с примерами)
```json
{
  "event_id": "550e8400-e29b-41d4-a716-446655440002",
  "event_type": "OrderCreated",
  "timestamp": "2024-05-03T11:20:00Z",
  "version": "1.0",
  "data": {
    "order_id": "507f1f77bcf86cd799439013",
    "user_id": "507f1f77bcf86cd799439011",
    "pickup_address": "Москва, ул. Тверская, 1",
    "destination_address": "Москва, ул. Арбат, 10",
    "status": "active",
    "created_at": "2024-05-03T11:20:00Z"
  }
}
```

### Поля Payload (таблица: поле, тип, описание)

| Поле | Тип | Описание |
|------|-----|---------|
| event_id | string (UUID) | Уникальный идентификатор события |
| event_type | string | Тип события: "OrderCreated" |
| timestamp | ISO-8601 | Время создания события |
| version | string | Версия схемы события: "1.0" |
| data.order_id | string (ObjectId) | Уникальный идентификатор заказа |
| data.user_id | string (ObjectId) | Идентификатор пассажира |
| data.pickup_address | string | Адрес подачи |
| data.destination_address | string | Адрес назначения |
| data.status | string | Статус заказа: "active" |
| data.created_at | ISO-8601 | Время создания заказа |

### Гарантии доставки
**at-least-once** через `PublishReliable` + manual ACK в consumer

### Обработка ошибок
- Retry через NACK: сообщение возвращается в очередь при ошибке обработки
- Dead Letter Queue: `taxi-events-dlq` для сообщений, которые не удалось обработать после 4 попыток

### Пример кода

**Producer:**
```cpp
void PublishOrderCreated(const Order& order) {
    formats::json::ValueBuilder builder{formats::json::Type::kObject};
    builder["event_id"] = utils::generators::GenerateUuid();
    builder["event_type"] = "OrderCreated";
    builder["timestamp"] = std::chrono::system_clock::now();
    builder["version"] = "1.0";
    
    auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
    data["order_id"] = order.id;
    data["user_id"] = order.user_id;
    data["pickup_address"] = order.pickup_address;
    data["destination_address"] = order.destination_address;
    data["status"] = order.status;
    data["created_at"] = order.created_at;
    
    builder["data"] = data.ExtractValue();
    PublishEvent(builder.ExtractValue());
}
```

**Consumer:**
```cpp
void ProcessOrderCreated(const userver::formats::json::Value& event) {
    auto data = event["data"];
    std::string order_id = data["order_id"].As<std::string>();
    std::string pickup_address = data["pickup_address"].As<std::string>();
    
    GetLogger()->info("Processing OrderCreated: order_id='{}', pickup_address='{}'", 
                    order_id, pickup_address);
    // Отправить уведомление, обновить аналитику, начать поиск водителя, и т.д.
}
```

---

## 4. OrderAccepted

### Описание
Событие принятия заказа водителем.

### Команда (HTTP)
```
POST /api/orders/{order_id}/accept
Content-Type: application/json
Authorization: Bearer <JWT_TOKEN>

{
  "driver_id": "507f1f77bcf86cd799439012"
}
```

### Производитель (сервис + файл + класс)
- **Сервис:** Match Service
- **Файл:** `src/match/handlers.cpp`
- **Класс:** `AcceptOrderHandler`
- **Метод:** `PublishOrderAccepted(const Order& order, const std::string& driver_id)`

### Потребители (список с описанием действия)
1. **Notification Service** — отправляет уведомление пассажиру о принятии заказа
2. **Analytics Service** — обновляет статистику по принятым заказам

### Структура Payload (полный JSON с примерами)
```json
{
  "event_id": "550e8400-e29b-41d4-a716-446655440003",
  "event_type": "OrderAccepted",
  "timestamp": "2024-05-03T11:25:00Z",
  "version": "1.0",
  "data": {
    "order_id": "507f1f77bcf86cd799439013",
    "driver_id": "507f1f77bcf86cd799439012",
    "user_id": "507f1f77bcf86cd799439011",
    "pickup_address": "Москва, ул. Тверская, 1",
    "destination_address": "Москва, ул. Арбат, 10",
    "accepted_at": "2024-05-03T11:25:00Z"
  }
}
```

### Поля Payload (таблица: поле, тип, описание)

| Поле | Тип | Описание |
|------|-----|---------|
| event_id | string (UUID) | Уникальный идентификатор события |
| event_type | string | Тип события: "OrderAccepted" |
| timestamp | ISO-8601 | Время создания события |
| version | string | Версия схемы события: "1.0" |
| data.order_id | string (ObjectId) | Уникальный идентификатор заказа |
| data.driver_id | string (ObjectId) | Идентификатор водителя |
| data.user_id | string (ObjectId) | Идентификатор пассажира |
| data.pickup_address | string | Адрес подачи |
| data.destination_address | string | Адрес назначения |
| data.accepted_at | ISO-8601 | Время принятия заказа |

### Гарантии доставки
**at-least-once** через `PublishReliable` + manual ACK в consumer

### Обработка ошибок
- Retry через NACK: сообщение возвращается в очередь при ошибке обработки
- Dead Letter Queue: `taxi-events-dlq` для сообщений, которые не удалось обработать после 4 попыток

### Пример кода

**Producer:**
```cpp
void PublishOrderAccepted(const Order& order, const std::string& driver_id) {
    formats::json::ValueBuilder builder{formats::json::Type::kObject};
    builder["event_id"] = utils::generators::GenerateUuid();
    builder["event_type"] = "OrderAccepted";
    builder["timestamp"] = std::chrono::system_clock::now();
    builder["version"] = "1.0";
    
    auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
    data["order_id"] = order.id;
    data["driver_id"] = driver_id;
    data["user_id"] = order.user_id;
    data["pickup_address"] = order.pickup_address;
    data["destination_address"] = order.destination_address;
    data["accepted_at"] = std::chrono::system_clock::now();
    
    builder["data"] = data.ExtractValue();
    PublishEvent(builder.ExtractValue());
}
```

**Consumer:**
```cpp
void ProcessOrderAccepted(const userver::formats::json::Value& event) {
    auto data = event["data"];
    std::string order_id = data["order_id"].As<std::string>();
    std::string driver_id = data["driver_id"].As<std::string>();
    
    GetLogger()->info("Processing OrderAccepted: order_id='{}', driver_id='{}'", 
                    order_id, driver_id);
    // Отправить уведомление пассажиру, обновить аналитику, и т.д.
}
```

---

## 5. OrderCompleted

### Описание
Событие завершения поездки.

### Команда (HTTP)
```
POST /api/orders/{order_id}/complete
Content-Type: application/json
Authorization: Bearer <JWT_TOKEN>

{
  "rating": 5,
  "comment": "Great ride!"
}
```

### Производитель (сервис + файл + класс)
- **Сервис:** Order Service
- **Файл:** `src/order/handlers.cpp`
- **Класс:** `CompleteOrderHandler`
- **Метод:** `PublishOrderCompleted(const Order& order)`

### Потребители (список с описанием действия)
1. **Analytics Service** — обновляет статистику по завершенным поездкам
2. **Billing Service** — рассчитывает платеж и создает счет
3. **Audit Service** — логирует завершение поездки

### Структура Payload (полный JSON с примерами)
```json
{
  "event_id": "550e8400-e29b-41d4-a716-446655440004",
  "event_type": "OrderCompleted",
  "timestamp": "2024-05-03T11:35:00Z",
  "version": "1.0",
  "data": {
    "order_id": "507f1f77bcf86cd799439013",
    "driver_id": "507f1f77bcf86cd799439012",
    "user_id": "507f1f77bcf86cd799439011",
    "pickup_address": "Москва, ул. Тверская, 1",
    "destination_address": "Москва, ул. Арбат, 10",
    "completed_at": "2024-05-03T11:35:00Z"
  }
}
```

### Поля Payload (таблица: поле, тип, описание)

| Поле | Тип | Описание |
|------|-----|---------|
| event_id | string (UUID) | Уникальный идентификатор события |
| event_type | string | Тип события: "OrderCompleted" |
| timestamp | ISO-8601 | Время создания события |
| version | string | Версия схемы события: "1.0" |
| data.order_id | string (ObjectId) | Уникальный идентификатор заказа |
| data.driver_id | string (ObjectId) | Идентификатор водителя |
| data.user_id | string (ObjectId) | Идентификатор пассажира |
| data.pickup_address | string | Адрес подачи |
| data.destination_address | string | Адрес назначения |
| data.completed_at | ISO-8601 | Время завершения поездки |

### Гарантии доставки
**at-least-once** через `PublishReliable` + manual ACK в consumer

### Обработка ошибок
- Retry через NACK: сообщение возвращается в очередь при ошибке обработки
- Dead Letter Queue: `taxi-events-dlq` для сообщений, которые не удалось обработать после 4 попыток

### Пример кода

**Producer:**
```cpp
void PublishOrderCompleted(const Order& order) {
    formats::json::ValueBuilder builder{formats::json::Type::kObject};
    builder["event_id"] = utils::generators::GenerateUuid();
    builder["event_type"] = "OrderCompleted";
    builder["timestamp"] = std::chrono::system_clock::now();
    builder["version"] = "1.0";
    
    auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
    data["order_id"] = order.id;
    data["driver_id"] = order.driver_id;
    data["user_id"] = order.user_id;
    data["pickup_address"] = order.pickup_address;
    data["destination_address"] = order.destination_address;
    data["completed_at"] = std::chrono::system_clock::now();
    
    builder["data"] = data.ExtractValue();
    PublishEvent(builder.ExtractValue());
}
```

**Consumer:**
```cpp
void ProcessOrderCompleted(const userver::formats::json::Value& event) {
    auto data = event["data"];
    std::string order_id = data["order_id"].As<std::string>();
    std::string driver_id = data["driver_id"].As<std::string>();
    
    GetLogger()->info("Processing OrderCompleted: order_id='{}', driver_id='{}'", 
                    order_id, driver_id);
    // Обновить аналитику, рассчитать платеж, логировать, и т.д.
}
```

---

## Сравнение всех 5 событий

| Событие | Производитель | Потребители | Статус | Примечание |
|---------|---------------|-------------|--------|-----------|
| UserCreated | Input Service | Notification, Analytics, Audit | ✅ | Базовое событие |
| DriverRegistered | Input Service | Analytics, Audit | ✅ | Специфично для водителей |
| OrderCreated | Order Service | Notification, Analytics, Matching | ✅ | Инициирует поиск водителя |
| OrderAccepted | Match Service | Notification, Analytics | ✅ | Завершает поиск водителя |
| OrderCompleted | Order Service | Analytics, Billing, Audit | ✅ | Финальное событие |

---

## Топология RabbitMQ

```
Exchange: taxi-events
├── Type: fanout (все события идут всем потребителям)
├── Durable: true
├── Auto-delete: false
└── Bindings:
    └── taxi-events-queue

Queue: taxi-events-queue
├── Durable: true
├── Auto-delete: false
├── Messages: 0 (при нормальной работе)
└── Consumers:
    ├── Notification Service
    ├── Analytics Service
    ├── Billing Service
    └── Audit Service

Dead Letter Queue: taxi-events-dlq
├── Durable: true
├── Auto-delete: false
└── Назначение: сообщения, которые не удалось обработать после 4 попыток
```

---

## Версионирование событий

### Стратегия версионирования

**Текущая версия:** 1.0

**Правила:**
- **Minor версия (1.0 → 1.1):** добавление новых опциональных полей
- **Major версия (1.0 → 2.0):** удаление или переименование полей

### Пример миграции (1.0 → 1.1)

Если нужно добавить поле `estimated_price` в `OrderCreated`:

```json
{
  "event_id": "...",
  "event_type": "OrderCreated",
  "timestamp": "...",
  "version": "1.1",  // ← обновлена версия
  "data": {
    "order_id": "...",
    "user_id": "...",
    "pickup_address": "...",
    "destination_address": "...",
    "status": "...",
    "estimated_price": 450.00,  // ← новое поле
    "created_at": "..."
  }
}
```

Consumer должен быть готов обрабатывать обе версии (1.0 и 1.1).

---

## Retry логика

### Конфигурация

- **Максимум попыток:** 4
- **Интервал между попытками:** 1 сек (экспоненциальная задержка)
- **Dead Letter Queue:** `taxi-events-dlq`

### Поток обработки

```
1. Consumer получает сообщение
   ↓
2. Попытка обработки (попытка 1)
   ├─ Успех → ACK, сообщение удалено
   └─ Ошибка → NACK, сообщение возвращается в очередь
   ↓
3. Повторная попытка (попытка 2)
   ├─ Успех → ACK, сообщение удалено
   └─ Ошибка → NACK, сообщение возвращается в очередь
   ↓
4. Повторная попытка (попытка 3)
   ├─ Успех → ACK, сообщение удалено
   └─ Ошибка → NACK, сообщение возвращается в очередь
   ↓
5. Последняя попытка (попытка 4)
   ├─ Успех → ACK, сообщение удалено
   └─ Ошибка → отправить в Dead Letter Queue
   ↓
6. Dead Letter Queue
   └─ Требует ручного вмешательства для анализа и исправления
```

---

## Заключение

Каталог событий определяет полную топологию Event-Driven системы Taxi Service:

1. **5 основных событий** покрывают весь жизненный цикл заказа
2. **at-least-once гарантия** обеспечивает надежность доставки
3. **Версионирование** позволяет эволюционировать события без нарушения совместимости
4. **Retry логика** обеспечивает обработку временных ошибок
5. **Dead Letter Queue** позволяет отследить и исправить критические ошибки
