#pragma once

#include <mongocxx/collection.hpp>
#include <bsoncxx/oid.hpp>
#include <crow/crow_all.h>
#include <string>

// These function aren't needed in the header as they are only used in the .cpp. They will be declared as static functions in the .cpp file.
// They are not part of the public API and should not be exposed in the header file.
// void updateEntityPopularity(mongocxx::collection& entity_collection, const bsoncxx::oid& entityOid);
// void updateEntityMatches(mongocxx::collection& source_collection, 
//                         mongocxx::collection& target_collection,
//                         const bsoncxx::oid& sourceEntityOid,
//                         const bsoncxx::oid& targetEntityOid);

// void handleEntityLike(mongocxx::collection& source_collection,
//                       mongocxx::collection& target_collection,
//                       mongocxx::collection& swipe_collection,
//                       const bsoncxx::oid& sourceEntityOid,
//                       const bsoncxx::oid& targetEntityOid);

// void handleEntitySwipe(mongocxx::collection& source_collection,
//                         mongocxx::collection& swipe_collection,
//                         const bsoncxx::oid& sourceEntityOid,
//                         const bsoncxx::oid& targetEntityOid);

// bool swipeExists(mongocxx::collection& swipe_collection, const bsoncxx::oid& sourceEntityOid, const bsoncxx::oid& targetEntityOid);
// High-level API for main.cpp
crow::json::wvalue getRecommendations(const std::string& currentUserId, const std::string& type);
crow::json::wvalue getUserWhoLikedEntity(const std::string& entityId, const std::string& type);
crow::json::wvalue processSwipe(const std::string& sourceId, const std::string& targetId, const std::string& type, bool isLike);
