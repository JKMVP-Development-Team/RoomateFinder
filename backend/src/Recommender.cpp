#include "Recommender.h"
#include "DBManager.h"
#include "Profile.h"

#include <crow/crow_all.h>
#include <bsoncxx/oid.hpp>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <regex>


using mongocxx::collection;


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
 * Calculates the document frequency for each token across all profiles.
 * This is used to compute the inverse document frequency (IDF) for TF-IDF.
 * @param profiles A vector of Profile objects containing user data.
 * @return A map where keys are tokens and values are their document frequencies.
 */
static std::unordered_map<std::string, int> documentFrequency(const std::vector<Profile>& profiles) {
    // SOURCE: https://www.learndatasci.com/glossary/tf-idf-term-frequency-inverse-document-frequency/#pdatablockkeyydefePythonImplementationp

    // Document Frequency: Increment for each unique token across all profiles
    // Term Frequency: Count of each token in the profile
    // Tokens that are currently accounted for are: city, state, country, zipcode, budget
    std::unordered_map<std::string, int> DF;
    for (const auto& profile : profiles) {
        std::unordered_set<std::string> seen;
        for (const auto& token : profile.tokens) {
            if (seen.insert(token).second) {
                DF[token]++;
            }
        }
    }
    return DF;
}

// Inverse Document Frequency: Calculating the proportion of documents in the profiles that contain each token
// IDF = log((N + 1) / (DF + 1)) + 1 to avoid division by zero and smoothing
static std::unordered_map<std::string, double> inverseDocumentFrequency(const std::unordered_map<std::string, int>& DF, size_t N_Docs) {
    std::unordered_map<std::string, double> IDF;
    for (const auto& [token, count] : DF) {
        IDF[token] = std::log((double)(N_Docs + 1) / (count + 1)) + 1; // Smoothing
    }
    return IDF;
}

static std::vector<std::unordered_map<std::string, double>> TF_IDF(const std::vector<Profile>& profiles) {
    size_t N_Docs = profiles.size();
    auto DF = documentFrequency(profiles);
    auto IDF = inverseDocumentFrequency(DF, N_Docs);

    // Calculate TF-IDF for each profile: Multiply term frequency by inverse document frequency
    std::vector<std::unordered_map<std::string, double>> V(N_Docs);
    for (size_t i = 0; i < N_Docs; ++i) {
        std::unordered_map<std::string, int> TF;

        // Calculate Term Frequency
        for (const auto& token : profiles[i].tokens) 
            TF[token]++;
        // Calculate TF-IDF
        for (const auto& [token, count] : TF) 
            V[i][token] = count * IDF[token];
    }
    return V;
}



// Normalizes a vector by dividing each element by the Euclidean norm of the vector.
static std::vector<double> normalizeVector(const std::unordered_map<std::string, double>& vec) {
    double sum = 0.0;
    for (const auto& [_, val] : vec) {
        sum += val * val;
    }
    double norm = std::sqrt(sum);
    std::vector<double> normalized;
    for (const auto& [_, val] : vec) {
        normalized.push_back(val / norm);
    }
    return normalized;
}

// Calculate cosine similarity Source: https://www.learndatasci.com/glossary/cosine-similarity
// There is no library for cosine similarity in C++ standard library, so we implement it manually
// Similarity = (A . B) / (||A|| * ||B||)
// where ||A|| and ||B|| are the Euclidean norms of vectors A and B, respectively.
// A . B is the dot product of vectors A and B
static std::vector<std::pair<double, int>> cosineSimilarity(
    const std::vector<std::unordered_map<std::string, double>>& V,
    const std::vector<double>& norms,
    int targetIndex) {
    
    size_t N_Docs = V.size();
    std::vector<std::pair<double, int>> similarities;

    for (int i = 0; i < N_Docs; ++i) {
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
    return similarities;
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

    auto norm = normalizeVector(TF_IDF(profiles));

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

    auto similarities = cosineSimilarity(V, norm, targetIndex);
    std::sort(similarities.begin(), similarities.end(), [](auto &a, auto &b){ return a.first > b.first; });

    crow::json::wvalue result;
    result["recommendations"] = crow::json::wvalue::list();
    for (size_t idx=0; idx<std::min(maxResults, similarities.size()); ++idx) {
        auto [score,i] = similarities[idx];
        crow::json::wvalue obj;
        obj["userId"]   = profiles[i].id;
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