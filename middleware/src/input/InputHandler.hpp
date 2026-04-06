#pragma once
#include "middleware/input/InputParser.hpp"

namespace middleware::input {

class InputParser final : public InputParser {
public:
    InputParser() = default;

    [[nodiscard]] std::expected<LoginRequest, InputError> parseLogin(std::string_view body) const override;

    [[nodiscard]] std::expected<CameraSelectRequest, InputError> parseCameraSelect(std::string_view body) const override;

    [[nodiscard]] std::expected<EdrDeviceRequest, InputError> parseEdrRequest(std::string_view body) const override;

private:
    [[nodiscard]] std::expected<std::string, InputError> extractField(const nlohmann::json& json, std::string_view field) const;
};

}