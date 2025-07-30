#include "crow/crow_all.h"

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([](){
        return "Roommate Finder Backend is running.";
    });

    std::cout << "🟢 Backend starting on 0.0.0.0:18080\n";

    app.bindaddr("0.0.0.0").port(18080).multithreaded().run();
}
