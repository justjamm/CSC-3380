#include "CameraProcessor.h"
#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>

CameraProcessor::CameraProcessor(const std::string& name, const std::string& backend_url,
                                 int idle_drop, int active_drop, int cooldown)
    : name_(name)
    , url_(backend_url + "/mjpeg/" + name)
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

bool CameraProcessor::hasFrames() const {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    return !latest_frame_.empty();
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
        if (has_motion) {
            state = ACTIVE;
            idle_streak = 0;
        } else {
            idle_streak++;
            if (idle_streak >= cooldown_) {
                state = IDLE;
            }
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
