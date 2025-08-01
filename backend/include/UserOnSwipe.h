#pragma once

#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <crow/crow_all.h>
#include <cstdlib>
#include <iostream>
#include <chrono>


double calculatePopularity(int swipeReceived, int swipeGiven);
void updateUserPopularity(mongocxx::collection& collection, const std::string& userId, int swipesReceivedIncrement = 0);
void handleUserSwipe(mongocxx::database& db, const std::string& sourceUserId, const std::string& targetUserId, bool isLike);
int test();