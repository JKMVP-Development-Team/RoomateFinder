#pragma once

#include <string>
#include <crow/crow_all.h>

/// Returns up to `maxResults` roommate‐to‐roommate recommendations
/// based on TF-IDF + cosine similarity of user profiles.
crow::json::wvalue rankUsers(const std::string& targetId,
                             const std::string& type,
                             size_t maxResults = 5);