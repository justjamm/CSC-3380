#pragma once
#include <string>

namespace middleware::input {

    struct LoginRequest {
        std::string username;
        std::string password;
    };

    struct OtpVerifyRequest {
        std::string username;
        std::string otp;
    };

    struct CameraSelectRequest {
        std::string cameraId;
    };

    struct EdrDeviceRequest {
        std::string deviceId;
    };

    enum class InputError {
        MalformedJson,
        MissingField,
        EmptyField
    };

}
