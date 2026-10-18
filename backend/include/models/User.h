#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace back2you {

enum class UserRole {
    User,
    Admin
};

inline std::string roleToString(UserRole role) {
    return role == UserRole::Admin ? "admin" : "user";
}

inline UserRole stringToRole(const std::string& str) {
    if (str == "admin") return UserRole::Admin;
    return UserRole::User;
}

struct User {
    int id = 0;
    std::string username;
    std::string email;
    std::string password_hash;
    std::string salt;
    UserRole role = UserRole::User;
    std::string full_name;
    std::string phone;
    std::string created_at;

    // Convert User to JSON (excluding sensitive password hash & salt by default)
    nlohmann::json toJson(bool includeSensitive = false) const {
        nlohmann::json j = {
            {"id", id},
            {"username", username},
            {"email", email},
            {"role", roleToString(role)},
            {"full_name", full_name},
            {"phone", phone},
            {"created_at", created_at}
        };
        if (includeSensitive) {
            j["password_hash"] = password_hash;
            j["salt"] = salt;
        }
        return j;
    }
};

} // namespace back2you
