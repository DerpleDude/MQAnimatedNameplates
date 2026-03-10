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

class Ui
{
public:
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

	class AnimatedNameplatesSettings
	{
	public:
		AnimatedNameplatesSettings() { LoadSettings(); }

		void SaveSettings();
		void LoadSettings();

		void SetShowBuffIcons(bool show) { ShowBuffIcons = show; m_configNode["ShowBuffIcons"] = show; SaveSettings(); }
		void SetPadding(const ImVec2& padding) { Padding = padding; m_configNode["PaddingX"] = padding.x; m_configNode["PaddingY"] = padding.y; SaveSettings(); }
		void SetFontSize(float size) { FontSize = size; m_configNode["FontSize"] = size; SaveSettings(); }
		void SetIconSize(float size) { IconSize = size; m_configNode["IconSize"] = size; SaveSettings(); }
		void SetBarRounding(float rounding) { BarRounding = rounding; m_configNode["BarRounding"] = rounding; SaveSettings(); }
		void SetBarBorderThickness(float thickness) { BarBorderThickness = thickness; m_configNode["BarBorderThickness"] = thickness; SaveSettings(); }
		void SetShowDebugPanel(bool show) { ShowDebugPlanel = show; m_configNode["ShowDebugPanel"] = show; SaveSettings(); }
		void SetRenderForSelf(bool show) { RenderForSelf = show; m_configNode["RenderForSelf"] = show; SaveSettings(); }
		void SetRenderForTarget(bool show) { RenderForTarget = show; m_configNode["RenderForTarget"] = show; SaveSettings(); }
		void SetRenderForGroup(bool show) { RenderForGroup = show; m_configNode["RenderForGroup"] = show; SaveSettings(); }
		void SetNameplateWidth(float width) { NameplateWidth = width; m_configNode["NameplateWidth"] = width; SaveSettings(); }
		void SetShowGuild(bool show) { ShowGuild = show; m_configNode["ShowGuild"] = show; SaveSettings(); }
		void SetShowPurpose(bool show) { ShowPurpose = show; m_configNode["ShowPurpose"] = show; SaveSettings(); }

		bool GetShowBuffIcons() const { return ShowBuffIcons; }
		const ImVec2& GetPadding() const { return Padding; }
		float GetFontSize() const { return FontSize; }
		float GetIconSize() const { return IconSize; }
		float GetBarRounding() const { return BarRounding; }
		float GetBarBorderThickness() const { return BarBorderThickness; }
		bool GetShowDebugPanel() const { return ShowDebugPlanel; }
		bool GetRenderForSelf() const { return RenderForSelf; }
		bool GetRenderForTarget() const { return RenderForTarget; }
		bool GetRenderForGroup() const { return RenderForGroup; }
		float GetNameplateWidth() const { return NameplateWidth; }
		bool GetShowGuild() const { return ShowGuild; }
		bool GetShowPurpose() const { return ShowPurpose; }

	private:
		bool RenderForSelf = true;
		bool RenderForTarget = true;
		bool RenderForGroup = true;
		bool ShowGuild = true;
		bool ShowPurpose = true;

		bool ShowBuffIcons = true;
		bool ShowDebugPlanel = false;
		ImVec2 Padding = ImVec2(8, 4);
		float FontSize = 20.0f;
		float IconSize = 20.0f;
		float NameplateWidth = 500.0f;

		float BarRounding = 6.0f;
		float BarBorderThickness = 2.5f;

		std::string m_configFile = "MQAnimatedNameplates.yaml";
		YAML::Node m_configNode =YAML::Node();
	};

	struct StateStruct
	{ 
		std::unordered_map<std::string, TrendState> ProgBarTrendState;
		std::unordered_map<std::string, AnimState> ProgBarAnimState;

		TooltipState TooltipAnimationState;
	};

	static void RenderNamePlateText(CursorState& cursor, ImU32 color, const char* text);

	static void RenderNamePlateRect(
		CursorState& cursor,
		const ImVec2& size,
		ImU32 color,
		float rounding,
		float thickness,
		bool filled
	);

	static void DrawInspectableSpellIcon(
		CursorState& cursor,
		eqlib::EQ_Spell* pSpell
	);

	static void RenderAnimatedPercentage(
		CursorState& cursor,
		const std::string& id,
		float barPct,
		float height,
		float width,
		const ImVec4& colLow,
		const ImVec4& colMid,
		const ImVec4& colHigh,
		ImU32 colHighlight,
		const std::string& label = ""
	);

	static void RenderFancyHPBar(
		CursorState& cursor,
		const std::string& id,
		float hpPct,
		float height,
		float width,
		ImU32 hpHighlight,
		const std::string& label = ""
	);

	static void RenderSettingsPanel();

	static AnimatedNameplatesSettings Settings;
	static StateStruct State;
};

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