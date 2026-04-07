#pragma once

#include "MjpegClient.h"
#include "MotionDetector.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct MotionAlert {
    std::string id;
    std::string severity;
    std::string source;
    std::string message;
    std::string cameraId;
    std::string room;
    std::string state;
    std::int64_t timestampMs{0};
    bool active{false};
};

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
    std::vector<MotionAlert> getRecentAlerts(std::size_t limit = 50) const;

    const std::string& name() const { return name_; }
    bool hasFrames() const;

private:
    void run();
    void recordAlert(bool active, const std::string& message);

    std::string name_;
    std::string url_;
    std::string room_name_;
    int idle_drop_;
    int active_drop_;
    int cooldown_;

    mutable std::mutex frame_mutex_;
    std::vector<uint8_t> latest_frame_;
    mutable std::mutex alerts_mutex_;
    std::deque<MotionAlert> alerts_;
    std::uint64_t alert_seq_{0};
    static constexpr std::size_t kMaxAlertHistory = 200;

    std::atomic<bool> running_{false};
    std::thread thread_;
};
