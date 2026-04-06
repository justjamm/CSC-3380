#include "Server.h"
#include "httplib.h"
#include "Logger.h"
#include "ShutdownManager.h"
#include <string>

// ============================================================
// Server.cpp — TCP / network lifecycle.
//
// Owns the bind-and-listen loop so App can stay focused on
// routes and business logic.  This clean separation means:
//   - App can be unit-tested without opening a port
//   - Server can be swapped for a TLS variant (httplib::SSLServer)
//     without modifying any route logic
// ============================================================

Server::Server(const Config& cfg)
    : m_cfg(cfg)
    , m_app(std::make_unique<App>(cfg))
{}

void Server::init() {
    Logger::info("Server::init() — wiring application", "server").log();

    // Let App register all routes and readiness probes
    m_app->setup();

    // Register graceful-shutdown hook: stop accepting new connections first,
    // then drain in-flight work inside App::stop()
    ShutdownManager::onShutdown("http-server", [this]() {
        m_app->stop();
    });

    Logger::info("Server initialised — ready to listen",  "server")
        .field("host", m_cfg.host)
        .field("port", m_cfg.port)
        .log();
}

void Server::run() {
    Logger::info("Server::run() — binding port", "server")
        .field("host", m_cfg.host)
        .field("port", m_cfg.port)
        .log();

    // httplib::Server::listen() blocks until stop() is called.
    // ShutdownManager's signal handler sets the stop flag; our
    // shutdown hook calls m_app->stop() which calls httplib stop().
    bool ok = m_app->httpServer().listen(m_cfg.host, m_cfg.port);

    if (!ok) {
        Logger::error("httplib listen() failed — check host/port and permissions",
                      "server")
            .field("host", m_cfg.host)
            .field("port", m_cfg.port)
            .log();
        ShutdownManager::requestStop();
    }

    Logger::info("Server::run() — listen loop exited", "server").log();
}
