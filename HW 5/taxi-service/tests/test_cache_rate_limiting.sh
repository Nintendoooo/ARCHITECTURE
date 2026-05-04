#!/bin/bash

echo "=========================================="
echo "Testing Cache and Rate Limiting..."
echo "=========================================="

BASE_URL="http://localhost:8081"
METRICS_URL="$BASE_URL/metrics"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print test results
print_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓ PASS${NC}: $2"
    else
        echo -e "${RED}✗ FAIL${NC}: $2"
    fi
}

# Function to extract metric value
get_metric() {
    echo "$1" | grep "$2" | awk '{print $NF}'
}

echo ""
echo "Test 1: Cache hit rate for user lookup"
echo "--------------------------------------"

# First request - should be cache miss
echo "Making first request (cache miss expected)..."
response1=$(curl -s -w "\n%{http_code}" "$BASE_URL/api/users?login=test_user")
http_code1=$(echo "$response1" | tail -n1)
if [ "$http_code1" = "404" ]; then
    echo -e "${YELLOW}User not found (expected for test)${NC}"
fi

# Make 10 more requests - should be cache hits
echo "Making 10 more requests (cache hits expected)..."
for i in {1..10}; do
    curl -s "$BASE_URL/api/users?login=test_user" > /dev/null
done

# Get metrics
metrics=$(curl -s "$METRICS_URL")
cache_hits=$(get_metric "$metrics" 'cache_hits_total{cache="user"}')
cache_misses=$(get_metric "$metrics" 'cache_misses_total{cache="user"}')

echo "Cache hits: $cache_hits"
echo "Cache misses: $cache_misses"

if [ "$cache_hits" -gt 0 ]; then
    print_result 0 "Cache is working - hits recorded"
else
    print_result 1 "Cache not working - no hits recorded"
fi

echo ""
echo "Test 2: Rate limiting for user registration"
echo "-------------------------------------------"

# Try to create 15 users (limit is 10 per minute)
echo "Attempting to create 15 users (limit: 10/min)..."
blocked_count=0
success_count=0

for i in {1..15}; do
    response=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/api/users" \
        -H "Content-Type: application/json" \
        -d "{
            \"login\":\"testuser$i\",
            \"email\":\"testuser$i@example.com\",
            \"first_name\":\"Test\",
            \"last_name\":\"User$i\",
            \"phone\":\"+7900123456$i\",
            \"password\":\"password123\"
        }")
    
    http_code=$(echo "$response" | tail -n1)
    
    if [ "$http_code" = "429" ]; then
        blocked_count=$((blocked_count + 1))
        echo "Request $i: ${RED}BLOCKED (429)${NC}"
    elif [ "$http_code" = "201" ]; then
        success_count=$((success_count + 1))
        echo "Request $i: ${GREEN}SUCCESS (201)${NC}"
    else
        echo "Request $i: HTTP $http_code"
    fi
    
    sleep 0.1
done

echo ""
echo "Results:"
echo "Successful requests: $success_count"
echo "Blocked requests: $blocked_count"

if [ "$blocked_count" -gt 0 ]; then
    print_result 0 "Rate limiting is working - $blocked_count requests blocked"
else
    print_result 1 "Rate limiting may not be working - no requests blocked"
fi

echo ""
echo "Test 3: Cache invalidation on user update"
echo "-----------------------------------------"

# Create a test user
echo "Creating test user..."
create_response=$(curl -s -X POST "$BASE_URL/api/users" \
    -H "Content-Type: application/json" \
    -d "{
        \"login\":\"cache_test_user\",
        \"email\":\"cache_test@example.com\",
        \"first_name\":\"Original\",
        \"last_name\":\"Name\",
        \"phone\":\"+79009998877\",
        \"password\":\"password123\"
    }")

echo "$create_response" | jq -r '.id' > /tmp/test_user_id.txt
user_id=$(cat /tmp/test_user_id.txt)

if [ -n "$user_id" ] && [ "$user_id" != "null" ]; then
    echo "User created with ID: $user_id"
    
    # Get user (cache miss)
    echo "Fetching user (cache miss)..."
    curl -s "$BASE_URL/api/users?login=cache_test_user" > /dev/null
    
    # Get metrics before update
    metrics_before=$(curl -s "$METRICS_URL")
    hits_before=$(get_metric "$metrics_before" 'cache_hits_total{cache="user"}')
    
    # Update user (should invalidate cache)
    echo "Updating user (should invalidate cache)..."
    update_response=$(curl -s -X PUT "$BASE_URL/api/users/$user_id" \
        -H "Content-Type: application/json" \
        -d "{\"first_name\":\"Updated\"}")
    
    # Get user again (should be cache miss due to invalidation)
    echo "Fetching user again (should be cache miss after invalidation)..."
    curl -s "$BASE_URL/api/users?login=cache_test_user" > /dev/null
    
    # Get metrics after update
    metrics_after=$(curl -s "$METRICS_URL")
    hits_after=$(get_metric "$metrics_after" 'cache_hits_total{cache="user"}')
    
    echo "Cache hits before update: $hits_before"
    echo "Cache hits after update: $hits_after"
    
    if [ "$hits_after" -eq "$hits_before" ]; then
        print_result 0 "Cache invalidation working - no new hits after update"
    else
        print_result 1 "Cache invalidation may not be working"
    fi
    
    # Cleanup
    echo "Cleaning up test user..."
    curl -s -X DELETE "$BASE_URL/api/users/$user_id" > /dev/null
else
    print_result 1 "Failed to create test user"
fi

echo ""
echo "Test 4: Active orders caching"
echo "------------------------------"

# Get active orders multiple times
echo "Fetching active orders 5 times..."
for i in {1..5}; do
    curl -s "$BASE_URL/api/orders?status=active" > /dev/null
done

# Get metrics
metrics=$(curl -s "$METRICS_URL")
order_cache_hits=$(get_metric "$metrics" 'cache_hits_total{cache="order"}')
order_cache_misses=$(get_metric "$metrics" 'cache_misses_total{cache="order"}')

echo "Order cache hits: $order_cache_hits"
echo "Order cache misses: $order_cache_misses"

if [ "$order_cache_hits" -gt 0 ]; then
    print_result 0 "Active orders caching is working"
else
    print_result 1 "Active orders caching may not be working"
fi

echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo "All tests completed!"
echo ""
echo "To view detailed metrics, visit:"
echo "  - Prometheus: http://localhost:9090"
echo "  - Grafana: http://localhost:3000 (admin/admin)"
echo ""
echo "To view service metrics directly:"
echo "  curl $METRICS_URL"
echo ""
