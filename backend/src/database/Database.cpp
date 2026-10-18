#include "database/Database.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace back2you {

namespace {
    // Current UTC timestamp in ISO 8601 format
    std::string currentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
}

Database& Database::getInstance(const std::string& dbPath) {
    static Database instance(dbPath);
    return instance;
}

Database::Database(std::string path) : db_path_(std::move(path)) {}

Database::~Database() {
    close();
}

void Database::close() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::init() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database] Failed to open database " << db_path_ << ": " 
                  << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    // Enable foreign keys
    char* errMsg = nullptr;
    rc = sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database] Failed to enable foreign keys: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }

    if (!executeSchema()) {
        return false;
    }

    seedDefaultData();
    std::cout << "[Database] Initialized successfully at " << db_path_ << std::endl;
    return true;
}

bool Database::executeSchema() {
    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            email TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            role TEXT NOT NULL DEFAULT 'user',
            full_name TEXT NOT NULL,
            phone TEXT,
            created_at TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS sessions (
            token TEXT PRIMARY KEY,
            user_id INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            expires_at TEXT NOT NULL,
            FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL,
            description TEXT,
            icon TEXT
        );

        CREATE TABLE IF NOT EXISTS items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            type TEXT NOT NULL, -- 'LOST' or 'FOUND'
            title TEXT NOT NULL,
            category_id INTEGER NOT NULL,
            description TEXT NOT NULL,
            location TEXT NOT NULL,
            date_event TEXT NOT NULL,
            image_path TEXT,
            contact_details TEXT NOT NULL,
            held_at TEXT,
            status TEXT NOT NULL DEFAULT 'LOST',
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
            FOREIGN KEY (category_id) REFERENCES categories(id)
        );

        CREATE TABLE IF NOT EXISTS claims (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            item_id INTEGER NOT NULL,
            claimant_id INTEGER NOT NULL,
            proof_details TEXT NOT NULL,
            contact_info TEXT NOT NULL,
            status TEXT NOT NULL DEFAULT 'PENDING', -- 'PENDING', 'APPROVED', 'REJECTED'
            review_notes TEXT,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            FOREIGN KEY (item_id) REFERENCES items(id) ON DELETE CASCADE,
            FOREIGN KEY (claimant_id) REFERENCES users(id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS notifications (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            title TEXT NOT NULL,
            message TEXT NOT NULL,
            type TEXT NOT NULL DEFAULT 'SYSTEM', -- 'MATCH', 'CLAIM', 'SYSTEM'
            is_read INTEGER NOT NULL DEFAULT 0,
            item_id INTEGER DEFAULT 0,
            created_at TEXT NOT NULL,
            FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
        );

        -- Performance Indexes
        CREATE INDEX IF NOT EXISTS idx_items_status ON items(status);
        CREATE INDEX IF NOT EXISTS idx_items_type ON items(type);
        CREATE INDEX IF NOT EXISTS idx_items_category ON items(category_id);
        CREATE INDEX IF NOT EXISTS idx_claims_item ON claims(item_id);
        CREATE INDEX IF NOT EXISTS idx_claims_claimant ON claims(claimant_id);
        CREATE INDEX IF NOT EXISTS idx_notifs_user ON notifications(user_id, is_read);
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, schema, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database] Schema creation failed: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::seedDefaultData() {
    const char* seedCategories = R"(
        INSERT OR IGNORE INTO categories (id, name, description, icon) VALUES
        (1, 'Electronics', 'Phones, laptops, chargers, headphones, tablets', 'laptop'),
        (2, 'Wallets & IDs', 'Wallets, purses, student IDs, credit cards, licenses', 'credit-card'),
        (3, 'Keys', 'Dorm keys, car keys, bike lock keys, keychains', 'key'),
        (4, 'Books & Stationery', 'Textbooks, notebooks, calculators, stationery sets', 'book-open'),
        (5, 'Bags & Backpacks', 'Backpacks, tote bags, gym bags, briefcases', 'briefcase'),
        (6, 'Clothing & Apparel', 'Jackets, hoodies, caps, scarves, umbrellas', 'shirt'),
        (7, 'Eyewear & Accessories', 'Prescription glasses, sunglasses, watches, jewelry', 'glasses'),
        (8, 'Sports & Fitness', 'Water bottles, sports gear, fitness trackers', 'activity'),
        (9, 'Other Items', 'Miscellaneous personal belongings and campus gear', 'help-circle');
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, seedCategories, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database] Seed categories failed: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

// ----------------- USER OPERATIONS -----------------

int Database::createUser(const User& user) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "INSERT INTO users (username, email, password_hash, salt, role, full_name, phone, created_at) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    std::string now = user.created_at.empty() ? currentTimestamp() : user.created_at;
    std::string roleStr = roleToString(user.role);

    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.password_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, user.salt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, roleStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, user.full_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, user.phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, now.c_str(), -1, SQLITE_TRANSIENT);

    int id = 0;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    }
    sqlite3_finalize(stmt);
    return id;
}

