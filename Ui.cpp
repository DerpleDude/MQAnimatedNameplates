#include "Ui.h"
#include "eqlib/EQLib.h"
#include "mq/imgui/Widgets.h"
#include "imgui/ImGuiUtils.h"

#include <mq/Plugin.h>

#include <fstream>
#include <algorithm>
#include <cmath>

using namespace eqlib;

Ui::AnimatedNameplatesSettings Ui::Settings;

Ui::StateStruct Ui::State;

void Ui::RenderNamePlateText(CursorState& cursor, ImU32 color, const char* text)
{
	ImVec2 size = ImGui::CalcTextSize(text);

	ImDrawList* dl = ImGui::GetForegroundDrawList();
	dl->AddText(cursor.GetPos(), color, text);

	cursor.Move(size);
}

void Ui::RenderNamePlateRect(
	CursorState& cursor,
	const ImVec2& size,
	ImU32 color,
	float rounding,
	float thickness,
	bool filled)
{
	ImDrawList* dl = ImGui::GetForegroundDrawList();

	ImVec2 max(cursor.GetPos() + size);

	if (filled)
	{
		dl->AddRectFilled(cursor.GetPos(), max, color, rounding);
	}
	else
	{
		dl->AddRect(cursor.GetPos(), max, color, rounding, 0, thickness);
	}

	cursor.Move(size);
}

void Ui::DrawInspectableSpellIcon(CursorState& cursor, EQ_Spell* pSpell)
{
	const ImVec2& cursorPos = cursor.GetPos();
	ImVec2 size(Settings.GetIconSize(), Settings.GetIconSize());
	ImVec2 max(cursorPos + size);

	ImDrawList* dl = ImGui::GetForegroundDrawList();
	ImVec2 mouse = ImGui::GetIO().MousePos;

	bool hovered = mouse.x >= cursorPos.x && mouse.x <= max.x
		&& mouse.y >= cursorPos.y && mouse.y <= max.y;

	bool clicked = hovered && ImGui::IsMouseClicked(0);

	if (pSidlMgr)
	{
		if (CTextureAnimation* anim = pSidlMgr->FindAnimation("A_SpellGems"))
		{
			int iconID = pSpell->SpellIcon;
			anim->SetCurCell(iconID);
			mq::imgui::DrawTextureAnimation(dl, anim, cursorPos, size);
		}
	}

	if (clicked && pSpell)
	{
		if (pSpellDisplayManager)
			pSpellDisplayManager->ShowSpell(pSpell->ID, true, true, SpellDisplayType_SpellBookWnd);
	}

	cursor.Move(size);
}

// helper
static float Clamp(float v, float min, float max)
{
	return std::max(min, std::min(max, v));
}

static ImVec4 ImLerp(const ImVec4& a, const ImVec4& b, float t)
{
	return ImVec4(
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t,
		a.z + (b.z - a.z) * t,
		a.w + (b.w - a.w) * t
	);
}

