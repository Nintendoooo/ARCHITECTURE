# Отчёт по домашнему заданию №2: REST API сервис заказа такси

**Студентка группы М8О-106СВ-21:** Нина  
**Вариант:** №16 — Система заказа такси (Uber)

---

## Что было сделано

Реализовала REST API для системы заказа такси на C++ с использованием фреймворка Yandex uServer. Архитектура микросервисная — три независимых сервиса. Работа через Nginx API Gateway.

### Архитектура решения

Сервисы разделены по доменам в соответствии с архитектурой из ДЗ №1:

- **Input Service** (порт 8081) — регистрация пользователей и водителей, аутентификация, поиск
- **Order Service** (порт 8082) — создание заказов, история поездок, завершение поездки
- **Match Service** (порт 8083) — просмотр активных заказов, принятие заказа водителем
- **Nginx** (порт 8080) — API Gateway, единая точка входа для всех клиентов

```
Клиент/Водитель → Nginx :8080 → Input Service  :8081
                              → Order Service  :8082
                              → Match Service  :8083
                                     ↓
                               SQLite (taxi.db)
```

Ключевая особенность: роутинг `/api/orders` в Nginx зависит от HTTP-метода — `POST` идёт в Order Service (создание), `GET` — в Match Service (активные заказы).

### Реализованные API endpoints

Все 9 требуемых операций + логин:

**Пользователи и аутентификация (Input Service):**
- `POST /api/users` — регистрация нового пользователя
- `POST /api/auth/login` — вход и получение JWT токена
- `GET /api/users?login=...` — поиск пользователя по точному логину (требует авторизации)
- `GET /api/users?name=...` — поиск по маске имени/фамилии (требует авторизации)

**Водители (Input Service):**
- `POST /api/drivers` — регистрация текущего пользователя как водителя (требует авторизации)

**Заказы (Order Service):**
- `POST /api/orders` — создание нового заказа поездки (требует авторизации)
- `GET /api/users/{id}/orders` — история поездок пользователя (требует авторизации)
- `POST /api/orders/{id}/complete` — завершение поездки (только для водителя)

**Сопоставление (Match Service):**
- `GET /api/orders?status=active` — список активных заказов (требует авторизации)
- `POST /api/orders/{id}/accept` — принятие заказа водителем (только для водителя)

### Технические детали

**Стек технологий:**
- C++20 для основного кода
- Yandex uServer как веб-фреймворк
- SQLite для хранения данных (WAL-режим + foreign keys)
- JWT-CPP v0.7.0 для работы с токенами (HS256)
- Docker + Docker Compose для контейнеризации
- Nginx как reverse proxy с method-based routing

**Аутентификация и роли:**

Реализована JWT-аутентификация с поддержкой ролей. При логине пользователь получает токен на 24 часа. Токен содержит `user_id` и `role` (`passenger` или `driver`). Роль `driver` назначается автоматически, если пользователь зарегистрирован через `/api/drivers`.

Кастомный `TaxiJwtAuthCheckerFactory` (middleware uServer):
1. Извлекает токен из заголовка `Authorization: Bearer <token>`
2. Проверяет подпись (HS256) и срок действия
3. Кладёт `user_id` и `role` в контекст запроса

Это позволяет handler'ам проверять не только факт авторизации, но и роль пользователя.

**Модели данных:**

Три сущности с жизненным циклом заказа:
- `User` — пользователь (логин, email, имя, фамилия, телефон, хеш пароля)
- `Driver` — профиль водителя (привязан к User, номер ВУ, автомобиль)
- `Order` — заказ поездки со статусами: `active → accepted → completed`

**Жизненный цикл заказа:**
```
[создан] → active → accepted → completed
                 ↘ cancelled
```

**Обработка ошибок:**

Правильные HTTP статус-коды во всех сценариях:
- `200 OK` — успешное получение/обновление данных
- `201 Created` — успешное создание ресурса
- `400 Bad Request` — невалидные данные (пустые поля, неверный формат, неверный статус)
- `401 Unauthorized` — отсутствует или невалидный токен
- `403 Forbidden` — нет прав (не водитель, не тот водитель, чужая история)
- `404 Not Found` — ресурс не найден
- `409 Conflict` — дубликат (логин/email уже занят, уже зарегистрирован как водитель)

**Валидация:**
- Обязательные поля во всех запросах
- Формат email (наличие `@`)
- Телефон начинается с `+`
- Адреса подачи и назначения не совпадают
- Атомарные переходы статусов заказа (только из допустимого состояния)

**Безопасность:**
- Пароли хешируются через SHA-256, не возвращаются в ответах
- Пассажир видит только свою историю поездок
- Завершить поездку может только назначенный водитель
- JWT токены с ограниченным сроком жизни (24 часа)



