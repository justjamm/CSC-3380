#include "App.h"
#include "httplib.h"
#include "ErrorHandler.h"
#include "HealthCheck.h"
#include "Logger.h"

#include <algorithm>
#include <atomic>
#include <any>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

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

namespace {

using Json = nlohmann::json;

constexpr std::string_view kPendingAuthPrefix = "auth.pending.";
constexpr std::string_view kCameraSelectedKey = "camera.selected";
constexpr std::string_view kDevicesLatestKey = "devices.latest";
constexpr std::string_view kAlertsLatestKey = "alerts.latest";
constexpr std::string_view kRealtimePath = "/ws";
constexpr std::string_view kDevelopmentOtp = "123456";
constexpr std::int64_t kPendingOtpLifetimeSecs = 300;
constexpr std::int64_t kRealtimeHeartbeatIntervalSecs = 15;

struct RealtimeSession {
    std::uint64_t id{0};
    std::string userId;
    std::string role;
    httplib::ws::WebSocket* socket{nullptr};
};

std::mutex g_realtimeSessionsMutex;
std::unordered_map<std::uint64_t, RealtimeSession> g_realtimeSessions;
std::atomic<std::uint64_t> g_nextRealtimeSessionId{1};
std::atomic<std::uint64_t> g_nextRealtimeEventId{1};
std::atomic<bool> g_realtimeHeartbeatRunning{false};
std::thread g_realtimeHeartbeatThread;
std::mutex g_realtimeHeartbeatMutex;

class InMemoryStateStore final : public middleware::state::IStateStore {
public:
    void set(const middleware::state::StateKey& key,
             middleware::state::StateValue value,
             middleware::state::StateScope scope) override {
        entries_[key] = Entry{std::move(value), scope};
    }

    void remove(const middleware::state::StateKey& key) noexcept override {
        entries_.erase(key);
    }

    void clear(middleware::state::StateScope scope) noexcept override {
        for (auto it = entries_.begin(); it != entries_.end();) {
            if (it->second.scope == scope) {
                it = entries_.erase(it);
            } else {
                ++it;
            }
        }
    }

    [[nodiscard]] std::optional<middleware::state::StateValue>
    get(const middleware::state::StateKey& key) const override {
        const auto it = entries_.find(key);
        if (it == entries_.end()) {
            return std::nullopt;
        }
        return it->second.value;
    }

    [[nodiscard]] bool has(const middleware::state::StateKey& key) const noexcept override {
        return entries_.find(key) != entries_.end();
    }

    [[nodiscard]] std::size_t count(middleware::state::StateScope scope) const noexcept override {
        std::size_t total = 0;
        for (const auto& [_, entry] : entries_) {
            if (entry.scope == scope) {
                ++total;
            }
        }
        return total;
    }

private:
    struct Entry {
        middleware::state::StateValue value;
        middleware::state::StateScope scope;
    };

