// MongoDB Test Data for Taxi Service
// Variant 16: Taxi Service (Migration from PostgreSQL to MongoDB)

db = db.getSiblingDB('taxi_db');

// Clear existing data
db.users.deleteMany({});
db.drivers.deleteMany({});
db.orders.deleteMany({});

// Insert users (passengers and drivers)
const userResult = db.users.insertMany([
  // Passengers
  { login: "ivan_petrov", email: "ivan@example.com", first_name: "Иван", last_name: "Петров", phone: "+79001234567", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "passenger", created_at: new Date("2024-01-15T10:30:00Z"), updated_at: new Date("2024-01-15T10:30:00Z") },
  { login: "anna_smirnova", email: "anna@example.com", first_name: "Анна", last_name: "Смирнова", phone: "+79007654321", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "passenger", created_at: new Date("2024-01-16T11:45:00Z"), updated_at: new Date("2024-01-16T11:45:00Z") },
  { login: "alex_kuznetsov", email: "alex@example.com", first_name: "Алексей", last_name: "Кузнецов", phone: "+79005557777", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "passenger", created_at: new Date("2024-01-17T09:15:00Z"), updated_at: new Date("2024-01-17T09:15:00Z") },
  
  // Drivers (users with role "driver")
  { login: "mike_driver", email: "mike@driver.com", first_name: "Михаил", last_name: "Сидоров", phone: "+79003334455", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "driver", created_at: new Date("2024-01-17T09:00:00Z"), updated_at: new Date("2024-01-17T09:00:00Z") },
  { login: "sergey_driver", email: "sergey@driver.com", first_name: "Сергей", last_name: "Волков", phone: "+79004445566", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "driver", created_at: new Date("2024-01-18T08:00:00Z"), updated_at: new Date("2024-01-18T08:00:00Z") },
  { login: "dmitry_driver", email: "dmitry@driver.com", first_name: "Дмитрий", last_name: "Новиков", phone: "+79006667788", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "driver", created_at: new Date("2024-01-19T07:30:00Z"), updated_at: new Date("2024-01-19T07:30:00Z") },
  
  // Additional passengers
  { login: "olga_ivanova", email: "olga@example.com", first_name: "Ольга", last_name: "Иванова", phone: "+79008889999", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "passenger", created_at: new Date("2024-01-20T14:20:00Z"), updated_at: new Date("2024-01-20T14:20:00Z") },
  { login: "petr_sokolov", email: "petr@example.com", first_name: "Петр", last_name: "Соколов", phone: "+79001112233", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "passenger", created_at: new Date("2024-01-21T16:30:00Z"), updated_at: new Date("2024-01-21T16:30:00Z") },
  
  // Additional passengers (to reach 10+)
  { login: "natalia_volkov", email: "natalia@example.com", first_name: "Наталья", last_name: "Волкова", phone: "+79002223344", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "passenger", created_at: new Date("2024-01-22T10:00:00Z"), updated_at: new Date("2024-01-22T10:00:00Z") },
  { login: "andrey_morozov", email: "andrey@example.com", first_name: "Андрей", last_name: "Морозов", phone: "+79003334455", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "passenger", created_at: new Date("2024-01-22T11:30:00Z"), updated_at: new Date("2024-01-22T11:30:00Z") },
  { login: "elena_fedorova", email: "elena@example.com", first_name: "Елена", last_name: "Фёдорова", phone: "+79004455667", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "passenger", created_at: new Date("2024-01-23T09:00:00Z"), updated_at: new Date("2024-01-23T09:00:00Z") },
  
  // Additional drivers (to reach 10+)
  { login: "alex_driver", email: "alex@driver.com", first_name: "Александр", last_name: "Петров", phone: "+79005556789", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "driver", created_at: new Date("2024-01-20T08:00:00Z"), updated_at: new Date("2024-01-20T08:00:00Z") },
  { login: "vladimir_driver", email: "vladimir@driver.com", first_name: "Владимир", last_name: "Иванов", phone: "+79006667890", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "driver", created_at: new Date("2024-01-21T07:00:00Z"), updated_at: new Date("2024-01-21T07:00:00Z") },
  { login: "nikolay_driver", email: "nikolay@driver.com", first_name: "Николай", last_name: "Степанов", phone: "+79007778901", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "driver", created_at: new Date("2024-01-22T06:30:00Z"), updated_at: new Date("2024-01-22T06:30:00Z") },
  { login: "ivan_driver", email: "ivan@driver.com", first_name: "Иван", last_name: "Смирнов", phone: "+79008889012", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "driver", created_at: new Date("2024-01-23T08:00:00Z"), updated_at: new Date("2024-01-23T08:00:00Z") },
  { login: "evgeny_driver", email: "evgeny@driver.com", first_name: "Евгений", last_name: "Козлов", phone: "+79009990123", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "driver", created_at: new Date("2024-01-24T09:00:00Z"), updated_at: new Date("2024-01-24T09:00:00Z") },
  { login: "maxim_driver", email: "maxim@driver.com", first_name: "Максим", last_name: "Лебедев", phone: "+79010011223", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "driver", created_at: new Date("2024-01-25T07:30:00Z"), updated_at: new Date("2024-01-25T07:30:00Z") },
  { login: "anton_driver", email: "anton@driver.com", first_name: "Антон", last_name: "Макаров", phone: "+79021122334", password_hash: "0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879", role: "driver", created_at: new Date("2024-01-26T08:30:00Z"), updated_at: new Date("2024-01-26T08:30:00Z") }
]);

