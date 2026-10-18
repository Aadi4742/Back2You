#pragma once

#include <httplib.h>

namespace back2you {

class Router {
public:
    static void setupRoutes(httplib::Server& server);
};

} // namespace back2you
