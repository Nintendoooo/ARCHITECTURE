workspace {
    name "UBER"
    description "Система заказа такси"

    model {
        properties { 
            structurizr.groupSeparator "/"
            workspace_cmdb "cmdb_mnemonic"
            architect "имя архитектора"
        }

        my_user = person "Пользователь" description "Клиент, заказывающий такси"
        my_admin = person "Администратор системы" description "Управляет системой"
        my_support = person "Специалист ТП" description "Помогает пользователям"
        my_driver = person "Водитель" description "Исполнитель заказов"
        

        my_system = softwareSystem "UBER"{ 
            description "Система заказа такси"
        }
        
        ebank = softwareSystem "Е БАНК"{
            description "Платежная система"
        }

        map = softwareSystem "Карты"{
            description "Система навигации"
        }

        checkdoc = softwareSystem "Верификация" {
            description "Система проверки паспортов и ВУ водителей"
        }

        my_user -> my_system description "Создание заказа, получение истории"
        my_driver -> my_system description "Прием заказов, завершение поездки"
        my_admin -> my_system description "Администрирование водителей"
        my_support -> my_system description "Решение инцидентов, помощь в работе с системой"


        my_system -> ebank description "Запрос на списание средств с клиента, Запрос на выплату водителю"
        my_system -> map description "Запрос маршрута, запрос рассчетного времени прибытия"
        my_system -> checkdoc description "Отправка документов водителей для проверки"
        
        ebank -> my_system description "Подтверждение списание средств с клиента. Подтверждение выплаты водителю"
        map -> my_system description "Маршрут, расстояние и время"
        checkdoc -> my_system description "Результат проверки документов водителя"
        
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


        # Диаграмма контекста
        systemContext my_system {
            include *
            autoLayout
        }

   
    }
}