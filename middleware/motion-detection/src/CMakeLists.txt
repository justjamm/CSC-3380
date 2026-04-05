add_executable(motion_detection
    main.cpp
    MjpegClient.cpp
    MotionDetector.cpp
    CameraProcessor.cpp
)

target_include_directories(motion_detection PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/third_party/cpp-httplib
)

target_link_libraries(motion_detection PRIVATE
    ${OpenCV_LIBS}
    ${CURL_LIBRARIES}
    Threads::Threads
)

target_compile_options(motion_detection PRIVATE -Wall -Wextra)
