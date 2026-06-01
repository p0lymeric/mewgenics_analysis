#include "amoeba.hpp"
#include "types/glaiel.hpp"
#include "types/msvc.hpp"
#include "utilities/checksum.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/function_hook.hpp"
#include "utilities/strings.hpp"
#include "utilities/memory.hpp"
#include "utilities/portal.hpp"
#include "utilities/imgui_support.hpp"
#include "ffi/cat_factory.hpp"
#include "ffi/experimental.hpp"

#include "SDL3/SDL.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "misc/cpp/imgui_stdlib.h"
#include "lua.hpp"

#include <cstdint>
#include <cstring>
#include <functional>
#include <list>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <filesystem>

// Cat therapy? Sign me up!
//
// Injects imgui into Mewgenics to facilitate in-process exploration.
//
// polymeric 2026

struct ImguiPrivateState {
    bool initialized = false;
    bool swapwindow_hook_nested_call_guard = false;
    bool request_dll_eject = false;

    bool user_shown_version_mismatch_modal = false;

    bool hide_all = false;
    bool enable_experimental_widgets = false;

    bool show_feline_therapist = false;
    bool show_data_explorer = false;
    bool show_save_explorer = false;
    bool show_debug_console = false;
    bool show_lua_repl = false;
    bool show_imgui_demo = false;

    bool show_tlog_config = false;

    // save explorer
    std::unordered_map<int64_t, ManagedCatData> save_explorer_cats;

    // Lua REPL
    lua_State *repl_state = nullptr;
    Ringbuffer<std::string> repl_lines = Ringbuffer<std::string>(10000);
};

static ImguiPrivateState P;

MAKE_SDPORTAL(DATAOFF_glaiel__MewDirector__p_singleton,
    MewDirector *, get_p_mewdirector_singleton
)

MAKE_SDPORTAL(DATAOFF_maybe_housecat_component_pool,
    VirtualGenerationalArenaAllocator<HouseCat>, get_housecat_component_pool
)

MAKE_STPORTAL(0, TLS0OFF_xoshiro256p_rng_context,
    Xoshiro256pContext, get_xoshiro256p_rng_context
)

MAKE_SFPORTAL(ADDRESS_maybe_Director_create_Scene,
    Scene *, __cdecl, maybe_Director_create_Scene,
    (Director *director, MsvcReleaseModeXString *name),
    (director, name)
)

MAKE_SFPORTAL(ADDRESS_glaiel__Scene__CreateComponent_FishingMinigameScene,
    Component *, __cdecl, glaiel__Scene__CreateComponent_FishingMinigameScene,
    (Scene *thiss, Entity *entity),
    (thiss, entity)
)

MAKE_SFPORTAL(ADDRESS_glaiel__Scene__CreateEntity,
    Entity *, __cdecl, glaiel__Scene__CreateEntity,
    (Scene *thiss),
    (thiss)
)

MAKE_SFPORTAL(ADDRESS_glaiel__Director__DestroyScene,
    Entity *, __cdecl, glaiel__Director__DestroyScene,
    (Director *thiss, MsvcReleaseModeXString *scene_name),
    (thiss, scene_name)
)

