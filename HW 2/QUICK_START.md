# Quick Start — Taxi Service

## Сборка

```bash
cd "taxi-service"
docker-compose up --build
```

## Запуск

```bash
cd "taxi-service"
docker-compose up
```

Все сервисы готовы, когда видно `(healthy)` в `docker ps`.

## Проверка

```bash
curl http://localhost:8080/ping        # → pong
curl http://localhost:8080/swagger/    # Swagger UI
```

## Остановка

```bash
cd "taxi-service"
docker-compose down
```
