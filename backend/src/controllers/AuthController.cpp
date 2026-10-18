// AuthController.cpp - handles register, login, and auth tokens
// team: Sameer Chand & Aditya Rai

#include "controllers/AuthController.h"
#include "database/Database.h"
#include "utils/Crypto.h"
#include "utils/Validation.h"
#include "utils/Auth.h"
#include <nlohmann/json.hpp>
#include <iostream>

namespace back2you {

// Sameer: registration handler with input checks and password salting
void AuthController::registerUser(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

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

    std::string username = reqJson.value("username", "");
    std::string email = reqJson.value("email", "");
    std::string password = reqJson.value("password", "");
    std::string fullName = reqJson.value("full_name", "");
    std::string phone = reqJson.value("phone", "");

    // Validation
    if (!Validation::isValidUsername(username)) {
        res.status = 400;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Username must be 3-32 characters and contain only alphanumeric, dash, dot, or underscore"}
        }.dump(), "application/json");
        return;
    }

    if (!Validation::isValidEmail(email)) {
        res.status = 400;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Invalid email address format"}
        }.dump(), "application/json");
        return;
    }

    if (password.length() < 6) {
        res.status = 400;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Password must be at least 6 characters long"}
        }.dump(), "application/json");
        return;
    }

    if (fullName.empty()) {
        res.status = 400;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Full name is required"}
        }.dump(), "application/json");
        return;
    }

    auto& db = Database::getInstance();

    if (db.getUserByUsername(username).has_value()) {
        res.status = 409;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Username is already taken"}
        }.dump(), "application/json");
        return;
    }

    if (db.getUserByEmail(email).has_value()) {
        res.status = 409;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Email is already registered"}
        }.dump(), "application/json");
        return;
    }

    std::string salt = Crypto::generateRandomHex(16);
    std::string hash = Crypto::hashPassword(password, salt);

    User newUser;
    newUser.username = username;
    newUser.email = email;
    newUser.password_hash = hash;
    newUser.salt = salt;
    newUser.role = UserRole::User;
    newUser.full_name = fullName;
    newUser.phone = phone;

    int newId = db.createUser(newUser);
    if (newId <= 0) {
        res.status = 500;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Failed to register user in database"}
        }.dump(), "application/json");
        return;
    }

    newUser.id = newId;
    std::string token = Crypto::generateToken();
    db.createSession(token, newId);

    res.status = 201;
    res.set_content(nlohmann::json{
        {"success", true},
        {"message", "Registration successful"},
        {"token", token},
        {"user", newUser.toJson()}
    }.dump(), "application/json");
}

void AuthController::loginUser(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

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

    std::string identifier = reqJson.value("username", "");
    if (identifier.empty()) {
        identifier = reqJson.value("email", "");
    }
    std::string password = reqJson.value("password", "");

    if (identifier.empty() || password.empty()) {
        res.status = 400;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Username/email and password are required"}
        }.dump(), "application/json");
        return;
    }

    auto& db = Database::getInstance();
    std::optional<User> userOpt = db.getUserByUsername(identifier);
    if (!userOpt.has_value()) {
        userOpt = db.getUserByEmail(identifier);
    }

    if (!userOpt.has_value()) {
        res.status = 401;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Invalid username/email or password"}
        }.dump(), "application/json");
        return;
    }

    const User& user = *userOpt;
    if (!Crypto::verifyPassword(password, user.salt, user.password_hash)) {
        res.status = 401;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Invalid username/email or password"}
        }.dump(), "application/json");
        return;
    }

    std::string token = Crypto::generateToken();
    db.createSession(token, user.id);

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"message", "Login successful"},
        {"token", token},
        {"user", user.toJson()}
    }.dump(), "application/json");
}

void AuthController::logoutUser(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    std::string token = Auth::extractToken(req);
    if (!token.empty()) {
        Database::getInstance().deleteSession(token);
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"message", "Logged out successfully"}
    }.dump(), "application/json");
}

void AuthController::getCurrentUser(const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "application/json");

    auto user = Auth::getAuthenticatedUser(req);
    if (!user.has_value()) {
        res.status = 401;
        res.set_content(nlohmann::json{
            {"success", false},
            {"error", "Unauthorized. Invalid or expired token."}
        }.dump(), "application/json");
        return;
    }

    res.status = 200;
    res.set_content(nlohmann::json{
        {"success", true},
        {"user", user->toJson()}
    }.dump(), "application/json");
}

} // namespace back2you