void Ui::RenderAnimatedPercentage(
	CursorState& cursor,
	const std::string& id,
	float barPct,
	float height,
	float width,
	const ImVec4& colLow,
	const ImVec4& colMid,
	const ImVec4& colHigh,
	ImU32 colHighlight,
	const std::string& label)
{
	float targetPct = Clamp(barPct, 0.0f, 100.0f);

	// FIXME: This should be accumulated time, not absolute time.
	float now = static_cast<float>(ImGui::GetTime());
	ImDrawList* drawList = ImGui::GetForegroundDrawList();

	AnimState& animState = State.ProgBarAnimState[id];

	if (animState.lastTarget == 0)
		animState.lastTarget = targetPct;

	if (targetPct != animState.lastTarget)
		animState.lastTarget = targetPct;

	// simple animation (instead of ImAnim tween)
	float dt = ImGui::GetIO().DeltaTime;
	float pct = animState.lastTarget + (targetPct - animState.lastTarget) * (dt * 8.0f);
	animState.lastTarget = pct;

	float fraction = pct / 100.0f;

	TrendState& trend = State.ProgBarTrendState[id];

	if (trend.lastPct == 0)
	{
		trend.lastPct = pct;
		trend.direction = 1;
	}
	else
	{
		if (targetPct < (trend.lastPct - 0.05f))
			trend.direction = -1;
		else if (targetPct > (trend.lastPct + 0.05f))
			trend.direction = 1;

		trend.lastPct = pct;
	}

	ImVec2 curPos = cursor.GetPos();

	float minX = curPos.x;
	float minY = curPos.y;
	float maxX = curPos.x + width;
	float maxY = curPos.y + height;

	float barW = width;
	float barH = height;

	ImU32 bgTop = IM_COL32(28, 30, 41, 247);
	ImU32 bgBottom = IM_COL32(10, 13, 20, 247);

	drawList->AddRectFilledMultiColor(
		ImVec2(minX + 1, minY + 1),
		ImVec2(maxX - 1, maxY - 1),
		bgTop, bgTop, bgBottom, bgBottom
	);

	drawList->AddRectFilled(
		ImVec2(minX + 1, minY + 1),
		ImVec2(maxX - 1, minY + std::max(2.0f, barH * 0.35f)),
		IM_COL32(255, 255, 255, 14),
		Settings.GetBarRounding()
	);

	float fillWidth = barW * fraction;

	if (fillWidth > 0)
	{
		ImVec4 edge;

		if (fraction < 0.5f)
		{
			float t = fraction / 0.5f;
			edge = ImLerp(colLow, colMid, t);
		}
		else
		{
			float t = (fraction - 0.5f) / 0.5f;
			edge = ImLerp(colMid, colHigh, t);
		}

		ImU32 topLeft = ImGui::GetColorU32(colLow);
		ImU32 topRight = ImGui::GetColorU32(edge);
		ImU32 bottomLeft = topLeft;
		ImU32 bottomRight = topRight;

		float fillMaxX = minX + fillWidth;

		float fillRounding = std::min(
			Settings.GetBarRounding(),
			std::min(barH * 0.5f, fillWidth * 0.5f)
		);

		drawList->AddRectFilled(
			ImVec2(minX, minY),
			ImVec2(fillMaxX, maxY),
			ImGui::ColorConvertFloat4ToU32(colLow),
			fillRounding
		);

		float innerMinX = minX + 1;
		float innerMaxX = fillMaxX - 1;
		float innerMinY = minY + 1;
		float innerMaxY = maxY - 1;

		if (innerMaxX > innerMinX && innerMaxY > innerMinY)
		{
			drawList->AddRectFilledMultiColor(
				ImVec2(innerMinX, innerMinY),
				ImVec2(innerMaxX, innerMaxY),
				topLeft, topRight, bottomRight, bottomLeft
			);

			float glossMaxY = std::min(innerMaxY, minY + std::max(2.0f, barH * 0.45f));

			if (glossMaxY > innerMinY)
			{
				drawList->AddRectFilledMultiColor(
					ImVec2(innerMinX, innerMinY),
					ImVec2(innerMaxX, glossMaxY),
					IM_COL32(255, 255, 255, 14),
					IM_COL32(255, 255, 255, 8),
					IM_COL32(255, 255, 255, 2),
					IM_COL32(255, 255, 255, 8)
				);
			}
		}

		if (fillWidth > 12)
		{
			bool isAnimating = fabs(targetPct - pct) > 0.5f;

			float sweepSpeed = isAnimating ? 1.2f : 0.65f;
			float sweepBase = fmodf(now * sweepSpeed, 1.0f);

			float sweep = (isAnimating || trend.direction < 0)
				? (1.0f - sweepBase)
				: sweepBase;

			float sheenCenter = minX + (fillWidth * sweep);
			float sheenHalf = std::min(16.0f, fillWidth * 0.22f);

			float sheenLeft = std::max(minX + 1, sheenCenter - sheenHalf);
			float sheenRight = std::min(fillMaxX - 1, sheenCenter + sheenHalf);

			if (sheenRight > sheenLeft)
			{
				float sheenMid = (sheenLeft + sheenRight) * 0.5f;

				float sheenAlpha = isAnimating ? 0.25f : 0.18f;

				drawList->AddRectFilledMultiColor(
					ImVec2(sheenLeft, minY),
					ImVec2(sheenMid, maxY),
					IM_COL32(255, 255, 255, 0),
					IM_COL32(255, 255, 255, static_cast<int>(sheenAlpha * 255)),
					IM_COL32(255, 255, 255, static_cast<int>((sheenAlpha * 0.55f) * 255)),
					IM_COL32(255, 255, 255, 0)
				);

				drawList->AddRectFilledMultiColor(
					ImVec2(sheenMid, minY),
					ImVec2(sheenRight, maxY),
					IM_COL32(255, 255, 255, static_cast<int>(sheenAlpha * 255)),
					IM_COL32(255, 255, 255, 0),
					IM_COL32(255, 255, 255, 0),
					IM_COL32(255, 255, 255, static_cast<int>((sheenAlpha * 0.55f) * 255))
				);
			}
		}
	}

	for (int i = 1; i < 10; ++i)
	{
		float tx = minX + (barW * (i / 10.0f));
		bool reached = tx <= (minX + fillWidth);

		float a = reached ? 0.64f : 0.34f;

		drawList->AddLine(
			ImVec2(tx, minY + 1),
			ImVec2(tx, maxY - 1),
			IM_COL32(255, 255, 255, static_cast<int>(a * 255)),
			1.0f
		);
	}

	drawList->AddRect(
		ImVec2(minX, minY),
		ImVec2(maxX, maxY),
		colHighlight,
		Settings.GetBarRounding(),
		0,
		Settings.GetBarBorderThickness()
	);

	std::string text = label.empty()
		? std::to_string((int)std::floor(pct + 0.5f)) + "%"
		: label;

	ImVec2 textSize = ImGui::CalcTextSize(text.c_str());

	float textX = minX + ((maxX - minX - textSize.x) * 0.5f);
	float textY = minY + ((barH - ImGui::GetTextLineHeight()) * 0.5f);

	drawList->AddText(ImVec2(textX + 1, textY + 1), IM_COL32(0, 0, 0, 230), text.c_str());
	drawList->AddText(ImVec2(textX, textY), IM_COL32(255, 255, 255, 255), text.c_str());
}

