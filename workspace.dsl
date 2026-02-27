workspace {
    name "UBER"
    description "Система заказа такси"

    model {
        properties { 
            structurizr.groupSeparator "/"
            workspace_cmdb "cmdb_mnemonic"
            architect "имя архитектора"
        }

        my_user = person "Пользователь"
        my_admin = person "Администратор системы"
        my_support = person "Специалист ТП"
        my_driver = person "Водитель"
        

        my_system = softwareSystem "UBER"{
            description "Система заказа такси"
        }
        
        ebank = softwareSystem "Е БАНК"{
            description "Система платежей"
        }

        emap = softwareSystem "Карты"{
            description "Система навигации"
        }
        epolice = softwareSystem "ГИБДД" {
            description "Система контроля документов водителей"
        }

        my_user -> my_system description "Заказывает такси"
        my_driver -> my_system description "Поиск заказа"
        my_admin -> my_system description "Администрирование водителей"
        my_support -> my_system description "Решение инцедентов, помощь в работе с системой"


        my_system -> ebank description "Прием оплаты от клиентов, вывод средств на карты водителям"
        my_system -> emap description "Построение маршрутов, рассчет расстояния и времени в пути"
        my_system -> epolice description "Проверка акутальности документов водителей"
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