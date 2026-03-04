workspace {
    name "UBER"
    description "Система заказа такси"

    model {
        properties { 
            structurizr.groupSeparator "/"
            workspace_cmdb "cmdb_mnemonic"
            architect "имя архитектора"
        }

        # ПОЛЬЗОВАТЕЛИ
        my_user = person "Пользователь" "Клиент, заказывающий такси"
        my_admin = person "Администратор системы" "Управляет системой"
        my_support = person "Специалист ТП" "Помогает пользователям"
        my_driver = person "Водитель" "Исполнитель заказов"
        
        # СИСТЕМА
        my_system = softwareSystem "UBER"{ 
            description "Система заказа такси"
        
            web = container "Веб-приложение"

            app = container "Мобильное приложение"

            api = container "API"{
                description "Проверка прав, маршрутизация"
            }

            input = container "Сервис регистрации и входа"            
            db_user = container "База данных пользователей" {
                tags "Database"
            }
            db_driver = container "База данных водителей" {
                tags "Database"
            }


            order = container "Сервис заказов"
            db_order = container "База данных заказов" {
                tags "Database"
            }

            pay = container "Сервис оплаты"
            db_pay = container "База данных платежей" {
                tags "Database"
            }

            match = container "Сервис стыковки"
            mess = container "Сервис уведомлений"
        }
        
        # ВНЕШНИЕ СИСТЕМЫ
        ebank = softwareSystem "Е БАНК"{
            description "Платежная система"
        }

        map = softwareSystem "Карты"{
            description "Система навигации"
        }

        checkdoc = softwareSystem "Верификация" {
            description "Система проверки паспортов и ВУ водителей"
        }

        # СВЯЗИ УРОВНЯ С1
        my_user -> my_system "Создание заказа, получение истории"
        my_driver -> my_system "Прием заказов, завершение поездки"
        my_admin -> my_system "Администрирование водителей"
        my_support -> my_system "Решение инцидентов, помощь в работе с системой"

        my_system -> ebank "Запрос на списание средств с клиента, Запрос на выплату водителю"
        my_system -> map "Запрос маршрута, запрос рассчетного времени прибытия"
        my_system -> checkdoc "Отправка документов водителей для проверки"
        
        ebank -> my_system "Подтверждение списание средств с клиента. Подтверждение выплаты водителю"
        map -> my_system "Маршрут, расстояние и время"
        checkdoc -> my_system "Результат проверки документов водителя"
        
        # СВЯЗИ УРОВНЯ С2
        web -> api "Запрос в систему"
        app -> api "Запрос в систему"
        api -> web "Ответ"
        api -> app "Ответ"

        api -> input "Запросы на регистрацию/вход"
        api -> order "Запросы на управление заказами"
        api -> match "Запросы на получение активных заказов для водителя"
        api -> pay "Запросы на оплату"

        api -> input "Запросы админа на поиск пользователя"
        input -> api "Ответы на запросы админа"
        
        # С2. Cвязи с БД
        db_user -> input "Данные о пользователях"
        db_driver -> input "Данные о водителях"
   
        input -> db_user "Запись, редактирование, удаление и запросы данных о пользователях"
        input -> db_driver "Запись, редактирование, удаление и запросы данных о водителях"
        
        db_order -> order "Данные об активных, выполняемых и завершенных заказах"
        order -> db_order "Запись, редактирование, удаление и запросы данных о заказах"

        db_pay -> pay "Данные о транзакциях"
        pay -> db_pay "Запись, редактирование, удаление и запросы данных о транзакциях"

        # С2. Связи между контейнерами
        order -> input "Проверка существования пользователя/водителя"
        order -> match "Активный заказ"
        match -> order "Заказ принят/отклонен"
        order -> pay "Инициирование оплаты после завершения"
        match -> mess "Уведомление о найденном/принятом заказе"
        order -> mess "Уведомление о смене статуса заказа"

        # С2. Работа с внешними системами
        pay -> ebank "Запрос на списание/выплату"
        order -> map "Запрос маршрута и расчетного времени прибытия"
        input -> checkdoc "Отправка документов водителей для проверки"
        checkdoc -> input "Результаты проверки документов водителя"

        }

    views {
        # Конфигурируем настройки отображения plant uml
        properties {
            plantuml.format     "svg"
            kroki.format        "svg"
            structurizr.sort created
            structurizr.tooltips true
        }

        # Задаем стили для отображения
        themes default

        styles {
            element "Database" {
                shape cylinder
            }
        }

        # Диаграмма контекста
        systemContext my_system {
            include *
            autoLayout lr
        }

        container my_system {
            include *
            autoLayout
        }

   
    }
}