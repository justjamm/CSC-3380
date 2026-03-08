
#include <string>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <functional>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace core {


using UserId        = std::string;   
using DeviceId      = std::string; 
using SessionToken  = std::string;   
using ChallengeId   = std::string;   
using StreamId      = std::string;  
using AlertId       = std::string;   
using Timestamp     = std::chrono::system_clock::time_point;


enum class ServiceStatus {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    ERROR
};


enum class AlertSeverity { LOW, MEDIUM, HIGH, CRITICAL };

enum class TrafficDirection { IOT_TO_ROUTER, ROUTER_TO_IOT, UNKNOWN };

struct TokenClaims {
    UserId    user_id;
    Timestamp issued_at;
    Timestamp expires_at;
};

struct StreamSource {
    StreamId    stream_id;
    std::string rtsp_url;
    DeviceId    device_id;   
    std::string label;        
};

struct CameraSelectQuery {
    SessionToken token;       
    StreamId     stream_id;
};


struct PacketDescriptor {
    Timestamp        timestamp;
    std::string      src_ip;
    std::string      dst_ip;
    uint16_t         src_port{};
    uint16_t         dst_port{};
    uint32_t         payload_bytes{};
    TrafficDirection direction{ TrafficDirection::UNKNOWN };
    DeviceId         device_id;
};


struct EdrAlert {
    AlertId          alert_id;
    Timestamp        timestamp;
    AlertSeverity    severity;
    std::string      rule_name;
    std::string      description;
    PacketDescriptor source_packet;
};


enum class ErrorCode : uint32_t {
    UNKNOWN             = 0,
    NOT_FOUND           = 1,
    INVALID_INPUT       = 2,
    INTERNAL_ERROR      = 3,
    WRONG_CREDENTIALS   = 100,
    ACCOUNT_LOCKED      = 101,
    RATE_LIMITED        = 102,
    OTP_EXPIRED         = 103,
    OTP_WRONG_CODE      = 104,
    OTP_REPLAY          = 105,
    OTP_LOCKED_OUT      = 106,
    TOKEN_INVALID       = 107,
    TOKEN_EXPIRED       = 108,
    STREAM_NOT_FOUND    = 200,
    STREAM_UNAVAILABLE  = 201,
    UNAUTHORIZED_STREAM = 202,
    DB_CONNECTION_FAIL  = 300,
    DB_QUERY_FAIL       = 301,
    RECORD_NOT_FOUND    = 302,
    RULE_EVAL_FAIL      = 400,
    ALERT_QUEUE_FULL    = 401,
    DEVICE_UNREACHABLE  = 500,
    COMMAND_REJECTED    = 501,
    CAPTURE_FAIL        = 502,
};

struct Error {
    ErrorCode   code{ ErrorCode::UNKNOWN };
    std::string message;
};

template <typename T>
class Result {
public:
    

    static Result ok(const T& value) {
        Result r;
        r.ok_    = true;
        r.value_ = value;
        return r;
    }

    static Result fail(ErrorCode code, const std::string& message) {
        Result r;
        r.ok_    = false;
        r.error_ = Error{ code, message };
        return r;
    }


    bool is_ok()    const { return ok_; }
    bool is_error() const { return !ok_; }

    const T& value() const {
        if (!ok_)
            throw std::logic_error("Result::value() called on an error result");
        return value_;
    }

    T& value() {
        if (!ok_)
            throw std::logic_error("Result::value() called on an error result");
        return value_;
    }

    const Error& error() const {
        if (ok_)
            throw std::logic_error("Result::error() called on a success result");
        return error_;
    }

private:
    bool  ok_{ false };
    T     value_{};   
    Error error_{};    
};


template <>
class Result<void> {
public:
    static Result ok() {
        Result r;
        r.ok_ = true;
        return r;
    }

    static Result fail(ErrorCode code, const std::string& message) {
        Result r;
        r.ok_    = false;
        r.error_ = Error{ code, message };
        return r;
    }

    bool is_ok()    const { return ok_; }
    bool is_error() const { return !ok_; }

    const Error& error() const {
        if (ok_)
            throw std::logic_error("Result::error() called on a success result");
        return error_;
    }

private:
    bool  ok_{ false };
    Error error_{};
};

class IService {
public:
    virtual ~IService() = default;

    virtual void          start()        = 0;
    virtual void          stop()         = 0;
    virtual const char*   name()   const = 0;
    virtual ServiceStatus status() const = 0;
};

class ITokenValidator {
public:
    virtual ~ITokenValidator() = default;

    virtual Result<TokenClaims> validate(const SessionToken& token) const = 0;
};

class IAlertSink {
public:
    virtual ~IAlertSink() = default;

    virtual void on_alert(const EdrAlert& alert) = 0;
};

class IPacketSource {
public:
    virtual ~IPacketSource() = default;

    virtual void on_packet(std::function<void(const PacketDescriptor&)> callback) = 0;
};

class ServiceLocator {
public:
    template <typename T>
    void register_service(std::shared_ptr<T> service) {
        services_[std::type_index(typeid(T))] =
            std::static_pointer_cast<void>(std::move(service));
    }

    template <typename T>
    std::shared_ptr<T> resolve() const {
        auto it = services_.find(std::type_index(typeid(T)));
        if (it == services_.end())
            throw std::runtime_error(
                std::string("ServiceLocator: no service registered for '") +
                typeid(T).name() + "'");
        return std::static_pointer_cast<T>(it->second);
    }

    
    template <typename T>
    bool has() const {
        return services_.count(std::type_index(typeid(T))) > 0;
    }

    
    template <typename T>
    void unregister() { services_.erase(std::type_index(typeid(T))); }

   
    void clear() { services_.clear(); }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> services_;
};

} 