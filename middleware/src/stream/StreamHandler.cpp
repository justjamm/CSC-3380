#include "stream/StreamHandler.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace middleware::stream {

struct StreamHandler::Impl {
    SourcePtr                  source;
    SinkPtr                    sink;
    std::vector<TransformPtr>  transforms;
    StreamCallbacks            callbacks;
    StreamState                state{StreamState::Idle};
    std::size_t                bytesProcessed{0};
    std::size_t                chunkSize;

    explicit Impl(std::size_t chunkSize)
        : chunkSize(chunkSize) {}

    void start() {
        if (!source) throw std::runtime_error("StreamHandler: no source set");
        if (!sink)   throw std::runtime_error("StreamHandler: no sink set");

        source->open();
        state = StreamState::Open;

        while (source->hasMore()) {
            if (state == StreamState::Paused) break;
            if (state == StreamState::Error)  break;

            Chunk chunk = source->read(chunkSize);
            if (chunk.empty()) break;

            for (const auto& t : transforms) {
                chunk = t->transform(chunk);
            }

            bytesProcessed += chunk.size();

            sink->write(chunk);

            if (callbacks.onChunk) {
                callbacks.onChunk(chunk);
            }
        }

        if (state == StreamState::Open) {
            sink->flush();
            sink->close();
            source->close();
            state = StreamState::Closed;

            if (callbacks.onComplete) {
                callbacks.onComplete();
            }
        }
    }

    void pause() {
        if (state == StreamState::Open) {
            state = StreamState::Paused;
        }
    }

    void resume() {
        if (state == StreamState::Paused) {
            state = StreamState::Open;
            start();
        }
    }

    void stop() noexcept {
        if (source && source->isOpen()) source->close();
        if (sink)                       sink->close();
        state = StreamState::Closed;
    }

    void signalError(std::string_view message) noexcept {
        state = StreamState::Error;
        if (callbacks.onError) {
            callbacks.onError(message);
        }
    }
};

StreamHandler::StreamHandler(std::size_t chunkSize)
    : impl_(std::make_unique<Impl>(chunkSize)) {}

StreamHandler::~StreamHandler() {
    if (impl_) impl_->stop();
}

void StreamHandler::setSource(SourcePtr source) {
    if (!source) throw std::invalid_argument("StreamHandler: source must not be null");
    impl_->source = std::move(source);
}

void StreamHandler::setSink(SinkPtr sink) {
    if (!sink) throw std::invalid_argument("StreamHandler: sink must not be null");
    impl_->sink = std::move(sink);
}

void StreamHandler::addTransform(TransformPtr transform) {
    if (!transform) throw std::invalid_argument("StreamHandler: transform must not be null");
    impl_->transforms.push_back(std::move(transform));
}

void StreamHandler::setCallbacks(StreamCallbacks callbacks) {
    impl_->callbacks = std::move(callbacks);
}

void StreamHandler::start() {
    impl_->start();
}

void StreamHandler::pause() {
    impl_->pause();
}

void StreamHandler::resume() {
    impl_->resume();
}

void StreamHandler::stop() noexcept {
    impl_->stop();
}

StreamState StreamHandler::state() const noexcept {
    return impl_->state;
}

std::size_t StreamHandler::bytesProcessed() const noexcept {
    return impl_->bytesProcessed;
}

std::size_t StreamHandler::transformCount() const noexcept {
    return impl_->transforms.size();
}

void StreamHandler::clearTransforms() noexcept {
    impl_->transforms.clear();
}

}
