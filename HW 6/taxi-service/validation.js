// validation.js
// MongoDB Schema Validation Tests for Taxi Service
// Tests that $jsonSchema validation works correctly

db = db.getSiblingDB('taxi_db');

print("=== SCHEMA VALIDATION TESTS ===\n");

// ============================================
// Test 1: Valid user insertion - should succeed
// ============================================
print("Test 1: Insert valid user");
try {
  const ts = new Timestamp();
  const validUser = {
    login: "test_valid_user_" + ts.t,
    email: "test_" + ts.t + "@example.com",
    first_name: "Тест",
    last_name: "Пользователь",
    phone: "+79001234567",
    password_hash: "abc123hash",
    role: "passenger",
    created_at: new Date()
  };
  const result = db.users.insertOne(validUser);
  print("  ✓ SUCCESS: Valid user inserted with ID: " + result.insertedId);
  
  // Cleanup
  db.users.deleteOne({_id: result.insertedId});
} catch (e) {
  print("  ✗ FAILED: " + e.message);
}

// ============================================
// Test 2: Invalid email (no @ symbol) - should fail
// ============================================
print("\nTest 2: Insert user with invalid email (no @)");
try {
  db.users.insertOne({
    login: "test_invalid_email",
    email: "not-an-email",
    first_name: "Тест",
    last_name: "Пользователь",
    phone: "+79001234567",
    password_hash: "abc123hash",
    role: "passenger",
    created_at: new Date()
  });
  print("  ✗ FAILED: Should have rejected invalid email");
} catch (e) {
  if (e.message.includes("Document failed validation")) {
    print("  ✓ SUCCESS: Document failed validation as expected");
  } else {
    print("  ✗ FAILED: Wrong error: " + e.message);
  }
}

// ============================================
// Test 3: Invalid email (no domain) - should fail
// ============================================
print("\nTest 3: Insert user with invalid email (no domain)");
try {
  db.users.insertOne({
    login: "test_invalid_email2",
    email: "test@",
    first_name: "Тест",
    last_name: "Пользователь",
    phone: "+79001234567",
    password_hash: "abc123hash",
    role: "passenger",
    created_at: new Date()
  });
  print("  ✗ FAILED: Should have rejected invalid email");
} catch (e) {
  if (e.message.includes("Document failed validation")) {
    print("  ✓ SUCCESS: Document failed validation as expected");
  } else {
    print("  ✗ FAILED: Wrong error: " + e.message);
  }
}

// ============================================
// Test 4: Invalid phone (no + prefix) - should fail
// ============================================
print("\nTest 4: Insert user with invalid phone (no + prefix)");
try {
  db.users.insertOne({
    login: "test_invalid_phone",
    email: "test4@example.com",
    first_name: "Тест",
    last_name: "Пользователь",
    phone: "89001234567",  // Without +
    password_hash: "abc123hash",
    role: "passenger",
    created_at: new Date()
  });
  print("  ✗ FAILED: Should have rejected invalid phone");
} catch (e) {
  if (e.message.includes("Document failed validation")) {
    print("  ✓ SUCCESS: Document failed validation as expected");
  } else {
    print("  ✗ FAILED: Wrong error: " + e.message);
  }
}

// ============================================
// Test 5: Invalid role (not passenger/driver) - should fail
// ============================================
print("\nTest 5: Insert user with invalid role");
try {
  db.users.insertOne({
    login: "test_invalid_role",
    email: "test5@example.com",
    first_name: "Тест",
    last_name: "Пользователь",
    phone: "+79001234567",
    password_hash: "abc123hash",
    role: "admin",  // Invalid role
    created_at: new Date()
  });
  print("  ✗ FAILED: Should have rejected invalid role");
} catch (e) {
  if (e.message.includes("Document failed validation")) {
    print("  ✓ SUCCESS: Document failed validation as expected");
  } else {
    print("  ✗ FAILED: Wrong error: " + e.message);
  }
}

// ============================================
// Test 6: Missing required field (login) - should fail
// ============================================
print("\nTest 6: Insert user without required field (login)");
try {
  db.users.insertOne({
    email: "test6@example.com",
    first_name: "Тест",
    last_name: "Пользователь",
    phone: "+79001234567",
    password_hash: "abc123hash",
    role: "passenger",
    created_at: new Date()
  });
  print("  ✗ FAILED: Should have rejected missing login");
} catch (e) {
  if (e.message.includes("Document failed validation")) {
    print("  ✓ SUCCESS: Document failed validation as expected");
  } else {
    print("  ✗ FAILED: Wrong error: " + e.message);
  }
}

// ============================================
// Test 7: Invalid login (special characters) - should fail
// ============================================
print("\nTest 7: Insert user with invalid login (special chars)");
try {
  db.users.insertOne({
    login: "test-user@123",  // Contains @ which is not allowed
    email: "test7@example.com",
    first_name: "Тест",
    last_name: "Пользователь",
    phone: "+79001234567",
    password_hash: "abc123hash",
    role: "passenger",
    created_at: new Date()
  });
  print("  ✗ FAILED: Should have rejected invalid login");
} catch (e) {
  if (e.message.includes("Document failed validation")) {
    print("  ✓ SUCCESS: Document failed validation as expected");
  } else {
    print("  ✗ FAILED: Wrong error: " + e.message);
  }
}

