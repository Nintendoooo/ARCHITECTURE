-- ============================================================================
-- Taxi Service Test Data
-- Variant 16: Система заказа такси (Taxi Service)
-- ============================================================================

-- ============================================================================
-- Insert test users (12+ users — passengers and future drivers)
-- Password for all users: "securePass123" (hashed with SHA-256)
-- Hash: 0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879
-- ============================================================================

INSERT INTO users (login, email, first_name, last_name, phone, password_hash) VALUES
('alice.smith',    'alice.smith@example.com',    'Alice',   'Smith',    '+79001234501', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879'),
('bob.jones',      'bob.jones@example.com',      'Bob',     'Jones',    '+79001234502', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879'),
('charlie.brown',  'charlie.brown@example.com',  'Charlie', 'Brown',    '+79001234503', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879'),
('diana.prince',   'diana.prince@example.com',   'Diana',   'Prince',   '+79001234504', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879'),
('edward.norton',  'edward.norton@example.com',   'Edward',  'Norton',   '+79001234505', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879'),
('fiona.apple',    'fiona.apple@example.com',    'Fiona',   'Apple',    '+79001234506', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879'),
('george.martin',  'george.martin@example.com',  'George',  'Martin',   '+79001234507', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879'),
('helen.mirren',   'helen.mirren@example.com',   'Helen',   'Mirren',   '+79001234508', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879'),
('ivan.petrov',    'ivan.petrov@example.com',    'Ivan',    'Petrov',   '+79001234509', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879'),
('julia.roberts',  'julia.roberts@example.com',  'Julia',   'Roberts',  '+79001234510', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879'),
('kevin.spacey',   'kevin.spacey@example.com',   'Kevin',   'Spacey',   '+79001234511', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879'),
('laura.palmer',   'laura.palmer@example.com',   'Laura',   'Palmer',   '+79001234512', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879'),
('mike.tyson',     'mike.tyson@example.com',     'Mike',    'Tyson',    '+79001234513', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879'),
('nina.simone',    'nina.simone@example.com',    'Nina',    'Simone',   '+79001234514', '0ecf0285c52337d198723ffcb06c19313d1790be40acb6c6887e1810e5ada879')
ON CONFLICT (login) DO NOTHING;

-- ============================================================================
-- Insert test drivers (10+ drivers linked to user accounts)
-- Users 3-12 are registered as drivers
-- ============================================================================

INSERT INTO drivers (user_id, license_number, car_model, car_plate, is_available) VALUES
((SELECT id FROM users WHERE login = 'charlie.brown'),  'DL-77-001234', 'Toyota Camry',       'A001AA77',  true),
((SELECT id FROM users WHERE login = 'diana.prince'),   'DL-77-005678', 'Hyundai Solaris',    'B002BB77',  true),
((SELECT id FROM users WHERE login = 'edward.norton'),  'DL-50-009012', 'Kia Rio',            'C003CC50',  true),
((SELECT id FROM users WHERE login = 'fiona.apple'),    'DL-77-003456', 'Volkswagen Polo',    'D004DD77',  false),
((SELECT id FROM users WHERE login = 'george.martin'),  'DL-99-007890', 'Skoda Octavia',      'E005EE99',  true),
((SELECT id FROM users WHERE login = 'helen.mirren'),   'DL-77-002345', 'Mercedes E-Class',   'F006FF77',  true),
((SELECT id FROM users WHERE login = 'ivan.petrov'),    'DL-50-006789', 'BMW 3 Series',       'G007GG50',  false),
((SELECT id FROM users WHERE login = 'julia.roberts'),  'DL-77-001122', 'Audi A4',            'H008HH77',  true),
((SELECT id FROM users WHERE login = 'kevin.spacey'),   'DL-99-003344', 'Renault Logan',      'I009II99',  true),
((SELECT id FROM users WHERE login = 'laura.palmer'),   'DL-77-005566', 'Nissan Almera',      'J010JJ77',  true)
ON CONFLICT (user_id) DO NOTHING;

-- ============================================================================
-- Insert test orders (15+ orders in various statuses)
-- Passengers: alice.smith (user 1), bob.jones (user 2), mike.tyson (user 13), nina.simone (user 14)
-- Also some drivers ordering rides as passengers
-- ============================================================================

INSERT INTO orders (user_id, driver_id, pickup_address, destination_address, status, price, created_at, completed_at) VALUES
-- Active orders (waiting for a driver)
((SELECT id FROM users WHERE login = 'alice.smith'),
 NULL,
 'Москва, ул. Тверская, д. 1',
 'Москва, Красная площадь, д. 1',
 'active', NULL,
 '2024-03-01 08:30:00', NULL),

((SELECT id FROM users WHERE login = 'bob.jones'),
 NULL,
 'Москва, Ленинградский проспект, д. 10',
 'Москва, Шереметьево, Терминал D',
 'active', NULL,
 '2024-03-01 09:00:00', NULL),

((SELECT id FROM users WHERE login = 'mike.tyson'),
 NULL,
 'Москва, ул. Арбат, д. 25',
 'Москва, Парк Горького',
 'active', NULL,
 '2024-03-01 09:15:00', NULL),

((SELECT id FROM users WHERE login = 'nina.simone'),
 NULL,
 'Москва, Кутузовский проспект, д. 5',
 'Москва, ВДНХ, Главный вход',
 'active', NULL,
 '2024-03-01 09:30:00', NULL),

-- Accepted orders (driver assigned, ride in progress)
((SELECT id FROM users WHERE login = 'alice.smith'),
 (SELECT id FROM drivers WHERE user_id = (SELECT id FROM users WHERE login = 'charlie.brown')),
 'Москва, Пушкинская площадь',
 'Москва, Домодедово, Терминал 1',
 'accepted', NULL,
 '2024-03-01 07:00:00', NULL),

((SELECT id FROM users WHERE login = 'bob.jones'),
 (SELECT id FROM drivers WHERE user_id = (SELECT id FROM users WHERE login = 'diana.prince')),
 'Москва, ул. Новый Арбат, д. 15',
 'Москва, Москва-Сити, Башня Федерация',
 'accepted', NULL,
 '2024-03-01 07:30:00', NULL),

((SELECT id FROM users WHERE login = 'mike.tyson'),
 (SELECT id FROM drivers WHERE user_id = (SELECT id FROM users WHERE login = 'edward.norton')),
 'Москва, Лубянская площадь',
 'Москва, Воробьёвы горы',
 'accepted', NULL,
 '2024-03-01 08:00:00', NULL),

-- Completed orders (ride finished)
((SELECT id FROM users WHERE login = 'alice.smith'),
 (SELECT id FROM drivers WHERE user_id = (SELECT id FROM users WHERE login = 'george.martin')),
 'Москва, Казанский вокзал',
 'Москва, ул. Мясницкая, д. 20',
 'completed', 450.00,
 '2024-02-28 10:00:00', '2024-02-28 10:35:00'),

((SELECT id FROM users WHERE login = 'alice.smith'),
 (SELECT id FROM drivers WHERE user_id = (SELECT id FROM users WHERE login = 'helen.mirren')),
 'Москва, Третьяковская галерея',
 'Москва, Большой театр',
 'completed', 320.50,
 '2024-02-27 14:00:00', '2024-02-27 14:25:00'),

((SELECT id FROM users WHERE login = 'bob.jones'),
 (SELECT id FROM drivers WHERE user_id = (SELECT id FROM users WHERE login = 'julia.roberts')),
 'Москва, Парк Зарядье',
 'Москва, Останкинская башня',
 'completed', 780.00,
 '2024-02-26 16:00:00', '2024-02-26 17:10:00'),

((SELECT id FROM users WHERE login = 'nina.simone'),
 (SELECT id FROM drivers WHERE user_id = (SELECT id FROM users WHERE login = 'kevin.spacey')),
 'Москва, ГУМ',
 'Москва, Измайловский парк',
 'completed', 550.00,
 '2024-02-25 11:00:00', '2024-02-25 11:45:00'),

((SELECT id FROM users WHERE login = 'mike.tyson'),
 (SELECT id FROM drivers WHERE user_id = (SELECT id FROM users WHERE login = 'laura.palmer')),
 'Москва, Патриаршие пруды',
 'Москва, Сокольники',
 'completed', 410.00,
 '2024-02-24 09:00:00', '2024-02-24 09:40:00'),

((SELECT id FROM users WHERE login = 'nina.simone'),
 (SELECT id FROM drivers WHERE user_id = (SELECT id FROM users WHERE login = 'charlie.brown')),
 'Москва, Цветной бульвар',
 'Москва, Лужники',
 'completed', 620.00,
 '2024-02-23 18:00:00', '2024-02-23 18:50:00'),

-- Cancelled orders
((SELECT id FROM users WHERE login = 'bob.jones'),
 NULL,
 'Москва, ул. Бауманская, д. 5',
 'Москва, Внуково, Терминал A',
 'cancelled', NULL,
 '2024-02-28 08:00:00', NULL),

((SELECT id FROM users WHERE login = 'alice.smith'),
 NULL,
 'Москва, Чистые пруды',
 'Москва, Коломенское',
 'cancelled', NULL,
 '2024-02-27 12:00:00', NULL),

((SELECT id FROM users WHERE login = 'mike.tyson'),
 NULL,
 'Москва, Китай-город',
 'Москва, Битцевский парк',
 'cancelled', NULL,
 '2024-02-26 20:00:00', NULL)
ON CONFLICT DO NOTHING;

-- ============================================================================
-- Verify data insertion
-- ============================================================================

DO $$
DECLARE
    user_count INTEGER;
    driver_count INTEGER;
    order_count INTEGER;
BEGIN
    SELECT COUNT(*) INTO user_count FROM users;
    SELECT COUNT(*) INTO driver_count FROM drivers;
    SELECT COUNT(*) INTO order_count FROM orders;

    RAISE NOTICE 'Data insertion complete:';
    RAISE NOTICE '  Users: %', user_count;
    RAISE NOTICE '  Drivers: %', driver_count;
    RAISE NOTICE '  Orders: %', order_count;
END $$;

-- ============================================================================
-- END OF TEST DATA
-- ============================================================================
