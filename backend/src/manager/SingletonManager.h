#pragma once
#include "../devices/IDevice.h"
#include "../devices/ICamera.h"
#include "../factories/AbstractDeviceFactory.h"
#include <map>
#include <memory>
#include <vector>
#include <string>

class SingletonManager {
public:
    static SingletonManager& getInstance();

    // Disable copy/move
    SingletonManager(const SingletonManager&) = delete;
    SingletonManager& operator=(const SingletonManager&) = delete;

    // Add a device via any factory
    bool addDevice(
        AbstractDeviceFactory& factory,
        const std::string& name,
        const std::string& address
    );

    void removeDevice(const std::string& name);

    // Get any device by name
    IDevice* getDevice(const std::string& name);

    // Get specifically cameras (downcast-safe)
    ICamera* getCamera(const std::string& name);
    std::vector<ICamera*> getAllCameras() const;

    std::vector<std::string> getDeviceNames() const;

private:
    SingletonManager() = default;
    std::map<std::string, std::unique_ptr<IDevice>> devices_;
};