#include "SingletonManager.h"
#include <iostream>

SingletonManager& SingletonManager::getInstance() {
    static SingletonManager instance;
    return instance;
}

bool SingletonManager::addDevice(
    AbstractDeviceFactory& factory,
    const std::string& name,
    const std::string& address)
{
    if (devices_.count(name)) {
        std::cerr << "Device already exists: " << name << std::endl;
        return false;
    }

    auto device = factory.createDevice(name, address);
    if (!device->start()) return false;

    devices_[name] = std::move(device);
    return true;
}

void SingletonManager::removeDevice(const std::string& name) {
    devices_.erase(name);  // destructor calls stop()
}

IDevice* SingletonManager::getDevice(const std::string& name) {
    auto it = devices_.find(name);
    return it != devices_.end() ? it->second.get() : nullptr;
}

ICamera* SingletonManager::getCamera(const std::string& name) {
    // Downcast to ICamera if the device is a camera
    return dynamic_cast<ICamera*>(getDevice(name));
}

std::vector<ICamera*> SingletonManager::getAllCameras() const {
    std::vector<ICamera*> cameras;
    for (const auto& [name, device] : devices_) {
        if (auto* cam = dynamic_cast<ICamera*>(device.get()))
            cameras.push_back(cam);
    }
    return cameras;
}

std::vector<std::string> SingletonManager::getDeviceNames() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : devices_)
        names.push_back(name);
    return names;
}