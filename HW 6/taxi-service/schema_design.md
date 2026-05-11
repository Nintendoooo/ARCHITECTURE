# MongoDB Schema Design for Taxi Service

## Домашнее задание 4: Проектирование и работа с MongoDB

**Дата создания:** 2026-04-18  
**Вариант:** 16 - Система заказа такси (Uber)  
**СУБД:** MongoDB 7.0  
**Подход:** Гибридный (Hybrid)  

---

## Оглавление

1. [Обзор архитектуры](#обзор-архитектуры)
2. [Проектирование коллекций](#проектирование-коллекций)
3. [Обоснование выбора подхода](#обоснование-выбора-подхода)
4. [Сравнение подходов](#сравнение-подходов)
5. [Стратегия индексирования](#стратегия-индексирования)
6. [Примеры запросов](#примеры-запросов)
7. [Валидация схем](#валидация-схем)

---

## Обзор архитектуры

### ER-диаграмма MongoDB коллекций

```
┌─────────────────────────────────────────────────────────────┐
│                      MongoDB 7.0                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  users (коллекция)                                          │
│  ├─ _id (ObjectId PK)                                       │
│  ├─ login (UNIQUE)                                          │
│  ├─ email (UNIQUE)                                          │
│  ├─ first_name, last_name                                   │
│  ├─ phone                                                   │
│  ├─ password_hash                                           │
│  ├─ role (passenger | driver)                               │
│  └─ created_at, updated_at                                  │
│                                                             │
│  drivers (коллекция)                                        │
│  ├─ _id (ObjectId PK)                                       │
│  ├─ user_id (ObjectId) ──► users._id (reference)            │
│  ├─ license_number                                          │
│  ├─ car_model                                               │
│  ├─ car_plate                                               │
│  ├─ is_available (boolean)                                  │
│  ├─ rating (double)                                         │
│  └─ created_at, updated_at                                  │
│                                                             │
│  orders (коллекция)                                         │
│  ├─ _id (ObjectId PK)                                       │
│  ├─ user_id (ObjectId) ──► users._id (reference)            │
│  ├─ driver_id (ObjectId, nullable) ──► drivers._id          │
│  ├─ passenger (EMBEDDED: {name, phone})                     │
│  ├─ pickup_address                                          │
│  ├─ destination_address                                     │
│  ├─ status (active | accepted | completed | cancelled)      │
│  ├─ price (double, nullable)                                │
│  ├─ created_at                                              │
│  └─ completed_at (datetime, nullable)                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Связи между коллекциями

| Связь | Тип | Обоснование |
|-------|-----|-------------|
| `drivers.user_id` → `users._id` | Reference (ObjectId) | Связь 1:1 между пользователем и профилем водителя |
| `orders.user_id` → `users._id` | Reference (ObjectId) | Заказ принадлежит пассажиру, требуется история поездок |
| `orders.driver_id` → `drivers._id` | Reference (ObjectId, nullable) | Заказ может быть принят водителем (связь 1:many) |
| `orders.passenger` (embedded) | Embedded | Информация о пассажире встроена для быстрого отображения |

---

## Проектирование коллекций

### Коллекция `users`

#### Структура документа

```javascript
{
  _id: ObjectId("507f1f77bcf86cd799439011"),
  login: "ivan_petrov",
  email: "ivan@example.com",
  first_name: "Иван",
  last_name: "Петров",
  phone: "+79001234567",
  password_hash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8",
  role: "passenger",
  created_at: ISODate("2024-01-15T10:30:00Z"),
  updated_at: ISODate("2024-01-15T10:30:00Z")
}
```

#### Индексы

| Индекс | Тип | Обоснование |
|--------|-----|-------------|
| `{ login: 1 }` | UNIQUE | Быстрый поиск по логину (API: Поиск пользователя по логину) |
| `{ email: 1 }` | UNIQUE | Проверка уникальности email при регистрации |
| `{ first_name: 1, last_name: 1 }` | COMPOSITE | Поиск по маске имени (API: Поиск по маске имя+фамилия) |
| `{ role: 1 }` | SINGLE | Фильтрация пользователей по роли (пассажиры/водители) |
| `{ created_at: -1 }` | SINGLE | Сортировка по дате создания |

#### Валидация схемы

```javascript
{
  $jsonSchema: {
    bsonType: "object",
    required: ["login", "email", "first_name", "last_name", "phone", "password_hash", "role", "created_at"],
    properties: {
      login: {
        bsonType: "string",
        pattern: "^[a-zA-Z0-9_]+$",
        description: "Unique user login"
      },
      email: {
        bsonType: "string",
        pattern: "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$",
        description: "Unique email"
      },
      first_name: {
        bsonType: "string",
        minLength: 1,
        description: "User first name"
      },
      last_name: {
        bsonType: "string",
        minLength: 1,
        description: "User last name"
      },
      phone: {
        bsonType: "string",
        pattern: "^\\+[0-9]+$",
        description: "Phone number"
      },
      password_hash: {
        bsonType: "string",
        description: "Password hash (SHA-256)"
      },
      role: {
        enum: ["passenger", "driver"],
        description: "User role"
      },
      created_at: {
        bsonType: "date",
        description: "Creation date"
      },
      updated_at: {
        bsonType: "date",
        description: "Last update date"
      }
    }
  }
}
```

---

### Коллекция `drivers`

#### Структура документа

```javascript
{
  _id: ObjectId("507f1f77bcf86cd799439012"),
  user_id: ObjectId("507f1f77bcf86cd799439011"),  // Reference to users._id
  license_number: "АА123456",
  car_model: "Toyota Camry",
  car_plate: "А777АА77",
  is_available: true,
  rating: 4.8,
  created_at: ISODate("2024-01-17T09:00:00Z"),
  updated_at: ISODate("2024-01-17T09:00:00Z")
}
```

> **Примечание:** `user_id` — поле типа `ObjectId`, ссылающееся на документ пользователя в коллекции `users`. Это обеспечивает целостность данных и позволяет легко получить информацию о водителе через связь с пользователем.

#### Индексы

| Индекс | Тип | Обоснование |
|--------|-----|-------------|
| `{ user_id: 1 }` | UNIQUE | Связь 1:1 с users (каждый пользователь может иметь только один профиль водителя) |
| `{ is_available: 1 }` | SINGLE | Быстрый поиск доступных водителей (API: Принятие заказа) |
| `{ rating: -1 }` | SINGLE | Сортировка водителей по рейтингу |
| `{ created_at: -1 }` | SINGLE | Сортировка по дате регистрации |

#### Валидация схемы

```javascript
{
  $jsonSchema: {
    bsonType: "object",
    required: ["user_id", "license_number", "car_model", "car_plate", "is_available", "created_at"],
    properties: {
      user_id: {
        bsonType: "objectId",
        description: "Reference to user"
      },
      license_number: {
        bsonType: "string",
        minLength: 1,
        description: "Driver license number"
      },
      car_model: {
        bsonType: "string",
        minLength: 1,
        description: "Car model"
      },
      car_plate: {
        bsonType: "string",
        minLength: 1,
        description: "License plate"
      },
      is_available: {
        bsonType: "bool",
        description: "Available for orders"
      },
      rating: {
        bsonType: "double",
        minimum: 0,
        maximum: 5,
        description: "Driver rating (0-5)"
      },
      created_at: {
        bsonType: "date",
        description: "Creation date"
      },
      updated_at: {
        bsonType: "date",
        description: "Last update date"
      }
    }
  }
}
```

---

### Коллекция `orders`

#### Структура документа

```javascript
{
  _id: ObjectId("507f1f77bcf86cd799439013"),
  user_id: ObjectId("507f1f77bcf86cd799439011"),  // Reference to users._id (passenger)
  driver_id: null,  // Reference to drivers._id (nullable until accepted)
  passenger: {
    name: "Иван Петров",
    phone: "+79001234567"
  },
  pickup_address: "ул. Ленина, 10",
  destination_address: "ул. Пушкина, 25",
  status: "active",
  price: null,
  created_at: ISODate("2024-01-20T14:30:00Z"),
  completed_at: null
}
```

> **Примечание:** Поле `passenger` содержит встроенный объект с информацией о пассажире. Это позволяет получить все данные о заказе в одном запросе без необходимости дополнительного JOIN с коллекцией users.

#### Индексы

| Индекс | Тип | Обоснование |
|--------|-----|-------------|
| `{ user_id: 1, created_at: -1 }` | COMPOSITE | История поездок пользователя (API: История поездок) |
| `{ driver_id: 1, status: 1 }` | COMPOSITE | Заказы водителя по статусу |
| `{ status: 1, created_at: -1 }` | COMPOSITE | Активные заказы (API: Получение активных заказов) |
| `{ created_at: -1 }` | SINGLE | Сортировка по дате создания |

#### Валидация схемы

```javascript
{
  $jsonSchema: {
    bsonType: "object",
    required: ["user_id", "pickup_address", "destination_address", "status", "created_at"],
    properties: {
      user_id: {
        bsonType: "objectId",
        description: "Reference to user (passenger)"
      },
      driver_id: {
        bsonType: ["objectId", "null"],
        description: "Reference to driver (nullable)"
      },
      passenger: {
        bsonType: "object",
        description: "Embedded passenger info",
        properties: {
          name: { bsonType: "string" },
          phone: { bsonType: "string" }
        }
      },
      pickup_address: {
        bsonType: "string",
        minLength: 1,
        description: "Pickup address"
      },
      destination_address: {
        bsonType: "string",
        minLength: 1,
        description: "Destination address"
      },
      status: {
        enum: ["active", "accepted", "completed", "cancelled"],
        description: "Order status"
      },
      price: {
        bsonType: ["double", "null"],
        minimum: 0,
        description: "Order price"
      },
      created_at: {
        bsonType: "date",
        description: "Creation date"
      },
      completed_at: {
        bsonType: ["date", "null"],
        description: "Completion date"
      }
    }
  }
}
```

---

## Обоснование выбора подхода

### 1. Reference vs Embedded для `drivers.user_id`

**Выбор:** Reference (ссылка)

**Обоснование:**
- **Связь 1:1** — каждый пользователь может иметь не более одного профиля водителя
- **Частое обновление** — данные водителя (доступность, рейтинг) часто меняются отдельно от пользователя
- **Нормализация** — избежание дублирования данных пользователя
- **Запросы:** При регистрации водителя нужно проверить, что пользователь существует; при получении водителя — получить его данные вместе с данными пользователя

**Пример запроса:**
```javascript
// Получение водителя с данными пользователя
db.drivers.aggregate([
  { $match: { _id: ObjectId("...") } },
  { $lookup: {
      from: "users",
      localField: "user_id",
      foreignField: "_id",
      as: "user"
    }
  },
  { $unwind: "$user" }
])
```

### 2. Reference vs Embedded для `orders.user_id`

**Выбор:** Reference (ссылка)

**Обоснование:**
- **Связь 1:many** — один пользователь может создать множество заказов
- **История поездок** — требуется эффективный поиск всех заказов пользователя
- **Целостность данных** — при изменении данных пользователя (имя, телефон) заказы не должны автоматически обновляться
- **Индексирование** — возможность создать составной индекс `{ user_id: 1, created_at: -1 }`

**Пример запроса:**
```javascript
// История поездок пользователя
db.orders.find({ user_id: ObjectId("...") }).sort({ created_at: -1 })
```

### 3. Reference vs Embedded для `orders.driver_id`

**Выбор:** Reference (ссылка, nullable)

**Обоснование:**
- **Связь 1:many** — один водитель может принять множество заказов
- **Динамическое состояние** — в момент создания заказа водитель ещё не назначен (null)
- **История заказов водителя** — требуется поиск всех заказов конкретного водителя
- **Атомарность** — принятие заказа водителем выполняется как atomic update

**Пример запроса:**
```javascript
// Принятие заказа водителем
db.orders.updateOne(
  { _id: ObjectId("..."), status: "active" },
  { $set: { driver_id: ObjectId("..."), status: "accepted" } }
)
```

### 4. Embedded для `orders.passenger`

**Выбор:** Embedded (встроенный документ)

**Обоснование:**
- **Частота чтения** — данные пассажира читаются при каждом просмотре заказа
- **Без изменений** — информация о пассажире в заказе не меняется после создания заказа
- **Один запрос** — не требуется дополнительный JOIN для получения информации о пассажире
- **Денормализация** — допустимо дублирование ради производительности

**Пример:**
```javascript
// При создании заказа
{
  user_id: ObjectId("..."),  // Ссылка для истории
  passenger: {               // Встроено для быстрого чтения
    name: "Иван Петров",
    phone: "+79001234567"
  },
  pickup_address: "...",
  destination_address: "..."
}
```

**Когда НЕ использовать embedded:**
- Если данные пассажира могут меняться и эти изменения должны отражаться в старых заказах
- Если заказов у пользователя тысячи и дублирование данных станет проблемой

---

## Сравнение подходов

### Embedded vs Reference: когда что использовать

| Критерий | Embedded | Reference |
|----------|----------|-----------|
| **Отношение** | 1:1, 1:few (до ~100) | 1:many, many:many |
| **Частота чтения** | Всегда читаются вместе | Могут читаться отдельно |
| **Частота обновлений** | Редко меняются | Часто меняются |
| **Размер данных** | Маленький (<16MB) | Любой |
| **Денормализация** | Допустима | Избегается |

### Для нашей системы

| Связь | Выбранный подход | Причина |
|-------|------------------|---------|
| users → drivers | Reference | Связь 1:1, разные жизненные циклы |
| users → orders | Reference | Связь 1:many, история поездок |
| drivers → orders | Reference | Связь 1:many, nullable |
| orders → passenger | **Embedded** | Частое чтение, без изменений |

---

## Стратегия индексирования

### users

```javascript
db.users.createIndex({ login: 1 }, { unique: true })
db.users.createIndex({ email: 1 }, { unique: true })
db.users.createIndex({ first_name: 1, last_name: 1 })
db.users.createIndex({ role: 1 })
db.users.createIndex({ created_at: -1 })
```

### drivers

```javascript
db.drivers.createIndex({ user_id: 1 }, { unique: true })
db.drivers.createIndex({ is_available: 1 })
db.drivers.createIndex({ rating: -1 })
db.drivers.createIndex({ created_at: -1 })
```

### orders

```javascript
db.orders.createIndex({ user_id: 1, created_at: -1 })
db.orders.createIndex({ driver_id: 1, status: 1 })
db.orders.createIndex({ status: 1, created_at: -1 })
db.orders.createIndex({ created_at: -1 })
```

---

## Примеры запросов

### Create Operations

```javascript
// Создание пользователя
db.users.insertOne({
  login: "ivan_petrov",
  email: "ivan@example.com",
  first_name: "Иван",
  last_name: "Петров",
  phone: "+79001234567",
  password_hash: "...",
  role: "passenger",
  created_at: new Date(),
  updated_at: new Date()
})

// Регистрация водителя
db.drivers.insertOne({
  user_id: ObjectId("..."),
  license_number: "АА123456",
  car_model: "Toyota Camry",
  car_plate: "А777АА77",
  is_available: true,
  rating: 5.0,
  created_at: new Date(),
  updated_at: new Date()
})

// Создание заказа
db.orders.insertOne({
  user_id: ObjectId("..."),
  driver_id: null,
  passenger: { name: "Иван Петров", phone: "+79001234567" },
  pickup_address: "ул. Ленина, 10",
  destination_address: "ул. Пушкина, 25",
  status: "active",
  price: null,
  created_at: new Date(),
  completed_at: null
})
```

### Read Operations

```javascript
// Поиск пользователя по логину
db.users.findOne({ login: "ivan_petrov" })

// Поиск по маске имя+фамилия
db.users.find({
  $or: [
    { first_name: { $regex: "Иван", $options: "i" } },
    { last_name: { $regex: "Петров", $options: "i" } }
  ]
})

// Активные заказы
db.orders.find({ status: "active" }).sort({ created_at: -1 })

// История поездок пользователя
db.orders.find({ user_id: ObjectId("...") }).sort({ created_at: -1 })

// Заказы водителя
db.orders.find({ driver_id: ObjectId("..."), status: "accepted" })
```

### Update Operations

```javascript
// Принятие заказа водителем
db.orders.updateOne(
  { _id: ObjectId("..."), status: "active" },
  { $set: { driver_id: ObjectId("..."), status: "accepted" } }
)

// Завершение поездки
db.orders.updateOne(
  { _id: ObjectId("..."), status: "accepted" },
  { $set: { status: "completed", price: 450.00, completed_at: new Date() } }
)

// Изменение доступности водителя
db.drivers.updateOne(
  { _id: ObjectId("...") },
  { $set: { is_available: false }, $currentDate: { updated_at: true } }
)
```

### Delete Operations

```javascript
// Отмена заказа
db.orders.updateOne(
  { _id: ObjectId("..."), status: "active" },
  { $set: { status: "cancelled" } }
)

// Удаление водителя (с сохранением истории)
db.drivers.deleteOne({ _id: ObjectId("...") })
```

---

## Валидация схем

### Тестирование валидации

```javascript
// Попытка вставить невалидные данные — успех
db.users.insertOne({
  login: "test_user",
  email: "test@example.com",
  first_name: "Тест",
  last_name: "Пользователь",
  phone: "+79001234567",
  password_hash: "abc123",
  role: "passenger",
  created_at: new Date()
})
// Результат: Success

// Попытка вставить невалидный email — ошибка
db.users.insertOne({
  login: "test_user2",
  email: "not-an-email",
  first_name: "Тест",
  last_name: "Пользователь",
  phone: "+79001234567",
  password_hash: "abc123",
  role: "passenger",
  created_at: new Date()
})
// Результат: Document failed validation

// Попытка вставить невалидный phone — ошибка
db.users.insertOne({
  login: "test_user3",
  email: "test3@example.com",
  first_name: "Тест",
  last_name: "Пользователь",
  phone: "89001234567",  // Без +
  password_hash: "abc123",
  role: "passenger",
  created_at: new Date()
})
// Результат: Document failed validation

// Попытка вставить невалидный role — ошибка
db.users.insertOne({
  login: "test_user4",
  email: "test4@example.com",
  first_name: "Тест",
  last_name: "Пользователь",
  phone: "+79001234567",
  password_hash: "abc123",
  role: "admin",  // Неверное значение
  created_at: new Date()
})
// Результат: Document failed validation
```

---

## Заключение

Спроектированная схема MongoDB для системы заказа такси использует гибридный подход:

1. **Reference** — для связей между коллекциями (users → drivers, users → orders, drivers → orders)
2. **Embedded** — для часто читаемых данных, которые не меняются (orders.passenger)

Выбор обоснован:
- Эффективные запросы для всех API операций
- Нормализация данных для избежания дублирования
- Возможность индексирования по ключевым полям
- Валидация схемы на уровне MongoDB
