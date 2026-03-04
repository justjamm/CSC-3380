add_executable(rtsp_hub
    main.cpp
    devices/CameraStream.cpp
    factories/CameraFactory.cpp
    manager/SingletonManager.cpp
)

target_include_directories(rtsp_hub PRIVATE
    ${GSTREAMER_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/third_party/cpp-httplib
)

target_link_libraries(rtsp_hub PRIVATE
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_APP_LIBRARIES}
    Threads::Threads
)

target_compile_options(rtsp_hub PRIVATE -Wall -Wextra)
