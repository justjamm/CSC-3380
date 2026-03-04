#include "CameraStream.h" //since everything is included in the .h, dont need to include here
#include <iostream> //duh

CameraStream::CameraStream(
    const std::string& name; 
    const std::string& rtsp_url
) : name_(name), rtsp_url_(rtsp_url) {} // i don't think i've ever seen such beautiful inline

CameraStream::~CameraStream(){
    stop();
}

bool CameraStream::start() {
    std::string pipeline_str = 
        "rtspsrc location=" + rtsp_url_ + " latency=0 ! "
        "decodebin ! videoconvert ! "
        "jpegenc ! appsink name=sink"; //ngl i think this is magic 


        GError* error = nullptr;
        pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
        if (error) {
            std:cerr << "[" << name_ << "]Pilepine error: " << error->message << std:: endl;
            g_error_free(error);
            return false;
        }

        sink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");

        GstAppSinkCallbacks callbacks = {};
        callbacks.new_sample=onNewSample;
        gst_app_sink_set_callbacks(GST_APP_SINK(sink_),&callbacks, this, nullptr);

        gst_element_set_state(pipeline_, GST_STATE_PLAYING);

        GstBus* bus = gst_element_get_bus(pipeline_);
        GstMessage* msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND, 
            (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_ASYNC_DONE));
        gst_object_unref(bus);
        




}
