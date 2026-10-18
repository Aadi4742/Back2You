#pragma once

#include <string>

namespace back2you {

class Crypto {
public:
    // Generate secure random hex string of given byte length
    static std::string generateRandomHex(size_t numBytes = 16);

    // Compute SHA-256 hex digest
    static std::string sha256(const std::string& input);

    // Salt and hash password: SHA256(password + ":" + salt)
    static std::string hashPassword(const std::string& password, const std::string& salt);

    // Verify password against salt and expected hash
    static bool verifyPassword(const std::string& password, const std::string& salt, const std::string& expectedHash);

    // Generate secure session token
    static std::string generateToken();
};

} // namespace back2you
