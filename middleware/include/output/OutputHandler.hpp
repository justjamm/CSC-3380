#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace middleware::output {

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
class IOutputSerializer;
class IOutputWriter;
class IOutputFilter;
class IOutputPipeline;
class OutputPipeline;

// ---------------------------------------------------------------------------
// Aliases
// ---------------------------------------------------------------------------
using Headers    = std::unordered_map<std::string, std::string>;
using Body       = std::vector<std::uint8_t>;
using FilterFn   = std::function<bool(const std::string_view)>;

// ---------------------------------------------------------------------------
// StatusCode
// Typed HTTP-aligned status codes; avoids raw integer scatter.
// ---------------------------------------------------------------------------
enum class StatusCode : std::uint16_t {
    Ok                  = 200,
    Created             = 201,
    NoContent           = 204,
    BadRequest          = 400,
    Unauthorized        = 401,
    Forbidden           = 403,
    NotFound            = 404,
    InternalServerError = 500,
    ServiceUnavailable  = 503,
};

// ---------------------------------------------------------------------------
// OutputResponse
// Plain value type representing the fully-assembled outbound response.
// Passed by value through the pipeline; copy-constructible for serialisation.
// ---------------------------------------------------------------------------
struct OutputResponse {
    StatusCode  status{StatusCode::Ok};
    Headers     headers;
    Body        body;
    std::string contentType{"application/json"};

    [[nodiscard]] bool isSuccess() const noexcept {
        const auto code = static_cast<std::uint16_t>(status);
        return code >= 200 && code < 300;
    }
};

// ---------------------------------------------------------------------------
// IOutputSerializer
// Transforms an arbitrary payload into a raw byte body (SRP).
// One implementation per format: JSON, MessagePack, plain text, etc.
// ---------------------------------------------------------------------------
class IOutputSerializer {
public:
    virtual ~IOutputSerializer() = default;

    virtual Body        serialize(const OutputResponse& response) const = 0;
    virtual std::string contentType() const noexcept = 0;
};

using SerializerPtr = std::unique_ptr<IOutputSerializer>;

// ---------------------------------------------------------------------------
// IOutputFilter
// Single-responsibility transform applied to a response before writing.
// Filters are composable and must not hold response state between calls.
// ---------------------------------------------------------------------------
class IOutputFilter {
public:
    virtual ~IOutputFilter() = default;

    virtual void        apply(OutputResponse& response) const = 0;
    virtual std::string_view name() const noexcept = 0;
};

using FilterPtr = std::unique_ptr<IOutputFilter>;

// ---------------------------------------------------------------------------
// IOutputWriter
// Flushes the final OutputResponse to a transport (socket, buffer, file).
// Decoupled from serialisation (ISP: knows nothing about format).
// ---------------------------------------------------------------------------
class IOutputWriter {
public:
    virtual ~IOutputWriter() = default;

    virtual void        write(const OutputResponse& response) = 0;
    virtual void        flush() noexcept = 0;
    virtual std::string_view writerName() const noexcept = 0;
};

using WriterPtr = std::unique_ptr<IOutputWriter>;

// ---------------------------------------------------------------------------
// IOutputPipeline
// Orchestrates serialise → filter → write for a single response (SRP / OCP).
// Callers inject concrete stages; the pipeline owns their lifetimes.
// ---------------------------------------------------------------------------
class IOutputPipeline {
public:
    virtual ~IOutputPipeline() = default;

    virtual void send(OutputResponse response) = 0;

    virtual void setSerializer(SerializerPtr serializer) = 0;
    virtual void addFilter(FilterPtr filter) = 0;
    virtual void setWriter(WriterPtr writer) = 0;

    [[nodiscard]] virtual std::size_t filterCount() const noexcept = 0;
};

// ---------------------------------------------------------------------------
// OutputPipeline
// Concrete, LSP-compliant implementation of IOutputPipeline.
// Executes filters in insertion order; thread-compatible (not thread-safe).
// ---------------------------------------------------------------------------
class OutputPipeline final : public IOutputPipeline {
public:
    OutputPipeline();
    ~OutputPipeline() override;

    // IOutputPipeline
    void send(OutputResponse response)       override;
    void setSerializer(SerializerPtr serializer) override;
    void addFilter(FilterPtr filter)         override;
    void setWriter(WriterPtr writer)         override;
    [[nodiscard]] std::size_t filterCount() const noexcept override;

    void clearFilters() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace middleware::output