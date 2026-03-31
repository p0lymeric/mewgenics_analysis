#include "amoeba.hpp"
#include "types/glaiel.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/function_hook.hpp"
#include "utilities/strings.hpp"
#include "utilities/memory.hpp"
#include "ffi/cat_factory.hpp"

#include "SDL3/SDL.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include <cstring>
#include <list>
#include <set>
#include <unordered_map>
#include <utility>
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

    bool hide_all = false;
    bool enable_experimental_widgets = false;

    bool show_feline_therapist = false;
    bool show_data_explorer = false;
    bool show_save_explorer = false;
    bool show_debug_console = false;
    bool show_imgui_demo = false;

    // save explorer
    std::unordered_map<int64_t, ManagedCatData> save_explorer_cats;
};

static ImguiPrivateState P;

template<class... Args>
void ImguiTextStdFmt(std::format_string<Args...> fmt, Args&&... args) {
    std::string s = std::format(fmt, std::forward<Args>(args)...);
    ImGui::TextUnformatted(s.data(), s.data() + s.size());
}

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

void edit_cat(CatData &cat) {
    ImguiTextStdFmt("p_CatData: {:p}", reinterpret_cast<void *>(&cat));
    ImguiTextStdFmt("Name: {}", convert_utf16_wstring_to_utf8_string(cat.name));
    ImguiTextStdFmt("Nameplate symbol: {}", cat.nameplate_symbol);
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
    ImguiTextStdFmt("unknown 2: {}", cat.unknown_2);
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
    ImguiTextStdFmt("Voice: {}", cat.body_parts.voice);
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
    ImguiTextStdFmt("Last debuff: {}", cat.last_injury_debuffed_stat);
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
        for(auto p_mod = cat.campaign_stats.event_stat_modifiers._Myfirst; p_mod < cat.campaign_stats.event_stat_modifiers._Mylast; p_mod++) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{:p}", reinterpret_cast<void *>(p_mod));
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_mod->expression.SaveToStr(true));
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_mod->battles_remaining);
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
    ImguiTextStdFmt("Passive 0: {} {}", cat.passive_0, cat.passive_0_sidecar);
    ImguiTextStdFmt("Passive 1: {} {}", cat.passive_1, cat.passive_1_sidecar);
    ImguiTextStdFmt("Mutation 0: {} {}", cat.passive_2, cat.passive_2_sidecar);
    ImguiTextStdFmt("Mutation 1: {} {}", cat.passive_3, cat.passive_3_sidecar);
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
            ImguiTextStdFmt("{}", p_base[i].name);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_base[i].aux_string);
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
    ImguiTextStdFmt("unknown 17");
    ImguiTextStdFmt("unknown 17 size/capacity: {} {}", cat.unknown_17.size, cat.unknown_17.capacity);
    if(ImGui::BeginTable("unknown_17_table", 2)) {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableHeadersRow();
        for(uint32_t i = 0; i < cat.unknown_17.size; i++) {
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", i);
            ImGui::TableNextColumn();
            ImGui::InputScalar("##data", ImGuiDataType_U8, &cat.unknown_17.ptr[i]);
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
    ImGui::InputScalar("unknown_21", ImGuiDataType_U8, &cat.unknown_21);
    ImGui::InputScalar("num_visited_zones", ImGuiDataType_U8, &cat.num_visited_zones);
    ImGui::InputScalar("unknown_23", ImGuiDataType_U8, &cat.unknown_23);
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
        for(auto p_mod = cat.campaign_stats.event_stat_modifiers._Myfirst; p_mod < cat.campaign_stats.event_stat_modifiers._Mylast; p_mod++) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{:p}", reinterpret_cast<void *>(p_mod));
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_mod->expression.SaveToStr(true));
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", p_mod->battles_remaining);
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
    ImguiTextStdFmt("Passive 0: {} {}", cat.passive_0, cat.passive_0_sidecar);
    ImguiTextStdFmt("Passive 1: {} {}", cat.passive_1, cat.passive_1_sidecar);
    ImguiTextStdFmt("Mutation 0: {} {}", cat.passive_2, cat.passive_2_sidecar);
    ImguiTextStdFmt("Mutation 1: {} {}", cat.passive_3, cat.passive_3_sidecar);
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
    ImguiTextStdFmt("unknown 17");
    ImguiTextStdFmt("unknown 17 size/capacity: {} {}", cat.unknown_17.size, cat.unknown_17.capacity);
    if(ImGui::BeginTable("unknown_17_table", 2)) {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableHeadersRow();
        for(uint32_t i = 0; i < cat.unknown_17.size; i++) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", i);
            ImGui::TableNextColumn();
            ImguiTextStdFmt("{}", cat.unknown_17.ptr[i]);
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
    ImguiTextStdFmt("unknown 21: {}", cat.unknown_21);
    ImguiTextStdFmt("Num visited zones: {}", cat.num_visited_zones);
    ImguiTextStdFmt("unknown 23: {}", cat.unknown_23);
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
        MewDirector *p_md = *reinterpret_cast<MewDirector **>(ADDRESS_glaiel__MewDirector__p_singleton + G.host_exec_base_va);

        if(ImGui::TreeNode("Pointers")) {
            ImguiTextStdFmt("Hook base VA: {:p}", reinterpret_cast<void *>(G.dll_base_va));
            ImguiTextStdFmt("Executable base VA: {:p}", reinterpret_cast<void *>(G.host_exec_base_va));
            ImguiTextStdFmt("p_MewDirector: {:p}", static_cast<void *>(p_md));
            if(p_md != nullptr) {
                CatDatabase *p_cdb = p_md->cats;
                ImguiTextStdFmt("p_CatDatabase: {:p}", static_cast<void *>(p_cdb));
                ImguiTextStdFmt("p_SQLSaveFile: {:p}", static_cast<void *>(&p_md->sqlsavefile));
            }
            ImGui::TreePop();
        }

        if(ImGui::TreeNode("Thread-local storage")) {
            auto *p_tls = get_tls0_base<char>();
            ImguiTextStdFmt("TLS Slot 0 base VA: {:p}", reinterpret_cast<void *>(p_tls));
            Xoshiro256pContext *p_rng = reinterpret_cast<Xoshiro256pContext *>(p_tls + TLS0OFF_xoshiro256p_rng_context);
            for(int i = 0; i < 4; i++) {
                ImguiTextStdFmt("RNG context {}: 0x{:x}", i, p_rng->ctx[i]);
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
                        for(auto p_bucket = cats._Vec._Myfirst; p_bucket < cats._Vec._Mylast; p_bucket++) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{:02x}", i);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", reinterpret_cast<void *>(p_bucket->first));
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", reinterpret_cast<void *>(p_bucket->last));
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
                        for(auto p_bucket = cats._Vec._Myfirst; p_bucket < cats._Vec._Mylast; p_bucket++) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{:02x}", i);
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", reinterpret_cast<void *>(p_bucket->first));
                            ImGui::TableNextColumn();
                            ImguiTextStdFmt("{}", reinterpret_cast<void *>(p_bucket->last));
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
                    for(auto p_name = names._Myfirst; p_name < names._Mylast; p_name++) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImguiTextStdFmt("{}", convert_utf16_wstring_to_utf8_string(*p_name));
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

std::unordered_map<int64_t, CatData *> build_unified_cat_table() {
    // merge the two maps, taking game CatData instances over analyzer CatData, and sort by sql id
    std::unordered_map<int64_t, CatData *> unified_cat_table;

    MewDirector *p_md = *reinterpret_cast<MewDirector **>(ADDRESS_glaiel__MewDirector__p_singleton + G.host_exec_base_va);
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
    MewDirector *p_md = *reinterpret_cast<MewDirector **>(ADDRESS_glaiel__MewDirector__p_singleton + G.host_exec_base_va);
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

std::vector<SqlKeyCatDataPair> derive_picker_table(std::unordered_map<int64_t, CatData *> unified_cat_table, char *search_string, ImGuiTableSortSpecs *sortspecs) {
    // sort and filter the unified cat table by the provided specs

    size_t search_string_len = std::strlen(search_string);
    auto filter_reject = [search_string, search_string_len](SqlKeyCatDataPair &cat) -> bool {
        // fast abort for empty search condition
        if(search_string_len == 0) {
            return false;
        }
        // partial matching of numerals in the sql key
        std::string key_as_string = std::to_string(cat.sql_key);
        if(std::search(key_as_string.begin(), key_as_string.end(), search_string, search_string + search_string_len) != key_as_string.end()) {
            return false;
        }
        // case-insensitive search of substrings in name
        std::string cat_name = convert_utf16_wstring_to_utf8_string(cat.cat->name);
        if(std::search(cat_name.begin(), cat_name.end(),
            search_string, search_string + search_string_len,
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
        static char searchbox_buf[256];
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

        ImGui::InputTextWithHint("##searchbox", "Search", searchbox_buf, sizeof(searchbox_buf));
        if(ImGui::BeginTable("picker_table", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableHeadersRow();

            std::vector<SqlKeyCatDataPair> picker_table;
            if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
                // if (sort_specs->SpecsDirty) {} // TODO don't apply a list filter every frame
                picker_table = derive_picker_table(unified_cat_table, searchbox_buf, sort_specs);
                sort_specs->SpecsDirty = false;
            } else {
                picker_table = derive_picker_table(unified_cat_table, searchbox_buf, nullptr);
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
            MewDirector *p_md = *reinterpret_cast<MewDirector **>(ADDRESS_glaiel__MewDirector__p_singleton + G.host_exec_base_va);
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
                // if(ImGui::TreeNode("Siblings")) {
                //     // and stepsiblings
                //     ImGui::TreePop();
                // }
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
                    auto *p_tls = get_tls0_base<char>();
                    Xoshiro256pContext *p_rng = reinterpret_cast<Xoshiro256pContext *>(p_tls + TLS0OFF_xoshiro256p_rng_context);
                    our_prng_state = *p_rng;
                }
                ImGui::InputScalar("RNG context 0", ImGuiDataType_U64, &our_prng_state.ctx[0], nullptr, nullptr, "%016llx");
                ImGui::InputScalar("RNG context 1", ImGuiDataType_U64, &our_prng_state.ctx[1], nullptr, nullptr, "%016llx");
                ImGui::InputScalar("RNG context 2", ImGuiDataType_U64, &our_prng_state.ctx[2], nullptr, nullptr, "%016llx");
                ImGui::InputScalar("RNG context 3", ImGuiDataType_U64, &our_prng_state.ctx[3], nullptr, nullptr, "%016llx");
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("Stray generator")) {
                static ManagedCatData random_cat;
                if(ImGui::Button("Roll a stray!")) {
                    random_cat = make_stray(&our_prng_state);
                }
                if(random_cat != nullptr) {
                    show_cat(*random_cat);
                }
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("Kitten generator")) {
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
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void deinitialize_imgui() {
    if(P.initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
}

MAKE_PHOOK("SDL_GL_SwapWindow",
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
        P.initialized = true;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if(!P.hide_all) {
        show_main_menu_bar();
        if(P.show_imgui_demo) {
            ImGui::ShowDemoWindow(&P.show_imgui_demo);
        }
        show_feline_therapist_window();
        show_data_explorer_window();
        show_save_explorer_window();
        show_debug_console_window();
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

MAKE_PHOOK("SDL_PollEvent",
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
