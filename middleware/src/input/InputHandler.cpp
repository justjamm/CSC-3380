#include "input/InputHandler.hpp"

#include <utility>

namespace middleware::input {

std::expected<std::string, InputError> InputHandler::extractField(
    const nlohmann::json& json,
    std::string_view field
) const {
    auto key = std::string(field);

    if (!json.contains(key)) {
        return std::unexpected(InputError::MissingField);
    }

    std::string value;
    try {
        value = json.at(key).get<std::string>();
    } catch (...) {
        return std::unexpected(InputError::MalformedJson);
    }

    if (value.empty()) {
        return std::unexpected(InputError::EmptyField);
    }

    return value;
}

std::expected<LoginRequest, InputError> InputHandler::parseLogin(std::string_view body) const {
    nlohmann::json json;

    try {
        json = nlohmann::json::parse(body);
    } catch (...) {
        return std::unexpected(InputError::MalformedJson);
    }

    auto username = extractField(json, "username");
    if (!username) return std::unexpected(username.error());

    auto password = extractField(json, "password");
    if (!password) return std::unexpected(password.error());

    return LoginRequest{
        .username = std::move(*username),
        .password = std::move(*password)
    };
}

std::expected<OtpVerifyRequest, InputError> InputHandler::parseOtpVerify(std::string_view body) const {
    nlohmann::json json;

    try {
        json = nlohmann::json::parse(body);
    } catch (...) {
        return std::unexpected(InputError::MalformedJson);
    }

    auto username = extractField(json, "username");
    if (!username) return std::unexpected(username.error());

    auto otp = extractField(json, "otp");
    if (!otp) return std::unexpected(otp.error());

    return OtpVerifyRequest{
        .username = std::move(*username),
        .otp = std::move(*otp)
    };
}

std::expected<CameraSelectRequest, InputError> InputHandler::parseCameraSelect(std::string_view body) const {
    nlohmann::json json;

    try {
        json = nlohmann::json::parse(body);
    } catch (...) {
        return std::unexpected(InputError::MalformedJson);
    }

    auto cameraId = extractField(json, "cameraId");
    if (!cameraId) return std::unexpected(cameraId.error());

    return CameraSelectRequest{
        .cameraId = std::move(*cameraId)
    };
}

std::expected<EdrDeviceRequest, InputError> InputHandler::parseEdrRequest(std::string_view body) const {
    nlohmann::json json;

    try {
        json = nlohmann::json::parse(body);
    } catch (...) {
        return std::unexpected(InputError::MalformedJson);
    }

    auto deviceId = extractField(json, "deviceId");
    if (!deviceId) return std::unexpected(deviceId.error());

    return EdrDeviceRequest{
        .deviceId = std::move(*deviceId)
    };
}

}  // namespace middleware::input
