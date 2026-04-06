#include "routes/RouteHandler.hpp"

#include <stdexcept>
#include <utility>
#include <vector>
#include <sstream>
#include <regex>

namespace middleware::routes {

struct Route::Impl {
    HttpMethod  method_;
    std::string pattern_;
    HandlerFn   handler_;
    std::regex  compiled_;
    std::vector<std::string> paramNames_;

    Impl(HttpMethod method, std::string pattern, HandlerFn handler)
        : method_(method)
        , pattern_(std::move(pattern))
        , handler_(std::move(handler)) {
        buildRegex();
    }

    void buildRegex() {
        std::string regexStr;
        std::istringstream stream(pattern_);
        std::string segment;

        regexStr += "^";
        while (std::getline(stream, segment, '/')) {
            if (segment.empty()) continue;
            regexStr += "/";
            if (!segment.empty() && segment.front() == ':') {
                paramNames_.push_back(segment.substr(1));
                regexStr += "([^/]+)";
            } else {
                regexStr += segment;
            }
        }
        regexStr += "/?$";
        compiled_ = std::regex(regexStr);
    }

    RouteMatch match(HttpMethod method, std::string_view path) const {
        if (method != method_) return {false, {}};

        std::string pathStr(path);
        std::smatch m;
        if (!std::regex_match(pathStr, m, compiled_)) return {false, {}};

        PathParams params;
        for (std::size_t i = 0; i < paramNames_.size(); ++i) {
            params[paramNames_[i]] = m[static_cast<int>(i + 1)].str();
        }
        return {true, std::move(params)};
    }
};

Route::Route(HttpMethod method, std::string pattern, HandlerFn handler)
    : impl_(std::make_unique<Impl>(method, std::move(pattern), std::move(handler))) {}

RouteMatch Route::match(HttpMethod method, std::string_view path) const {
    return impl_->match(method, path);
}

RouteResponse Route::dispatch(RouteRequest request) const {
    return impl_->handler_(std::move(request));
}

std::string_view Route::pattern() const noexcept {
    return impl_->pattern_;
}

HttpMethod Route::method() const noexcept {
    return impl_->method_;
}

// ===========================================================================

struct Router::Impl {
    std::vector<RoutePtr>           routes;
    std::vector<RouteMiddlewarePtr> middleware;

    RouteResponse route(RouteRequest request) {
        for (auto& mw : middleware) {
            RouteResponse early;
            if (!mw->handle(request, early)) {
                return early;
            }
        }

        for (const auto& r : routes) {
            auto match = r->match(request.method, request.path);
            if (match.matched) {
                request.pathParams = std::move(match.params);
                return r->dispatch(std::move(request));
            }
        }

        return RouteResponse{404, "Not Found", {}};
    }
};

Router::Router()
    : impl_(std::make_unique<Impl>()) {}

Router::~Router() = default;

void Router::addRoute(RoutePtr route) {
    if (!route) throw std::invalid_argument("Router: route must not be null");
    impl_->routes.push_back(std::move(route));
}

void Router::addMiddleware(RouteMiddlewarePtr mw) {
    if (!mw) throw std::invalid_argument("Router: middleware must not be null");
    impl_->middleware.push_back(std::move(mw));
}

RouteResponse Router::route(RouteRequest request) {
    return impl_->route(std::move(request));
}

std::size_t Router::routeCount() const noexcept {
    return impl_->routes.size();
}

std::size_t Router::middlewareCount() const noexcept {
    return impl_->middleware.size();
}

void Router::clearRoutes() noexcept {
    impl_->routes.clear();
}

void Router::clearMiddleware() noexcept {
    impl_->middleware.clear();
}

}
