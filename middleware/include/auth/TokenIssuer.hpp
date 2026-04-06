#pragma once

#include "auth/AuthTypes.hpp"

#include <string>
#include <string_view>

namespace middleware::auth {

class ITokenIssuer {
public:
    virtual ~ITokenIssuer() = default;

    [[nodiscard]] virtual TokenPair issue(std::string_view userId, std::string_view role) const = 0;
    [[nodiscard]] virtual std::string refresh(std::string_view refreshToken) const = 0;
};

using TokenIssuer = ITokenIssuer;

}  // namespace middleware::auth
