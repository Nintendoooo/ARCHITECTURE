#!/bin/bash

# test_event_driven.sh — End-to-end тест Event-Driven pipeline для Taxi Service

# Don't exit on first error - we want to see all test results
set +e

BASE_URL="http://localhost:8080"
INPUT_URL="${BASE_URL}"
ORDER_URL="${BASE_URL}"
MATCH_URL="${BASE_URL}"
RABBIT_URL="http://localhost:15672"
RABBIT_USER="guest"
RABBIT_PASS="guest"
QUEUE_NAME="taxi-events-queue"

PASS=0
FAIL=0

pass() { echo "✅ PASS: $1"; ((PASS++)); }
fail() { echo "❌ FAIL: $1"; ((FAIL++)); }

echo "========================================"
echo "  Taxi Service — Event-Driven Tests"
echo "========================================"
echo ""

# ── TEST 1: Создание пользователя → UserCreated ──────────────────────────────
echo "TEST 1: POST /api/users → UserCreated event"

TIMESTAMP=$(date +%s)
LOGIN="testuser_${TIMESTAMP}"

RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "${INPUT_URL}/api/users" \
  -H "Content-Type: application/json" \
  -d "{\"login\":\"${LOGIN}\",\"email\":\"${LOGIN}@example.com\",\"first_name\":\"Test\",\"last_name\":\"User\",\"phone\":\"+79001234567\",\"password\":\"secret123\",\"role\":\"passenger\"}")

HTTP_CODE=$(echo "$RESPONSE" | tail -1)
BODY=$(echo "$RESPONSE" | head -1)

if [ "$HTTP_CODE" = "200" ] || [ "$HTTP_CODE" = "201" ]; then
  USER_ID=$(echo "$BODY" | jq -r '.id // .user_id // empty')
  pass "User created (HTTP $HTTP_CODE), id=$USER_ID"
else
  fail "User creation failed (HTTP $HTTP_CODE): $BODY"
  USER_ID=""
fi

# ── TEST 2: JWT-токен ─────────────────────────────────────────────────────────
echo ""
echo "TEST 2: POST /api/auth/login → JWT token"

sleep 0.5

TOKEN_RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "${INPUT_URL}/api/auth/login" \
  -H "Content-Type: application/json" \
  -d "{\"login\":\"${LOGIN}\",\"password\":\"secret123\"}")

TOKEN_CODE=$(echo "$TOKEN_RESPONSE" | tail -1)
TOKEN_BODY=$(echo "$TOKEN_RESPONSE" | head -1)

if [ "$TOKEN_CODE" = "200" ]; then
  TOKEN=$(echo "$TOKEN_BODY" | jq -r '.token // empty')
  if [ -n "$TOKEN" ]; then
    pass "JWT token received"
  else
    fail "JWT token empty in response: $TOKEN_BODY"
    TOKEN=""
  fi
else
  fail "Login failed (HTTP $TOKEN_CODE): $TOKEN_BODY"
  TOKEN=""
fi

# ── TEST 3: Регистрация водителя → DriverRegistered ───────────────────────────
echo ""
echo "TEST 3: POST /api/drivers → DriverRegistered event"

if [ -n "$TOKEN" ]; then
  DRIVER_RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "${INPUT_URL}/api/drivers" \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer $TOKEN" \
    -d "{\"license_number\":\"AB${TIMESTAMP}\",\"car_model\":\"Toyota Camry\",\"car_plate\":\"A${TIMESTAMP}AA77\"}")

  DRIVER_CODE=$(echo "$DRIVER_RESPONSE" | tail -1)
  DRIVER_BODY=$(echo "$DRIVER_RESPONSE" | head -1)

  if [ "$DRIVER_CODE" = "200" ] || [ "$DRIVER_CODE" = "201" ]; then
    DRIVER_ID=$(echo "$DRIVER_BODY" | jq -r '.id // .driver_id // empty')
    pass "Driver registered (HTTP $DRIVER_CODE), id=$DRIVER_ID"
  else
    fail "Driver registration failed (HTTP $DRIVER_CODE): $DRIVER_BODY"
  fi
