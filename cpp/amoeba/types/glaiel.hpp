#pragma once
#include "utilities/memory.hpp"

#include "msvc.hpp"
#include "sqlite3.hpp"

#include <cstdint>
#include <string>
#include <format>

// Reconstructions of Mewgenics structures.
//
// polymeric 2026

// overall size is not correct, we only care about file_path
struct SQLSaveFile {
    sqlite3 *maybe_sqlite3_hdl;
    MsvcReleaseModeXString file_path;
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

    std::string untrusted_format();
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
inline std::string SqlData::untrusted_format() {
    // for debug printing, to allow "maybe pointers" to SqlData to be scrutinized without punishment
    SqlData buf;
    if(jf_read(reinterpret_cast<void *>(this), &buf)) {
        switch(buf.type) {
            case Text:
                return std::format("{} <string data@{:p}, unused={}>", buf.type, buf.value.as_blob_ptr, buf.length);
                break;
            default:
                return std::format("{} {}", buf.type, buf);
                break;
        }
    } else {
        return "<illegal pointer dereference>";
    }
}

struct SqlParam {
    const char* name;
    SqlData data;
};
template<>
struct std::formatter<SqlParam> : std::formatter<std::string> {
    auto format(const SqlParam &param, std::format_context& ctx) const {
        std::string s;
        s += std::format("({}, {}, {})", std::string(param.name, param.name + strlen(param.name)), param.data.type, param.data);
        // s += std::format("({}, {})", std::string(param.name, param.name + strlen(param.name)), param.data);
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

// TODO there are multiple variants of this std::function capture layout
struct glaiel__SQLSaveFile__SQL_CallableLayout1 {
    int32_t *sqlite3_datatype;
    SqlData *result;
};

struct CatData {
    uint64_t random_u64;
    int64_t unknown_17;
    char* field_10;
    struct MsvcReleaseModeXWString name;
    struct MsvcReleaseModeXString nametag_decoration;
    int32_t sex;
    int32_t sex_dup;
    // ...
};

// possibly a std::pair but we'll represent it as a struct
struct SqlKeyCatDataPair {
    int64_t sql_key;
    CatData *cat;
};

struct CatDatabase {
    void *vtable;
    char _8[0x30];
    char _38[8];
    char _40[0x40];
    char _80[0x40];
    char _c0[0x30];
    MsvcReleaseModeList<SqlKeyCatDataPair> cats;
    char _100[0x28];
    char _128[8];
    MsvcReleaseModeList<int64_t> cats_to_delete;
    // likely more stuff...
};

struct MewDirector {
    char _0[0x28];
    char _28[0x10];
    char sql_related[0x8];
    char _40[0x40];
    char _80[0x40];
    char _c0[0x40];
    char _100[0x40];
    char _140[0x40];
    char _180[0x40];
    char _1c0[0x40];
    char _200[0x40];
    char _240[0x40];
    char _280[0x40];
    char _2c0[0x40];
    char _300[0x40];
    char _340[0x40];
    char _380[0x40];
    char _3c0[0x40];
    char _400[0x40];
    char _440[0x40];
    char _480[0x28];
    char _4a8[0x18];
    char _4c0[0x40];
    char _500[0x40];
    char _540[0x40];
    char _580[8];
    char _588[8];
    char _590[8];
    CatDatabase* cats;
    // likely a LOT more stuff...
};
