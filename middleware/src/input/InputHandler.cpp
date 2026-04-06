#include "InputHandler.hpp"
#include <nlohmann/json.hpp>

namespace middleware::input {


    std::expected<std::string, InputError> InputParser::extractField(const nlohmann::json& json, std::string_view field) const
    {
        auto key = std::string(field);

        if (!json.contains(key)) {
            return std::unexpected(InputError::MissingField);
        }

        auto value = json.at(key).get<std::string>();

        if (value.empty()) {
            return std::unexpected(InputError::EmptyField);
        }

        return value;
    }


    std::expected<LoginRequest, InputError> InputParser::parseLogin(std::string_view body) const
    {
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

    std::expected<CameraSelectRequest, InputError> InputParser::parseCameraSelect(std::string_view body) const
    {
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

    std::expected<EdrDeviceRequest, InputError>
    InputParser::parseEdrRequest(std::string_view body) const
    {
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

}