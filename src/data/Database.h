// RAII wrapper over an SQLite connection plus a prepared-statement helper.
// All SQL in the project goes through Statement with bound parameters (no string concatenation
// of user input), and only the data-access layer (SqliteRepositories) uses this header.
#pragma once
#include <sqlite3.h>

#include <functional>
#include <optional>
#include <string>

#include "common/Errors.h"
#include "domain/Types.h"

namespace lms {

// Abstraction used by services so that several repository calls commit or roll back together.
class ITransactionRunner {
public:
    virtual ~ITransactionRunner() = default;
    virtual void run(const std::function<void()>& work) = 0;
};

class Database final : public ITransactionRunner {
public:
    explicit Database(const std::string& path);  // ":memory:" for tests
    ~Database() override;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    sqlite3* handle() const { return db_; }
    void initialiseSchema();                               // idempotent (CREATE ... IF NOT EXISTS)
    void run(const std::function<void()>& work) override;  // nested calls join the outer transaction
    void execute(const std::string& sql);  // parameter-free statements only (schema, BEGIN, ...)

private:
    sqlite3* db_ = nullptr;
    int depth_ = 0;
};

class Statement {
public:
    Statement(const Database& db, const std::string& sql);
    ~Statement();
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    Statement& bind(int idx, const std::string& v);
    Statement& bind(int idx, std::int64_t v);
    Statement& bind(int idx, double v);
    Statement& bindNull(int idx);
    Statement& bind(int idx, const std::optional<double>& v);
    Statement& bind(int idx, const std::optional<Id>& v);

    bool step();  // true if a row is available; throws DatabaseError on failure
    void exec() { step(); }

    std::string text(int col) const;
    std::int64_t integer(int col) const;
    double real(int col) const;
    bool isNull(int col) const;
    std::optional<double> optReal(int col) const;
    std::optional<Id> optInteger(int col) const;

private:
    sqlite3* db_;
    sqlite3_stmt* stmt_ = nullptr;
};

}  // namespace lms