// Get inserted user ObjectIds
const user_ids = Object.keys(userResult.insertedIds).map(k => userResult.insertedIds[k]);

// Insert drivers (driver profiles linked to users) - 10+ drivers
const driverResult = db.drivers.insertMany([
  {
    user_id: user_ids[3],  // mike_driver
    license_number: "АА123456",
    car_model: "Toyota Camry",
    car_plate: "А777АА77",
    is_available: true,
    rating: 4.8,
    created_at: new Date("2024-01-17T09:00:00Z"),
    updated_at: new Date("2024-01-17T09:00:00Z")
  },
  {
    user_id: user_ids[4],  // sergey_driver
    license_number: "АВ654321",
    car_model: "Kia Optima",
    car_plate: "А888АА88",
    is_available: true,
    rating: 4.5,
    created_at: new Date("2024-01-18T08:00:00Z"),
    updated_at: new Date("2024-01-18T08:00:00Z")
  },
  {
    user_id: user_ids[5],  // dmitry_driver
    license_number: "АС987654",
    car_model: "Hyundai Solaris",
    car_plate: "А999АА99",
    is_available: false,
    rating: 4.9,
    created_at: new Date("2024-01-19T07:30:00Z"),
    updated_at: new Date("2024-01-20T10:00:00Z")
  },
  {
    user_id: user_ids[10],  // alex_driver
    license_number: "АЕ111111",
    car_model: "Volkswagen Polo",
    car_plate: "А111АА11",
    is_available: true,
    rating: 4.7,
    created_at: new Date("2024-01-20T08:00:00Z"),
    updated_at: new Date("2024-01-20T08:00:00Z")
  },
  {
    user_id: user_ids[11],  // vladimir_driver
    license_number: "АК222222",
    car_model: "Skoda Rapid",
    car_plate: "А222АА22",
    is_available: true,
    rating: 4.6,
    created_at: new Date("2024-01-21T07:00:00Z"),
    updated_at: new Date("2024-01-21T07:00:00Z")
  },
  {
    user_id: user_ids[12],  // nikolay_driver
    license_number: "АМ333333",
    car_model: "Renault Logan",
    car_plate: "А333АА33",
    is_available: false,
    rating: 4.4,
    created_at: new Date("2024-01-22T06:30:00Z"),
    updated_at: new Date("2024-01-22T06:30:00Z")
  },
  {
    user_id: user_ids[13],  // ivan_driver
    license_number: "АО444444",
    car_model: "Lada Vesta",
    car_plate: "А444АА44",
    is_available: true,
    rating: 4.3,
    created_at: new Date("2024-01-23T08:00:00Z"),
    updated_at: new Date("2024-01-23T08:00:00Z")
  },
  {
    user_id: user_ids[14],  // evgeny_driver
    license_number: "АП555555",
    car_model: "Nissan Almera",
    car_plate: "А555АА55",
    is_available: true,
    rating: 4.8,
    created_at: new Date("2024-01-24T09:00:00Z"),
    updated_at: new Date("2024-01-24T09:00:00Z")
  },
  {
    user_id: user_ids[15],  // maxim_driver
    license_number: "АР666666",
    car_model: "Mazda 3",
    car_plate: "А666АА66",
    is_available: false,
    rating: 4.7,
    created_at: new Date("2024-01-25T07:30:00Z"),
    updated_at: new Date("2024-01-25T07:30:00Z")
  },
  {
    user_id: user_ids[16],  // anton_driver
    license_number: "АС777777",
    car_model: "Honda Civic",
    car_plate: "А777АА77",
    is_available: true,
    rating: 4.6,
    created_at: new Date("2024-01-26T08:30:00Z"),
    updated_at: new Date("2024-01-26T08:30:00Z")
  }
]);

