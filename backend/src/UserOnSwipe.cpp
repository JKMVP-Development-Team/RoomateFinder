#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <crow.h/crow_all.h>
#include <cstdlib>
#include <iostream>

double calculatePopularity(int swipeReceived, int swipeGiven) {
    return static_cast<double>(swipesReceived) / std::max(swipeGiven, 1);
}

void updateUserPopularity(mongocxx::collection& collection, const std::string& userId) {

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;


    auto maybe_user = collection.find_one(document{} << "userId" << userId << finalize);

    if (!maybe_user) {
        std::cerr << "User not found: " << userId << std::endl;
        return;
    }  

    auto user_doc = maybe_user->view();

    int currentSwipesMade = user_doc["swipesMade"] ? user_doc["swipesMade"].get_int32().value : 0;
    int currentSwipesReceived = user_doc["swipesReceived"] ? user_doc["swipesReceived"].get_int32().value : 0;

    int newSwipesMade = currentSwipesMade + swipesMadeIncrement;
    int newSwipesReceived = currentSwipesReceived + swipesReceivedIncrement;
    double newPopularity = calculatePopularity(newSwipesReceived, newSwipesMade);

    auto update_doc = document{}
        << "$set" << open_document
        << "swipesMade" << newSwipesMade
        << "swipesReceived" << newSwipesReceived
        << "popularity" << newPopularity
        << close_document
        << finalize;

    usersCollection.update_one(document{} << "username" << username << finalize, update_doc);
}


int test()
{ 
  try
  {
    const char* path_value = std::getenv("backend/.env")
    ;
    if (path_value != nullptr) {
        std::cout << "PATH: " << path_value << std::endl;
    } else {
        std::cout << "PATH environment variable is not set." << std::endl;
    }

    mongocxx::instance inst{};
  
    const auto uri = mongocxx::uri{std::getenv("MONGODB_URI")};
  
    mongocxx::options::client client_options;
    const auto api = mongocxx::options::server_api{ mongocxx::options::server_api::version::k_version_1 };
    client_options.server_api_opts(api);
  
    // Setup the connection and get a handle on the "admin" database.
    mongocxx::client conn{ uri, client_options };
    mongocxx::database db = conn["admin"];
    
    // Ping the database.
    const auto ping_cmd = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("ping", 1));
    db.run_command(ping_cmd.view());
    std::cout << "Pinged your deployment. You successfully connected to MongoDB!" << std::endl;
  }
  catch (const std::exception& e) 
  {
    std::cout<< "Exception: " << e.what() << std::endl;
  }

  return 0;
}
