# Quick Start — Taxi Service (MongoDB)

## Структура проекта

- `init-mongo.js` — скрипт инициализации БД (создание коллекций, индексов)
- `data.js` — тестовые данные для загрузки
- `queries.js` — примеры MongoDB-запросов (CRUD операции)
- `validation.js` — схемы валидации документов

## Сборка

```bash
cd "taxi-service"
docker-compose build
```

Первая сборка может занять 5-10 минут.

## Запуск

```bash
cd "taxi-service"
docker-compose up -d
```

Дождитесь готовности сервисов (~1-2 минуты):

```bash
docker-compose ps   # все сервисы должны быть (healthy)
```

## Загрузка тестовых данных

База данных инициализируется автоматически при первом запуске через файл `init-mongo.js`. 
Для загрузки дополнительных тестовых данных из `data.js`:

```bash
docker exec -i taxi-mongodb mongosh -u taxi_user -p taxi_pass --authenticationDatabase taxi_db taxi_db < data.js
```

## Проверка

### Проверка сервисов

```bash
curl http://localhost:8080/ping        # → pong
curl http://localhost:8080/swagger/    # Swagger UI
```

### Проверка MongoDB

```bash
# Подключиться к БД
docker exec -it taxi-mongodb mongosh -u taxi_user -p taxi_pass --authenticationDatabase taxi_db taxi_db

# Проверить коллекции
show collections

# Проверить данные
db.users.countDocuments()
db.drivers.countDocuments()
db.orders.countDocuments()

# Выйти
exit
```

## Быстрый тест API

```bash
BASE_URL="http://localhost:8080"

# 1. Регистрация пользователя
curl -s -X POST $BASE_URL/api/users \
  -H "Content-Type: application/json" \
  -d '{"login":"test_user","email":"test@example.com","first_name":"Test","last_name":"User","phone":"+79001234567","password":"pass123"}'

# 2. Логин
TOKEN=$(curl -s -X POST $BASE_URL/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login":"test_user","password":"pass123"}' | python3 -c "import sys,json; print(json.load(sys.stdin)['token'])")

echo "Token: $TOKEN"

# 3. Поиск пользователя
curl -s -X GET "$BASE_URL/api/users?login=test_user" \
  -H "Authorization: Bearer $TOKEN"
```

## Тестовые пользователи

После загрузки `data.js` доступны тестовые пользователи (пароль: `securePass123`):

**Пассажиры:** `ivan_petrov`, `anna_smirnova`, `alex_kuznetsov`, `olga_ivanova`, `petr_sokolov`, `natalia_volkov`, `andrey_morozov`, `elena_fedorova`

**Водители:** `mike_driver`, `sergey_driver`, `dmitry_driver`, `alex_driver`, `vladimir_driver`, `nikolay_driver`, `ivan_driver`, `evgeny_driver`, `maxim_driver`, `anton_driver`

Пример авторизации:

```bash
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login":"ivan_petrov","password":"securePass123"}'
```

## Запуск примеров запросов и валидации

### queries.js — примеры MongoDB-запросов

```bash
docker exec -i taxi-mongodb mongosh -u taxi_user -p taxi_pass --authenticationDatabase taxi_db taxi_db < queries.js
```

### validation.js — схемы валидации

```bash
docker exec -i taxi-mongodb mongosh -u taxi_user -p taxi_pass --authenticationDatabase taxi_db taxi_db < validation.js
```

## Остановка

```bash
docker-compose down
```

Для полной очистки (включая данные MongoDB):

```bash
docker-compose down -v
```
