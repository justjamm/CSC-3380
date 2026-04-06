#pragma once

#include <curl/curl.h>
#include <cstdint>
#include <string>
#include <vector>

class MjpegClient {
public:
    explicit MjpegClient(const std::string& url);
    ~MjpegClient();

    MjpegClient(const MjpegClient&) = delete;
    MjpegClient& operator=(const MjpegClient&) = delete;

    // Blocking — pumps curl until a complete JPEG frame arrives.
    // Returns empty vector on disconnect (caller should reconnect).
    std::vector<uint8_t> nextFrame();

    void connect();
    void disconnect();
    bool isConnected() const { return connected_; }

private:
    static size_t writeCallback(char* data, size_t size, size_t nmemb, void* userdata);
    bool extractFrame(std::vector<uint8_t>& out);

    std::string url_;
    CURL* easy_ = nullptr;
    CURLM* multi_ = nullptr;
    bool connected_ = false;

    std::vector<uint8_t> buf_;
};