    std::unordered_map<middleware::state::StateKey, Entry> entries_;
};

[[nodiscard]] middleware::auth::AuthenticatorConfig buildAuthConfig() {
    middleware::auth::AuthenticatorConfig cfg{};
    cfg.accessTokenExpireSecs = 900;
    cfg.refreshTokenExpireSecs = 86400;
    cfg.secret = "middleware-dev-signing-secret";
    cfg.issuer = "middleware";
    cfg.audience = "frontend";
    return cfg;
}

[[nodiscard]] std::string buildPendingKey(std::string_view username,
                                          std::string_view suffix) {
    return std::string(kPendingAuthPrefix) + std::string(username) + "." + std::string(suffix);
}

[[nodiscard]] std::int64_t nowEpochSeconds() {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

[[nodiscard]] std::string inputErrorMessage(middleware::input::InputError error) {
    switch (error) {
        case middleware::input::InputError::MalformedJson:
            return "Malformed JSON payload";
        case middleware::input::InputError::MissingField:
            return "Missing required field";
        case middleware::input::InputError::EmptyField:
            return "Field must not be empty";
    }
    return "Invalid input";
}

[[nodiscard]] std::string authValidationMessage(middleware::auth::AuthError error) {
    switch (error) {
        case middleware::auth::AuthError::InvalidToken:
            return "Invalid bearer token";
        case middleware::auth::AuthError::TokenExpired:
            return "Bearer token expired";
        case middleware::auth::AuthError::TokenRevoked:
            return "Bearer token revoked";
        case middleware::auth::AuthError::InsufficientPermissions:
            return "Insufficient permissions";
    }
    return "Authentication failed";
}

template <typename T>
[[nodiscard]] std::optional<T> getStateValue(
    const middleware::state::StateHandler& stateHandler,
    const std::string& key
) {
    const auto value = stateHandler.get(key);
    if (!value.has_value()) {
        return std::nullopt;
    }

    try {
        return std::any_cast<T>(*value);
    } catch (const std::bad_any_cast&) {
        return std::nullopt;
    }
}

void clearPendingAuth(middleware::state::StateHandler& stateHandler,
                      std::string_view username) {
    stateHandler.remove(buildPendingKey(username, "otp"));
    stateHandler.remove(buildPendingKey(username, "role"));
    stateHandler.remove(buildPendingKey(username, "expires_at"));
}

[[nodiscard]] std::string deriveUserRole(std::string_view username) {
    return username == "admin" ? "admin" : "operator";
}

[[nodiscard]] bool isCredentialAccepted(const middleware::input::LoginRequest& request) {
    return !request.username.empty() && !request.password.empty();
}

void configureBackendClient(httplib::Client& client, int readTimeoutSeconds = 5) {
    client.set_follow_location(true);
    client.set_connection_timeout(2, 0);
    client.set_read_timeout(readTimeoutSeconds, 0);
    client.set_write_timeout(5, 0);
}

[[nodiscard]] std::string extractPathId(const httplib::Request& req) {
    if (req.matches.size() < 2) {
        return {};
    }
    return req.matches[1].str();
}

[[nodiscard]] std::vector<std::string> parseDeviceIds(std::string_view body) {
    std::vector<std::string> ids;
    std::istringstream in{std::string(body)};
    std::string line;

    while (std::getline(in, line)) {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        const auto first = line.find_first_not_of(" \t");
        if (first == std::string::npos) {
            continue;
        }
        const auto last = line.find_last_not_of(" \t");
        const auto id = line.substr(first, last - first + 1);
        if (!id.empty()) {
            ids.push_back(id);
        }
    }

    return ids;
}

void setBackendUnavailable(httplib::Response& res, std::string_view message) {
    res.status = 502;
    res.set_content(
        Json{
            {"error", "backend_unavailable"},
            {"message", std::string(message)}
        }.dump(),
        "application/json");
}

[[nodiscard]] std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

[[nodiscard]] bool isWebSocketUpgradeRequest(const httplib::Request& req) {
    const auto upgrade = toLower(req.get_header_value("Upgrade"));
    return upgrade == "websocket";
}

[[nodiscard]] std::optional<std::string> extractRealtimeToken(const httplib::Request& req) {
    const auto authHeader = req.get_header_value("Authorization");
    if (authHeader.rfind("Bearer ", 0) == 0 && authHeader.size() > 7) {
        return authHeader.substr(7);
    }

    if (req.has_param("token")) {
        auto token = req.get_param_value("token");
        if (!token.empty()) {
            return token;
        }
    }

    return std::nullopt;
}

[[nodiscard]] Json stateValueToJson(const middleware::state::StateValue& value) {
    if (const auto* text = std::any_cast<std::string>(&value)) {
        try {
            return Json::parse(*text);
        } catch (...) {
            return *text;
        }
    }
    if (const auto* number64 = std::any_cast<std::int64_t>(&value)) {
        return *number64;
    }
    if (const auto* number32 = std::any_cast<int>(&value)) {
        return *number32;
    }
    if (const auto* decimal = std::any_cast<double>(&value)) {
        return *decimal;
    }
    if (const auto* flag = std::any_cast<bool>(&value)) {
        return *flag;
    }

    return Json{
        {"unsupported", true}
    };
}

[[nodiscard]] Json makeRealtimeEnvelope(std::string_view event, const Json& data) {
    const auto eventId = g_nextRealtimeEventId.fetch_add(1);
    return Json{
        {"event", std::string(event)},
        {"type", std::string(event)},
        {"eventId", eventId},
        {"timestamp", nowEpochSeconds()},
        {"data", data}
    };
}

bool sendRealtimeEventToSocket(
    httplib::ws::WebSocket& socket,
    std::string_view event,
    const Json& data
) {
    return socket.send(makeRealtimeEnvelope(event, data).dump());
}

void broadcastRealtimeEvent(std::string_view event, const Json& data) {
    const auto payload = makeRealtimeEnvelope(event, data).dump();
    std::vector<std::uint64_t> staleSessionIds;

    std::lock_guard<std::mutex> lock(g_realtimeSessionsMutex);
    for (auto& [sessionId, session] : g_realtimeSessions) {
        if (session.socket == nullptr || !session.socket->is_open()) {
            staleSessionIds.push_back(sessionId);
            continue;
        }
        if (!session.socket->send(payload)) {
            staleSessionIds.push_back(sessionId);
        }
    }

    for (const auto sessionId : staleSessionIds) {
        g_realtimeSessions.erase(sessionId);
    }
}

void clearRealtimeSessions() {
    std::lock_guard<std::mutex> lock(g_realtimeSessionsMutex);
    g_realtimeSessions.clear();
}

void startRealtimeHeartbeat() {
    std::lock_guard<std::mutex> lock(g_realtimeHeartbeatMutex);
    if (g_realtimeHeartbeatRunning.exchange(true)) {
        return;
    }

    g_realtimeHeartbeatThread = std::thread([] {
        while (g_realtimeHeartbeatRunning.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(kRealtimeHeartbeatIntervalSecs));
            if (!g_realtimeHeartbeatRunning.load()) {
                break;
            }

            broadcastRealtimeEvent(
                "heartbeat",
                Json{
                    {"status", "ok"}
                });
        }
    });
}

void stopRealtimeHeartbeat() {
    {
        std::lock_guard<std::mutex> lock(g_realtimeHeartbeatMutex);
        if (!g_realtimeHeartbeatRunning.exchange(false)) {
            return;
        }
    }

    if (g_realtimeHeartbeatThread.joinable()) {
        g_realtimeHeartbeatThread.join();
    }
}

void sendRealtimeSnapshot(
    httplib::ws::WebSocket& socket,
    const middleware::state::StateHandler& stateHandler
) {
    if (const auto devices = getStateValue<std::string>(stateHandler, std::string(kDevicesLatestKey));
        devices.has_value()) {
        Json payload;
        try {
            payload = Json::parse(*devices);
        } catch (...) {
            payload = Json{{"devices", Json::array()}};
        }
        sendRealtimeEventToSocket(socket, "device_update", payload);
    }

    if (const auto selected = getStateValue<std::string>(stateHandler, std::string(kCameraSelectedKey));
        selected.has_value()) {
        sendRealtimeEventToSocket(
            socket,
            "device_update",
            Json{
                {"selectedCameraId", *selected}
            });
    }

    if (const auto alerts = getStateValue<std::string>(stateHandler, std::string(kAlertsLatestKey));
        alerts.has_value()) {
        Json payload;
        try {
            payload = Json::parse(*alerts);
        } catch (...) {
            payload = Json{{"alerts", Json::array()}};
        }
        sendRealtimeEventToSocket(socket, "alert", payload);
    }
}

class RealtimeStateObserver final : public middleware::state::IStateObserver {
public:
    void onStateChanged(const middleware::state::StateKey& key,
                        const middleware::state::StateValue& value) override {
        if (key == kAlertsLatestKey) {
            broadcastRealtimeEvent("alert", stateValueToJson(value));
            return;
        }

        if (key == kDevicesLatestKey) {
            broadcastRealtimeEvent("device_update", stateValueToJson(value));
            return;
        }

        if (key == kCameraSelectedKey) {
            if (const auto* cameraId = std::any_cast<std::string>(&value)) {
                broadcastRealtimeEvent(
                    "device_update",
                    Json{
                        {"selectedCameraId", *cameraId}
                    });
                return;
            }

            broadcastRealtimeEvent(
                "device_update",
                Json{
                    {"selectedCameraId", stateValueToJson(value)}
                });
        }
    }

