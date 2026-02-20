#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <httplib.h>
#include <iostream>
#include <mutex>
#include <vector>

std::vector<uint8_t> latest_frame;
std::mutex frame_mutex;

GstFlowReturn on_new_sample(GstAppSink *appsink, gpointer) {
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;

    gst_buffer_map(buffer, &map, GST_MAP_READ);
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

    std::string rtsp_url = argc > 1 ? argv[1] : "rtsp://mediamtx:8554/cam";

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

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    //debug
    GstBus *bus = gst_element_get_bus(pipeline);
GstMessage *msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
    (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_ASYNC_DONE));

if (!msg) {
    std::cerr << "Pipeline timed out - could not connect to stream" << std::endl;
    return -1;
}
if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
    GError *err;
    gst_message_parse_error(msg, &err, nullptr);
    std::cerr << "Pipeline error: " << err->message << std::endl;
    return -1;
}






    std::cout << "Connected to stream: " << rtsp_url << std::endl;

    httplib::Server svr;

    svr.Get("/stream", [](const httplib::Request &, httplib::Response &res) {
        std::lock_guard<std::mutex> lock(frame_mutex);
        if (latest_frame.empty()) {
            res.status = 503;
            return;
        }
        res.set_content(
            std::string(latest_frame.begin(), latest_frame.end()),
            "image/jpeg"
        );
    });

    std::cout << "Serving on port 8080" << std::endl;
    std::cout << "Serving on port 8080" << std::endl;
if (!svr.listen("0.0.0.0", 8080)) {
    std::cerr << "Failed to start HTTP server on port 8080" << std::endl;
    return -1;
}

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}