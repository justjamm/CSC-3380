#pragma once

#include "MjpegClient.h"
#include "MotionDetector.h"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class CameraProcessor {
public:
    CameraProcessor(const std::string& name, const std::string& backend_url,
                    const std::string& room_name,
                    int idle_drop, int active_drop, int cooldown);
    ~CameraProcessor();

    CameraProcessor(const CameraProcessor&) = delete;
    CameraProcessor& operator=(const CameraProcessor&) = delete;

    void start();
    void stop();

    // Thread-safe — returns latest annotated JPEG (or empty if none yet)
    std::vector<uint8_t> getLatestFrame() const;

    const std::string& name() const { return name_; }
    bool hasFrames() const;

private:
    void run();

    std::string name_;
    std::string url_;
    std::string room_name_;
    int idle_drop_;
    int active_drop_;
    int cooldown_;

    mutable std::mutex frame_mutex_;
    std::vector<uint8_t> latest_frame_;

    std::atomic<bool> running_{false};
    std::thread thread_;
};
