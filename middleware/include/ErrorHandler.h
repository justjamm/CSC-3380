#pragma once
#include <string>
#include <functional>
#include <stdexcept>
#include "Logger.h"

// ============================================================
// ErrorHandler — centralised error handling for the Duncan
// middle-layer service.
//
// All components (InputHandler, OutputHandler, StreamHandler,
// MiddleLogic) surface errors by throwing a MiddlewareError.
// The server's request loop calls ErrorHandler::handle() once
// per request so no scattered try/catch blocks are needed
// elsewhere.
//
// HTTP error-code mapping:
//   MiddlewareError(400) → 400 Bad Request
//   MiddlewareError(401) → 401 Unauthorized
//   MiddlewareError(503) → 503 Service Unavailable
//   Any other / std::exception → 500 Internal Server Error
// ============================================================

// ---- Domain exception ----
class MiddlewareError : public std::runtime_error {
public:
    explicit MiddlewareError(const std::string& msg, int httpStatus = 500)
        : std::runtime_error(msg), m_status(httpStatus) {}

    int status() const { return m_status; }

private:
    int m_status;
};

// ---- Specific typed errors (match your DFD components) ----

class AuthError : public MiddlewareError {
public:
    explicit AuthError(const std::string& msg)
        : MiddlewareError(msg, 401) {}
};

class BackendUnavailableError : public MiddlewareError {
public:
    explicit BackendUnavailableError(const std::string& msg)
        : MiddlewareError(msg, 503) {}
};

class BadRequestError : public MiddlewareError {
public:
    explicit BadRequestError(const std::string& msg)
        : MiddlewareError(msg, 400) {}
};

// ---- Centralized handler ----
class ErrorHandler {
public:
    // Wraps a handler lambda; catches all exceptions and maps them to
    // an HTTP status + JSON error body.
    //
    // Usage inside your httplib route:
    //   ErrorHandler::handle(req, res, [&](){
    //       // your logic here — throw MiddlewareError / AuthError / etc.
    //   });
    static void handle(
        const std::string& routeDescription,
        std::function<void()> handler,
        std::function<void(int, const std::string&)> respondFn)
    {
        try {
            handler();
        }
        catch (const MiddlewareError& e) {
            Logger::error(e.what(), "error-handler")
                .field("route", routeDescription)
                .field("http_status", e.status())
                .log();
            respondFn(e.status(), buildJsonError(e.what(), e.status()));
        }
        catch (const std::exception& e) {
            Logger::error(e.what(), "error-handler")
                .field("route", routeDescription)
                .field("http_status", 500)
                .log();
            respondFn(500, buildJsonError("Internal server error", 500));
        }
        catch (...) {
            Logger::error("Unknown exception caught", "error-handler")
                .field("route", routeDescription)
                .log();
            respondFn(500, buildJsonError("Internal server error", 500));
        }
    }

private:
    static std::string buildJsonError(const std::string& msg, int status) {
        // Simple inline build — no external JSON dependency needed
        return "{\"error\":{\"status\":" + std::to_string(status)
             + ",\"message\":\"" + jsonEscape(msg) + "\"}}";
    }

    static std::string jsonEscape(const std::string& s) {
        std::string out;
        for (char c : s) {
            if      (c == '"')  out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else                out += c;
        }
        return out;
    }
};
