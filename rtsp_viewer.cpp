#include <opencv2/opencv.hpp>
#include <iostream>

int main(int argc, char** argv) {
    // Default RTSP URL (you can change this to your stream)
    std::string rtsp_url = "rtsp://tai123:happymeal123@localhost:8554/stream1";
    
    // If URL is provided as command line argument, use it
    if (argc > 1) {
        rtsp_url = argv[1];
    }
    
    std::cout << "Attempting to connect to RTSP stream: " << rtsp_url << std::endl;
    
    // Create VideoCapture object with RTSP URL
    cv::VideoCapture cap(rtsp_url, cv::CAP_FFMPEG);
    
    // Check if stream opened successfully
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open RTSP stream!" << std::endl;
        std::cerr << "Please check:" << std::endl;
        std::cerr << "  - RTSP URL is correct" << std::endl;
        std::cerr << "  - Network connection is available" << std::endl;
        std::cerr << "  - Camera/server is accessible" << std::endl;
        return -1;
    }
    
    std::cout << "Successfully connected to stream!" << std::endl;
    std::cout << "Press 'q' or ESC to quit" << std::endl;
    
    // Get stream properties
    int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = cap.get(cv::CAP_PROP_FPS);
    
    std::cout << "Stream properties:" << std::endl;
    std::cout << "  Resolution: " << frame_width << "x" << frame_height << std::endl;
    std::cout << "  FPS: " << fps << std::endl;
    
    cv::Mat frame;
    
    // Main loop to read and display frames
    while (true) {
        // Capture frame
        cap >> frame;
        
        // Check if frame is empty (stream ended or error)
        if (frame.empty()) {
            std::cerr << "Warning: Received empty frame. Stream may have ended." << std::endl;
            break;
        }
        
        // Display the frame
        cv::imshow("RTSP Stream Viewer", frame);
        
        // Wait for key press (1ms delay)
        // Press 'q' or ESC to exit
        int key = cv::waitKey(1);
        if (key == 'q' || key == 'Q' || key == 27) { // 27 is ESC key
            std::cout << "Exiting..." << std::endl;
            break;
        }
    }
    
    // Release resources
    cap.release();
    cv::destroyAllWindows();
    
    std::cout << "Stream closed successfully." << std::endl;
    return 0;
}