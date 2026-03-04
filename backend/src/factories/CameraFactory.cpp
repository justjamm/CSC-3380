#include "CameraFactory.h"
#include "../devices/CameraStream.h"

std::unique_ptr<IDevice> CameraFactory::createDevice(
    const std::string& name,
    const std::string& address
)
{
    return std::make_unique<CameraStream>(name, address);
}