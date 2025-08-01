#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <crow/crow_all.h>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include "DBManager.h"
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;

// --- Logic Functions ---
double calculatePopularity(int received, int made, int matches) {
    return static_cast<double>(received + 2 * matches) / std::max(made, 1);
}

bool swipeExists(mongocxx::collection& swipe_collection, const bsoncxx::oid& sourceEntityOid, const bsoncxx::oid& targetEntityOid) {

    auto existing_swipe = swipe_collection.find_one(
        document{}
            << "sourceEntityId" << sourceEntityOid.to_string()
            << "targetEntityId" << targetEntityOid.to_string()
            << finalize
    );
    return existing_swipe ? true : false;
}


void updateEntityPopularity(mongocxx::collection& entity_collection, const bsoncxx::oid& entityOid) {
    auto maybe_entity = entity_collection.find_one(document{} << "_id" << entityOid << finalize);
    if (!maybe_entity) {
        std::cerr << "Entity not found: " << entityOid.to_string() << std::endl;
        return;
    }

    auto entity_doc = maybe_entity->view();
    int currentMade = entity_doc["swipesMade"] ? entity_doc["swipesMade"].get_int32().value : 0;
    int currentReceived = entity_doc["swipesReceived"] ? entity_doc["swipesReceived"].get_int32().value : 0;
    int currentMatches = entity_doc["matches"] ? entity_doc["matches"].get_int32().value : 0;


    int newReceived = currentReceived + 1;
    double newPopularity = calculatePopularity(newReceived, currentMade, currentMatches);

    auto update_doc = document{}
        << "$set" << open_document
        << "swipesReceived" << newReceived
        << "popularity" << newPopularity
        << close_document
        << finalize;

    entity_collection.update_one(document{} << "_id" << entityOid << finalize, std::move(update_doc));
}

void updateEntityMatches(mongocxx::collection& entity_collection,
                        const bsoncxx::oid& sourceEntityOid,
                        const bsoncxx::oid& targetEntityOid) {
    auto source_entity = entity_collection.find_one(document{} << "_id" << sourceEntityOid << finalize);
    auto target_entity = entity_collection.find_one(document{} << "_id" << targetEntityOid << finalize);

    if (!source_entity || !target_entity) {
        std::cerr << "One of the entities not found." << std::endl;
        return;
    }

    auto update_source = document{}
        << "$inc" << open_document
        << "matches" << 1
        << close_document
        << finalize;

    auto update_target = document{}
        << "$inc" << open_document
        << "matches" << 1
        << close_document
        << finalize;

    entity_collection.update_one(document{} << "_id" << sourceEntityOid << finalize, std::move(update_source));
    entity_collection.update_one(document{} << "_id" << targetEntityOid << finalize, std::move(update_target));
}

void handleEntitySwipe(mongocxx::collection& entity_collection,
                      mongocxx::collection& swipe_collection,
                      const bsoncxx::oid& sourceEntityOid,
                      const bsoncxx::oid& targetEntityOid,
                      bool isLike,
                      const std::string& madeField = "swipesMade",
                      const std::string& receivedField = "swipesReceived",
                      const std::string& matchesField = "matches",
                      const std::string& popularityField = "popularity") {

    if (swipeExists(swipe_collection, sourceEntityOid, targetEntityOid)) {
        std::cerr << "Swipe already exists from " << sourceEntityOid.to_string()
                << " to " << targetEntityOid.to_string() << std::endl;
        return;
    }

    auto timestamp = std::chrono::system_clock::now();
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()).count();

    auto swipe_doc = document{}
        << "sourceEntityId" << sourceEntityOid.to_string()
        << "targetEntityId" << targetEntityOid.to_string()
        << "timestamp" << bsoncxx::types::b_date{std::chrono::milliseconds{timestamp_ms}}
        << finalize;

    swipe_collection.insert_one(swipe_doc.view());

    auto source_entity = entity_collection.find_one(document{} << "_id" << sourceEntityOid << finalize);
    if (!source_entity) {
        std::cerr << "Source entity not found: " << sourceEntityOid.to_string() << std::endl;
        return;
    }

    auto source_doc = source_entity->view();
    int currentMade = source_doc[madeField] ? source_doc[madeField].get_int32().value : 0;

    auto update_source = document{}
        << "$set" << open_document
        << madeField << (currentMade + 1)
        << close_document
        << finalize;
    entity_collection.update_one(document{} << "_id" << sourceEntityOid << finalize, std::move(update_source));

    if (isLike) {
        updateEntityPopularity(entity_collection, targetEntityOid);

        // Check for mutual like
        auto mutual_swipe = swipe_collection.find_one(
            document{}
                << "sourceEntityId" << targetEntityOid.to_string()
                << "targetEntityId" << sourceEntityOid.to_string()
                << finalize
        );
        if (mutual_swipe) {
            updateEntityMatches(entity_collection, sourceEntityOid, targetEntityOid);
        }
    }
}


