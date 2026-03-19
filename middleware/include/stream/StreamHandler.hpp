#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstddef>

namespace middleware::stream {

class IStreamSource;
class IStreamSink;
class IStreamTransform;
class IStreamHandler;
class StreamHandler;

using Chunk         = std::vector<std::uint8_t>;
using OnChunkFn     = std::function<void(const Chunk&)>;
using OnCompleteFn  = std::function<void()>;
using OnErrorFn     = std::function<void(std::string_view)>;

enum class StreamState : std::uint8_t {
    Idle,
    Open,
    Paused,
    Closed,
    Error,
};

struct StreamCallbacks {
    OnChunkFn    onChunk;
    OnCompleteFn onComplete;
    OnErrorFn    onError;
};

class IStreamSource {
public:
    virtual ~IStreamSource() = default;

    virtual void        open()                           = 0;
    virtual void        close() noexcept                 = 0;
    virtual Chunk       read(std::size_t maxBytes)       = 0;
    [[nodiscard]] virtual bool        isOpen()  const noexcept = 0;
    [[nodiscard]] virtual bool        hasMore() const noexcept = 0;
    [[nodiscard]] virtual std::string_view sourceName() const noexcept = 0;
};

using SourcePtr = std::unique_ptr<IStreamSource>;

class IStreamSink {
public:
    virtual ~IStreamSink() = default;

    virtual void write(const Chunk& chunk)  = 0;
    virtual void flush() noexcept           = 0;
    virtual void close() noexcept           = 0;
    [[nodiscard]] virtual std::string_view sinkName() const noexcept = 0;
};

using SinkPtr = std::unique_ptr<IStreamSink>;

class IStreamTransform {
public:
    virtual ~IStreamTransform() = default;

    virtual Chunk            transform(const Chunk& input) = 0;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
};

using TransformPtr = std::unique_ptr<IStreamTransform>;

class IStreamHandler {
public:
    virtual ~IStreamHandler() = default;

    virtual void setSource(SourcePtr source)         = 0;
    virtual void setSink(SinkPtr sink)               = 0;
    virtual void addTransform(TransformPtr transform) = 0;
    virtual void setCallbacks(StreamCallbacks callbacks) = 0;

    virtual void start()        = 0;
    virtual void pause()        = 0;
    virtual void resume()       = 0;
    virtual void stop() noexcept = 0;

    [[nodiscard]] virtual StreamState  state()          const noexcept = 0;
    [[nodiscard]] virtual std::size_t  bytesProcessed() const noexcept = 0;
    [[nodiscard]] virtual std::size_t  transformCount() const noexcept = 0;
};

class StreamHandler final : public IStreamHandler {
public:
    explicit StreamHandler(std::size_t chunkSize = 4096);
    ~StreamHandler() override;

    void setSource(SourcePtr source)              override;
    void setSink(SinkPtr sink)                    override;
    void addTransform(TransformPtr transform)     override;
    void setCallbacks(StreamCallbacks callbacks)  override;

    void start()         override;
    void pause()         override;
    void resume()        override;
    void stop() noexcept override;

    [[nodiscard]] StreamState  state()          const noexcept override;
    [[nodiscard]] std::size_t  bytesProcessed() const noexcept override;
    [[nodiscard]] std::size_t  transformCount() const noexcept override;

    void clearTransforms() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}