// Get inserted driver ObjectIds
const driver_ids = Object.keys(driverResult.insertedIds).map(k => driverResult.insertedIds[k]);

// Insert orders
const orderResult = db.orders.insertMany([
  // Active order (no driver assigned)
  {
    user_id: user_ids[0],  // ivan_petrov
    driver_id: null,
    passenger: {
      name: "Иван Петров",
      phone: "+79001234567"
    },
    pickup_address: "ул. Ленина, 1",
    destination_address: "ул. Пушкина, 10",
    status: "active",
    price: null,
    created_at: new Date("2024-01-20T14:00:00Z"),
    completed_at: null
  },
  
  // Accepted order (driver assigned, in progress)
  {
    user_id: user_ids[1],  // anna_smirnova
    driver_id: driver_ids[0],  // mike_driver
    passenger: {
      name: "Анна Смирнова",
      phone: "+79007654321"
    },
    pickup_address: "пр. Мира, 5",
    destination_address: "ул. Гагарина, 15",
    status: "accepted",
    price: null,
    created_at: new Date("2024-01-20T15:00:00Z"),
    completed_at: null
  },
  
  // Completed order
  {
    user_id: user_ids[0],  // ivan_petrov
    driver_id: driver_ids[1],  // sergey_driver
    passenger: {
      name: "Иван Петров",
      phone: "+79001234567"
    },
    pickup_address: "ул. Советская, 20",
    destination_address: "аэропорт Шереметьево",
    status: "completed",
    price: 850.0,
    created_at: new Date("2024-01-19T10:00:00Z"),
    completed_at: new Date("2024-01-19T10:45:00Z")
  },
  
  // Another completed order
  {
    user_id: user_ids[2],  // alex_kuznetsov
    driver_id: driver_ids[0],  // mike_driver
    passenger: {
      name: "Алексей Кузнецов",
      phone: "+79005557777"
    },
    pickup_address: "ул. Тверская, 1",
    destination_address: "ул. Арбат, 15",
    status: "completed",
    price: 320.0,
    created_at: new Date("2024-01-18T12:00:00Z"),
    completed_at: new Date("2024-01-18T12:25:00Z")
  },
  
  // Cancelled order
  {
    user_id: user_ids[6],  // olga_ivanova
    driver_id: null,
    passenger: {
      name: "Ольга Иванова",
      phone: "+79008889999"
    },
    pickup_address: "ул. Таганская, 20",
    destination_address: "ул. Плющиха, 50",
    status: "cancelled",
    price: null,
    created_at: new Date("2024-01-19T16:00:00Z"),
    completed_at: null
  },
  
  // Another active order
  {
    user_id: user_ids[7],  // petr_sokolov
    driver_id: null,
    passenger: {
      name: "Петр Соколов",
      phone: "+79001112233"
    },
    pickup_address: "ВДНХ, главный вход",
    destination_address: "ул. Яблочкова, 12",
    status: "active",
    price: null,
    created_at: new Date("2024-01-20T16:30:00Z"),
    completed_at: null
  },
  
  // Accepted order with sergey_driver
  {
    user_id: user_ids[6],  // olga_ivanova
    driver_id: driver_ids[1],  // sergey_driver
    passenger: {
      name: "Ольга Иванова",
      phone: "+79008889999"
    },
    pickup_address: "ул. Профсоюзная, 30",
    destination_address: "ул. Нахимовский проспект, 10",
    status: "accepted",
    price: null,
    created_at: new Date("2024-01-20T17:00:00Z"),
    completed_at: null
  },
  
  // Completed order with alex_driver
  {
    user_id: user_ids[8],  // natalia_volkov
    driver_id: driver_ids[3],  // alex_driver
    passenger: {
      name: "Наталья Волкова",
      phone: "+79002223344"
    },
    pickup_address: "ул. Вавилова, 5",
    destination_address: "Московский вокзал",
    status: "completed",
    price: 450.0,
    created_at: new Date("2024-01-21T10:00:00Z"),
    completed_at: new Date("2024-01-21T10:35:00Z")
  },
  
  // Completed order with vladimir_driver
  {
    user_id: user_ids[9],  // andrey_morozov
    driver_id: driver_ids[4],  // vladimir_driver
    passenger: {
      name: "Андрей Морозов",
      phone: "+79003334455"
    },
    pickup_address: "пл. Революции, 1",
    destination_address: "ул. Цветной бульвар, 15",
    status: "completed",
    price: 280.0,
    created_at: new Date("2024-01-22T14:00:00Z"),
    completed_at: new Date("2024-01-22T14:25:00Z")
  },
  
  // Cancelled order
  {
    user_id: user_ids[7],  // petr_sokolov
    driver_id: null,
    passenger: {
      name: "Петр Соколов",
      phone: "+79001112233"
    },
    pickup_address: "ул. Щёлковское шоссе, 100",
    destination_address: "ул. Амурская, 5",
    status: "cancelled",
    price: null,
    created_at: new Date("2024-01-23T11:00:00Z"),
    completed_at: null
  },
  
  // Active order
  {
    user_id: user_ids[8],  // natalia_volkov
    driver_id: null,
    passenger: {
      name: "Наталья Волкова",
      phone: "+79002223344"
    },
    pickup_address: "пр. Ленинский, 30",
    destination_address: "ул. Дмитрия Ульянова, 7",
    status: "active",
    price: null,
    created_at: new Date("2024-01-24T12:00:00Z"),
    completed_at: null
  },
  
  // Completed order with nikolay_driver
  {
    user_id: user_ids[0],  // ivan_petrov
    driver_id: driver_ids[5],  // nikolay_driver
    passenger: {
      name: "Иван Петров",
      phone: "+79001234567"
    },
    pickup_address: "ул. Косыгина, 1",
    destination_address: "Воробьёвы горы",
    status: "completed",
    price: 520.0,
    created_at: new Date("2024-01-25T09:00:00Z"),
    completed_at: new Date("2024-01-25T09:40:00Z")
  },
  
  // Accepted order with ivan_driver
  {
    user_id: user_ids[1],  // anna_smirnova
    driver_id: driver_ids[6],  // ivan_driver
    passenger: {
      name: "Анна Смирнова",
      phone: "+79007654321"
    },
    pickup_address: "ул. Покрышкина, 8",
    destination_address: "ул. Маршала Жукова, 25",
    status: "accepted",
    price: null,
    created_at: new Date("2024-01-26T10:00:00Z"),
    completed_at: null
  },
  
  // Completed order with evgeny_driver
  {
    user_id: user_ids[2],  // alex_kuznetsov
    driver_id: driver_ids[7],  // evgeny_driver
    passenger: {
      name: "Алексей Кузнецов",
      phone: "+79005557777"
    },
    pickup_address: "ул. Обручева, 15",
    destination_address: "Казанский вокзал",
    status: "completed",
    price: 680.0,
    created_at: new Date("2024-01-27T08:00:00Z"),
    completed_at: new Date("2024-01-27T08:50:00Z")
  }
]);

// Print summary
print("=== Taxi Service Test Data Inserted ===");
print("Users: " + db.users.countDocuments());
print("  - Passengers: " + db.users.countDocuments({ role: "passenger" }));
print("  - Drivers: " + db.users.countDocuments({ role: "driver" }));
print("Drivers: " + db.drivers.countDocuments());
print("  - Available: " + db.drivers.countDocuments({ is_available: true }));
print("  - Busy: " + db.drivers.countDocuments({ is_available: false }));
print("Orders: " + db.orders.countDocuments());
print("  - Active: " + db.orders.countDocuments({ status: "active" }));
print("  - Accepted: " + db.orders.countDocuments({ status: "accepted" }));
print("  - Completed: " + db.orders.countDocuments({ status: "completed" }));
print("  - Cancelled: " + db.orders.countDocuments({ status: "cancelled" }));
print("======================================");
