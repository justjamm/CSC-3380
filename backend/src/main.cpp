#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <httplib.h>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

std::vector<uint8_t> latest_frame;
std::mutex frame_mutex;

GstFlowReturn on_new_sample(GstAppSink *appsink, gpointer) {
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (!sample) {
        return GST_FLOW_EOS;
    }

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }
    {
        std::lock_guard<std::mutex> lock(frame_mutex);
        latest_frame.assign(map.data, map.data + map.size);
    }
    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

int main(int argc, char *argv[]) {
    std::cout << "Starting..." << std::endl;
    std::cerr << "Starting stderr..." << std::endl;
    std::cout.flush();
    std::cerr.flush();
    gst_init(&argc, &argv);

    const char *rtsp_env = std::getenv("RTSP_INPUT_URL");
    std::string rtsp_url = argc > 1 ? argv[1]
                                    : (rtsp_env ? rtsp_env : "rtsp://mediamtx:8554/cam");

    std::string pipeline_str =
        "rtspsrc location=" + rtsp_url + " latency=0 ! "
        "decodebin ! videoconvert ! "
        "jpegenc ! appsink name=sink";

    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (error) {
        std::cerr << "Pipeline error: " << error->message << std::endl;
        return -1;
    }

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    GstAppSinkCallbacks callbacks = { nullptr, nullptr, on_new_sample };
    gst_app_sink_set_callbacks(GST_APP_SINK(sink), &callbacks, nullptr, nullptr);
    GstStateChangeReturn state_change = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (state_change == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set GStreamer pipeline to PLAYING" << std::endl;
        return -1;
    }

    std::cout << "Connected to stream: " << rtsp_url << std::endl;

    httplib::Server svr;
    svr.set_keep_alive_max_count(100);
    svr.set_keep_alive_timeout(30);

    svr.Get("/stream", [](const httplib::Request &, httplib::Response &res) {
        std::lock_guard<std::mutex> lock(frame_mutex);
        if (latest_frame.empty()) {
            res.status = 503;
            res.set_content("Frame not available yet", "text/plain");
            return;
        }
        res.set_content(
            std::string(latest_frame.begin(), latest_frame.end()),
            "image/jpeg"
        );
    });

    svr.Get("/mjpeg", [](const httplib::Request &, httplib::Response &res) {
        res.set_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        res.set_header("Pragma", "no-cache");

        res.set_chunked_content_provider(
            "multipart/x-mixed-replace; boundary=frame",
            [](uint64_t, httplib::DataSink &sink) {
                while (sink.is_writable()) {
                    std::vector<uint8_t> frame_copy;
                    {
                        std::lock_guard<std::mutex> lock(frame_mutex);
                        frame_copy = latest_frame;
                    }

                    if (frame_copy.empty()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(80));
                        continue;
                    }

                    std::string header =
                        "--frame\r\n"
                        "Content-Type: image/jpeg\r\n"
                        "Content-Length: " + std::to_string(frame_copy.size()) + "\r\n\r\n";

                    if (!sink.write(header.data(), header.size())) {
                        break;
                    }
                    if (!sink.write(reinterpret_cast<const char *>(frame_copy.data()), frame_copy.size())) {
                        break;
                    }
                    if (!sink.write("\r\n", 2)) {
                        break;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(80));
                }
                sink.done();
                return true;
            }
        );
    });

    svr.Get("/health", [](const httplib::Request &, httplib::Response &res) {
        bool has_frame = false;
        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            has_frame = !latest_frame.empty();
        }

        if (has_frame) {
            res.status = 200;
            res.set_content("{\"status\":\"ok\",\"stream\":\"ready\"}", "application/json");
        } else {
            res.status = 503;
            res.set_content("{\"status\":\"starting\",\"stream\":\"waiting_for_frames\"}", "application/json");
        }
    });

    std::cout << "Serving on port 8080" << std::endl;
    if (!svr.listen("0.0.0.0", 8080)) {
        std::cerr << "Failed to start HTTP server on port 8080" << std::endl;
        return -1;
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
