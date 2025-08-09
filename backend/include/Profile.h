#pragma once

#include <string>
#include <vector>




struct Profile {
    std::string id, city, country, budget, state, zipcode;
    std::vector<std::string> tokens;
};