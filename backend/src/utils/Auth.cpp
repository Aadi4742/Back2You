#include "utils/Auth.h"
#include "database/Database.h"

namespace back2you {

std::string Auth::extractToken(const httplib::Request& req) {
    if (req.has_header("Authorization")) {
        std::string auth = req.get_header_value("Authorization");
        const std::string prefix = "Bearer ";
        if (auth.rfind(prefix, 0) == 0) {
            return auth.substr(prefix.length());
        }
        return auth;
    }
    if (req.has_header("X-Auth-Token")) {
        return req.get_header_value("X-Auth-Token");
    }
    return "";
}

std::optional<User> Auth::getAuthenticatedUser(const httplib::Request& req) {
    std::string token = extractToken(req);
    if (token.empty()) return std::nullopt;

    auto userId = Database::getInstance().getUserIdBySession(token);
    if (!userId.has_value()) return std::nullopt;

    return Database::getInstance().getUserById(*userId);
}

bool Auth::isAdmin(const httplib::Request& req) {
    auto user = getAuthenticatedUser(req);
    return user.has_value() && user->role == UserRole::Admin;
}

} // namespace back2you
