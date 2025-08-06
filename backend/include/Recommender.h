#pragma once

#include <string>
#include <crow/json.h>

/// Returns up to `maxResults` roommate‐to‐roommate recommendations
/// based on TF-IDF + cosine similarity of user profiles.
crow::json::wvalue getUserRecommendations(const std::string& userId,
                                      const std::string& type,
                                      size_t maxResults = 5);