std::optional<User> Database::getUserById(int id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "SELECT id, username, email, password_hash, salt, role, full_name, phone, created_at "
                      "FROM users WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, id);

    std::optional<User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;
        u.id = sqlite3_column_int(stmt, 0);
        u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        u.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        u.role = stringToRole(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        u.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        const auto* phonePtr = sqlite3_column_text(stmt, 7);
        u.phone = phonePtr ? reinterpret_cast<const char*>(phonePtr) : "";
        u.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        result = u;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<User> Database::getUserByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "SELECT id, username, email, password_hash, salt, role, full_name, phone, created_at "
                      "FROM users WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;
        u.id = sqlite3_column_int(stmt, 0);
        u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        u.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        u.role = stringToRole(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        u.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        const auto* phonePtr = sqlite3_column_text(stmt, 7);
        u.phone = phonePtr ? reinterpret_cast<const char*>(phonePtr) : "";
        u.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        result = u;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<User> Database::getUserByEmail(const std::string& email) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "SELECT id, username, email, password_hash, salt, role, full_name, phone, created_at "
                      "FROM users WHERE email = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;
        u.id = sqlite3_column_int(stmt, 0);
        u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        u.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        u.role = stringToRole(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        u.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        const auto* phonePtr = sqlite3_column_text(stmt, 7);
        u.phone = phonePtr ? reinterpret_cast<const char*>(phonePtr) : "";
        u.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        result = u;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<User> Database::getAllUsers() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<User> users;
    const char* sql = "SELECT id, username, email, '', '', role, full_name, phone, created_at FROM users ORDER BY id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return users;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;
        u.id = sqlite3_column_int(stmt, 0);
        u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.role = stringToRole(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        u.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        const auto* phonePtr = sqlite3_column_text(stmt, 7);
        u.phone = phonePtr ? reinterpret_cast<const char*>(phonePtr) : "";
        u.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        users.push_back(u);
    }
    sqlite3_finalize(stmt);
    return users;
}

bool Database::updateUserRole(int userId, UserRole role) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "UPDATE users SET role = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string roleStr = roleToString(role);
    sqlite3_bind_text(stmt, 1, roleStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, userId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::deleteUser(int userId) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "DELETE FROM users WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, userId);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// ----------------- SESSION OPERATIONS -----------------

bool Database::createSession(const std::string& token, int userId, int expiryHours) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "INSERT OR REPLACE INTO sessions (token, user_id, created_at, expires_at) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    auto now = std::chrono::system_clock::now();
    auto expires = now + std::chrono::hours(expiryHours);

    auto to_str = [](std::chrono::system_clock::time_point tp) {
        auto in_t = std::chrono::system_clock::to_time_t(tp);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&in_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    };

    std::string nowStr = to_str(now);
    std::string expStr = to_str(expires);

    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_text(stmt, 3, nowStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, expStr.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::optional<int> Database::getUserIdBySession(const std::string& token) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "SELECT user_id, expires_at FROM sessions WHERE token = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<int> userId;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int uid = sqlite3_column_int(stmt, 0);
        std::string expiresAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string now = currentTimestamp();
        if (now < expiresAt) {
            userId = uid;
        }
    }
    sqlite3_finalize(stmt);
    return userId;
}

bool Database::deleteSession(const std::string& token) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "DELETE FROM sessions WHERE token = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// ----------------- CATEGORY OPERATIONS -----------------

std::vector<Category> Database::getCategories() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Category> list;
    const char* sql = "SELECT id, name, description, icon FROM categories ORDER BY id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Category c;
        c.id = sqlite3_column_int(stmt, 0);
        c.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const auto* descPtr = sqlite3_column_text(stmt, 2);
        c.description = descPtr ? reinterpret_cast<const char*>(descPtr) : "";
        const auto* iconPtr = sqlite3_column_text(stmt, 3);
        c.icon = iconPtr ? reinterpret_cast<const char*>(iconPtr) : "";
        list.push_back(c);
    }
    sqlite3_finalize(stmt);
    return list;
}

std::optional<Category> Database::getCategoryById(int id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "SELECT id, name, description, icon FROM categories WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, id);
    std::optional<Category> cat;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Category c;
        c.id = sqlite3_column_int(stmt, 0);
        c.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const auto* descPtr = sqlite3_column_text(stmt, 2);
        c.description = descPtr ? reinterpret_cast<const char*>(descPtr) : "";
        const auto* iconPtr = sqlite3_column_text(stmt, 3);
        c.icon = iconPtr ? reinterpret_cast<const char*>(iconPtr) : "";
        cat = c;
    }
    sqlite3_finalize(stmt);
    return cat;
}

