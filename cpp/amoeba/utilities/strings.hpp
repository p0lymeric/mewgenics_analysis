#pragma once

#include <string>
#include <filesystem>

#include <windows.h>

// String manipulation utilities
//
// polymeric 2026

inline std::wstring convert_utf8_string_to_utf16_wstring(std::string_view multibyte) {
    int wchar_count = MultiByteToWideChar(CP_UTF8, 0, multibyte.data(), static_cast<int>(multibyte.length()), NULL, 0);
    std::wstring wide(wchar_count, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, multibyte.data(), static_cast<int>(multibyte.length()), wide.data(), wchar_count);
    return wide;
}

inline std::string convert_utf16_wstring_to_utf8_string(std::wstring_view wide) {
    int char_count = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.length()), NULL, 0, NULL, NULL);
    std::string multibyte(char_count, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.length()), multibyte.data(), char_count, NULL, NULL);
    return multibyte;
}

inline std::string convert_filesystem_path_to_utf8_string(std::filesystem::path path) {
    // path.u8string() will perform similar convert-via-copy steps
    // and yield a type that won't play nicely with most code
    return convert_utf16_wstring_to_utf8_string(path.wstring());
}
