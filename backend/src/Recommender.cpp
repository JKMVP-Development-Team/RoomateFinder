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
    std::string id, username, city, country, budget, state, zipcode;
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

/**
 * Fetches user data from the database and tokenizes it for the recommender system.
 * @return A vector of Profile objects containing user data.
 */
static std::vector<Profile> fetchUserData() {
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

/**
 * Ranks users based on their similarity to a target user using TF-IDF and cosine similarity.
 * @param targetId The ID of the target user.
 * @param type The type of recommendation (currently only "roommate" is supported).
 * @param maxResults The maximum number of results to return.
 * @return A JSON object containing ranked user recommendations.
 */
crow::json::wvalue rankUsers(const std::string& targetId,
                             const std::string& type,
                             size_t maxResults) {
    // As of now, the recommender system only supports roommate type
    if (type != "roommate") {
        throw std::invalid_argument("Invalid type, expected 'roommate'");
    }

    auto profiles = fetchUserData();
    size_t N_Docs = profiles.size();

    // SOURCE: https://www.learndatasci.com/glossary/tf-idf-term-frequency-inverse-document-frequency/#pdatablockkeyydefePythonImplementationp

    // Document Frequency: Increment for each unique token across all profiles
    // Term Frequency: Count of each token in the profile
    // Tokens that are currently accounted for are: username, city, state, country, zipcode, budget
    std::unordered_map<std::string, int> DF;
    for (const auto& profile : profiles) {  
        std::unordered_set<std::string> seen;
        for (auto& token : profile.tokens) {
            if (seen.insert(token).second) {
                DF[token]++;
            }
        }
    }

    // Inverse Document Frequency: Calculating the proportion of documents in the profiles that contain each token
    // IDF = log((N + 1) / (DF + 1)) + 1 to avoid division by zero and smoothing
    std::unordered_map<std::string, double> IDF;
    for (const auto& [token, count] : DF) {
        IDF[token] = std::log((double)(N_Docs + 1) / (count + 1)) + 1;
    }

    // Calculate TF-IDF for each profile: Multiply term frequency by inverse document frequency
    // Importance of a term for 
    std::vector<std::unordered_map<std::string, double>> V(N_Docs);
    for (size_t i = 0; i < N_Docs; ++i) {
        std::unordered_map<std::string, int> TF;

        // Calculate Term Frequency
        for (auto& token : profiles[i].tokens) 
            TF[token]++;
        // Calculate TF-IDF
        for (auto& [token, count] : TF) 
            V[i][token] = count * IDF[token];
    }

    // Nomalize vectors using Euclidean norm 
    std::vector<double> norms(N_Docs, 0.0);
    for (size_t i=0; i<N_Docs; ++i) {
        double sum = 0;
        for (auto& [w,val] : V[i]) sum += val*val;
        norms[i] = std::sqrt(sum);
    }

    // Target Idex
    int targetIndex = -1;
    for (int i = 0; i < N_Docs; ++i) {
        if (profiles[i].id == targetId) {
            targetIndex = i;
            break;
        }
    }

    if (targetIndex == -1) {
        throw std::invalid_argument("Target user not found");
    }

    // Calculate cosine similarity Source: https://www.learndatasci.com/glossary/cosine-similarity
    // There is no library for cosine similarity in C++ standard library, so we implement it manually
    // Similarity = (A . B) / (||A|| * ||B||)
    // where ||A|| and ||B|| are the Euclidean norms of vectors A and B, respectively.
    // A . B is the dot product of vectors A and B
    std::vector<std::pair<double, int>> similarities;
    for (int i = 0; i < N_Docs; ++i) {
        if (i == targetIndex) continue;

        double dotProduct = 0.0;
        for (const auto& [token, val] : V[targetIndex]) {
            // Try to find the same token in the i-th user’s TF-IDF map
            auto it = V[i].find(token);
            if (it != V[i].end())
                // If present, multiply the two weights and add to the running sum
                dotProduct += val * it->second;
        }

        // Calculate cosine similarity if both norms are non-zero
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
        obj["state"]    = profiles[i].state;
        obj["country"]  = profiles[i].country;
        obj["zipcode"]  = profiles[i].zipcode;
        obj["budget"]   = profiles[i].budget;
        obj["score"]    = score;
        result["recommendations"][idx] = std::move(obj);

        // TODO - Add more user information like preferences, interests, etc.
        // TODO - Sort by popularity as well
    }

    return result;
}