// ----------------- ITEM OPERATIONS -----------------

int Database::createItem(const Item& item) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "INSERT INTO items (user_id, type, title, category_id, description, location, "
                      "date_event, image_path, contact_details, held_at, status, created_at, updated_at) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    std::string now = item.created_at.empty() ? currentTimestamp() : item.created_at;
    std::string typeStr = itemTypeToString(item.type);
    std::string statusStr = itemStatusToString(item.status);

    sqlite3_bind_int(stmt, 1, item.user_id);
    sqlite3_bind_text(stmt, 2, typeStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, item.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, item.category_id);
    sqlite3_bind_text(stmt, 5, item.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, item.location.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, item.date_event.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, item.image_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, item.contact_details.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, item.held_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, statusStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, now.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 13, now.c_str(), -1, SQLITE_TRANSIENT);

    int id = 0;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    }
    sqlite3_finalize(stmt);
    return id;
}

std::optional<Item> Database::getItemById(int id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "SELECT i.id, i.user_id, u.username, i.type, i.title, i.category_id, c.name, "
                      "i.description, i.location, i.date_event, i.image_path, i.contact_details, "
                      "i.held_at, i.status, i.created_at, i.updated_at "
                      "FROM items i "
                      "LEFT JOIN users u ON i.user_id = u.id "
                      "LEFT JOIN categories c ON i.category_id = c.id "
                      "WHERE i.id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, id);

    std::optional<Item> item;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Item it;
        it.id = sqlite3_column_int(stmt, 0);
        it.user_id = sqlite3_column_int(stmt, 1);
        const auto* uNamePtr = sqlite3_column_text(stmt, 2);
        it.user_name = uNamePtr ? reinterpret_cast<const char*>(uNamePtr) : "";
        it.type = stringToItemType(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        it.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        it.category_id = sqlite3_column_int(stmt, 5);
        const auto* cNamePtr = sqlite3_column_text(stmt, 6);
        it.category_name = cNamePtr ? reinterpret_cast<const char*>(cNamePtr) : "";
        it.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        it.location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        it.date_event = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        const auto* imgPtr = sqlite3_column_text(stmt, 10);
        it.image_path = imgPtr ? reinterpret_cast<const char*>(imgPtr) : "";
        it.contact_details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        const auto* heldPtr = sqlite3_column_text(stmt, 12);
        it.held_at = heldPtr ? reinterpret_cast<const char*>(heldPtr) : "";
        it.status = stringToItemStatus(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13)));
        it.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
        it.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 15));
        item = it;
    }
    sqlite3_finalize(stmt);
    return item;
}

