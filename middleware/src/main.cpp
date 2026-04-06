#include <iostream>
#include <stdexcept>

#include "Config.h"
#include "Logger.h"
#include "ShutdownManager.h"
#include "Server.h"

// ============================================================
// main.cpp — Duncan Home Security middle-layer service entry.
//
// Boot sequence:
//   1. Install signal handlers           (ShutdownManager::init)
//   2. Load + validate all config/env    (Config::load)
//   3. Configure logger                  (Logger::setLevel)
//   4. Build and initialise server       (Server::init)
//   5. Block on server listen loop       (Server::run)
//   6. On signal: run shutdown hooks     (ShutdownManager::runHooks)
//   7. Exit 0
//
// Any failure in steps 2–4 exits immediately with a non-zero
// status and a structured error log line (fail-fast guarantee).
// ============================================================

int main() {
    // ---- Step 1: Signal handlers must be installed first ----
    ShutdownManager::init();

    // ---- Step 2: Config validation (fail-fast) ----
    Config cfg;
    try {
        cfg = Config::load();
    }
    catch (const ConfigError& e) {
        // Config error: use plain stderr because Logger isn't configured yet
        std::cerr << "{\"level\":\"error\",\"component\":\"main\","
                  << "\"msg\":\"" << e.what() << "\"}\n";
        return 1;
    }

    // ---- Step 3: Logger ----
    Logger::setLevel(cfg.logLevel);
    Logger::info("Duncan Home Security — Middle Layer starting", "main")
        .field("host",      cfg.host)
        .field("port",      cfg.port)
        .field("log_level", cfg.logLevel)
        .log();
    Logger::info("Backend target", "main")
        .field("backend_host", cfg.backendHost)
        .field("backend_port", cfg.backendPort)
        .log();

    // ---- Step 4: Server initialisation ----
    Server server(cfg);
    try {
        server.init();
    }
    catch (const std::exception& e) {
        Logger::error(std::string("Server init failed: ") + e.what(), "main").log();
        return 1;
    }

    // ---- Step 5: Blocking listen loop ----
    server.run();

    // ---- Step 6: Graceful shutdown hooks ----
    Logger::info("Server stopped — running shutdown hooks", "main").log();
    ShutdownManager::runHooks();

    Logger::info("Duncan Home Security middleware exited cleanly", "main").log();
    return 0;
}
