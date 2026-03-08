#pragma once
#include <string>
#include <vector>
#include <chrono>

namespace middleware::auth {
    struct AuthContext {
        std::string userID;
        std::string role;
        std::vector<std::string> permissions;
        std::chrono::system_clock::time_point expiresAt;
    };

    struct TokenPair {
        std::string accessToken;
        std::string refreshToken;
    };

    enum class AuthError {
        InvalidToken,
        TokenExpired,
        TokenRevoked,
        InsufficientPermissions
    };
}