#pragma once

#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>
#include <string>

double calculatePopularity(int swipeReceived, int swipeGiven);
void updateUserPopularity(mongocxx::collection& collection, const std::string& userId, int swipesReceivedIncrement = 0);
void handleUserSwipe(mongocxx::database& db, const std::string& sourceUserId, const std::string& targetUserId, bool isLike);
int test();