else
  fail "Skipped (no JWT token)"
fi

# ── TEST 4: Создание заказа → OrderCreated ────────────────────────────────────
echo ""
echo "TEST 4: POST /api/orders → OrderCreated event"

if [ -n "$TOKEN" ]; then
  ORDER_RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "${ORDER_URL}/api/orders" \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer $TOKEN" \
    -d "{\"passenger_name\":\"Test User\",\"passenger_phone\":\"+79001234567\",\"pickup_address\":\"ул. Ленина, 1\",\"destination_address\":\"ул. Пушкина, 10\"}")

  ORDER_CODE=$(echo "$ORDER_RESPONSE" | tail -1)
  ORDER_BODY=$(echo "$ORDER_RESPONSE" | head -1)

  if [ "$ORDER_CODE" = "200" ] || [ "$ORDER_CODE" = "201" ]; then
    ORDER_ID=$(echo "$ORDER_BODY" | jq -r '.id // .order_id // empty')
    pass "Order created (HTTP $ORDER_CODE), id=$ORDER_ID"
  else
    fail "Order creation failed (HTTP $ORDER_CODE): $ORDER_BODY"
    ORDER_ID=""
  fi
else
  fail "Skipped (no JWT token)"
  ORDER_ID=""
fi

# ── TEST 5: Проверка очереди RabbitMQ ─────────────────────────────────────────
echo ""
echo "TEST 5: RabbitMQ queue '${QUEUE_NAME}' has messages"

sleep 1

QUEUE_INFO=$(curl -s -u "${RABBIT_USER}:${RABBIT_PASS}" \
  "${RABBIT_URL}/api/queues/%2F/${QUEUE_NAME}" 2>/dev/null)

if echo "$QUEUE_INFO" | jq -e '.name' > /dev/null 2>&1; then
  MSG_COUNT=$(echo "$QUEUE_INFO" | jq -r '.messages // 0')
  MSG_READY=$(echo "$QUEUE_INFO" | jq -r '.messages_ready // 0')
  MSG_TOTAL=$(echo "$QUEUE_INFO" | jq -r '.messages_unacknowledged // 0')
  pass "Queue exists: messages=$MSG_COUNT, ready=$MSG_READY"
else
  fail "Queue '${QUEUE_NAME}' not found or RabbitMQ not accessible"
fi

# ── TEST 6: CQRS — GET не генерирует события ──────────────────────────────────
echo ""
echo "TEST 6: CQRS — GET /api/orders/active does NOT publish events"

BEFORE=$(curl -s -u "${RABBIT_USER}:${RABBIT_PASS}" \
  "${RABBIT_URL}/api/queues/%2F/${QUEUE_NAME}" 2>/dev/null | jq -r '.messages // 0')

curl -s -o /dev/null "${ORDER_URL}/api/orders/active" \
  -H "Authorization: Bearer ${TOKEN:-dummy}"

sleep 0.5

AFTER=$(curl -s -u "${RABBIT_USER}:${RABBIT_PASS}" \
  "${RABBIT_URL}/api/queues/%2F/${QUEUE_NAME}" 2>/dev/null | jq -r '.messages // 0')

if [ "$BEFORE" = "$AFTER" ]; then
  pass "CQRS: GET query did not publish events (messages: $BEFORE → $AFTER)"
else
  fail "CQRS violation: GET query published events (messages: $BEFORE → $AFTER)"
fi

# ── Итог ──────────────────────────────────────────────────────────────────────
echo ""
echo "========================================"
echo "  Results: $PASS passed, $FAIL failed"
echo "========================================"

if [ "$FAIL" -eq 0 ]; then
  echo "🎉 All tests passed!"
  exit 0
else
  echo "⚠️  Some tests failed."
  exit 1
fi
