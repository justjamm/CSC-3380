#pragma once
#include "middleware/input/InputTypes.hpp"
#include <expected>
#include <string_view>

namespace middleware::input {

    class InputParser {
    public:
        virtual ~IInputParser() = default;

        [[nodiscard]] virtual std::expected<LoginRequest, InputError> parseLogin(std::string_view body) const = 0;

        [[nodiscard]] virtual std::expected<CameraSelectRequest, InputError> parseCameraSelect(std::string_view body) const = 0;

        [[nodiscard]] virtual std::expected<EdrDeviceRequest, InputError> parseEdrRequest(std::string_view body) const = 0;
    };

}