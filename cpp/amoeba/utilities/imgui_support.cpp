#include "utilities/imgui_support.hpp"
#include "utilities/strings.hpp"

#include <windows.h>

// ImGui support functions.
//
// polymeric 2026

// MsvcReleaseModeXString/MsvcReleaseModeXWString InputText support functions are based off imgui_stdlib.cpp

// MsvcReleaseModeXString
struct InputTextCallback_MsvcReleaseModeXString_UserData {
    MsvcReleaseModeXString *Str;
    ImGuiInputTextCallback ChainCallback;
    void*                  ChainCallbackUserData;
};

static int MsvcReleaseModeXString_InputTextCallback(ImGuiInputTextCallbackData* data) {
    InputTextCallback_MsvcReleaseModeXString_UserData* user_data = (InputTextCallback_MsvcReleaseModeXString_UserData*)data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        // Resize string callback
        // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
        MsvcReleaseModeXString *str = user_data->Str;
        IM_ASSERT(data->Buf == str->begin());
        str->resize(data->BufTextLen, '\0'); // "Exclude zero-terminator storage. In C land: == strlen(some_text), in C++ land: string.length()"
        data->Buf = (char*)str->begin();
    } else if (user_data->ChainCallback) {
        // Forward to user callback, if any
        data->UserData = user_data->ChainCallbackUserData;
        return user_data->ChainCallback(data);
    }
    return 0;
}

bool ImGuiInputText(const char* label, MsvcReleaseModeXString* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data) {
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_MsvcReleaseModeXString_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    // "Include zero-terminator storage. In C land: == ARRAYSIZE(my_char_array), in C++ land: string.capacity()+1"
    return ImGui::InputText(label, (char*)str->begin(), str->_Myres + 1, flags, MsvcReleaseModeXString_InputTextCallback, &cb_user_data);
}

// MsvcReleaseModeXWString
struct InputTextCallback_MsvcReleaseModeXWString_UserData {
    std::string            *Str;
    ImGuiInputTextCallback ChainCallback;
    void*                  ChainCallbackUserData;
};

static int MsvcReleaseModeXWString_InputTextCallback(ImGuiInputTextCallbackData* data) {
    InputTextCallback_MsvcReleaseModeXWString_UserData* user_data = (InputTextCallback_MsvcReleaseModeXWString_UserData*)data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        // Resize string callback
        // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
        std::string *str = user_data->Str;
        IM_ASSERT(data->Buf == str->c_str());
        str->resize(data->BufTextLen, '\0'); // "Exclude zero-terminator storage. In C land: == strlen(some_text), in C++ land: string.length()"
        data->Buf = (char*)str->c_str();
    } else if (user_data->ChainCallback) {
        // Forward to user callback, if any
        data->UserData = user_data->ChainCallbackUserData;
        return user_data->ChainCallback(data);
    }
    return 0;
}

bool ImGuiInputText(const char* label, MsvcReleaseModeXWString* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data) {
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    std::string multibyte = convert_utf16_wstring_to_utf8_string(str->as_native_wstring_view());

    InputTextCallback_MsvcReleaseModeXWString_UserData cb_user_data;
    cb_user_data.Str = &multibyte;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    // "Include zero-terminator storage. In C land: == ARRAYSIZE(my_char_array), in C++ land: string.capacity()+1"
    // C++11 note: "Both string::data and string::c_str are synonyms and return the same value."
    bool was_modified = ImGui::InputText(label, (char*)multibyte.c_str(), multibyte.capacity() + 1, flags, MsvcReleaseModeXWString_InputTextCallback, &cb_user_data);

    if(was_modified) {
        int wchar_count = MultiByteToWideChar(CP_UTF8, 0, multibyte.data(), static_cast<int>(multibyte.length()), NULL, 0);
        str->resize(wchar_count, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, multibyte.data(), static_cast<int>(multibyte.length()), str->begin(), wchar_count);
    }

    return was_modified;
}
