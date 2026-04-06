#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace middleware::auth {

struct AuthContext {
    std::string userId;
    std::string role;
    std::vector<std::string> permissions;
    std::chrono::system_clock::time_point expiresAt;
};

struct Claims {
    std::string userId;
    std::string role;
    std::string issuer;
    std::string audience;
    std::string tokenType;
    std::vector<std::string> permissions;
    std::chrono::system_clock::time_point issuedAt;
    std::chrono::system_clock::time_point expiresAt;

    // Compatibility fields kept during merge reconciliation.
    std::string subject;
    std::chrono::system_clock::time_point expiry{};
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

}  // namespace middleware::auth
