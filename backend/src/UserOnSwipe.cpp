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

void updateUserPopularity(mongocxx::collection& collection, const bsoncxx::oid& userOid, int swipesReceivedIncrement = 0) {

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_document;
    using bsoncxx::builder::stream::close_document;


    auto maybe_user = collection.find_one(document{} << "_id" << userOid << finalize);

    if (!maybe_user) {
        std::cerr << "User not found: " << userOid.to_string() << std::endl;
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

    collection.update_one(document{} << "_id" << userOid << finalize, std::move(update_doc));
}

void handleUserSwipe(mongocxx::database& db, 
                    const bsoncxx::oid& sourceUserOid,
                    const bsoncxx::oid& targetUserOid,
                    bool isLike) {
    
    auto users_collection = db["users"];
    auto swipes_collection = db["swipes"];
    
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_document;
    using bsoncxx::builder::stream::close_document;
    
    auto timestamp = std::chrono::system_clock::now();
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        timestamp.time_since_epoch()).count();
    
    auto swipe_doc = document{}
        << "sourceUserId" << sourceUserOid.to_string()
        << "targetUserId" << targetUserOid.to_string()
        << "timestamp" << bsoncxx::types::b_date{std::chrono::milliseconds{timestamp_ms}}
        << finalize;
    
    swipes_collection.insert_one(swipe_doc.view());
    
    auto source_user = users_collection.find_one(document{} << "_id" << sourceUserOid << finalize);
    if (!source_user) {
        std::cerr << "Source user not found: " << sourceUserOid.to_string() << std::endl;
        return;
    }
    
    auto source_doc = source_user->view();
    int currentSwipesMade = source_doc["swipesMade"] ? source_doc["swipesMade"].get_int32().value : 0;
    
    auto update_source = document{}
        << "$set" << open_document
        << "swipesMade" << (currentSwipesMade + 1)
        << close_document
        << finalize;
    users_collection.update_one(document{} << "_id" << sourceUserOid << finalize, std::move(update_source));
    
    if (isLike) {
        updateUserPopularity(users_collection, targetUserOid, 1);
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
        return 1;
    }

    mongocxx::instance inst{};
    const auto uri = mongocxx::uri{mongo_uri};
    mongocxx::options::client client_options;
    const auto api = mongocxx::options::server_api{ mongocxx::options::server_api::version::k_version_1 };
    client_options.server_api_opts(api);

    mongocxx::client conn{ uri, client_options };
    mongocxx::database db = conn["admin"];

    // Ping the database.
    const auto ping_cmd = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("ping", 1));
    db.run_command(ping_cmd.view());
    std::cout << "Pinged your deployment. You successfully connected to MongoDB!" << std::endl;

    // --- Testing functions below ---
    auto users_collection = db["users"];
    users_collection.delete_many({});
    auto swipes_collection = db["swipes"];
    swipes_collection.delete_many({}); // Clear existing data for testing

            bsoncxx::builder::stream::document user_doc_builder1, user_doc_builder2;
        user_doc_builder1 << "swipesMade" << 0 << "swipesReceived" << 0 << "popularity" << 0.0;
        user_doc_builder2 << "swipesMade" << 0 << "swipesReceived" << 0 << "popularity" << 0.0;

        auto result1 = users_collection.insert_one(user_doc_builder1 << bsoncxx::builder::stream::finalize);
        auto result2 = users_collection.insert_one(user_doc_builder2 << bsoncxx::builder::stream::finalize);

        bsoncxx::oid sourceUserOid, targetUserOid;
        // Check if the insertions were successful and retrieve the OIDs
        if (result1 && result1->inserted_id().type() == bsoncxx::type::k_oid) {
            sourceUserOid = result1->inserted_id().get_oid().value;
            std::cout << "Inserted source user with _id: " << sourceUserOid.to_string() << std::endl;
        }
        if (result2 && result2->inserted_id().type() == bsoncxx::type::k_oid) {
            targetUserOid = result2->inserted_id().get_oid().value;
            std::cout << "Inserted target user with _id: " << targetUserOid.to_string() << std::endl;
        }

        std::cout << "Inserted test users." << std::endl;

        // Test handleUserSwipe (simulate a like swipe)
        handleUserSwipe(db, sourceUserOid, targetUserOid, true);
        std::cout << "handleUserSwipe executed (like)." << std::endl;

        // Test handleUserSwipe (simulate a dislike swipe)
        handleUserSwipe(db, sourceUserOid, targetUserOid, false);
        std::cout << "handleUserSwipe executed (dislike)." << std::endl;

    }
    catch (const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}