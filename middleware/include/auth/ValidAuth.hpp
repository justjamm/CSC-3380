#pragma once

#include "auth/AuthTypes.hpp"

#include <expected>
#include <string_view>

namespace middleware::auth {

class IAuthValidator {
public:
    virtual ~IAuthValidator() = default;

    [[nodiscard]] virtual std::expected<AuthContext, AuthError> authenticate(std::string_view token) const = 0;
    virtual bool revoke(std::string_view token) = 0;
    [[nodiscard]] virtual bool hasPermission(std::string_view token, std::string_view permission) const = 0;
};

using ValidAuth = IAuthValidator;

}  // namespace middleware::auth
