#include "MjpegClient.h"
#include <algorithm>
#include <cstring>
#include <iostream>

MjpegClient::MjpegClient(const std::string& url) : url_(url) {}

MjpegClient::~MjpegClient() {
    disconnect();
}

void MjpegClient::connect() {
    disconnect();

    easy_ = curl_easy_init();
    if (!easy_) {
        std::cerr << "[MjpegClient] Failed to init curl" << std::endl;
        return;
    }

    curl_easy_setopt(easy_, CURLOPT_URL, url_.c_str());
    curl_easy_setopt(easy_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(easy_, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(easy_, CURLOPT_TIMEOUT, 0L);
    curl_easy_setopt(easy_, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(easy_, CURLOPT_LOW_SPEED_TIME, 10L);
    curl_easy_setopt(easy_, CURLOPT_NOSIGNAL, 1L);

    multi_ = curl_multi_init();
    curl_multi_add_handle(multi_, easy_);

    connected_ = true;
    buf_.clear();
}

void MjpegClient::disconnect() {
    if (multi_ && easy_) {
        curl_multi_remove_handle(multi_, easy_);
    }
    if (multi_) {
        curl_multi_cleanup(multi_);
        multi_ = nullptr;
    }
    if (easy_) {
        curl_easy_cleanup(easy_);
        easy_ = nullptr;
    }
    connected_ = false;
    buf_.clear();
}

size_t MjpegClient::writeCallback(char* data, size_t size, size_t nmemb, void* userdata) {
    auto* self = static_cast<MjpegClient*>(userdata);
    size_t total = size * nmemb;
    self->buf_.insert(self->buf_.end(), data, data + total);
    return total;
}

bool MjpegClient::extractFrame(std::vector<uint8_t>& out) {
    const std::string boundary = "--frame\r\n";
    auto it = std::search(buf_.begin(), buf_.end(), boundary.begin(), boundary.end());
    if (it == buf_.end()) return false;

    auto header_start = it + boundary.size();

    const std::string header_end_marker = "\r\n\r\n";
    auto header_end = std::search(header_start, buf_.end(),
                                  header_end_marker.begin(), header_end_marker.end());
    if (header_end == buf_.end()) return false;

    // Parse Content-Length
    std::string headers(header_start, header_end);
    size_t content_length = 0;
    const std::string cl_key = "Content-Length: ";
    auto cl_pos = headers.find(cl_key);
    if (cl_pos != std::string::npos) {
        content_length = std::stoul(headers.substr(cl_pos + cl_key.size()));
    }
    if (content_length == 0) return false;

    auto data_start = header_end + header_end_marker.size();
    size_t available = static_cast<size_t>(std::distance(data_start, buf_.end()));
    if (available < content_length) return false;

    out.assign(data_start, data_start + content_length);

    // Consume through trailing \r\n
    auto consume_end = data_start + content_length;
    if (consume_end + 2 <= buf_.end() && consume_end[0] == '\r' && consume_end[1] == '\n') {
        consume_end += 2;
    }
    buf_.erase(buf_.begin(), consume_end);

    return true;
}

std::vector<uint8_t> MjpegClient::nextFrame() {
    std::vector<uint8_t> frame;

    if (!connected_ || !multi_) return frame;

    while (true) {
        // Try to extract a frame from what we already have
        if (extractFrame(frame)) return frame;

        // Pump curl for more data
        int still_running = 0;
        CURLMcode mc = curl_multi_perform(multi_, &still_running);
        if (mc != CURLM_OK) {
            std::cerr << "[MjpegClient] curl error: " << curl_multi_strerror(mc) << std::endl;
            disconnect();
            return frame;
        }

        // Check if a complete frame arrived from that perform
        if (extractFrame(frame)) return frame;

        if (still_running == 0) {
            // Stream ended
            disconnect();
            return frame;
        }

        // Block up to 200ms waiting for socket activity
        int numfds = 0;
        curl_multi_wait(multi_, nullptr, 0, 200, &numfds);
    }
}
