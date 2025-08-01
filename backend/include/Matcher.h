#pragma once

#include <mongocxx/collection.hpp>
#include <bsoncxx/oid.hpp>
#include <crow/crow_all.h>
#include <string>

// Business logic
double calculatePopularity(int received, int made, int matches = 0);
void updateEntityPopularity(mongocxx::collection& entity_collection, const bsoncxx::oid& entityOid);
void updateEntityMatches(mongocxx::collection& entity_collection,
                        const bsoncxx::oid& sourceEntityOid,
                        const bsoncxx::oid& targetEntityOid);
void handleEntitySwipe(mongocxx::collection& entity_collection,
                      mongocxx::collection& swipe_collection,
                      const bsoncxx::oid& sourceEntityOid,
                      const bsoncxx::oid& targetEntityOid,
                      bool isLike,
                      const std::string& madeField = "swipesMade",
                      const std::string& receivedField = "swipesReceived",
                      const std::string& matchesField = "matches",
                      const std::string& popularityField = "popularity");
bool swipeExists(mongocxx::collection& swipe_collection, const bsoncxx::oid& sourceEntityOid, const bsoncxx::oid& targetEntityOid);
// High-level API for main.cpp
crow::json::wvalue getRecommendedRoommates();
crow::json::wvalue getRecommendedRooms();
crow::json::wvalue processRoommateSwipe(const std::string& sourceId, const std::string& targetId, bool isLike);
crow::json::wvalue processRoomSwipe(const std::string& sourceId, const std::string& targetId, bool isLike);

int test(); // For testing purposes