void Ui::RenderFancyHPBar(
	CursorState& cursor,
	const std::string& id,
	float hpPct,
	float height,
	float width,
	ImU32 hpHighlight,
	const std::string& label)
{
	ImVec4 hpLow = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
	ImVec4 hpMid = ImVec4(0.9f, 0.7f, 0.2f, 1.0f);
	ImVec4 hpHigh = ImVec4(0.2f, 0.9f, 0.2f, 1.0f);

	RenderAnimatedPercentage(
		cursor,
		id,
		hpPct,
		height,
		width,
		hpLow,
		hpMid,
		hpHigh,
		hpHighlight,
		label
	);
}

void Ui::RenderSettingsPanel() 
{
	ImGui::PushFont(mq::imgui::LargeTextFont);
	ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "Nameplate Settings");
	ImGui::Separator();
	ImGui::PopFont();

	bool showDebugPanel = Settings.GetShowDebugPanel();
	if (ImGui::Checkbox("Show Debug Panel", &showDebugPanel))
		Settings.SetShowDebugPanel(showDebugPanel);

	bool showBuffIcons = Settings.GetShowBuffIcons();
	if (ImGui::Checkbox("Show Buff Icons", &showBuffIcons))
		Settings.SetShowBuffIcons(showBuffIcons);

	bool renderForSelf = Settings.GetRenderForSelf();
	if (ImGui::Checkbox("Render For Self", &renderForSelf))
		Settings.SetRenderForSelf(renderForSelf);

	bool renderForTarget = Settings.GetRenderForTarget();
	if (ImGui::Checkbox("Render For Target", &renderForTarget))
		Settings.SetRenderForTarget(renderForTarget);

	bool renderForGroup = Settings.GetRenderForGroup();
	if (ImGui::Checkbox("Render For Group", &renderForGroup))
		Settings.SetRenderForGroup(renderForGroup);

	bool showGuild = Settings.GetShowGuild();
	if (ImGui::Checkbox("Show Guild", &showGuild))
		Settings.SetShowGuild(showGuild);

	bool showPurpose = Settings.GetShowPurpose();
	if (ImGui::Checkbox("Show Purpose", &showPurpose))
		Settings.SetShowPurpose(showPurpose);

	float nameplateWidth = Settings.GetNameplateWidth();
	if (ImGui::SliderFloat("Nameplate Width", &nameplateWidth, 100.0f, 1500.0f))
		Settings.SetNameplateWidth(nameplateWidth);

	float fontSize = Settings.GetFontSize();
	if (ImGui::SliderFloat("Font Size", &fontSize, 10.0f, 30.0f))
		Settings.SetFontSize(fontSize);

	float iconSize = Settings.GetIconSize();
	if (ImGui::SliderFloat("Icon Size", &iconSize, 10.0f, 100.0f))
		Settings.SetIconSize(iconSize);

	float barRounding = Settings.GetBarRounding();
	if (ImGui::SliderFloat("Bar Rounding", &barRounding, 0.0f, 20.0f))
		Settings.SetBarRounding(barRounding);

	float barBorderThickness = Settings.GetBarBorderThickness();
	if (ImGui::SliderFloat("Bar Border Thickness", &barBorderThickness, 0.0f, 10.0f))
		Settings.SetBarBorderThickness(barBorderThickness);
}

