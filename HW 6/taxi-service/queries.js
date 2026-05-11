// MongoDB CRUD Operations for Taxi Service
// Variant 16: Taxi Service (Migration from PostgreSQL to MongoDB)

db = db.getSiblingDB('taxi_db');

print("=== CREATE OPERATIONS ===");

// Q1: Create User (passenger)
const newPassenger = {
  login: "test_passenger",
  email: "test@passenger.com",
  first_name: "Тест",
  last_name: "Пассажир",
  phone: "+79009990000",
  password_hash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8",
  role: "passenger",
  created_at: new Date(),
  updated_at: new Date()
};
const passengerResult = db.users.insertOne(newPassenger);
print("Q1 Create Passenger: " + passengerResult.insertedId);

// Q2: Create Driver Profile
const testDriver = db.users.findOne({login: "test_passenger"});
const newDriver = {
  user_id: testDriver._id,
  license_number: "ТТ999999",
  car_model: "Tesla Model 3",
  car_plate: "Т000ТТ99",
  is_available: true,
  rating: 5.0,
  created_at: new Date(),
  updated_at: new Date()
};
const driverResult = db.drivers.insertOne(newDriver);
print("Q2 Create Driver: " + driverResult.insertedId);

// Q3: Create Order
const newOrder = {
  user_id: testDriver._id,
  driver_id: null,
  passenger: {
    name: "Тест Пассажир",
    phone: "+79009990000"
  },
  pickup_address: "ул. Тестовая, 1",
  destination_address: "ул. Тестовая, 2",
  status: "active",
  price: null,
  created_at: new Date(),
  completed_at: null
};
const orderResult = db.orders.insertOne(newOrder);
print("Q3 Create Order: " + orderResult.insertedId);

print("\n=== READ OPERATIONS ===");

// Q4: Find User by Login
const userByLogin = db.users.findOne({login: "ivan_petrov"});
print("Q4 Find User by Login: " + userByLogin.login + " (" + userByLogin.email + ")");

// Q5: Search Users by Name Mask
const usersByName = db.users.find({
  $or: [
    {first_name: {$regex: "Иван", $options: "i"}}, 
    {last_name: {$regex: "Петров", $options: "i"}}
  ]
}).toArray();
print("Q5 Search Users by Name: found " + usersByName.length + " users");
usersByName.forEach(function(u) { 
  print("  - " + u.login + ": " + u.first_name + " " + u.last_name + " (" + u.role + ")"); 
});

// Q6: Find User by ID
const userById = db.users.findOne({_id: userByLogin._id});
print("Q6 Find User by ID: " + userById.login);

// Q7: Find Driver by User ID
const driverByUserId = db.drivers.findOne({user_id: userByLogin._id});
if (driverByUserId) {
  print("Q7 Find Driver by User ID: " + driverByUserId.car_model + " (" + driverByUserId.car_plate + ")");
} else {
  print("Q7 Find Driver by User ID: User is not a driver");
}

// Q8: Find Available Drivers
const availableDrivers = db.drivers.find({is_available: true}).toArray();
print("Q8 Find Available Drivers: " + availableDrivers.length + " drivers");
availableDrivers.forEach(function(d) { 
  print("  - " + d.car_model + " " + d.car_plate + " (rating: " + d.rating + ")"); 
});

// Q9: Find Active Orders
const activeOrders = db.orders.find({status: "active"}).sort({created_at: -1}).toArray();
print("Q9 Find Active Orders: " + activeOrders.length + " orders");
activeOrders.forEach(function(o) { 
  print("  - " + o.pickup_address + " -> " + o.destination_address); 
});

// Q10: Find Order by ID
const orderById = db.orders.findOne({_id: orderResult.insertedId});
print("Q10 Find Order by ID: " + orderById.pickup_address + " -> " + orderById.destination_address);

// Q11: Find Orders by User ID
const ordersByUser = db.orders.find({user_id: userByLogin._id}).sort({created_at: -1}).toArray();
print("Q11 Find Orders by User ID: " + ordersByUser.length + " orders");
ordersByUser.forEach(function(o) { 
  print("  - " + o.status + ": " + o.pickup_address + " -> " + o.destination_address); 
});

