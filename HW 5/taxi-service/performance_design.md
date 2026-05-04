# Performance Design: Taxi Service Optimization

## Hot Paths Analysis

### Frequently Executed Operations

1. **User Lookup by Login**
   - Endpoint: `GET /api/v1/users/by-login?login=...`
   - Frequency: High (every user action)
   - Current latency: 50-100ms (MongoDB query)
   - Target latency: < 5ms (cached)

2. **User Search by Name Mask**
   - Endpoint: `GET /api/v1/users/search?mask=...`
   - Frequency: Medium
   - Current latency: 80-150ms (MongoDB aggregation)
   - Target latency: < 10ms (cached)

3. **Driver Lookup by Login**
   - Endpoint: `GET /api/v1/drivers/by-login?login=...`
   - Frequency: High (order matching)
   - Current latency: 50-100ms (MongoDB query)
   - Target latency: < 5ms (cached)

4. **Active Orders Retrieval**
   - Endpoint: `GET /api/v1/orders/active`
   - Frequency: Very High (drivers polling every 5-10s)
   - Current latency: 100-200ms (MongoDB aggregation)
   - Target latency: < 10ms (cached)

5. **Order History**
   - Endpoint: `GET /api/v1/orders/history/{userId}`
   - Frequency: Medium
   - Current latency: 100-200ms (MongoDB query with filters)
   - Target latency: < 20ms (cached)

## Slow Operations

1. **MongoDB Queries**
   - Latency: 50-100ms per query
   - Impact: High (blocking operations)

2. **Aggregations for Active Orders**
   - Latency: 100-200ms
   - Impact: Very High (called frequently by drivers)

3. **Complex Filters for Order History**
   - Latency: 100-200ms
   - Impact: Medium

## Performance Requirements

### Response Time Targets
- Cached operations: < 5ms
- Non-cached operations: < 100ms
- P99 latency: < 200ms

### Throughput Targets
- Cached endpoints: 1000+ RPS
- Non-cached endpoints: 100+ RPS
- Cache hit rate: > 80%

## Caching Strategy

### Cache Patterns

1. **Cache-Aside Pattern**
   - Used for: Users, Drivers
   - Flow: Check cache → Miss → Query DB → Update cache → Return
   - TTL: 5 minutes
   - Invalidation: On user/driver update

2. **Read-Through Pattern**
   - Used for: Active Orders
   - Flow: Check cache → Miss → Cache loader queries DB → Update cache → Return
   - TTL: 30 seconds (short due to frequent updates)
   - Invalidation: On order status change

3. **Write-Through Pattern**
   - Used for: Order creation
   - Flow: Write to DB → Update cache → Return
   - TTL: 30 seconds

### Cache Configuration

| Cache Type | TTL | Max Size | Eviction Policy |
|------------|-----|----------|-----------------|
| User Cache | 5 min | 10,000 entries | LRU |
| Driver Cache | 5 min | 5,000 entries | LRU |
| Order Cache | 30 sec | 1,000 entries | LRU |

### Cache Keys

- User: `user:{login}`
- Driver: `driver:{login}`
- Active Orders: `orders:active`
- Order History: `orders:history:{userId}`

## Rate Limiting Strategy

### Token Bucket Algorithm

**Configuration:**
- Capacity: Maximum tokens in bucket
- Refill Rate: Tokens added per second
- Time Window: Measurement period

### Rate Limits

| Endpoint | Limit | Time Window | Key |
|----------|-------|-------------|-----|
| POST /api/v1/users | 10 req/min | 60s | Client IP |
| POST /api/v1/drivers | 10 req/min | 60s | Client IP |
| POST /api/v1/orders | 20 req/min | 60s | User ID |
| GET /api/v1/orders/active | 100 req/min | 60s | Driver ID |
| All other endpoints | 1000 req/min | 60s | Client IP |

### Rate Limit Headers

- `X-RateLimit-Limit`: Total limit
- `X-RateLimit-Remaining`: Remaining requests
- `X-RateLimit-Reset`: Unix timestamp when limit resets

## Monitoring Strategy

### Metrics to Track

1. **Cache Metrics**
   - Cache hits per endpoint
   - Cache misses per endpoint
   - Cache hit rate
   - Cache size
   - Cache eviction count

2. **Rate Limit Metrics**
   - Rate limit violations per endpoint
   - Rate limit resets
   - Token bucket utilization

3. **Performance Metrics**
   - Request latency (P50, P95, P99)
   - Request throughput (RPS)
   - Error rate

### Metrics Endpoint

- Path: `/metrics`
- Format: JSON
- Update frequency: Real-time

## Implementation Priority

1. **Phase 1**: Implement caching for user/driver lookups (highest impact)
2. **Phase 2**: Implement caching for active orders (very high impact)
3. **Phase 3**: Implement rate limiting (protection)
4. **Phase 4**: Implement metrics and monitoring (observability)

## Expected Performance Improvements

- **User/Driver Lookup**: 50-100ms → < 5ms (10-20x improvement)
- **Active Orders**: 100-200ms → < 10ms (10-20x improvement)
- **Overall Throughput**: 100 RPS → 1000+ RPS (10x improvement)
- **Cache Hit Rate**: Target > 80%
