// init-mongo.js
// MongoDB initialization script for Taxi Service

db = db.getSiblingDB('taxi_db');

// Create application user (matching docker-compose.yaml credentials)
db.createUser({
  user: 'taxi_user',
  pwd: 'taxi_pass',
  roles: [
    { role: 'readWrite', db: 'taxi_db' }
  ]
});

// Switch to the database
db = db.getSiblingDB('taxi_db');

// ============================================
// Create users collection with schema validation
// ============================================
db.createCollection('users', {
  validator: {
    $jsonSchema: {
      bsonType: "object",
      required: ["login", "email", "first_name", "last_name", "phone", "password_hash", "role", "created_at"],
      properties: {
        login: {
          bsonType: "string",
          description: "Unique user login",
          pattern: "^[a-zA-Z0-9_]+$"
        },
        email: {
          bsonType: "string",
          description: "Unique email",
          pattern: "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$"
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
          description: "Password hash"
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
});

// ============================================
// Create drivers collection with schema validation
// ============================================
db.createCollection('drivers', {
  validator: {
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
});

// ============================================
// Create orders collection with schema validation
// ============================================
db.createCollection('orders', {
  validator: {
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
          description: "Reference to driver"
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
          bsonType: ["double", "int", "null"],
          minimum: 0,
          description: "Trip price"
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
});

// ============================================
// Create indexes for users collection
// ============================================
db.users.createIndex({ login: 1 }, { unique: true });
db.users.createIndex({ email: 1 }, { unique: true });
db.users.createIndex({ first_name: 1, last_name: 1 });
db.users.createIndex({ role: 1 });

// ============================================
// Create indexes for drivers collection
// ============================================
db.drivers.createIndex({ user_id: 1 }, { unique: true });
db.drivers.createIndex({ is_available: 1 });

// ============================================
// Create indexes for orders collection
// ============================================
db.orders.createIndex({ user_id: 1, created_at: -1 });
db.orders.createIndex({ driver_id: 1, status: 1 });
db.orders.createIndex({ status: 1, created_at: -1 });

print("MongoDB initialization completed!");
print("Collections created: users, drivers, orders");
print("Indexes created successfully");

// ============================================
// Create example user for API documentation (ivan_petrov)
// Password: securePass123 (hash: 0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879)
// ============================================
db.users.insertOne({
  login: "ivan_petrov",
  email: "ivan@example.com",
  first_name: "Иван",
  last_name: "Петров",
  phone: "+79001234567",
  password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879",
  role: "passenger",
  created_at: new Date("2024-01-15T10:30:00Z"),
  updated_at: new Date("2024-01-15T10:30:00Z")
});

print("Example user created: ivan_petrov");
