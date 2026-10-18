// ItemController.cpp - lost/found reporting, filtering, and search
// team: Aditya Rai & Sameer Chand

#include "controllers/ItemController.h"
#include "database/Database.h"
#include "utils/Auth.h"
#include "utils/Validation.h"
#include "utils/MatchingEngine.h"
#include <nlohmann/json.hpp>
#include <iostream>

namespace back2you {

// Aditya: handle user posting a lost item report
void ItemController::reportLostItem(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    auto user = Auth::getAuthenticatedUser(req);
    if (!user.has_value()) {
        res.status = 401;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Unauthorized. Please log in to report a lost item."}
        }.dump(), "application/json");
        return;
    }

    nlohmann::json reqJson;
    try {
        reqJson = nlohmann::json::parse(req.body);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Invalid JSON payload"}
        }.dump(), "application/json");
        return;
    }

    std::string title = reqJson.value("title", "");
    int categoryId = reqJson.value("category_id", 1);
    std::string description = reqJson.value("description", "");
    std::string location = reqJson.value("location", "");
    std::string dateEvent = reqJson.value("date_event", "");
    std::string imagePath = reqJson.value("image_path", "");
    std::string contactDetails = reqJson.value("contact_details", "");

    if (title.empty() || description.empty() || location.empty() || dateEvent.empty()) {
        res.status = 400;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Title, description, location, and date_event (YYYY-MM-DD) are required"}
        }.dump(), "application/json");
        return;
    }

    if (!Validation::isValidDate(dateEvent)) {
        res.status = 400;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "date_event must follow YYYY-MM-DD format"}
        }.dump(), "application/json");
        return;
    }

    if (contactDetails.empty()) {
        contactDetails = user->email + (user->phone.empty() ? "" : " | " + user->phone);
    }

    Item item;
    item.user_id = user->id;
    item.type = ItemType::Lost;
    item.title = Validation::sanitize(title);
    item.category_id = categoryId;
    item.description = Validation::sanitize(description);
    item.location = Validation::sanitize(location);
    item.date_event = dateEvent;
    item.image_path = imagePath;
    item.contact_details = Validation::sanitize(contactDetails);
    item.held_at = "";
    item.status = ItemStatus::Lost;

    int newId = Database::getInstance().createItem(item);
    if (newId <= 0) {
        res.status = 500;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Failed to save item in database"}
        }.dump(), "application/json");
        return;
    }

    auto createdItem = Database::getInstance().getItemById(newId);
    if (createdItem.has_value()) {
        // Trigger smart matching engine asynchronously / synchronously
        MatchingEngine::notifyPotentialMatches(*createdItem);
    }

    res.status = 201;
    res.set_content(nlohmann::json{
        {"success", true},
        {"message", "Lost item reported successfully"},
        {"item", createdItem.has_value() ? createdItem->toJson() : nlohmann::json{{"id", newId}}}
    }.dump(), "application/json");
}

void ItemController::reportFoundItem(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    auto user = Auth::getAuthenticatedUser(req);
    if (!user.has_value()) {
        res.status = 401;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Unauthorized. Please log in to report a found item."}
        }.dump(), "application/json");
        return;
    }

    nlohmann::json reqJson;
    try {
        reqJson = nlohmann::json::parse(req.body);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Invalid JSON payload"}
        }.dump(), "application/json");
        return;
    }

    std::string title = reqJson.value("title", "");
    int categoryId = reqJson.value("category_id", 1);
    std::string description = reqJson.value("description", "");
    std::string location = reqJson.value("location", "");
    std::string dateEvent = reqJson.value("date_event", "");
    std::string imagePath = reqJson.value("image_path", "");
    std::string contactDetails = reqJson.value("contact_details", "");
    std::string heldAt = reqJson.value("held_at", "");

    if (title.empty() || description.empty() || location.empty() || dateEvent.empty() || heldAt.empty()) {
        res.status = 400;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Title, description, location, date_event, and held_at are required"}
        }.dump(), "application/json");
        return;
    }

    if (!Validation::isValidDate(dateEvent)) {
        res.status = 400;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "date_event must follow YYYY-MM-DD format"}
        }.dump(), "application/json");
        return;
    }

    if (contactDetails.empty()) {
        contactDetails = user->email + (user->phone.empty() ? "" : " | " + user->phone);
    }

    Item item;
    item.user_id = user->id;
    item.type = ItemType::Found;
    item.title = Validation::sanitize(title);
    item.category_id = categoryId;
    item.description = Validation::sanitize(description);
    item.location = Validation::sanitize(location);
    item.date_event = dateEvent;
    item.image_path = imagePath;
    item.contact_details = Validation::sanitize(contactDetails);
    item.held_at = Validation::sanitize(heldAt);
    item.status = ItemStatus::Found;

    int newId = Database::getInstance().createItem(item);
    if (newId <= 0) {
        res.status = 500;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Failed to save found item in database"}
        }.dump(), "application/json");
        return;
    }

    auto createdItem = Database::getInstance().getItemById(newId);
    if (createdItem.has_value()) {
        // Trigger smart matching engine
        MatchingEngine::notifyPotentialMatches(*createdItem);
    }

    res.status = 201;
    res.set_content(nlohmann::json{
        {"success", true},
        {"message", "Found item reported successfully"},
        {"item", createdItem.has_value() ? createdItem->toJson() : nlohmann::json{{"id", newId}}}
    }.dump(), "application/json");
}

