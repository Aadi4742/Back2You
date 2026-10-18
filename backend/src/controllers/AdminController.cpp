#include "controllers/AdminController.h"
#include "database/Database.h"
#include "utils/Auth.h"
#include <nlohmann/json.hpp>
#include <iostream>

namespace back2you {

void AdminController::getAllUsers(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    if (!Auth::isAdmin(req)) {
        res.status = 403;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Forbidden. Admin access required."}}.dump(), "application/json");
        return;
    }

    auto users = Database::getInstance().getAllUsers();
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& u : users) {
        arr.push_back(u.toJson());
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"count", arr.size()},
        {"users", arr}
    }.dump(), "application/json");
}

void AdminController::updateUserRole(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    if (!Auth::isAdmin(req)) {
        res.status = 403;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Forbidden. Admin access required."}}.dump(), "application/json");
        return;
    }

    if (req.matches.size() < 2) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Missing user ID"}}.dump(), "application/json");
        return;
    }

    int userId = 0;
    try {
        userId = std::stoi(req.matches[1]);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Invalid user ID"}}.dump(), "application/json");
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

    std::string roleStr = reqJson.value("role", "");
    if (roleStr != "admin" && roleStr != "user") {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Role must be 'admin' or 'user'"}}.dump(), "application/json");
        return;
    }

    UserRole newRole = stringToRole(roleStr);
    bool ok = Database::getInstance().updateUserRole(userId, newRole);
    if (!ok) {
        res.status = 500;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Failed to update user role"}}.dump(), "application/json");
        return;
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"message", "User role updated successfully"},
        {"user_id", userId},
        {"role", roleStr}
    }.dump(), "application/json");
}

void AdminController::deleteUser(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    if (!Auth::isAdmin(req)) {
        res.status = 403;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Forbidden. Admin access required."}}.dump(), "application/json");
        return;
    }

    if (req.matches.size() < 2) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Missing user ID"}}.dump(), "application/json");
        return;
    }

    int userId = 0;
    try {
        userId = std::stoi(req.matches[1]);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Invalid user ID"}}.dump(), "application/json");
        return;
    }

    auto caller = Auth::getAuthenticatedUser(req);
    if (caller.has_value() && caller->id == userId) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Admins cannot delete their own account."}}.dump(), "application/json");
        return;
    }

    Database::getInstance().deleteUser(userId);

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"message", "User deleted successfully"}
    }.dump(), "application/json");
}

void AdminController::getAllItems(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    if (!Auth::isAdmin(req)) {
        res.status = 403;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Forbidden. Admin access required."}}.dump(), "application/json");
        return;
    }

    auto items = Database::getInstance().getItems("", 0, "", "", "", "", 200, 0);
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

void AdminController::updateItemStatus(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    if (!Auth::isAdmin(req)) {
        res.status = 403;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Forbidden. Admin access required."}}.dump(), "application/json");
        return;
    }

    if (req.matches.size() < 2) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Missing item ID"}}.dump(), "application/json");
        return;
    }

    int itemId = 0;
    try {
        itemId = std::stoi(req.matches[1]);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Invalid item ID"}}.dump(), "application/json");
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

    std::string statusStr = reqJson.value("status", "");
    ItemStatus status = stringToItemStatus(statusStr);

    bool ok = Database::getInstance().updateItemStatus(itemId, status);
    if (!ok) {
        res.status = 500;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Failed to update item status"}}.dump(), "application/json");
        return;
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"message", "Item status updated to " + itemStatusToString(status)},
        {"item_id", itemId},
        {"status", itemStatusToString(status)}
    }.dump(), "application/json");
}

void AdminController::getAllClaims(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    if (!Auth::isAdmin(req)) {
        res.status = 403;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Forbidden. Admin access required."}}.dump(), "application/json");
        return;
    }

    auto claims = Database::getInstance().getAllClaims();
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& c : claims) {
        arr.push_back(c.toJson());
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"count", arr.size()},
        {"claims", arr}
    }.dump(), "application/json");
}

void AdminController::getStats(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    auto& db = Database::getInstance();
    int totalLost = db.countItems("LOST", "");
    int totalFound = db.countItems("FOUND", "");
    int totalReturned = db.countItems("", "RETURNED");
    int activeClaims = db.countItems("", "CLAIM_PENDING");
    int totalUsers = static_cast<int>(db.getAllUsers().size());

    double recoveryRate = 0.0;
    int resolvedOrActive = totalLost + totalFound;
    if (resolvedOrActive > 0) {
        recoveryRate = (static_cast<double>(totalReturned) / resolvedOrActive) * 100.0;
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"stats", {
            {"total_lost", totalLost},
            {"total_found", totalFound},
            {"total_returned", totalReturned},
            {"active_claims_pending", activeClaims},
            {"registered_users", totalUsers},
            {"recovery_rate_percent", std::round(recoveryRate * 10.0) / 10.0}
        }}
    }.dump(), "application/json");
}

} // namespace back2you
