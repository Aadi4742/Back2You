// Back2You - College Campus Lost and Found Project
// Developed by Sameer Chand & Aditya Rai
// A simple C++17 backend with SQLite for managing lost/found reports on campus

#include <iostream>
#include <string>
#include <cstdlib>
#include <httplib.h>
#include "database/Database.h"
#include "routes/Router.h"
#include "utils/Crypto.h"

using namespace back2you;

// Sameer: populate default test items on first run so the site isn't blank
void seedInitialData() {
    auto& db = Database::getInstance();

    // default admin account for grading/demo purposes
    if (!db.getUserByUsername("admin").has_value()) {
        std::string salt = Crypto::generateRandomHex(16);
        std::string hash = Crypto::hashPassword("Admin@123", salt);

        User admin;
        admin.username = "admin";
        admin.email = "admin@back2you.campus.edu";
        admin.password_hash = hash;
        admin.salt = salt;
        admin.role = UserRole::Admin;
        admin.full_name = "System Administrator";
        admin.phone = "+1-800-555-0199";

        int adminId = db.createUser(admin);
        std::cout << "[Seed] Created default administrator account (username: 'admin', pass: 'Admin@123', id: " << adminId << ")" << std::endl;
    }

    // 2. Ensure default demo student account exists
    if (!db.getUserByUsername("student1").has_value()) {
        std::string salt = Crypto::generateRandomHex(16);
        std::string hash = Crypto::hashPassword("Student@123", salt);

        User student;
        student.username = "student1";
        student.email = "alex.rivera@campus.edu";
        student.password_hash = hash;
        student.salt = salt;
        student.role = UserRole::User;
        student.full_name = "Alex Rivera";
        student.phone = "+1-555-014-8822";

        int studentId = db.createUser(student);

        // Seed initial items if empty
        if (db.countItems() == 0) {
            Item it1;
            it1.user_id = studentId;
            it1.type = ItemType::Lost;
            it1.title = "MacBook Air M2 13-inch (Space Gray)";
            it1.category_id = 1; // Electronics
            it1.description = "Left in Library 2nd floor reading room. Has a GitHub and React sticker on the lid.";
            it1.location = "Central Library - 2nd Floor";
            it1.date_event = "2026-10-15";
            it1.image_path = "https://images.unsplash.com/photo-1517336714731-489689fd1ca8?auto=format&fit=crop&w=800&q=80";
            it1.contact_details = "alex.rivera@campus.edu | Desk #14";
            it1.status = ItemStatus::Lost;
            db.createItem(it1);

            Item it2;
            it2.user_id = 1; // admin/staff
            it2.type = ItemType::Found;
            it2.title = "Brown Leather Wallet with Student ID";
            it2.category_id = 2; // Wallets & IDs
            it2.description = "Found near cafeteria cash counter during lunch hours. Contains cash and a campus meal card.";
            it2.location = "Student Union Cafeteria";
            it2.date_event = "2026-10-16";
            it2.image_path = "https://images.unsplash.com/photo-1627123424574-724758594e93?auto=format&fit=crop&w=800&q=80";
            it2.contact_details = "security@campus.edu";
            it2.held_at = "Campus Security Central Desk (Room 102)";
            it2.status = ItemStatus::Found;
            db.createItem(it2);

            Item it3;
            it3.user_id = studentId;
            it3.type = ItemType::Lost;
            it3.title = "Casio fx-991EX Scientific Calculator";
            it3.category_id = 4; // Books & Stationery
            it3.description = "Classwiz solar calculator forgotten after Calculus midterm examination.";
            it3.location = "Science Block - Lecture Hall B";
            it3.date_event = "2026-10-14";
            it3.image_path = "https://images.unsplash.com/photo-1594980596870-8aa52a78d8cd?auto=format&fit=crop&w=800&q=80";
            it3.contact_details = "alex.rivera@campus.edu";
            it3.status = ItemStatus::Lost;
            db.createItem(it3);

            Item it4;
            it4.user_id = 1;
            it4.type = ItemType::Found;
            it4.title = "Set of 3 Keys on Red Lanyard";
            it4.category_id = 3; // Keys
            it4.description = "Dorm room key and bike lock key on a red collegiate lanyard found on bench.";
            it4.location = "Sports Complex & Gymnasium Bench";
            it4.date_event = "2026-10-17";
            it4.image_path = "https://images.unsplash.com/photo-1582139329536-e7284fece509?auto=format&fit=crop&w=800&q=80";
            it4.contact_details = "gym.reception@campus.edu";
            it4.held_at = "Gymnasium Reception Front Desk";
            it4.status = ItemStatus::Found;
            db.createItem(it4);

            std::cout << "[Seed] Initial sample items created." << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << R"(
  ==============================================================
    ____             _      ____  __   __            
   |  _ \           | |    |___ \ \ \ / /            
   | |_) | __ _  ___| | __   __) | \ V /___  _   _   
   |  _ < / _` |/ __| |/ /  |__ <   | // _ \| | | |  
   | |_) | (_| | (__|   <   ___) |  | | (_) | |_| |  
   |____/ \__,_|\___|_|\_\ |____/   |_|\___/ \__,_|  
                                                     
   Lost & Found Campus Management Platform (C++ Backend)
   Designed & Developed by Sameer Chand & Aditya Rai
  ==============================================================
    )" << std::endl;

    // open sqlite database file
    std::string dbPath = "back2you.db";
    if (argc > 1) {
        dbPath = argv[1];
    }
    if (!Database::getInstance(dbPath).init()) {
        std::cerr << "[Fatal] Failed to initialize SQLite database." << std::endl;
        return 1;
    }

    // insert initial demo records if this is a fresh database
    seedInitialData();

    // start http listener
    httplib::Server server;

    // register endpoints and static files
    Router::setupRoutes(server);
    server.set_mount_point("/", "./frontend");

    // Port configuration
    int port = 8080;
    const char* envPort = std::getenv("PORT");
    if (envPort) {
        try { port = std::stoi(envPort); } catch (...) {}
    }

    std::cout << "\n[Back2You Server] Listening on http://0.0.0.0:" << port << std::endl;
    std::cout << "[Back2You Server] API Documentation: ./API_DOCS.md" << std::endl;
    std::cout << "[Back2You Server] Health check: http://localhost:" << port << "/api/health\n" << std::endl;

    if (!server.listen("0.0.0.0", port)) {
        std::cerr << "[Fatal] Server failed to start on port " << port << std::endl;
        return 1;
    }

    return 0;
}
