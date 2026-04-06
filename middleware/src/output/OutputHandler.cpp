#include "middleware\include\output\OutputHandler.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace middleware::output {


struct OutputPipeline::Impl {
    SerializerPtr              serializer;
    std::vector<FilterPtr>     filters;
    WriterPtr                  writer;

    void send(OutputResponse response) {
        if (!serializer) {
            throw std::runtime_error("OutputPipeline: no serializer set");
        }
        if (!writer) {
            throw std::runtime_error("OutputPipeline: no writer set");
        }

        for (const auto& filter : filters) {
            filter->apply(response);
        }

        response.body        = serializer->serialize(response);
        response.contentType = serializer->contentType();

        writer->write(response);
        writer->flush();
    }

    void clearFilters() noexcept {
        filters.clear();
    }
};


OutputPipeline::OutputPipeline()
    : impl_(std::make_unique<Impl>()) {}

OutputPipeline::~OutputPipeline() = default;

void OutputPipeline::send(OutputResponse response) {
    impl_->send(std::move(response));
}

void OutputPipeline::setSerializer(SerializerPtr serializer) {
    if (!serializer) {
        throw std::invalid_argument("OutputPipeline: serializer must not be null");
    }
    impl_->serializer = std::move(serializer);
}

void OutputPipeline::addFilter(FilterPtr filter) {
    if (!filter) {
        throw std::invalid_argument("OutputPipeline: filter must not be null");
    }
    impl_->filters.push_back(std::move(filter));
}

void OutputPipeline::setWriter(WriterPtr writer) {
    if (!writer) {
        throw std::invalid_argument("OutputPipeline: writer must not be null");
    }
    impl_->writer = std::move(writer);
}

std::size_t OutputPipeline::filterCount() const noexcept {
    return impl_->filters.size();
}

void OutputPipeline::clearFilters() noexcept {
    impl_->clearFilters();
}

}