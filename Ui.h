#pragma once

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

struct TooltipState
{
    int   was_hovered;
    float tooltip_time;
};

struct TrendState
{
    float lastPct;
    int   direction;
};

struct AnimState
{
    float lastTarget;
};

struct StateStruct
{
    std::unordered_map<std::string, TrendState> ProgBarTrendState;
    std::unordered_map<std::string, AnimState>  ProgBarAnimState;

    TooltipState TooltipAnimationState;
};
extern StateStruct State;

class AnimatedNameplatesSettings
{
  public:
    enum HPBarStyle
    {
        HPBarStyle_SolidRed,
        HPBarStyle_ConColor,
        HPBarStyle_ColorRange
    };

    AnimatedNameplatesSettings() { LoadSettings(); }

    void SaveSettings();
    void LoadSettings();

    void SetShowBuffIcons(bool show)
    {
        m_showBuffIcons               = show;
        m_configNode["ShowBuffIcons"] = show;
        SaveSettings();
    }
    void SetPadding(const ImVec2& padding)
    {
        m_padding                = padding;
        m_configNode["PaddingX"] = padding.x;
        m_configNode["PaddingY"] = padding.y;
        SaveSettings();
    }
    void SetFontSize(float size)
    {
        m_fontSize               = size;
        m_configNode["FontSize"] = size;
        SaveSettings();
    }
    void SetIconSize(float size)
    {
        m_iconSize               = size;
        m_configNode["IconSize"] = size;
        SaveSettings();
    }
    void SetBarRounding(float rounding)
    {
        m_barRounding               = rounding;
        m_configNode["BarRounding"] = rounding;
        SaveSettings();
    }
    void SetBarBorderThickness(float thickness)
    {
        m_barBorderThickness               = thickness;
        m_configNode["BarBorderThickness"] = thickness;
        SaveSettings();
    }
    void SetShowDebugPanel(bool show)
    {
        m_showDebugPanel               = show;
        m_configNode["ShowDebugPanel"] = show;
        SaveSettings();
    }
    void SetRenderForSelf(bool show)
    {
        m_renderForSelf               = show;
        m_configNode["RenderForSelf"] = show;
        SaveSettings();
    }
    void SetRenderForTarget(bool show)
    {
        m_renderForTarget               = show;
        m_configNode["RenderForTarget"] = show;
        SaveSettings();
    }
    void SetRenderForGroup(bool show)
    {
        m_renderForGroup               = show;
        m_configNode["RenderForGroup"] = show;
        SaveSettings();
    }
    void SetRenderForAllHaters(bool show)
    {
        m_renderForAllHaters               = show;
        m_configNode["RenderForAllHaters"] = show;
        SaveSettings();
    }
    void SetRenderNoLOS(bool show)
    {
        m_renderNoLOS               = show;
        m_configNode["RenderNoLOS"] = show;
        SaveSettings();
    }
    void SetNameplateWidth(float width)
    {
        m_nameplateWidth               = width;
        m_configNode["NameplateWidth"] = width;
        SaveSettings();
    }
    void SetShowGuild(bool show)
    {
        m_showGuild               = show;
        m_configNode["ShowGuild"] = show;
        SaveSettings();
    }
    void SetShowPurpose(bool show)
    {
        m_showPurpose               = show;
        m_configNode["ShowPurpose"] = show;
        SaveSettings();
    }
    void SetRenderToForeground(bool show)
    {
        m_renderToForeground               = show;
        m_configNode["RenderToForeground"] = show;
        SaveSettings();
    }
    void SetShowClass(bool show)
    {
        m_showClass               = show;
        m_configNode["ShowClass"] = show;
        SaveSettings();
    }
    void SetShortClassName(bool show)
    {
        m_shortClassName               = show;
        m_configNode["ShortClassName"] = show;
        SaveSettings();
    }
    void SetShowLevel(bool show)
    {
        m_showLevel               = show;
        m_configNode["ShowLevel"] = show;
        SaveSettings();
    }
    void SetNameplateHeightOffset(float offset)
    {
        m_nameplateHeightOffset               = offset;
        m_configNode["NameplateHeightOffset"] = offset;
        SaveSettings();
    }
    void SetHPBarStyle(HPBarStyle style)
    {
        m_hpBarStyle               = style;
        m_configNode["HPBarStyle"] = static_cast<int>(style);
        SaveSettings();
    }

