#pragma once
#include "imgui.h"

namespace PewPew
{
    inline void SetupImGuiStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // ---------------------------------------------------------------------
        // Catppuccin Mocha (RGBA floats)
        // ---------------------------------------------------------------------
        const ImVec4 Base      = { 0.117f, 0.117f, 0.172f, 1.0f }; // #1e1e2e
        const ImVec4 Mantle    = { 0.109f, 0.109f, 0.156f, 1.0f }; // #181825

        const ImVec4 Surface0  = { 0.200f, 0.207f, 0.286f, 1.0f }; // #313244
        const ImVec4 Surface1  = { 0.247f, 0.254f, 0.337f, 1.0f }; // #3f4056
        const ImVec4 Surface2  = { 0.290f, 0.301f, 0.388f, 1.0f }; // #4a4d63

        const ImVec4 Overlay0  = { 0.396f, 0.403f, 0.486f, 1.0f }; // #65677c
        const ImVec4 Overlay2  = { 0.576f, 0.584f, 0.654f, 1.0f }; // #9399b2

        const ImVec4 Text      = { 0.803f, 0.815f, 0.878f, 1.0f }; // #cdd6f4
        const ImVec4 Subtext0  = { 0.639f, 0.658f, 0.764f, 1.0f }; // #a3a8c3

        const ImVec4 Mauve     = { 0.796f, 0.698f, 0.972f, 1.0f }; // #cba6f7
        const ImVec4 Peach     = { 0.980f, 0.709f, 0.572f, 1.0f }; // #fab387
        const ImVec4 Yellow    = { 0.980f, 0.913f, 0.596f, 1.0f }; // #f9e2af
        const ImVec4 Green     = { 0.650f, 0.890f, 0.631f, 1.0f }; // #a6e3a1
        const ImVec4 Teal      = { 0.580f, 0.886f, 0.819f, 1.0f }; // #94e2d5
        const ImVec4 Sapphire  = { 0.458f, 0.784f, 0.878f, 1.0f }; // #74c7ec
        const ImVec4 Blue      = { 0.533f, 0.698f, 0.976f, 1.0f }; // #89b4fa
        const ImVec4 Lavender  = { 0.709f, 0.764f, 0.980f, 1.0f }; // #b4befe

        // ---------------------------------------------------------------------
        // Colors
        // ---------------------------------------------------------------------

        // Text
        colors[ImGuiCol_Text]         = Text;
        colors[ImGuiCol_TextDisabled] = Subtext0;

        // Backgrounds
        colors[ImGuiCol_WindowBg] = Base;
        colors[ImGuiCol_ChildBg]  = Base;
        colors[ImGuiCol_PopupBg]  = Surface0;

        // Borders / separators
        colors[ImGuiCol_Border]       = Surface1;
        colors[ImGuiCol_BorderShadow] = { 0.0f, 0.0f, 0.0f, 0.0f };

        colors[ImGuiCol_Separator]        = Surface1;
        colors[ImGuiCol_SeparatorHovered] = Mauve;
        colors[ImGuiCol_SeparatorActive]  = Mauve;

        // Frames (inputs, etc.)
        colors[ImGuiCol_FrameBg]        = Surface0;
        colors[ImGuiCol_FrameBgHovered] = Surface1;
        colors[ImGuiCol_FrameBgActive]  = Surface2;

        // Titles / menubar
        colors[ImGuiCol_TitleBg]          = Mantle;
        colors[ImGuiCol_TitleBgActive]    = Surface0;
        colors[ImGuiCol_TitleBgCollapsed] = Mantle;
        colors[ImGuiCol_MenuBarBg]        = Mantle;

        // Scrollbar
        colors[ImGuiCol_ScrollbarBg]          = Surface0;
        colors[ImGuiCol_ScrollbarGrab]        = Surface2;
        colors[ImGuiCol_ScrollbarGrabHovered] = Overlay0;
        colors[ImGuiCol_ScrollbarGrabActive]  = Overlay2;

