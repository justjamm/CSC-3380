#pragma once
#include <string>
#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

// ============================================================
// Config — loads and validates environment variables for the
// Duncan Home Security middle-layer service.
//
// Load order (later values win):
//   1. .env file (if present in working directory)
//   2. Real environment variables
//
// Call Config::load() once at startup; it throws
// ConfigError immediately if any required key is absent or
// malformed, giving a "fail-fast" guarantee before the server
// binds to any port.
// ============================================================

class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& msg)
        : std::runtime_error("[CONFIG ERROR] " + msg) {}
};

struct Config {
    // ---- Network ----
    int         port;               // PORT (default 8080)
    std::string host;               // HOST (default 0.0.0.0)

    // ---- Backend connection ----
    std::string backendHost;        // BACKEND_HOST (required)
    int         backendPort;        // BACKEND_PORT (required)
    
    // ---- Stream source connection ----
    std::string streamHost;         // STREAM_HOST (optional; defaults to BACKEND_HOST)
    int         streamPort;         // STREAM_PORT (optional; defaults to BACKEND_PORT)

    // ---- Frontend / Auth ----
    std::string jwtSecret;          // JWT_SECRET (required)

    // ---- Logging ----
    std::string logLevel;           // LOG_LEVEL: debug|info|warn|error (default info)

    // ---- EDR tuning ----
    int         edrPollIntervalMs;  // EDR_POLL_MS (default 500)

    // ---- Static factory ----
    static Config load() {
        // 1. Parse .env file (best-effort; missing file is silently skipped)
        loadDotEnv(".env");

        Config c;

        // Optional with defaults
        c.host              = getOr("HOST",     "0.0.0.0");
        c.port              = getIntOr("PORT",  8080);
        c.logLevel          = getOr("LOG_LEVEL","info");
        c.edrPollIntervalMs = getIntOr("EDR_POLL_MS", 500);

        // Required — will throw ConfigError if missing
        c.backendHost = getRequired("BACKEND_HOST");
        c.backendPort = getRequiredInt("BACKEND_PORT");
        c.jwtSecret   = getRequired("JWT_SECRET");
        
        // Optional stream source — defaults to backend if not overridden
        c.streamHost = getOr("STREAM_HOST", c.backendHost);
        c.streamPort = getIntOr("STREAM_PORT", c.backendPort);

        // Validate log level enum
        if (c.logLevel != "debug" && c.logLevel != "info" &&
            c.logLevel != "warn"  && c.logLevel != "error") {
            throw ConfigError("LOG_LEVEL must be one of: debug, info, warn, error");
        }

        // Validate port range
        if (c.port < 1 || c.port > 65535)
            throw ConfigError("PORT must be between 1 and 65535");
        if (c.backendPort < 1 || c.backendPort > 65535)
            throw ConfigError("BACKEND_PORT must be between 1 and 65535");
        if (c.streamPort < 1 || c.streamPort > 65535)
            throw ConfigError("STREAM_PORT must be between 1 and 65535");

        return c;
    }

private:
    // ---- Helpers ----
    static std::string getRequired(const std::string& key) {
        const char* val = std::getenv(key.c_str());
        if (!val || std::string(val).empty())
            throw ConfigError("Required environment variable '" + key + "' is not set.");
        return std::string(val);
    }

    static int getRequiredInt(const std::string& key) {
        std::string s = getRequired(key);
        try { return std::stoi(s); }
        catch (...) {
            throw ConfigError("'" + key + "' must be a valid integer, got: " + s);
        }
    }

    static std::string getOr(const std::string& key, const std::string& def) {
        const char* val = std::getenv(key.c_str());
        return (val && !std::string(val).empty()) ? std::string(val) : def;
    }

    static int getIntOr(const std::string& key, int def) {
        const char* val = std::getenv(key.c_str());
        if (!val || std::string(val).empty()) return def;
        try { return std::stoi(std::string(val)); }
        catch (...) {
            throw ConfigError("'" + key + "' must be a valid integer, got: "
                              + std::string(val));
        }
    }

    // Parse a simple KEY=VALUE .env file; sets env vars only if not already set
    static void loadDotEnv(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) return; // silently skip missing file

        std::string line;
        while (std::getline(f, line)) {
            // Strip inline comments and leading whitespace
            auto comment = line.find('#');
            if (comment != std::string::npos) line = line.substr(0, comment);
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t'))
                line.erase(line.begin());
            if (line.empty()) continue;

            auto eq = line.find('=');
            if (eq == std::string::npos) continue;

            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);

            // Trim trailing whitespace
            while (!key.empty() && key.back() == ' ') key.pop_back();
            while (!val.empty() && val.back() == ' ') val.pop_back();

            // Only set if not already in environment (env vars take priority)
            if (!key.empty() && std::getenv(key.c_str()) == nullptr)
                setenv(key.c_str(), val.c_str(), 0);
        }
    }
};
