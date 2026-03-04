#pragma once
#include "IDevice.h"
#include <vector>
#include <cstdint>

class ICamera : public IDevice {
    public:
    virtual ~ICamera() =default; //virtual destructor, as mentioned in IDevice.h

    virtual std::vector<uint8_t> getLatestFrame() const = 0; //raw image data to be cast to jpeg later
    virtual const std::string& getRtspUrl() const = 0; //self explanatory

};