#pragma once

#include <httplib.h>

namespace back2you {

class AuthController {
public:
    static void registerUser(const httplib::Request& req, httplib::Response& res);
    static void loginUser(const httplib::Request& req, httplib::Response& res);
    static void logoutUser(const httplib::Request& req, httplib::Response& res);
    static void getCurrentUser(const httplib::Request& req, httplib::Response& res);
};

} // namespace back2you
