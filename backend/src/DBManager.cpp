#include "DBManager.h"

DBManager::DBManager(const std::string& mongo_uri)
    : client_(mongocxx::uri{mongo_uri}) {
    user_db_ = client_["roommatefinder"];
    room_db_ = client_["roomfinder"];
}

mongocxx::collection DBManager::getUserCollection() { return user_db_["users"]; }
mongocxx::collection DBManager::getRoomCollection() { return room_db_["rooms"]; }
mongocxx::collection DBManager::getUserSwipeCollection() { return user_db_["user_swipes"]; }
mongocxx::collection DBManager::getRoomSwipeCollection() { return room_db_["room_swipes"]; }

