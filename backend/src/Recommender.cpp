#include "Recommender.h"
#include "DBManager.h"

#include <bsoncxx/oid.hpp>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <regex>


using mongocxx::collection;

struct Profile {
    std::string id, username, city; 
    std::vector<std::string> tokens; 
    // std::unordered_map<std::string, double> tfidf;
    // double popularity = 0.0;
};

/**
 * Tokenizes a string into words, converting them to lowercase.
 * @param text The input string to tokenize.
 */
static std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> out;
    std::regex re(R"(\w+)");
    for (auto it = std::sregex_iterator(text.begin(), text.end(), re);
         it != std::sregex_iterator(); ++it) {
        std::string w = it->str();
        std::transform(w.begin(), w.end(), w.begin(), ::tolower);
        out.push_back(w);
    }
    return out;
}


std::vector<Profile> fetchUserData() {
    auto& db = getDbManager();
    auto user_collection = db.getUserCollection();
    std::vector<Profile> profiles;

    for (auto&& doc : user_collection.find({})) {
        Profile profile;
        profile.id = doc["_id"].get_oid().value.to_string();
        profile.username = doc["username"].get_string().value;
        profile.city = doc["city"].get_string().value;
        
        // TODO - Cap the number of tokens to more recent ones using timestamps
        std::string all = p.username + " " + p.city;
        profile.tokens = tokenize(all);
        profiles.push_back(std::move(profile));
    }
}

/**
 * Rank Users Based on Cosine Similarity between Their Profiles
 * @param profiles List of user profiles to rank
 * @return A JSON object containing ranked user profiles
 */
crow::json::wvalue rankUsers(const std::vector<Profile>& profiles) {
    crow::json::wvalue result;
    result["users"] = crow::json::wvalue::list();

    // Calculate TF-IDF for each profile
    std::unordered_map<std::string, double> idf;
    for (const auto& profile : profiles) {
        for (const auto& token : profile.tokens) {
            idf[token]++;
        }
    }
    for (auto& [token, count] : idf) {
        idf[token] = log(profiles.size() / count);
    }

    // Calculate cosine similarity
    for (const auto& profile : profiles) {
        double norm = 0.0;
        for (const auto& token : profile.tokens) {
            norm += idf[token] * idf[token];
        }
        norm = sqrt(norm);

        crow::json::wvalue user;
        user["id"] = profile.id;
        user["username"] = profile.username;
        user["city"] = profile.city;
        user["similarity"] = norm; // Placeholder for actual similarity score
        result["users"].emplace_back(std::move(user));
    }

    return result;
}