    [[nodiscard]] std::string_view name() const noexcept override {
        return "realtime-state-observer";
    }
};

}  // namespace

// ============================================================
// Construction / Destruction
// ============================================================

App::App(const Config& cfg)
    : m_cfg(cfg)
    , m_http(std::make_unique<httplib::Server>())
    , m_authenticator(buildAuthConfig())
{
    m_stateHandler.setStore(std::make_unique<InMemoryStateStore>());
    m_stateHandler.addObserver(std::make_unique<RealtimeStateObserver>());
}

App::~App() {
    stopRealtimeHeartbeat();
    clearRealtimeSessions();
}

httplib::Server& App::httpServer() {
    return *m_http;
}

void App::stop() {
    Logger::info("App stopping HTTP server...", "app").log();
    stopRealtimeHeartbeat();
    clearRealtimeSessions();
    m_http->stop();
}

// ============================================================
// setup() — wire everything together
// ============================================================

void App::setup() {
    Logger::info("App::setup() — registering routes", "app").log();

    // Global pre-routing hook: log every inbound request
    m_http->set_pre_routing_handler([this](const httplib::Request& req,
                                           httplib::Response& res) {
        Logger::info("Inbound request", "router")
            .field("method", req.method)
            .field("path", req.path)
            .log();

        if (req.path == kRealtimePath && isWebSocketUpgradeRequest(req)) {
            const auto token = extractRealtimeToken(req);
            if (!token.has_value()) {
                res.status = 401;
                res.set_content(
                    Json{
                        {"error", "missing_token"},
                        {"message", "Provide a bearer token via Authorization header or token query param"}
                    }.dump(),
                    "application/json");
                return httplib::Server::HandlerResponse::Handled;
            }

            const auto authContext = m_authenticator.authenticate(*token);
            if (!authContext.has_value()) {
                res.status = 401;
                res.set_content(
                    Json{
                        {"error", "invalid_token"},
                        {"message", authValidationMessage(authContext.error())}
                    }.dump(),
                    "application/json");
                return httplib::Server::HandlerResponse::Handled;
            }
        }

        return httplib::Server::HandlerResponse::Unhandled; // continue to matched route
    });

    // Register readiness probes (backend TCP reachability)
    HealthCheck::addReadinessProbe(makeBackendProbe());

    registerHealthRoutes();
    registerAuthRoutes();
    registerCameraRoutes();
    registerDeviceRoutes();
    registerStreamRoutes();
    registerEdrRoutes();

    m_http->WebSocket(std::string(kRealtimePath), [this](const httplib::Request& req,
                                                         httplib::ws::WebSocket& socket) {
        const auto token = extractRealtimeToken(req);
        if (!token.has_value()) {
            sendRealtimeEventToSocket(
                socket,
                "error",
                Json{
                    {"message", "Missing authentication token"}
                });
            socket.close(httplib::ws::CloseStatus::PolicyViolation, "missing token");
            return;
        }

        const auto authContext = m_authenticator.authenticate(*token);
        if (!authContext.has_value()) {
            sendRealtimeEventToSocket(
                socket,
                "error",
                Json{
                    {"message", authValidationMessage(authContext.error())}
                });
            socket.close(httplib::ws::CloseStatus::PolicyViolation, "invalid token");
            return;
        }

        const auto sessionId = g_nextRealtimeSessionId.fetch_add(1);
        {
            std::lock_guard<std::mutex> lock(g_realtimeSessionsMutex);
            g_realtimeSessions.emplace(sessionId, RealtimeSession{
                .id = sessionId,
                .userId = authContext->userId,
                .role = authContext->role,
                .socket = &socket,
            });
        }

        Logger::info("Realtime websocket connected", "realtime")
            .field("session_id", std::to_string(sessionId))
            .field("user_id", authContext->userId)
            .field("role", authContext->role)
            .log();

        sendRealtimeEventToSocket(
            socket,
            "subscribed",
            Json{
                {"sessionId", sessionId},
                {"channels", Json::array({"alert", "device_update"})},
                {"heartbeatSeconds", kRealtimeHeartbeatIntervalSecs},
                {"reconnectSafe", true}
            });
        sendRealtimeSnapshot(socket, m_stateHandler);

        std::string incoming;
        while (socket.is_open()) {
            const auto result = socket.read(incoming);
            if (result == httplib::ws::Fail) {
                break;
            }

            if (result != httplib::ws::Text) {
                continue;
            }

            std::string type;
            try {
                const auto parsed = Json::parse(incoming);
                if (parsed.is_object() && parsed.contains("type") && parsed["type"].is_string()) {
                    type = parsed["type"].get<std::string>();
                }
            } catch (...) {
                type = incoming;
            }

            if (type == "ping" || type == "heartbeat") {
                sendRealtimeEventToSocket(
                    socket,
                    "heartbeat",
                    Json{
                        {"status", "ok"}
                    });
            } else if (type == "snapshot" || type == "resync") {
                sendRealtimeSnapshot(socket, m_stateHandler);
            }
        }

        {
            std::lock_guard<std::mutex> lock(g_realtimeSessionsMutex);
            g_realtimeSessions.erase(sessionId);
        }

        Logger::info("Realtime websocket disconnected", "realtime")
            .field("session_id", std::to_string(sessionId))
            .log();
    });
    startRealtimeHeartbeat();

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
    // Receives: {"username":"...","password":"..."}
    // Returns:  {"status":"otp_required","username":"...","otp":"123456"}
    m_http->Post("/auth/login", [this](const httplib::Request& req,
                                       httplib::Response& res) {
        ErrorHandler::handle(
            "POST /auth/login",
            [&]() {
                Logger::info("Login attempt", "auth").log();

                auto loginRequest = m_inputHandler.parseLogin(req.body);
                if (!loginRequest) {
                    res.status = 400;
                    res.set_content(
                        Json{
                            {"error", "invalid_request"},
                            {"message", inputErrorMessage(loginRequest.error())}
                        }.dump(),
                        "application/json");
                    return;
                }

                if (!isCredentialAccepted(*loginRequest)) {
                    res.status = 401;
                    res.set_content(
                        Json{
                            {"error", "invalid_credentials"},
                            {"message", "Username or password was rejected"}
                        }.dump(),
                        "application/json");
                    return;
                }

                const auto expiresAt = nowEpochSeconds() + kPendingOtpLifetimeSecs;
                const auto role = deriveUserRole(loginRequest->username);
                m_stateHandler.set(
                    buildPendingKey(loginRequest->username, "otp"),
                    std::string(kDevelopmentOtp),
                    middleware::state::StateScope::Session);
                m_stateHandler.set(
                    buildPendingKey(loginRequest->username, "role"),
                    role,
                    middleware::state::StateScope::Session);
                m_stateHandler.set(
                    buildPendingKey(loginRequest->username, "expires_at"),
                    expiresAt,
                    middleware::state::StateScope::Session);

                res.status = 200;
                res.set_content(
                    Json{
                        {"status", "otp_required"},
                        {"username", loginRequest->username},
                        {"otp", kDevelopmentOtp},
                        {"expiresInSeconds", kPendingOtpLifetimeSecs}
                    }.dump(),
                    "application/json");
            },
            [&](int code, const std::string& body) {
                res.status = code;
                res.set_content(body, "application/json");
            });
    });

    // POST /auth/verify-otp
    // Receives: {"username":"...","otp":"..."}
    // Returns:  {"token":"<jwt>","tokenType":"Bearer"}
    m_http->Post("/auth/verify-otp", [this](const httplib::Request& req,
                                            httplib::Response& res) {
        ErrorHandler::handle(
            "POST /auth/verify-otp",
            [&]() {
                Logger::info("OTP verification attempt", "auth").log();

                auto otpRequest = m_inputHandler.parseOtpVerify(req.body);
                if (!otpRequest) {
                    res.status = 400;
                    res.set_content(
                        Json{
                            {"error", "invalid_request"},
                            {"message", inputErrorMessage(otpRequest.error())}
                        }.dump(),
                        "application/json");
                    return;
                }

                const auto otpKey = buildPendingKey(otpRequest->username, "otp");
                const auto roleKey = buildPendingKey(otpRequest->username, "role");
                const auto expiresAtKey = buildPendingKey(otpRequest->username, "expires_at");

                const auto expectedOtp = getStateValue<std::string>(m_stateHandler, otpKey);
                const auto role = getStateValue<std::string>(m_stateHandler, roleKey);
                const auto expiresAt = getStateValue<std::int64_t>(m_stateHandler, expiresAtKey);

                if (!expectedOtp.has_value() || !expiresAt.has_value()) {
                    res.status = 401;
                    res.set_content(
                        Json{
                            {"error", "otp_session_missing"},
                            {"message", "No active login challenge. Call /auth/login first."}
                        }.dump(),
                        "application/json");
                    return;
                }

                if (nowEpochSeconds() > *expiresAt) {
                    clearPendingAuth(m_stateHandler, otpRequest->username);
                    res.status = 401;
                    res.set_content(
                        Json{
                            {"error", "otp_expired"},
                            {"message", "OTP challenge expired. Call /auth/login again."}
                        }.dump(),
                        "application/json");
                    return;
                }

                if (otpRequest->otp != *expectedOtp) {
                    res.status = 401;
                    res.set_content(
                        Json{
                            {"error", "otp_invalid"},
                            {"message", "OTP is invalid"}
                        }.dump(),
                        "application/json");
                    return;
                }

                const auto tokens = m_authenticator.issue(
                    otpRequest->username,
                    role.value_or("operator"));
                clearPendingAuth(m_stateHandler, otpRequest->username);

                res.status = 200;
                res.set_content(
                    Json{
                        {"status", "ok"},
                        {"tokenType", "Bearer"},
                        {"token", tokens.accessToken},
                        {"accessToken", tokens.accessToken},
                        {"refreshToken", tokens.refreshToken}
                    }.dump(),
                    "application/json");
            },
            [&](int code, const std::string& body) {
                res.status = code;
                res.set_content(body, "application/json");
            });
    });

    Logger::debug("Auth routes registered: POST /auth/login, POST /auth/verify-otp", "app").log();
}

