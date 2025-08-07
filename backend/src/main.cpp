#include "crow/crow_all.h"
#include "Matcher.h"

int main() {
    crow::SimpleApp app;


    CROW_ROUTE(app, "/")([](){
        return crow::response("Roommate Finder Backend is running.");
    });

    // Get recommended roommates for a user
    CROW_ROUTE(app, "/api/recommend").methods("GET"_method)
    ([](const crow::request& req){
        auto type = req.url_params.get("type");
        if (!type) return crow::response(400, "Missing type parameter.");
        auto userId = req.url_params.get("userId");
        if (!userId) return crow::response(400, "Missing userId parameter.");

        return crow::response(getRecommendations(userId, type));
    });

    CROW_ROUTE(app, "/api/swipe").methods("POST"_method)
    ([](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "Invalid JSON.");

        std::string type = body["type"].s();
        std::string sourceId = body["sourceId"].s();
        std::string targetId = body["targetId"].s();
        bool isLike = body["isLike"].b();
        return crow::response(processSwipe(sourceId, targetId, type, isLike));
    });

    CROW_ROUTE(app, "/api/likes").methods("GET"_method)
    ([](const crow::request& req){
        auto id = req.url_params.get("id");
        auto type = req.url_params.get("type");
        if (!id || !type) return crow::response(400, "Missing id or type parameter.");
        return crow::response(getUserWhoLikedEntity(id, type));
    });

    std::cout << "🟢 Backend starting on 0.0.0.0:18080\n";

    // test();

    app.bindaddr("0.0.0.0").port(18080).multithreaded().run();
}
