#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace back2you {

enum class ItemType {
    Lost,
    Found
};

inline std::string itemTypeToString(ItemType type) {
    return type == ItemType::Found ? "FOUND" : "LOST";
}

inline ItemType stringToItemType(const std::string& str) {
    if (str == "FOUND" || str == "found") return ItemType::Found;
    return ItemType::Lost;
}

enum class ItemStatus {
    Lost,
    Found,
    ClaimPending,
    Returned
};

inline std::string itemStatusToString(ItemStatus status) {
    switch (status) {
        case ItemStatus::Lost: return "LOST";
        case ItemStatus::Found: return "FOUND";
        case ItemStatus::ClaimPending: return "CLAIM_PENDING";
        case ItemStatus::Returned: return "RETURNED";
    }
    return "LOST";
}

inline ItemStatus stringToItemStatus(const std::string& str) {
    if (str == "FOUND") return ItemStatus::Found;
    if (str == "CLAIM_PENDING") return ItemStatus::ClaimPending;
    if (str == "RETURNED") return ItemStatus::Returned;
    return ItemStatus::Lost;
}

struct Item {
    int id = 0;
    int user_id = 0;
    std::string user_name;
    ItemType type = ItemType::Lost;
    std::string title;
    int category_id = 1;
    std::string category_name;
    std::string description;
    std::string location;
    std::string date_event; // YYYY-MM-DD
    std::string image_path;
    std::string contact_details;
    std::string held_at; // For found items: e.g. "Library Help Desk", "Security Office"
    ItemStatus status = ItemStatus::Lost;
    std::string created_at;
    std::string updated_at;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"user_id", user_id},
            {"user_name", user_name},
            {"type", itemTypeToString(type)},
            {"title", title},
            {"category_id", category_id},
            {"category_name", category_name},
            {"description", description},
            {"location", location},
            {"date_event", date_event},
            {"image_path", image_path},
            {"contact_details", contact_details},
            {"held_at", held_at},
            {"status", itemStatusToString(status)},
            {"created_at", created_at},
            {"updated_at", updated_at}
        };
    }
};

} // namespace back2you
