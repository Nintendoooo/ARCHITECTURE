# Event-Driven архитектура Taxi Service

## 1. Обзор архитектуры

Taxi Service трансформируется из синхронной архитектуры в Event-Driven архитектуру с использованием RabbitMQ как брокера сообщений. Это позволяет:

- **Слабую связанность** между сервисами
- **Асинхронную обработку** событий
- **Масштабируемость** через добавление новых потребителей событий
- **Надежность** через гарантии доставки сообщений

## 2. Ключевые события в системе

### 2.1 UserCreated

**Описание:** Событие создания нового пользователя (пассажира или водителя)

**Команда:** CreateUser (POST /api/users)

**Структура payload:**
```json
{
  "event_id": "uuid",
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

**Производитель:** Input Service
**Потенциальные потребители:** 
- Notification Service (отправка приветственного сообщения)
- Analytics Service (сбор статистики)
- Audit Service (логирование)

**Гарантии доставки:** at-least-once

---

### 2.2 DriverRegistered

**Описание:** Событие регистрации нового водителя

**Команда:** RegisterDriver (POST /api/drivers)

**Структура payload:**
```json
{
  "event_id": "uuid",
  "event_type": "DriverRegistered",
  "timestamp": "2024-05-03T11:10:00Z",
  "version": "1.0",
  "data": {
    "driver_id": "507f1f77bcf86cd799439012",
    "user_id": "507f1f77bcf86cd799439011",
    "license_number": "AB123456",
    "car_model": "Toyota Camry",
    "car_plate": "A123BC77",
    "created_at": "2024-05-03T11:10:00Z"
  }
}
```

**Производитель:** Input Service
**Потенциальные потребители:**
- Analytics Service (сбор статистики водителей)
- Audit Service (логирование)

**Гарантии доставки:** at-least-once

---

### 2.3 OrderCreated

**Описание:** Событие создания нового заказа поездки

**Команда:** CreateOrder (POST /api/orders)

**Структура payload:**
```json
{
  "event_id": "uuid",
  "event_type": "OrderCreated",
  "timestamp": "2024-05-03T11:10:00Z",
  "version": "1.0",
  "data": {
    "order_id": "507f1f77bcf86cd799439013",
    "user_id": "507f1f77bcf86cd799439011",
    "pickup_address": "Москва, ул. Тверская, 1",
    "destination_address": "Москва, ул. Арбат, 10",
    "status": "active",
    "created_at": "2024-05-03T11:10:00Z"
  }
}
```

**Производитель:** Order Service
**Потенциальные потребители:**
- Notification Service (отправка уведомления пассажиру)
- Analytics Service (сбор статистики заказов)
- Matching Service (поиск подходящего водителя)

**Гарантии доставки:** at-least-once

---

### 2.4 OrderAccepted

**Описание:** Событие принятия заказа водителем

**Команда:** AcceptOrder (POST /api/orders/{id}/accept)

**Структура payload:**
```json
{
  "event_id": "uuid",
  "event_type": "OrderAccepted",
  "timestamp": "2024-05-03T11:10:00Z",
  "version": "1.0",
  "data": {
    "order_id": "507f1f77bcf86cd799439013",
    "driver_id": "507f1f77bcf86cd799439012",
    "user_id": "507f1f77bcf86cd799439011",
    "pickup_address": "Москва, ул. Тверская, 1",
    "destination_address": "Москва, ул. Арбат, 10",
    "accepted_at": "2024-05-03T11:10:00Z"
  }
}
```

**Производитель:** Match Service
**Потенциальные потребители:**
- Notification Service (отправка уведомления пассажиру о принятии)
- Analytics Service (сбор статистики)

**Гарантии доставки:** at-least-once

---

### 2.5 OrderCompleted

**Описание:** Событие завершения поездки

**Команда:** CompleteOrder (POST /api/orders/{id}/complete)

**Структура payload:**
```json
{
  "event_id": "uuid",
  "event_type": "OrderCompleted",
  "timestamp": "2024-05-03T11:10:00Z",
  "version": "1.0",
  "data": {
    "order_id": "507f1f77bcf86cd799439013",
    "driver_id": "507f1f77bcf86cd799439012",
    "user_id": "507f1f77bcf86cd799439011",
    "pickup_address": "Москва, ул. Тверская, 1",
    "destination_address": "Москва, ул. Арбат, 10",
    "completed_at": "2024-05-03T11:10:00Z"
  }
}
```

**Производитель:** Order Service
**Потенциальные потребители:**
- Analytics Service (сбор статистики завершенных поездок)
- Billing Service (расчет платежа)
- Audit Service (логирование)

**Гарантии доставки:** at-least-once

---

## 3. Архитектура системы

### 3.1 Компоненты системы

```
┌─────────────────────────────────────────────────────────────────┐
│                     Taxi Service System                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────┐  ┌──────────────────┐  ┌───────────────┐  │
│  │  Input Service   │  │  Order Service   │  │ Match Service │  │
│  │                  │  │                  │  │               │  │
│  │ - CreateUser     │  │ - CreateOrder    │  │ - AcceptOrder │  │
│  │ - RegisterDriver │  │ - CompleteOrder  │  │ - GetOrders   │  │
│  │ - GetUser        │  │ - GetOrders      │  │               │  │
│  │ - SearchUser     │  │ - GetActiveOrders│  │               │  │
│  └────────┬─────────┘  └────────┬─────────┘  └────────┬──────┘  │
│           │                     │                     │         │
│           │ Publish Events      │ Publish Events      │         │
│           └─────────────────────┼─────────────────────┘         │
│                                 │                               │
│                    ┌────────────▼────────────┐                  │
│                    │   RabbitMQ Broker       │                  │
│                    │                         │                  │
│                    │ Exchange: taxi-events   │                  │
│                    │ Queue: taxi-events-q    │                  │
│                    └────────────┬────────────┘                  │
│                                 │                               │
│                    ┌────────────▼────────────┐                  │
│                    │  Event Consumers        │                  │
│                    │                         │                  │
│                    │ - Notification Service  │                  │
│                    │ - Analytics Service     │                  │
│                    │ - Billing Service       │                  │
│                    │ - Audit Service         │                  │
│                    └─────────────────────────┘                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 Производители и потребители событий

