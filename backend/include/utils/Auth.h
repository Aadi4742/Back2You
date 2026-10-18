#pragma once

#include <optional>
#include <string>
#include "models/User.h"
#include <httplib.h>

namespace back2you {

class Auth {
public:
    // Extract token from "Authorization: Bearer <token>" or "X-Auth-Token: <token>"
    static std::string extractToken(const httplib::Request& req);

    // Get current authenticated user from request
    static std::optional<User> getAuthenticatedUser(const httplib::Request& req);

    // Check if the request is from an admin
    static bool isAdmin(const httplib::Request& req);
};

} // namespace back2you
