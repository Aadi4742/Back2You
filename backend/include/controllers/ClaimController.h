#pragma once

#include <httplib.h>

namespace back2you {

class ClaimController {
public:
    static void submitClaim(const httplib::Request& req, httplib::Response& res);
    static void getItemClaims(const httplib::Request& req, httplib::Response& res);
    static void getMyClaims(const httplib::Request& req, httplib::Response& res);
    static void reviewClaim(const httplib::Request& req, httplib::Response& res);
};

} // namespace back2you
