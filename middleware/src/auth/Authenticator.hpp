#pragma once
#include "middleware/auth/ValidAuth.hpp"
#include "middleware/auth/TokenIssuer.hpp"
#include <memory>
#include <unordered_set>
#include <shared_mutex>
#include <string>

namespace middleware::auth {

    struct Config {
        int accessTokenExpireSecs = 900;    // 15 min to login in with access token
        int refreshTokenExpireSecs = 86400;  // 24 hr before they have to login again
        std::string secret;
        std::string issuer;
        std::string audience;
    };

    //Implements both interfaces — one class, two contracts
    class Authenticator final
        : public ValidAuth
        , public TokenIssuer
    {
        public:
            explicit Authenticator(Config config);

            //ValidAuth
            [[nodiscard]] std::expected<AuthContext, AuthError> authenticate(std::string_view token) const override;
            bool revoke(std::string_view token) override;
            [[nodiscard]] bool hasPermission(std::string_view token, std::string_view permission) const override;

            //TokenIssuer
            [[nodiscard]] TokenPair issue(std::string_view userId, std::string_view role) const override;
            [[nodiscard]] std::string refresh(std::string_view refreshToken) const override;

        private:
            [[nodiscard]] bool isRevoked(std::string_view token) const;
            [[nodiscard]] std::string signToken(const Claims& claims) const;
            [[nodiscard]] Claims verifyAndDecode(std::string_view token) const;

            Config config_;

            mutable std::shared_mutex revokedMutex_;
            std::unordered_set<std::string> revokedTokens_;
    };

}