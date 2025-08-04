#pragma once

#include <mongocxx/collection.hpp>
#include <bsoncxx/oid.hpp>
#include <crow/crow_all.h>
#include <string>

// Business logic
double calculatePopularity(int received, int made, int matches, double budget);
void updateEntityPopularity(mongocxx::collection& entity_collection, const bsoncxx::oid& entityOid);
void updateEntityMatches(mongocxx::collection& source_collection, 
                        mongocxx::collection& target_collection,
                        const bsoncxx::oid& sourceEntityOid,
                        const bsoncxx::oid& targetEntityOid);

void handleEntityLike(mongocxx::collection& source_collection,
                      mongocxx::collection& target_collection,
                      mongocxx::collection& swipe_collection,
                      const bsoncxx::oid& sourceEntityOid,
                      const bsoncxx::oid& targetEntityOid);

void handleEntitySwipe(mongocxx::collection& source_collection,
                        mongocxx::collection& swipe_collection,
                        const bsoncxx::oid& sourceEntityOid,
                        const bsoncxx::oid& targetEntityOid);

bool swipeExists(mongocxx::collection& swipe_collection, const bsoncxx::oid& sourceEntityOid, const bsoncxx::oid& targetEntityOid);
double normalizeBudget(double budget);
double normalizePopularity(double popularity, double minPopularity = 0.0, double maxPopularity = 100.0);
// High-level API for main.cpp
crow::json::wvalue getRecommendations(const std::string& currentUserId, const std::string& type);
crow::json::wvalue getUsersWhoLikedRoom(const std::string& roomId);
crow::json::wvalue getUsersWhoLiked(const std::string& userId);
crow::json::wvalue processSwipe(const std::string& sourceId, const std::string& targetId, const std::string& type, bool isLike);
