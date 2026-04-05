#include "CameraProcessor.h"
#include <httplib.h>
#include <json.hpp>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

static std::string getEnv(const char* key, const std::string& fallback) {
    const char* val = std::getenv(key);
    return val ? val : fallback;
}

static int getEnvInt(const char* key, int fallback) {
    const char* val = std::getenv(key);
    return val ? std::stoi(val) : fallback;
}

static std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> tokens;
    std::istringstream stream(s);
    std::string token;
    while (std::getline(stream, token, delim)) {
        if (!token.empty()) tokens.push_back(token);
    }
    return tokens;
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::string backend_url = getEnv("BACKEND_URL", "http://backend:8080");
    std::string cameras_str = getEnv("CAMERAS", "cam1,cam2");
    int idle_drop = getEnvInt("IDLE_DROP", 5);
    int active_drop = getEnvInt("ACTIVE_DROP", 1);
    int cooldown = getEnvInt("COOLDOWN_FRAMES", 30);

    auto camera_names = split(cameras_str, ',');

    // Load camera-to-room mapping from zones.json
    std::map<std::string, std::string> room_map;
    std::ifstream zones_file("zones.json");
    if (zones_file.is_open()) {
        nlohmann::json j;
        zones_file >> j;
        for (auto& [cam, room] : j.items())
            room_map[cam] = room.get<std::string>();
        std::cout << "Loaded zones.json: " << room_map.size() << " room(s) mapped" << std::endl;
    } else {
        std::cerr << "Warning: zones.json not found, using camera IDs as room names" << std::endl;
    }

    // Create and start processors
    std::vector<std::unique_ptr<CameraProcessor>> processors;
    for (const auto& name : camera_names) {
        std::string room = room_map.count(name) ? room_map[name] : name;
        auto proc = std::make_unique<CameraProcessor>(name, backend_url, room,
                                                       idle_drop, active_drop, cooldown);
        proc->start();
        processors.push_back(std::move(proc));
    }

    // Helper to find processor by name
    auto findProcessor = [&](const std::string& name) -> CameraProcessor* {
        for (auto& p : processors) {
            if (p->name() == name) return p.get();
        }
        return nullptr;
    };

    httplib::Server svr;

    // Serve MJPEG stream — mirrors backend pattern exactly
    auto serve_mjpeg = [&](const std::string& name, httplib::Response& res) {
        auto* proc = findProcessor(name);
        if (!proc) {
            res.status = 404;
            res.set_content("Camera not found", "text/plain");
            return;
        }

        res.set_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        res.set_header("Pragma", "no-cache");

        res.set_chunked_content_provider(
            "multipart/x-mixed-replace; boundary=frame",
            [proc](uint64_t, httplib::DataSink& sink) {
                while (sink.is_writable()) {
                    auto frame = proc->getLatestFrame();
                    if (frame.empty()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(80));
                        continue;
                    }

                    std::string header =
                        "--frame\r\n"
                        "Content-Type: image/jpeg\r\n"
                        "Content-Length: " + std::to_string(frame.size()) + "\r\n\r\n";

                    if (!sink.write(header.data(), header.size())) break;
                    if (!sink.write(reinterpret_cast<const char*>(frame.data()), frame.size())) break;
                    if (!sink.write("\r\n", 2)) break;

                    std::this_thread::sleep_for(std::chrono::milliseconds(80));
                }

                sink.done();
                return true;
            }
        );
    };

    svr.Get("/cameras", [&](const httplib::Request&, httplib::Response& res) {
        std::string body;
        for (const auto& p : processors) {
            body += p->name() + "\n";
        }
        res.set_content(body, "text/plain");
    });

    svr.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        bool has_frame = false;
        for (const auto& p : processors) {
            if (p->hasFrames()) {
                has_frame = true;
                break;
            }
        }

        if (has_frame) {
            res.status = 200;
            res.set_content("{\"status\":\"ok\",\"stream\":\"ready\"}", "application/json");
        } else {
            res.status = 503;
            res.set_content("{\"status\":\"starting\",\"stream\":\"waiting_for_frames\"}", "application/json");
        }
    });

    svr.Get("/stream", [&](const httplib::Request&, httplib::Response& res) {
        auto* proc = findProcessor(camera_names.front());
        if (!proc || !proc->hasFrames()) {
            res.status = 503;
            res.set_content("Frame not available yet", "text/plain");
            return;
        }
        auto frame = proc->getLatestFrame();
        res.set_content(std::string(frame.begin(), frame.end()), "image/jpeg");
    });

    svr.Get("/stream/(.*)", [&](const httplib::Request& req, httplib::Response& res) {
        std::string name = req.matches[1];
        auto* proc = findProcessor(name);
        if (!proc) {
            res.status = 404;
            res.set_content("Camera not found", "text/plain");
            return;
        }
        auto frame = proc->getLatestFrame();
        if (frame.empty()) {
            res.status = 503;
            res.set_content("Frame not available yet", "text/plain");
            return;
        }
        res.set_content(std::string(frame.begin(), frame.end()), "image/jpeg");
    });

    svr.Get("/mjpeg", [&](const httplib::Request&, httplib::Response& res) {
        serve_mjpeg(camera_names.front(), res);
    });

    svr.Get("/mjpeg/(.*)", [&](const httplib::Request& req, httplib::Response& res) {
        std::string name = req.matches[1];
        serve_mjpeg(name, res);
    });

    std::cout << "Motion detection serving on port 8080" << std::endl;
    std::cout << "  Backend: " << backend_url << std::endl;
    std::cout << "  Cameras: " << cameras_str << std::endl;
    std::cout << "  Idle drop: " << idle_drop << ", Active drop: " << active_drop
              << ", Cooldown: " << cooldown << std::endl;

    svr.listen("0.0.0.0", 8080);

    // Cleanup
    for (auto& p : processors) p->stop();
    curl_global_cleanup();
    return 0;
}
