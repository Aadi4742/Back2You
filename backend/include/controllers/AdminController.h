#pragma once

#include <httplib.h>

namespace back2you {

class AdminController {
public:
    static void getAllUsers(const httplib::Request& req, httplib::Response& res);
    static void updateUserRole(const httplib::Request& req, httplib::Response& res);
    static void deleteUser(const httplib::Request& req, httplib::Response& res);
    static void getAllItems(const httplib::Request& req, httplib::Response& res);
    static void updateItemStatus(const httplib::Request& req, httplib::Response& res);
    static void getAllClaims(const httplib::Request& req, httplib::Response& res);
    static void getStats(const httplib::Request& req, httplib::Response& res);
};

} // namespace back2you
