#pragma once
#include <string>
#include <memory>
#include "Config.h"
#include "App.h"
#include "ShutdownManager.h"
#include "Logger.h"

// ============================================================
// Server — network / TCP lifecycle entrypoint.
//
// Owns the bind-and-listen loop.  Delegates all business logic
// to App.  This separation lets you:
//   - Swap transport (TLS, Unix socket) without touching routes
//   - Test App routes independently of a live port
//
// Lifecycle:
//   Server s(cfg);
//   s.init();   // calls App::setup(), registers shutdown hook
//   s.run();    // blocks until ShutdownManager::stopRequested()
//   // after run() returns, ShutdownManager::runHooks() is called
//   //   → App::stop() → server stops accepting connections
// ============================================================

class Server {
public:
    explicit Server(const Config& cfg);

    // Build App, register routes, register shutdown hooks.
    // Does NOT bind a port yet.
    void init();

    // Bind port and block until a stop signal is received.
    void run();

private:
    Config          m_cfg;
    std::unique_ptr<App> m_app;
};
