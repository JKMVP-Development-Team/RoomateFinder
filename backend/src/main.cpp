#include "crow/crow_all.h"
#include "Matcher.h"

int main() {
    crow::SimpleApp app;


    CROW_ROUTE(app, "/")([](){
        return crow::response("Roommate Finder Backend is running.");
    });

    CROW_ROUTE(app, "/api/recommend").methods("GET"_method)
    ([](const crow::request& req){
        auto type = req.url_params.get("type");
        if (type && std::string(type) == "roommate") {
            return crow::response(getRecommendedRoommates());
        } else if (type && std::string(type) == "room") {
            return crow::response(getRecommendedRooms());
        } else {
            return crow::response(400, "Invalid type parameter.");
        }
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


    std::cout << "🟢 Backend starting on 0.0.0.0:18080\n";

    // test();

    app.bindaddr("0.0.0.0").port(18080).multithreaded().run();
}
