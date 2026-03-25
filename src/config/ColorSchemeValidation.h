#pragma once
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// Validates scheme JSON structure and value ranges.
// Returns true if valid, otherwise fills errors.
bool validateColorSchemeJson(const nlohmann::json &root,
                             std::vector<std::string> &errors);