// ============================================
// Test 8: Empty first_name - should fail (minLength: 1)
// ============================================
print("\nTest 8: Insert user with empty first_name");
try {
  db.users.insertOne({
    login: "test_empty_name",
    email: "test8@example.com",
    first_name: "",  // Empty string
    last_name: "Пользователь",
    phone: "+79001234567",
    password_hash: "abc123hash",
    role: "passenger",
    created_at: new Date()
  });
  print("  ✗ FAILED: Should have rejected empty first_name");
} catch (e) {
  if (e.message.includes("Document failed validation")) {
    print("  ✓ SUCCESS: Document failed validation as expected");
  } else {
    print("  ✗ FAILED: Wrong error: " + e.message);
  }
}

// ============================================
// Test 9: Valid order insertion - should succeed
// ============================================
print("\nTest 9: Insert valid order");
try {
  const user = db.users.findOne({login: "ivan_petrov"});
  if (user) {
    const validOrder = {
      user_id: user._id,
      driver_id: null,
      passenger: {
        name: "Иван Петров",
        phone: "+79001234567"
      },
      pickup_address: "ул. Ленина, 1",
      destination_address: "ул. Пушкина, 10",
      status: "active",
      price: null,
      created_at: new Date(),
      completed_at: null
    };
    const result = db.orders.insertOne(validOrder);
    print("  ✓ SUCCESS: Valid order inserted with ID: " + result.insertedId);
    
    // Cleanup
    db.orders.deleteOne({_id: result.insertedId});
  } else {
    print("  ⚠ SKIPPED: No test user found");
  }
} catch (e) {
  print("  ✗ FAILED: " + e.message);
}

// ============================================
// Test 10: Invalid order status - should fail
// ============================================
print("\nTest 10: Insert order with invalid status");
try {
  const user = db.users.findOne({login: "ivan_petrov"});
  if (user) {
    db.orders.insertOne({
      user_id: user._id,
      driver_id: null,
      passenger: {
        name: "Иван Петров",
        phone: "+79001234567"
      },
      pickup_address: "ул. Ленина, 1",
      destination_address: "ул. Пушкина, 10",
      status: "invalid_status",  // Invalid status
      price: null,
      created_at: new Date(),
      completed_at: null
    });
    print("  ✗ FAILED: Should have rejected invalid status");
  } else {
    print("  ⚠ SKIPPED: No test user found");
  }
} catch (e) {
  if (e.message.includes("Document failed validation")) {
    print("  ✓ SUCCESS: Document failed validation as expected");
  } else {
    print("  ✗ FAILED: Wrong error: " + e.message);
  }
}

// ============================================
// Test 11: Negative price - should fail (minimum: 0)
// ============================================
print("\nTest 11: Insert order with negative price");
try {
  const user = db.users.findOne({login: "ivan_petrov"});
  if (user) {
    db.orders.insertOne({
      user_id: user._id,
      driver_id: null,
      passenger: {
        name: "Иван Петров",
        phone: "+79001234567"
      },
      pickup_address: "ул. Ленина, 1",
      destination_address: "ул. Пушкина, 10",
      status: "completed",
      price: -100.0,  // Negative price
      created_at: new Date(),
      completed_at: new Date()
    });
    print("  ✗ FAILED: Should have rejected negative price");
  } else {
    print("  ⚠ SKIPPED: No test user found");
  }
} catch (e) {
  if (e.message.includes("Document failed validation")) {
    print("  ✓ SUCCESS: Document failed validation as expected");
  } else {
    print("  ✗ FAILED: Wrong error: " + e.message);
  }
}

// ============================================
// Test 12: Valid driver insertion - should succeed
// ============================================
print("\nTest 12: Insert valid driver");
try {
  const ts = new Timestamp();
  // Create a temporary user for driver
  const tempUser = db.users.insertOne({
    login: "temp_driver_" + ts.t,
    email: "temp_" + ts.t + "@driver.com",
    first_name: "Тест",
    last_name: "Водитель",
    phone: "+7900" + ts.t,
    password_hash: "abc123hash",
    role: "driver",
    created_at: new Date()
  });
  
  const validDriver = {
    user_id: tempUser.insertedId,
    license_number: "АА" + ts.t,
    car_model: "Toyota Camry",
    car_plate: "А" + (ts.t % 100) + "АА" + (ts.t % 100),
    is_available: true,
    created_at: new Date(),
    updated_at: new Date()
  };
  const result = db.drivers.insertOne(validDriver);
  print("  ✓ SUCCESS: Valid driver inserted with ID: " + result.insertedId);
  
  // Cleanup
  db.drivers.deleteOne({_id: result.insertedId});
  db.users.deleteOne({_id: tempUser.insertedId});
} catch (e) {
  print("  ✗ FAILED: " + e.message);
}

// ============================================
// Test 13: Invalid is_available type - should fail
// ============================================
print("\nTest 13: Insert driver with invalid is_available type");
try {
  const user = db.users.findOne({role: "driver"});
  if (user) {
    db.drivers.insertOne({
      user_id: user._id,
      license_number: "АА123456",
      car_model: "Toyota Camry",
      car_plate: "А777АА77",
      is_available: "yes",  // Should be boolean
      created_at: new Date(),
      updated_at: new Date()
    });
    print("  ✗ FAILED: Should have rejected invalid is_available type");
  } else {
    print("  ⚠ SKIPPED: No driver user found");
  }
} catch (e) {
  if (e.message.includes("Document failed validation")) {
    print("  ✓ SUCCESS: Document failed validation as expected");
  } else {
    print("  ✗ FAILED: Wrong error: " + e.message);
  }
}

print("\n=== VALIDATION TESTS COMPLETED ===");