void Ui::AnimatedNameplatesSettings::LoadSettings()
{
	m_configFile = (std::filesystem::path(gPathConfig) / "MQAnimatedNameplates.yaml").string();

	try
	{
		m_configNode = YAML::LoadFile(m_configFile);

		ShowBuffIcons = m_configNode["ShowBuffIcons"].as<bool>(ShowBuffIcons);
		ShowDebugPlanel = m_configNode["ShowDebugPanel"].as<bool>(ShowDebugPlanel);
		FontSize = m_configNode["FontSize"].as<float>(FontSize);
		IconSize = m_configNode["IconSize"].as<float>(IconSize);
		BarRounding = m_configNode["BarRounding"].as<float>(BarRounding);
		BarBorderThickness = m_configNode["BarBorderThickness"].as<float>(BarBorderThickness);
		RenderForSelf = m_configNode["RenderForSelf"].as<bool>(RenderForSelf);
		RenderForTarget = m_configNode["RenderForTarget"].as<bool>(RenderForTarget);
		RenderForGroup = m_configNode["RenderForGroup"].as<bool>(RenderForGroup);
		NameplateWidth = m_configNode["NameplateWidth"].as<float>(NameplateWidth);
		ShowGuild = m_configNode["ShowGuild"].as<bool>(ShowGuild);
		ShowPurpose = m_configNode["ShowPurpose"].as<bool>(ShowPurpose);

		Padding = ImVec2(
			m_configNode["PaddingX"].as<float>(Padding.x),
			m_configNode["PaddingY"].as<float>(Padding.y)
		);
	}
	catch (const YAML::ParserException& ex)
	{
		// failed to parse, notify and return
		SPDLOG_ERROR("Failed to parse YAML in {}: {}", m_configFile, ex.what());
		return;
	}
	catch (const YAML::BadFile&)
	{
		// if we can't read the file, then try to write it with an empty config
		SaveSettings();
		return;
	}
}

void Ui::AnimatedNameplatesSettings::SaveSettings()
{
	try
	{
		std::fstream file(m_configFile, std::ios::out);

		if (!m_configNode.IsNull())
		{
			YAML::Emitter y_out;
			y_out.SetIndent(4);
			y_out.SetFloatPrecision(2);
			y_out.SetDoublePrecision(2);
			y_out << m_configNode;

			file << y_out.c_str();
		}

	}
	catch (const std::exception&)
	{
		SPDLOG_ERROR("Failed to write settings file: {}", m_configFile);
	}
}