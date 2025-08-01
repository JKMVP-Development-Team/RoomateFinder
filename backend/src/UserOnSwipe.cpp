#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <crow/crow_all.h>
#include <cstdlib>
#include <iostream>
#include <chrono>

double calculatePopularity(int swipeReceived, int swipeGiven) {
    return static_cast<double>(swipeReceived) / std::max(swipeGiven, 1);
}

void updateUserPopularity(mongocxx::collection& collection, const std::string& userId, int swipesReceivedIncrement = 0) {

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_document;
    using bsoncxx::builder::stream::close_document;


    auto maybe_user = collection.find_one(document{} << "userId" << userId << finalize);

    if (!maybe_user) {
        std::cerr << "User not found: " << userId << std::endl;
        return;
    }  

    auto user_doc = maybe_user->view();

    int currentSwipesMade = user_doc["swipesMade"] ? user_doc["swipesMade"].get_int32().value : 0;
    int currentSwipesReceived = user_doc["swipesReceived"] ? user_doc["swipesReceived"].get_int32().value : 0;

    int newSwipesReceived = currentSwipesReceived + swipesReceivedIncrement;
    double newPopularity = calculatePopularity(newSwipesReceived, currentSwipesMade);

    auto update_doc = document{}
        << "$set" << open_document
        << "swipesReceived" << newSwipesReceived
        << "popularity" << newPopularity
        << close_document
        << finalize;

    collection.update_one(document{} << "userId" << userId << finalize, std::move(update_doc));
}

void handleUserSwipe(mongocxx::database& db, 
                    const std::string& sourceUserId, 
                    const std::string& targetUserId, 
                    bool isLike) {
    
    auto users_collection = db["users"];
    auto swipes_collection = db["swipes"];
    
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_document;
    using bsoncxx::builder::stream::close_document;
    
    // 1. Record the swipe in swipes collection
    auto timestamp = std::chrono::system_clock::now();
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        timestamp.time_since_epoch()).count();
    
    auto swipe_doc = document{}
        << "sourceUserId" << sourceUserId
        << "targetUserId" << targetUserId
        << "isLike" << isLike
        << "timestamp" << bsoncxx::types::b_date{std::chrono::milliseconds{timestamp_ms}}
        << finalize;
    
    swipes_collection.insert_one(swipe_doc.view());
    
    // 2. Update source user's swipe count
    auto source_user = users_collection.find_one(document{} << "userId" << sourceUserId << finalize);
    if (!source_user) {
        std::cerr << "Source user not found: " << sourceUserId << std::endl;
        return;
    }
    
    auto source_doc = source_user->view();
    int currentSwipesMade = source_doc["swipesMade"] ? source_doc["swipesMade"].get_int32().value : 0;
    
    auto update_source = document{}
        << "$set" << open_document
        << "swipesMade" << (currentSwipesMade + 1)
        << close_document
        << finalize;
    users_collection.update_one(document{} << "userId" << sourceUserId << finalize, std::move(update_source));
    
    // 3. If it's a like, update target user's popularity
    if (isLike) {
        updateUserPopularity(users_collection, targetUserId, 1);
    }
}


int test()
{ 
  try
  {
    const char* mongo_uri = std::getenv("MONGODB_URI");
    if (mongo_uri != nullptr) {
        std::cout << "MONGODB_URI: " << mongo_uri << std::endl;
    } else {
        std::cout << "MONGODB_URI environment variable is not set." << std::endl;
    }

    mongocxx::instance inst{};
  
    const auto uri = mongocxx::uri{std::getenv("MONGODB_URI")};
  
    mongocxx::options::client client_options;
    const auto api = mongocxx::options::server_api{ mongocxx::options::server_api::version::k_version_1 };
    client_options.server_api_opts(api);
  
    // Setup the connection and get a handle on the "admin" database.
    mongocxx::client conn{ uri, client_options };
    mongocxx::database db = conn["admin"];
    
    // Ping the database.
    const auto ping_cmd = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("ping", 1));
    db.run_command(ping_cmd.view());
    std::cout << "Pinged your deployment. You successfully connected to MongoDB!" << std::endl;
  }
  catch (const std::exception& e) 
  {
    std::cout<< "Exception: " << e.what() << std::endl;
  }

  return 0;
}
