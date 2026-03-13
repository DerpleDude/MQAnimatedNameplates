#pragma once
#include "Config.h"
#include "ConfigVariable.h"

#include "imgui.h"
#include "imgui/imanim/im_anim.h"
#include "imgui_internal.h"

#include "mq/Plugin.h"
#include "mq/api/Textures.h"
#include <string>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

struct CursorState;

namespace eqlib
{
class EQ_Spell;
class PlayerClient;
} // namespace eqlib

namespace Ui
{
void RenderNamePlateText(CursorState& cursor, ImU32 color, const char* text);
void AddRectFilledMultiColorRounded(ImDrawList& draw_list, const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left,
                                    ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left, float rounding,
                                    ImDrawFlags flags);

void RenderNamePlateRect(CursorState& cursor, const ImVec2& size, ImU32 color, float rounding, float thickness,
                         bool filled);
void DrawInspectableSpellIcon(CursorState& cursor, eqlib::EQ_Spell* pSpell);

void RenderAnimatedPercentage(CursorState& cursor, const std::string& id, const float barPct, const float height,
                              const float width, ImU32 colLow, ImU32 colMid, ImU32 colHigh, ImU32 colHighlight,
                              const std::string& label = "", bool currentTarget = false);

void RenderFancyHPBar(CursorState& cursor, const std::string& id, float hpPct, float height, float width,
                      ImU32 conColor, bool currentTarget, const std::string& label = "",
                      HPBarStyle style = HPBarStyle::HPBarStyle_ColorRange);

void RenderSettingsPanel();
bool AnimatedCheckbox(const std::string& label, bool* value);
bool AnimatedSlider(const std::string& label, float* slider_value, float slider_min, float slider_max,
                    const char* format = "%.2f", float labelWidthOverride = 0.0f);
bool AnimatedCombo(const std::string& label, int* value, std::vector<std::string> items);

ImDrawList* GetDrawList();

class AnimatedCheckmark
{
  public:
    AnimatedCheckmark(bool value, ImGuiID path1Id, ImGuiID path2Id)
        : m_newValue(value), m_animIdPath1(path1Id), m_animIdPath2(path2Id)
    {
        Reset(value);
    }

    void Reset(bool newVal);
    void Render(ImDrawList* dl, const ImRect& check_bb, float box_size);

  private:
    ImGuiID m_animIdPath1;
    ImGuiID m_animIdPath2;

    float m_path1Time       = 0.0f;
    float m_path2Time       = 0.0f;
    bool  m_newValue        = false;
    bool  m_pathInitialized = false;
    bool  m_path1Complete   = false;
    bool  m_path2Complete   = false;
    float m_animSpeed       = 6.0f;
};

struct AnimatedTabState
{
    int                   idx;
    std::string           name;
    std::function<void()> content;
};

struct AnimatedComboState
{
    bool  open      = false;
    float open_time = 0.0f;
};

struct AnimatedTooltipState
{
    int   was_hovered;
    float tooltip_time;
};

struct AnimatedTrendState
{
    float lastPct;
    int   direction;
};

struct AnimationState
{
    float lastTarget;
};

struct ProgressBarStateStruct
{
    std::unordered_map<std::string, AnimatedTrendState> ProgBarTrendState;
    std::unordered_map<std::string, AnimationState>     ProgBarAnimState;

    AnimatedTooltipState TooltipAnimationState;
};
extern ProgressBarStateStruct ProgressBarState;

} // namespace Ui

struct CursorState
{
    ImVec2 CursorPos         = ImVec2(0, 0);
    ImVec2 LastCursorLinePos = ImVec2(0, 0);
    float  LineStartXPos     = 0.0f;

    explicit CursorState(const ImVec2& startingPos)
    {
        SetPos(startingPos + ImVec2(Ui::Config::Get().PaddingX, Ui::Config::Get().PaddingY));
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
        ImVec2 p = ImVec2(Ui::Config::Get().PaddingX, Ui::Config::Get().PaddingY) + pos;

        LastCursorLinePos.x = CursorPos.x + p.x;
        LastCursorLinePos.y = CursorPos.y;

        CursorPos.y += p.y;
        CursorPos.x = LineStartXPos;
    }

    void SameLine() { CursorPos = LastCursorLinePos; }

    void NewLine()
    {
        CursorPos.y += ImGui::GetTextLineHeight() + Ui::Config::Get().PaddingY;
        CursorPos.x = LineStartXPos;
    }
};
