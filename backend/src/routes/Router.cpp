#include "routes/Router.h"
#include "controllers/AuthController.h"
#include "controllers/ItemController.h"
#include "controllers/ClaimController.h"
#include "controllers/NotificationController.h"
#include "controllers/AdminController.h"
#include <nlohmann/json.hpp>
#include <iostream>

namespace back2you {

void Router::setupRoutes(httplib::Server& server) {
    // CORS preflight and basic headers
    server.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Auth-Token");

        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // server health check
    server.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Content-Type", "application/json");
        res.status = 200;
        res.set_content(nlohmann::json{
            {"status", "ok"},
            {"service", "Back2You REST API"},
            {"version", "1.0.0"},
            {"authors", {"Sameer Chand", "Aditya Rai"}}
        }.dump(), "application/json");
    });

    // auth routes
    server.Post("/api/auth/register", AuthController::registerUser);
    server.Post("/api/auth/login", AuthController::loginUser);
    server.Post("/api/auth/logout", AuthController::logoutUser);
    server.Get("/api/auth/me", AuthController::getCurrentUser);

    // categories list
    server.Get("/api/categories", ItemController::getCategories);

    // item reporting, search and matching
    server.Get("/api/items", ItemController::searchItems);
    server.Post("/api/items/lost", ItemController::reportLostItem);
    server.Post("/api/items/found", ItemController::reportFoundItem);
    server.Get(R"(/api/items/(\d+))", ItemController::getItem);
    server.Put(R"(/api/items/(\d+))", ItemController::updateItem);
    server.Delete(R"(/api/items/(\d+))", ItemController::deleteItem);
    server.Get(R"(/api/items/(\d+)/matches)", ItemController::getItemMatches);

    // claims submission & resolution
    server.Post("/api/claims", ClaimController::submitClaim);
    server.Get("/api/claims/my", ClaimController::getMyClaims);
    server.Get(R"(/api/claims/item/(\d+))", ClaimController::getItemClaims);
    server.Post(R"(/api/claims/(\d+)/review)", ClaimController::reviewClaim);

    // user alerts & notifications
    server.Get("/api/notifications", NotificationController::getUserNotifications);
    server.Post(R"(/api/notifications/(\d+)/read)", NotificationController::markAsRead);
    server.Post("/api/notifications/read-all", NotificationController::markAllAsRead);

    // admin review & statistics
    server.Get("/api/admin/users", AdminController::getAllUsers);
    server.Put(R"(/api/admin/users/(\d+)/role)", AdminController::updateUserRole);
    server.Delete(R"(/api/admin/users/(\d+))", AdminController::deleteUser);
    server.Get("/api/admin/items", AdminController::getAllItems);
    server.Put(R"(/api/admin/items/(\d+)/status)", AdminController::updateItemStatus);
    server.Get("/api/admin/claims", AdminController::getAllClaims);
    server.Get("/api/admin/stats", AdminController::getStats);

    std::cout << "[Router] All REST API endpoints and CORS middlewares successfully registered." << std::endl;
}

} // namespace back2you
