#pragma once
#include "middleware/auth/AuthTypes.hpp"
#include <string_view>
#include <expected>

namespace middleware::auth {
    class Authenticator {
        public:
        virtual ~Authenticator() = default;

        //Check token and authenthicate, otherwise output error
        [[nodiscard]] virtual std::expected<AuthContext, AuthError> authenticate(std::string_view token) const = 0;

        //Remove login token
        virtual bool revoke(std::string_view token) = 0;

        //Check specific permission
        [[nodiscard]] virtual bool hasPermission(std::string_view token, std::string_view permission) const = 0;
    };
}