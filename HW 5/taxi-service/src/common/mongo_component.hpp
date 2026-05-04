#pragma once

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#include <cstdlib>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "database.hpp"

namespace taxi {

class MongoComponent : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "mongo-db";

    MongoComponent(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context)
        : ComponentBase(config, context) {
        
        // Get MongoDB URI from config or environment
        const char* env_uri = std::getenv("MONGO_URI");
        std::string mongo_uri;
        if (env_uri != nullptr && strlen(env_uri) > 0) {
            mongo_uri = env_uri;
        } else {
            // Default fallback - will be overridden by config
            mongo_uri = "mongodb://taxi_user:taxi_pass@mongodb:27017/taxi_db";
        }
        
        // Get database name from config if available
        std::string database_name = "taxi_db";
        if (config.HasMember("database")) {
            database_name = config["database"].As<std::string>();
        }
        
        db_ = std::make_shared<taxi_service::Database>(mongo_uri, database_name);
    }

    std::shared_ptr<taxi_service::Database> GetDatabase() const {
        return db_;
    }

private:
    std::shared_ptr<taxi_service::Database> db_;
};

}  // namespace taxi