## Запуск проекта

### Требования

- Docker 20.10+
- Docker Compose 2.0+

### Сборка и запуск

1. Перейти в директорию проекта
```bash
cd hw2/taxi-service"
```
2. Собрать и запустить все сервисы
```bash
docker-compose up --build
```

После запуска доступны:
- **API Gateway:** http://localhost:8080
- **Swagger UI:** http://localhost:8080/swagger/
- **OpenAPI spec:** http://localhost:8080/openapi.yaml
- **Input Service:** http://localhost:8081
- **Order Service:** http://localhost:8082
- **Match Service:** http://localhost:8083

### Проверка работоспособности

```bash
# Проверить статус контейнеров
docker-compose ps

# Проверить health check
curl http://localhost:8081/ping
curl http://localhost:8082/ping
curl http://localhost:8083/ping
```

### Остановка

```bash
docker-compose down
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

**Ответ (201 Created):**
```json
{
  "id": 1,
  "login": "ivan_petrov",
  "email": "ivan@example.com",
  "first_name": "Иван",
  "last_name": "Петров",
  "phone": "+79001234567",
  "created_at": "2024-01-15 10:30:00"
}
```

### 2. Вход в систему

```bash
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "login": "ivan_petrov",
    "password": "securePass123"
  }'
```

**Ответ (200 OK):**
```json
{
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "user_id": 1,
  "expires_at": "2024-01-16 10:30:00"
}
```

Сохраните токен для последующих запросов:
```bash
TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
```

### 3. Поиск пользователя по логину

```bash
curl -X GET "http://localhost:8080/api/users?login=ivan_petrov" \
  -H "Authorization: Bearer $TOKEN"
```

**Ответ (200 OK):**
```json
{
  "id": 1,
  "login": "ivan_petrov",
  "email": "ivan@example.com",
  "first_name": "Иван",
  "last_name": "Петров",
  "phone": "+79001234567",
  "created_at": "2024-01-15 10:30:00"
}
```

### 4. Поиск пользователей по маске имени

```bash
curl -X GET "http://localhost:8080/api/users?name=Иван" \
  -H "Authorization: Bearer $TOKEN"
```

**Ответ (200 OK):**
```json
[
  {
    "id": 1,
    "login": "ivan_petrov",
    "first_name": "Иван",
    "last_name": "Петров",
    ...
  }
]
```

### 5. Регистрация водителя

Сначала зарегистрируйте пользователя и войдите, получите TOKEN
```bash
curl -X POST http://localhost:8080/api/drivers \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "license_number": "77 АА 123456",
    "car_model": "Toyota Camry",
    "car_plate": "А123БВ77"
  }'
```

**Ответ (201 Created):**
```json
{
  "id": 1,
  "user_id": 1,
  "license_number": "77 АА 123456",
  "car_model": "Toyota Camry",
  "car_plate": "А123БВ77",
  "is_available": true,
  "created_at": "2024-01-15 10:30:00"
}
```

После регистрации водителя — перелогиньтесь, чтобы получить токен с ролью `driver`.

### 6. Создание заказа поездки

```bash
# Войдите как пассажир (не водитель)
curl -X POST http://localhost:8080/api/orders \
  -H "Authorization: Bearer $PASSENGER_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "pickup_address": "ул. Ленина, 10",
    "destination_address": "ул. Пушкина, 25"
  }'
```

**Ответ (201 Created):**
```json
{
  "id": 1,
  "user_id": 2,
  "driver_id": null,
  "pickup_address": "ул. Ленина, 10",
  "destination_address": "ул. Пушкина, 25",
  "status": "active",
  "price": null,
  "created_at": "2024-01-15 10:30:00",
  "completed_at": null
}
```

### 7. Получение активных заказов (для водителя)

```bash
curl -X GET "http://localhost:8080/api/orders?status=active" \
  -H "Authorization: Bearer $DRIVER_TOKEN"
```

**Ответ (200 OK):**
```json
[
  {
    "id": 1,
    "user_id": 2,
    "driver_id": null,
    "pickup_address": "ул. Ленина, 10",
    "destination_address": "ул. Пушкина, 25",
    "status": "active",
    "price": null,
    "created_at": "2024-01-15 10:30:00",
    "completed_at": null
  }
]
```

### 8. Принятие заказа водителем

```bash
curl -X POST http://localhost:8080/api/orders/1/accept \
  -H "Authorization: Bearer $DRIVER_TOKEN"