| Событие | Производитель | Потребители | Тип |
|---------|---------------|-------------|-----|
| UserCreated | Input Service | Notification, Analytics, Audit | Domain |
| DriverRegistered | Input Service | Analytics, Audit | Domain |
| OrderCreated | Order Service | Notification, Analytics, Matching | Domain |
| OrderAccepted | Match Service | Notification, Analytics | Domain |
| OrderCompleted | Order Service | Analytics, Billing, Audit | Domain |

---

## 4. Применение паттерна CQRS

### 4.1 Разделение на Commands и Queries

#### Commands (Write операции)
Команды изменяют состояние системы и публикуют события:

```
Input Service Commands:
├── CreateUser → публикует UserCreated
└── RegisterDriver → публикует DriverRegistered

Order Service Commands:
├── CreateOrder → публикует OrderCreated
└── CompleteOrder → публикует OrderCompleted

Match Service Commands:
└── AcceptOrder → публикует OrderAccepted
```

#### Queries (Read операции)
Запросы только читают данные, не изменяя состояние:

```
Input Service Queries:
├── GetUser
└── SearchUserByNameMask

Order Service Queries:
├── GetOrders
└── GetActiveOrders

Match Service Queries:
└── GetOrders
```

### 4.2 Синхронизация Read и Write моделей

