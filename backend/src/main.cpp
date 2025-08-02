#include "crow/crow_all.h"
#include "Matcher.h"

int main() {
    crow::SimpleApp app;


    CROW_ROUTE(app, "/")([](){
        return crow::response("Roommate Finder Backend is running.");
    });

    // Get recommended roommates for a user
    CROW_ROUTE(app, "/api/roommate/recommend").methods("GET"_method)
    ([](const crow::request& req){
        auto userId = req.url_params.get("userId");
        if (!userId) return crow::response(400, "Missing userId parameter.");
        return crow::response(getRecommendedRoommates(userId));
    });

    // Get recommended rooms for a user
    CROW_ROUTE(app, "/api/room/recommend").methods("GET"_method)
    ([](const crow::request& req){
        auto userId = req.url_params.get("userId");
        if (!userId) return crow::response(400, "Missing userId parameter.");
        return crow::response(getRecommendedRooms(userId));
    });

    CROW_ROUTE(app, "/api/swipe").methods("POST"_method)
    ([](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "Invalid JSON.");

        std::string type = body["type"].s();
        std::string sourceId = body["sourceId"].s();
        std::string targetId = body["targetId"].s();
        bool isLike = body["isLike"].b();
        
        if (type == "roommate") {
            return crow::response(processRoommateSwipe(sourceId, targetId, isLike));
        } else if (type == "room") {
            return crow::response(processRoomSwipe(sourceId, targetId, isLike));
        } else {
            return crow::response(400, "Invalid type parameter.");
        }
    });

    // Get users who liked the room
    CROW_ROUTE(app, "/api/room/likes").methods("GET"_method)
    ([](const crow::request& req){
        auto roomId = req.url_params.get("roomId");
        if (!roomId) return crow::response(400, "Missing roomId parameter.");
        return crow::response(getUsersWhoLikedRoom(roomId));
    });

    // Get users who liked the roommate
    CROW_ROUTE(app, "/api/user/liked-rooms").methods("GET"_method)
    ([](const crow::request& req){
        auto userId = req.url_params.get("userId");
        if (!userId) return crow::response(400, "Missing userId parameter.");
        return crow::response(getUsersWhoLiked(userId));
    });

    std::cout << "🟢 Backend starting on 0.0.0.0:18080\n";

    // test();

    app.bindaddr("0.0.0.0").port(18080).multithreaded().run();
}
