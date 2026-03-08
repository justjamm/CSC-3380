#pragma once
#include <string>
#include <functional>
#include <vector>
#include <chrono>
#include "Logger.h"
#include "Config.h"

// ============================================================
// HealthCheck — liveness and readiness probes for GET /health.
//
// Liveness  → "is this process alive?"
//   Always returns 200.  If the process is running, it's alive.
//
// Readiness → "is this service ready to serve traffic?"
//   Runs all registered ReadinessProbe callbacks.
//   Returns 200 only when ALL probes pass; otherwise 503.
//
// Per your DFD, the middle layer depends on:
//   - Backend Manager (camera/device stream source)
//   - Database (user credentials / JWT)
// Register a probe for each using addReadinessProbe().
//
// Response body (both endpoints) is JSON:
//   {"status":"ok","checks":{"backend":"ok","db":"ok"}}
//   {"status":"degraded","checks":{"backend":"unreachable","db":"ok"}}
// ============================================================

struct ProbeResult {
    std::string name;
    bool        ok;
    std::string detail; // optional human-readable detail
};

using ReadinessProbe = std::function<ProbeResult()>;

class HealthCheck {
public:
    // ---- Register a dependency check ----
    static void addReadinessProbe(ReadinessProbe probe) {
        s_probes.push_back(std::move(probe));
    }

    // ---- GET /health/live ----
    // Always 200 — if the process is running, liveness passes.
    static std::pair<int, std::string> liveness() {
        Logger::debug("Liveness probe hit", "health").log();
        return {200, R"({"status":"ok","probe":"liveness"})"};
    }

    // ---- GET /health/ready ----
    // Runs all registered probes; 503 if any fail.
    static std::pair<int, std::string> readiness() {
        auto start = std::chrono::steady_clock::now();

        bool allOk = true;
        std::string checks = "\"checks\":{";
        bool first = true;

        for (const auto& probe : s_probes) {
            ProbeResult r = probe();
            if (!r.ok) allOk = false;

            if (!first) checks += ",";
            first = false;
            checks += "\"" + r.name + "\":{\"status\":\""
                    + (r.ok ? "ok" : "unreachable")
                    + "\"";
            if (!r.detail.empty())
                checks += ",\"detail\":\"" + r.detail + "\"";
            checks += "}";
        }
        checks += "}";

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - start).count();

        std::string status = allOk ? "ok" : "degraded";
        int httpStatus     = allOk ? 200 : 503;

        std::string body = "{\"status\":\"" + status + "\","
                         + checks
                         + ",\"latency_ms\":" + std::to_string(ms)
                         + "}";

        if (!allOk)
            Logger::warn("Readiness probe failed", "health").field("body", body).log();
        else
            Logger::debug("Readiness probe passed", "health").log();

        return {httpStatus, body};
    }

    // ---- Combined GET /health (liveness + readiness) ----
    static std::pair<int, std::string> combined() {
        auto [code, body] = readiness();
        return {code, body};
    }

private:
    static std::vector<ReadinessProbe> s_probes;
};

// Static member definition
inline std::vector<ReadinessProbe> HealthCheck::s_probes;
