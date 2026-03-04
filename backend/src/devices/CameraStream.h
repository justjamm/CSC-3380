#pragma once
#include "ICamera.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <mutex>

class CameraStream : public ICamera { //CameraStream implements ICamera
                                        //actually fascinating how this works. You can choose to make the methods in the interface private
    public:
    CameraStream(
        const std::string& name,
        const std::string& rtsp_url
    );
    ~CameraStream() override;


    //actually important overrides
    bool start() override;
    void stop() override;
    


    //getters
    bool isConnected() const override{ return connected_;} //beautiful that this can be done inline, takes away a ton of clutter from the main cpp
    const std::string& getName() const override {return name_;}

    
    const std::string& getRtspUrl() const override{return rtsp_url_;}
    std::vector<uint8_t> getLatestFrame() const override; //can't inline this, needs more complex logic to guarantee race condition-free code.

    private:
    static GstFlowReturn onNewSample(GstAppSink* appsink, gpointer user_data); //hell. check the cpp for more descriptive language  
                                                                                //I lied. this is a purgatory that I'm not willing to try to understand. if it works, it works. image data
                                                                                //is image data. 

    std::string name_;
    std::string rtsp_url_;
    bool connected_ =false;

    GstElement* pipeline_ = nullptr;
    GstElement* sink_ = nullptr;

    mutable std::mutex frame_mutex_;
    std::vector<uint8_t> latest_frame_;



};