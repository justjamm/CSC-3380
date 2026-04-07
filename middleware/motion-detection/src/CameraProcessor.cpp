#include "CameraProcessor.h"
#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>

namespace {
std::int64_t nowEpochMillis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
}  // namespace

CameraProcessor::CameraProcessor(const std::string& name, const std::string& backend_url,
                                 const std::string& room_name,
                                 int idle_drop, int active_drop, int cooldown)
    : name_(name)
    , url_(backend_url + "/mjpeg/" + name)
    , room_name_(room_name)
    , idle_drop_(idle_drop)
    , active_drop_(active_drop)
    , cooldown_(cooldown) {}

CameraProcessor::~CameraProcessor() {
    stop();
}

void CameraProcessor::start() {
    running_ = true;
    thread_ = std::thread(&CameraProcessor::run, this);
}

void CameraProcessor::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

std::vector<uint8_t> CameraProcessor::getLatestFrame() const {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    return latest_frame_;
}

std::vector<MotionAlert> CameraProcessor::getRecentAlerts(std::size_t limit) const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    std::vector<MotionAlert> recent;
    if (alerts_.empty() || limit == 0) {
        return recent;
    }

    const std::size_t start = alerts_.size() > limit ? alerts_.size() - limit : 0;
    recent.reserve(alerts_.size() - start);
    for (std::size_t i = start; i < alerts_.size(); ++i) {
        recent.push_back(alerts_[i]);
    }
    return recent;
}

bool CameraProcessor::hasFrames() const {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    return !latest_frame_.empty();
}

void CameraProcessor::recordAlert(bool active, const std::string& message) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);

    MotionAlert alert;
    alert_seq_ += 1;
    alert.id = "motion-" + name_ + "-" + std::to_string(alert_seq_);
    alert.severity = active ? "high" : "info";
    alert.source = "motion-detection";
    alert.message = message;
    alert.cameraId = name_;
    alert.room = room_name_;
    alert.state = active ? "active" : "cleared";
    alert.timestampMs = nowEpochMillis();
    alert.active = active;

    alerts_.push_back(std::move(alert));
    while (alerts_.size() > kMaxAlertHistory) {
        alerts_.pop_front();
    }
}

void CameraProcessor::run() {
    MjpegClient client(url_);
    MotionDetector detector;

    enum State { IDLE, ACTIVE };
    State state = IDLE;
    int frame_count = 0;
    int idle_streak = 0;

    std::cout << "[" << name_ << "] Starting processor, consuming " << url_ << std::endl;

    while (running_) {
        if (!client.isConnected()) {
            std::cout << "[" << name_ << "] Connecting..." << std::endl;
            client.connect();
            if (!client.isConnected()) {
                std::cerr << "[" << name_ << "] Connect failed, retrying in 2s" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }
        }

        auto jpeg = client.nextFrame();
        if (jpeg.empty()) {
            // Disconnected — reconnect with backoff
            std::cerr << "[" << name_ << "] Stream lost, reconnecting in 2s" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        // Adaptive frame dropping
        int skip_rate = (state == IDLE) ? idle_drop_ : active_drop_;
        frame_count++;
        if (skip_rate > 0 && (frame_count % skip_rate) != 0) {
            continue;  // Drop — consumed but not decoded
        }

        // Decode JPEG
        cv::Mat frame = cv::imdecode(cv::Mat(jpeg), cv::IMREAD_COLOR);
        if (frame.empty()) continue;

        // Process with motion detector
        bool has_motion = false;
        cv::Mat annotated = detector.process(frame, has_motion);

        // Update state machine
        State prev_state = state;
        if (has_motion) {
            state = ACTIVE;
            idle_streak = 0;
        } else {
            idle_streak++;
            if (idle_streak >= cooldown_) {
                state = IDLE;
            }
        }

        if (prev_state == IDLE && state == ACTIVE) {
            const std::string message = "Motion detected in " + room_name_;
            std::cout << "[" << name_ << "] " << message << std::endl;
            recordAlert(true, message);
        } else if (prev_state == ACTIVE && state == IDLE) {
            const std::string message = "Motion cleared in " + room_name_;
            std::cout << "[" << name_ << "] " << message << std::endl;
            recordAlert(false, message);
        }

        // Encode annotated frame back to JPEG
        std::vector<uint8_t> out_jpeg;
        cv::imencode(".jpg", annotated, out_jpeg);

        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            latest_frame_ = std::move(out_jpeg);
        }
    }
}
