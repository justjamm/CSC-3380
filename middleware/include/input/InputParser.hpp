#pragma once

#include "input/InputTypes.hpp"

#include <expected>
#include <string_view>

namespace middleware::input {

class IInputParser {
public:
    virtual ~IInputParser() = default;

    [[nodiscard]] virtual std::expected<LoginRequest, InputError> parseLogin(std::string_view body) const = 0;
    [[nodiscard]] virtual std::expected<OtpVerifyRequest, InputError> parseOtpVerify(std::string_view body) const = 0;

    [[nodiscard]] virtual std::expected<CameraSelectRequest, InputError> parseCameraSelect(std::string_view body) const = 0;

    [[nodiscard]] virtual std::expected<EdrDeviceRequest, InputError> parseEdrRequest(std::string_view body) const = 0;
};

using InputParser = IInputParser;

}  // namespace middleware::input
