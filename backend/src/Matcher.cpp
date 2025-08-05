#define _USE_MATH_DEFINES
#include <cmath>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
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

/**
 * Normalizes a budget value to a range of 0.0 to 1.0.
 * @param budget The budget value to normalize.
 * @return A normalized budget value between 0.0 and 1.0.
 */
// TODO: Adjust the min and max budget values
double normalizeBudget(double budget) {
    double minBudget = 500.0;
    double maxBudget = 100000.0;
    return (budget - minBudget) / (maxBudget - minBudget); // Result: 0.0 to 1.0
}

/**
 * Normalizes a popularity score to a range of 0.0 to 1.0.
 * @param popularity The raw popularity score.
 * @param minPopularity The minimum possible popularity score.
 * @param maxPopularity The maximum possible popularity score.
 * @return A normalized popularity score between 0.0 and 1.0.
 */
double normalizePopularity(double popularity, double minPopularity = -50, double maxPopularity = 3200) {
    if (maxPopularity == minPopularity) return 0.0;
    return std::max(0.0, std::min(1.0, (popularity - minPopularity) / (maxPopularity - minPopularity)));
}

/**
 * Calculates a popularity score based on various factors.
 * @param received Number of swipes received.
 * @param made Number of swipes made.
 * @param matches Number of matches.
 * @param budget The budget of the entity.
 * @return A calculated popularity score.
 */
double calculatePopularity(int received, int made, int matches, double budget) {
    // Adjust weights for each factor based on their importance
    // TODO: Train the data based on data to a machine learning model for a popularity score
    double w_received = 1.0;
    double w_made = -0.5;
    double w_matches = 2.0;
    double w_budget = 0.2;
    double normalizedBudget = normalizeBudget(budget);

    return w_received * received
         + w_made * made
         + w_matches * matches
         + w_budget * normalizedBudget;
}


/**
 * Fetches recommended roommates for the current user based on their preferences.
 * @param currentUserId The ID of the current user.
 * @return A JSON object containing recommended roommates.
 */
bool swipeExists(mongocxx::collection& swipe_collection, const bsoncxx::oid& sourceEntityOid, const bsoncxx::oid& targetEntityOid) {
    auto swipe_doc = swipe_collection.find_one(document{} << "sourceEntityId" << sourceEntityOid.to_string() << finalize);
    if (!swipe_doc) return false;

    auto view = swipe_doc->view();
    if (view["targetEntityId"]) {
        auto target_array = view["targetEntityId"].get_array().value;
        for (auto&& oid_elem : target_array) {
            std::string target_id = std::string(oid_elem.get_string().value);
            if (target_id == targetEntityOid.to_string()) {
                return true;
            }
        }
    }
    return false;
}



/**
 * Updates the popularity score of an entity (user or room) based on swipes received.
 * @param entity_collection The collection of entities (users or rooms).
 * @param entityOid The OID of the entity to update.
 * @param proximity The proximity score of the entity.
 */
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
    double newPopularity = calculatePopularity(newReceived, currentMade, currentMatches, budget);

    entity_collection.update_one(document{} << "_id" << entityOid << finalize,
        document{}
        << "$set" << open_document
        << "swipesReceived" << newReceived
        << "popularity" << newPopularity
        << close_document
        << finalize);
}

/**
 * Updates the matches count for both entities involved in a swipe.
 * @param source_collection The collection of the source entity (user or room).
 * @param target_collection The collection of the target entity (user or room).
 * @param sourceEntityOid The OID of the source entity.
 * @param targetEntityOid The OID of the target entity.
 */
void updateEntityMatches(mongocxx::collection& source_collection,
                         mongocxx::collection& target_collection,
                         const bsoncxx::oid& sourceEntityOid,
                         const bsoncxx::oid& targetEntityOid) {
    source_collection.update_one(
        document{} << "_id" << sourceEntityOid << finalize,
        document{} << "$inc" << open_document << "matches" << 1 << close_document << finalize);

    target_collection.update_one(
        document{} << "_id" << targetEntityOid << finalize,
        document{} << "$inc" << open_document << "matches" << 1 << close_document << finalize);
}

/**
 * Handles the like action for an entity (user or room).
 * @param source_collection The collection of the source entity (user or room).
 * @param target_collection The collection of the target entity (user or room).
 * @param swipe_collection The collection for storing swipe actions.
 * @param sourceEntityOid The OID of the source entity.
 * @param targetEntityOid The OID of the target entity.
 */
