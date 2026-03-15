#pragma once

#include "imgui.h"

struct CursorState
{
    ImVec2 CursorPos         = ImVec2(0, 0);
    ImVec2 LastCursorLinePos = ImVec2(0, 0);
    float  LineStartXPos     = 0.0f;

    explicit CursorState(const ImVec2& startingPos)
    {
        ImVec2 padding = ImGui::GetStyle().FramePadding;
        SetPos(startingPos + padding);
    }

    void SetPos(const ImVec2& pos)
    {
        CursorPos         = pos;
        LastCursorLinePos = pos;
        LineStartXPos     = pos.x;
    }

    const ImVec2& GetPos() const { return CursorPos; }

    void Move(const ImVec2& pos)
    {
        ImVec2 padding = ImGui::GetStyle().FramePadding;
        ImVec2 p       = padding + pos;

        LastCursorLinePos.x = CursorPos.x + p.x;
        LastCursorLinePos.y = CursorPos.y;

        CursorPos.y += p.y;
        CursorPos.x = LineStartXPos;
    }

    void SameLine() { CursorPos = LastCursorLinePos; }

    void NewLine()
    {
        ImVec2 padding = ImGui::GetStyle().FramePadding;
        CursorPos.y += ImGui::GetTextLineHeight() + padding.y;
        CursorPos.x = LineStartXPos;
    }
};