#pragma once

#include <string>
#include <format>
#include <chrono>
#include <windows.h>

#ifdef ENABLE_DEBUG_CONSOLE
template<class... Args>
void printfmt(std::format_string<Args...> fmt, Args&&... args) {
    std::string multibyte = std::format(fmt, std::forward<Args>(args)...);
    int wchar_count = MultiByteToWideChar(CP_UTF8, 0, multibyte.data(), static_cast<int>(multibyte.length()), NULL, 0);
    std::wstring wide(wchar_count, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, multibyte.data(), static_cast<int>(multibyte.length()), wide.data(), wchar_count);
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), wide.data(), static_cast<DWORD>(wide.length()), NULL, NULL);
}
#define DPRINTFMTPRE(...) printfmt("amoeba - {:%F %T} - ", std::chrono::system_clock::now()); printfmt(__VA_ARGS__)
#define DPRINTFMT(...) printfmt(__VA_ARGS__)
#else
template<class... Args>
void printfmt(std::format_string<Args...> fmt, Args&&... args) {
}
#define DPRINTFMTPRE(...)
#define DPRINTFMT(...)
#endif
