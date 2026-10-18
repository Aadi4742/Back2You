#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace back2you {

struct Category {
    int id = 0;
    std::string name;
    std::string description;
    std::string icon;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"name", name},
            {"description", description},
            {"icon", icon}
        };
    }
};

} // namespace back2you
