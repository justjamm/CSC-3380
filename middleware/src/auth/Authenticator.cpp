#include "auth/Authenticator.hpp"

#include <jwt-cpp/jwt.h>

#include <algorithm>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace middleware::auth {
namespace {

std::string resolveUserId(const Claims& claims) {
    if (!claims.userId.empty()) {
        return claims.userId;
    }
    return claims.subject;
}

std::chrono::system_clock::time_point resolveExpiresAt(const Claims& claims) {
    if (claims.expiresAt != std::chrono::system_clock::time_point{}) {
        return claims.expiresAt;
    }
    return claims.expiry;
}

void syncCompatibilityFields(Claims& claims) {
    if (claims.userId.empty()) {
        claims.userId = claims.subject;
    }
    if (claims.subject.empty()) {
        claims.subject = claims.userId;
    }

    if (claims.expiresAt == std::chrono::system_clock::time_point{}) {
        claims.expiresAt = claims.expiry;
    }
    if (claims.expiry == std::chrono::system_clock::time_point{}) {
        claims.expiry = claims.expiresAt;
    }
}

std::string joinPermissions(const std::vector<std::string>& permissions) {
    std::ostringstream out;
    for (std::size_t i = 0; i < permissions.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << permissions[i];
    }
    return out.str();
}

std::vector<std::string> splitPermissions(const std::string& permissions) {
    std::vector<std::string> result;
    std::stringstream stream(permissions);
    std::string item;
    while (std::getline(stream, item, ',')) {
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    return result;
}

}  // namespace

Authenticator::Authenticator(Config config)
    : config_(std::move(config)) {}

std::expected<AuthContext, AuthError>
Authenticator::authenticate(std::string_view token) const {
    if (isRevoked(token)) {
        return std::unexpected(AuthError::TokenRevoked);
    }

    try {
        const Claims claims = verifyAndDecode(token);
        return AuthContext{
            .userId = resolveUserId(claims),
            .role = claims.role,
            .permissions = claims.permissions,
            .expiresAt = resolveExpiresAt(claims),
        };
    } catch (const jwt::error::token_verification_exception&) {
        return std::unexpected(AuthError::TokenExpired);
    } catch (const jwt::error::signature_verification_exception&) {
        return std::unexpected(AuthError::InvalidToken);
    } catch (const std::exception&) {
        return std::unexpected(AuthError::InvalidToken);
    }
}

bool Authenticator::revoke(std::string_view token) {
    std::unique_lock lock(revokedMutex_);
    const auto [_, inserted] = revokedTokens_.emplace(std::string(token));
    return inserted;
}

bool Authenticator::hasPermission(std::string_view token, std::string_view permission) const {
    const auto result = authenticate(token);
    if (!result) {
        return false;
    }

    const auto& perms = result->permissions;
    return std::find(perms.begin(), perms.end(), std::string(permission)) != perms.end();
}

TokenPair Authenticator::issue(std::string_view userId, std::string_view role) const {
    using namespace std::chrono;

    const auto now = system_clock::now();

    Claims accessClaims{};
    accessClaims.userId = std::string(userId);
    accessClaims.role = std::string(role);
    accessClaims.issuer = config_.issuer;
    accessClaims.audience = config_.audience;
    accessClaims.tokenType = "access";
    accessClaims.permissions = {};
    accessClaims.issuedAt = now;
    accessClaims.expiresAt = now + seconds(config_.accessTokenExpireSecs);
    syncCompatibilityFields(accessClaims);

    Claims refreshClaims = accessClaims;
    refreshClaims.tokenType = "refresh";
    refreshClaims.expiresAt = now + seconds(config_.refreshTokenExpireSecs);
    syncCompatibilityFields(refreshClaims);

    return TokenPair{
        .accessToken = signToken(accessClaims),
        .refreshToken = signToken(refreshClaims),
    };
}

std::string Authenticator::refresh(std::string_view refreshToken) const {
    const auto result = authenticate(refreshToken);
    if (!result) {
        throw std::runtime_error("Cannot refresh: invalid token");
    }
    return issue(result->userId, result->role).accessToken;
}

bool Authenticator::isRevoked(std::string_view token) const {
    std::shared_lock lock(revokedMutex_);
    return revokedTokens_.contains(std::string(token));
}

std::string Authenticator::signToken(const Claims& claims) const {
    if (config_.secret.empty()) {
        throw std::runtime_error("Authenticator secret must not be empty");
    }

    return jwt::create()
        .set_type("JWT")
        .set_subject(resolveUserId(claims))
        .set_issuer(claims.issuer)
        .set_audience(claims.audience)
        .set_issued_at(claims.issuedAt)
        .set_expires_at(resolveExpiresAt(claims))
        .set_payload_claim("role", jwt::claim(claims.role))
        .set_payload_claim("token_type", jwt::claim(claims.tokenType))
        .set_payload_claim("permissions", jwt::claim(joinPermissions(claims.permissions)))
        .sign(jwt::algorithm::hs256{config_.secret});
}

Claims Authenticator::verifyAndDecode(std::string_view token) const {
    if (config_.secret.empty()) {
        throw std::runtime_error("Authenticator secret must not be empty");
    }

    const auto decoded = jwt::decode(std::string(token));

    auto verifier = jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{config_.secret});
    if (!config_.issuer.empty()) {
        verifier.with_issuer(config_.issuer);
    }
    if (!config_.audience.empty()) {
        verifier.with_audience(config_.audience);
    }
    verifier.verify(decoded);

    Claims claims{};
    claims.userId = decoded.get_subject();
    claims.role = decoded.has_payload_claim("role")
        ? decoded.get_payload_claim("role").as_string()
        : std::string{};
    claims.issuer = config_.issuer;
    claims.audience = config_.audience;
    claims.tokenType = decoded.has_payload_claim("token_type")
        ? decoded.get_payload_claim("token_type").as_string()
        : std::string{};
    claims.permissions = decoded.has_payload_claim("permissions")
        ? splitPermissions(decoded.get_payload_claim("permissions").as_string())
        : std::vector<std::string>{};
    claims.issuedAt = decoded.get_issued_at();
    claims.expiresAt = decoded.get_expires_at();
    syncCompatibilityFields(claims);

    return claims;
}

}  // namespace middleware::auth
