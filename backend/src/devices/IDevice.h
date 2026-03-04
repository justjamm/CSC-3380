#pragma once //stops the linker from linking bs
#include <string>

class IDevice {
    public:
    virtual ~IDevice() = default; //virtual basically means "Call the lowest method of an IDevice, not IDevice"

    virtual bool start() = 0;       //0 means that all subclasses MUST use the method
    virtual void stop() = 0;
    virtual bool isConnected() const= 0;
    virtual const std::string& getName() const = 0;
};