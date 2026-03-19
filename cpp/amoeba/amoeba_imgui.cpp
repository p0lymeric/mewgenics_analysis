#include "amoeba.hpp"
#include "types/glaiel.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/function_hook.hpp"
#include "utilities/strings.hpp"

#include "SDL3/SDL.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

// Cat therapy? Sign me up!
//
// Injects imgui into Mewgenics to facilitate in-process exploration.
//
// polymeric 2026

void show_log_window() {
    if(ImGui::Begin("Log")) {
        if(ImGui::BeginChild("Scroller", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar)) {
            ImGuiListClipper clipper;
            int log_size = static_cast<int>(D::get().internal_buffer.size());
            clipper.Begin(log_size);
            while(clipper.Step()) {
                for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    auto message = D::get().internal_buffer[log_size - i - 1];
                    // FIXME scrolling is broken. ImGuiListClipper is not a correct solution for handling multiline strings
                    ImGui::TextUnformatted(message.message.data(), message.message.data() + message.message.size());
                }
            }
            clipper.End();
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void show_debug_window() {
    if(ImGui::Begin("Dashboard du jour")) {
        MewDirector *p_md = *reinterpret_cast<MewDirector **>(ADDRESS_glaiel__MewDirector__p_singleton + G.host_exec_base_va);
        ImGui::Text("%s", std::format("p_MewDirector {:p}", static_cast<void *>(p_md)).c_str());
        if(p_md != nullptr) {
            CatDatabase *p_cdb = p_md->cats;
            ImGui::Text("%s", std::format("p_CatDatabase {:p}", static_cast<void *>(p_cdb)).c_str());
            ImGui::Text("%s", std::format("N cats {}", p_cdb->cats._Mysize).c_str());
            ImGui::Text("%s", std::format("N cats to delete {:}", p_cdb->cats_to_delete._Mysize).c_str());
        }
    }
    ImGui::End();
}

void show_feline_shrink_window() {
    if(ImGui::Begin("Feline Therapist")) {
        if(ImGui::BeginChild("Scroller", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar)) {
            MewDirector *p_md = *reinterpret_cast<MewDirector **>(ADDRESS_glaiel__MewDirector__p_singleton + G.host_exec_base_va);
            if(p_md != nullptr) {
                CatDatabase *p_cdb = p_md->cats;
                auto head = p_cdb->cats._Myhead;
                auto current = head->_Next;
                while(current != head) {
                    ImGui::Text("%s", std::format("{} {}", current->_Myval.sql_key, convert_utf16_wstring_to_utf8_string(current->_Myval.cat->name)).c_str());
                    current = current->_Next;
                }
                ImGui::Text("%s", std::format("N cats {}", p_cdb->cats._Mysize).c_str());
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}


MAKE_VHOOK(bool, __cdecl, SDL_GL_SwapWindow,
    SDL_Window *window
) {
    // multi-viewport support code will call SDL_GL_SwapWindow to paint onto other windows
    // if so, don't attempt to inject imgui again
    if(G.ig.swapwindow_hook_nested_call_guard) {
        return SDL_GL_SwapWindow_hook.orig(window);
    }

    if(!G.ig.initialized) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        // multi-viewport works, but is glitchy with fullscreen (dragging to edge of window will cause flickering)
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui_ImplSDL3_InitForOpenGL(window, SDL_GL_GetCurrentContext());
        ImGui_ImplOpenGL3_Init();
        G.ig.initialized = true;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // ImGui::ShowDemoWindow();
    show_log_window();
    show_debug_window();
    show_feline_shrink_window();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        G.ig.swapwindow_hook_nested_call_guard = true;
        ImGui::RenderPlatformWindowsDefault();
        G.ig.swapwindow_hook_nested_call_guard = false;
        SDL_GL_MakeCurrent(window, backup_current_context);
    }

    return SDL_GL_SwapWindow_hook.orig(window);
}

MAKE_VHOOK(bool, __cdecl, SDL_PollEvent,
    SDL_Event *event
) {
    if(G.ig.initialized) {
        // retrieve event, let imgui take a look first
        // if imgui uses the event, fetch the next event and retry
        // return the validity indicator for the first event that imgui did not use
        ImGuiIO& io = ImGui::GetIO();
        while(SDL_PollEvent_hook.orig(event)) {
            // function returns a bool here, "I positively decoded the event"
            // but it does not tell us "I reacted to the event so it should be masked"
            ImGui_ImplSDL3_ProcessEvent(event);

            // instead imgui gives us io.WantCaptureMouse/io.WantCaptureKeyboard
            // we match the list of event types processed by the backend against those indicators
            switch (event->type){
                case SDL_EVENT_MOUSE_MOTION:
                case SDL_EVENT_MOUSE_WHEEL:
                // imgui will internally pair click/button downs/ups and filter accordingly to prevent sticking
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if(io.WantCaptureMouse) {
                        continue;
                    } else {
                        return true;
                    }
                    break;

                case SDL_EVENT_TEXT_INPUT:
                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:
                    if(io.WantCaptureKeyboard) {
                        continue;
                    } else {
                        return true;
                    }
                    break;

                // sampled by backend, but likely unrelated to io.WantCaptureMouse/io.WantCaptureKeyboard
                // case SDL_EVENT_WINDOW_MOUSE_ENTER:
                // case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                // case SDL_EVENT_WINDOW_FOCUS_GAINED:
                // case SDL_EVENT_WINDOW_FOCUS_LOST:
                // case SDL_EVENT_GAMEPAD_ADDED:
                // case SDL_EVENT_GAMEPAD_REMOVED:
                default:
                    return true;
                    break;
            }
        }
        // no events for the host
        return false;
    } else {
        return SDL_PollEvent_hook.orig(event);
    }
}
