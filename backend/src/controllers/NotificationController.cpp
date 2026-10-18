#include "controllers/NotificationController.h"
#include "database/Database.h"
#include "utils/Auth.h"
#include <nlohmann/json.hpp>
#include <iostream>

namespace back2you {

void NotificationController::getUserNotifications(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    auto user = Auth::getAuthenticatedUser(req);
    if (!user.has_value()) {
        res.status = 401;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Unauthorized"}}.dump(), "application/json");
        return;
    }

    bool unreadOnly = false;
    if (req.has_param("unread") && req.get_param_value("unread") == "true") {
        unreadOnly = true;
    }

    auto notifs = Database::getInstance().getNotificationsForUser(user->id, unreadOnly);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& n : notifs) {
        arr.push_back(n.toJson());
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"count", arr.size()},
        {"notifications", arr}
    }.dump(), "application/json");
}

void NotificationController::markAsRead(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    auto user = Auth::getAuthenticatedUser(req);
    if (!user.has_value()) {
        res.status = 401;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Unauthorized"}}.dump(), "application/json");
        return;
    }

    if (req.matches.size() < 2) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Missing notification ID"}}.dump(), "application/json");
        return;
    }

    int notifId = 0;
    try {
        notifId = std::stoi(req.matches[1]);
    } catch (...) {
        res.status = 400;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Invalid notification ID"}}.dump(), "application/json");
        return;
    }

    Database::getInstance().markNotificationRead(notifId, user->id);

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"message", "Notification marked as read"}
    }.dump(), "application/json");
}

void NotificationController::markAllAsRead(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    auto user = Auth::getAuthenticatedUser(req);
    if (!user.has_value()) {
        res.status = 401;
        res.set_content(nlohmann::json{{"success", false}, {"error", "Unauthorized"}}.dump(), "application/json");
        return;
    }

    Database::getInstance().markAllNotificationsRead(user->id);

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"message", "All notifications marked as read"}
    }.dump(), "application/json");
}

} // namespace back2you
