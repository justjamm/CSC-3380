#include "MotionDetector.h"

MotionDetector::MotionDetector(int min_area) : min_area_(min_area) {
    bg_ = cv::createBackgroundSubtractorMOG2(500, 16, true);
    kernel_open_ = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    kernel_close_ = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(15, 15));
}

cv::Mat MotionDetector::process(const cv::Mat& frame, bool& has_motion) {
    has_motion = false;

    cv::Mat fg_mask;
    bg_->apply(frame, fg_mask);

    // Threshold to remove shadows (MOG2 marks shadows as 127)
    cv::threshold(fg_mask, fg_mask, 200, 255, cv::THRESH_BINARY);

    // Morphological open to remove noise
    cv::morphologyEx(fg_mask, fg_mask, cv::MORPH_OPEN, kernel_open_);

    // Morphological close to fill gaps
    cv::morphologyEx(fg_mask, fg_mask, cv::MORPH_CLOSE, kernel_close_);

    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(fg_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    cv::Mat annotated = frame.clone();

    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area < min_area_) continue;

        has_motion = true;
        cv::Rect box = cv::boundingRect(contour);
        cv::rectangle(annotated, box, cv::Scalar(0, 255, 0), 2);
    }

    return annotated;
}
