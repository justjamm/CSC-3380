#include "Authenticator.hpp"
#include <jwt-cpp/jwt.h>
#include <chrono>

namespace middleware::auth {

Authenticator::Authenticator(Config config)
    : config_(std::move(config))

//Authenticator Part

std::expected<AuthContext, AuthError>
Authenticator::authenticate(std::string_view token) const
{
    //Check if revoked
    if (isRevoked(token)) {
        return std::unexpected(AuthError::TokenRevoked);
    }

    //Verify token
    Claims claims;
    try {
        claims = verifyAndDecode(token);
    } catch (const jwt::signature_verification_exception&) {
        return std::unexpected(AuthError::InvalidToken);
    } catch (const jwt::token_verification_exception&) {
        return std::unexpected(AuthError::TokenExpired);
    }

    //Return authenticated info
    return AuthContext{
        .userId = claims.subject,
        .role = claims.role,
        .permissions = claims.permissions,
        .expiresAt = claims.expiry
    };
}

bool Authenticator::revoke(std::string_view token)
{   
    //Revoke token
    std::unique_lock lock(revokedMutex_);
    auto [_, inserted] = revokedTokens_.emplace(token);
    return inserted;
}

bool Authenticator::hasPermission(std::string_view token, std::string_view permission) const
{   
    //Check permission
    auto result = authenticate(token);
    if (!result) return false;

    const auto& perms = result->permissions;
    return std::ranges::find(perms, permission) != perms.end();
}

//TokenIssuer part

TokenPair Authenticator::issue(std::string_view userId, std::string_view role) const
{
    using namespace std::chrono;
    auto now = system_clock::now();

    auto accessClaims = Claims{
        .subject = std::string(userId),
        .role = std::string(role),
        .issuer = config_.issuer,
        .audience = config_.audience,
        .issuedAt = now,
        .expiry = now + seconds(config_.accessTokenTtlSeconds)
    };

    auto refreshClaims  = accessClaims;
    refreshClaims.expiry = now + seconds(config_.refreshTokenTtlSeconds);

    return TokenPair{
        .accessToken  = signToken(accessClaims),
        .refreshToken = signToken(refreshClaims)
    };
}

std::string Authenticator::refresh(std::string_view refreshToken) const
{
    auto result = authenticate(refreshToken);
    if (!result) {
        throw std::runtime_error("Cannot refresh: invalid token");
    }
    //Issue new access token 
    return issue(result->userId, result->role).accessToken;
}

bool Authenticator::isRevoked(std::string_view token) const
{
    std::shared_lock lock(revokedMutex_);
    return revokedTokens_.contains(std::string(token));
}

}