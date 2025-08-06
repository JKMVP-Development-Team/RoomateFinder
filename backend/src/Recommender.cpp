#include "Recommender.h"
#include "DBManager.h"

#include <crow/crow_all.h>
#include <bsoncxx/oid.hpp>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <regex>


using mongocxx::collection;

struct Profile {
    std::string id, username, city, country, budget;
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


static std::vector<Profile> fetchUserData() {
    try{
        auto& db = getDbManager();
        auto user_collection = db.getUserCollection();
        std::vector<Profile> profiles;

        for (auto&& doc : user_collection.find({})) {
            Profile profile;
            profile.id = doc["_id"].get_oid().value.to_string();
            profile.username = std::string(doc["username"].get_string().value);
            profile.city = std::string(doc["city"].get_string().value);
            profile.country = std::string(doc["country"].get_string().value);
            profile.budget = std::string(doc["budget"].get_string().value);

            // TODO - Cap the number of tokens to more recent ones using timestamps
            std::string all = profile.username + " " + profile.city;
            profile.tokens = tokenize(all);
            profiles.push_back(std::move(profile));
        }
        return profiles;
    } catch (const std::exception& e) {
        std::cerr << "Error fetching user data: " << e.what() << std::endl;
        throw std::runtime_error("Failed to fetch user data");
    }
}

/**
 * Rank Users Based on Cosine Similarity between Their Profiles
 * @param profiles List of user profiles to rank
 * @return A JSON object containing ranked user profiles
 */
crow::json::wvalue rankUsers(const std::string& targetId,
                             const std::string& type,
                             size_t maxResults) {
    // As of now, the recommender system only supports roommate type
    if (type != "roommate") {
        throw std::invalid_argument("Invalid type, expected 'roommate'");
    }

    auto profiles = fetchUserData();
    size_t N = profiles.size();

    // Document Frequency
    std::unordered_map<std::string, int> DF;
    for (const auto& profile : profiles) {  
        std::unordered_set<std::string> seen;
        for (auto& token : profile.tokens) {
            if (seen.insert(token).second) {
                DF[token]++;
            }
        }
    }

    // Inverse Document Frequency
    std::unordered_map<std::string, double> IDF;
    for (const auto& [token, count] : DF) {
        IDF[token] = std::log((double)(N + 1) / (count + 1)) + 1;
    }

    // Calculate TF-IDF for each profile
    std::vector<std::unordered_map<std::string, double>> V(N);
    for (size_t i = 0; i < N; ++i) {
        std::unordered_map<std::string, int> TF;

        // Calculate Term Frequency
        for (auto& token : profiles[i].tokens) 
            TF[token]++;
        // Calculate TF-IDF
        for (auto& [token, count] : TF) 
            V[i][token] = count * IDF[token];
    }

    // Nomalize vectors
    std::vector<double> norms(N, 0.0);
    for (size_t i=0; i<N; ++i) {
        double sum = 0;
        for (auto& [w,val] : V[i]) sum += val*val;
        norms[i] = std::sqrt(sum);
    }

    // Target Idex
    int targetIndex = -1;
    for (int i = 0; i < N; ++i) {
        if (profiles[i].id == targetId) {
            targetIndex = i;
            break;
        }
    }

    if (targetIndex == -1) {
        throw std::invalid_argument("Target user not found");
    }

    // Calculate cosine similarity
    std::vector<std::pair<double, int>> similarities;
    for (int i = 0; i < N; ++i) {
        if (i == targetIndex) continue;

        double dotProduct = 0.0;
        for (const auto& [token, val] : V[targetIndex]) {
            auto it = V[i].find(token);
            if (it != V[i].end())
                dotProduct += val * it->second;
        }

        double similarity = (norms[targetIndex] && norms[i])
                     ? dotProduct/(norms[targetIndex]*norms[i])
                     : 0.0;
        similarities.emplace_back(similarity, i);
    }

    std::sort(similarities.begin(), similarities.end(), [](auto &a, auto &b){ return a.first > b.first; });

    crow::json::wvalue result;
    result["recommendations"] = crow::json::wvalue::list();
    for (size_t idx=0; idx<std::min(maxResults, similarities.size()); ++idx) {
        auto [score,i] = similarities[idx];
        crow::json::wvalue obj;
        obj["userId"]   = profiles[i].id;
        obj["username"] = profiles[i].username;
        obj["city"]     = profiles[i].city;
        obj["score"]    = score;
        result["recommendations"][idx] = std::move(obj);
    }

    return result;
}