void handleEntityLike(mongocxx::collection& source_collection,
                      mongocxx::collection& target_collection,
                      mongocxx::collection& swipe_collection,
                      const bsoncxx::oid& sourceEntityOid,
                      const bsoncxx::oid& targetEntityOid) {

    updateEntityPopularity(target_collection, targetEntityOid);

    // Check for mutual like
    auto filter = document{}
                  << "sourceEntityId" << targetEntityOid.to_string()
                  << "targetEntityId" << sourceEntityOid.to_string()
                  << finalize;
    if (swipe_collection.find_one(filter.view())) {
        updateEntityMatches(source_collection, target_collection, sourceEntityOid, targetEntityOid);
    }
    
}

/**
 * Handles the swipe action between two entities (users or rooms).
 * @param source_collection The collection of the source entity (user or room).
 * @param swipe_collection The collection for storing swipe actions.
 * @param sourceEntityOid The OID of the source entity.
 * @param targetEntityOid The OID of the target entity.
 */
void handleEntitySwipe(mongocxx::collection& source_collection,
                        mongocxx::collection& swipe_collection,
                        const bsoncxx::oid& sourceEntityOid,
                        const bsoncxx::oid& targetEntityOid) {

    mongocxx::options::update opts;
    opts.upsert(true);
    swipe_collection.update_one(
        document{} << "sourceEntityId" << sourceEntityOid.to_string() << finalize,
        document{}
            << "$setOnInsert" << open_document
                << "sourceEntityId" << sourceEntityOid.to_string()
            << close_document
            << "$addToSet" << open_document
                << "targetEntityId" << targetEntityOid.to_string()
            << close_document
            << finalize,
        opts);

    source_collection.update_one(
        document{} << "_id" << sourceEntityOid << finalize,
        document{}
            << "$inc" << open_document
                << "swipesMade" << 1
            << close_document
            << finalize);
}


// --- High-level API Functions ---
/**
 * Fetches recommended roommates or rooms for the current user based on their preferences.
 * @param currentUserId The ID of the current user.
 * @param type The type of recommendation to fetch ("roommate" or "room").
 * @return A JSON object containing recommended roommates or rooms.
 */
crow::json::wvalue getRecommendations(const std::string& currentUserId, const std::string& type) {

    if (type != "roommate" && type != "room") {
        return crow::json::wvalue({{"error", "Invalid type parameter. Use 'roommate' or 'room'."}});
    }

    crow::json::wvalue result;
    auto& db = getDbManager();
    auto& userColl = db.getUserCollection();
    auto& roomColl = db.getRoomCollection();
    auto& entityColl = (type == "roommate") ? userColl : roomColl;

    auto currentDoc = userColl.find_one(
        document{} << "_id" << bsoncxx::oid(currentUserId) << finalize
    );
    if (!currentDoc) {
        return { { "error", "Current user not found." } };
    }

    auto current_view = currentDoc->view();
    std::string country = std::string(current_view["country"].get_string().value);
    std::string city    = std::string(current_view["city"].get_string().value);
    auto cursor = entityColl.find(document{} << "country" << country << "city" << city << finalize);
    
    std::vector<std::pair<double, crow::json::wvalue>> scored;

    for (auto&& doc : cursor) {
        try {
            if (type == "room") {
                if (doc["ownerId"] && doc["ownerId"].type() == bsoncxx::type::k_oid) {
                    auto ownerId = doc["ownerId"].get_oid().value;
                    if (ownerId.to_string() == currentUserId) {
                        continue;
                    }
                }
            } else {
                if (doc["_id"].type() == bsoncxx::type::k_oid &&
                    doc["_id"].get_oid().value.to_string() == currentUserId)
                    continue;
            }
            
            double popularity = doc["popularity"] ? doc["popularity"].get_double().value : 0.0;
            double norm_Pop = normalizePopularity(popularity);

            crow::json::wvalue entity;
            entity["id"] = doc["_id"].get_oid().value.to_string();

            if (type == "roommate") {
                entity["username"]  = doc["username"]  ? std::string(doc["username"] .get_string().value) : "";
                entity["firstName"] = doc["firstName"] ? std::string(doc["firstName"].get_string().value) : "";
                entity["lastName"]  = doc["lastName"]  ? std::string(doc["lastName"] .get_string().value) : "";
            }

            entity["address"]      = doc["address"]      ? std::string(doc["address"]     .get_string().value) : "";
            entity["address_line"] = doc["address_line"] ? std::string(doc["address_line"].get_string().value) : "";
            entity["city"]         = doc["city"]         ? std::string(doc["city"]        .get_string().value) : "";
            entity["country"]      = doc["country"]      ? std::string(doc["country"]     .get_string().value) : ""; 
            entity["phone"]        = doc["phone"]        ? std::string(doc["phone"]       .get_string().value) : "";
            entity["budget"]       = doc["budget"]       ? std::string(doc["budget"]      .get_string().value) : "0.0";
            entity["popularity"]   = norm_Pop;

            scored.emplace_back(norm_Pop, std::move(entity));
        } catch (const std::exception& e) {
            std::cerr << "Exception for doc: " << bsoncxx::to_json(doc)
                        << "\nError: " << e.what() << std::endl;
            result["error"] = "Backend error: " + std::string(e.what());
        }
    }

    std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });

    result["entity"] = crow::json::wvalue::list();
    for (size_t i = 0; i < std::min(scored.size(), size_t(10)); ++i) {
        result["entity"][i] = std::move(scored[i].second);
    }

    return result;
}