// --- High-level API Functions ---
crow::json::wvalue getRecommendedRoommates() {
    crow::json::wvalue result;
    auto& dbManager = getDbManager();
    result["roommates"] = crow::json::wvalue::list();
    try {
        auto user_collection = dbManager.getUserCollection();
        auto cursor = user_collection.find({});
        for (auto&& doc : cursor) {
            crow::json::wvalue roommate;
            roommate["id"] = doc["_id"].get_oid().value.to_string();
            roommate["username"] = doc["username"] ? std::string(doc["username"].get_string().value) : "";
            roommate["popularity"] = doc["popularity"] ? doc["popularity"].get_double().value : 0.0;
            roommate["matches"] = doc["matches"] ? doc["matches"].get_int32().value : 0;
            result["roommates"][result["roommates"].size()] = std::move(roommate);
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        result["error"] = "Backend error: " + std::string(e.what());
    }
    return result;
}

crow::json::wvalue getRecommendedRooms() {
    auto& dbManager = getDbManager();
    auto room_collection = dbManager.getRoomCollection();
    crow::json::wvalue result;
    result["rooms"] = crow::json::wvalue::list();

    auto cursor = room_collection.find(
        {},
        mongocxx::options::find{}.sort(document{} << "popularity" << -1 << finalize).limit(10)
    );

    for (auto&& doc : cursor) {
        crow::json::wvalue room;
        room["id"] = doc["_id"].get_oid().value.to_string();
        room["name"] = doc["name"] ? std::string(doc["name"].get_string().value) : "";
        room["popularity"] = doc["popularity"] ? doc["popularity"].get_double().value : 0.0;
        result["rooms"][result["rooms"].size()] = std::move(room);
    }

    return result;
}

crow::json::wvalue processRoommateSwipe(const std::string& sourceId, const std::string& targetId, bool isLike) {
    auto& dbManager = getDbManager();
    auto user_collection = dbManager.getUserCollection();
    auto user_swipe_collection = dbManager.getUserSwipeCollection();
    handleEntitySwipe(user_collection, user_swipe_collection,
                      bsoncxx::oid(sourceId), bsoncxx::oid(targetId), isLike);
    return crow::json::wvalue({{"status", "Roommate swipe processed"}});
}

crow::json::wvalue processRoomSwipe(const std::string& sourceId, const std::string& targetId, bool isLike) {
    auto& dbManager = getDbManager();
    auto room_collection = dbManager.getRoomCollection();
    auto room_swipe_collection = dbManager.getRoomSwipeCollection();
    handleEntitySwipe(room_collection, room_swipe_collection,
                      bsoncxx::oid(sourceId), bsoncxx::oid(targetId), isLike);
    return crow::json::wvalue({{"status", "Room swipe processed"}});
}

// --- Test function remains unchanged ---
int test() {
    try
    {
        auto& dbManager = getDbManager();
        // --- Testing functions below ---
        auto entity_collection = dbManager.getUserCollection();
        auto swipe_collection = dbManager.getUserSwipeCollection();

        document entity_doc_builder1, entity_doc_builder2;
        entity_doc_builder1 << "name" << "EntityA" << "swipesMade" << 0 << "swipesReceived" << 0 << "matches" << 0 << "popularity" << 0.0;
        entity_doc_builder2 << "name" << "EntityB" << "swipesMade" << 0 << "swipesReceived" << 0 << "matches" << 0 << "popularity" << 0.0;

        auto result1 = entity_collection.insert_one(entity_doc_builder1 << finalize);
        auto result2 = entity_collection.insert_one(entity_doc_builder2 << finalize);

        bsoncxx::oid sourceEntityOid, targetEntityOid;
        if (result1 && result1->inserted_id().type() == bsoncxx::type::k_oid) {
            sourceEntityOid = result1->inserted_id().get_oid().value;
            std::cout << "Inserted source entity with _id: " << sourceEntityOid.to_string() << std::endl;
        }
        if (result2 && result2->inserted_id().type() == bsoncxx::type::k_oid) {
            targetEntityOid = result2->inserted_id().get_oid().value;
            std::cout << "Inserted target entity with _id: " << targetEntityOid.to_string() << std::endl;
        }

        std::cout << "Inserted test entities." << std::endl;

        // Test handleEntitySwipe (simulate a like swipe)
        handleEntitySwipe(entity_collection, swipe_collection, sourceEntityOid, targetEntityOid, true);
        std::cout << "handleEntitySwipe executed (like)." << std::endl;

        // Test handleEntitySwipe (simulate a dislike swipe)
        handleEntitySwipe(entity_collection, swipe_collection, sourceEntityOid, targetEntityOid, false);
        std::cout << "handleEntitySwipe executed (dislike)." << std::endl;

        // Simulate EntityA likes EntityB (no match yet)
        handleEntitySwipe(entity_collection, swipe_collection, sourceEntityOid, targetEntityOid, true);
        std::cout << "EntityA liked EntityB." << std::endl;

        // Check matches count for both entities (should be 0)
        auto entityA = entity_collection.find_one(document{} << "_id" << sourceEntityOid << finalize);
        auto entityB = entity_collection.find_one(document{} << "_id" << targetEntityOid << finalize);
        int matchesA = entityA && entityA->view()["matches"] ? entityA->view()["matches"].get_int32().value : 0;
        int matchesB = entityB && entityB->view()["matches"] ? entityB->view()["matches"].get_int32().value : 0;
        std::cout << "Matches after first like: EntityA=" << matchesA << ", EntityB=" << matchesB << std::endl;

        // Simulate EntityB likes EntityA (should trigger a match)
        handleEntitySwipe(entity_collection, swipe_collection, targetEntityOid, sourceEntityOid, true);
        std::cout << "EntityB liked EntityA." << std::endl;

        // Check matches count for both entities (should be 1)
        entityA = entity_collection.find_one(document{} << "_id" << sourceEntityOid << finalize);
        entityB = entity_collection.find_one(document{} << "_id" << targetEntityOid << finalize);
        matchesA = entityA && entityA->view()["matches"] ? entityA->view()["matches"].get_int32().value : 0;
        matchesB = entityB && entityB->view()["matches"] ? entityB->view()["matches"].get_int32().value : 0;
        std::cout << "Matches after mutual like: EntityA=" << matchesA << ", EntityB=" << matchesB << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}