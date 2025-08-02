#pragma once

#include <mongocxx/collection.hpp>
#include <bsoncxx/oid.hpp>
#include <crow/crow_all.h>
#include <string>

// Business logic
double calculatePopularity(int received, int made, int matches, double budget, double proximity);
void updateEntityPopularity(mongocxx::collection& entity_collection, const bsoncxx::oid& entityOid, double proximity = 0.0);
void updateEntityMatches(mongocxx::collection& entity_collection,
                        const bsoncxx::oid& sourceEntityOid,
                        const bsoncxx::oid& targetEntityOid);
void handleEntitySwipe(mongocxx::collection& source_collection,
                      mongocxx::collection& target_collection,
                      mongocxx::collection& swipe_collection,
                      const bsoncxx::oid& sourceEntityOid,
                      const bsoncxx::oid& targetEntityOid,
                      bool isLike);
bool swipeExists(mongocxx::collection& swipe_collection, const bsoncxx::oid& sourceEntityOid, const bsoncxx::oid& targetEntityOid);
double normalizeBudget(double budget);
double normalizePopularity(double popularity, double minPopularity = 0.0, double maxPopularity = 100.0);
// High-level API for main.cpp
crow::json::wvalue getRecommendedRoommates(const std::string& currentUserId);
crow::json::wvalue getRecommendedRooms(const std::string& currentUserId);
crow::json::wvalue getUsersWhoLikedRoom(const std::string& roomId);
crow::json::wvalue processRoommateSwipe(const std::string& sourceId, const std::string& targetId, bool isLike);
crow::json::wvalue processRoomSwipe(const std::string& sourceId, const std::string& targetId, bool isLike);
