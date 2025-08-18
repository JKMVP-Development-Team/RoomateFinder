#include "crow/crow_all.h"
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <iostream>

int main() {
    try {
        // Initialize local MongoDB connection 
        mongocxx::instance inst{};
        const auto uri = mongocxx::uri{"mongodb://localhost:27017"};
        mongocxx::client conn{uri};
        auto db = conn["roommate_finder"];
        auto profiles = db["profiles"];

        // Test connection
        try {
            auto result = db.run_command(bsoncxx::builder::basic::make_document(
                bsoncxx::builder::basic::kvp("ping", 1)));
            std::cout << "Successfully connected to local MongoDB!" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Failed to connect to MongoDB: " << e.what() << std::endl;
            return 1;
        }

        crow::SimpleApp app;

        // Helper functions for JSON field extraction
        auto getStringField = [](const crow::json::rvalue& json, const std::string& field, const std::string& defaultVal) {
            return json.has(field) ? std::string(json[field].s()) : defaultVal;
        };

        auto getIntField = [](const crow::json::rvalue& json, const std::string& field, int defaultVal) {
            return json.has(field) ? json[field].i() : defaultVal;
        };

        auto getBoolField = [](const crow::json::rvalue& json, const std::string& field, bool defaultVal) {
            return json.has(field) ? json[field].b() : defaultVal;
        };

        // Helper function to create profile document from JSON
        auto createProfileDocument = [&](const crow::json::rvalue& json) {
            using bsoncxx::builder::stream::document;
            using bsoncxx::builder::stream::array;
            using bsoncxx::builder::stream::open_array;
            using bsoncxx::builder::stream::close_array;

            document doc{};
            doc << "username" << getStringField(json, "username", "")
                << "location" << getStringField(json, "location", "")
                << "bio" << getStringField(json, "bio", "")
                << "budget" << open_array
                    << (json.has("budget") ? json["budget"][0].i() : 500)
                    << (json.has("budget") ? json["budget"][1].i() : 1200)
                << close_array
                << "cleanliness" << getIntField(json, "cleanliness", 3)
                << "socialLevel" << getIntField(json, "socialLevel", 3)
                << "noiseTolerance" << getIntField(json, "noiseTolerance", 3)
                << "smoking" << getStringField(json, "smoking", "non-smoker")
                << "hasPets" << getBoolField(json, "hasPets", false)
                << "petFriendly" << getBoolField(json, "petFriendly", false)
                << "workSchedule" << getStringField(json, "workSchedule", "9-5")
                << "createdAt" << bsoncxx::types::b_date{std::chrono::system_clock::now()};
            
            return doc;
        };

        CROW_ROUTE(app, "/")([](){
            return crow::response("Roommate Finder Backend is running.");
        });

        // Create profile endpoint
        CROW_ROUTE(app, "/api/profiles").methods("POST"_method)
        ([&profiles, &createProfileDocument](const crow::request& req) {
            try {
                auto json = crow::json::load(req.body);
                
                if (!json) {
                    return crow::response(400, "Invalid JSON");
                }

                // Validate required fields
                if (!json.has("username") || !json.has("location")) {
                    return crow::response(400, R"({"error": "Username and location are required"})");
                }

                // Create document using helper function
                auto doc = createProfileDocument(json);
                auto result = profiles.insert_one(doc.view());
                
                if (result) {
                    std::string response = R"({"success": true, "message": "Profile created successfully", "id": ")" + 
                                          result->inserted_id().get_oid().value.to_string() + R"("})";
                    return crow::response(201, response);
                } else {
                    return crow::response(500, R"({"error": "Failed to create profile"})");
                }

            } catch (const std::exception& e) {
                std::cout << "Error creating profile: " << e.what() << std::endl;
                return crow::response(500, R"({"error": "Internal server error"})");
            }
        });

        // Get all profiles endpoint
        CROW_ROUTE(app, "/api/profiles").methods("GET"_method)
        ([&profiles](const crow::request& req) {
            try {
                auto cursor = profiles.find({});
                
                std::string json_response = R"({"profiles": [)";
                bool first = true;
                
                for (auto&& doc : cursor) {
                    if (!first) json_response += ",";
                    first = false;
                    
                    json_response += R"({"id": ")" + doc["_id"].get_oid().value.to_string() + R"(",)";
                    json_response += R"("username": ")" + std::string(doc["username"].get_string().value) + R"(",)";
                    json_response += R"("location": ")" + std::string(doc["location"].get_string().value) + R"(",)";
                    
                    if (doc["bio"]) {
                        json_response += R"("bio": ")" + std::string(doc["bio"].get_string().value) + R"(",)";
                    }
                    
                    if (doc["budget"]) {
                        auto budget_array = doc["budget"].get_array().value;
                        json_response += R"("budget": [)";
                        
                        // Handle different number types safely
                        auto first_elem = budget_array[0];
                        auto second_elem = budget_array[1];
                        
                        if (first_elem.type() == bsoncxx::type::k_int32) {
                            json_response += std::to_string(first_elem.get_int32().value);
                        } else if (first_elem.type() == bsoncxx::type::k_int64) {
                            json_response += std::to_string(first_elem.get_int64().value);
                        } else if (first_elem.type() == bsoncxx::type::k_double) {
                            json_response += std::to_string((int)first_elem.get_double().value);
                        } else {
                            json_response += "500"; // default
                        }
                        
                        json_response += ",";
                        
                        if (second_elem.type() == bsoncxx::type::k_int32) {
                            json_response += std::to_string(second_elem.get_int32().value);
                        } else if (second_elem.type() == bsoncxx::type::k_int64) {
                            json_response += std::to_string(second_elem.get_int64().value);
                        } else if (second_elem.type() == bsoncxx::type::k_double) {
                            json_response += std::to_string((int)second_elem.get_double().value);
                        } else {
                            json_response += "1200"; // default
                        }
                        
                        json_response += "],";
                    }
                    
                    // Handle integer fields safely
                    auto get_int_value = [](const bsoncxx::document::element& elem, int default_val) -> int {
                        if (!elem) return default_val;
                        if (elem.type() == bsoncxx::type::k_int32) return elem.get_int32().value;
                        if (elem.type() == bsoncxx::type::k_int64) return (int)elem.get_int64().value;
                        if (elem.type() == bsoncxx::type::k_double) return (int)elem.get_double().value;
                        return default_val;
                    };
                    
                    json_response += R"("cleanliness": )" + std::to_string(get_int_value(doc["cleanliness"], 3)) + ",";
                    json_response += R"("socialLevel": )" + std::to_string(get_int_value(doc["socialLevel"], 3)) + ",";
                    json_response += R"("noiseTolerance": )" + std::to_string(get_int_value(doc["noiseTolerance"], 3)) + ",";
                    json_response += R"("smoking": ")" + (doc["smoking"] ? std::string(doc["smoking"].get_string().value) : "non-smoker") + R"(",)";
                    json_response += R"("hasPets": )" + (doc["hasPets"] ? (doc["hasPets"].get_bool().value ? std::string("true") : std::string("false")) : std::string("false")) + ",";
                    json_response += R"("petFriendly": )" + (doc["petFriendly"] ? (doc["petFriendly"].get_bool().value ? std::string("true") : std::string("false")) : std::string("false")) + ",";
                    json_response += R"("workSchedule": ")" + (doc["workSchedule"] ? std::string(doc["workSchedule"].get_string().value) : "9-5") + R"(")";
                    
                    json_response += "}";
                }
                
                json_response += "]}";
                
                return crow::response(200, json_response);
                
            } catch (const std::exception& e) {
                std::cout << "Error fetching profiles: " << e.what() << std::endl;
                return crow::response(500, R"({"error": "Internal server error"})");
            }
        });

        // Delete all profiles endpoint
        CROW_ROUTE(app, "/api/profiles").methods("DELETE"_method)
        ([&profiles](const crow::request& req) {
            try {
                auto result = profiles.delete_many({});
                
                std::string response = R"({"success": true, "message": "All profiles deleted successfully", "deletedCount": )" + 
                                      std::to_string(result->deleted_count()) + "}";
                
                std::cout << "Deleted " << result->deleted_count() << " profiles" << std::endl;
                return crow::response(200, response);
                
            } catch (const std::exception& e) {
                std::cout << "Error deleting profiles: " << e.what() << std::endl;
                return crow::response(500, R"({"error": "Internal server error"})");
            }
        });

        std::cout << "Backend starting on 0.0.0.0:18080\n";
        std::cout << "MongoDB connection: mongodb://localhost:27017" << std::endl;
        std::cout << "Database: roommate_finder" << std::endl;
        std::cout << "Collection: profiles" << std::endl;

        app.bindaddr("0.0.0.0").port(18080).multithreaded().run();

    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
