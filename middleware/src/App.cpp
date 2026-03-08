#include "App.h"
#include "httplib.h"
#include "ErrorHandler.h"
#include "HealthCheck.h"
#include "Logger.h"

#include <stdexcept>
#include <string>

// ============================================================
// App.cpp — route registration + business logic wiring.
//
// Each route group maps directly to your DFD:
//
//   /health/**   → HealthCheck probes (liveness + readiness)
//   /auth/**     → Frontend → Database credential check
//   /devices     → InputHandler → Backend Manager
//   /stream/**   → StreamHandler → CameraStream → Manager
//   /edr/**      → MiddleLogic EDR → OutputHandler → Frontend
//
// ErrorHandler::handle() wraps every route so all exceptions
// surface as structured JSON — no per-route try/catch needed.
// ============================================================

// ---- Helper: build a minimal JSON object ----
static std::string json(std::initializer_list<std::pair<std::string,std::string>> kv) {
    std::string out = "{";
    bool first = true;
    for (auto& [k, v] : kv) {
        if (!first) out += ",";
        first = false;
        out += "\"" + k + "\":\"" + v + "\"";
    }
    return out + "}";
}

// ============================================================
// Construction / Destruction
// ============================================================

App::App(const Config& cfg)
    : m_cfg(cfg)
    , m_http(std::make_unique<httplib::Server>())
{}

App::~App() = default;

httplib::Server& App::httpServer() {
    return *m_http;
}

void App::stop() {
    Logger::info("App stopping HTTP server...", "app").log();
    m_http->stop();
}

// ============================================================
// setup() — wire everything together
// ============================================================

void App::setup() {
    Logger::info("App::setup() — registering routes", "app").log();

    // Global pre-routing hook: log every inbound request
    m_http->set_pre_routing_handler([](const httplib::Request& req,
                                   httplib::Response& /*res*/) {
    Logger::info("Inbound request", "router")
        .field("method", req.method)
        .field("path",   req.path)
        .log();
    return httplib::Server::HandlerResponse::Unhandled; // continue to matched route
    });

    // Register readiness probes (backend TCP reachability)
    HealthCheck::addReadinessProbe(makeBackendProbe());

    registerHealthRoutes();
    registerAuthRoutes();
    registerDeviceRoutes();
    registerStreamRoutes();
    registerEdrRoutes();

    Logger::info("All routes registered", "app").log();
}

// ============================================================
// Health routes
// ============================================================

void App::registerHealthRoutes() {
    // Combined liveness + readiness (used by load balancers & frontend)
    m_http->Get("/health", [](const httplib::Request& /*req*/,
                               httplib::Response& res) {
        auto [code, body] = HealthCheck::combined();
        res.status = code;
        res.set_content(body, "application/json");
    });

    // Liveness only — always 200 if process is alive
    m_http->Get("/health/live", [](const httplib::Request& /*req*/,
                                   httplib::Response& res) {
        auto [code, body] = HealthCheck::liveness();
        res.status = code;
        res.set_content(body, "application/json");
    });

    // Readiness — checks all registered probes (backend, DB, etc.)
    m_http->Get("/health/ready", [](const httplib::Request& /*req*/,
                                    httplib::Response& res) {
        auto [code, body] = HealthCheck::readiness();
        res.status = code;
        res.set_content(body, "application/json");
    });

    Logger::debug("Health routes registered: /health, /health/live, /health/ready", "app").log();
}

// ============================================================
// Auth routes  (Frontend → Database, per DFD)
// ============================================================

void App::registerAuthRoutes() {
    // POST /auth/login
    // Receives: {"username":"...","password":"...","otp":"..."}
    // Returns:  {"token":"<jwt>"}  or  401 AuthError
    m_http->Post("/auth/login", [this](const httplib::Request& /*req*/,
                                       httplib::Response& res) {
        ErrorHandler::handle(
            "POST /auth/login",
            [&]() {
                // TODO (Milestone 3): parse body JSON, validate credentials
                // against DB, verify TOTP OTP, sign JWT with m_cfg.jwtSecret
                Logger::info("Login attempt", "auth").log();

                // Placeholder — replace with real DB + OTP check
                res.set_content(
                    json({{"status","stub"},{"token","<replace-with-jwt>"}}),
                    "application/json");
            },
            [&](int code, const std::string& body) {
                res.status = code;
                res.set_content(body, "application/json");
            });
    });

    Logger::debug("Auth routes registered: POST /auth/login", "app").log();
}

