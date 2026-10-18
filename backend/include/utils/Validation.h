#pragma once

#include <string>
#include <regex>

namespace back2you {

class Validation {
public:
    static bool isValidEmail(const std::string& email) {
        if (email.empty() || email.size() > 254) return false;
        // Standard email regex check
        const std::regex pattern(R"(^[a-zA-Z0-9_.+-]+@[a-zA-Z0-9-]+\.[a-zA-Z0-9-.]+$)");
        return std::regex_match(email, pattern);
    }

    static bool isValidUsername(const std::string& username) {
        if (username.size() < 3 || username.size() > 32) return false;
        const std::regex pattern(R"(^[a-zA-Z0-9_.-]+$)");
        return std::regex_match(username, pattern);
    }

    static bool isValidDate(const std::string& date) {
        // YYYY-MM-DD format
        const std::regex pattern(R"(^\d{4}-\d{2}-\d{2}$)");
        return std::regex_match(date, pattern);
    }

    static std::string sanitize(const std::string& input) {
        std::string out;
        out.reserve(input.size());
        for (char c : input) {
            // strip control chars
            if (static_cast<unsigned char>(c) >= 32 || c == '\n' || c == '\r' || c == '\t') {
                out += c;
            }
        }
        return out;
    }
};

} // namespace back2you
