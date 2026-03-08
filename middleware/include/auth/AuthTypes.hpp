#pragma once
#include <string>
#include <vector>
#include <chrono>

namespace middleware::auth {
    //Assign Info
    struct AuthContext {
        std::string userID;
        std::string role;
        std::vector<std::string> permissions;
        std::chrono::system_clock::time_point expiresAt;
    };

    //Verify Token
    struct Claims {
        std::string subject;      
        std::string role;
        std::string issuer;
        std::string audience;
        std::string tokenType;
        std::vector<std::string> permissions;
        std::chrono::system_clock::time_point issuedAt;
        std::chrono::system_clock::time_point expiry;
    };

    //Token
    struct TokenPair {
        std::string accessToken;
        std::string refreshToken;
    };

    //Token Errors
    enum class AuthError {
        InvalidToken,
        TokenExpired,
        TokenRevoked,
        InsufficientPermissions
    };
}