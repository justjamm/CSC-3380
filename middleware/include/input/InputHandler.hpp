#pragma once

#include "input/InputParser.hpp"

#include <expected>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

namespace middleware::input {

class InputHandler final : public IInputParser {
public:
    InputHandler() = default;

    [[nodiscard]] std::expected<LoginRequest, InputError> parseLogin(std::string_view body) const override;
    [[nodiscard]] std::expected<OtpVerifyRequest, InputError> parseOtpVerify(std::string_view body) const override;
    [[nodiscard]] std::expected<CameraSelectRequest, InputError> parseCameraSelect(std::string_view body) const override;
    [[nodiscard]] std::expected<EdrDeviceRequest, InputError> parseEdrRequest(std::string_view body) const override;

private:
    [[nodiscard]] std::expected<std::string, InputError> extractField(
        const nlohmann::json& json,
        std::string_view field
    ) const;
};

}  // namespace middleware::input
