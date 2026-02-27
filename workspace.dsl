workspace {
    name "UBER"
    description "Система заказа такси"

    model {
        properties { 
            structurizr.groupSeparator "/"
            workspace_cmdb "cmdb_mnemonic"
            architect "имя архитектора"
        }

        my_user    = person "Пользователь"
        my_admin   = person "Администратор системы"
        my_support = person "Специалист ТП"
        my_driver = person "Водитель"
        

        my_system = softwareSystem "UBER"{
            description "Система заказа такси"
        }
        
        ebank = softwareSystem "Е БАНК"{
            description "Система платежей"
        }

        emap = softwareSystem "Карта города"{
            description "Система навигации"
        }

        my_user -> my_system description "Заказывает такси"
        my_driver -> my_system description "Водитель ищет заказ"
        my_admin -> my_system description "Администрирование водителей"

        my_system ->
        
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