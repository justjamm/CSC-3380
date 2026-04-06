#include "middleware\include\state\StateHandler.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace middleware::state {

struct StateHandler::Impl {
    StorePtr                    store;
    std::vector<ObserverPtr>    observers;
    std::vector<ValidatorPtr>   validators;

    void notifyObservers(const StateKey& key, const StateValue& value) {
        for (const auto& observer : observers) {
            observer->onStateChanged(key, value);
        }
    }

    bool runValidators(const StateKey& key, const StateValue& value) const {
        for (const auto& validator : validators) {
            if (!validator->validate(key, value)) return false;
        }
        return true;
    }

    void set(const StateKey& key, StateValue value, StateScope scope) {
        if (!store) throw std::runtime_error("StateHandler: no store set");

        if (!runValidators(key, value)) {
            throw std::invalid_argument("StateHandler: validation failed for key: " + key);
        }

        store->set(key, value, scope);
        notifyObservers(key, value);
    }

    void remove(const StateKey& key) noexcept {
        if (store) store->remove(key);
    }

    void clear(StateScope scope) noexcept {
        if (store) store->clear(scope);
    }

    std::optional<StateValue> get(const StateKey& key) const {
        if (!store) return std::nullopt;
        return store->get(key);
    }

    bool has(const StateKey& key) const noexcept {
        if (!store) return false;
        return store->has(key);
    }
};

StateHandler::StateHandler()
    : impl_(std::make_unique<Impl>()) {}

StateHandler::~StateHandler() = default;

void StateHandler::setStore(StorePtr store) {
    if (!store) throw std::invalid_argument("StateHandler: store must not be null");
    impl_->store = std::move(store);
}

void StateHandler::addObserver(ObserverPtr observer) {
    if (!observer) throw std::invalid_argument("StateHandler: observer must not be null");
    impl_->observers.push_back(std::move(observer));
}

void StateHandler::addValidator(ValidatorPtr validator) {
    if (!validator) throw std::invalid_argument("StateHandler: validator must not be null");
    impl_->validators.push_back(std::move(validator));
}

void StateHandler::set(const StateKey& key, StateValue value, StateScope scope) {
    impl_->set(key, std::move(value), scope);
}

void StateHandler::remove(const StateKey& key) noexcept {
    impl_->remove(key);
}

void StateHandler::clear(StateScope scope) noexcept {
    impl_->clear(scope);
}

std::optional<StateValue> StateHandler::get(const StateKey& key) const {
    return impl_->get(key);
}

bool StateHandler::has(const StateKey& key) const noexcept {
    return impl_->has(key);
}

std::size_t StateHandler::observerCount() const noexcept {
    return impl_->observers.size();
}

std::size_t StateHandler::validatorCount() const noexcept {
    return impl_->validators.size();
}

void StateHandler::clearObservers() noexcept {
    impl_->observers.clear();
}

void StateHandler::clearValidators() noexcept {
    impl_->validators.clear();
}

}