// Q12: Find Orders by Driver ID
const driver = db.drivers.findOne({is_available: true});
if (driver) {
  const ordersByDriver = db.orders.find({driver_id: driver._id}).sort({created_at: -1}).toArray();
  print("Q12 Find Orders by Driver ID: " + ordersByDriver.length + " orders");
  ordersByDriver.forEach(function(o) { 
    print("  - " + o.status + ": " + o.pickup_address + " -> " + o.destination_address); 
  });
}

print("\n=== UPDATE OPERATIONS ===");

// Q13: Update User
db.users.updateOne(
  {login: "test_passenger"}, 
  {$set: {phone: "+79009991111"}, $currentDate: {updated_at: true}}
);
print("Q13 Update User: phone changed to " + db.users.findOne({login: "test_passenger"}).phone);

// Q14: Update Driver Availability
if (driver) {
  db.drivers.updateOne(
    {_id: driver._id}, 
    {$set: {is_available: false}, $currentDate: {updated_at: true}}
  );
  print("Q14 Update Driver Availability: " + db.drivers.findOne({_id: driver._id}).car_plate + " is now " + (db.drivers.findOne({_id: driver._id}).is_available ? "available" : "unavailable"));
}

// Q15: Assign Driver to Order
db.orders.updateOne(
  {_id: orderResult.insertedId}, 
  {$set: {driver_id: driver._id, status: "accepted"}}
);
const updatedOrder = db.orders.findOne({_id: orderResult.insertedId});
print("Q15 Assign Driver to Order: order status = " + updatedOrder.status);

// Q16: Complete Order
db.orders.updateOne(
  {_id: orderResult.insertedId},
  {$set: {status: "completed", price: 450.00, completed_at: new Date()}}
);
const completedOrder = db.orders.findOne({_id: orderResult.insertedId});
print("Q16 Complete Order: status = " + completedOrder.status + ", price = " + completedOrder.price);

// Q17: Cancel Order
const activeOrder = db.orders.findOne({status: "active"});
if (activeOrder) {
  db.orders.updateOne(
    {_id: activeOrder._id}, 
    {$set: {status: "cancelled"}}
  );
  print("Q17 Cancel Order: order " + activeOrder._id + " status = " + db.orders.findOne({_id: activeOrder._id}).status);
}

// Q18: $inc operator - increment driver rating
if (driver) {
  db.drivers.updateOne({_id: driver._id}, {$inc: {rating: 0.1}});
  print("Q18 $inc Operator: driver rating = " + db.drivers.findOne({_id: driver._id}).rating);
}

print("\n=== DELETE OPERATIONS ===");

// Q19: Delete Order
const delOrder = db.orders.deleteOne({_id: orderResult.insertedId});
print("Q19 Delete Order: " + delOrder.deletedCount + " deleted");

// Q20: Delete Driver
const delDriver = db.drivers.deleteOne({_id: driverResult.insertedId});
print("Q20 Delete Driver: " + delDriver.deletedCount + " deleted");

// Q21: Delete User
const delUser = db.users.deleteOne({login: "test_passenger"});
print("Q21 Delete User: " + delUser.deletedCount + " deleted");

print("\n=== AGGREGATION OPERATIONS ===");

// Q22: Orders by Status
const ordersByStatus = db.orders.aggregate([
  {$group: {_id: "$status", count: {$sum: 1}}}
]).toArray();
print("Q22 Orders by Status:");
ordersByStatus.forEach(function(s) { 
  print("  - " + s._id + ": " + s.count); 
});

// Q23: Average Order Price
const avgPrice = db.orders.aggregate([
  {$match: {status: "completed", price: {$ne: null}}},
  {$group: {_id: null, avgPrice: {$avg: "$price"}}}
]).toArray();
if (avgPrice.length > 0) {
  print("Q23 Average Order Price: " + avgPrice[0].avgPrice.toFixed(2));
}

// Q24: Top Drivers by Completed Orders
const topDrivers = db.orders.aggregate([
  {$match: {status: "completed", driver_id: {$ne: null}}},
  {$group: {_id: "$driver_id", completedOrders: {$sum: 1}}},
  {$sort: {completedOrders: -1}},
  {$limit: 3}
]).toArray();
print("Q24 Top Drivers by Completed Orders:");
topDrivers.forEach(function(d) {
  const driverInfo = db.drivers.findOne({_id: d._id});
  if (driverInfo) {
    print("  - " + driverInfo.car_model + ": " + d.completedOrders + " orders");
  }
});

print("\n=== QUERIES COMPLETED ===");