std::vector<Item> Database::getItems(
    const std::string& keyword,
    int categoryId,
    const std::string& location,
    const std::string& date,
    const std::string& status,
    const std::string& type,
    int limit,
    int offset
) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Item> items;

    std::stringstream sql;
    sql << "SELECT i.id, i.user_id, u.username, i.type, i.title, i.category_id, c.name, "
        << "i.description, i.location, i.date_event, i.image_path, i.contact_details, "
        << "i.held_at, i.status, i.created_at, i.updated_at "
        << "FROM items i "
        << "LEFT JOIN users u ON i.user_id = u.id "
        << "LEFT JOIN categories c ON i.category_id = c.id "
        << "WHERE 1=1 ";

    if (!keyword.empty()) {
        sql << "AND (i.title LIKE ? OR i.description LIKE ? OR i.location LIKE ?) ";
    }
    if (categoryId > 0) {
        sql << "AND i.category_id = ? ";
    }
    if (!location.empty()) {
        sql << "AND i.location LIKE ? ";
    }
    if (!date.empty()) {
        sql << "AND i.date_event = ? ";
    }
    if (!status.empty()) {
        sql << "AND i.status = ? ";
    }
    if (!type.empty()) {
        sql << "AND i.type = ? ";
    }

    sql << "ORDER BY i.id DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return items;
    }

    int bindIdx = 1;
    std::string kwParam = "%" + keyword + "%";
    std::string locParam = "%" + location + "%";

    if (!keyword.empty()) {
        sqlite3_bind_text(stmt, bindIdx++, kwParam.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, bindIdx++, kwParam.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, bindIdx++, kwParam.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (categoryId > 0) {
        sqlite3_bind_int(stmt, bindIdx++, categoryId);
    }
    if (!location.empty()) {
        sqlite3_bind_text(stmt, bindIdx++, locParam.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (!date.empty()) {
        sqlite3_bind_text(stmt, bindIdx++, date.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (!status.empty()) {
        sqlite3_bind_text(stmt, bindIdx++, status.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (!type.empty()) {
        sqlite3_bind_text(stmt, bindIdx++, type.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int(stmt, bindIdx++, limit);
    sqlite3_bind_int(stmt, bindIdx++, offset);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Item it;
        it.id = sqlite3_column_int(stmt, 0);
        it.user_id = sqlite3_column_int(stmt, 1);
        const auto* uNamePtr = sqlite3_column_text(stmt, 2);
        it.user_name = uNamePtr ? reinterpret_cast<const char*>(uNamePtr) : "";
        it.type = stringToItemType(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        it.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        it.category_id = sqlite3_column_int(stmt, 5);
        const auto* cNamePtr = sqlite3_column_text(stmt, 6);
        it.category_name = cNamePtr ? reinterpret_cast<const char*>(cNamePtr) : "";
        it.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        it.location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        it.date_event = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        const auto* imgPtr = sqlite3_column_text(stmt, 10);
        it.image_path = imgPtr ? reinterpret_cast<const char*>(imgPtr) : "";
        it.contact_details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        const auto* heldPtr = sqlite3_column_text(stmt, 12);
        it.held_at = heldPtr ? reinterpret_cast<const char*>(heldPtr) : "";
        it.status = stringToItemStatus(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13)));
        it.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
        it.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 15));
        items.push_back(it);
    }
    sqlite3_finalize(stmt);
    return items;
}

bool Database::updateItem(const Item& item) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "UPDATE items SET title = ?, category_id = ?, description = ?, location = ?, "
                      "date_event = ?, image_path = ?, contact_details = ?, held_at = ?, status = ?, "
                      "updated_at = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string now = currentTimestamp();
    std::string statusStr = itemStatusToString(item.status);

    sqlite3_bind_text(stmt, 1, item.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, item.category_id);
    sqlite3_bind_text(stmt, 3, item.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, item.location.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, item.date_event.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, item.image_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, item.contact_details.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, item.held_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, statusStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, now.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 11, item.id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::updateItemStatus(int itemId, ItemStatus status) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "UPDATE items SET status = ?, updated_at = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string now = currentTimestamp();
    std::string statusStr = itemStatusToString(status);

    sqlite3_bind_text(stmt, 1, statusStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, now.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, itemId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::deleteItem(int itemId) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "DELETE FROM items WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, itemId);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

int Database::countItems(const std::string& type, const std::string& status) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::stringstream sql;
    sql << "SELECT COUNT(*) FROM items WHERE 1=1 ";
    if (!type.empty()) sql << "AND type = ? ";
    if (!status.empty()) sql << "AND status = ? ";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) return 0;

    int bindIdx = 1;
    if (!type.empty()) sqlite3_bind_text(stmt, bindIdx++, type.c_str(), -1, SQLITE_TRANSIENT);
    if (!status.empty()) sqlite3_bind_text(stmt, bindIdx++, status.c_str(), -1, SQLITE_TRANSIENT);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

// ----------------- CLAIM OPERATIONS -----------------

int Database::createClaim(const Claim& claim) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "INSERT INTO claims (item_id, claimant_id, proof_details, contact_info, status, review_notes, created_at, updated_at) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    std::string now = claim.created_at.empty() ? currentTimestamp() : claim.created_at;
    std::string statusStr = claimStatusToString(claim.status);

    sqlite3_bind_int(stmt, 1, claim.item_id);
    sqlite3_bind_int(stmt, 2, claim.claimant_id);
    sqlite3_bind_text(stmt, 3, claim.proof_details.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, claim.contact_info.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, statusStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, claim.review_notes.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, now.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, now.c_str(), -1, SQLITE_TRANSIENT);

    int id = 0;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    }
    sqlite3_finalize(stmt);
    return id;
}

std::optional<Claim> Database::getClaimById(int id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "SELECT c.id, c.item_id, i.title, c.claimant_id, u.username, c.proof_details, "
                      "c.contact_info, c.status, c.review_notes, c.created_at, c.updated_at "
                      "FROM claims c "
                      "LEFT JOIN items i ON c.item_id = i.id "
                      "LEFT JOIN users u ON c.claimant_id = u.id "
                      "WHERE c.id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, id);

    std::optional<Claim> claim;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Claim cl;
        cl.id = sqlite3_column_int(stmt, 0);
        cl.item_id = sqlite3_column_int(stmt, 1);
        const auto* titlePtr = sqlite3_column_text(stmt, 2);
        cl.item_title = titlePtr ? reinterpret_cast<const char*>(titlePtr) : "";
        cl.claimant_id = sqlite3_column_int(stmt, 3);
        const auto* uNamePtr = sqlite3_column_text(stmt, 4);
        cl.claimant_name = uNamePtr ? reinterpret_cast<const char*>(uNamePtr) : "";
        cl.proof_details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        cl.contact_info = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        cl.status = stringToClaimStatus(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        const auto* notesPtr = sqlite3_column_text(stmt, 8);
        cl.review_notes = notesPtr ? reinterpret_cast<const char*>(notesPtr) : "";
        cl.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        cl.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        claim = cl;
    }
    sqlite3_finalize(stmt);
    return claim;
}

std::vector<Claim> Database::getClaimsByItemId(int itemId) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Claim> list;
    const char* sql = "SELECT c.id, c.item_id, i.title, c.claimant_id, u.username, c.proof_details, "
                      "c.contact_info, c.status, c.review_notes, c.created_at, c.updated_at "
                      "FROM claims c "
                      "LEFT JOIN items i ON c.item_id = i.id "
                      "LEFT JOIN users u ON c.claimant_id = u.id "
                      "WHERE c.item_id = ? ORDER BY c.id DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, itemId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Claim cl;
        cl.id = sqlite3_column_int(stmt, 0);
        cl.item_id = sqlite3_column_int(stmt, 1);
        const auto* titlePtr = sqlite3_column_text(stmt, 2);
        cl.item_title = titlePtr ? reinterpret_cast<const char*>(titlePtr) : "";
        cl.claimant_id = sqlite3_column_int(stmt, 3);
        const auto* uNamePtr = sqlite3_column_text(stmt, 4);
        cl.claimant_name = uNamePtr ? reinterpret_cast<const char*>(uNamePtr) : "";
        cl.proof_details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        cl.contact_info = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        cl.status = stringToClaimStatus(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        const auto* notesPtr = sqlite3_column_text(stmt, 8);
        cl.review_notes = notesPtr ? reinterpret_cast<const char*>(notesPtr) : "";
        cl.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        cl.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        list.push_back(cl);
    }
    sqlite3_finalize(stmt);
    return list;
}

std::vector<Claim> Database::getClaimsByUserId(int userId) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Claim> list;
    const char* sql = "SELECT c.id, c.item_id, i.title, c.claimant_id, u.username, c.proof_details, "
                      "c.contact_info, c.status, c.review_notes, c.created_at, c.updated_at "
                      "FROM claims c "
                      "LEFT JOIN items i ON c.item_id = i.id "
                      "LEFT JOIN users u ON c.claimant_id = u.id "
                      "WHERE c.claimant_id = ? ORDER BY c.id DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, userId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Claim cl;
        cl.id = sqlite3_column_int(stmt, 0);
        cl.item_id = sqlite3_column_int(stmt, 1);
        const auto* titlePtr = sqlite3_column_text(stmt, 2);
        cl.item_title = titlePtr ? reinterpret_cast<const char*>(titlePtr) : "";
        cl.claimant_id = sqlite3_column_int(stmt, 3);
        const auto* uNamePtr = sqlite3_column_text(stmt, 4);
        cl.claimant_name = uNamePtr ? reinterpret_cast<const char*>(uNamePtr) : "";
        cl.proof_details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        cl.contact_info = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        cl.status = stringToClaimStatus(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        const auto* notesPtr = sqlite3_column_text(stmt, 8);
        cl.review_notes = notesPtr ? reinterpret_cast<const char*>(notesPtr) : "";
        cl.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        cl.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        list.push_back(cl);
    }
    sqlite3_finalize(stmt);
    return list;
}

std::vector<Claim> Database::getAllClaims() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Claim> list;
    const char* sql = "SELECT c.id, c.item_id, i.title, c.claimant_id, u.username, c.proof_details, "
                      "c.contact_info, c.status, c.review_notes, c.created_at, c.updated_at "
                      "FROM claims c "
                      "LEFT JOIN items i ON c.item_id = i.id "
                      "LEFT JOIN users u ON c.claimant_id = u.id "
                      "ORDER BY c.id DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Claim cl;
        cl.id = sqlite3_column_int(stmt, 0);
        cl.item_id = sqlite3_column_int(stmt, 1);
        const auto* titlePtr = sqlite3_column_text(stmt, 2);
        cl.item_title = titlePtr ? reinterpret_cast<const char*>(titlePtr) : "";
        cl.claimant_id = sqlite3_column_int(stmt, 3);
        const auto* uNamePtr = sqlite3_column_text(stmt, 4);
        cl.claimant_name = uNamePtr ? reinterpret_cast<const char*>(uNamePtr) : "";
        cl.proof_details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        cl.contact_info = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        cl.status = stringToClaimStatus(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        const auto* notesPtr = sqlite3_column_text(stmt, 8);
        cl.review_notes = notesPtr ? reinterpret_cast<const char*>(notesPtr) : "";
        cl.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        cl.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        list.push_back(cl);
    }
    sqlite3_finalize(stmt);
    return list;
}

bool Database::updateClaimStatus(int claimId, ClaimStatus status, const std::string& reviewNotes) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "UPDATE claims SET status = ?, review_notes = ?, updated_at = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string now = currentTimestamp();
    std::string statusStr = claimStatusToString(status);

    sqlite3_bind_text(stmt, 1, statusStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, reviewNotes.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, now.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, claimId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// ----------------- NOTIFICATION OPERATIONS -----------------

int Database::createNotification(const Notification& notif) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "INSERT INTO notifications (user_id, title, message, type, is_read, item_id, created_at) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    std::string now = notif.created_at.empty() ? currentTimestamp() : notif.created_at;

    sqlite3_bind_int(stmt, 1, notif.user_id);
    sqlite3_bind_text(stmt, 2, notif.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, notif.message.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, notif.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, notif.is_read ? 1 : 0);
    sqlite3_bind_int(stmt, 6, notif.item_id);
    sqlite3_bind_text(stmt, 7, now.c_str(), -1, SQLITE_TRANSIENT);

    int id = 0;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    }
    sqlite3_finalize(stmt);
    return id;
}

std::vector<Notification> Database::getNotificationsForUser(int userId, bool unreadOnly) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Notification> list;

    std::stringstream sql;
    sql << "SELECT id, user_id, title, message, type, is_read, item_id, created_at "
        << "FROM notifications WHERE user_id = ? ";
    if (unreadOnly) {
        sql << "AND is_read = 0 ";
    }
    sql << "ORDER BY id DESC LIMIT 50;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, userId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Notification n;
        n.id = sqlite3_column_int(stmt, 0);
        n.user_id = sqlite3_column_int(stmt, 1);
        n.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        n.message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        n.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        n.is_read = sqlite3_column_int(stmt, 5) != 0;
        n.item_id = sqlite3_column_int(stmt, 6);
        n.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        list.push_back(n);
    }
    sqlite3_finalize(stmt);
    return list;
}

bool Database::markNotificationRead(int notifId, int userId) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "UPDATE notifications SET is_read = 1 WHERE id = ? AND user_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, notifId);
    sqlite3_bind_int(stmt, 2, userId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::markAllNotificationsRead(int userId) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "UPDATE notifications SET is_read = 1 WHERE user_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, userId);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

} // namespace back2you
