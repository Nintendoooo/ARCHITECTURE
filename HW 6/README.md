# Отчёт по домашнему заданию №6: Event-Driven архитектура

**Вариант:** №16 — Система заказа такси (Taxi Service)

---

## Оглавление

1. [Что было сделано](#что-было-сделано)
2. [Архитектура системы](#архитектура-системы)
3. [Анализ событий и команд](#анализ-событий-и-команд)
4. [Брокер сообщений (RabbitMQ)](#брокер-сообщений-rabbitmq)
5. [Применение CQRS](#применение-cqrs)
6. [Реализация Producer и Consumer](#реализация-producer-и-consumer)
7. [Каталог событий](#каталог-событий)
8. [API Endpoints](#api-endpoints)
9. [Тестирование](#тестирование)
10. [Инструкции по запуску](#инструкции-по-запуску)
11. [Мониторинг](#мониторинг)
12. [Структура проекта](#структура-проекта)

**Дополнительные материалы:**
- [`event_driven_design.md`](taxi-service/event_driven_design.md) — проектирование Event-Driven архитектуры
- [`event_catalog.md`](taxi-service/event_catalog.md) — каталог событий с описанием
- [`QUICK_START.md`](QUICK_START.md) — быстрый старт
- [`TEST_EVENT_DRIVEN.md`](TEST_EVENT_DRIVEN.md) — документация тестирования

---

## Что было сделано

Реализована **Event-Driven архитектура** для системы заказа такси с использованием RabbitMQ и паттерна CQRS.

### Новое в HW6

| Компонент | Описание | Файл |
|-----------|----------|------|
| **RabbitMQ** | Брокер сообщений в docker-compose | [`docker-compose.yaml`](taxi-service/docker-compose.yaml) |
| **TaxiEventProducer** | Публикация событий в RabbitMQ | [`rabbitmq_producer.hpp`](taxi-service/src/common/rabbitmq_producer.hpp) |
| **TaxiEventConsumer** | Потребление событий из RabbitMQ | [`rabbitmq_consumer.hpp`](taxi-service/src/common/rabbitmq_consumer.hpp) |
| **Документация** | Event-Driven дизайн и каталог событий | [`event_driven_design.md`](taxi-service/event_driven_design.md), [`event_catalog.md`](taxi-service/event_catalog.md) |
| **Тестовый скрипт** | End-to-end тест Event-Driven pipeline | [`test_event_driven.sh`](taxi-service/tests/test_event_driven.sh) |

---

## Архитектура системы

```
┌─────────────────────────────────────────────────────────────────────┐
│                     Taxi Service (ARCHITECTURE/HW 6)                │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  Nginx API Gateway (host port 8080 → container port 80)      │   │
│  │  Маршрутизация /api/... → микросервисы                       │   │
│  └──────────────────────────────────────────────────────────────┘   │
│         │                    │                    │                 │
│         ▼                    ▼                    ▼                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐           │
│  │Input Service │  │Order Service │  │ Match Service    │           │
│  │  Port 8081   │  │  Port 8082   │  │   Port 8083      │           │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────────┘           │
│         │                 │                  │                      │
│         │  PublishUserCreated  PublishOrderCreated                  │
│         │  PublishDriverRegistered  PublishOrderCompleted           │
│         │                 │  PublishOrderAccepted                   │
│         └─────────────────┼──────────────────┘                      │
│                           │                                         │
│                           ▼                                         │
│                  ┌──────────────────┐                               │
│                  │   RabbitMQ       │                               │
│                  │ AMQP: 5672       │                               │
│                  │ UI:   15672      │                               │
│                  │                  │                               │
│                  │ Exchange:        │                               │
│                  │  taxi-events     │                               │
│                  │  (type: fanout)  │                               │
│                  │                  │                               │
│                  │ Queue:           │                               │
│                  │  taxi-events-    │                               │
│                  │  queue           │                               │
│                  └──────────────────┘                               │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │              MongoDB 7.0 (Port 27017)                        │   │
│  │         Write Database для всех сервисов                     │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  Мониторинг                                                  │   │
│  │  Prometheus (9090) │ Grafana (3000)                          │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Анализ событий и команд

| Команда | HTTP Endpoint | Событие | Статус |
|---------|---------------|---------|--------|
| CreateUser | `POST /api/users` | `UserCreated` | ✅ |
| RegisterDriver | `POST /api/drivers` | `DriverRegistered` | ✅ |
| CreateOrder | `POST /api/orders` | `OrderCreated` | ✅ |
| AcceptOrder | `POST /api/orders/{id}/accept` | `OrderAccepted` | ✅ |
| CompleteOrder | `POST /api/orders/{id}/complete` | `OrderCompleted` | ✅ |
| GetUser | `GET /api/users/{id}` | — | ❌ |
| ListOrders | `GET /api/orders` | — | ❌ |
| GetActiveOrders | `GET /api/orders/active` | — | ❌ |

---

## Брокер сообщений (RabbitMQ)

### Топология

- **Exchange:** `taxi-events` (type: fanout, durable)
- **Queue:** `taxi-events-queue` (durable)
- **Routing Key:** `taxi-routing-key`
- **Delivery Guarantee:** at-least-once (PublishReliable + manual ACK)

### Конфигурация в docker-compose

```yaml
rabbitmq:
  image: rabbitmq:3.12-management-alpine
  container_name: taxi-rabbitmq
  environment:
    RABBITMQ_DEFAULT_USER: guest
    RABBITMQ_DEFAULT_PASS: guest
    RABBITMQ_DEFAULT_VHOST: /
  ports:
    - "5672:5672"      # AMQP
    - "15672:15672"    # Management UI
```

### Доступ к Management UI

- **URL:** http://localhost:15672
- **Логин:** guest
- **Пароль:** guest

### Гарантии доставки

Используется паттерн **at-least-once** через:
1. `PublishReliable()` — гарантирует, что сообщение попадёт в RabbitMQ
2. Manual ACK — потребитель подтверждает обработку события
3. Dead Letter Queue (опционально) — для обработки ошибок

---

## Применение CQRS

| Тип | Операция | Метод | Генерирует событие |
|-----|----------|-------|-------------------|
| Command | Создать пользователя | `POST /api/users` | ✅ UserCreated |
| Command | Зарегистрировать водителя | `POST /api/drivers` | ✅ DriverRegistered |
| Command | Создать заказ | `POST /api/orders` | ✅ OrderCreated |
| Command | Принять заказ | `POST /api/orders/{id}/accept` | ✅ OrderAccepted |
| Command | Завершить заказ | `POST /api/orders/{id}/complete` | ✅ OrderCompleted |
| Query | Получить пользователя | `GET /api/users/{id}` | ❌ |
| Query | Список заказов | `GET /api/orders` | ❌ |
| Query | Активные заказы | `GET /api/orders/active` | ❌ |

**Принцип:** Commands (write) публикуют события, Queries (read) только читают данные из кэша/БД.

---

## Реализация Producer и Consumer

### TaxiEventProducer

Класс `TaxiEventProducer` (в [`rabbitmq_producer.hpp`](taxi-service/src/common/rabbitmq_producer.hpp)) отвечает за публикацию событий:

```cpp
class TaxiEventProducer : public userver::components::LoggableComponentBase {
public:
    void PublishUserCreated(const User& user);
    void PublishDriverRegistered(const Driver& driver);
    void PublishOrderCreated(const Order& order);
    void PublishOrderAccepted(const Order& order, const std::string& driver_id);
    void PublishOrderCompleted(const Order& order);

private:
    void PublishEvent(const formats::json::Value& event);
    userver::rabbitmq::Client* client_;
    std::string exchange_{"taxi-events"};
    std::string queue_{"taxi-events-queue"};
    std::string routing_key_{"taxi-routing-key"};
};
```

### TaxiEventConsumer

Класс `TaxiEventConsumer` (в [`rabbitmq_consumer.hpp`](taxi-service/src/common/rabbitmq_consumer.hpp)) обрабатывает события:

```cpp
class TaxiEventConsumer : public userver::components::LoggableComponentBase {
private:
    void ProcessEvent(const formats::json::Value& event);
    void ProcessUserCreated(const formats::json::Value& data);
    void ProcessDriverRegistered(const formats::json::Value& data);
    void ProcessOrderCreated(const formats::json::Value& data);
    void ProcessOrderAccepted(const formats::json::Value& data);
    void ProcessOrderCompleted(const formats::json::Value& data);
};
```

---

## Каталог событий

Полное описание всех событий находится в [`event_catalog.md`](taxi-service/event_catalog.md).

### Основные события

1. **UserCreated** — пользователь создан
2. **DriverRegistered** — водитель зарегистрирован
3. **OrderCreated** — заказ создан
4. **OrderAccepted** — заказ принят водителем
5. **OrderCompleted** — заказ завершён

---

## API Endpoints

### Input Service (Port 8081)

```
POST   /api/users                    — Создать пользователя
GET    /api/users                    — Получить пользователей
POST   /api/auth/login               — Получить JWT-токен
POST   /api/drivers                  — Зарегистрировать водителя
GET    /ping                         — Liveness probe
GET    /metrics                      — Метрики
```

### Order Service (Port 8082)

```
POST   /api/orders                   — Создать заказ
GET    /api/orders                   — Получить активные заказы
GET    /api/users/{user_id}/orders   — История заказов пользователя
POST   /api/orders/{order_id}/complete — Завершить заказ
GET    /ping                         — Liveness probe
GET    /metrics                      — Метрики
```

### Match Service (Port 8083)

```
GET    /api/orders                   — Получить активные заказы
POST   /api/orders/{order_id}/accept — Принять заказ (водитель)
GET    /ping                         — Liveness probe
GET    /metrics                      — Метрики
```

---

## Тестирование

### Автоматический тест

```bash
cd taxi-service
chmod +x tests/test_event_driven.sh
./tests/test_event_driven.sh
```

Тест проверяет:
1. Создание пользователя → событие `UserCreated`
2. Получение JWT-токена
3. Регистрация водителя → событие `DriverRegistered`
4. Создание заказа → событие `OrderCreated`
5. Проверка очереди RabbitMQ
6. CQRS: GET-запросы не генерируют события

### Ручная проверка через RabbitMQ Management UI

1. Открыть http://localhost:15672 (guest/guest)
2. Перейти в **Queues** → `taxi-events-queue`
3. Нажать **Get messages** → увидеть JSON-события

---

## Инструкции по запуску

### Быстрый старт

```bash
cd taxi-service
docker compose up --build -d

# Проверить статус
docker compose ps

# Просмотреть логи
docker compose logs -f input-service
```

### Остановка

```bash
docker compose down
```

### Очистка данных

```bash
docker compose down -v
```

---

## Мониторинг

### Prometheus

- **URL:** http://localhost:9090
- **Метрики:** http://localhost:9090/metrics

### Grafana

- **URL:** http://localhost:3000
- **Логин:** admin
- **Пароль:** admin

### RabbitMQ Management UI

- **URL:** http://localhost:15672
- **Логин:** guest
- **Пароль:** guest

**Вкладки для проверки:**
- **Queues** → `taxi-events-queue` — количество сообщений, consumers
- **Exchanges** → `taxi-events` — тип fanout, bindings
- **Connections** — активные подключения от 3 сервисов

---

## Структура проекта

```
ARCHITECTURE/HW 6/
├── taxi-service/
│   ├── src/
│   │   ├── common/
│   │   │   ├── rabbitmq_producer.hpp  [NEW] TaxiEventProducer
│   │   │   ├── rabbitmq_consumer.hpp  [NEW] TaxiEventConsumer
│   │   │   ├── models.hpp
│   │   │   ├── database.hpp
│   │   │   ├── cache_component.hpp
│   │   │   ├── rate_limiter.hpp
│   │   │   └── metrics_handler.hpp
│   │   ├── input/
│   │   │   ├── main.cpp              [UPDATED] +RabbitMQ components
│   │   │   ├── handlers.hpp
│   │   │   └── handlers.cpp
│   │   ├── order/
│   │   │   ├── main.cpp              [UPDATED] +RabbitMQ components
│   │   │   ├── handlers.hpp
│   │   │   └── handlers.cpp
│   │   ├── match/
│   │   │   ├── main.cpp              [UPDATED] +RabbitMQ components
│   │   │   ├── handlers.hpp
│   │   │   └── handlers.cpp
│   │   └── auth/
│   │       ├── jwt_auth_checker.hpp
│   │       └── jwt_auth_factory.hpp
│   ├── configs/
│   │   ├── static_config_input.yaml   [UPDATED] +RabbitMQ
│   │   ├── static_config_order.yaml   [UPDATED] +RabbitMQ
│   │   ├── static_config_match.yaml   [UPDATED] +RabbitMQ
│   │   └── dynamic_config_fallback.json
│   ├── monitoring/
│   │   ├── prometheus/
│   │   │   ├── prometheus.yml
│   │   │   └── alerts.yml
│   │   └── grafana/
│   │       ├── dashboards/
│   │       └── provisioning/
│   ├── tests/
│   │   └── test_event_driven.sh       [NEW]
│   ├── docker-compose.yaml            [UPDATED] +RabbitMQ service
│   ├── CMakeLists.txt                 [UPDATED] RABBITMQ ON
│   ├── Dockerfile
│   ├── event_driven_design.md         [NEW]
│   ├── event_catalog.md               [NEW]
│   ├── performance_design.md
│   ├── schema_design.md
│   ├── openapi.yaml
│   └── nginx.conf
├── README.md                          [NEW] Этот файл
├── QUICK_START.md                     [UPDATED]
├── TEST_EVENT_DRIVEN.md               [NEW]
└── TASK_INFO.md
```

---

## Заключение

Реализована полнофункциональная Event-Driven архитектура для системы заказа такси с использованием:
- **RabbitMQ** для асинхронной доставки событий
- **CQRS** для разделения команд и запросов
- **at-least-once** гарантией доставки
- **Мониторингом** через Prometheus и Grafana

Система готова к масштабированию и добавлению новых потребителей событий.
