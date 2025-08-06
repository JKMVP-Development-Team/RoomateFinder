#include "ParseUser.cpp"
#include "DBManager.h"
#include <mongocxx/instance.hpp>

DBManager::DBManager(const std::string& mongo_uri)
    : client_(mongocxx::uri{mongo_uri}) {
    db = client_["roommatefinder"];
}

mongocxx::collection DBManager::getUserCollection() { return db["users"]; }
mongocxx::collection DBManager::getRoomCollection() { return db["rooms"]; }
mongocxx::collection DBManager::getUserSwipeCollection() { return db["user_swipes"]; }
mongocxx::collection DBManager::getRoomSwipeCollection() { return db["room_swipes"]; }

DBManager& getDbManager() {
    static mongocxx::instance inst{};
    static DBManager dbManager(getenv("MONGODB_URI") ? getenv("MONGODB_URI") : "mongodb://localhost:27017");
    return dbManager;
}

crow::json::wvalue fetchUserInfo(const std::string& userId) {
    auto& dbManager = getDbManager();
    auto user_collection = dbManager.getUserCollection();

    ParseUser parser(user_collection);
    return parser.parse(userId);
}