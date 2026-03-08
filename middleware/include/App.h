#pragma once
#include <string>
#include <memory>

// Forward-declare httplib to avoid pulling the full header into
// every translation unit — only App.cpp needs it fully.
namespace httplib { class Server; }

#include "Config.h"
#include "Logger.h"
#include "HealthCheck.h"
#include "ErrorHandler.h"

// ============================================================
// App — application layer for the Duncan middle-layer service.
//
// Responsibilities (mirrors Logic End in your component diagram):
//   - Register all HTTP routes
//   - Wire MiddleLogic, InputHandler, OutputHandler, StreamHandler
//   - Register readiness probes (backend, DB)
//   - Provide setup() and stop() lifecycle methods
//
// App is deliberately separate from Server so that:
//   a) routes can be unit-tested without binding a port
//   b) Server owns the TCP lifecycle; App owns the business logic
//
// DFD mapping:
//   POST /auth/login     → validates credentials → issues JWT token
//   GET  /devices        → InputHandler → Backend Manager
//   GET  /stream/:id     → StreamHandler → CameraStream → Manager
//   GET  /edr/alerts     → MiddleLogic EDR → OutputHandler → Frontend
//   GET  /health         → HealthCheck (liveness + readiness)
//   GET  /health/live    → HealthCheck::liveness()
//   GET  /health/ready   → HealthCheck::readiness()
// ============================================================

class App {
public:
    explicit App(const Config& cfg);
    ~App();

    // Register all routes onto the internal httplib::Server instance.
    // Called by Server before it starts listening.
    void setup();

    // Return the underlying httplib::Server so Server.cpp can call listen().
    httplib::Server& httpServer();

    // Graceful stop — called by ShutdownManager hook
    void stop();

private:
    Config                          m_cfg;
    std::unique_ptr<httplib::Server> m_http;

    // ---- Route handlers ----
    void registerHealthRoutes();
    void registerAuthRoutes();
    void registerDeviceRoutes();
    void registerStreamRoutes();
    void registerEdrRoutes();

    // ---- Middleware helpers ----
    // Returns true and populates outToken if Authorization header is valid JWT.
    // Throws AuthError otherwise.
    void requireAuth(const std::string& authHeader);

    // ---- Readiness probe factories ----
    ReadinessProbe makeBackendProbe() const;
};
