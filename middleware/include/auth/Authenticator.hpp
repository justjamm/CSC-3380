#pragma once

#include "auth/TokenIssuer.hpp"
#include "auth/ValidAuth.hpp"

#include <expected>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_set>

namespace middleware::auth {

struct AuthenticatorConfig {
    int accessTokenExpireSecs = 900;
    int refreshTokenExpireSecs = 86400;
    std::string secret;
    std::string issuer;
    std::string audience;
};

using Config = AuthenticatorConfig;

class Authenticator final : public IAuthValidator, public ITokenIssuer {
public:
    explicit Authenticator(AuthenticatorConfig config);

    [[nodiscard]] std::expected<AuthContext, AuthError> authenticate(std::string_view token) const override;
    bool revoke(std::string_view token) override;
    [[nodiscard]] bool hasPermission(std::string_view token, std::string_view permission) const override;

    [[nodiscard]] TokenPair issue(std::string_view userId, std::string_view role) const override;
    [[nodiscard]] std::string refresh(std::string_view refreshToken) const override;

private:
    [[nodiscard]] bool isRevoked(std::string_view token) const;
    [[nodiscard]] std::string signToken(const Claims& claims) const;
    [[nodiscard]] Claims verifyAndDecode(std::string_view token) const;

    AuthenticatorConfig config_;

    mutable std::shared_mutex revokedMutex_;
    std::unordered_set<std::string> revokedTokens_;
};

}  // namespace middleware::auth
