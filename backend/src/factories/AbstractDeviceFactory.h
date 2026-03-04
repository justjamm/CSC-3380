#pragma once
#include "../devices/IDevice.h"
#include <memory>
#include <string>

class AbstractDeviceFactory {
    public:
    virtual ~AbstractDeviceFactory() = default;

    virtual std::unique_ptr<IDevice> createDevice(
        const std::string& name,
        const std::string& address
    ) = 0;


};