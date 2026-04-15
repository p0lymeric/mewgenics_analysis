#pragma once

#include "types/msvc.hpp"

#include <utility>

#include "imgui.h"

// ImGui support functions.
//
// polymeric 2026

template<class... Args>
void ImguiTextStdFmt(std::format_string<Args...> fmt, Args&&... args) {
    std::string s = std::format(fmt, std::forward<Args>(args)...);
    ImGui::TextUnformatted(s.data(), s.data() + s.size());
}

bool ImGuiInputText(const char* label, MsvcReleaseModeXString* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data);
bool ImGuiInputText(const char* label, MsvcReleaseModeXWString* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data);
