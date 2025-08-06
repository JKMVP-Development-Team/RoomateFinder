#include <mongocxx/collection.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/oid.hpp>
#include <crow/crow_all.h>
#include <iostream>
#include <string>
using namespace bsoncxx::builder::stream;
using bsoncxx::oid;

class ParseUser {
private:
    mongocxx::collection& user_collection;

public:
    explicit ParseUser(mongocxx::collection& collection) : user_collection(collection) {}

    crow::json::wvalue parse(const std::string& userId) {
        crow::json::wvalue result;

        try {
            auto user_doc = user_collection.find_one(
                document{} << "_id" << oid(userId) << finalize
            );

            if (!user_doc) {
                result["error"] = "User not found.";
                return result;
            }

            auto user_view = user_doc->view();
            result["id"] = userId;
            result["username"] =       user_view["username"]       ? std::string(user_view["username"].get_string().value) : "";
            result["email"] =          user_view["email"]          ? std::string(user_view["email"].get_string().value) : "";
            result["phone"] =          user_view["phone"]          ? std::string(user_view["phone"].get_string().value) : "";
            result["gender"] =         user_view["gender"]         ? std::string(user_view["gender"].get_string().value) : "";
            result["address"] =        user_view["address"]        ? std::string(user_view["address"].get_string().value) : "";
            result["address_line"] =   user_view["address_line"]   ? std::string(user_view["address_line"].get_string().value) : "";
            result["zipcode"] =        user_view["zipcode"]        ? std::string(user_view["zipcode"].get_string().value) : "";
            result["city"] =           user_view["city"]           ? std::string(user_view["city"].get_string().value) : "";
            result["country"] =        user_view["country"]        ? std::string(user_view["country"].get_string().value) : "";
            result["budget"] =         user_view["budget"]         ? std::string(user_view["budget"].get_string().value) : "0.0";
            result["popularity"] =     user_view["popularity"]     ? user_view["popularity"].get_double().value : 0.0;
            result["matches"] =        user_view["matches"]        ? user_view["matches"].get_int32().value : 0;
            result["swipesMade"] =     user_view["swipesMade"]     ? user_view["swipesMade"].get_int32().value : 0;
            result["swipesReceived"] = user_view["swipesReceived"] ? user_view["swipesReceived"].get_int32().value : 0;

        } catch (const std::exception& e) {
            std::cerr << "Error fetching user info: " << e.what() << std::endl;
            result["error"] = "Backend error: " + std::string(e.what());
        }

        return result;
    }

};