// ============================================================
// Camera routes
// ============================================================

void App::registerCameraRoutes() {
    // POST /camera/select — persist current camera selection in middleware state
    m_http->Post("/camera/select", [this](const httplib::Request& req,
                                          httplib::Response& res) {
        ErrorHandler::handle(
            "POST /camera/select",
            [&]() {
                requireAuth(req.get_header_value("Authorization"));

                auto cameraRequest = m_inputHandler.parseCameraSelect(req.body);
                if (!cameraRequest) {
                    res.status = 400;
                    res.set_content(
                        Json{
                            {"error", "invalid_request"},
                            {"message", inputErrorMessage(cameraRequest.error())}
                        }.dump(),
                        "application/json");
                    return;
                }

                std::string previousCamera;
                if (const auto previous = getStateValue<std::string>(m_stateHandler, std::string(kCameraSelectedKey))) {
                    previousCamera = *previous;
                }

                m_stateHandler.set(
                    std::string(kCameraSelectedKey),
                    cameraRequest->cameraId,
                    middleware::state::StateScope::Application);

                Json response{
                    {"status", "ok"},
                    {"selectedCameraId", cameraRequest->cameraId}
                };
                if (!previousCamera.empty()) {
                    response["previousCameraId"] = previousCamera;
                }

                res.status = 200;
                res.set_content(response.dump(), "application/json");
            },
            [&](int code, const std::string& body) {
                res.status = code;
                res.set_content(body, "application/json");
            });
    });

    Logger::debug("Camera routes registered: POST /camera/select", "app").log();
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
                Logger::info("Device list requested", "devices").log();

                httplib::Client backend(m_cfg.backendHost, m_cfg.backendPort);
                configureBackendClient(backend);
                const auto upstream = backend.Get("/cameras");
                if (!upstream) {
                    setBackendUnavailable(res, "Failed to query backend cameras endpoint");
                    return;
                }
                if (upstream->status != 200) {
                    res.status = 502;
                    res.set_content(
                        Json{
                            {"error", "backend_error"},
                            {"message", "Backend returned non-success for /cameras"},
                            {"upstreamStatus", upstream->status}
                        }.dump(),
                        "application/json");
                    return;
                }

                const auto ids = parseDeviceIds(upstream->body);
                Json devices = Json::array();
                for (const auto& id : ids) {
                    devices.push_back(Json{
                        {"id", id},
                        {"label", id},
                        {"streamPath", "/stream/" + id},
                        {"mjpegPath", "/mjpeg/" + id}
                    });
                }

                const Json payload{
                    {"devices", devices},
                    {"count", devices.size()}
                };
                m_stateHandler.set(
                    std::string(kDevicesLatestKey),
                    payload.dump(),
                    middleware::state::StateScope::Application);

                res.status = 200;
                res.set_content(payload.dump(), "application/json");
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
    // GET /stream/:id — single-frame JPEG passthrough adapter
    m_http->Get(R"(/stream/([^/]+))", [this](const httplib::Request& req,
                                             httplib::Response& res) {
        ErrorHandler::handle(
            "GET /stream/:id",
            [&]() {
                requireAuth(req.get_header_value("Authorization"));

                const std::string deviceId = extractPathId(req);
                if (deviceId.empty()) {
                    res.status = 400;
                    res.set_content(
                        Json{
                            {"error", "invalid_request"},
                            {"message", "Missing path parameter ':id'"}
                        }.dump(),
                        "application/json");
                    return;
                }

                Logger::info("Camera stream requested", "stream")
                    .field("device_id", deviceId)
                    .log();

                httplib::Client backend(m_cfg.backendHost, m_cfg.backendPort);
                configureBackendClient(backend);
                const auto upstream = backend.Get(("/stream/" + deviceId).c_str());
                if (!upstream) {
                    setBackendUnavailable(res, "Failed to query backend stream endpoint");
                    return;
                }

                const auto contentType = upstream->get_header_value("Content-Type");
                if (upstream->status != 200) {
                    res.status = upstream->status;
                    res.set_content(
                        upstream->body,
                        contentType.empty() ? "text/plain" : contentType.c_str());
                    return;
                }

                if (const auto cacheControl = upstream->get_header_value("Cache-Control");
                    !cacheControl.empty()) {
                    res.set_header("Cache-Control", cacheControl);
                }

                res.status = 200;
                res.set_content(
                    upstream->body,
                    contentType.empty() ? "image/jpeg" : contentType.c_str());
            },
            [&](int code, const std::string& body) {
                res.status = code;
                res.set_content(body, "application/json");
            });
    });

    // GET /mjpeg/:id — continuous MJPEG passthrough adapter
    m_http->Get(R"(/mjpeg/([^/]+))", [this](const httplib::Request& req,
                                            httplib::Response& res) {
        ErrorHandler::handle(
            "GET /mjpeg/:id",
            [&]() {
                requireAuth(req.get_header_value("Authorization"));

                const std::string deviceId = extractPathId(req);
                if (deviceId.empty()) {
                    res.status = 400;
                    res.set_content(
                        Json{
                            {"error", "invalid_request"},
                            {"message", "Missing path parameter ':id'"}
                        }.dump(),
                        "application/json");
                    return;
                }

                Logger::info("MJPEG stream requested", "stream")
                    .field("device_id", deviceId)
                    .log();

                const std::string backendHost = m_cfg.backendHost;
                const int backendPort = m_cfg.backendPort;
                const std::string backendPath = "/mjpeg/" + deviceId;

                res.set_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
                res.set_header("Pragma", "no-cache");
                res.set_chunked_content_provider(
                    "multipart/x-mixed-replace; boundary=frame",
                    [backendHost, backendPort, backendPath](std::uint64_t /*offset*/,
                                                            httplib::DataSink& sink) {
                        httplib::Client backend(backendHost, backendPort);
                        configureBackendClient(backend, 30);

                        const auto upstream = backend.Get(
                            backendPath.c_str(),
                            [&](const char* data, std::size_t dataLength) {
                                if (!sink.is_writable()) {
                                    return false;
                                }
                                return sink.write(data, dataLength);
                            });

                        sink.done();
                        return static_cast<bool>(upstream) && upstream->status == 200;
                    });
            },
            [&](int code, const std::string& body) {
                res.status = code;
                res.set_content(body, "application/json");
            });
    });

    Logger::debug("Stream routes registered: GET /stream/:id, GET /mjpeg/:id", "app").log();
}

