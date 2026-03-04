#include "CameraStream.h" //since everything is included in the .h, dont need to include here
#include <iostream> //duh


//not even gonna pretend i know half of what's happening here, this is lowkey purgatory
CameraStream::CameraStream(
    const std::string& name,
    const std::string& rtsp_url
) : name_(name), rtsp_url_(rtsp_url) {} // i don't think i've ever seen such beautiful inline

CameraStream::~CameraStream(){
    stop();
}

bool CameraStream::start() {
    std::string pipeline_str = 
        "rtspsrc location=" + rtsp_url_ + " latency=0 ! "
        "decodebin ! videoconvert ! "
        "jpegenc ! appsink name=sink"; 
        //GStreamer needs a pipeline string, and it doesn't exist as a struct for some reason, so its a string. 

    GError* error = nullptr;
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
    if (error) {
        std::cerr << "[" << name_ << "]Pilepine error: " << error->message << std:: endl;
        g_error_free(error);
        return false;
    }
    //gstreamer does a whole lotta nonsense here, then returns sink,  a GstElement.
    sink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");

    GstAppSinkCallbacks callbacks = {};
    callbacks.new_sample=onNewSample;
    gst_app_sink_set_callbacks(GST_APP_SINK(sink_),&callbacks, this, nullptr);

    gst_element_set_state(pipeline_, GST_STATE_PLAYING);

    GstBus* bus = gst_element_get_bus(pipeline_);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND, 
        (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_ASYNC_DONE));
    gst_object_unref(bus);
        
    if (!msg) {
    std::cerr << "[" << name_ << "] Timed out connecting" << std::endl;
    return false;
    }
    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError* err = nullptr;
        gst_message_parse_error(msg, &err, nullptr);
        std::cerr << "[" << name_ << "] Error: " << err->message << std::endl;
        g_error_free(err);
        gst_message_unref(msg);
        return false;
    }

    gst_message_unref(msg);
    connected_ = true;
    std::cout << "[" << name_ << "] Connected to " << rtsp_url_ << std::endl;
    return true;
}

void CameraStream::stop(){
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;

    }
    if (sink_){
        gst_object_unref(sink_);
        sink_ = nullptr;

    }
    connected_ = false;
}

std::vector<uint8_t> CameraStream::getLatestFrame() const {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    return latest_frame_;
}

GstFlowReturn CameraStream::onNewSample(GstAppSink* appsink, gpointer user_data) {
    auto* self = static_cast<CameraStream*>(user_data);

    GstSample* sample = gst_app_sink_pull_sample(appsink);
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);

    {
        std::lock_guard<std::mutex> lock(self->frame_mutex_);
        self->latest_frame_.assign(map.data, map.data + map.size);
    }

    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}




