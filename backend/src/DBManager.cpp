#include "DBManager.h"
#include "Profile.h"    
#include <mongocxx/instance.hpp>

DBManager::DBManager(const std::string& mongo_uri)
    : client_(mongocxx::uri{mongo_uri}) {
    db = client_["roommatefinder"];
}

mongocxx::collection DBManager::getUserCollection() { return db["users"]; }
mongocxx::collection DBManager::getRoomCollection() { return db["rooms"]; }
mongocxx::collection DBManager::getUserSwipeCollection() { return db["user_swipes"]; }
mongocxx::collection DBManager::getRoomSwipeCollection() { return db["room_swipes"]; }

DBManager& getDbManager() {
    static mongocxx::instance inst{};
    static DBManager dbManager(getenv("MONGODB_URI") ? getenv("MONGODB_URI") : "mongodb://localhost:27017");
    return dbManager;
}

crow::json::wvalue fetchUserInfo(const std::string& userId) {
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
        result["state"] =          user_view["state"]          ? std::string(user_view["state"].get_string().value) : "";
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


/**
 * Fetches user data from the database and tokenizes it for the recommender system.
 * @return A vector of Profile objects containing user data.
 */
std::vector<Profile> fetchUserData() {
    try{
        auto& db = getDbManager();
        auto user_collection = db.getUserCollection();
        std::vector<Profile> profiles;

        for (auto&& doc : user_collection.find({})) {
            Profile profile;
            profile.id = doc["_id"].get_oid().value.to_string();

            profile.city =      doc["city"]     ? std::string(doc["city"].get_string().value) : "";
            profile.state =     doc["state"]    ? std::string(doc["state"].get_string().value) : "";
            profile.country =   doc["country"]  ? std::string(doc["country"].get_string().value) : "";
            profile.zipcode =   doc["zipcode"]  ? std::string(doc["zipcode"].get_string().value) : "";
            profile.budget =    doc["budget"]   ? std::string(doc["budget"].get_string().value) : "";
            
            // TODO - Cap the number of tokens to more recent ones using timestamps
            // TODO - Add preferences and interests to the recommender system
            std::string all = profile.city + " " +
                              profile.state + " " +
                              profile.country + " " +
                              profile.zipcode + " " +
                              profile.budget;
            profile.tokens = tokenize(all);
            profiles.push_back(std::move(profile));
        }
        return profiles;
    } catch (const std::exception& e) {
        std::cerr << "Error fetching user data: " << e.what() << std::endl;
        throw std::runtime_error("Failed to fetch user data");
    }
}