// ============================================================
// EDR routes  (MiddleLogic → OutputHandler → Frontend, per DFD)
// ============================================================

void App::registerEdrRoutes() {
    auto alertsHandler = [this](const httplib::Request& req,
                                httplib::Response& res) {
        ErrorHandler::handle(
            "GET /alerts",
            [&]() {
                requireAuth(req.get_header_value("Authorization"));

                Logger::info("EDR alert list requested", "edr").log();

                Json alerts = Json::array();

                httplib::Client backend(m_cfg.backendHost, m_cfg.backendPort);
                configureBackendClient(backend);
                const auto upstream = backend.Get("/health");
                if (!upstream) {
                    alerts.push_back(Json{
                        {"id", "backend-unreachable"},
                        {"severity", "critical"},
                        {"source", "backend"},
                        {"message", "Backend dependency is unreachable"},
                        {"active", true}
                    });
                } else if (upstream->status != 200) {
                    alerts.push_back(Json{
                        {"id", "backend-health-degraded"},
                        {"severity", "warning"},
                        {"source", "backend"},
                        {"message", "Backend health check returned non-ready"},
                        {"upstreamStatus", upstream->status},
                        {"active", true}
                    });
                }

                const Json payload{
                    {"alerts", alerts},
                    {"count", alerts.size()}
                };
                m_stateHandler.set(
                    std::string(kAlertsLatestKey),
                    payload.dump(),
                    middleware::state::StateScope::Application);

                res.status = 200;
                res.set_content(payload.dump(), "application/json");
            },
            [&](int code, const std::string& body) {
                res.status = code;
                res.set_content(body, "application/json");
            });
    };

    // New frontend-ready endpoint.
    m_http->Get("/alerts", alertsHandler);
    // Temporary compatibility alias to keep existing consumers functional.
    m_http->Get("/edr/alerts", alertsHandler);

    Logger::debug("EDR routes registered: GET /alerts, GET /edr/alerts", "app").log();
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

    const auto authContext = m_authenticator.authenticate(token);
    if (!authContext) {
        throw AuthError(authValidationMessage(authContext.error()));
    }

    Logger::debug("Auth check passed", "auth")
        .field("user_id", authContext->userId)
        .field("role", authContext->role)
        .log();
}

// ============================================================
// Readiness probe: backend TCP reachability
// ============================================================

ReadinessProbe App::makeBackendProbe() const {
    std::string host = m_cfg.backendHost;
    int         port = m_cfg.backendPort;

    return [host, port]() -> ProbeResult {
        httplib::Client backend(host, port);
        configureBackendClient(backend, 3);

        const auto upstream = backend.Get("/health");
        if (!upstream) {
            return ProbeResult{"backend", false, "failed to connect to backend /health"};
        }
        if (upstream->status != 200) {
            return ProbeResult{
                "backend",
                false,
                "backend /health status " + std::to_string(upstream->status)
            };
        }

        return ProbeResult{"backend", true, ""};
    };
}