/**
 * Fetches users who liked a specific entity (user or room).
 * @param entityId The ID of the entity to check likes for.
 * @param type The type of entity ("roommate" or "room").
 * @return A JSON object containing users who liked the entity.
 */
crow::json::wvalue getUserWhoLikedEntity(const std::string& entityId, const std::string& type) {
    if (type != "roommate" && type != "room") {
        return crow::json::wvalue({{"error", "Invalid type parameter. Use 'roommate' or 'room'."}});
    }

    auto& dbManager = getDbManager();
    auto swipe_collection = (type == "roommate") ? dbManager.getUserSwipeCollection() : dbManager.getRoomSwipeCollection();
    auto user_collection = dbManager.getUserCollection();

    crow::json::wvalue result;
    result["users"] = crow::json::wvalue::list();

    try {
        // Find all swipe documents where the entity is the target
        auto cursor = swipe_collection.find(
            document{} << "targetEntityId" << entityId << finalize
        );

        for (auto&& swipe_doc : cursor) {
            auto swipe_view = swipe_doc.view();
            std::string userId = std::string(swipe_view["sourceEntityId"].get_string().value);

            // Fetch user details from the user collection
            auto user_doc = user_collection.find_one(document{} << "_id" << bsoncxx::oid(userId) << finalize);
            if (user_doc) {
                auto user_view = user_doc->view();
                crow::json::wvalue user;
                user["id"] = userId;
                user["username"] = user_view["username"] ? std::string(user_view["username"].get_string().value) : "";
                user["popularity"] = user_view["popularity"] ? user_view["popularity"].get_double().value : 0.0;
                user["matches"] = user_view["matches"] ? user_view["matches"].get_int32().value : 0;

                result["users"][result["users"].size()] = std::move(user);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error fetching users who liked the entity: " << e.what()
                    << std::endl;
        result["error"] = "Backend error: " + std::string(e.what());
    }
    return result;
}



/**
 * Processes a swipe action for a room.
 * @param sourceId The ID of the user performing the swipe.
 * @param targetId The ID of the room being swiped on.
 * @param type The type of entity being swiped on ("roommate" or "room").
 * @param isLike True if it's a like, false if it's a dislike.
 * @return A JSON object indicating the status of the swipe action.
 */
crow::json::wvalue processSwipe(const std::string& sourceId, const std::string& targetId, const std::string& type, bool isLike) {
    auto& dbManager = getDbManager();
    auto& db       = getDbManager();
    auto& userColl = db.getUserCollection();
    bsoncxx::oid srcOid(sourceId);
    bsoncxx::oid tgtOid(targetId);

    if (type != "roommate" && type != "room") {
        return crow::json::wvalue({{"error", "Invalid type parameter. Use 'roommate' or 'room'."}});
    }

    auto& swipeColl  = (type == "roommate" ? db.getUserSwipeCollection() : db.getRoomSwipeCollection());
    auto& targetColl = (type == "roommate" ? userColl : db.getRoomCollection());

    handleEntitySwipe(userColl, swipeColl, srcOid, tgtOid);

    if (isLike) handleEntityLike(userColl, targetColl, swipeColl, srcOid, tgtOid);   

    return crow::json::wvalue({{"status", "Room swipe processed"}});
}
