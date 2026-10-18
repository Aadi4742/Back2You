#pragma once

#include <httplib.h>

namespace back2you {

class ItemController {
public:
    static void reportLostItem(const httplib::Request& req, httplib::Response& res);
    static void reportFoundItem(const httplib::Request& req, httplib::Response& res);
    static void getItem(const httplib::Request& req, httplib::Response& res);
    static void updateItem(const httplib::Request& req, httplib::Response& res);
    static void deleteItem(const httplib::Request& req, httplib::Response& res);
    static void getCategories(const httplib::Request& req, httplib::Response& res);
};

} // namespace back2you
