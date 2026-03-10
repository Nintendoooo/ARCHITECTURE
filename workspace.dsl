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
        
            web = container "Веб-приложение"{
                technology "HTML + CSS + JavaScript"
                tags "Web"
            }

            app = container "Мобильное приложение"{
                technology "React Native"
                tags "Mobile"
            }

            api = container "API"{
                description "Проверка прав, маршрутизация"
                technology "C++"
                tags "Gateway"
            }

            input = container "Сервис регистрации и входа"{
                technology "C++"
                tags "Service"
            }            

            db = container "База данных" {
                description "Хранилище данных (БД пользователей, БД водителей, БД заказов, БД платежей)"
                technology "PostgreSQL"
                tags "Database"
            }
            
            order = container "Сервис заказов"{
                technology "C++"
                tags "Service"
            }

            pay = container "Сервис оплаты" {
                technology "C++"
                tags "Service"
            }

            match = container "Сервис стыковки" {
                technology "C++"
                tags "Service"
            }
            mess = container "Сервис уведомлений"{
                technology "C++"
                tags "Service"
            }
        }
        
        # ВНЕШНИЕ СИСТЕМЫ
        ebank = softwareSystem "Е БАНК"{
            description "Платежная система"
            tags "External"
        }

        map = softwareSystem "Карты"{
            description "Система навигации"
            tags "External"
        }

        checkdoc = softwareSystem "Верификация" {
            description "Система проверки паспортов и ВУ водителей"
            tags "External"
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
        my_user -> app "Регистрация, создание заказа, получение истории заказов" "HTTPS/WSS"
        my_driver -> app "Регистрация, принятие заказа, завершение поездки" "HTTPS/WSS"
        my_admin -> web "Блокировка/разблокировка пользователей, подтверждение водителя" "HTTPS"
        my_support -> web "Запросы на детали и историю заказов, инициирование возврата платежа" "HTTPS"

        #app -> my_user "Результат операции"
        #app -> my_driver "Результат операции"
        #web -> my_admin "Результат операции"
        #web -> my_support "Результат операции"
        
        web -> api "Запрос в систему" "HTTPS/REST"
        app -> api "Запрос в систему" "HTTPS/REST"
        #api -> web "Ответ"
        #api -> app "Ответ"

        api -> input "Запросы на регистрацию/вход, поиск пользователя админом" "HTTP/REST"
        api -> order "Запросы на управление заказами" "HTTP/REST"
        api -> match "Запросы на получение активных заказов для водителя" "HTTP/REST"
        api -> pay "Запросы на оплату" "HTTP/REST"

        #api -> input "Запросы админа на поиск пользователя"
        #input -> api "Ответы на запросы админа"
        
        # С2. Cвязи с БД
        #db -> input "Данные о водителях и пользователях" "PostgreSQL"
        input -> db "Запись, редактирование, удаление и запросы данных о водителях и пользователях" "PostgreSQL"
        #db -> order "Данные об активных, выполняемых и завершенных заказах" "PostgreSQL"
        order -> db "Запись, редактирование, удаление и запросы данных о заказах" "PostgreSQL"
        #db -> pay "Данные о транзакциях" "PostgreSQL"
        pay -> db "Запись, редактирование, удаление и запросы данных о транзакциях" "PostgreSQL"

        # С2. Связи между контейнерами
        order -> input "Проверка существования пользователя/водителя" "HTTP/REST"
        order -> match "Активный заказ" "HTTP/REST"
        #match -> order "Заказ принят/отклонен" "HTTP/REST"
        order -> pay "Инициирование оплаты после завершения" "HTTP/REST"
        match -> mess "Уведомление о найденном/принятом заказе" "HTTP/REST"
        order -> mess "Уведомление о смене статуса заказа" "HTTP/REST"

        # С2. Работа с внешними системами
        pay -> ebank "Запрос на списание/выплату" "HTTPS/REST"
        order -> map "Запрос маршрута и расчетного времени прибытия" "HTTPS/REST"
        input -> checkdoc "Отправка документов водителей для проверки" "HTTPS/REST"
        #checkdoc -> input "Результаты проверки документов водителя" "HTTPS/REST"

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

            element "External" {
                background #999999
                color #ffffff 
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