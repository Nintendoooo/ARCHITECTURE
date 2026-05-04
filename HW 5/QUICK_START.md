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

## Загрузить тестовые данные

```bash
docker exec -i taxi-mongodb mongosh -u taxi_user -p taxi_pass --authenticationDatabase taxi_db taxi_db < data.js
```

Результат: тестовые пользователи, водители и заказы.

## Тестирование кэширования и Rate Limiting

### Запуск автоматических тестов

```bash
cd "taxi-service/tests"
./test_cache_rate_limiting.sh
```

Этот скрипт проверяет:
1. **Cache Hit Rate** - эффективность кэширования при повторных запросах
2. **Rate Limiting** - ограничение запросов (10 запросов/мин для регистрации)
3. **Cache Invalidation** - инвалидация кэша при обновлении данных
4. **Active Orders Caching** - кеширование активных заказов

### Нагрузочное тестирование

```bash
cd "taxi-service/tests"
./load_test.sh
```

Этот скрипт выполняет комплексное нагрузочное тестирование с использованием Apache Bench (ab):

**Тесты включают:**
1. **User Registration** - создание пользователей под нагрузкой
2. **User Login** - вход в систему под нагрузкой
3. **Find User by Login** - поиск пользователя по логину (с кэшированием)
4. **Find Driver by Login** - поиск водителя по логину (с кэшированием)
5. **Get Active Orders** - получение активных заказов (с кэшированием)
6. **Get Order History** - получение истории заказов (с кэшированием)

**Результаты тестов:**
- Сохраняются в директорию `./load_test_results/`
- Включают метрики производительности (RPS, latency, etc.)
- Генерируют графики в формате gnuplot

**Параметры тестирования:**
- Количество запросов: 1000
- Уровень параллелизма: 10-50
- Различные сценарии нагрузки для разных endpoints

## Мониторинг в Grafana

### Доступ к дашбордам

1. Откройте Grafana: http://localhost:3000
2. Войдите с учетными данными admin/admin
3. Перейдите в раздел Dashboards

**Доступные дашборды:**
- **Taxi Service - Cache & Rate Limiting Dashboard** — основной дашборд для мониторинга кэша и rate limiting
- **Taxi Service Performance** — общие метрики производительности

### Метрики дашборда "Taxi Service - Cache & Rate Limiting Dashboard"

Дашборд показывает следующие метрики:

1. **Cache Hit Rate** - процент попаданий в кэш (user/driver/order)
2. **Cache Hits/Misses Rate** - скорость попаданий и промахов кэша
3. **Rate Limit Violations** - количество нарушений rate limiting
4. **Request Rate (Allowed vs Denied)** - скорость разрешенных и отклоненных запросов
5. **Cache Size** - текущий размер кэша
6. **Cache Evictions Rate** - скорость вытеснения из кэша
7. **Active Rate Limit Buckets** - количество активных bucket'ов rate limiting

### Просмотр метрик напрямую

```bash
# Получить все метрики кэша и rate limiting
curl http://localhost:8081/metrics

# Получить метрики только кэша
curl http://localhost:8081/metrics | grep cache_

# Получить метрики только rate limiting
curl http://localhost:8081/metrics | grep rate_limit_

# Получить конкретную метрику
curl http://localhost:8081/metrics | grep cache_hit_rate
```

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

## Проверка работы кэша

### Тест 1: Cache Hit Rate

```bash
# Первый запрос (cache miss)
curl http://localhost:8081/api/users?login=testuser

# Повторные запросы (cache hits)
for i in {1..10}; do
  curl http://localhost:8081/api/users?login=testuser
done

# Проверка метрик
curl http://localhost:8081/metrics | grep cache_hits_total
curl http://localhost:8081/metrics | grep cache_misses_total
```

### Тест 2: Rate Limiting

```bash
# Попытка создать 15 пользователей (лимит: 10/мин)
for i in {1..15}; do
  curl -X POST http://localhost:8081/api/users \
    -H "Content-Type: application/json" \
    -d "{
      \"login\":\"testuser$i\",
      \"email\":\"testuser$i@example.com\",
      \"first_name\":\"Test\",
      \"last_name\":\"User$i\",
      \"phone\":\"+7900123456$i\",
      \"password\":\"password123\"
    }"
  echo "Request $i completed"
done
```

После 10-го запроса вы должны получить HTTP 429 (Too Many Requests).

## Остановка сервисов

```bash
cd "ARCHITECTURE/HW 5/taxi-service"
docker-compose down
```