```
┌─────────────────────────────────────────────────────────────┐
│                    CQRS Pattern                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Write Model (Commands)          Read Model (Queries)       │
│  ┌──────────────────────┐        ┌──────────────────────┐   │
│  │ Order Service        │        │ Order Read Model     │   │
│  │ - CreateOrder        │        │ - GetOrders          │   │
│  │ - CompleteOrder      │        │ - GetActiveOrders    │   │
│  │                      │        │                      │   │
│  │ MongoDB (Write DB)   │        │ MongoDB (Read DB)    │   │
│  └──────────┬───────────┘        └──────────┬───────────┘   │
│             │                               │               │
│             │ Publish Events                │               │
│             └───────────────────────────────┘               │
│                    via RabbitMQ                             │
│                                                             │
│  Event: OrderCreated                                        │
│  {                                                          │
│    "order_id": "...",                                       │
│    "user_id": "...",                                        │
│    "pickup_address": "...",                                 │
│    "destination_address": "...",                            │
│    "status": "..."                                          │
│  }                                                          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 4.3 Преимущества CQRS в Taxi Service

1. **Независимое масштабирование** - read и write модели масштабируются отдельно
2. **Оптимизированные запросы** - read модель может быть оптимизирована для быстрого поиска активных заказов
3. **Асинхронная синхронизация** - события обновляют read модель асинхронно
4. **Аудит** - все изменения записываются как события

---

## 5. Выбор RabbitMQ

### 5.1 Обоснование выбора

**RabbitMQ выбран вместо Kafka по следующим причинам:**

1. **Простота интеграции** - встроенная поддержка в userver
2. **Надежность** - гарантии доставки сообщений (at-least-once, exactly-once)
3. **Управление** - встроенный Management UI для мониторинга
4. **Производительность** - достаточна для taxi service
5. **Экосистема** - хорошая документация и примеры

### 5.2 Конфигурация RabbitMQ

```yaml
# RabbitMQ сервис в docker-compose.yaml
rabbitmq:
  image: rabbitmq:3.12-management-alpine
  environment:
    RABBITMQ_DEFAULT_USER: guest
    RABBITMQ_DEFAULT_PASS: guest
    RABBITMQ_DEFAULT_VHOST: /
  ports:
    - "5672:5672"      # AMQP port
    - "15672:15672"    # Management UI
```

### 5.3 Топология RabbitMQ

```
Exchange: taxi-events
├── Type: fanout (все события идут всем потребителям)
├── Durable: true
└── Auto-delete: false

Queue: taxi-events-queue
├── Durable: true
├── Auto-delete: false
├── Binding: taxi-events → taxi-events-queue
└── Consumers:
    ├── Notification Service
    ├── Analytics Service
    ├── Billing Service
    └── Audit Service
```

---

## 6. Гарантии доставки сообщений

### 6.1 At-Least-Once гарантия

**Используется для:** Все события в taxi service

**Как работает:**
1. Producer публикует сообщение в RabbitMQ
2. RabbitMQ подтверждает получение
3. Consumer получает сообщение
4. Consumer обрабатывает сообщение
5. Consumer отправляет ACK (acknowledgment)
6. RabbitMQ удаляет сообщение из очереди

**Гарантия:** Сообщение будет доставлено минимум один раз

**Потенциальная проблема:** Сообщение может быть обработано несколько раз

**Решение:** Consumers должны быть идемпотентными (обработка одного сообщения несколько раз дает тот же результат)

### 6.2 Реализация в коде

```cpp
// Producer - reliable publishing
client_->PublishReliable(
    exchange_,
    routing_key_,
    envelope,
    deadline
);

// Consumer - manual ACK
void Process(std::string message) override {
    try {
        // Обработка сообщения
        ProcessEvent(message);
        // ACK отправляется автоматически при успехе
    } catch (const std::exception& e) {
        // NACK отправляется при ошибке
        // Сообщение вернется в очередь
        throw;
    }
}
```

---

## 7. Поток событий в системе

### 7.1 Пример: Создание пользователя

```
1. Client отправляет POST /api/users
   ↓
2. Input Service получает запрос
   ↓
3. Input Service создает пользователя в MongoDB
   ↓
4. Input Service публикует UserCreated событие в RabbitMQ
   ↓
5. RabbitMQ доставляет событие потребителям:
   ├─→ Notification Service (отправляет приветственное сообщение)
   ├─→ Analytics Service (обновляет статистику)
   └─→ Audit Service (логирует создание)
   ↓
