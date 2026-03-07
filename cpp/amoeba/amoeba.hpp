#pragma once

#include <cstdint>
#include <string>
#include <format>

// STL objects can be manipulated so long as we roughly match compiler version and build profile
typedef std::string HostStdString;

// overall size is not correct, we only care about file_path
struct SQLSaveFile {
    void *maybe_sqlite3_hdl;
    char *file_path;
    // ... likely more ...
};

enum SqlParamType : uint32_t {
    Blob = 1,      // BLOB
    Text = 2,      // TEXT SQLITE_UTF8
    // treat these as unknown until we encounter them
    // WText = 3,     // TEXT SQLITE_UTF16LE?
    // Integer32 = 4, // INTEGER s32?
    Integer = 5, // INTEGER s64
    Real = 6,    // REAL double
};
template<>
struct std::formatter<SqlParamType> : std::formatter<std::string> {
    auto format(SqlParamType ty, std::format_context& ctx) const {
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

struct SqlParam {
    const char* name;
    SqlParamType type; // + 4B padding
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
struct std::formatter<SqlParam> : std::formatter<std::string> {
    auto format(SqlParam param, std::format_context& ctx) const {
        std::string s;
        // s += std::format("({}, {}, ", std::string(param.name, param.name + strlen(param.name)), param.type);
        s += std::format("({}, ", std::string(param.name, param.name + strlen(param.name)));
        switch(param.type) {
            case Blob:
                s += std::format("<blob data@{:p}, len={}>", param.value.as_blob_ptr, param.length);
                break;
            case Text:
                s += std::format("\"{}\"", std::string(param.value.as_c_str, param.value.as_c_str + strlen(param.value.as_c_str)));
                break;
            // case WText:
            //     s += std::format("L\"{}\"", param.value.as_wc_str);
            //     break;
            // case Integer32:
            //     s += std::format("{}", param.value.as_int);
            //     break;
            case Integer:
                s += std::format("{}", param.value.as_int64);
                break;
            case Real:
                s += std::format("{:#}", param.value.as_double);
                break;
            default:
                s += std::format("<unknown 0x{:x}, 0x{:x}>", param.value.as_int64, param.length);
                break;
        }
        s += std::format(")");
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