    bool          GetShowBuffIcons() const { return m_showBuffIcons; }
    const ImVec2& GetPadding() const { return m_padding; }
    float         GetFontSize() const { return m_fontSize; }
    float         GetIconSize() const { return m_iconSize; }
    float         GetBarRounding() const { return m_barRounding; }
    float         GetBarBorderThickness() const { return m_barBorderThickness; }
    bool          GetShowDebugPanel() const { return m_showDebugPanel; }
    bool          GetRenderForSelf() const { return m_renderForSelf; }
    bool          GetRenderForTarget() const { return m_renderForTarget; }
    bool          GetRenderForGroup() const { return m_renderForGroup; }
    bool          GetRenderForAllHaters() const { return m_renderForAllHaters; }
    bool          GetRenderNoLOS() const { return m_renderNoLOS; }
    float         GetNameplateWidth() const { return m_nameplateWidth; }
    bool          GetShowGuild() const { return m_showGuild; }
    bool          GetShowPurpose() const { return m_showPurpose; }
    bool          GetRenderToForeground() const { return m_renderToForeground; }
    bool          GetShowClass() const { return m_showClass; }
    bool          GetShortClassName() const { return m_shortClassName; }
    bool          GetShowLevel() const { return m_showLevel; }
    float         GetNameplateHeightOffset() const { return m_nameplateHeightOffset; }
    HPBarStyle    GetHPBarStyle() const { return m_hpBarStyle; }

  private:
    bool m_renderForSelf      = true;
    bool m_renderForTarget    = true;
    bool m_renderForGroup     = true;
    bool m_renderForAllHaters = true;
    bool m_showGuild          = false;
    bool m_showPurpose        = false;
    bool m_showLevel          = true;
    bool m_showClass          = true;
    bool m_shortClassName     = false;
    bool m_renderToForeground = false;
    bool m_renderNoLOS        = false;

    bool m_showBuffIcons  = true;
    bool m_showDebugPanel = false;

    ImVec2 m_padding        = ImVec2(8, 4);
    float  m_fontSize       = 20.0f;
    float  m_iconSize       = 20.0f;
    float  m_nameplateWidth = 500.0f;

    float m_nameplateHeightOffset = 35.0f;

    float m_barRounding        = 6.0f;
    float m_barBorderThickness = 2.5f;

    HPBarStyle m_hpBarStyle = HPBarStyle_ColorRange;

    std::string m_configFile = "MQAnimatedNameplates.yaml";
    YAML::Node  m_configNode;
};

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

extern AnimatedNameplatesSettings Settings;

void RenderNamePlateText(CursorState& cursor, ImU32 color, const char* text);
void AddRectFilledMultiColorRounded(ImDrawList& draw_list, const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left,
                                    ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left, float rounding,
                                    ImDrawFlags flags);

void RenderNamePlateRect(CursorState& cursor, const ImVec2& size, ImU32 color, float rounding, float thickness,
                         bool filled);
void DrawInspectableSpellIcon(CursorState& cursor, eqlib::EQ_Spell* pSpell);

void RenderAnimatedPercentage(CursorState& cursor, const std::string& id, const float barPct, const float height,
                              const float width, ImU32 colLow, ImU32 colMid, ImU32 colHigh, ImU32 colHighlight,
                              const std::string& label = "");

void RenderFancyHPBar(CursorState& cursor, const std::string& id, float hpPct, float height, float width,
                      ImU32 conColor, bool currentTarget, const std::string& label = "");

void RenderSettingsPanel();
bool AnimatedCheckbox(const std::string& label, bool* value);
bool AnimatedSlider(const std::string& label, float* slider_value, float slider_min, float slider_max,
                    const char* format = "%.2f", float labelWidthOverride = 0.0f);
bool AnimatedCombo(const std::string& label, int* value, std::vector<std::string> items);

ImDrawList* GetDrawList();

} // namespace Ui

struct AnimatedComboState
{
    bool  open      = false;
    float open_time = 0.0f;
};

struct CursorState
{
    ImVec2 CursorPos         = ImVec2(0, 0);
    ImVec2 LastCursorLinePos = ImVec2(0, 0);
    float  LineStartXPos     = 0.0f;

    explicit CursorState(const ImVec2& startingPos) { SetPos(startingPos + Ui::Settings.GetPadding()); }

    void SetPos(const ImVec2& pos)
    {
        CursorPos         = pos;
        LastCursorLinePos = pos;
        LineStartXPos     = pos.x;
    }

    const ImVec2& GetPos() const { return CursorPos; }

    void Move(const ImVec2& pos)
    {
        ImVec2 p = Ui::Settings.GetPadding() + pos;

        LastCursorLinePos.x = CursorPos.x + p.x;
        LastCursorLinePos.y = CursorPos.y;

        CursorPos.y += p.y;
        CursorPos.x = LineStartXPos;
    }

    void SameLine() { CursorPos = LastCursorLinePos; }

    void NewLine()
    {
        CursorPos.y += ImGui::GetTextLineHeight() + Ui::Settings.GetPadding().y;
        CursorPos.x = LineStartXPos;
    }
};