// ============================================================
// Device routes  (InputHandler → Backend Manager, per DFD)
// ============================================================

void App::registerDeviceRoutes() {
    // GET /devices — list all known IoT devices from backend
    m_http->Get("/devices", [this](const httplib::Request& req,
                                   httplib::Response& res) {
        ErrorHandler::handle(
            "GET /devices",
            [&]() {
                requireAuth(req.get_header_value("Authorization"));

                // TODO (Milestone 3): forward to InputHandler → Backend Manager
                // Example: auto devices = m_inputHandler->listDevices();
                Logger::info("Device list requested", "devices").log();

                res.set_content(
                    R"({"devices":[],"note":"stub — wire InputHandler here"})",
                    "application/json");
            },
            [&](int code, const std::string& body) {
                res.status = code;
                res.set_content(body, "application/json");
            });
    });

    Logger::debug("Device routes registered: GET /devices", "app").log();
}

// ============================================================
// Stream routes  (StreamHandler → CameraStream → Manager, per DFD)
// ============================================================

void App::registerStreamRoutes() {
    // GET /stream/:id — request camera feed for device <id>
    m_http->Get("/stream/:id", [this](const httplib::Request& req,
                                      httplib::Response& res) {
        ErrorHandler::handle(
            "GET /stream/:id",
            [&]() {
                requireAuth(req.get_header_value("Authorization"));

                std::string deviceId = req.get_param_value("id");
                Logger::info("Camera stream requested", "stream")
                    .field("device_id", deviceId)
                    .log();

                // TODO (Milestone 3): m_streamHandler->openStream(deviceId)
                // For RTSP: return stream URL or proxy frames to frontend
                res.set_content(
                    json({{"device_id", deviceId},
                          {"rtsp_url",  "rtsp://stub — wire StreamHandler here"}}),
                    "application/json");
            },
            [&](int code, const std::string& body) {
                res.status = code;
                res.set_content(body, "application/json");
            });
    });

    Logger::debug("Stream routes registered: GET /stream/:id", "app").log();
}

// ============================================================
// EDR routes  (MiddleLogic → OutputHandler → Frontend, per DFD)
// ============================================================

void App::registerEdrRoutes() {
    // GET /edr/alerts — latest EDR anomaly detections
    m_http->Get("/edr/alerts", [this](const httplib::Request& req,
                                      httplib::Response& res) {
        ErrorHandler::handle(
            "GET /edr/alerts",
            [&]() {
                requireAuth(req.get_header_value("Authorization"));

                // TODO (Milestone 3): m_middleLogic->getAlerts()
                Logger::info("EDR alert list requested", "edr").log();

                res.set_content(
                    R"({"alerts":[],"note":"stub — wire MiddleLogic EDR here"})",
                    "application/json");
            },
            [&](int code, const std::string& body) {
                res.status = code;
                res.set_content(body, "application/json");
            });
    });

    Logger::debug("EDR routes registered: GET /edr/alerts", "app").log();
}

// ============================================================
// Auth middleware helper
// ============================================================

void App::requireAuth(const std::string& authHeader) {
    // Expects:  Authorization: Bearer <token>
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ")
        throw AuthError("Missing or malformed Authorization header");

    std::string token = authHeader.substr(7);
    if (token.empty())
        throw AuthError("Empty bearer token");

    // TODO (Milestone 3): validate JWT signature using m_cfg.jwtSecret
    // e.g. jwt::decode(token, m_cfg.jwtSecret);
    Logger::debug("Auth check passed (stub)", "auth").log();
}

// ============================================================
// Readiness probe: backend TCP reachability
// ============================================================

ReadinessProbe App::makeBackendProbe() const {
    std::string host = m_cfg.backendHost;
    int         port = m_cfg.backendPort;

    return [host, port]() -> ProbeResult {
        // TODO (Milestone 3): attempt a real TCP connect to the backend
        // For now this is a stub that always returns ok.
        // Replace with: httplib::Client c(host, port); auto res = c.Get("/ping");
        (void)host; (void)port;
        return ProbeResult{"backend", true, ""};
    };
}
