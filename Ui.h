#pragma once

#include "imgui.h"

#include <yaml-cpp/yaml.h>
#include <unordered_map>
#include <string>

struct CursorState;

namespace eqlib
{
	class EQ_Spell;
	class PlayerClient;
}

namespace Ui {


struct TooltipState
{
	int was_hovered;
	float tooltip_time;
};

struct TrendState
{
	float lastPct;
	int direction;
};

struct AnimState
{
	float lastTarget;
};

struct StateStruct
{
	std::unordered_map<std::string, TrendState> ProgBarTrendState;
	std::unordered_map<std::string, AnimState> ProgBarAnimState;

	TooltipState TooltipAnimationState;
};
extern StateStruct State;


class AnimatedNameplatesSettings
{
public:
	AnimatedNameplatesSettings() { LoadSettings(); }

	void SaveSettings();
	void LoadSettings();

	void SetShowBuffIcons(bool show) { m_showBuffIcons = show; m_configNode["ShowBuffIcons"] = show; SaveSettings(); }
	void SetPadding(const ImVec2& padding) { m_padding = padding; m_configNode["PaddingX"] = padding.x; m_configNode["PaddingY"] = padding.y; SaveSettings(); }
	void SetFontSize(float size) { m_fontSize = size; m_configNode["FontSize"] = size; SaveSettings(); }
	void SetIconSize(float size) { m_iconSize = size; m_configNode["IconSize"] = size; SaveSettings(); }
	void SetBarRounding(float rounding) { m_barRounding = rounding; m_configNode["BarRounding"] = rounding; SaveSettings(); }
	void SetBarBorderThickness(float thickness) { m_barBorderThickness = thickness; m_configNode["BarBorderThickness"] = thickness; SaveSettings(); }
	void SetShowDebugPanel(bool show) { m_showDebugPanel = show; m_configNode["ShowDebugPanel"] = show; SaveSettings(); }
	void SetRenderForSelf(bool show) { m_renderForSelf = show; m_configNode["RenderForSelf"] = show; SaveSettings(); }
	void SetRenderForTarget(bool show) { m_renderForTarget = show; m_configNode["RenderForTarget"] = show; SaveSettings(); }
	void SetRenderForGroup(bool show) { m_renderForGroup = show; m_configNode["RenderForGroup"] = show; SaveSettings(); }
	void SetRenderForAllHaters(bool show) { m_renderForAllHaters = show; m_configNode["RenderForAllHaters"] = show; SaveSettings(); }
	void SetRenderNoLOS(bool show) { m_renderNoLOS = show; m_configNode["RenderNoLOS"] = show; SaveSettings(); }
	void SetNameplateWidth(float width) { m_nameplateWidth = width; m_configNode["NameplateWidth"] = width; SaveSettings(); }
	void SetShowGuild(bool show) { m_showGuild = show; m_configNode["ShowGuild"] = show; SaveSettings(); }
	void SetShowPurpose(bool show) { m_showPurpose = show; m_configNode["ShowPurpose"] = show; SaveSettings(); }
	void SetRenderToForeground(bool show) { m_renderToForeground = show; m_configNode["RenderToForeground"] = show; SaveSettings(); }

	bool GetShowBuffIcons() const { return m_showBuffIcons; }
	const ImVec2& GetPadding() const { return m_padding; }
	float GetFontSize() const { return m_fontSize; }
	float GetIconSize() const { return m_iconSize; }
	float GetBarRounding() const { return m_barRounding; }
	float GetBarBorderThickness() const { return m_barBorderThickness; }
	bool GetShowDebugPanel() const { return m_showDebugPanel; }
	bool GetRenderForSelf() const { return m_renderForSelf; }
	bool GetRenderForTarget() const { return m_renderForTarget; }
	bool GetRenderForGroup() const { return m_renderForGroup; }
	bool GetRenderForAllHaters() const { return m_renderForAllHaters; }
	bool GetRenderNoLOS() const { return m_renderNoLOS; }
	float GetNameplateWidth() const { return m_nameplateWidth; }
	bool GetShowGuild() const { return m_showGuild; }
	bool GetShowPurpose() const { return m_showPurpose; }
	bool GetRenderToForeground() const { return m_renderToForeground; }
	

private:

	bool m_renderForSelf = true;
	bool m_renderForTarget = true;
	bool m_renderForGroup = true;
	bool m_renderForAllHaters = true;
	bool m_showGuild = true;
	bool m_showPurpose = true;
	bool m_renderToForeground = true;
	bool m_renderNoLOS = false;

	bool m_showBuffIcons = true;
	bool m_showDebugPanel = false;

	ImVec2 m_padding = ImVec2(8, 4);
	float m_fontSize = 20.0f;
	float m_iconSize = 20.0f;
	float m_nameplateWidth = 500.0f;

	float m_barRounding = 6.0f;
	float m_barBorderThickness = 2.5f;

	std::string m_configFile = "MQAnimatedNameplates.yaml";
	YAML::Node m_configNode;
};

extern AnimatedNameplatesSettings Settings;

void RenderNamePlateText(CursorState& cursor, ImU32 color, const char* text);
void AddRectFilledMultiColorRounded(ImDrawList& draw_list, const ImVec2& p_min, const ImVec2& p_max,
	ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left, float rounding, ImDrawFlags flags);

void RenderNamePlateRect(CursorState& cursor, const ImVec2& size, ImU32 color, float rounding,
	float thickness, bool filled);
void DrawInspectableSpellIcon(CursorState& cursor, eqlib::EQ_Spell* pSpell);
void RenderAnimatedPercentage(CursorState& cursor, const std::string& id, float barPct, float height, float width,
	const ImVec4& colLow, const ImVec4& colMid, const ImVec4& colHigh, ImU32 colHighlight,
	const std::string& label = "");
void RenderFancyHPBar(CursorState& cursor, const std::string& id, float hpPct, float height, float width, ImU32 hpHighlight,
	const std::string& label = "");

void RenderSettingsPanel();

ImDrawList* GetDrawList();

} // namespace Ui

struct CursorState
{
	ImVec2 CursorPos = ImVec2(0, 0);
	ImVec2 LastCursorLinePos = ImVec2(0, 0);
	float LineStartXPos = 0.0f;

	explicit CursorState(const ImVec2& startingPos)
	{
		SetPos(startingPos + Ui::Settings.GetPadding());
	}

	void SetPos(const ImVec2& pos)
	{
		CursorPos = pos;
		LastCursorLinePos = pos;
		LineStartXPos = pos.x;
	}

	const ImVec2& GetPos() const
	{
		return CursorPos;
	}

	void Move(const ImVec2& pos)
	{
		ImVec2 p = Ui::Settings.GetPadding() + pos;

		LastCursorLinePos.x = CursorPos.x + p.x;
		LastCursorLinePos.y = CursorPos.y;

		CursorPos.y += p.y;
		CursorPos.x = LineStartXPos;
	}

	void SameLine()
	{
		CursorPos = LastCursorLinePos;
	}

	void NewLine()
	{
		CursorPos.y += ImGui::GetTextLineHeight() + Ui::Settings.GetPadding().y;
		CursorPos.x = LineStartXPos;
	}
};
