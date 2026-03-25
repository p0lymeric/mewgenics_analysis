#include "utilities/sqlite3_conn_wrapper.hpp"

#include "utilities/debug_console.hpp"
#include "utilities/strings.hpp"

// Another sqlite3 C++ wrapper
//
// polymeric 2026

// std::variant visitor helper
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

Sqlite3ConnWrapper::Sqlite3ConnWrapper() :
    enable_connection_keepalive(false)
{
}

Sqlite3ConnWrapper::~Sqlite3ConnWrapper() {
    // TODO this can fail
    this->close();
}

bool Sqlite3ConnWrapper::open(std::filesystem::path db_path) {
    if(!this->close()) {
        return false;
    }
    this->path = db_path;
    if(sqlite3_open_v2(convert_filesystem_path_to_utf8_string(db_path).c_str(), &this->conn, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        // sqlite3 can return a valid conn even if an error were to occur
        // it will write NULL into ppDb iff it didn't return a valid conn
        // try closing the handle for the unsuccessful attempt
        if(sqlite3_close(this->conn) == SQLITE_OK) {
            D::error("Unable to open sqlite3 database.");
            // zero the pointer and path only if closing was successful
            this->conn = NULL;
            this->path.clear();
        } else {
            D::error("Unable to open sqlite3 database. The failed connection's handle also refuses to close.");
            // don't let go of the pointer if we couldn't close it
            // probably an untraversable case
            this->conn_refused_to_close = true;
        }
        return false;
    }
    return true;
}

bool Sqlite3ConnWrapper::close() {
    // it's OK to call close with a null pointer
    if(sqlite3_close(this->conn) != SQLITE_OK) {
        D::error("Unable to close sqlite3 connection handle.");
        this->conn_refused_to_close = true;
        return false;
    }
    this->conn = NULL;
    this->path.clear();
    this->conn_refused_to_close = false;
    return true;
}

bool Sqlite3ConnWrapper::exec_path(std::filesystem::path db_path, std::string query, std::list<std::pair<std::string, Sqlite3BindingDTs>> bindings, std::function<bool(sqlite3_stmt *stmt)> row_callback) {
    // reopen db if our caller references a different file than what we have opened
    if(conn == NULL || !std::filesystem::equivalent(db_path, this->path)) {
        // open is safe to call even with an active connection
        if(!this->open(db_path)) {
            return false;
        }
    }

    bool success = this->exec(query, bindings, row_callback);

    // close the connection if keepalive is disabled
    if(!this->enable_connection_keepalive) {
        // if this were to fail, the caller doesn't need to know as it already has its results
        // next caller or the destructor will need to handle consequences
        this->close();
    }

    return success;
}

// guarantees the stmt is freed
bool Sqlite3ConnWrapper::exec(std::string query, std::list<std::pair<std::string, Sqlite3BindingDTs>> bindings, std::function<bool(sqlite3_stmt *stmt)> row_callback) {
    if(this->conn == NULL) {
        return false;
    }

    sqlite3_stmt *stmt;
    // pzTail can be used to execute multiple statements, currently unused
    if(sqlite3_prepare_v2(this->conn, query.c_str(), static_cast<int>(query.length()) + 1, &stmt, NULL) != SQLITE_OK) {
        // sqlite3_finalize tolerates NULLs
        sqlite3_finalize(stmt);
        return false;
    }

    for(auto kv : bindings) {
        auto key = kv.first;
        auto val = kv.second;
        int parameter_idx = sqlite3_bind_parameter_index(stmt, key.c_str());
        int result;
        std::visit(overloaded {
            [stmt, parameter_idx, &result](std::vector<uint8_t> &vec) { result = sqlite3_bind_blob64(stmt, parameter_idx, vec.data(), vec.size(), SQLITE_STATIC); },
            [stmt, parameter_idx, &result](double &dbl) { result = sqlite3_bind_double(stmt, parameter_idx, dbl); },
            [stmt, parameter_idx, &result](int32_t &i32) { result = sqlite3_bind_int(stmt, parameter_idx, i32); },
            [stmt, parameter_idx, &result](int64_t &i64) { result = sqlite3_bind_int64(stmt, parameter_idx, i64); },
            [stmt, parameter_idx, &result](std::monostate &) { result = sqlite3_bind_null(stmt, parameter_idx); },
            // "If a non-negative fourth parameter is provided [...] then that parameter must be the byte offset where the
            // NUL terminator would occur assuming the string were NUL terminated."
            [stmt, parameter_idx, &result](std::string &str) { result = sqlite3_bind_text64(stmt, parameter_idx, str.data(), str.length() + 1, SQLITE_STATIC, SQLITE_UTF8); },
        }, val);
        if(result != SQLITE_OK) {
            sqlite3_finalize(stmt);
            return false;
        }
    }

    while(true) {
        int result = sqlite3_step(stmt);
        switch(result) {
            case SQLITE_ROW:
                if(!row_callback(stmt)) {
                    sqlite3_finalize(stmt);
                    return true;
                }
                continue;
            case SQLITE_DONE:
                sqlite3_finalize(stmt);
                return true;
            default:
                sqlite3_finalize(stmt);
                return false;
        }
    }

    // unreachable
    return true;
}