6. Input Service возвращает ответ клиенту
```

### 7.2 Пример: Создание заказа

```
1. Client отправляет POST /api/orders
   ↓
2. Order Service получает запрос
   ↓
3. Order Service создает заказ в MongoDB
   ↓
4. Order Service публикует OrderCreated событие в RabbitMQ
   ↓
5. RabbitMQ доставляет событие потребителям:
   ├─→ Notification Service (отправляет уведомление пассажиру)
   ├─→ Analytics Service (обновляет статистику)
   └─→ Matching Service (начинает поиск водителя)
   ↓
6. Order Service возвращает ответ клиенту
```

---

## 8. Компоненты для реализации

### 8.1 TaxiEventProducer

**Файл:** `src/common/rabbitmq_producer.hpp`

**Функционал:**
- Публикация событий в RabbitMQ
- Методы для каждого типа события:
  - `PublishUserCreated(const User& user)`
  - `PublishDriverRegistered(const Driver& driver)`
  - `PublishOrderCreated(const Order& order)`
  - `PublishOrderAccepted(const Order& order, const std::string& driver_id)`
  - `PublishOrderCompleted(const Order& order)`

### 8.2 TaxiEventConsumer

**Файл:** `src/common/rabbitmq_consumer.hpp`

**Функционал:**
- Потребление событий из RabbitMQ
- Парсинг JSON сообщений
- Логирование потребленных событий
- Хранение обработанных сообщений для тестирования

---

## 9. Интеграция в существующие сервисы

### 9.1 Input Service

**Изменения в `src/input/handlers.cpp`:**

```cpp
// В CreateUserHandler
auto created_user = database_.CreateUser(request);
producer_.PublishUserCreated(created_user);  // Новая строка
return created_user.ToJson();

// В RegisterDriverHandler
auto created_driver = database_.RegisterDriver(request);
producer_.PublishDriverRegistered(created_driver);  // Новая строка
return created_driver.ToJson();
```

### 9.2 Order Service

**Изменения в `src/order/handlers.cpp`:**

```cpp
// В CreateOrderHandler
auto created_order = database_.CreateOrder(request);
producer_.PublishOrderCreated(created_order);  // Новая строка
return created_order.ToJson();

// В CompleteOrderHandler
auto completed_order = database_.CompleteOrder(order_id);
producer_.PublishOrderCompleted(completed_order);  // Новая строка
return completed_order.ToJson();
```

### 9.3 Match Service

**Изменения в `src/match/handlers.cpp`:**

```cpp
// В AcceptOrderHandler
auto accepted_order = database_.AcceptOrder(order_id, driver_id);
producer_.PublishOrderAccepted(accepted_order, driver_id);  // Новая строка
return accepted_order.ToJson();
```

---

## 10. Мониторинг и отладка

### 10.1 RabbitMQ Management UI

Доступен по адресу: `http://localhost:15672`

**Учетные данные:**
- Username: `guest`
- Password: `guest`

**Что можно мониторить:**
- Exchanges и их bindings
- Queues и количество сообщений
- Consumers и их статус
- Message rate (сообщений в секунду)

### 10.2 Логирование

Все события логируются в консоль:

```
[2024-05-03 11:10:00] [info] Publishing UserCreated event: user_id=507f1f77bcf86cd799439011
[2024-05-03 11:10:01] [info] Consumed UserCreated event: user_id=507f1f77bcf86cd799439011
```

---

## 11. Заключение

Event-Driven архитектура с RabbitMQ позволяет Taxi Service:

1. **Масштабироваться** - добавлять новые потребители без изменения producers
2. **Быть надежнее** - асинхронная обработка не блокирует основной поток
3. **Быть гибче** - легко добавлять новые функции через новые consumers
4. **Быть проще** - слабая связанность между компонентами

Применение CQRS паттерна позволяет:

1. **Оптимизировать** - read и write модели независимо
2. **Масштабировать** - read и write масштабируются отдельно
3. **Аудировать** - все изменения записываются как события

Выбор RabbitMQ обоснован его простотой, надежностью и встроенной поддержкой в userver.
