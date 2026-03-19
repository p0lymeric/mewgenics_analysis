#pragma once

#include <cstdint>
#include <string>
#include <format>

// Layout descriptions of C++ standard library class implementations shipped with MSVC 2022.
//
// These descriptions allow host process objects to be manipulated without dependence
// on C++ stdlib implementation or C++ ABI used by the hook's native environment.
// However, C ABI must match, for the case of invoking vtable function pointers.
// ...
// "for reasons of sanitization", not to be mistaken for "for reasons of sanity"
//
// polymeric 2026

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

struct MsvcReleaseModeXWString {
    union {
        wchar_t _Buf[8];
        wchar_t *_Ptr;
    } _Bx;
    uint64_t _Mysize;
    uint64_t _Myres;

    // delete the copy constructor to block implicit copying
    MsvcReleaseModeXWString(const MsvcReleaseModeXWString&) = delete;
    MsvcReleaseModeXWString& operator=(const MsvcReleaseModeXWString&) = delete;

    const wchar_t *begin() const {
        if(this->_Myres < 8) {
            return &this->_Bx._Buf[0];
        } else {
            return this->_Bx._Ptr;
        }
    }

    const wchar_t *end() const {
        if(this->_Myres < 8) {
            return &this->_Bx._Buf[this->_Mysize];
        } else {
            return this->_Bx._Ptr + this->_Mysize;
        }
    }

    std::wstring copy_to_native_wstring() const {
        return std::wstring(this->begin(), this->end());
    }

    operator std::wstring() const {
        return this->copy_to_native_wstring();
    }

    operator std::wstring_view() const {
        return std::wstring_view(this->begin(), this->_Mysize);
    }
};
template<>
struct std::formatter<MsvcReleaseModeXWString> : std::formatter<std::wstring_view, wchar_t> {
    auto format(const MsvcReleaseModeXWString &s, std::wformat_context& ctx) const {
        return std::formatter<std::wstring_view, wchar_t>::format(s, ctx);
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

// MSVC List (std::list), laid out as compiled in Release mode
// https://github.com/microsoft/STL/blob/2626cf1ee9d26be701b9bdbd2fb30db240456456/stl/inc/list
template<class _Value_type>
struct MsvcReleaseModeListNode {
    MsvcReleaseModeListNode *_Next; // successor node, or first element if head
    MsvcReleaseModeListNode *_Prev; // predecessor node, or last element if head
    _Value_type _Myval; // the stored value, unused if head
};

template<class _Value_type>
struct MsvcReleaseModeList {
    MsvcReleaseModeListNode<_Value_type> *_Myhead; // pointer to head node
    uint64_t _Mysize; // number of elements
};