        // Widgets
        colors[ImGuiCol_CheckMark]        = Green;
        colors[ImGuiCol_SliderGrab]       = Sapphire;
        colors[ImGuiCol_SliderGrabActive] = Blue;

        // Buttons
        colors[ImGuiCol_Button]        = Surface0;
        colors[ImGuiCol_ButtonHovered] = Surface1;
        colors[ImGuiCol_ButtonActive]  = Surface2;

        // Headers (tree nodes, selectable, etc.)
        colors[ImGuiCol_Header]        = Surface0;
        colors[ImGuiCol_HeaderHovered] = Surface1;
        colors[ImGuiCol_HeaderActive]  = Surface2;

        // Resize grips
        colors[ImGuiCol_ResizeGrip]        = Surface2;
        colors[ImGuiCol_ResizeGripHovered] = Mauve;
        colors[ImGuiCol_ResizeGripActive]  = Mauve;

        // Tabs
        colors[ImGuiCol_Tab]                = Surface0;
        colors[ImGuiCol_TabHovered]         = Surface2;
        colors[ImGuiCol_TabActive]          = Surface1;
        colors[ImGuiCol_TabUnfocused]       = Surface0;
        colors[ImGuiCol_TabUnfocusedActive] = Surface1;

        // Docking
        colors[ImGuiCol_DockingPreview] = Sapphire;
        colors[ImGuiCol_DockingEmptyBg] = Base;

        // Plots
        colors[ImGuiCol_PlotLines]            = Blue;
        colors[ImGuiCol_PlotLinesHovered]     = Peach;
        colors[ImGuiCol_PlotHistogram]        = Teal;
        colors[ImGuiCol_PlotHistogramHovered] = Green;

        // Tables
        colors[ImGuiCol_TableHeaderBg]     = Surface0;
        colors[ImGuiCol_TableBorderStrong] = Surface1;
        colors[ImGuiCol_TableBorderLight]  = Surface0;
        colors[ImGuiCol_TableRowBg]        = { 0.0f, 0.0f, 0.0f, 0.0f };
        colors[ImGuiCol_TableRowBgAlt]     = { 1.0f, 1.0f, 1.0f, 0.06f };

        // Selection / drag-drop / navigation
        colors[ImGuiCol_TextSelectedBg]        = Surface2;
        colors[ImGuiCol_DragDropTarget]        = Yellow;
        colors[ImGuiCol_NavHighlight]          = Lavender;
        colors[ImGuiCol_NavWindowingHighlight] = { 1.0f, 1.0f, 1.0f, 0.7f };
        colors[ImGuiCol_NavWindowingDimBg]     = { 0.8f, 0.8f, 0.8f, 0.2f };
        colors[ImGuiCol_ModalWindowDimBg]      = { 0.0f, 0.0f, 0.0f, 0.35f };

        // ---------------------------------------------------------------------
        // Rounding
        // ---------------------------------------------------------------------
        style.WindowRounding    = 6.0f;
        style.ChildRounding     = 6.0f;
        style.FrameRounding     = 4.0f;
        style.PopupRounding     = 4.0f;
        style.ScrollbarRounding = 9.0f;
        style.GrabRounding      = 4.0f;
        style.TabRounding       = 4.0f;

        // ---------------------------------------------------------------------
        // Padding / spacing
        // ---------------------------------------------------------------------
        style.WindowPadding    = { 8.0f, 8.0f };
        style.FramePadding     = { 5.0f, 3.0f };
        style.ItemSpacing      = { 8.0f, 4.0f };
        style.ItemInnerSpacing = { 4.0f, 4.0f };
        style.IndentSpacing    = 21.0f;

        style.ScrollbarSize = 14.0f;
        style.GrabMinSize   = 10.0f;

        // ---------------------------------------------------------------------
        // Borders
        // ---------------------------------------------------------------------
        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize  = 1.0f;
        style.PopupBorderSize  = 1.0f;
        style.FrameBorderSize  = 0.0f;
        style.TabBorderSize    = 0.0f;
    }
}
