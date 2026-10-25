#include "data/Database.h"

#include "data/Schema.h"

namespace lms {
namespace {

[[noreturn]] void fail(sqlite3* db, const std::string& what) {
    throw DatabaseError(what + ": " + (db ? sqlite3_errmsg(db) : "no connection"));
}

}  // namespace

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        const std::string msg = db_ ? sqlite3_errmsg(db_) : "out of memory";
        sqlite3_close(db_);
        db_ = nullptr;
        throw DatabaseError("Cannot open database '" + path + "': " + msg);
    }
    sqlite3_busy_timeout(db_, 5000);
    execute("PRAGMA foreign_keys = ON;");  // required by the Phase 3 schema notes
}

Database::~Database() {
    if (db_) sqlite3_close(db_);
}

void Database::execute(const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        const std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw DatabaseError("SQL execution failed: " + msg);
    }
}

void Database::initialiseSchema() {
    run([this] { execute(kSchemaSql); });
}

void Database::run(const std::function<void()>& work) {
    if (depth_ > 0) {  // already inside a transaction: join it
        ++depth_;
        try {
            work();
        } catch (...) {
            --depth_;
            throw;
        }
        --depth_;
        return;
    }
    execute("BEGIN IMMEDIATE;");
    depth_ = 1;
    try {
        work();
    } catch (...) {
        depth_ = 0;
        try {
            execute("ROLLBACK;");
        } catch (...) {
        }
        throw;
    }
    depth_ = 0;
    execute("COMMIT;");
}

Statement::Statement(const Database& db, const std::string& sql) : db_(db.handle()) {
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK)
        fail(db_, "Failed to prepare statement");
}

Statement::~Statement() {
    sqlite3_finalize(stmt_);
}

Statement& Statement::bind(int idx, const std::string& v) {
    if (sqlite3_bind_text(stmt_, idx, v.c_str(), static_cast<int>(v.size()), SQLITE_TRANSIENT) != SQLITE_OK)
        fail(db_, "bind text");
    return *this;
}
Statement& Statement::bind(int idx, std::int64_t v) {
    if (sqlite3_bind_int64(stmt_, idx, v) != SQLITE_OK) fail(db_, "bind integer");
    return *this;
}
Statement& Statement::bind(int idx, double v) {
    if (sqlite3_bind_double(stmt_, idx, v) != SQLITE_OK) fail(db_, "bind real");
    return *this;
}
Statement& Statement::bindNull(int idx) {
    if (sqlite3_bind_null(stmt_, idx) != SQLITE_OK) fail(db_, "bind null");
    return *this;
}
Statement& Statement::bind(int idx, const std::optional<double>& v) {
    return v ? bind(idx, *v) : bindNull(idx);
}
Statement& Statement::bind(int idx, const std::optional<Id>& v) {
    return v ? bind(idx, static_cast<std::int64_t>(*v)) : bindNull(idx);
}

bool Statement::step() {
    const int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) return true;
    if (rc == SQLITE_DONE) return false;
    fail(db_, "Statement failed");
}

std::string Statement::text(int col) const {
    const unsigned char* t = sqlite3_column_text(stmt_, col);
    return t ? reinterpret_cast<const char*>(t) : std::string();
}
std::int64_t Statement::integer(int col) const {
    return sqlite3_column_int64(stmt_, col);
}
double Statement::real(int col) const {
    return sqlite3_column_double(stmt_, col);
}
bool Statement::isNull(int col) const {
    return sqlite3_column_type(stmt_, col) == SQLITE_NULL;
}
std::optional<double> Statement::optReal(int col) const {
    return isNull(col) ? std::nullopt : std::optional<double>(real(col));
}
std::optional<Id> Statement::optInteger(int col) const {
    return isNull(col) ? std::nullopt : std::optional<Id>(integer(col));
}

}  // namespace lms
