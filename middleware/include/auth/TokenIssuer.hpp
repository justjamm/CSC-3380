#pragma once
#include "middleware/auth/AuthTypes.hpp"
#include <string_view>

namespace middleware::auth {

    class TokenIssuer {
        public:
            virtual ~TokenIssuer() = default;
            
            //Issue the token based on user and permissions
            [[nodiscard]] virtual TokenPair issue(std::string_view userId, std::string_view role) const = 0;
            
            //Refresh expired tokens
            [[nodiscard]] virtual std::string refresh(std::string_view refreshToken) const = 0;
        };
    }
}