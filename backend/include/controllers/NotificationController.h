#pragma once

#include <httplib.h>

namespace back2you {

class NotificationController {
public:
    static void getUserNotifications(const httplib::Request& req, httplib::Response& res);
    static void markAsRead(const httplib::Request& req, httplib::Response& res);
    static void markAllAsRead(const httplib::Request& req, httplib::Response& res);
};

} // namespace back2you
