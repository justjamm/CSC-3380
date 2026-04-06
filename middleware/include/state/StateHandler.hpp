#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include <unordered_map>
#include <vector>
#include <any>
#include <optional>
#include <cstdint>

namespace middleware::state {

class IStateStore;
class IStateObserver;
class IStateValidator;
class IStateHandler;
class StateHandler;

using StateKey      = std::string;
using StateValue    = std::any;
using StateMap      = std::unordered_map<StateKey, StateValue>;
using OnChangeFn    = std::function<void(const StateKey&, const StateValue&)>;

enum class StateScope : std::uint8_t {
    Request,
    Session,
    Application,
};

struct StateEntry {
    StateKey   key;
    StateValue value;
    StateScope scope{StateScope::Request};
};

class IStateObserver {
public:
    virtual ~IStateObserver() = default;

    virtual void onStateChanged(const StateKey& key, const StateValue& value) = 0;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
};

using ObserverPtr = std::unique_ptr<IStateObserver>;

class IStateValidator {
public:
    virtual ~IStateValidator() = default;

    virtual bool             validate(const StateKey& key, const StateValue& value) const = 0;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
};

using ValidatorPtr = std::unique_ptr<IStateValidator>;

class IStateStore {
public:
    virtual ~IStateStore() = default;

    virtual void                       set(const StateKey& key, StateValue value, StateScope scope = StateScope::Request) = 0;
    virtual void                       remove(const StateKey& key) noexcept = 0;
    virtual void                       clear(StateScope scope) noexcept = 0;

    [[nodiscard]] virtual std::optional<StateValue> get(const StateKey& key) const = 0;
    [[nodiscard]] virtual bool                      has(const StateKey& key) const noexcept = 0;
    [[nodiscard]] virtual std::size_t               count(StateScope scope) const noexcept = 0;
};

using StorePtr = std::unique_ptr<IStateStore>;

class IStateHandler {
public:
    virtual ~IStateHandler() = default;

    virtual void setStore(StorePtr store)              = 0;
    virtual void addObserver(ObserverPtr observer)     = 0;
    virtual void addValidator(ValidatorPtr validator)  = 0;

    virtual void set(const StateKey& key, StateValue value, StateScope scope = StateScope::Request) = 0;
    virtual void remove(const StateKey& key) noexcept  = 0;
    virtual void clear(StateScope scope) noexcept       = 0;

    [[nodiscard]] virtual std::optional<StateValue> get(const StateKey& key) const = 0;
    [[nodiscard]] virtual bool                      has(const StateKey& key) const noexcept = 0;
    [[nodiscard]] virtual std::size_t               observerCount()  const noexcept = 0;
    [[nodiscard]] virtual std::size_t               validatorCount() const noexcept = 0;
};

class StateHandler final : public IStateHandler {
public:
    StateHandler();
    ~StateHandler() override;

    void setStore(StorePtr store)              override;
    void addObserver(ObserverPtr observer)     override;
    void addValidator(ValidatorPtr validator)  override;

    void set(const StateKey& key, StateValue value, StateScope scope = StateScope::Request) override;
    void remove(const StateKey& key) noexcept  override;
    void clear(StateScope scope) noexcept       override;

    [[nodiscard]] std::optional<StateValue> get(const StateKey& key) const override;
    [[nodiscard]] bool                      has(const StateKey& key) const noexcept override;
    [[nodiscard]] std::size_t               observerCount()  const noexcept override;
    [[nodiscard]] std::size_t               validatorCount() const noexcept override;

    void clearObservers()  noexcept;
    void clearValidators() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}