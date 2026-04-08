# Quick Start — Taxi Service (PostgreSQL)

## Сборка

```bash
cd "taxi-service"
docker-compose build
```

Первая сборка может занять 5-10 минут в зависимости от скорости интернета.

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

База данных инициализируется автоматически при первом запуске через файл `init-db.sql`. 
Для загрузки дополнительных тестовых данных из `data.sql`:

```bash
docker exec -i taxi-postgres psql -U taxi_user -d taxi_db < data.sql
```

**Важно:** Файл `data.sql` содержит расширенный набор тестовых данных (14 пользователей, 10 водителей, 15+ заказов).

## Проверка

### Проверка сервисов

```bash
curl http://localhost:8080/ping        # → pong
curl http://localhost:8080/swagger/    # Swagger UI
```

### Проверка PostgreSQL

```bash
# Подключиться к БД
docker exec -it taxi-postgres psql -U taxi_user -d taxi_db

# Проверить таблицы
\dt

# Проверить данные
SELECT COUNT(*) FROM users;
SELECT COUNT(*) FROM drivers;
SELECT COUNT(*) FROM orders;

# Выйти
\q
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

После загрузки `data.sql` доступны следующие тестовые пользователи (пароль: `securePass123`):

- `alice.smith` - пассажир
- `bob.jones` - пассажир  
- `ivan.petrov` - водитель
- `julia.roberts` - водитель
- и другие...

Пример авторизации:

```bash
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login":"ivan_petrov","password":"securePass123"}'
```

## Остановка

```bash
docker-compose down
```

Для полной очистки (включая данные PostgreSQL):

```bash
docker-compose down -v
```
