# Быстрый старт

## Запуск проекта

```bash
cd "taxi-service"
docker-compose up -d
sleep 20
docker-compose ps
```

Все сервисы должны быть в статусе `healthy`.

## Доступ к сервисам

### API Endpoints
- **Swagger UI:** http://localhost:8080/swagger/
- **Input Service:** http://localhost:8081
- **Order Service:** http://localhost:8082
- **Match Service:** http://localhost:8083
- **MongoDB:** localhost:27017

### Мониторинг
- **Grafana:** http://localhost:3000 (admin/admin)
- **Prometheus:** http://localhost:9090
- **Metrics:** http://localhost:8081/metrics, http://localhost:8082/metrics, http://localhost:8083/metrics

### RabbitMQ Management UI
- **URL:** http://localhost:15672
- **Логин:** guest
- **Пароль:** guest

Вкладки для проверки:
- **Queues** → `taxi-events-queue` — количество сообщений, consumers
- **Exchanges** → `taxi-events` — тип fanout, bindings
- **Connections** — активные подключения от 3 сервисов

## Загрузить тестовые данные

```bash
docker exec -i taxi-mongodb mongosh -u taxi_user -p taxi_pass --authenticationDatabase taxi_db taxi_db < data.js
```

Результат: тестовые пользователи, водители и заказы.

## Тестирование Event-Driven pipeline

### Автоматический тест

```bash
chmod +x taxi-service/tests/test_event_driven.sh
./taxi-service/tests/test_event_driven.sh
```

Тест проверяет 6 сценариев:
1. Создание пользователя → событие `UserCreated` в RabbitMQ
2. Получение JWT-токена
3. Регистрация водителя → событие `DriverRegistered`
4. Создание заказа → событие `OrderCreated`
5. Проверка очереди `taxi-events-queue` через RabbitMQ API
6. CQRS: GET-запросы не генерируют события

### Ручная проверка событий

```bash
# Создать пользователя (публикует UserCreated)
curl -s -X POST http://localhost:8081/api/users \
  -H "Content-Type: application/json" \
  -d '{"login":"testuser","email":"test@example.com","first_name":"Test","last_name":"User","phone":"+79001234567","password":"secret123","role":"passenger"}' | jq .

# Получить JWT-токен
TOKEN=$(curl -s -X POST http://localhost:8081/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login":"testuser","password":"secret123"}' | jq -r '.token')

# Зарегистрировать водителя (публикует DriverRegistered)
curl -s -X POST http://localhost:8081/api/drivers \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{"license_number":"AB123456","car_model":"Toyota Camry","car_plate":"A001AA77"}' | jq .

# Создать заказ (публикует OrderCreated)
curl -s -X POST http://localhost:8082/api/orders \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{"passenger_name":"Test User","passenger_phone":"+79001234567","pickup_address":"ул. Ленина, 1","destination_address":"ул. Пушкина, 10"}' | jq .

# Проверить очередь RabbitMQ
curl -s -u guest:guest http://localhost:15672/api/queues/%2F | \
  jq '.[] | select(.name == "taxi-events-queue") | {name, messages, consumers}'
```

---

## Примеры запросов к API

### Создание пользователя

```bash
curl -X POST http://localhost:8081/api/users \
  -H "Content-Type: application/json" \
  -d '{
    "login": "testuser",
    "email": "test@example.com",
    "first_name": "Test",
    "last_name": "User",
    "phone": "+79001234567",
    "password": "password123"
  }'
```

### Авторизация

```bash
curl -X POST http://localhost:8081/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "login": "testuser",
    "password": "password123"
  }'
```

### Создание заказа

```bash
TOKEN="your_jwt_token_here"

curl -X POST http://localhost:8082/api/orders \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{
    "pickup_address": "Москва, ул. Тверская, 1",
    "destination_address": "Москва, ул. Арбат, 10"
  }'
```

### Получение активных заказов

```bash
curl http://localhost:8083/api/orders?status=active
```

### Принятие заказа водителем

```bash
TOKEN="driver_jwt_token_here"

curl -X POST http://localhost:8083/api/orders/{order_id}/accept \
  -H "Authorization: Bearer $TOKEN"
```

## Остановка сервисов

```bash
cd "taxi-service"
docker-compose down
```
