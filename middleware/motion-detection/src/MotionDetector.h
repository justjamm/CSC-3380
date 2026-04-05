#pragma once

#include <opencv2/opencv.hpp>

class MotionDetector {
public:
    explicit MotionDetector(int min_area = 500);

    // Returns annotated frame with green bounding boxes.
    // Sets has_motion to true if contours were found.
    cv::Mat process(const cv::Mat& frame, bool& has_motion);

private:
    cv::Ptr<cv::BackgroundSubtractorMOG2> bg_;
    int min_area_;
    cv::Mat kernel_open_;
    cv::Mat kernel_close_;
};
