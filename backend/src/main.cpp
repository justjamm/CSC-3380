#include "manager/SingletonManager.h"
#include "factories/CameraFactory.h"
#include <httplib.h>
#include <gst/gst.h>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);

    CameraFactory cameraFactory;
    auto& manager = SingletonManager::getInstance();

    const char* rtsp_env = std::getenv("RTSP_INPUT_URL");
    const char* rtsp_env_2 = std::getenv("RTSP_INPUT_URL_2");
    const char* rtsp_env_3 = std::getenv("RTSP_INPUT_URL_3");
    std::string rtsp_url = argc > 1 ? argv[1]
                                    : (rtsp_env ? rtsp_env : "rtsp://mediamtx:8554/cam");
    std::string rtsp_url_2 = argc > 2 ? argv[2]
                                      : (rtsp_env_2 ? rtsp_env_2 : "rtsp://mediamtx:8554/cam2");
    std::string rtsp_url_3 = argc > 3 ? argv[3]
                                      : (rtsp_env_3 ? rtsp_env_3 : "rtsp://mediamtx:8554/cam3");

    if (!manager.addDevice(cameraFactory, "cam1", rtsp_url)) {
        std::cerr << "Failed to add camera device cam1" << std::endl;
        //return 1;
        // made optional
    }
    if (!manager.addDevice(cameraFactory, "cam2", rtsp_url_2)) {
        std::cerr << "Failed to add camera device cam2" << std::endl;
        //return 1;
        // made optional
    }
    if (!manager.addDevice(cameraFactory, "cam3", rtsp_url_3)) {
        std::cerr << "Failed to add camera device cam3" << std::endl;
        //return 1;
        // made optional
    }

    httplib::Server svr;

    auto serve_single_frame = [&](const std::string& name, httplib::Response& res) {
        auto* cam = manager.getCamera(name);
        if (!cam) {
            res.status = 404;
            res.set_content("Camera not found", "text/plain");
            return;
        }

        auto frame = cam->getLatestFrame();
        if (frame.empty()) {
            res.status = 503;
            res.set_content("Frame not available yet", "text/plain");
            return;
        }

        res.set_content(std::string(frame.begin(), frame.end()), "image/jpeg");
    };

    auto serve_mjpeg = [&](const std::string& name, httplib::Response& res) {
        auto* cam = manager.getCamera(name);
        if (!cam) {
            res.status = 404;
            res.set_content("Camera not found", "text/plain");
            return;
        }

        res.set_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        res.set_header("Pragma", "no-cache");

        res.set_chunked_content_provider(
            "multipart/x-mixed-replace; boundary=frame",
            [cam](uint64_t, httplib::DataSink& sink) {
                while (sink.is_writable()) {
                    auto frame = cam->getLatestFrame();
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
        for (const auto& name : manager.getDeviceNames())
            body += name + "\n";
        res.set_content(body, "text/plain");
    });

    svr.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        bool has_frame = false;
        for (auto* cam : manager.getAllCameras()) {
            if (cam && !cam->getLatestFrame().empty()) {
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
        serve_single_frame("cam1", res);
    });

    svr.Get("/stream/(.*)", [&](const httplib::Request& req, httplib::Response& res) {
        std::string name = req.matches[1];
        serve_single_frame(name, res);
    });

    svr.Get("/mjpeg", [&](const httplib::Request&, httplib::Response& res) {
        serve_mjpeg("cam1", res);
    });

    svr.Get("/mjpeg/(.*)", [&](const httplib::Request& req, httplib::Response& res) {
        std::string name = req.matches[1];
        serve_mjpeg(name, res);
    });

    std::cout << "Serving on port 8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}
