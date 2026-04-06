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


class IOutputSerializer {
public:
    virtual ~IOutputSerializer() = default;

    virtual Body        serialize(const OutputResponse& response) const = 0;
    virtual std::string contentType() const noexcept = 0;
};

using SerializerPtr = std::unique_ptr<IOutputSerializer>;


class IOutputFilter {
public:
    virtual ~IOutputFilter() = default;

    virtual void        apply(OutputResponse& response) const = 0;
    virtual std::string_view name() const noexcept = 0;
};

using FilterPtr = std::unique_ptr<IOutputFilter>;


class IOutputWriter {
public:
    virtual ~IOutputWriter() = default;

    virtual void        write(const OutputResponse& response) = 0;
    virtual void        flush() noexcept = 0;
    virtual std::string_view writerName() const noexcept = 0;
};

using WriterPtr = std::unique_ptr<IOutputWriter>;


class IOutputPipeline {
public:
    virtual ~IOutputPipeline() = default;

    virtual void send(OutputResponse response) = 0;

    virtual void setSerializer(SerializerPtr serializer) = 0;
    virtual void addFilter(FilterPtr filter) = 0;
    virtual void setWriter(WriterPtr writer) = 0;

    [[nodiscard]] virtual std::size_t filterCount() const noexcept = 0;
};


class OutputPipeline final : public IOutputPipeline {
public:
    OutputPipeline();
    ~OutputPipeline() override;

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

}