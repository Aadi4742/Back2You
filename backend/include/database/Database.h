#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <mutex>
#include <sqlite3.h>

#include "models/User.h"
#include "models/Category.h"
#include "models/Item.h"
#include "models/Claim.h"
#include "models/Notification.h"

namespace back2you {

class Database {
public:
    static Database& getInstance(const std::string& dbPath = "back2you.db");

    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool init();
    void close();

    // User Operations
    int createUser(const User& user);
    std::optional<User> getUserById(int id);
    std::optional<User> getUserByUsername(const std::string& username);
    std::optional<User> getUserByEmail(const std::string& email);
    std::vector<User> getAllUsers();
    bool updateUserRole(int userId, UserRole role);
    bool deleteUser(int userId);

    // Session Operations
    bool createSession(const std::string& token, int userId, int expiryHours = 72);
    std::optional<int> getUserIdBySession(const std::string& token);
    bool deleteSession(const std::string& token);

    // Category Operations
    std::vector<Category> getCategories();
    std::optional<Category> getCategoryById(int id);

    // Item Operations
    int createItem(const Item& item);
    std::optional<Item> getItemById(int id);
    std::vector<Item> getItems(
        const std::string& keyword = "",
        int categoryId = 0,
        const std::string& location = "",
        const std::string& date = "",
        const std::string& status = "",
        const std::string& type = "",
        int limit = 50,
        int offset = 0
    );
    bool updateItem(const Item& item);
    bool updateItemStatus(int itemId, ItemStatus status);
    bool deleteItem(int itemId);
    int countItems(const std::string& type = "", const std::string& status = "");

    // Claim Operations
    int createClaim(const Claim& claim);
    std::optional<Claim> getClaimById(int id);
    std::vector<Claim> getClaimsByItemId(int itemId);
    std::vector<Claim> getClaimsByUserId(int userId);
    std::vector<Claim> getAllClaims();
    bool updateClaimStatus(int claimId, ClaimStatus status, const std::string& reviewNotes);

    // Notification Operations
    int createNotification(const Notification& notif);
    std::vector<Notification> getNotificationsForUser(int userId, bool unreadOnly = false);
    bool markNotificationRead(int notifId, int userId);
    bool markAllNotificationsRead(int userId);

private:
    explicit Database(std::string path);
    bool executeSchema();
    bool seedDefaultData();

    std::string db_path_;
    sqlite3* db_ = nullptr;
    std::mutex db_mutex_;
};

} // namespace back2you
