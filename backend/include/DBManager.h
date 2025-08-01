#pragma once

#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>
#include <string>

class DBManager {
public:
    DBManager(const std::string& mongo_uri);
    mongocxx::collection getUserCollection();
    mongocxx::collection getRoomCollection();
    mongocxx::collection getUserSwipeCollection();
    mongocxx::collection getRoomSwipeCollection();
private:
    mongocxx::client client_;
    mongocxx::database user_db_;
    mongocxx::database room_db_;
};