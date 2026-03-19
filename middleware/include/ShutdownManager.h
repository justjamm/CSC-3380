#pragma once
#include <atomic>
#include <functional>
#include <vector>
#include <csignal>
#include <string>
#include <unistd.h>
#include <cstring>
#include "Logger.h"

// ============================================================
// ShutdownManager — graceful shutdown for the Duncan middle
// layer.
//
// Catches SIGTERM and SIGINT.  On receipt:
//   1. Sets the shared stop flag  (httplib server polls this)
//   2. Runs all registered shutdown hooks IN ORDER (LIFO would
//      also be acceptable; we use registration order here)
//   3. Logs each hook for observability
//
// Register hooks for:
//   - StreamHandler  (close RTSP/camera streams)
//   - StateManager   (flush in-memory state)
//   - DB connection  (close connection pool)
//   - Server itself  (stop() called by App)
//
// Usage:
//   ShutdownManager::init();
//   ShutdownManager::onShutdown("streams",  [&](){ streamHandler.stop(); });
//   ShutdownManager::onShutdown("db",       [&](){ db.close(); });
//   // ... later, in server loop:
//   while (!ShutdownManager::stopRequested()) { ... }
// ============================================================

class ShutdownManager {
public:
    // Call once at startup to install signal handlers
    static void init() {
        s_stopFlag.store(false);
        std::signal(SIGINT,  ShutdownManager::handleSignal);
        std::signal(SIGTERM, ShutdownManager::handleSignal);
        Logger::info("Signal handlers registered (SIGINT, SIGTERM)", "shutdown").log();
    }

    // Register a named shutdown hook
    static void onShutdown(const std::string& name, std::function<void()> hook) {
        s_hooks.push_back({name, std::move(hook)});
    }

    // Poll this in server loops / background threads
    static bool stopRequested() {
        return s_stopFlag.load();
    }

    // Manually trigger shutdown (e.g. from a fatal error path)
    static void requestStop() {
        s_stopFlag.store(true);
    }

    // Run all registered hooks in order — called once by App after
    // the HTTP server has stopped accepting new connections.
    static void runHooks() {
        Logger::info("Running shutdown hooks...", "shutdown").log();
        for (auto& [name, fn] : s_hooks) {
            Logger::info("Shutdown hook: " + name, "shutdown").log();
            try {
                fn();
                Logger::info("Shutdown hook complete: " + name, "shutdown").log();
            } catch (const std::exception& e) {
                Logger::error("Shutdown hook threw: " + std::string(e.what()), "shutdown")
                    .field("hook", name).log();
            } catch (...) {
                Logger::error("Shutdown hook threw unknown exception", "shutdown")
                    .field("hook", name).log();
            }
        }
        Logger::info("All shutdown hooks complete.", "shutdown").log();
    }

private:
    static void handleSignal(int sig) {
        (void)sig; // signal name logged via write() below
        // Use write() — async-signal-safe (std::cout is NOT safe in signal handlers)
        const char* msg = "[shutdown] Signal received — initiating graceful shutdown\n";
        const ssize_t written = ::write(STDERR_FILENO, msg, std::strlen(msg));
        (void)written;
        s_stopFlag.store(true);
    }

    static std::atomic<bool>                              s_stopFlag;
    static std::vector<std::pair<std::string,
                                 std::function<void()>>>  s_hooks;
};

// Static member definitions
inline std::atomic<bool> ShutdownManager::s_stopFlag{false};
inline std::vector<std::pair<std::string, std::function<void()>>>
    ShutdownManager::s_hooks;
