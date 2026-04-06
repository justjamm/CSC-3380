#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <cstdint>

namespace middleware::routes {

class IRoute;
class IRouter;
class IRouteMiddleware;
class Router;

using PathParams  = std::unordered_map<std::string, std::string>;
using QueryParams = std::unordered_map<std::string, std::string>;

enum class HttpMethod : std::uint8_t {
    GET,
    POST,
    PUT,
    PATCH,
    DELETE,
    OPTIONS,
    HEAD,
};

struct RouteRequest {
    HttpMethod  method;
    std::string path;
    PathParams  pathParams;
    QueryParams queryParams;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
};

struct RouteResponse {
    int         status{200};
    std::string body;
    std::unordered_map<std::string, std::string> headers;
};

using HandlerFn = std::function<RouteResponse(const RouteRequest&)>;

struct RouteMatch {
    bool       matched{false};
    PathParams params;
};

class IRouteMiddleware {
public:
    virtual ~IRouteMiddleware() = default;

    virtual bool            handle(RouteRequest& request, RouteResponse& response) = 0;
    virtual std::string_view name() const noexcept = 0;
};

using RouteMiddlewarePtr = std::unique_ptr<IRouteMiddleware>;

class IRoute {
public:
    virtual ~IRoute() = default;

    virtual RouteMatch       match(HttpMethod method, std::string_view path) const = 0;
    virtual RouteResponse    dispatch(RouteRequest request) const = 0;
    virtual std::string_view pattern() const noexcept = 0;
    virtual HttpMethod       method()  const noexcept = 0;
};

using RoutePtr = std::unique_ptr<IRoute>;

class IRouter {
public:
    virtual ~IRouter() = default;

    virtual void          addRoute(RoutePtr route) = 0;
    virtual void          addMiddleware(RouteMiddlewarePtr middleware) = 0;
    virtual RouteResponse route(RouteRequest request) = 0;

    [[nodiscard]] virtual std::size_t routeCount()      const noexcept = 0;
    [[nodiscard]] virtual std::size_t middlewareCount() const noexcept = 0;
};

class Route final : public IRoute {
public:
    Route(HttpMethod method, std::string pattern, HandlerFn handler);
    ~Route() override = default;

    RouteMatch       match(HttpMethod method, std::string_view path) const override;
    RouteResponse    dispatch(RouteRequest request)                  const override;
    std::string_view pattern() const noexcept override;
    HttpMethod       method()  const noexcept override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class Router final : public IRouter {
public:
    Router();
    ~Router() override;

    void          addRoute(RoutePtr route)               override;
    void          addMiddleware(RouteMiddlewarePtr mw)   override;
    RouteResponse route(RouteRequest request)            override;

    [[nodiscard]] std::size_t routeCount()      const noexcept override;
    [[nodiscard]] std::size_t middlewareCount() const noexcept override;

    void clearRoutes()     noexcept;
    void clearMiddleware() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}