void ItemController::getItem(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    if (req.matches.size() < 2) {
        res.status = 400;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Missing item ID parameter"}
        }.dump(), "application/json");
        return;
    }

    int id = 0;
    try {
        std::string idStr = req.matches[1];
        id = std::stoi(idStr);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Invalid item ID"}}.dump(), "application/json");
        return;
    }

    auto item = Database::getInstance().getItemById(id);
    if (!item.has_value()) {
        res.status = 404;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Item not found"}}.dump(), "application/json");
        return;
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"item", item->toJson()}
    }.dump(), "application/json");
}

void ItemController::updateItem(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    auto user = Auth::getAuthenticatedUser(req);
    if (!user.has_value()) {
        res.status = 401;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Unauthorized"}}.dump(), "application/json");
        return;
    }

    int id = 0;
    try {
        std::string idStr = req.matches[1];
        id = std::stoi(idStr);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Invalid item ID"}}.dump(), "application/json");
        return;
    }

    auto existingItem = Database::getInstance().getItemById(id);
    if (!existingItem.has_value()) {
        res.status = 404;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Item not found"}}.dump(), "application/json");
        return;
    }

    // Must be owner or admin
    if (existingItem->user_id != user->id && user->role != UserRole::Admin) {
        res.status = 403;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Permission denied. Only item reporter or admin can edit this item."}}.dump(), "application/json");
        return;
    }

    nlohmann::json reqJson;
    try {
        reqJson = nlohmann::json::parse(req.body);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Invalid JSON"}}.dump(), "application/json");
        return;
    }

    Item updated = *existingItem;
    if (reqJson.contains("title")) updated.title = Validation::sanitize(reqJson["title"].get<std::string>());
    if (reqJson.contains("description")) updated.description = Validation::sanitize(reqJson["description"].get<std::string>());
    if (reqJson.contains("location")) updated.location = Validation::sanitize(reqJson["location"].get<std::string>());
    if (reqJson.contains("category_id")) updated.category_id = reqJson["category_id"].get<int>();
    if (reqJson.contains("held_at")) updated.held_at = Validation::sanitize(reqJson["held_at"].get<std::string>());
    if (reqJson.contains("contact_details")) updated.contact_details = Validation::sanitize(reqJson["contact_details"].get<std::string>());
    if (reqJson.contains("image_path")) updated.image_path = reqJson["image_path"].get<std::string>();

    bool ok = Database::getInstance().updateItem(updated);
    if (!ok) {
        res.status = 500;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Failed to update item"}}.dump(), "application/json");
        return;
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"message", "Item updated successfully"},
        {"item", updated.toJson()}
    }.dump(), "application/json");
}

void ItemController::deleteItem(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    auto user = Auth::getAuthenticatedUser(req);
    if (!user.has_value()) {
        res.status = 401;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Unauthorized"}}.dump(), "application/json");
        return;
    }

    int id = 0;
    try {
        std::string idStr = req.matches[1];
        id = std::stoi(idStr);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Invalid item ID"}}.dump(), "application/json");
        return;
    }

    auto existingItem = Database::getInstance().getItemById(id);
    if (!existingItem.has_value()) {
        res.status = 404;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Item not found"}}.dump(), "application/json");
        return;
    }

    if (existingItem->user_id != user->id && user->role != UserRole::Admin) {
        res.status = 403;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Permission denied"}}.dump(), "application/json");
        return;
    }

    Database::getInstance().deleteItem(id);

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"message", "Item deleted successfully"}
    }.dump(), "application/json");
}

void ItemController::getCategories(const httplib::Request&, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");
    auto categories = Database::getInstance().getCategories();

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& cat : categories) {
        arr.push_back(cat.toJson());
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"categories", arr}
    }.dump(), "application/json");
}

void ItemController::searchItems(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    std::string keyword;
    if (req.has_param("q")) keyword = req.get_param_value("q");
    else if (req.has_param("keyword")) keyword = req.get_param_value("keyword");

    int categoryId = 0;
    if (req.has_param("category_id")) {
        try { categoryId = std::stoi(req.get_param_value("category_id")); } catch (...) {}
    }

    std::string location;
    if (req.has_param("location")) location = req.get_param_value("location");

    std::string date;
    if (req.has_param("date")) date = req.get_param_value("date");

    std::string status;
    if (req.has_param("status")) status = req.get_param_value("status");

    std::string type;
    if (req.has_param("type")) type = req.get_param_value("type");

    int limit = 50;
    if (req.has_param("limit")) {
        try { limit = std::clamp(std::stoi(req.get_param_value("limit")), 1, 100); } catch (...) {}
    }

    int offset = 0;
    if (req.has_param("offset")) {
        try { offset = std::max(0, std::stoi(req.get_param_value("offset"))); } catch (...) {}
    }

    auto items = Database::getInstance().getItems(keyword, categoryId, location, date, status, type, limit, offset);

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& it : items) {
        arr.push_back(it.toJson());
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"count", arr.size()},
        {"items", arr}
    }.dump(), "application/json");
}

void ItemController::getItemMatches(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    if (req.matches.size() < 2) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Missing item ID"}}.dump(), "application/json");
        return;
    }

    int id = 0;
    try {
        id = std::stoi(req.matches[1]);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Invalid item ID"}}.dump(), "application/json");
        return;
    }

    auto item = Database::getInstance().getItemById(id);
    if (!item.has_value()) {
        res.status = 404;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Item not found"}}.dump(), "application/json");
        return;
    }

    auto matches = MatchingEngine::findMatches(*item, 25.0); // 25%+ match threshold

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& match : matches) {
        arr.push_back(match.toJson());
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"target_item", item->toJson()},
        {"match_count", arr.size()},
        {"matches", arr}
    }.dump(), "application/json");
}

} // namespace back2you
