#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace back2you {

struct Notification {
    int id = 0;
    int user_id = 0;
    std::string title;
    std::string message;
    std::string type; // "MATCH", "CLAIM", "SYSTEM"
    bool is_read = false;
    int item_id = 0;
    std::string created_at;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"user_id", user_id},
            {"title", title},
            {"message", message},
            {"type", type},
            {"is_read", is_read},
            {"item_id", item_id},
            {"created_at", created_at}
        };
    }
};

} // namespace back2you
