#define _USE_MATH_DEFINES
#include <cmath>
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

// TODO: Adjust the min and max budget values
double normalizeBudget(double budget) {
    double minBudget = 500.0;
    double maxBudget = 100000.0;
    return (budget - minBudget) / (maxBudget - minBudget); // Result: 0.0 to 1.0
}

// TODO: Adjust the max distance value 
double normalizeProximity(double distanceKm, double maxDistanceKm = 50.0) {
    double score = 1.0 - std::min(distanceKm, maxDistanceKm) / maxDistanceKm;
    return score; // 1.0 (very close) to 0.0 (far)
}


double calculatePopularity(int received, int made, int matches, double budget, double proximity) {
    // Adjust weights for each factor based on their importance
    // TODO: Train the data based on data to a machine learning model for a popularity score
    double w_received = 1.0;
    double w_made = -0.5;
    double w_matches = 2.0;
    double w_budget = 0.2;
    double w_proximity = 0.3;
    double normalizedBudget = normalizeBudget(budget);

    return w_received * received
         + w_made * made
         + w_matches * matches
         + w_budget * normalizedBudget
         + w_proximity * proximity;
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

/// yeah i didn't do this function chat just gave this to me
double haversine(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371.0; // Earth radius in km
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = sin(dLat/2) * sin(dLat/2) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return R * c; // Distance in km
}

void updateEntityPopularity(mongocxx::collection& entity_collection, const bsoncxx::oid& entityOid, double proximity = 0.0) {
    auto maybe_entity = entity_collection.find_one(document{} << "_id" << entityOid << finalize);
    if (!maybe_entity) {
        std::cerr << "Entity not found: " << entityOid.to_string() << std::endl;
        return;
    }

    auto entity_doc = maybe_entity->view();
    int currentMade = entity_doc["swipesMade"] ? entity_doc["swipesMade"].get_int32().value : 0;
    int currentReceived = entity_doc["swipesReceived"] ? entity_doc["swipesReceived"].get_int32().value : 0;
    int currentMatches = entity_doc["matches"] ? entity_doc["matches"].get_int32().value : 0;
    std::string budgetStr = entity_doc["budget"] && entity_doc["budget"].type() == bsoncxx::type::k_string
        ? std::string(entity_doc["budget"].get_string().value)
        : "0.0";
    double budget = 0.0;
    try {
        // Remove $ if present
        if (!budgetStr.empty() && budgetStr[0] == '$') budgetStr = budgetStr.substr(1);
        budget = std::stod(budgetStr);
    } catch (...) {
        budget = 0.0;
    }

    int newReceived = currentReceived + 1;
    double newPopularity = calculatePopularity(newReceived, currentMade, currentMatches, budget, proximity);

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
    auto target_entity = entity_collection.find_one(document{} << "_id" << targetEntityOid << finalize);
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
        double proximity = haversine(
            source_doc["lat"].get_double().value,
            source_doc["long"].get_double().value,
            target_entity->view()["lat"].get_double().value,
            target_entity->view()["long"].get_double().value
        );
        updateEntityPopularity(entity_collection, targetEntityOid, proximity);

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