void show_about_modal(bool signal) {
    if(signal) {
        ImGui::OpenPopup("About Amoeba");
    }
    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    bool x_button = true;
    if(ImGui::BeginPopupModal("About Amoeba", &x_button, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImguiTextStdFmt("Amoeba, a Mewgenics exploration tool.");
        ImguiTextStdFmt("© polymeric. All rights reserved.");
        ImguiTextStdFmt("Provided under the terms of the MIT license.");
        ImGui::TextLinkOpenURL("https://github.com/p0lymeric/mewgenics_analysis");
        ImGui::Separator();

        ImGui::SetItemDefaultFocus();
        if(ImGui::Button("Close", ImVec2(120, 0)) || !x_button) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void show_eject_confirmation_modal(bool signal) {
    if(signal) {
        ImGui::OpenPopup("Eject?");
    }
    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    bool x_button = true;
    if(ImGui::BeginPopupModal("Eject?", &x_button, ImGuiWindowFlags_AlwaysAutoResize)) {
        if(G.dll_can_self_eject) {
            ImguiTextStdFmt("Really eject Amoeba?");
            ImGui::Separator();

            if(ImGui::Button("Yes", ImVec2(120, 0))) {
                P.request_dll_eject = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if(ImGui::Button("No", ImVec2(120, 0)) || !x_button) {
                ImGui::CloseCurrentPopup();
            }
        } else {
            ImguiTextStdFmt("Amoeba cannot be safely ejected.");
            ImguiTextStdFmt("To allow for ejection, Mewjector must not be present.");
            ImGui::Separator();

            ImGui::SetItemDefaultFocus();
            if(ImGui::Button("Ok", ImVec2(120, 0)) || !x_button) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}

void show_exit_confirmation_modal(bool signal) {
    if(signal) {
        ImGui::OpenPopup("Kill process?");
    }
    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    bool x_button = true;
    if(ImGui::BeginPopupModal("Kill process?", &x_button, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImguiTextStdFmt("Really kill the game's process?");
        ImguiTextStdFmt("(Steven won't know.)");
        ImGui::Separator();

        if(ImGui::Button("Yes", ImVec2(120, 0))) {
            do_process_termination();
            // unreachable
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if(ImGui::Button("No", ImVec2(120, 0)) || !x_button) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void show_symbol_resolution_failure_modal(bool signal) {
    if(signal) {
        ImGui::OpenPopup("Error: Symbol resolution failure");
    }
    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    bool x_button = true;
    if(ImGui::BeginPopupModal("Error: Symbol resolution failure", &x_button, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImguiTextStdFmt("Amoeba was unable to locate some symbols that it expected to hook or call.");
        ImguiTextStdFmt("It has stopped loading at a safe point. Signatures in amoeba.hpp will need to be updated.");
        ImguiTextStdFmt("You may forcefully continue (skipping hooking unlocated symbols), eject Amoeba or exit Mewgenics.");
        ImGui::Separator();
        ImguiTextStdFmt("Expected SHA-256: {}", hash256bit_to_string(EXE_SHA256));
        ImguiTextStdFmt("Actual SHA-256: {}", G.exe_actual_sha256.has_value() ? hash256bit_to_string(G.exe_actual_sha256.value()) : "<unknown>");
        ImGui::Separator();

        ImGui::SetItemDefaultFocus();
        if(ImGui::Button("Force continue", ImVec2(120, 0)) || !x_button) {
            do_forced_hook_install();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Eject Amoeba", ImVec2(120, 0))) {
            P.request_dll_eject = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Exit process", ImVec2(120, 0))) {
            do_process_termination();
            // unreachable
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void show_version_mismatch_modal(bool signal) {
    if(signal) {
        ImGui::OpenPopup("Warning: Version mismatch");
    }
    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    bool x_button = true;
    if(ImGui::BeginPopupModal("Warning: Version mismatch", &x_button, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImguiTextStdFmt("Amoeba detected a different Mewgenics.exe than expected, by hash comparison.");
        ImguiTextStdFmt("To resolve this warning, the expected hash value in amoeba.hpp should be updated.");
        ImguiTextStdFmt("You may continue as if this mismatch was not checked, eject Amoeba, or exit Mewgenics.");
        ImGui::Separator();
        ImguiTextStdFmt("Expected SHA-256: {}", hash256bit_to_string(EXE_SHA256));
        ImguiTextStdFmt("Actual SHA-256: {}", G.exe_actual_sha256.has_value() ? hash256bit_to_string(G.exe_actual_sha256.value()) : "<unknown>");
        ImGui::Separator();

        ImGui::SetItemDefaultFocus();
        if(ImGui::Button("Continue", ImVec2(120, 0)) || !x_button) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Eject Amoeba", ImVec2(120, 0))) {
            P.request_dll_eject = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Exit process", ImVec2(120, 0))) {
            do_process_termination();
            // unreachable
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void show_load_exception_modals_if_needed() {
    if(G.symbol_resolution_failed && !P.user_shown_version_mismatch_modal) {
        show_symbol_resolution_failure_modal(true);
        P.user_shown_version_mismatch_modal = true;
    } else {
        show_symbol_resolution_failure_modal(false);
    }

    if(G.exe_hash_mismatch_detected && !P.user_shown_version_mismatch_modal) {
        show_version_mismatch_modal(true);
        P.user_shown_version_mismatch_modal = true;
    } else {
        show_version_mismatch_modal(false);
    }
}

void show_main_menu_bar() {
    bool signal_eject_confirmation_modal = false;
    bool signal_exit_confirmation_modal = false;
    bool signal_about_modal = false;

    if(ImGui::BeginMainMenuBar()) {
        if(ImGui::BeginMenu("File")) {
            if(ImGui::MenuItem("Eject Amoeba")) { signal_eject_confirmation_modal = true; }
            if(ImGui::MenuItem("Kill process")) { signal_exit_confirmation_modal = true; }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Tools")) {
            if(ImGui::MenuItem("Show Feline Therapist", NULL, &P.show_feline_therapist)) {}
            if(ImGui::MenuItem("Show data explorer", NULL, &P.show_data_explorer)) {}
            if(ImGui::MenuItem("Show save explorer", NULL, &P.show_save_explorer)) {}
            ImGui::Separator();
            if(ImGui::MenuItem("Enable experimental widgets", NULL, &P.enable_experimental_widgets)) {}
            if(ImGui::MenuItem("Show debug console", NULL, &P.show_debug_console)) {}
            if(ImGui::MenuItem("Show Lua REPL", NULL, &P.show_lua_repl)) {}
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Logging")) {
            if(ImGui::MenuItem("Transaction logging", NULL, &P.show_tlog_config)) {}
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Help")) {
            if(ImGui::MenuItem("Show Dear ImGui demo", NULL, &P.show_imgui_demo)) {}
            ImGui::Separator();
            if(ImGui::MenuItem("About Amoeba")) { signal_about_modal = true; }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    show_eject_confirmation_modal(signal_eject_confirmation_modal);
    show_exit_confirmation_modal(signal_exit_confirmation_modal);
    show_about_modal(signal_about_modal);
}

void show_debug_console_window() {
    if(!P.show_debug_console) {
        return;
    }
    ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
    ImGui::SetNextWindowSize(ImVec2(viewport_size.x * 0.4f, viewport_size.y * 0.4f), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Debug console", &P.show_debug_console)) {
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

void show_lua_repl_window() {
    if(!P.show_lua_repl) {
        return;
    }
    ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
    ImGui::SetNextWindowSize(ImVec2(viewport_size.x * 0.4f, viewport_size.y * 0.4f), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Lua REPL", &P.show_lua_repl)) {
        if(ImGui::BeginChild("Scroller", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar)) {
            ImGuiListClipper clipper;
            int lines_count = static_cast<int>(P.repl_lines.size());
            clipper.Begin(lines_count);
            while(clipper.Step()) {
                for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    auto line = P.repl_lines[lines_count - i - 1];
                    ImGui::TextUnformatted(line.data(), line.data() + line.size());
                }
            }
            clipper.End();
        }
        ImGui::EndChild();
        static std::string command;
        if(ImGui::InputText("##command", &command, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if(!command.empty()) {
                P.repl_lines.push(std::format("> {}", command));
                if(luaL_dostring(P.repl_state, command.c_str()) != LUA_OK) {
                    P.repl_lines.push(lua_tostring(P.repl_state, -1));
                    lua_pop(P.repl_state, 1);
                }
                command.clear();
            }
            ImGui::SetKeyboardFocusHere(-1);
        }
    }
    ImGui::End();
}

void edit_cat(CatData &cat) {
    ImguiTextStdFmt("p_CatData: {:p}", reinterpret_cast<void *>(&cat));
    ImGuiInputText("name", &cat.name, 0, nullptr, nullptr);
    ImGuiInputText("nameplate_symbol", &cat.nameplate_symbol, 0, nullptr, nullptr);
    ImGui::InputScalar("entropy", ImGuiDataType_U64, &cat.entropy);
    ImGui::InputScalar("sex", ImGuiDataType_U32, &cat.sex);
    ImGui::InputScalar("sex_dup", ImGuiDataType_U32, &cat.sex_dup);

    ImGui::InputScalar("flags", ImGuiDataType_U64, &cat.flags, nullptr, nullptr, "%016llx");
    std::string flags_list = "";
    for(int i = 63; i >= 0; i--) {
        if((cat.flags >> i) & 1) {
            flags_list += std::format("{} ", i);
        }
    }
    ImguiTextStdFmt("Flags: {}", flags_list);
    ImGuiInputText("unknown_2", &cat.unknown_2, 0, nullptr, nullptr);
    ImGui::InputScalar("unknown_3", ImGuiDataType_S32, &cat.unknown_3);

    ImGui::InputScalar("libido", ImGuiDataType_Double, &cat.libido);
    ImGui::InputScalar("sexuality", ImGuiDataType_Double, &cat.sexuality);
    ImGui::InputScalar("lover_sql_key", ImGuiDataType_S64, &cat.lover_sql_key);
    ImGui::InputScalar("lover_affinity", ImGuiDataType_Double, &cat.lover_affinity);
    ImGui::InputScalar("aggression", ImGuiDataType_Double, &cat.aggression);
    ImGui::InputScalar("hater_sql_key", ImGuiDataType_S64, &cat.hater_sql_key);
    ImGui::InputScalar("hater_affinity", ImGuiDataType_Double, &cat.hater_affinity);
    ImGui::InputScalar("fertility", ImGuiDataType_Double, &cat.fertility);
    ImGui::InputScalar("texture_sprite_idx", ImGuiDataType_U32, &cat.body_parts.texture_sprite_idx);
    ImGui::InputScalar("heritable_palette_idx", ImGuiDataType_U32, &cat.body_parts.heritable_palette_idx);
    ImGui::InputScalar("collar_palette_idx", ImGuiDataType_U32, &cat.body_parts.collar_palette_idx);
    ImGui::InputScalar("BodyParts.unknown_0", ImGuiDataType_U32, &cat.body_parts.unknown_0);
    ImGui::InputScalar("BodyParts.unknown_1", ImGuiDataType_U32, &cat.body_parts.unknown_1);
    ImguiTextStdFmt("BodyPartDescriptors");
    if(ImGui::BeginTable("bodyparts_table", 6)) {
        ImGui::TableSetupColumn("Part", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("part_sprite", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("texture_sprite", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("scar_sprite", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("unknown_1", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("unknown_2", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableHeadersRow();
        BodyPartDescriptor *p_base = &cat.body_parts.body;
        for(uint32_t i = 0; i < 14; i++) {
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            const char *label = i == 0 ? "body" :
                                i == 1 ? "head" :
                                i == 2 ? "tail" :
                                i == 3 ? "leg1" :
                                i == 4 ? "leg2" :
                                i == 5 ? "arm1" :
                                i == 6 ? "arm2" :
                                i == 7 ? "lefteye" :
                                i == 8 ? "righteye" :
                                i == 9 ? "lefteyebrow" :
                                i == 10 ? "righteyebrow" :
                                i == 11 ? "leftear" :
                                i == 12 ? "rightear" :
                                "mouth";
            ImguiTextStdFmt("{}", label);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##part_sprite_idx", ImGuiDataType_U32, &p_base[i].part_sprite_idx);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##texture_sprite_idx", ImGuiDataType_U32, &p_base[i].texture_sprite_idx);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##scar_sprite_idx", ImGuiDataType_U32, &p_base[i].scar_sprite_idx);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##unknown_1", ImGuiDataType_U32, &p_base[i].unknown_1);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##unknown_2", ImGuiDataType_U32, &p_base[i].unknown_2);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGuiInputText("body_parts.voice", &cat.body_parts.voice, 0, nullptr, nullptr);
    ImGui::InputScalar("body_parts.pitch", ImGuiDataType_Double, &cat.body_parts.pitch);
    ImguiTextStdFmt("Stats");
    if(ImGui::BeginTable("stats_table", 8)) {
        ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("str", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("dex", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("con", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("int", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("spd", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("cha", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("lck", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableHeadersRow();
        CatStats *p_base = &cat.stats_heritable;
        for(uint32_t i = 0; i < 3; i++) {
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", i == 0 ? "Heritable" : i == 1 ? "Levelling/Events" : "Injuries");
            ImGui::TableNextColumn();
            ImGui::InputScalar("##str", ImGuiDataType_S32, &p_base[i].str);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##dex", ImGuiDataType_S32, &p_base[i].dex);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##con", ImGuiDataType_S32, &p_base[i].con);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##int_", ImGuiDataType_S32, &p_base[i].int_);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##spd", ImGuiDataType_S32, &p_base[i].spd);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##cha", ImGuiDataType_S32, &p_base[i].cha);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##lck", ImGuiDataType_S32, &p_base[i].lck);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGuiInputText("last_injury_debuffed_stat", &cat.last_injury_debuffed_stat, 0, nullptr, nullptr);
    ImGui::InputScalar("campaign_stats.hp", ImGuiDataType_S32, &cat.campaign_stats.hp);
    ImGui::InputScalar("campaign_stats.dead", ImGuiDataType_U8, &cat.campaign_stats.dead);
    ImGui::InputScalar("campaign_stats.unknown_0", ImGuiDataType_U8, &cat.campaign_stats.unknown_0);
    ImGui::InputScalar("campaign_stats.unknown_1", ImGuiDataType_U32, &cat.campaign_stats.unknown_1);
    ImguiTextStdFmt("Event Stat Modifiers");
    if(ImGui::BeginTable("event_stat_modifiers_table", 3)) {
        ImGui::TableSetupColumn("pointer", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Expression", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Battles remaining", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableHeadersRow();
        int i = 0;
        for(auto &mod : cat.campaign_stats.event_stat_modifiers) {
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{:p}", reinterpret_cast<void *>(&mod));
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", mod.expression.SaveToStr(true)); // TODO GON editing
            ImGui::TableNextColumn();
            ImGui::InputScalar("##battles_remaining", ImGuiDataType_S32, &mod.battles_remaining);
            ImGui::PopID();
            i++;
        }
        ImGui::EndTable();
    }
    ImGuiInputText("actives_basic[0]", &cat.actives_basic[0], 0, nullptr, nullptr);
    ImGuiInputText("actives_basic[1]", &cat.actives_basic[1], 0, nullptr, nullptr);
    ImGuiInputText("actives_accessible[0]", &cat.actives_accessible[0], 0, nullptr, nullptr);
    ImGuiInputText("actives_accessible[1]", &cat.actives_accessible[1], 0, nullptr, nullptr);
    ImGuiInputText("actives_accessible[2]", &cat.actives_accessible[2], 0, nullptr, nullptr);
    ImGuiInputText("actives_accessible[3]", &cat.actives_accessible[3], 0, nullptr, nullptr);
    ImGuiInputText("actives_inherited[0]", &cat.actives_inherited[0], 0, nullptr, nullptr);
    ImGuiInputText("actives_inherited[1]", &cat.actives_inherited[1], 0, nullptr, nullptr);
    ImGuiInputText("actives_inherited[2]", &cat.actives_inherited[2], 0, nullptr, nullptr);
    ImGuiInputText("actives_inherited[3]", &cat.actives_inherited[3], 0, nullptr, nullptr);
    ImGuiInputText("passive_0", &cat.passive_0, 0, nullptr, nullptr);
    ImGui::InputScalar("passive_0_level", ImGuiDataType_S64, &cat.passive_0_level);
    ImGuiInputText("passive_1", &cat.passive_1, 0, nullptr, nullptr);
    ImGui::InputScalar("passive_1_level", ImGuiDataType_S64, &cat.passive_1_level);
    ImGuiInputText("mutation_0", &cat.mutation_0, 0, nullptr, nullptr);
    ImGui::InputScalar("mutation_0_level", ImGuiDataType_S64, &cat.mutation_0_level);
    ImGuiInputText("mutation_1", &cat.mutation_1, 0, nullptr, nullptr);
    ImGui::InputScalar("mutation_1_level", ImGuiDataType_S64, &cat.mutation_1_level);
    ImguiTextStdFmt("Equipment");
    if(ImGui::BeginTable("equipment_table", 10)) {
        ImGui::TableSetupColumn("Thing", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Aux", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Uses left", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("unknown_2", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("unknown_3", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("unknown_4", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("unknown_5", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Num adventures", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableHeadersRow();
        Equipment *p_base = &cat.head;
        for(uint32_t i = 0; i < 5; i++) {
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            const char *label = i == 0 ? "head" :
                                i == 1 ? "face" :
                                i == 2 ? "neck" :
                                i == 3 ? "weapon" :
                                "trinket";
            ImguiTextStdFmt("{}", label);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##id", ImGuiDataType_S64, &p_base[i].id);
            ImGui::TableNextColumn();
            ImGuiInputText("##name", &p_base[i].name, 0, nullptr, nullptr);
            ImGui::TableNextColumn();
            ImGuiInputText("##aux_string", &p_base[i].aux_string, 0, nullptr, nullptr);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##uses_left", ImGuiDataType_S32, &p_base[i].uses_left);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##unknown_2", ImGuiDataType_S32, &p_base[i].unknown_2);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##unknown_3", ImGuiDataType_S32, &p_base[i].unknown_3);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##unknown_4", ImGuiDataType_S32, &p_base[i].unknown_4);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##unknown_5", ImGuiDataType_S32, &p_base[i].unknown_5);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##times_taken_on_adventure", ImGuiDataType_U8, &p_base[i].times_taken_on_adventure);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImguiTextStdFmt("Collar: {}", cat.collar);
    ImGui::InputScalar("level", ImGuiDataType_U32, &cat.level);
    ImGui::InputScalar("coi", ImGuiDataType_Double, &cat.coi);
    ImGui::InputScalar("birthday", ImGuiDataType_S64, &cat.birthday);
    ImGui::InputScalar("deathday_house", ImGuiDataType_S64, &cat.deathday_house);
    ImguiTextStdFmt("house_boss_kills");
    ImguiTextStdFmt("house_boss_kills size/capacity: {} {}", cat.house_boss_kills.size_, cat.house_boss_kills.capacity_);
    if(ImGui::BeginTable("house_boss_kills_table", 2)) {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableHeadersRow();
        for(uint32_t i = 0; i < cat.house_boss_kills.size_; i++) {
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", i);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##data", ImGuiDataType_U8, &cat.house_boss_kills.data_[i]);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::InputScalar("lifestage", ImGuiDataType_U32, &cat.lifestage);
    ImGui::InputScalar("cleared_zones", ImGuiDataType_U64, &cat.cleared_zones, nullptr, nullptr, "%016llx");
    std::string cleared_zones_list = "";
    for(int i = 63; i >= 0; i--) {
        if((cat.cleared_zones >> i) & 1) {
            cleared_zones_list += std::format("{} ", i);
        }
    }
    ImguiTextStdFmt("Cleared zones: {}", cleared_zones_list);
    ImGui::InputScalar("completed_act", ImGuiDataType_U8, &cat.completed_act);
    ImGui::InputScalar("completed_chapter", ImGuiDataType_U8, &cat.completed_chapter);
    ImGui::InputScalar("completed_difficulty", ImGuiDataType_U8, &cat.completed_difficulty);
    ImguiTextStdFmt("Injuries");
    if(ImGui::BeginTable("injury_table", 2)) {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableHeadersRow();
        for(int i = 0; i < 16; i++) {
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", i);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##count", ImGuiDataType_S32, &cat.injuries[i]);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

void show_cat(CatData &cat) {
    ImguiTextStdFmt("p_CatData: {:p}", reinterpret_cast<void *>(&cat));
    ImguiTextStdFmt("Name: {}", convert_utf16_wstring_to_utf8_string(cat.name));
    ImguiTextStdFmt("Nameplate symbol: {}", cat.nameplate_symbol);
    ImguiTextStdFmt("Entropy: 0x{:x}", cat.entropy);
    ImguiTextStdFmt("Sex: {} {}", cat.sex, cat.sex_dup);
    ImguiTextStdFmt("Flags: 0x{:x}", cat.flags);
    std::string flags_list = "";
    for(int i = 63; i >= 0; i--) {
        if((cat.flags >> i) & 1) {
            flags_list += std::format("{} ", i);
        }
    }
    ImguiTextStdFmt("Flags: {}", flags_list);
    ImguiTextStdFmt("unknown 2/3: {} {}", cat.unknown_2, cat.unknown_3);
    ImguiTextStdFmt("Libido: {}", cat.libido);
    ImguiTextStdFmt("Sexuality: {}", cat.sexuality);
    ImguiTextStdFmt("Loves: {} ({})", cat.lover_sql_key, cat.lover_affinity);
    ImguiTextStdFmt("Aggression: {}", cat.aggression);
    ImguiTextStdFmt("Hates: {} ({})", cat.hater_sql_key, cat.hater_affinity);
    ImguiTextStdFmt("Fertility: {}", cat.fertility);
    ImguiTextStdFmt("Texture/palettes: {} {} {}", cat.body_parts.texture_sprite_idx, cat.body_parts.heritable_palette_idx, cat.body_parts.collar_palette_idx);
    ImguiTextStdFmt("BodyParts.unknown_0/1: {} {}", cat.body_parts.unknown_0, cat.body_parts.unknown_1);
    ImguiTextStdFmt("BodyPartDescriptors");
    if(ImGui::BeginTable("bodyparts_table", 6)) {
        ImGui::TableSetupColumn("Part", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("part_sprite", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("texture_sprite", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("scar_sprite", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("unknown_1", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("unknown_2", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableHeadersRow();
        BodyPartDescriptor *p_base = &cat.body_parts.body;
        for(uint32_t i = 0; i < 14; i++) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            const char *label = i == 0 ? "body" :
                                i == 1 ? "head" :
                                i == 2 ? "tail" :
                                i == 3 ? "leg1" :
                                i == 4 ? "leg2" :
                                i == 5 ? "arm1" :
                                i == 6 ? "arm2" :
                                i == 7 ? "lefteye" :
                                i == 8 ? "righteye" :
                                i == 9 ? "lefteyebrow" :
                                i == 10 ? "righteyebrow" :
                                i == 11 ? "leftear" :
                                i == 12 ? "rightear" :
                                "mouth";
            ImguiTextStdFmt("{}", label);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].part_sprite_idx);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].texture_sprite_idx);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].scar_sprite_idx);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].unknown_1);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].unknown_2);
        }
        ImGui::EndTable();
    }
    ImguiTextStdFmt("Voice: {} ({})", cat.body_parts.voice, cat.body_parts.pitch);
    ImguiTextStdFmt("Stats");
    if(ImGui::BeginTable("stats_table", 8)) {
        ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("str", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("dex", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("con", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("int", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("spd", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("cha", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("lck", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableHeadersRow();
        CatStats *p_base = &cat.stats_heritable;
        for(uint32_t i = 0; i < 3; i++) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", i == 0 ? "Heritable" : i == 1 ? "Levelling/Events" : "Injuries");
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].str);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].dex);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].con);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].int_);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].spd);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].cha);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].lck);
        }
        ImGui::EndTable();
    }
    ImguiTextStdFmt("Last debuff: {}", cat.last_injury_debuffed_stat);
    ImguiTextStdFmt("HP: {}", cat.campaign_stats.hp);
    ImguiTextStdFmt("Dead: {}", cat.campaign_stats.dead);
    ImguiTextStdFmt("CampaignStats.unknown_0: {}", cat.campaign_stats.unknown_0);
    ImguiTextStdFmt("CampaignStats.unknown_1: {}", cat.campaign_stats.unknown_1);
    ImguiTextStdFmt("Event Stat Modifiers");
    if(ImGui::BeginTable("event_stat_modifiers_table", 3)) {
        ImGui::TableSetupColumn("pointer", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Expression", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Battles remaining", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableHeadersRow();
        for(auto &mod : cat.campaign_stats.event_stat_modifiers) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{:p}", reinterpret_cast<void *>(&mod));
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", mod.expression.SaveToStr(true));
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", mod.battles_remaining);
        }
        ImGui::EndTable();
    }
    for(int i = 0; i < 2; i++) {
        auto p_base = cat.actives_basic;
        ImguiTextStdFmt("Basic {}: {}", i, p_base[i]);
    }
    for(int i = 0; i < 4; i++) {
        auto p_base = cat.actives_accessible;
        ImguiTextStdFmt("Active (accessible) {}: {}", i, p_base[i]);
    }
    for(int i = 0; i < 4; i++) {
        auto p_base = cat.actives_inherited;
        ImguiTextStdFmt("Active (inherited) {}: {}", i, p_base[i]);
    }
    ImguiTextStdFmt("Passive 0: {} {}", cat.passive_0, cat.passive_0_level);
    ImguiTextStdFmt("Passive 1: {} {}", cat.passive_1, cat.passive_1_level);
    ImguiTextStdFmt("Mutation 0: {} {}", cat.mutation_0, cat.mutation_0_level);
    ImguiTextStdFmt("Mutation 1: {} {}", cat.mutation_1, cat.mutation_1_level);
    ImguiTextStdFmt("Equipment");
    if(ImGui::BeginTable("equipment_table", 10)) {
        ImGui::TableSetupColumn("Thing", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Aux", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Uses left", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("unknown_2", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("unknown_3", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("unknown_4", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("unknown_5", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Num adventures", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableHeadersRow();
        Equipment *p_base = &cat.head;
        for(uint32_t i = 0; i < 5; i++) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            const char *label = i == 0 ? "head" :
                                i == 1 ? "face" :
                                i == 2 ? "neck" :
                                i == 3 ? "weapon" :
                                "trinket";
            ImguiTextStdFmt("{}", label);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].id);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].name);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].aux_string);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].uses_left);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].unknown_2);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].unknown_3);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].unknown_4);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].unknown_5);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].times_taken_on_adventure);
        }
        ImGui::EndTable();
    }
    ImguiTextStdFmt("Collar: {}", cat.collar);
    ImguiTextStdFmt("Level: {}", cat.level);
    ImguiTextStdFmt("COI: {}", cat.coi);
    ImguiTextStdFmt("Birthday: {}", cat.birthday); // TODO calculate days ago
    ImguiTextStdFmt("Deathday (if died in house): {}", cat.deathday_house);
    ImguiTextStdFmt("House boss kills");
    ImguiTextStdFmt("House boss kills size/capacity: {} {}", cat.house_boss_kills.size_, cat.house_boss_kills.capacity_);
    if(ImGui::BeginTable("house_boss_kills_table", 2)) {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableHeadersRow();
        for(uint32_t i = 0; i < cat.house_boss_kills.size_; i++) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", i);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", cat.house_boss_kills.data_[i]);
        }
        ImGui::EndTable();
    }
    ImguiTextStdFmt("Lifestage: {}", cat.lifestage);
    ImguiTextStdFmt("Cleared zones: 0x{:x}", cat.cleared_zones);
    std::string cleared_zones_list = "";
    for(int i = 63; i >= 0; i--) {
        if((cat.cleared_zones >> i) & 1) {
            cleared_zones_list += std::format("{} ", i);
        }
    }
    ImguiTextStdFmt("Cleared zones: {}", cleared_zones_list);
    ImguiTextStdFmt("Completed act: {}", cat.completed_act);
    ImguiTextStdFmt("Completed chapter: {}", cat.completed_chapter);
    ImguiTextStdFmt("Completed difficulty: {}", cat.completed_difficulty);
    ImguiTextStdFmt("Injuries");
    if(ImGui::BeginTable("injury_table", 2)) {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableHeadersRow();
        for(int i = 0; i < 16; i++) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", i);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", cat.injuries[i]);
        }
        ImGui::EndTable();
    }
}

void show_data_explorer_window() {
    if(!P.show_data_explorer) {
        return;
    }
    ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
    ImGui::SetNextWindowSize(ImVec2(viewport_size.x * 0.4f, viewport_size.y * 0.4f), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Data explorer", &P.show_data_explorer)) {
        // good architecture, just need to null check one global and you're ready to go!
        MewDirector *p_md = get_p_mewdirector_singleton();

        if(ImGui::TreeNode("Pointers")) {
            ImguiTextStdFmt("Hook base VA: {:p}", reinterpret_cast<void *>(G.dll_base_va));
            ImguiTextStdFmt("Executable base VA: {:p}", reinterpret_cast<void *>(G.host_exec_base_va));
            ImguiTextStdFmt("p_MewDirector: {:p}", static_cast<void *>(p_md));
            if(p_md != nullptr) {
                ImguiTextStdFmt("p_Director: {:p}", static_cast<void *>(&p_md->director));
                ImguiTextStdFmt("p_CatDatabase: {:p}", static_cast<void *>(p_md->cats));
                ImguiTextStdFmt("p_SQLSaveFile: {:p}", static_cast<void *>(&p_md->sqlsavefile));
                ImguiTextStdFmt("p_GlobalProgressionData: {:p}", static_cast<void *>(p_md->global_progression_data));
            }
            ImGui::TreePop();
        }

        if(ImGui::TreeNode("Thread-local storage")) {
            auto *p_tls = get_tls_base(0);
            ImguiTextStdFmt("TLS Slot 0 base VA: {:p}", reinterpret_cast<void *>(p_tls));
            Xoshiro256pContext &rng = get_xoshiro256p_rng_context();
            for(int i = 0; i < 4; i++) {
                ImguiTextStdFmt("RNG context {}: 0x{:x}", i, rng.ctx[i]);
            }
            ImGui::TreePop();
        }


        if(ImGui::TreeNode("sqlite3 connection")) {
            if(p_md != nullptr) {
                SQLSaveFile *p_sqlsavefile = &p_md->sqlsavefile;
                ImguiTextStdFmt("p_conn: {:p}", static_cast<void *>(p_sqlsavefile->conn));
                ImguiTextStdFmt("Active DB file: {}", convert_filesystem_path_to_utf8_string(std::filesystem::path(p_sqlsavefile->file_path.as_native_string_view()).filename()));
            }
            ImGui::TreePop();
        }

        if(ImGui::TreeNode("Loaded cats")) {
            if(p_md != nullptr) {
                // the game appears to lazy-query cats as needed to view loved/hated cat names and family tree portraits
                auto &cats = p_md->cats->cats;
                ImguiTextStdFmt("Size: {}", cats._List._Mysize);
                if(ImGui::BeginTable("table1", 4)) {
                    ImGui::TableSetupColumn("Cat", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 3.0f);
                    ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                    ImGui::TableSetupColumn("p_CatData", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                    ImGui::TableHeadersRow();
                    auto head = cats._List._Myhead;
                    auto current = head->_Next;
                    while(current != head) {
                        auto &cat = *current->_Myval.cat;
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImguiTextStdFmt("{}", current->_Myval.sql_key);
                        ImGui::TableNextColumn();
                        ImguiTextStdFmt("{}", convert_utf16_wstring_to_utf8_string(cat.name));
                        ImGui::TableNextColumn();
                        ImguiTextStdFmt("{}", cat.nameplate_symbol);
                        ImGui::TableNextColumn();
                        ImguiTextStdFmt("{:p}", reinterpret_cast<void *>(&cat));
                        current = current->_Next;
                    }
                    ImGui::EndTable();
                }
                if(ImGui::TreeNode("Show hash map details")) {
                    ImguiTextStdFmt("_Max_bucket_size: {:f}", cats._Max_bucket_size);
                    ImguiTextStdFmt("_Mask: 0x{:x}", cats._Mask);
                    ImguiTextStdFmt("_Maxidx: 0x{:x}", cats._Maxidx);
                    ImguiTextStdFmt("_Unchecked_end: {:p}", reinterpret_cast<void *>(cats._List._Myhead));
                    if(ImGui::BeginTable("table1", 3)) {
                        ImGui::TableSetupColumn("Bucket", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                        ImGui::TableSetupColumn("Start (incl)", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                        ImGui::TableSetupColumn("End (incl)", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                        ImGui::TableHeadersRow();
                        size_t i = 0;
                        for(auto &bucket : cats._Vec) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{:02x}", i);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", reinterpret_cast<void *>(bucket.first));
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", reinterpret_cast<void *>(bucket.last));
                            i++;
                        }
                        ImGui::EndTable();
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }

        if(ImGui::TreeNode("Cats to delete")) {
            if(p_md != nullptr) {
                // the game appears to delete kittens and strays if they are discarded on their first day at the house
                // (unsure what happens with donations, or discarding a cat met on an adventure)
                auto &cats = p_md->cats->cats_to_delete;
                ImguiTextStdFmt("Size: {}", cats._List._Mysize);
                if(ImGui::BeginTable("table1", 1)) {
                    ImGui::TableSetupColumn("Cat", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableHeadersRow();
                    auto head = cats._List._Myhead;
                    auto current = head->_Next;
                    while(current != head) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImguiTextStdFmt("{}", current->_Myval);
                        current = current->_Next;
                    }
                    ImGui::EndTable();
                }
                if(ImGui::TreeNode("Show hash map details")) {
                    ImguiTextStdFmt("_Max_bucket_size: {:f}", cats._Max_bucket_size);
                    ImguiTextStdFmt("_Mask: 0x{:x}", cats._Mask);
                    ImguiTextStdFmt("_Maxidx: 0x{:x}", cats._Maxidx);
                    ImguiTextStdFmt("_Unchecked_end: {:p}", reinterpret_cast<void *>(cats._List._Myhead));
                    if(ImGui::BeginTable("table1", 3)) {
                        ImGui::TableSetupColumn("Bucket", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                        ImGui::TableSetupColumn("Start (incl)", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                        ImGui::TableSetupColumn("End (incl)", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                        ImGui::TableHeadersRow();
                        size_t i = 0;
                        for(auto &bucket : cats._Vec) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{:02x}", i);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", reinterpret_cast<void *>(bucket.first));
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", reinterpret_cast<void *>(bucket.last));
                            i++;
                        }
                        ImGui::EndTable();
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }

        if(ImGui::TreeNode("Pedigree map")) {
            if(p_md != nullptr) {
                auto &map = p_md->cats->pedigree.child_to_parents_and_coi_map;
                ImguiTextStdFmt("Size, cap, growth_left: {}, {}, {}", map.size, map.cap, map.growth_left);
                ImguiTextStdFmt("Hashing test passed: {}", map.verify_get());
                if(ImGui::BeginTable("table1", 6)) {
                    ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("H2", ImGuiTableColumnFlags_WidthStretch, 0.5f);
                    ImGui::TableSetupColumn("Child", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("Parent A", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("Parent B", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("COI", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                    ImGui::TableHeadersRow();

                    for(size_t i = 0; i < map.cap; i++) {
                        if(map.ctrl[i] <= 0x7F) {
                            auto entry = map.slots[i];
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", i);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{:02x}", map.ctrl[i]);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", entry.key);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", entry.val.parent_a);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", entry.val.parent_b);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", entry.val.coi);
                        }
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::TreePop();
        }

        if(ImGui::TreeNode("Parent COI map")) {
            if(p_md != nullptr) {
                auto &map = p_md->cats->pedigree.parents_to_coi_memo_map;
                ImguiTextStdFmt("Size, cap, growth_left: {}, {}, {}", map.size, map.cap, map.growth_left);
                ImguiTextStdFmt("Hashing test passed: {}", map.verify_get());
                if(ImGui::BeginTable("table1", 5)) {
                    ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("H2", ImGuiTableColumnFlags_WidthStretch, 0.5f);
                    ImGui::TableSetupColumn("Parent A", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("Parent B", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("COI", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                    ImGui::TableHeadersRow();

                    for(size_t i = 0; i < map.cap; i++) {
                        if(map.ctrl[i] <= 0x7F) {
                            auto entry = map.slots[i];
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", i);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{:02x}", map.ctrl[i]);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", entry.key.parent_a);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", entry.key.parent_b);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", entry.val);
                        }
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::TreePop();
        }

        if(ImGui::TreeNode("Accessible cat set")) {
            if(p_md != nullptr) {
                auto &map = p_md->cats->pedigree.accessible_cats;
                ImguiTextStdFmt("Size, cap, growth_left: {}, {}, {}", map.size, map.cap, map.growth_left);
                ImguiTextStdFmt("Hashing test passed: {}", map.verify_get());
                if(ImGui::BeginTable("table1", 3)) {
                    ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("H2", ImGuiTableColumnFlags_WidthStretch, 0.5f);
                    ImGui::TableSetupColumn("Cat", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableHeadersRow();

                    for(size_t i = 0; i < map.cap; i++) {
                        if(map.ctrl[i] <= 0x7F) {
                            auto entry = map.slots[i];
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", i);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{:02x}", map.ctrl[i]);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", entry.key);
                        }
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::TreePop();
        }

        if(ImGui::TreeNode("Generated name history")) {
            if(p_md != nullptr) {
                auto &names = p_md->cats->name_gen_history_w;
                ImguiTextStdFmt("Size, capacity: {}, {}", names.size(), names.capacity());
                if(ImGui::BeginTable("table1", 1)) {
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableHeadersRow();
                    for(auto &name : names) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImguiTextStdFmt("{}", convert_utf16_wstring_to_utf8_string(name));
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::TreePop();
        }

        auto draw_component_table = [](podvector<Component *> *components) -> void {
            if(ImGui::BeginTable("table1", 7)) {
                ImGui::TableSetupColumn("Pointer", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                ImGui::TableSetupColumn("ObjID", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                ImGui::TableSetupColumn("Overrides", ImGuiTableColumnFlags_WidthStretch, 1.2f);
                ImGui::TableSetupColumn("EE/D/E/S", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                ImGui::TableSetupColumn("Type ID", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                ImGui::TableSetupColumn("Type Name", ImGuiTableColumnFlags_WidthStretch, 3.0f);
                ImGui::TableSetupColumn("p_entity", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                // ImGui::TableSetupColumn("p_scene", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                // ImGui::TableSetupColumn("p_director", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                // ImGui::TableSetupColumn("p_entity->scene", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                ImGui::TableHeadersRow();
                for(auto &p_component : *components) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImguiTextStdFmt("{:p}", static_cast<void *>(p_component));
                    ImGui::TableNextColumn();
                    ImguiTextStdFmt("{}", p_component->_objid);
                    ImGui::TableNextColumn();
                    ImguiTextStdFmt("{:02x} {:02x}", p_component->override_tags_B0, p_component->override_tags_B1);
                    ImGui::TableNextColumn();
                    ImguiTextStdFmt("{:d} {:d} {:d} {:d}", p_component->entity_enabled, p_component->deleted, p_component->enabled, p_component->started);
                    ImGui::TableNextColumn();
                    ImguiTextStdFmt("{}", p_component->vtable->GetObjectType(p_component));
                    ImGui::TableNextColumn();
                    MsvcReleaseModeXString type_name = {};
                    p_component->vtable->GetObjectTypeSTR(p_component, &type_name); // in-place string construction
                    ImguiTextStdFmt("{}", type_name.as_native_string_view());
                    type_name.destroy();
                    ImGui::TableNextColumn();
                    ImguiTextStdFmt("{:p}", static_cast<void *>(p_component->entity));
                    // ImGui::TableNextColumn();
                    // ImguiTextStdFmt("{:p}", static_cast<void *>(p_component->scene));
                    // ImGui::TableNextColumn();
                    // ImguiTextStdFmt("{:p}", static_cast<void *>(p_component->director));
                    // ImGui::TableNextColumn();
                    // ImguiTextStdFmt("{:p}", static_cast<void *>(p_component->entity->scene));
                }
                ImGui::EndTable();
            }
        };

        if(ImGui::TreeNode("Scenes")) {
            if(p_md != nullptr) {
                for(auto &p_scene : p_md->director->scenes) {
                    if(ImGui::TreeNode(p_scene->name.copy_to_native_string().c_str())) {
                        if(p_scene != nullptr) {
                            ImguiTextStdFmt("p_scene: {:p}", static_cast<void *>(p_scene));
                            if(ImGui::TreeNode("By Entity")) {
                                for(auto &p_entity : p_scene->Entities) {
                                    if(ImGui::TreeNode(std::format("{:p}", static_cast<void *>(p_entity)).c_str())) {
                                        draw_component_table(&p_entity->components);
                                        ImGui::TreePop();
                                    }
                                }
                                ImGui::TreePop();
                            }
                            if(ImGui::TreeNode("All Components")) {
                                draw_component_table(p_scene->ComponentLists);
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::TreePop();
        }

        if(ImGui::TreeNode("House")) {
            if(p_md != nullptr) {
                Scene *p_house_scene = nullptr;
                for(auto p_scene : p_md->director->scenes) {
                    if(p_scene->name.as_native_string_view() == "House") {
                        p_house_scene = p_scene;
                        break;
                    }
                }
                ImguiTextStdFmt("HouseCats");
                if(p_house_scene != nullptr && ImGui::BeginTable("table2", 1)) {
                    ImGui::TableSetupColumn("SQL ID", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableHeadersRow();
                    for(auto p_housecat : *p_house_scene->ComponentLists) {
                        const char COMPONENT_TYPE_NAME_HOUSECAT[] = "HouseCat";
                        MsvcReleaseModeXString type_name = {};
                        p_housecat->vtable->GetObjectTypeSTR(p_housecat, &type_name); // in-place string construction
                        if(type_name.as_native_string_view() != COMPONENT_TYPE_NAME_HOUSECAT) {
                            type_name.destroy();
                            continue;
                        }
                        type_name.destroy();
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        auto housecat = static_cast<HouseCat *>(p_housecat);
                        ImguiTextStdFmt("{}", housecat->sql_key);
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::TreePop();
        }

        if(ImGui::TreeNode("Component pools")) {
            auto &housecat_pool = get_housecat_component_pool();
            if(ImGui::TreeNode("HouseCat")) {
                ImguiTextStdFmt("ReservedMemorySize: {} B", housecat_pool.ReservedMemorySize);
                ImguiTextStdFmt("MinItemsPerBlock: {}", housecat_pool.MinItemsPerBlock);
                ImguiTextStdFmt("ElementSize (sizeof(HouseCat) + 8 B): {} B", housecat_pool.ElementSize);
                ImguiTextStdFmt("next_uncommitted_page: {:p}", housecat_pool.next_uncommitted_page);
                ImguiTextStdFmt("last_uncommitted_page: {:p}", housecat_pool.last_uncommitted_page);
                ImguiTextStdFmt("next_available_element: {:p}", static_cast<void *>(housecat_pool.next_available_element));
                ImguiTextStdFmt("first_free_location: {:p}", reinterpret_cast<void *>(housecat_pool.first_free_location));
                ImguiTextStdFmt("needs_sort: {}", housecat_pool.needs_sort);

                if(ImGui::TreeNode("Block list")) {
                    if(ImGui::BeginTable("table1", 2)) {
                        ImGui::TableSetupColumn("p_reserved_block", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                        ImGui::TableSetupColumn("p_buffer", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                        ImGui::TableHeadersRow();
                        auto p_descriptor = housecat_pool.initial_block;
                        while(p_descriptor != nullptr) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{:p}", reinterpret_cast<void *>(p_descriptor));
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{:p}", p_descriptor->buffer);
                            // the current chunk's next pointer isn't zero-initialized, grrr...
                            if(p_descriptor == housecat_pool.last_block) {
                                break;
                            }
                            p_descriptor = p_descriptor->next_bucket;
                        }
                        ImGui::EndTable();
                    }
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Free list")) {
                    if(ImGui::BeginTable("table1", 1)) {
                        ImGui::TableSetupColumn("p_location", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                        ImGui::TableHeadersRow();
                        auto p_free = housecat_pool.first_free_location;
                        while(p_free != nullptr) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{:p}", reinterpret_cast<void *>(p_free));
                            p_free = p_free->u.next_free;
                        }
                        ImGui::EndTable();
                    }
                    ImGui::TreePop();
                }
                // can theoretically walk other blocks, but would probably need a clipper to view several million cats
                if(ImGui::TreeNode("Allocated elements (last block)")) {
                    if(ImGui::BeginTable("table1", 3)) {
                        ImGui::TableSetupColumn("p_element", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                        ImGui::TableSetupColumn("generation", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                        ImGui::TableSetupColumn("data_first_qword", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                        ImGui::TableHeadersRow();
                        char *p_first_element = reinterpret_cast<char *>(housecat_pool.last_uncommitted_page) - housecat_pool.ReservedMemorySize;
                        for(
                            auto p_element = reinterpret_cast<VGAAElement<HouseCat> *>(p_first_element);
                            p_element < housecat_pool.next_available_element;
                            p_element++
                        ) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{:p}", reinterpret_cast<void *>(p_element));
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("0x{:016x}", p_element->generation);
                            ImGui::TableNextColumn();
                            // read the first qword of data through the next_free data element
                            ImguiTextStdFmt("0x{:016x}", reinterpret_cast<uint64_t>(p_element->u.next_free));
                        }
                        ImGui::EndTable();
                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        if(ImGui::TreeNode("Global progression data")) {
            if(p_md != nullptr) {
                auto &gpd = *p_md->global_progression_data;
                auto npc_line = [&](std::string name, NPC npc_id) {
                    if(ImGui::TreeNode(name.c_str())) {
                        NPCInfo &info = gpd.npc_info[npc_id];
                        if(ImGui::TreeNode("Unlocks")) {
                            for(auto &su : info.unlocks) {
                                ImguiTextStdFmt("{}: {}", su.string, su.u32);
                            }
                            ImGui::TreePop();
                        }
                        ImguiTextStdFmt("Next unlock: {}", info.next_unlock);
                        ImguiTextStdFmt("a: {}", info.a);
                        ImguiTextStdFmt("next_unlock_cat_counter: {}", info.next_unlock_cat_counter);
                        ImguiTextStdFmt("c: {}", info.c);
                        ImguiTextStdFmt("d: {}", info.d);
                        ImguiTextStdFmt("e: {}", info.e);
                        ImguiTextStdFmt("f: {}", info.f);
                        ImguiTextStdFmt("g: {}", info.g);
                        std::string repr_h;
                        for(int i = 0; i < 32; i++) {
                            repr_h += std::format("{:02x}", info.h[i]);
                        }
                        ImguiTextStdFmt("h: {}", repr_h);
                        ImguiTextStdFmt("i: {}", info.i);
                        ImGui::TreePop();
                    }
                };
                auto shop_line = [&](std::string name, MsvcReleaseModeVector<ShopItem> &shop) {
                    if(ImGui::TreeNode(name.c_str())) {
                        for(auto &item : shop) {
                            // TODO more fields
                            ImguiTextStdFmt("name: {} ({})", item.name, item.equipment.name);
                        }
                        ImGui::TreePop();
                    }
                };
                auto unknown5_line = [&](std::string name, MsvcReleaseModeVector<DecompUnknown5Compartment> &v) {
                    if(ImGui::TreeNode(name.c_str())) {
                        for(auto &item : v) {
                            // TODO more fields
                            ImguiTextStdFmt("a: {}", item.a);
                        }
                        ImGui::TreePop();
                    }
                };
                auto collarstats_line = [&](std::string name, MsvcReleaseModeVector<CollarStats> &v) {
                    if(ImGui::TreeNode(name.c_str())) {
                        if(ImGui::BeginTable("table1", 7)) {
                            ImGui::TableSetupColumn("Chapter", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                            ImGui::TableSetupColumn("Collar", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                            ImGui::TableSetupColumn("a", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                            ImGui::TableSetupColumn("b", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                            ImGui::TableSetupColumn("c", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                            ImGui::TableSetupColumn("d", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                            ImGui::TableSetupColumn("e", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                            ImGui::TableHeadersRow();
                            for(auto &cs : v) {
                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                ImguiTextStdFmt("{}", cs.chapter);
                                ImGui::TableNextColumn();
                                ImguiTextStdFmt("{}", cs.collar);
                                ImGui::TableNextColumn();
                                ImguiTextStdFmt("{}", cs.a);
                                ImGui::TableNextColumn();
                                ImguiTextStdFmt("{}", cs.b);
                                ImGui::TableNextColumn();
                                ImguiTextStdFmt("{}", cs.c);
                                ImGui::TableNextColumn();
                                ImguiTextStdFmt("{}", cs.d);
                                ImGui::TableNextColumn();
                                ImguiTextStdFmt("{}", cs.e);
                            }
                            ImGui::EndTable();
                        }
                        ImGui::TreePop();
                    }
                };
                auto u32string_line = [&](std::string name, MsvcReleaseModeVector<U32StringPair> &v) {
                    if(ImGui::TreeNode(name.c_str())) {
                        for(auto &pair : v) {
                            // TODO more fields
                            ImguiTextStdFmt("{}: {}", pair.string, pair.u32);
                        }
                        ImGui::TreePop();
                    }
                };
                auto string_line = [&](std::string name, MsvcReleaseModeVector<MsvcReleaseModeXString> &v) {
                    if(ImGui::TreeNode(name.c_str())) {
                        for(auto &s : v) {
                            ImguiTextStdFmt("{}", s);
                        }
                        ImGui::TreePop();
                    }
                };
                auto u64_line = [&](std::string name, podvector<uint64_t> &v) {
                    if(ImGui::TreeNode(name.c_str())) {
                        for(auto &s : v) {
                            ImguiTextStdFmt("{}", s);
                        }
                        ImGui::TreePop();
                    }
                };
                if(ImGui::TreeNode("NPCs")) {
                    npc_line("Beanies", NPC::Beanies);
                    npc_line("Butch", NPC::Butch);
                    npc_line("Tink", NPC::Tink);
                    npc_line("Frank", NPC::Frank);
                    npc_line("Jack", NPC::Jack);
                    npc_line("Tracy", NPC::Tracy);
                    npc_line("Organ", NPC::Organ);
                    npc_line("Steven", NPC::Steven);
                    ImGui::TreePop();
                }

                if(ImGui::TreeNode("Shops")) {
                    shop_line("Jack", gpd.jack_shop);
                    shop_line("Tracy", gpd.tracy_shop);
                    shop_line("Organ", gpd.organ_shop);
                    shop_line("Idol", gpd.idol_shop);
                    ImGui::TreePop();
                }

                ImguiTextStdFmt("unknown_1: {}", gpd.unknown_1);
                ImguiTextStdFmt("unknown_2: {}", gpd.unknown_2);

                string_line("collars", gpd.collars);
                string_line("collarlessactives", gpd.collarlessactives);
                string_line("collaractivespassives", gpd.collaractivespassives);

                ImguiTextStdFmt("unknown_3: {}", gpd.unknown_3);

                u32string_line("unknown_4", gpd.unknown_4);
                u32string_line("plotflags", gpd.plotflags);

                string_line("unlocks", gpd.unlocks);
                string_line("class_unlock_helmets", gpd.class_unlock_helmets);
                unknown5_line("unknown_5", gpd.unknown_5);

                string_line("houseboss", gpd.houseboss);
                string_line("chapters", gpd.chapters);
                string_line("questitems", gpd.questitems);

                collarstats_line("Collar stats", gpd.collarstats);
                string_line("idols", gpd.idols);
                string_line("idols2", gpd.idols2);

                ImguiTextStdFmt("unknown_6: {}", gpd.unknown_6);
                ImguiTextStdFmt("unknown_7: {}", gpd.unknown_7);
                ImguiTextStdFmt("unknown_8: {}", gpd.unknown_8);

                string_line("unknown_9", gpd.unknown_9);

                ImguiTextStdFmt("steam_username: {}", gpd.steam_username);

                ImguiTextStdFmt("unknown_11: {}", gpd.unknown_11);
                ImguiTextStdFmt("unknown_12: {}", gpd.unknown_12);

                string_line("beanies_questitems", gpd.beanies_questitems);
                string_line("beanies_questitems2", gpd.beanies_questitems2);

                string_line("unknown_13", gpd.unknown_13);
                string_line("unknown_14", gpd.unknown_14);

                ImguiTextStdFmt("unknown_15: {}", gpd.unknown_15);
                ImguiTextStdFmt("unknown_16: {}", gpd.unknown_16);

                for(int i = 0; i < 18; i++) {
                    ImguiTextStdFmt("unknown_17[{}]: {}", i, gpd.unknown_17[i]);
                }

                ImguiTextStdFmt("unknown_35: {}", gpd.unknown_35);

                string_line("unknown_36", gpd.unknown_36);
                string_line("unknown_37", gpd.unknown_37);

                u64_line("unknown_38", gpd.unknown_38);

                ImguiTextStdFmt("kaiju: {}", gpd.kaiju);

                string_line("unknown_39", gpd.unknown_39);

                ImguiTextStdFmt("unknown_40: {}", gpd.unknown_40);
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

std::unordered_map<int64_t, CatData *> build_unified_cat_table() {
    // merge the two maps, taking game CatData instances over analyzer CatData, and sort by sql id
    std::unordered_map<int64_t, CatData *> unified_cat_table;

    MewDirector *p_md = get_p_mewdirector_singleton();
    if(p_md != nullptr) {
        CatDatabase *p_cdb = p_md->cats;
        unified_cat_table.reserve(std::max(p_cdb->cats._List._Mysize, P.save_explorer_cats.size()));
        auto head = p_cdb->cats._List._Myhead;
        auto current = head->_Next;
        // copy from the game's map
        while(current != head) {
            unified_cat_table.try_emplace(current->_Myval.sql_key, current->_Myval.cat);
            current = current->_Next;
        }
    } else {
        unified_cat_table.reserve(P.save_explorer_cats.size());
    }

    // conCATenate* entries from the analyzer's map
    // (we're all about amewsing puns here, frfr, no carp)
    for(auto &kv : P.save_explorer_cats) {
        unified_cat_table.try_emplace(kv.first, kv.second.get());
    }
    // *Pedancat note: technically just a meowrge now. As one may furriously note, conCATenation describes
    // the joining of two ordered sequences, but hashmaps are intrinsically unordered. A previous revision of
    // this function joined a list of elements to the tail of a vector, chased shortly by a sort.
    // We apologize for any purrcieved technical slights, but not for pawwful use of language.

    return unified_cat_table;
}

struct PedigreeIndex {
    std::unordered_map<int64_t, std::set<int64_t>> children_table;
    std::unordered_map<int64_t, std::unordered_map<int64_t, size_t>> mate_table;
};

PedigreeIndex build_pedigree_index() {
    MewDirector *p_md = get_p_mewdirector_singleton();
    std::unordered_map<int64_t, std::set<int64_t>> children_table;
    std::unordered_map<int64_t, std::unordered_map<int64_t, size_t>> mate_table;
    if(p_md != nullptr) {
        auto &map = p_md->cats->pedigree.child_to_parents_and_coi_map;
        for(size_t i = 0; i < map.cap; i++) {
            if(map.ctrl[i] <= 0x7F) {
                auto entry = map.slots[i];
                // operations on the children table are not redundant, but we use a set anyway
                // we intentionally do not exclude the null key (-1)
                // stray cats and the two starter cats are children of the null cat
                // sibling calculations should exclude treating the null cat as a valid (step)parent
                auto &parent_a_child_set = children_table[entry.val.parent_a]; // will retrieve or create on lookup
                parent_a_child_set.insert(entry.key);
                auto &parent_b_child_set = children_table[entry.val.parent_b]; // will retrieve or create on lookup
                parent_b_child_set.insert(entry.key);

                // track number of children within the mate submap
                auto &parent_a_mates = mate_table[entry.val.parent_a]; // will retrieve or create on lookup
                auto &parent_a_cross_b_children_cnt = parent_a_mates[entry.val.parent_b]; // will retrieve or create on lookup
                parent_a_cross_b_children_cnt++;
                auto &parent_b_mates = mate_table[entry.val.parent_b]; // will retrieve or create on lookup
                auto &parent_b_cross_a_children_cnt = parent_b_mates[entry.val.parent_a]; // will retrieve or create on lookup
                parent_b_cross_a_children_cnt++;
            }
        }
        // TODO iterate through all cats and create connections for links not described in the pedigree table
        // for now, assume that the pedigree table is complete
    }
    return PedigreeIndex {children_table, mate_table};
}

std::vector<SqlKeyCatDataPair> derive_picker_table(std::unordered_map<int64_t, CatData *> unified_cat_table, std::string_view search_string, ImGuiTableSortSpecs *sortspecs) {
    // sort and filter the unified cat table by the provided specs

    auto filter_reject = [search_string](SqlKeyCatDataPair &cat) -> bool {
        // fast abort for empty search condition
        if(search_string.length() == 0) {
            return false;
        }
        // partial matching of numerals in the sql key
        std::string key_as_string = std::to_string(cat.sql_key);
        if(std::search(key_as_string.begin(), key_as_string.end(), search_string.begin(), search_string.end()) != key_as_string.end()) {
            return false;
        }
        // case-insensitive search of substrings in name
        std::string cat_name = convert_utf16_wstring_to_utf8_string(cat.cat->name);
        if(std::search(cat_name.begin(), cat_name.end(),
            search_string.begin(), search_string.end(),
            [](char a, char b) { return std::tolower(a) == std::tolower(b); }
        ) != cat_name.end()) {
            return false;
        }
        return true;
    };

    // filter by search criteria
    std::vector<SqlKeyCatDataPair> picker_table;
    for(auto &kv : unified_cat_table) {
        SqlKeyCatDataPair pair = {kv.first, kv.second};
        if(!filter_reject(pair)) {
            picker_table.push_back(pair);
        }
    }

    // sort by the requested display specification
    // TODO should factor the decision tree out of the lambda
    std::sort(picker_table.begin(), picker_table.end(), [sortspecs](auto &a, auto &b) {
        if(sortspecs != nullptr && sortspecs->SpecsCount == 1) {
            if(sortspecs->Specs[0].ColumnIndex == 0) {
                // ID
                if(sortspecs->Specs[0].SortDirection == ImGuiSortDirection_Ascending) {
                    return a.sql_key < b.sql_key;
                } else {
                    return a.sql_key > b.sql_key;
                }
            } else {
                if(sortspecs->Specs[0].SortDirection == ImGuiSortDirection_Ascending) {
                    return a.cat->name.as_native_wstring_view() < b.cat->name.as_native_wstring_view();
                } else {
                    return a.cat->name.as_native_wstring_view() > b.cat->name.as_native_wstring_view();
                }
            }
        }
        return a.sql_key < b.sql_key;
    });

    return picker_table;
}

void show_feline_therapist_window() {
    if(!P.show_feline_therapist) {
        return;
    }

    static Ringbuffer<int64_t, true> navigation_history(64);

    int64_t picker_table_selected_sql_id = navigation_history.undo_peek().value_or(-1);

    // TODO don't recompute this map every frame
    std::unordered_map<int64_t, CatData *> unified_cat_table = build_unified_cat_table();

    std::string navigation_cat_desc;
    CatData *picked_cat = nullptr;
    if(auto selected_cat = unified_cat_table.find(picker_table_selected_sql_id); selected_cat != unified_cat_table.end()) {
        navigation_cat_desc = std::format("{} ({})", convert_utf16_wstring_to_utf8_string(selected_cat->second->name), selected_cat->first);
        picked_cat = selected_cat->second;
    } else {
        navigation_cat_desc = "No selection";
    }

    // need to reload cats at end of fn to avoid prematurely invalidating picked_cat
    // what we get for playing loose with unique_ptr borrows
    bool reload_cats = false;

    ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
    ImGui::SetNextWindowSize(ImVec2(viewport_size.x * 0.4f, viewport_size.y * 0.4f), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Feline Therapist (Picker)", &P.show_feline_therapist)) {
        static std::string searchbox;
        // Navigation bar
        if(navigation_history.undo_can_step_backward()) {
            if(ImGui::Button("<")) {
                navigation_history.undo_step_backward();
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::Button("<");
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if(navigation_history.undo_can_step_forward()) {
            if(ImGui::Button(">")) {
                navigation_history.undo_step_forward();
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::Button(">");
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if(ImGui::Button("Load all cats")) {
            reload_cats = true;
        }
        ImGui::SameLine();
        ImguiTextStdFmt("{}", navigation_cat_desc);

        ImGui::InputTextWithHint("##searchbox", "Search", &searchbox, 0, nullptr, nullptr);
        if(ImGui::BeginTable("picker_table", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableHeadersRow();

            std::vector<SqlKeyCatDataPair> picker_table;
            if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
                // if (sort_specs->SpecsDirty) {} // TODO don't apply a list filter every frame
                picker_table = derive_picker_table(unified_cat_table, searchbox, sort_specs);
                sort_specs->SpecsDirty = false;
            } else {
                picker_table = derive_picker_table(unified_cat_table, searchbox, nullptr);
            }
            for(auto &kv : picker_table) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if(ImGui::Selectable(std::format("{}", kv.sql_key).c_str(), picker_table_selected_sql_id == kv.sql_key, ImGuiSelectableFlags_SpanAllColumns)) {
                    if(picker_table_selected_sql_id != kv.sql_key) {
                        navigation_history.push(kv.sql_key);
                    }
                }
                ImGui::TableNextColumn();
                ImguiTextStdFmt("{}", convert_utf16_wstring_to_utf8_string(kv.cat->name));
            }

            ImGui::EndTable();
        }
    }
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(viewport_size.x * 0.4f, viewport_size.y * 0.4f), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Feline Therapist (Inspector)", &P.show_feline_therapist)) {
        // Navigation bar
        if(navigation_history.undo_can_step_backward()) {
            if(ImGui::Button("<")) {
                navigation_history.undo_step_backward();
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::Button("<");
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if(navigation_history.undo_can_step_forward()) {
            if(ImGui::Button(">")) {
                navigation_history.undo_step_forward();
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::Button(">");
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if(ImGui::Button("Load all cats")) {
            reload_cats = true;
        }
        ImGui::SameLine();
        ImguiTextStdFmt("{}", navigation_cat_desc);

        if(picked_cat != nullptr) {
            show_cat(*picked_cat);
        }
    }
    ImGui::End();

    if(P.enable_experimental_widgets) {
        ImGui::SetNextWindowSize(ImVec2(viewport_size.x * 0.4f, viewport_size.y * 0.4f), ImGuiCond_FirstUseEver);
        if(ImGui::Begin("Feline Therapist ([EXPERIMENTAL] Editor)", &P.show_feline_therapist)) {
            // Navigation bar
            if(navigation_history.undo_can_step_backward()) {
                if(ImGui::Button("<")) {
                    navigation_history.undo_step_backward();
                }
            } else {
                ImGui::BeginDisabled();
                ImGui::Button("<");
                ImGui::EndDisabled();
            }
            ImGui::SameLine();
            if(navigation_history.undo_can_step_forward()) {
                if(ImGui::Button(">")) {
                    navigation_history.undo_step_forward();
                }
            } else {
                ImGui::BeginDisabled();
                ImGui::Button(">");
                ImGui::EndDisabled();
            }
            ImGui::SameLine();
            if(ImGui::Button("Load all cats")) {
                reload_cats = true;
            }
            ImGui::SameLine();
            ImguiTextStdFmt("{}", navigation_cat_desc);

            ImguiTextStdFmt("Warning: this tool allows you to edit cats in ways that would crash your game or break your save!");
            ImguiTextStdFmt("It is not meant to be a save editor or game trainer!");
            ImguiTextStdFmt("(please only use this on a throwaway save!)");

            if(picked_cat != nullptr) {
                edit_cat(*picked_cat);
            }
        }
        ImGui::End();
    }

    ImGui::SetNextWindowSize(ImVec2(viewport_size.x * 0.4f, viewport_size.y * 0.4f), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Feline Therapist (Relationships)", &P.show_feline_therapist)) {
        // Navigation bar
        if(navigation_history.undo_can_step_backward()) {
            if(ImGui::Button("<")) {
                navigation_history.undo_step_backward();
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::Button("<");
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if(navigation_history.undo_can_step_forward()) {
            if(ImGui::Button(">")) {
                navigation_history.undo_step_forward();
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::Button(">");
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if(ImGui::Button("Load all cats")) {
            reload_cats = true;
        }
        ImGui::SameLine();
        ImguiTextStdFmt("{}", navigation_cat_desc);

        if(picked_cat != nullptr) {
            MewDirector *p_md = get_p_mewdirector_singleton();
            if(p_md != nullptr) {
                // TODO yet another all-cats operation executed every frame
                PedigreeIndex pedigree_index = build_pedigree_index();

                CatDatabase *p_cdb = p_md->cats;

                if(ImGui::TreeNode("Parents")) {
                    auto parents = p_cdb->pedigree.child_to_parents_and_coi_map.get(&picker_table_selected_sql_id);
                    if(parents != nullptr) {
                        // absolutely disgusting how much code it takes to dereference a map
                        if(auto it = unified_cat_table.find(parents->val.parent_a); it != unified_cat_table.end()) {
                            if(ImGui::TextLink(std::format("Parent A (Dad): {} ({})", convert_utf16_wstring_to_utf8_string(it->second->name), it->first).c_str())) {
                                if(picker_table_selected_sql_id != it->first) {
                                    navigation_history.push(it->first);
                                }
                            }
                        } else {
                            ImguiTextStdFmt("Parent A (Dad): ??? ({})", parents->val.parent_a);
                        }

                        if(auto it = unified_cat_table.find(parents->val.parent_b); it != unified_cat_table.end()) {
                            if(ImGui::TextLink(std::format("Parent B (Mom): {} ({})", convert_utf16_wstring_to_utf8_string(it->second->name), it->first).c_str())) {
                                if(picker_table_selected_sql_id != it->first) {
                                    navigation_history.push(it->first);
                                }
                            }
                        } else {
                            ImguiTextStdFmt("Parent B (Mom): ??? ({})", parents->val.parent_b);
                        }
                    }
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Significant others")) {
                    if(auto it = unified_cat_table.find(picked_cat->lover_sql_key); it != unified_cat_table.end()) {
                        if(ImGui::TextLink(std::format("Lover: {} ({})", convert_utf16_wstring_to_utf8_string(it->second->name), it->first).c_str())) {
                            if(picker_table_selected_sql_id != it->first) {
                                navigation_history.push(it->first);
                            }
                        }
                    } else {
                        ImguiTextStdFmt("Lover: ??? ({})", picked_cat->lover_sql_key);
                    }

                    if(auto it = unified_cat_table.find(picked_cat->hater_sql_key); it != unified_cat_table.end()) {
                        if(ImGui::TextLink(std::format("Rival: {} ({})", convert_utf16_wstring_to_utf8_string(it->second->name), it->first).c_str())) {
                            if(picker_table_selected_sql_id != it->first) {
                                navigation_history.push(it->first);
                            }
                        }
                    } else {
                        ImguiTextStdFmt("Rival: ??? ({})", picked_cat->hater_sql_key);
                    }
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Siblings")) {
                    // and stepsiblings
                    auto parents = p_cdb->pedigree.child_to_parents_and_coi_map.get(&picker_table_selected_sql_id);
                    std::set<int64_t> empty;
                    std::set<int64_t> *p_a_children = &empty;
                    std::set<int64_t> *p_b_children = &empty;
                    if(parents != nullptr) {
                        if(parents->val.parent_a != -1) {
                            if(auto it = pedigree_index.children_table.find(parents->val.parent_a); it != pedigree_index.children_table.end()) {
                                p_a_children = &it->second;
                            }
                        }
                        if(parents->val.parent_b != -1) {
                            if(auto it = pedigree_index.children_table.find(parents->val.parent_b); it != pedigree_index.children_table.end()) {
                                p_b_children = &it->second;
                            }
                        }
                    }
                    auto sibling_line = [&](int64_t child_id, std::string relationship) {
                        if(auto child_catdata = unified_cat_table.find(child_id); child_catdata != unified_cat_table.end()) {
                            if(ImGui::TextLink(std::format("{} ({}) - {}", convert_utf16_wstring_to_utf8_string(child_catdata->second->name), child_catdata->first, relationship).c_str())) {
                                if(picker_table_selected_sql_id != child_catdata->first) {
                                    navigation_history.push(child_catdata->first);
                                }
                            }
                        } else {
                            ImguiTextStdFmt("??? ({}) - {}", child_id, relationship);
                        }
                    };
                    for(auto sibling : *p_a_children) {
                        if(sibling != picker_table_selected_sql_id && p_b_children->contains(sibling)) {
                            sibling_line(sibling, "sibling");
                        }
                    }
                    for(auto sibling : *p_a_children) {
                        if(sibling != picker_table_selected_sql_id && !p_b_children->contains(sibling)) {
                            sibling_line(sibling, "A-stepsibling");
                        }
                    }
                    for(auto sibling : *p_b_children) {
                        if(sibling != picker_table_selected_sql_id && !p_a_children->contains(sibling)) {
                            sibling_line(sibling, "B-stepsibling");
                        }
                    }
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Mates")) {
                    if(auto my_mates = pedigree_index.mate_table.find(picker_table_selected_sql_id); my_mates != pedigree_index.mate_table.end()) {
                        for(auto mate : my_mates->second) {
                            if(auto mate_catdata = unified_cat_table.find(mate.first); mate_catdata != unified_cat_table.end()) {
                                if(ImGui::TextLink(std::format("{} ({}) - {} children", convert_utf16_wstring_to_utf8_string(mate_catdata->second->name), mate_catdata->first, mate.second).c_str())) {
                                    if(picker_table_selected_sql_id != mate_catdata->first) {
                                        navigation_history.push(mate_catdata->first);
                                    }
                                }
                            } else {
                                ImguiTextStdFmt("??? ({}) - {} children", mate.first, mate.second);
                            }
                        }
                    }
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Children")) {
                    if(auto my_beloved_kittens = pedigree_index.children_table.find(picker_table_selected_sql_id); my_beloved_kittens != pedigree_index.children_table.end()) {
                        for(auto child_id : my_beloved_kittens->second) {
                            if(auto child_catdata = unified_cat_table.find(child_id); child_catdata != unified_cat_table.end()) {
                                if(ImGui::TextLink(std::format("{} ({})", convert_utf16_wstring_to_utf8_string(child_catdata->second->name), child_catdata->first).c_str())) {
                                    if(picker_table_selected_sql_id != child_catdata->first) {
                                        navigation_history.push(child_catdata->first);
                                    }
                                }
                            } else {
                                ImguiTextStdFmt("??? ({})", child_id);
                            }
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }
    }

    if(reload_cats) {
        P.save_explorer_cats = load_all_cats();
    }

    ImGui::End();
}

void show_save_explorer_window() {
    if(!P.show_save_explorer) {
        return;
    }
    ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
    ImGui::SetNextWindowSize(ImVec2(viewport_size.x * 0.4f, viewport_size.y * 0.4f), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Save explorer", &P.show_save_explorer)) {
        if(ImGui::TreeNode("Cats")) {
            if(ImGui::Button("Load from save")) {
                P.save_explorer_cats = load_all_cats();
            }
            if(ImGui::Button("Clear")) {
                std::unordered_map<int64_t, ManagedCatData>().swap(P.save_explorer_cats);
            }
            for(auto &kv: P.save_explorer_cats) {
                if(ImGui::TreeNode(std::format("{} ({})", convert_utf16_wstring_to_utf8_string(kv.second->name), kv.first).c_str())) {
                    show_cat(*kv.second);
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        if(P.enable_experimental_widgets && ImGui::TreeNode("[EXPERIMENTAL] Catworks")) {
            ImguiTextStdFmt("These tools call odd bits of code from within the game's engine.");
            ImguiTextStdFmt("Consider only playing with them in a throwaway save!");
            static Xoshiro256pContext our_prng_state = {1, 0, 0, 0};
            if(ImGui::TreeNode("Analysis RNG state")) {
                if(ImGui::Button("Copy current game RNG state")) {
                    Xoshiro256pContext &rng = get_xoshiro256p_rng_context();
                    our_prng_state = rng;
                }
                ImGui::InputScalar("RNG context 0", ImGuiDataType_U64, &our_prng_state.ctx[0], nullptr, nullptr, "%016llx");
                ImGui::InputScalar("RNG context 1", ImGuiDataType_U64, &our_prng_state.ctx[1], nullptr, nullptr, "%016llx");
                ImGui::InputScalar("RNG context 2", ImGuiDataType_U64, &our_prng_state.ctx[2], nullptr, nullptr, "%016llx");
                ImGui::InputScalar("RNG context 3", ImGuiDataType_U64, &our_prng_state.ctx[3], nullptr, nullptr, "%016llx");
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("Stray CatData generator")) {
                static ManagedCatData random_cat;
                if(ImGui::Button("Roll a stray!")) {
                    random_cat = make_stray(&our_prng_state);
                }
                if(random_cat != nullptr) {
                    show_cat(*random_cat);
                }
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("Kitten CatData generator")) {
                ImguiTextStdFmt("To use this generator, you must first load your save's cats under the 'Cats' section.");
                static int64_t parent_a_key;
                static int64_t parent_b_key;
                static double coi;
                static ManagedCatData kitten;
                static bool autocalculate_coi = true;
                ImGui::InputScalar("Parent A", ImGuiDataType_S64, &parent_a_key);
                ImGui::InputScalar("Parent B", ImGuiDataType_S64, &parent_b_key);
                ImGui::Checkbox("Auto-calculate COI", &autocalculate_coi);
                double selected_coi;
                if(autocalculate_coi) {
                    ImGui::BeginDisabled();
                    double calculated_coi = calculate_coi(parent_a_key, parent_b_key);
                    ImGui::InputDouble("COI override", &calculated_coi);
                    ImGui::EndDisabled();
                    selected_coi = calculated_coi;
                } else {
                    ImGui::InputDouble("COI override", &coi);
                    if(coi < 0.0) {
                        coi = 0.0;
                    } else if(coi > 1.0) {
                        coi = 1.0;
                    }
                    selected_coi = coi;
                }
                if(ImGui::Button("Make a kitten!")) {
                    auto parent_a = P.save_explorer_cats.find(parent_a_key);
                    auto parent_b = P.save_explorer_cats.find(parent_b_key);
                    if(parent_a != P.save_explorer_cats.end() && parent_b != P.save_explorer_cats.end()) {
                        kitten = make_kitten(parent_a->second.get(), parent_b->second.get(), selected_coi, &our_prng_state);
                    }
                }
                if(kitten != nullptr) {
                    show_cat(*kitten);
                }
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("Stray spawner")) {
                ImguiTextStdFmt("Warning: These buttons will add new cats to the save.");
                ImguiTextStdFmt("Pressing them away from the house will crash the game!");
                if(ImGui::Button("Spawn a stray!")) {
                    spawn_stray_at_house([](CatData *) -> void {});
                }
                if(ImGui::Button("Spawn a gigacat!")) {
                    spawn_stray_at_house([](CatData *cat) -> void {
                        cat->body_parts.pitch = 0.5;
                        cat->body_parts.head.part_sprite_idx = 10;
                        cat->stats_heritable.cha = 10;
                        cat->name.destroy();
                        cat->name.construct(L"gigacat");
                    });
                }
                static int64_t cat_sql_key_to_clone;
                ImGui::InputScalar("Cat to clone (SQL key)", ImGuiDataType_S64, &cat_sql_key_to_clone);
                if(ImGui::Button("Spawn a clone!")) {
                    spawn_stray_at_house([](CatData *cat) -> void {
                        overwrite_cat(cat, cat_sql_key_to_clone);
                    });
                }
                static int64_t cat_sql_key_to_despawn;
                ImGui::InputScalar("Cat to despawn (SQL key)", ImGuiDataType_S64, &cat_sql_key_to_despawn);
                if(ImGui::Button("Despawn!")) {
                    despawn_housecat(cat_sql_key_to_despawn);
                }
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("Teleporter")) {
                if(ImGui::Button("Fishing minigame?")) {
                    MsvcReleaseModeXString scene_name = {};
                    scene_name.construct("MiniGame");
                    Scene *scene = maybe_Director_create_Scene(get_p_mewdirector_singleton()->director, &scene_name);
                    scene_name.destroy();
                    if(scene->doing_scene_destruction) {
                        //break
                    }
                    glaiel__Scene__CreateComponent_FishingMinigameScene(scene, glaiel__Scene__CreateEntity(scene));
                }
                if(ImGui::Button("Destroy MiniGame")) {
                    MsvcReleaseModeXString scene_name = {};
                    scene_name.construct("MiniGame");
                    glaiel__Director__DestroyScene(get_p_mewdirector_singleton()->director, &scene_name);
                    scene_name.destroy();
                }

                if(ImGui::Button("Flappy Cat")) {
                    create_flappy_cat_scene_scene();
                }
                if(ImGui::Button("Destroy Flappy Cat")) {
                    MsvcReleaseModeXString scene_name = {};
                    scene_name.construct("polymeric.amoeba.FlappyCat");
                    glaiel__Director__DestroyScene(get_p_mewdirector_singleton()->director, &scene_name);
                    scene_name.destroy();
                }

                ImGui::InputScalar("cat_jump_v_y", ImGuiDataType_Double, &G.cat_jump_v_y);
                ImGui::InputScalar("small_g", ImGuiDataType_Double, &G.small_g);
                ImGui::InputScalar("up_rotv", ImGuiDataType_Double, &G.up_rotv);
                ImGui::InputScalar("down_rotv", ImGuiDataType_Double, &G.down_rotv);
                ImGui::InputScalar("bob_freq", ImGuiDataType_Double, &G.bob_freq);
                ImGui::InputScalar("bob_amplitude", ImGuiDataType_Double, &G.bob_amplitude);
                ImGui::InputScalar("pipe_scroll_speed", ImGuiDataType_Double, &G.pipe_scroll_speed);
                ImGui::InputScalar("pipe_spawn_interval", ImGuiDataType_Double, &G.pipe_spawn_interval);
                ImGui::InputScalar("pipe_shift_dist_amp_half", ImGuiDataType_Double, &G.pipe_shift_dist_amp_half);
                ImGui::InputScalar("pipe_gap_height_half", ImGuiDataType_Double, &G.pipe_gap_height_half);
                ImGui::Checkbox("nyan_cat_mode", &G.nyan_cat_mode);
                if(ImGui::Button("Jump")) {
                    G.cat_jump = true;
                }

                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void show_tlog_config_window() {
    if(!P.show_tlog_config) {
        return;
    }
    ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
    ImGui::SetNextWindowSize(ImVec2(viewport_size.x * 0.4f, viewport_size.y * 0.4f), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Transaction logger", &P.show_tlog_config)) {
        static std::string file_path = "amoeba.tlog.lz4";
        bool enable_transaction_logging = G.tlogger.is_opened();
        if(enable_transaction_logging) {
            ImGui::BeginDisabled();
        }
        ImGui::InputText("File path", &file_path, 0, nullptr, nullptr);
        if(enable_transaction_logging) {
            ImGui::EndDisabled();
        }
        ImGui::Checkbox("Enable transaction logging", &enable_transaction_logging);
        if(G.tlogger.is_opened() && !enable_transaction_logging) {
            G.tlogger.reset();
            G.tlogger.close();
        } else if(!G.tlogger.is_opened() && enable_transaction_logging) {
            // Open the transaction logger's backing file for write
            G.tlogger.open(std::filesystem::path(convert_utf8_string_to_utf16_wstring(file_path)), true);
            // Write a schema hint to the meta channel
            G.tlogger.select_vsid(TlogVsid::Meta);
            G.tlogger.set_timestamp_now();
            G.tlogger.write_int64(TLOG_SCHEMA_VERSION_HINT);
        }
        if(ImGui::Button("Flush to disk")) {
            G.tlogger.flush();
        }
    }
    ImGui::End();
}

void deinitialize_imgui() {
    if(P.initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        lua_close(P.repl_state);
    }
}

MAKE_PHOOK(1, "SDL_GL_SwapWindow",
    bool, __cdecl, SDL_GL_SwapWindow,
    SDL_Window *window
) {
    // multi-viewport support code will call SDL_GL_SwapWindow to paint onto other windows
    // if so, don't attempt to inject imgui again
    if(P.swapwindow_hook_nested_call_guard) {
        return SDL_GL_SwapWindow_hook.orig(window);
    }

    if(!P.initialized) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        // multi-viewport works, but is glitchy with fullscreen (dragging to edge of window will cause flickering)
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui_ImplSDL3_InitForOpenGL(window, SDL_GL_GetCurrentContext());
        ImGui_ImplOpenGL3_Init();

        // This file is already 2.3 kLOCs and now it also instantiates a Lua interpreter context!
        P.repl_state = luaL_newstate();
        luaL_openlibs(P.repl_state);
        lua_pushcfunction(P.repl_state, [](lua_State *state) -> int {
            std::string line;
            int n_args = lua_gettop(state);
            for(int i = 1; i <= n_args; i++) {
                size_t str_len;
                const char *str = luaL_tolstring(state, i, &str_len);
                line.append(str, str_len);
                if(i < n_args) {
                    line += "\t";
                }
                lua_pop(state, 1);
            }
            P.repl_lines.push(line);
            return 0;
        });
        lua_setglobal(P.repl_state, "print");
        auto make_jf_read_lua = [&]<typename T>(std::string name) {
            lua_pushcfunction(P.repl_state, [](lua_State *state) -> int {
                void *addr = reinterpret_cast<void *>(luaL_checkinteger(state, 1));
                T read_value;
                if(!jf_read<T>(addr, &read_value)) {
                    luaL_error(state, "%s", std::format("cannot dereference: {:p}", addr).c_str());
                }
                lua_pushinteger(state, read_value);
                return 1;
            });
            lua_setglobal(P.repl_state, name.c_str());
        };
        make_jf_read_lua.operator()<uint8_t>("jf_read_u8");
        make_jf_read_lua.operator()<uint16_t>("jf_read_u16");
        make_jf_read_lua.operator()<uint32_t>("jf_read_u32");
        make_jf_read_lua.operator()<uint64_t>("jf_read_u64");
        make_jf_read_lua.operator()<int8_t>("jf_read_i8");
        make_jf_read_lua.operator()<int16_t>("jf_read_i16");
        make_jf_read_lua.operator()<int32_t>("jf_read_i32");
        make_jf_read_lua.operator()<int64_t>("jf_read_i64");

        P.initialized = true;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    show_load_exception_modals_if_needed();
    if(!P.hide_all) {
        show_main_menu_bar();
        if(P.show_imgui_demo) {
            ImGui::ShowDemoWindow(&P.show_imgui_demo);
        }
        show_feline_therapist_window();
        show_data_explorer_window();
        show_save_explorer_window();
        show_debug_console_window();
        show_lua_repl_window();
        show_tlog_config_window();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        P.swapwindow_hook_nested_call_guard = true;
        ImGui::RenderPlatformWindowsDefault();
        P.swapwindow_hook_nested_call_guard = false;
        SDL_GL_MakeCurrent(window, backup_current_context);
    }

    bool result = SDL_GL_SwapWindow_hook.orig(window);

    // make sure to call the original function before initiating dll eject
    // (Detours will repoint hook->orig to hook->target, but we don't want to depend on that)
    // (we want to exit immediately from the dll after starting eject)
    if(P.request_dll_eject) {
        P.request_dll_eject = false;
        initiate_dll_eject();
    }

    return result;
}

MAKE_PHOOK(1, "SDL_PollEvent",
    bool, __cdecl, SDL_PollEvent,
    SDL_Event *event
) {
    if(P.initialized) {
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
            switch (event->type) {
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
                        // Left Alt isn't used by the game by default, use it to toggle imgui
                        if(event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_LALT) {
                            P.hide_all ^= true;
                        }
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
