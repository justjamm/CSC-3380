#pragma once
#include "AbstractDeviceFactory.h"

class CameraFactory : public AbstractDeviceFactory { 
    public:
    std::unique_ptr<IDevice> createDevice(
        const std::string& name,
        const std::string& address
    ) override;

};