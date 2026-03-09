#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <chrono>

namespace middleware::db {

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
class IDbConnection;
class IDbStatement;
class IDbTransaction;
class IDbConnectionPool;
class DbConnectionPool;

// ---------------------------------------------------------------------------
// Aliases
// ---------------------------------------------------------------------------
using Row       = std::unordered_map<std::string, std::string>;
using ResultSet = std::vector<Row>;
using Params    = std::vector<std::string>;

// ---------------------------------------------------------------------------
// IDbStatement
// Represents a single prepared SQL statement (SRP).
// Owns no connection; lifetime must not exceed the issuing IDbConnection.
// ---------------------------------------------------------------------------
class IDbStatement {
public:
    virtual ~IDbStatement() = default;

    virtual void bind(int index, std::string_view value) = 0;

    [[nodiscard]] virtual int execute() = 0;

    [[nodiscard]] virtual ResultSet query() = 0;

    virtual void reset() noexcept = 0;
};

using StatementPtr = std::unique_ptr<IDbStatement>;

// ---------------------------------------------------------------------------
// IDbTransaction
// RAII transaction guard (OCP: commit/rollback policy is implementation detail).
// Destructor rolls back automatically if commit() was never called.
// ---------------------------------------------------------------------------
class IDbTransaction {
public:
    virtual ~IDbTransaction() = default;

    virtual void commit()   = 0;
    virtual void rollback() noexcept = 0;

    [[nodiscard]] virtual bool isActive() const noexcept = 0;
};

using TransactionPtr = std::unique_ptr<IDbTransaction>;

// ---------------------------------------------------------------------------
// IDbConnection
// Abstracts a single database connection (ISP: no pool concerns here).
// ---------------------------------------------------------------------------
class IDbConnection {
public:
    virtual ~IDbConnection() = default;

    virtual void open() = 0;

    virtual void close() noexcept = 0;

    [[nodiscard]] virtual bool isOpen() const noexcept = 0;

    [[nodiscard]] virtual StatementPtr prepare(std::string_view sql) = 0;

    virtual void execute(std::string_view sql) = 0;

    [[nodiscard]] virtual TransactionPtr beginTransaction() = 0;

    [[nodiscard]] virtual std::string_view connectionId() const noexcept = 0;
};

using ConnectionPtr = std::unique_ptr<IDbConnection>;

// ---------------------------------------------------------------------------
// DbConnectionConfig
// Plain value type; copy-constructible for pool bookkeeping.
// ---------------------------------------------------------------------------
struct DbConnectionConfig {
    std::string host{"localhost"};
    std::uint16_t port{5432};
    std::string database;
    std::string username;
    std::string password;
    std::size_t maxPoolSize{10};
    std::chrono::seconds connectTimeout{5};
    std::chrono::seconds idleTimeout{60};
    bool tlsEnabled{false};
};

// ---------------------------------------------------------------------------
// IDbConnectionPool
// Manages a bounded pool of reusable connections (SRP / DIP).
// Callers depend only on this interface; concrete pools are injected.
// ---------------------------------------------------------------------------
class IDbConnectionPool {
public:
    virtual ~IDbConnectionPool() = default;

    [[nodiscard]] virtual ConnectionPtr acquire() = 0;

    virtual void release(ConnectionPtr connection) noexcept = 0;

    [[nodiscard]] virtual std::size_t idleCount()  const noexcept = 0;

    [[nodiscard]] virtual std::size_t activeCount() const noexcept = 0;

    [[nodiscard]] virtual const DbConnectionConfig& config() const noexcept = 0;
};

// ---------------------------------------------------------------------------
// PooledConnection
// RAII wrapper returned to callers; automatically releases on destruction.
// Movable, non-copyable. Satisfies RAII idiom without manual release calls.
// ---------------------------------------------------------------------------
class PooledConnection final {
public:
    PooledConnection() = delete;
    PooledConnection(const PooledConnection&) = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;

    PooledConnection(IDbConnectionPool& pool, ConnectionPtr connection) noexcept;
    PooledConnection(PooledConnection&& other) noexcept;
    PooledConnection& operator=(PooledConnection&& other) noexcept;
    ~PooledConnection();

    [[nodiscard]] IDbConnection& get() noexcept;
    [[nodiscard]] const IDbConnection& get() const noexcept;

    IDbConnection* operator->()  noexcept       { return connection_.get(); }
    const IDbConnection* operator->() const noexcept { return connection_.get(); }

private:
    IDbConnectionPool* pool_{nullptr};
    ConnectionPtr      connection_;
};

// ---------------------------------------------------------------------------
// DbConnectionPool  (concrete, LSP-compliant implementation of IDbConnectionPool)
// Thread-safe fixed-size pool backed by a condition variable.
// ---------------------------------------------------------------------------
class DbConnectionPool final : public IDbConnectionPool {
public:
    explicit DbConnectionPool(DbConnectionConfig config,
                              std::function<ConnectionPtr()> factory);
    ~DbConnectionPool() override;

    // IDbConnectionPool
    [[nodiscard]] ConnectionPtr acquire()                   override;
    void                        release(ConnectionPtr conn) noexcept override;
    [[nodiscard]] std::size_t   idleCount()   const noexcept override;
    [[nodiscard]] std::size_t   activeCount() const noexcept override;
    [[nodiscard]] const DbConnectionConfig& config() const noexcept override;

    void initialise(std::size_t minConnections = 1);

    void shutdown() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace middleware::db