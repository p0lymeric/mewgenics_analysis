#pragma once

#include <vector>
#include <utility>
#include <variant>
#include <functional>
#include <filesystem>

#include "sqlite3.h"

// Another sqlite3 C++ wrapper
//
// This wrapper is not intrinsically tied to a particular database.
// Instead, it manages an internal connection object that can optionally keep the
// connection from its previous query alive for the next query.
// Users pass the path of the database they wish to
// access as part of their query.
//
// polymeric 2026

class Sqlite3ConnWrapper {
public:
    // if true, keep connection opened after each transaction
    // if false, eagerly close the connection after each transaction
    bool enable_connection_keepalive = false;

    using Sqlite3BindingDTs = std::variant<std::vector<uint8_t>, double, int32_t, int64_t, std::monostate, std::string>;

    Sqlite3ConnWrapper();
    ~Sqlite3ConnWrapper();

    bool open(std::filesystem::path db_path);
    bool close();

    bool exec_path(std::filesystem::path db_path, std::string query, std::list<std::pair<std::string, Sqlite3BindingDTs>> bindings, std::function<bool(sqlite3_stmt *stmt)> row_callback);

private:
    // the sqlite3 handle. if conn is NULL, there is no active connection.
    sqlite3 *conn;
    // the database path associated with conn. if conn is NULL, this is empty.
    std::filesystem::path path;
    // set if a previous close operation failed at closing conn
    // unset when conn is successfully closed
    bool conn_refused_to_close;

    bool exec(std::string query, std::list<std::pair<std::string, Sqlite3BindingDTs>> bindings, std::function<bool(sqlite3_stmt *stmt)> row_callback);
};
