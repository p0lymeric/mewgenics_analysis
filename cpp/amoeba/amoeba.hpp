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

// MSVC XString (std::string), laid out as compiled in Release mode
// https://github.com/microsoft/STL/blob/26b7ca58ee3151c81736088e554e5797cafca641/stl/inc/xstring
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

// MSVC Func (std::function)
// Based off _Func_impl_no_alloc & friends
// https://github.com/microsoft/STL/blob/26b7ca58ee3151c81736088e554e5797cafca641/stl/inc/functional
template <class _Callable, class _Rx, class... _Types>
struct MsvcFuncNoAlloc_vtable;

template <class _Callable, class _Rx, class... _Types>
struct MsvcFuncNoAlloc;
template <class _Callable, class _Rx, class... _Types>
struct MsvcFuncNoAlloc<_Callable, _Rx(_Types...)> {
    using VTable = MsvcFuncNoAlloc_vtable<_Callable, _Rx(_Types...)>;

    VTable *vtable;
    union {
        _Callable _Callee;
        void *_Ptrs[7];
    } _Mystorage;

    // This struct definition by itself does not depend on the contents of _Callee.
    // Users of this struct may describe capture layout in _Callable or use an opaque arbitrary sized dummy type.
    // However, we do require _Mystorage to be exactly 7 qwords in length.
    // Since we don't model heap indirection (_Is_large==true), we place this assertion on sizeof(_Callable).
    static_assert(sizeof(_Callable) <= 6*64, "_Callable can't fit into the inline buffer. _Is_large==true is not supported.");

    MsvcFuncNoAlloc(VTable *vtable, _Callable callee) :
        vtable(vtable), _Mystorage(callee)
    {}

    // delete the copy constructor to block implicit copying
    MsvcFuncNoAlloc(const MsvcFuncNoAlloc&) = delete;
    MsvcFuncNoAlloc& operator=(const MsvcFuncNoAlloc&) = delete;
};
template <class _Callable, class _Rx, class... _Types>
struct std::formatter<MsvcFuncNoAlloc<_Callable, _Rx(_Types...)>> : std::formatter<std::string_view> {
    auto format(const MsvcFuncNoAlloc<_Callable, _Rx(_Types...)> &f, std::format_context& ctx) const {
        std::string s = std::format("(vtable: {:p}, fp: {:p}, _getimpl: {:p})",
            reinterpret_cast<void *>(f.vtable),
            f._Mystorage._Ptrs[0],
            f._Mystorage._Ptrs[6]
        );
        return std::formatter<std::string_view>::format(s, ctx);
    }
};

typedef void MsvcTypeInfo;

// vtable for superclass _Func_base
template <class _Callable, class _Rx, class... _Types>
struct MsvcFuncNoAlloc_vtable<_Callable, _Rx(_Types...)> {
    using Impl = MsvcFuncNoAlloc<_Callable, _Rx(_Types...)>;
    Impl* (* _Copy)(Impl const* thiss, void* _Where);
    Impl* (* _Move)(Impl* thiss, void* _Where);
    _Rx (* _Do_call)(Impl* thiss, _Types*... _Args);
    const MsvcTypeInfo* (* _Target_type)(Impl const* thiss);
    void (* _Delete_this)(Impl* thiss, bool _Dealloc);
    const void* (* _Get)(Impl const* thiss);
};

// Wrapper implementation
// This wrapper implements a MsvcFuncNoAlloc that mirrors the call interface and
// contains a pointer to another MsvcFuncNoAlloc as a capture member.
// A function pointer is also stored in the capture environment whose code is executed in place
// of the wrapped MsvcFuncNoAlloc's lambda.

// i.e. a std::function "callback_wrapped" capturing externally provided std::function "callback" and function pointer "func"
// _Rx func(std::function<_Rx(_Types...)> capture, _Types...) { /**/ }
// std::function<_Rx(_Types...)> callback = /* externally provided */;
// std::function<_Rx(_Types...)> callback_wrapped = [&func, callback](_Args...) { (*func)(callback, _Args...); };
// /* callback_wrapped is used in place of callback */

// The _Callable layout for the wrapper
// Looks like capture of a function pointer and a pointer to another std::function
template <class _Callable, class _Rx, class... _Types>
struct CallableWrapped;
template <class _Callable, class _Rx, class... _Types>
struct CallableWrapped<_Callable, _Rx(_Types...)> {
    using Wrapped = MsvcFuncNoAlloc<_Callable, _Rx(_Types...)>;
    using Wrapper = MsvcFuncNoAlloc<CallableWrapped, _Rx(_Types...)>;
    _Rx (* fp)(Wrapper* thiss, _Types*... _Args);
    Wrapped *wrapped;
};