```

**Ответ (200 OK):**
```json
{
  "id": 1,
  "user_id": 2,
  "driver_id": 1,
  "pickup_address": "ул. Ленина, 10",
  "destination_address": "ул. Пушкина, 25",
  "status": "accepted",
  "price": null,
  "created_at": "2024-01-15 10:30:00",
  "completed_at": null
}
```

### 9. Завершение поездки

```bash
curl -X POST http://localhost:8080/api/orders/1/complete \
  -H "Authorization: Bearer $DRIVER_TOKEN"
```

**Ответ (200 OK):**
```json
{
  "id": 1,
  "user_id": 2,
  "driver_id": 1,
  "pickup_address": "ул. Ленина, 10",
  "destination_address": "ул. Пушкина, 25",
  "status": "completed",
  "price": null,
  "created_at": "2024-01-15 10:30:00",
  "completed_at": "2024-01-15 11:00:00"
}
```

### 10. История поездок пользователя

```bash
curl -X GET http://localhost:8080/api/users/2/orders \
  -H "Authorization: Bearer $PASSENGER_TOKEN"
```

**Ответ (200 OK):**
```json
[
  {
    "id": 1,
    "user_id": 2,
    "driver_id": 1,
    "pickup_address": "ул. Ленина, 10",
    "destination_address": "ул. Пушкина, 25",
    "status": "completed",
    "price": null,
    "created_at": "2024-01-15 10:30:00",
    "completed_at": "2024-01-15 11:00:00"
  }
]
```

---

## Полный сценарий тестирования

Cценарий для проверки ЖЦ заказа:

```bash
BASE_URL="http://localhost:8080"

# 1. Регистрация пассажира
curl -s -X POST $BASE_URL/api/users \
  -H "Content-Type: application/json" \
  -d '{"login":"passenger1","email":"p1@test.com","first_name":"Анна","last_name":"Иванова","phone":"+79001111111","password":"pass123"}'

# 2. Регистрация будущего водителя
curl -s -X POST $BASE_URL/api/users \
  -H "Content-Type: application/json" \
  -d '{"login":"driver1","email":"d1@test.com","first_name":"Борис","last_name":"Смирнов","phone":"+79002222222","password":"pass123"}'

# 3. Логин пассажира
PASSENGER_TOKEN=$(curl -s -X POST $BASE_URL/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login":"passenger1","password":"pass123"}' | python3 -c "import sys,json; print(json.load(sys.stdin)['token'])")

# 4. Логин водителя (пока без роли driver)
DRIVER_USER_TOKEN=$(curl -s -X POST $BASE_URL/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login":"driver1","password":"pass123"}' | python3 -c "import sys,json; print(json.load(sys.stdin)['token'])")

# 5. Регистрация водителя
curl -s -X POST $BASE_URL/api/drivers \
  -H "Authorization: Bearer $DRIVER_USER_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"license_number":"77 АА 000001","car_model":"Kia Rio","car_plate":"В001ВВ77"}'

# 6. Перелогин водителя — теперь получит роль driver
DRIVER_TOKEN=$(curl -s -X POST $BASE_URL/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login":"driver1","password":"pass123"}' | python3 -c "import sys,json; print(json.load(sys.stdin)['token'])")

# 7. Пассажир создаёт заказ
ORDER_ID=$(curl -s -X POST $BASE_URL/api/orders \
  -H "Authorization: Bearer $PASSENGER_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"pickup_address":"ул. Ленина, 10","destination_address":"ул. Пушкина, 25"}' \
  | python3 -c "import sys,json; print(json.load(sys.stdin)['id'])")

echo "Создан заказ #$ORDER_ID"

# 8. Водитель видит активные заказы
curl -s -X GET "$BASE_URL/api/orders?status=active" \
  -H "Authorization: Bearer $DRIVER_TOKEN" | python3 -m json.tool

# 9. Водитель принимает заказ
curl -s -X POST $BASE_URL/api/orders/$ORDER_ID/accept \
  -H "Authorization: Bearer $DRIVER_TOKEN" | python3 -m json.tool

# 10. Водитель завершает поездку
curl -s -X POST $BASE_URL/api/orders/$ORDER_ID/complete \
  -H "Authorization: Bearer $DRIVER_TOKEN" | python3 -m json.tool

# 11. Пассажир смотрит историю
curl -s -X GET $BASE_URL/api/users/1/orders \
  -H "Authorization: Bearer $PASSENGER_TOKEN" | python3 -m json.tool
```

---

## Документация API

Полная OpenAPI 3.0 спецификация находится в [`taxi-service/openapi.yaml`](taxi-service/openapi.yaml).

## Тестирование

**Swagger UI** доступен по адресу `http://localhost:8080/swagger/` — позволяет интерактивно тестировать все endpoints прямо из браузера. Для работы с защищёнными endpoints нажмите кнопку **Authorize 🔒** и введите токен из ответа `/api/auth/login`.
