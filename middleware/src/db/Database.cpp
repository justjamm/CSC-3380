#include "middleware\include\db\Database.hpp"

#include <cassert>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <utility>

namespace middleware::db {



PooledConnection::PooledConnection(IDbConnectionPool& pool,
                                   ConnectionPtr       connection) noexcept
    : pool_(&pool), connection_(std::move(connection)) {}

PooledConnection::PooledConnection(PooledConnection&& other) noexcept
    : pool_(other.pool_), connection_(std::move(other.connection_)) {
    other.pool_ = nullptr;
}

PooledConnection& PooledConnection::operator=(PooledConnection&& other) noexcept {
    if (this != &other) {

        if (pool_ && connection_) {
            pool_->release(std::move(connection_));
        }
        pool_       = other.pool_;
        connection_ = std::move(other.connection_);
        other.pool_ = nullptr;
    }
    return *this;
}

PooledConnection::~PooledConnection() {
    if (pool_ && connection_) {
        pool_->release(std::move(connection_));
    }
}

IDbConnection& PooledConnection::get() noexcept {
    assert(connection_ && "Dereferencing a moved-from PooledConnection");
    return *connection_;
}

const IDbConnection& PooledConnection::get() const noexcept {
    assert(connection_ && "Dereferencing a moved-from PooledConnection");
    return *connection_;
}


struct DbConnectionPool::Impl {
    explicit Impl(DbConnectionConfig                  cfg,
                  std::function<ConnectionPtr()>      factory)
        : config_(std::move(cfg))
        , factory_(std::move(factory))
        , active_(0) {}

    Impl(const Impl&)            = delete;
    Impl& operator=(const Impl&) = delete;

    DbConnectionConfig             config_;
    std::function<ConnectionPtr()> factory_;

    mutable std::mutex          mtx_;
    std::condition_variable     cv_;
    std::queue<ConnectionPtr>   idle_;
    std::size_t                 active_;
    bool                        shuttingDown_{false};

    ConnectionPtr acquire() {
        std::unique_lock lock(mtx_);

        cv_.wait(lock, [this] {
            return shuttingDown_
                || !idle_.empty()
                || (active_ < config_.maxPoolSize);
        });

        if (shuttingDown_) {
            throw std::runtime_error("DbConnectionPool: pool is shutting down");
        }

        ConnectionPtr conn;

        if (!idle_.empty()) {
            conn = std::move(idle_.front());
            idle_.pop();
        } else {
            
            lock.unlock();
            conn = factory_();
            if (!conn) {
                throw std::runtime_error("DbConnectionPool: factory returned null");
            }
            conn->open();
            lock.lock();
        }

        ++active_;
        return conn;
    }

    void release(ConnectionPtr conn) noexcept {
        if (!conn) { return; }

        std::lock_guard lock(mtx_);

        if (shuttingDown_ || !conn->isOpen()) {
            --active_;
            cv_.notify_one();
            return;
        }

        --active_;
        idle_.push(std::move(conn));
        cv_.notify_one();
    }

    void initialise(std::size_t minConnections) {
        const std::size_t target =
            std::min(minConnections, config_.maxPoolSize);

        for (std::size_t i = 0; i < target; ++i) {
            auto conn = factory_();
            conn->open();
            std::lock_guard lock(mtx_);
            idle_.push(std::move(conn));
        }
    }

    void shutdown() noexcept {
        std::lock_guard lock(mtx_);
        shuttingDown_ = true;
        
        while (!idle_.empty()) {
            idle_.pop(); 
        }
        cv_.notify_all();
    }

    [[nodiscard]] std::size_t idleCount() const noexcept {
        std::lock_guard lock(mtx_);
        return idle_.size();
    }

    [[nodiscard]] std::size_t activeCount() const noexcept {
        std::lock_guard lock(mtx_);
        return active_;
    }
};


DbConnectionPool::DbConnectionPool(DbConnectionConfig             config,
                                   std::function<ConnectionPtr()> factory)
    : impl_(std::make_unique<Impl>(std::move(config), std::move(factory))) {}

DbConnectionPool::~DbConnectionPool() {
    if (impl_) {
        impl_->shutdown();
    }
}

ConnectionPtr DbConnectionPool::acquire() {
    return impl_->acquire();
}

void DbConnectionPool::release(ConnectionPtr conn) noexcept {
    impl_->release(std::move(conn));
}

std::size_t DbConnectionPool::idleCount() const noexcept {
    return impl_->idleCount();
}

std::size_t DbConnectionPool::activeCount() const noexcept {
    return impl_->activeCount();
}

const DbConnectionConfig& DbConnectionPool::config() const noexcept {
    return impl_->config_;
}

void DbConnectionPool::initialise(std::size_t minConnections) {
    impl_->initialise(minConnections);
}

void DbConnectionPool::shutdown() noexcept {
    impl_->shutdown();
}

} 