// CallableWrapped is small and constant sized (1 qword for fp + 1 qword for wrapped)
template <class _Callable, class _Rx, class... _Types>
struct MsvcFuncNoAllocWrapper;
template <class _Callable, class _Rx, class... _Types>
struct MsvcFuncNoAllocWrapper<_Callable, _Rx(_Types...)> :
    MsvcFuncNoAlloc<CallableWrapped<_Callable, _Rx(_Types...)>, _Rx(_Types...)>
{
    // these template expansions are pretty horrifying
    using WrapperCallable = CallableWrapped<_Callable, _Rx(_Types...)>;
    using Wrapper = MsvcFuncNoAlloc<WrapperCallable, _Rx(_Types...)>;
    using WrapperVTable = MsvcFuncNoAlloc_vtable<WrapperCallable, _Rx(_Types...)>;
    using Wrapped = MsvcFuncNoAlloc<_Callable, _Rx(_Types...)>;

    static Wrapper* _Copy(Wrapper const* thiss, void* _Where) {
        Wrapper *dest = reinterpret_cast<MsvcFuncNoAllocWrapper *>(_Where);
        dest->vtable = thiss->vtable;
        // _Callee is just 2 pointers, copying is trivial
        dest->_Mystorage._Callee = thiss->_Mystorage._Callee;
        // TODO Need to implement reference counting if this wrapper is ever copied,
        // in order to know when to invoke _Delete_this on the wrapped function
        // We block MsvcFuncNoAllocWrapper from being copied while handled in our program,
        // but it can be copied in the host program
        __debugbreak();
        return dest;
    }
    static Wrapper* _Move(Wrapper* thiss, void* _Where) {
        // oddly enough, _Move degenerates to a constant zero return
        // not sure why, but information comes from decompilation
        // TODO If a wrapped function is ever moved, worth taking a look
        (void)thiss;
        (void)_Where;
        __debugbreak();
        return nullptr;
    }
    static _Rx _Do_call(Wrapper* thiss, _Types*... _Args) {
        // redirect control to a user-provided function pointer stored in the capture body
        return thiss->_Mystorage._Callee.fp(thiss , _Args...);
    }
    static const MsvcTypeInfo* _Target_type(Wrapper const* thiss) {
        // redirect this call to wrapped instance
        return thiss->_Mystorage._Callee.wrapped->vtable->_Target_type(thiss->_Mystorage._Callee.wrapped);
    }
    static void _Delete_this(Wrapper* thiss, bool _Dealloc) {
        // since we are always small, _Dealloc will never be true (outer destructor checks _Ptrs[6] == this)
        // we never would've been allocated in a fashion that requires us to perform any deletes
        (void)thiss;
        (void)_Dealloc;
        // forward destruction to the wrapped instance, which we consumed and need to terminate;
        // this is the destructor implementation that our owner is currently executing against us
        // if this wrapper is ever copied, probably need to use a refcount to ensure this is only called once
        auto wrapped = thiss->_Mystorage._Callee.wrapped;
        if(wrapped->_Mystorage._Ptrs[6] != nullptr) {
            bool is_large = wrapped->_Mystorage._Ptrs[6] != wrapped;
            wrapped->vtable->_Delete_this(wrapped, is_large);
            wrapped->_Mystorage._Ptrs[6] = nullptr;
        }
    }
    static const void* _Get(Wrapper const* thiss) {
        // redirect this call to wrapped instance
        return thiss->_Mystorage._Callee.wrapped->vtable->_Get(thiss->_Mystorage._Callee.wrapped);
    }

    static _Rx invoke_orig(Wrapper* thiss, _Types*... _Args) {
        return thiss->_Mystorage._Callee.wrapped->vtable->_Do_call(thiss->_Mystorage._Callee.wrapped, _Args...);
    }

    inline static WrapperVTable my_vtable = {
        _Copy, _Move, _Do_call, _Target_type, _Delete_this, _Get
    };

    MsvcFuncNoAllocWrapper(Wrapped *wrapped, _Rx (*fp)(Wrapper* thiss, _Types*... args)) :
        Wrapper(&my_vtable, WrapperCallable { fp, wrapped })
    {
        // point to start of this object to mark it as small-storage
        this->_Mystorage._Ptrs[6] = reinterpret_cast<void *>(this);
    }

    Wrapped *as_target_type() {
        return reinterpret_cast<Wrapped *>(this);
    }
};

// STL type aliases to create a distinction between host and native objects
typedef MsvcReleaseModeXString HostStdString;
template<class Capture, class Signature>
using HostStdFunctionNoAlloc = MsvcFuncNoAlloc<Capture, Signature>;

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
std::string SqlData::untrusted_format() {
    // for debug printing, to allow "maybe pointers" to SqlData to be scrutinized without punishment
    SqlData buf;
    if(ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(this), &buf, sizeof(buf), NULL)) {
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
