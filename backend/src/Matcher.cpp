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
 * Normalizes a proximity score based on the distance in kilometers.
 * @param distanceKm The distance in kilometers.
 * @param maxDistanceKm The maximum distance to consider for normalization.
 * @return A normalized proximity score between 0.0 (far) and 1.0 (very close).
 */
// TODO: Adjust the max distance value 
double normalizeProximity(double distanceKm, double maxDistanceKm = 50.0) {
    double score = 1.0 - std::min(distanceKm, maxDistanceKm) / maxDistanceKm;
    return score; // 1.0 (very close) to 0.0 (far)
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
 * @param proximity The proximity score of the entity.
 * @return A calculated popularity score.
 */
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

/**
 * Fetches recommended roommates for the current user based on their preferences.
 * @param currentUserId The ID of the current user.
 * @return A JSON object containing recommended roommates.
 */
bool swipeExists(mongocxx::collection& swipe_collection, const bsoncxx::oid& sourceEntityOid, const bsoncxx::oid& targetEntityOid) {
    auto swipe_doc = swipe_collection.find_one(document{} << "sourceEntityId" << sourceEntityOid.to_string() << finalize);
    if (!swipe_doc) return false;

    auto view = swipe_doc->view();
    if (view["targetEntityId"] && view["targetEntityId"].type() == bsoncxx::type::k_array) {
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

/// yeah i didn't do this function chat just gave this to me
/** 
 * Haversine formula to calculate the distance between two points on the Earth
 * given their latitude and longitude in degrees.
 * @param lat1 Latitude of point 1
 * @param lon1 Longitude of point 1
 * @param lat2 Latitude of point 2
 * @param lon2 Longitude of point 2
 * @return Distance in kilometers
 */
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

/**
 * Updates the popularity score of an entity (user or room) based on swipes received.
 * @param entity_collection The collection of entities (users or rooms).
 * @param entityOid The OID of the entity to update.
 * @param proximity The proximity score of the entity.
 */
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

/**
 * Updates the matches count for both entities involved in a swipe.
 * @param entity_collection The collection of entities (users or rooms).
 * @param sourceEntityOid The OID of the source entity.
 * @param targetEntityOid The OID of the target entity.
 */
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

/**
 * Handles the swipe action between two entities (users or rooms).
 * @param source_collection The collection of the source entity (user or room).
 * @param target_collection The collection of the target entity (user or room).
 * @param swipe_collection The collection for storing swipe actions.
 * @param sourceEntityOid The OID of the source entity.
 * @param targetEntityOid The OID of the target entity.
 * @param isLike True if it's a like, false if it's a dislike.
 */
void handleEntitySwipe(mongocxx::collection& source_collection,
                        mongocxx::collection& target_collection,
                        mongocxx::collection& swipe_collection,
                        const bsoncxx::oid& sourceEntityOid,
                        const bsoncxx::oid& targetEntityOid,
                        bool isLike) {

    if (swipeExists(swipe_collection, sourceEntityOid, targetEntityOid)) {
        std::cerr << "Swipe already exists from " << sourceEntityOid.to_string()
                << " to " << targetEntityOid.to_string() << std::endl;
        return;
    }
    auto swipe_doc = swipe_collection.find_one(
        document{} << "sourceEntityId" << sourceEntityOid.to_string() << finalize
    );

    // If swipe document exists, update it; otherwise, create a new one
    bsoncxx::oid userswipe_oid;
    if (swipe_doc) {
        userswipe_oid = swipe_doc->view()["_id"].get_oid().value;
        // Append targetEntityOid to the array
        swipe_collection.update_one(
            document{} << "_id" << userswipe_oid << finalize,
            document{} << "$addToSet" << open_document
                << "targetEntityId" << targetEntityOid.to_string()
            << close_document << finalize
        );
    } else {
        // Create new swipe document with array
        auto new_swipe_doc = document{}
            << "sourceEntityId" << sourceEntityOid.to_string()
            << "targetEntityId" << bsoncxx::builder::stream::open_array
                << targetEntityOid.to_string()
            << bsoncxx::builder::stream::close_array
            << finalize;
        auto insert_result = swipe_collection.insert_one(new_swipe_doc.view());
        userswipe_oid = insert_result->inserted_id().get_oid().value;
    }

    auto source_entity = source_collection.find_one(document{} << "_id" << sourceEntityOid << finalize);
    if (!source_entity) {
        std::cerr << "Source entity not found: " << sourceEntityOid.to_string() << std::endl;
        return;
    }

    auto target_entity = target_collection.find_one(document{} << "_id" << targetEntityOid << finalize);
    if (!target_entity) {
        std::cerr << "Target entity not found: " << targetEntityOid.to_string() << std::endl;
        return;
    }

    auto source_doc = source_entity->view();
    int currentMade = source_doc["swipesMade"] ? source_doc["swipesMade"].get_int32().value : 0;

    auto update_source = document{}
        << "$set" << open_document
        << "swipesMade" << (currentMade + 1)
        << close_document
        << finalize;
    source_collection.update_one(document{} << "_id" << sourceEntityOid << finalize, std::move(update_source));

    if (isLike) {
        double proximity = haversine(
            source_doc["lat"].get_double().value,
            source_doc["long"].get_double().value,
            target_entity->view()["lat"].get_double().value,
            target_entity->view()["long"].get_double().value
        );
        updateEntityPopularity(target_collection, targetEntityOid, proximity);

        // Check for mutual like
        auto mutual_swipe = swipe_collection.find_one(
            document{}
                << "sourceEntityId" << targetEntityOid.to_string()
                << "targetEntityId" << sourceEntityOid.to_string()
                << finalize
        );
        if (mutual_swipe) {
            updateEntityMatches(source_collection, sourceEntityOid, targetEntityOid);
        }
    }
}


// --- High-level API Functions ---

/*
    * Fetches recommended roommates for the current user based on proximity and popularity.
    * The roommates are scored based on their distance from the user's location and their popularity.
    * @param currentUserId The ID of the current user.
    * @return A JSON object containing the recommended roommates.
*/
crow::json::wvalue getRecommendedRoommates(const std::string& currentUserId) {
    crow::json::wvalue result;
    auto& dbManager = getDbManager();
    auto user_collection = dbManager.getUserCollection();

    result["roommates"] = crow::json::wvalue::list();
    auto current_user_doc = user_collection.find_one(document{} << "_id" << bsoncxx::oid(currentUserId) << finalize);
    if (!current_user_doc) {
        result["error"] = "Current user not found.";
        return result;
    }

    auto current_user_view = current_user_doc->view();
    double currentLat = current_user_view["lat"].get_double().value;
    double currentLong = current_user_view["long"].get_double().value;

    auto cursor = user_collection.find({});
    std::vector<std::pair<double, crow::json::wvalue>> scoredUsers;
    const double maxDistanceKm = 50.0;
    for (auto&& doc : cursor) {
        try {
            if (doc["_id"].get_oid().value.to_string() == currentUserId) {
                continue; 
            }
            double userLat = doc["lat"].get_double().value;
            double userLong = doc["long"].get_double().value;

            double distance = haversine(currentLat, currentLong, userLat, userLong);
            
            if (distance > maxDistanceKm) {
                continue;
            }

            double normalizedDistance = normalizeProximity(distance);

            double popularity = doc["popularity"] ? doc["popularity"].get_double().value : 0.0;
            double normalizedPopularity = normalizePopularity(popularity);

            double weightDistance = 0.4;
            double weightPopularity = 0.6;
            double combinedScore = (weightDistance * normalizedDistance) + (weightPopularity * normalizedPopularity);

            // Prepare user data
            crow::json::wvalue user;
            user["id"] = doc["_id"].get_oid().value.to_string();
            user["username"] = doc["username"] ? std::string(doc["username"].get_string().value) : "";
            user["distance"] = distance;
            user["popularity"] = normalizedPopularity;

            scoredUsers.emplace_back(combinedScore, std::move(user));
        } catch (const std::exception& e) {
            std::cerr << "Exception for doc: " << bsoncxx::to_json(doc) << "\nError: " << e.what() << std::endl;
        }
    }

    std::sort(scoredUsers.begin(), scoredUsers.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });

    for (size_t i = 0; i < std::min(scoredUsers.size(), size_t(10)); ++i) {
        result["roommates"][i] = std::move(scoredUsers[i].second);
    }

    return result;
}

/*
    * Fetches recommended rooms for the current user based on proximity and popularity.
    * The rooms are scored based on their distance from the user's location and their popularity.
    * @param currentUserId The ID of the current user.
    * @return A JSON object containing the recommended rooms.
*/
crow::json::wvalue getRecommendedRooms(const std::string& currentUserId) {
    crow::json::wvalue result;
    auto& dbManager = getDbManager();
    auto user_collection = dbManager.getUserCollection();
    auto current_user_doc = user_collection.find_one(document{} << "_id" << bsoncxx::oid(currentUserId) << finalize);
    if (!current_user_doc) {
        result["error"] = "Current user not found.";
        return result;
    }

    auto current_user_view = current_user_doc->view();
    double currentLat = current_user_view["lat"].get_double().value;
    double currentLong = current_user_view["long"].get_double().value;

    std::vector<std::pair<double, crow::json::wvalue>> scoredRooms;

    auto room_collection = dbManager.getRoomCollection();
    result["rooms"] = crow::json::wvalue::list();
    auto cursor = room_collection.find({});
    const double maxDistanceKm = 50.0;
    try {
        for (auto&& doc : cursor) {
            try {
                if (doc["ownerId"] && doc["ownerId"].type() == bsoncxx::type::k_oid) {
                    auto ownerId = doc["ownerId"].get_oid().value;
                    if (ownerId.to_string() == currentUserId) {
                        continue;
                    }
                }
                double roomLat = doc["lat"].get_double().value;
                double roomLong = doc["long"].get_double().value;

                double distance = haversine(currentLat, currentLong, roomLat, roomLong);
                
                if (distance > maxDistanceKm) {
                    continue;
                }

                double normalizedDistance = normalizeProximity(distance, maxDistanceKm);

                double popularity = doc["popularity"] ? doc["popularity"].get_double().value : 0.0;
                double normalizedPopularity = normalizePopularity(popularity);

                double weightDistance = 0.7;
                double weightPopularity = 0.3;
                double combinedScore = (weightDistance * normalizedDistance) + (weightPopularity * normalizedPopularity);

                crow::json::wvalue room;
                room["id"] = doc["_id"].get_oid().value.to_string();
                room["address"] = doc["address"] ? std::string(doc["address"].get_string().value) : "";
                room["address_line"] = doc["address_line"] ? std::string(doc["address_line"].get_string().value) : "";
                room["country"] = doc["country"] ? std::string(doc["country"].get_string().value) : "";
                room["phone"] = doc["phone"] ? std::string(doc["phone"].get_string().value) : "";
                room["budget"] = doc["budget"] ? std::string(doc["budget"].get_string().value) : "0.0";
                room["popularity"] = normalizedPopularity;
                room["distance"] = distance;
                result["rooms"][result["rooms"].size()] = std::move(room);
            } catch (const std::exception& e) {
                std::cerr << "Exception for doc: " << bsoncxx::to_json(doc) << "\nError: " << e.what() << std::endl;
                result["error"] = "Backend error: " + std::string(e.what());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error fetching recommended rooms: " << e.what() << std::endl;
        result["error"] = "Backend error: " + std::string(e.what());
    }

    std::sort(scoredRooms.begin(), scoredRooms.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });

    for (size_t i = 0; i < std::min(scoredRooms.size(), size_t(10)); ++i) {
        result["rooms"][i] = std::move(scoredRooms[i].second);
    }

    return result;
}

/**
 * Fetches users who liked the given user.
 * @param userId The ID of the user to check likes for.
 * @return A JSON object containing the users who liked the specified user.
 */
crow::json::wvalue getUsersWhoLiked(const std::string& userId) {
    auto& dbManager = getDbManager();
    auto user_swipe_collection = dbManager.getUserSwipeCollection();
    auto user_collection = dbManager.getUserCollection();

    crow::json::wvalue result;
    result["users"] = crow::json::wvalue::list();

    try {
        // Find all swipe documents where the user is the source
        auto cursor = user_swipe_collection.find(
            document{} << "sourceEntityId" << userId << finalize
        );

        for (auto&& swipe_doc : cursor) {
            if (swipe_doc["targetEntityId"] && swipe_doc["targetEntityId"].type() == bsoncxx::type::k_array) {
                for (auto&& oid_elem : swipe_doc["targetEntityId"].get_array().value) {
                    std::string targetId = std::string(oid_elem.get_string().value);

                    // Fetch user details from the user collection
                    auto user_doc = user_collection.find_one(document{} << "_id" << bsoncxx::oid(targetId) << finalize);
                    if (user_doc) {
                        auto user_view = user_doc->view();
                        crow::json::wvalue user;
                        user["id"] = targetId;
                        user["username"] = user_view["username"] ? std::string(user_view["username"].get_string().value) : "";
                        user["popularity"] = user_view["popularity"] ? user_view["popularity"].get_double().value : 0.0;
                        user["matches"] = user_view["matches"] ? user_view["matches"].get_int32().value : 0;

                        result["users"][result["users"].size()] = std::move(user);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error fetching users who liked the user: " << e.what()
                    << std::endl;
        result["error"] = "Backend error: " + std::string(e.what());
    }
    return result;
}

/**
 * Fetches users who liked the given room.
 * @param roomId The ID of the room to check likes for.
 * @return A JSON object containing the users who liked the specified room.
 */
crow::json::wvalue getUsersWhoLikedRoom(const std::string& roomId) {
    auto& dbManager = getDbManager();
    auto room_swipe_collection = dbManager.getRoomSwipeCollection();
    auto user_collection = dbManager.getUserCollection();

    crow::json::wvalue result;
    result["users"] = crow::json::wvalue::list();

    try {
        // Find all swipe documents where the room is the target
        auto cursor = room_swipe_collection.find(
            document{} << "targetEntityId" << roomId << finalize
        );

        for (auto&& swipe_doc : cursor) {
            auto swipe_view = swipe_doc;
            std::string userId = std::string(swipe_doc["sourceEntityId"].get_string().value);

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
        std::cerr << "Error fetching users who liked the room: " << e.what() << std::endl;
        result["error"] = "Backend error: " + std::string(e.what());
    }

    return result;
}

/**
 * Processes a swipe action for a roommate.
 * @param sourceId The ID of the user performing the swipe.
 * @param targetId The ID of the roommate being swiped on.
 * @param isLike True if it's a like, false if it's a dislike.
 * @return A JSON object indicating the status of the swipe action.
 */
crow::json::wvalue processRoommateSwipe(const std::string& sourceId, const std::string& targetId, bool isLike) {
    auto& dbManager = getDbManager();
    auto user_collection = dbManager.getUserCollection();
    auto user_swipe_collection = dbManager.getUserSwipeCollection();
    handleEntitySwipe(user_collection, user_collection, user_swipe_collection,
                      bsoncxx::oid(sourceId), bsoncxx::oid(targetId), isLike);
    return crow::json::wvalue({{"status", "Roommate swipe processed"}});
}

/**
 * Processes a swipe action for a room.
 * @param sourceId The ID of the user performing the swipe.
 * @param targetId The ID of the room being swiped on.
 * @param isLike True if it's a like, false if it's a dislike.
 * @return A JSON object indicating the status of the swipe action.
 */
crow::json::wvalue processRoomSwipe(const std::string& sourceId, const std::string& targetId, bool isLike) {
    auto& dbManager = getDbManager();
    auto user_collection = dbManager.getUserCollection();
    auto room_collection = dbManager.getRoomCollection();
    auto room_swipe_collection = dbManager.getRoomSwipeCollection();
    handleEntitySwipe(user_collection, room_collection, room_swipe_collection,
                      bsoncxx::oid(sourceId), bsoncxx::oid(targetId), isLike);
    return crow::json::wvalue({{"status", "Room swipe processed"}});
}
