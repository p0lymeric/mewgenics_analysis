#pragma once

#include "transaction_logger.hpp"

#include <cstdint>
#include <set>
#include <string>
#include <functional>
#include <format>

// Main program declarations.
//
// polymeric 2026

// INTERNAL DECLARATIONS

enum TlogVsid : uint32_t {
    Meta = 0,
    Info = 1,
    Sql = 2,
    SaveData = 3,
};

struct GlobalContext {
    uint32_t save_scope_counter;
    // std::string current_opened_db_path;
    std::set<std::string> witnessed_db_paths;
    TransactionLogger *tlogger;
};


// HOST STRUCTURE DECLARATIONS

// Host STL objects can be naturally manipulated so long as we roughly match compiler version and build profile
// ...
// unless of course we want to make life harder
// ...
// "for reasons of sanitization", not to be mistaken for "for reasons of sanity"

// MSVC XString (std::string), as laid out when compiled in Release mode
struct MsvcReleaseModeXString {
    union {
        char _Buf[16];
        char *_Ptr;
    } _Bx;
    uint64_t _Mysize;
    uint64_t _Myres;

    // delete the copy constructor to block implicit copying
    MsvcReleaseModeXString(const MsvcReleaseModeXString&) = delete;
    MsvcReleaseModeXString& operator=(const MsvcReleaseModeXString&) = delete;

    const char *begin() const {
        if(this->_Myres < 16) {
            return &this->_Bx._Buf[0];
        } else {
            return this->_Bx._Ptr;
        }
    }

    const char *end() const {
        if(this->_Myres < 16) {
            return &this->_Bx._Buf[this->_Mysize];
        } else {
            return this->_Bx._Ptr + this->_Mysize;
        }
    }

    std::string copy_to_native_string() const {
        return std::string(this->begin(), this->end());
    }

    operator std::string() const {
        return this->copy_to_native_string();
    }

    operator std::string_view() const {
        return std::string_view(this->begin(), this->_Mysize);
    }
};
template<>
struct std::formatter<MsvcReleaseModeXString> : std::formatter<std::string_view> {
    auto format(const MsvcReleaseModeXString &s, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(s, ctx);
    }
};

// STL type aliases to create a distinction between host and native objects
typedef MsvcReleaseModeXString HostStdString;
template<class T>
using HostStdFunction = void;

// treat sqlite objects as opaque for now
typedef void sqlite3_stmt;

// overall size is not correct, we only care about file_path
struct SQLSaveFile {
    void *maybe_sqlite3_hdl;
    HostStdString file_path;
    // ... likely more ...
};

enum SqlDataType : uint32_t {
    Blob = 1,      // BLOB
    // TODO double check if this is std::string or heap-allocated char[]
    Text = 2,      // TEXT SQLITE_UTF8
    // treat these as unknown until we encounter them
    // WText = 3,     // TEXT SQLITE_UTF16LE?
    // Integer32 = 4, // INTEGER s32?
    Integer = 5, // INTEGER s64
    Real = 6,    // REAL double
};
template<>
struct std::formatter<SqlDataType> : std::formatter<std::string> {
    auto format(SqlDataType ty, std::format_context& ctx) const {
        std::string s;
        switch(ty) {
            case Blob:
                s += std::format("Blob");
                break;
            case Text:
                s += std::format("Text");
                break;
            // case WText:
            //     s += std::format("WText");
            //     break;
            // case Integer32:
            //     s += std::format("Integer32");
            //     break;
            case Integer:
                s += std::format("Integer");
                break;
            case Real:
                s += std::format("Real");
                break;
            default:
                s += std::format("Unknown({})", static_cast<uint32_t>(ty));
                break;
        }
        return std::formatter<std::string>::format(s, ctx);
    }
};

struct SqlData {
    SqlDataType type; // + 4B padding
    union {
        const void *as_blob_ptr;
        const char *as_c_str;
        const wchar_t *as_wc_str;
        int32_t as_int;
        int64_t as_int64;
        double as_double;
    } value;
    int32_t length; // probably dword sized
};
template<>
struct std::formatter<SqlData> : std::formatter<std::string> {
    auto format(const SqlData &data, std::format_context& ctx) const {
        std::string s;
        switch(data.type) {
            case Blob:
                s += std::format("<blob data@{:p}, len={}>", data.value.as_blob_ptr, data.length);
                break;
            case Text:
                s += std::format("\"{}\"", std::string(data.value.as_c_str, data.value.as_c_str + strlen(data.value.as_c_str)));
                break;
            // case WText:
            //     s += std::format("L\"{}\"", data.value.as_wc_str);
            //     break;
            // case Integer32:
            //     s += std::format("{}", data.value.as_int);
            //     break;
            case Integer:
                s += std::format("{}", data.value.as_int64);
                break;
            case Real:
                s += std::format("{:#}", data.value.as_double);
                break;
            default:
                s += std::format("<unknown 0x{:x}, 0x{:x}>", data.value.as_int64, data.length);
                break;
        }
        return std::formatter<std::string>::format(s, ctx);
    }
};

struct SqlParam {
    const char* name;
    SqlData data;
};
template<>
struct std::formatter<SqlParam> : std::formatter<std::string> {
    auto format(const SqlParam &param, std::format_context& ctx) const {
        std::string s;
        // s += std::format("({}, {}, {})", std::string(param.name, param.name + strlen(param.name)), param.type, param.data);
        s += std::format("({}, {})", std::string(param.name, param.name + strlen(param.name)), param.data);
        return std::formatter<std::string>::format(s, ctx);
    }
};

template<typename T, uint32_t S>
struct PodBufferPreallocated {
    uint32_t capacity;
    uint32_t size;
    union {
        // prealloc for stack placement
        T buf[S];
        T *ptr;
    } u;

    T *begin() {
        if(this->capacity <= S) {
            return &this->u.buf[0];
        } else {
            return this->u.ptr;
        }
    }

    T *end() {
        if(this->capacity <= S) {
            return &this->u.buf[this->size];
        } else {
            return this->u.ptr + this->size;
        }
    }
};
