#include "Ui.h"
#include "eqlib/EQLib.h"
#include "mq/imgui/Widgets.h"
#include "imgui/ImGuiUtils.h"

#include "imgui_internal.h"
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

	ImDrawList* dl = Ui::GetDrawList();
	dl->AddText(cursor.GetPos(), color, text);

	cursor.Move(size);
}

void Ui::RenderNamePlateRect(CursorState& cursor, const ImVec2& size, ImU32 color, float rounding,
	float thickness, bool filled)
{
	ImDrawList* dl = Ui::GetDrawList();

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

	ImDrawList* dl = Ui::GetDrawList();
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

void Ui::RenderAnimatedPercentage(CursorState& cursor, const std::string& id, float barPct, float height, float width,
	const ImVec4& colLow, const ImVec4& colMid, const ImVec4& colHigh, ImU32 colHighlight,
	const std::string& label)
{
	float targetPct = std::clamp(barPct, 0.0f, 100.0f);

	// FIXME: This should be accumulated time, not absolute time.
	float now = static_cast<float>(ImGui::GetTime());
	ImDrawList* drawList = Ui::GetDrawList();

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

	AddRectFilledMultiColorRounded(*drawList,
		ImVec2(minX + 1, minY + 1),
		ImVec2(maxX - 1, maxY - 1),
		bgTop, bgTop, bgBottom, bgBottom,
		Settings.GetBarRounding(),
		0
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
			AddRectFilledMultiColorRounded(*drawList,
				ImVec2(innerMinX, innerMinY),
				ImVec2(innerMaxX, innerMaxY),
				topLeft, topRight, bottomRight, bottomLeft,
				Settings.GetBarRounding(), 0
			);

			float glossMaxY = std::min(innerMaxY, minY + std::max(2.0f, barH * 0.45f));

			if (glossMaxY > innerMinY)
			{
				AddRectFilledMultiColorRounded(*drawList,
					ImVec2(innerMinX, innerMinY),
					ImVec2(innerMaxX, glossMaxY),
					IM_COL32(255, 255, 255, 14),
					IM_COL32(255, 255, 255, 8),
					IM_COL32(255, 255, 255, 2),
					IM_COL32(255, 255, 255, 8),
					Settings.GetBarRounding(),
					0
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

				AddRectFilledMultiColorRounded(*drawList,
					ImVec2(sheenLeft, minY),
					ImVec2(sheenMid, maxY),
					IM_COL32(255, 255, 255, 0),
					IM_COL32(255, 255, 255, static_cast<int>(sheenAlpha * 255)),
					IM_COL32(255, 255, 255, static_cast<int>((sheenAlpha * 0.55f) * 255)),
					IM_COL32(255, 255, 255, 0),
					Settings.GetBarRounding(),
					0
				);

				AddRectFilledMultiColorRounded(*drawList,
					ImVec2(sheenMid, minY),
					ImVec2(sheenRight, maxY),
					IM_COL32(255, 255, 255, static_cast<int>(sheenAlpha * 255)),
					IM_COL32(255, 255, 255, 0),
					IM_COL32(255, 255, 255, 0),
					IM_COL32(255, 255, 255, static_cast<int>((sheenAlpha * 0.55f) * 255)),
					Settings.GetBarRounding(),
					0
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

void Ui::RenderFancyHPBar(CursorState& cursor, const std::string& id, float hpPct, float height, float width,
	ImU32 hpHighlight, const std::string& label)
{
	ImVec4 hpLow = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
	ImVec4 hpMid = ImVec4(0.9f, 0.7f, 0.2f, 1.0f);
	ImVec4 hpHigh = ImVec4(0.2f, 0.9f, 0.2f, 1.0f);

	RenderAnimatedPercentage(cursor, id, hpPct, height, width, hpLow, hpMid, hpHigh,
		hpHighlight, label);
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

	bool renderToForeground = Settings.GetRenderToForeground();
	if (ImGui::Checkbox("Render To Foreground", &renderToForeground))
		Settings.SetRenderToForeground(renderToForeground);

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

	bool renderForAllHaters = Settings.GetRenderForAllHaters();
	if (ImGui::Checkbox("Render For All Haters", &renderForAllHaters))
		Settings.SetRenderForAllHaters(renderForAllHaters);

	bool showGuild = Settings.GetShowGuild();
	if (ImGui::Checkbox("Show Guild", &showGuild))
		Settings.SetShowGuild(showGuild);

	bool showPurpose = Settings.GetShowPurpose();
	if (ImGui::Checkbox("Show Purpose", &showPurpose))
		Settings.SetShowPurpose(showPurpose);

	float nameplateWidth = Settings.GetNameplateWidth();
	if (ImGui::InputFloat("Nameplate Width", &nameplateWidth, 1.0f, 5.0f, "%.1f"))
		Settings.SetNameplateWidth(nameplateWidth);

	float fontSize = Settings.GetFontSize();
	if (ImGui::InputFloat("Font Size", &fontSize, 1.0f, 5.0f, "%.1f"))
		Settings.SetFontSize(fontSize);

	float iconSize = Settings.GetIconSize();
	if (ImGui::InputFloat("Icon Size", &iconSize, 1.0f, 5.0f, "%.1f"))
		Settings.SetIconSize(iconSize);

	float barRounding = Settings.GetBarRounding();
	if (ImGui::InputFloat("Bar Rounding", &barRounding, 1.0f, 5.0f, "%.1f"))
		Settings.SetBarRounding(barRounding);

	float barBorderThickness = Settings.GetBarBorderThickness();
	if (ImGui::InputFloat("Bar Border Thickness", &barBorderThickness, 1.0f, 5.0f, "%.1f"))
		Settings.SetBarBorderThickness(barBorderThickness);
}

void Ui::AnimatedNameplatesSettings::LoadSettings()
{
	m_configFile = (std::filesystem::path(gPathConfig) / "MQAnimatedNameplates.yaml").string();

	try
	{
		m_configNode = YAML::LoadFile(m_configFile);

		m_showBuffIcons = m_configNode["ShowBuffIcons"].as<bool>(m_showBuffIcons);
		m_showDebugPanel = m_configNode["ShowDebugPanel"].as<bool>(m_showDebugPanel);
		m_fontSize = m_configNode["FontSize"].as<float>(m_fontSize);
		m_iconSize = m_configNode["IconSize"].as<float>(m_iconSize);
		m_barRounding = m_configNode["BarRounding"].as<float>(m_barRounding);
		m_barBorderThickness = m_configNode["BarBorderThickness"].as<float>(m_barBorderThickness);
		m_renderForSelf = m_configNode["RenderForSelf"].as<bool>(m_renderForSelf);
		m_renderForTarget = m_configNode["RenderForTarget"].as<bool>(m_renderForTarget);
		m_renderForGroup = m_configNode["RenderForGroup"].as<bool>(m_renderForGroup);
		m_renderForAllHaters = m_configNode["RenderForAllHaters"].as<bool>(m_renderForAllHaters);
		m_nameplateWidth = m_configNode["NameplateWidth"].as<float>(m_nameplateWidth);
		m_showGuild = m_configNode["ShowGuild"].as<bool>(m_showGuild);
		m_showPurpose = m_configNode["ShowPurpose"].as<bool>(m_showPurpose);
		m_renderToForeground = m_configNode["RenderToForeground"].as<bool>(m_renderToForeground);

		m_padding = ImVec2(
			m_configNode["PaddingX"].as<float>(m_padding.x),
			m_configNode["PaddingY"].as<float>(m_padding.y)
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

ImDrawList* Ui::GetDrawList()
{
	return Ui::Settings.GetRenderToForeground() ? ImGui::GetForegroundDrawList() : ImGui::GetBackgroundDrawList();
}

// Helpers from imgui_draw.cpp
// 
// Normalize (x,y) if length > 0.0f; otherwise leave unchanged.
static void Normalize2fOverZero(float& x, float& y)
{
	const float d2 = x * x + y * y;
	if (d2 > 0.0f)
	{
		const float inv_len = ImRsqrt(d2);
		x *= inv_len;
		y *= inv_len;
	}
}

// Helper used by ImGui's AA fringe generation: scales by inv_len^2 and clamps
// to avoid extremely large normals on nearly-degenerate segments.
static void FixNormal2f(float& x, float& y)
{
	// This mirrors the intent in upstream imgui_draw.cpp (see ImGui issues #4053, #3366).
	constexpr float FixNormalMaxInvLen2 = 100.0f;

	const float d2 = x * x + y * y;
	if (d2 > 0.000001f)
	{
		float inv_len2 = 1.0f / d2;
		if (inv_len2 > FixNormalMaxInvLen2)
			inv_len2 = FixNormalMaxInvLen2;

		x *= inv_len2;
		y *= inv_len2;
	}
}

// AddRectFilledMultiColorRounded
// ------------------------------
// Draw a rounded rect filled with a 4-corner (bilinear) gradient.
//
// - We call PathRect() to generate the rounded outline points into draw_list._Path.
// - We fill the interior using a triangle fan over the "inner" vertices.
// - We generate an anti-aliased fringe ring around the shape (like ImDrawList::AddConvexPolyFilled),
//   computing per-vertex normals from the outline.
// - Each vertex color is computed by bilinear interpolation of the four corner colors based on its (u,v)
//   position within the rectangle.
void Ui::AddRectFilledMultiColorRounded(ImDrawList& draw_list, const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left, float rounding, ImDrawFlags flags)
{
	// All corners fully transparent => nothing to draw
	if (((col_upr_left | col_upr_right | col_bot_right | col_bot_left) & IM_COL32_A_MASK) == 0)
		return;

	// If no rounding requested, use built-in multicolor rect and exit.
	if (rounding <= 0.0f)
	{
		draw_list.AddRectFilledMultiColor(p_min, p_max, col_upr_left, col_upr_right, col_bot_right, col_bot_left);
		return;
	}

	// Build the rounded outline path.
	draw_list.PathRect(p_min, p_max, rounding, flags);
	const int points_count = draw_list._Path.Size;
	if (points_count < 3)
	{
		draw_list.PathClear();
		return;
	}

	// Corner colors as float4 for lerping
	const ImVec4 ul = ImGui::ColorConvertU32ToFloat4(col_upr_left);
	const ImVec4 ur = ImGui::ColorConvertU32ToFloat4(col_upr_right);
	const ImVec4 br = ImGui::ColorConvertU32ToFloat4(col_bot_right);
	const ImVec4 bl = ImGui::ColorConvertU32ToFloat4(col_bot_left);

	const float w = (p_max.x - p_min.x);
	const float h = (p_max.y - p_min.y);
	const float inv_w = (w != 0.0f) ? (1.0f / w) : 0.0f;
	const float inv_h = (h != 0.0f) ? (1.0f / h) : 0.0f;

	const ImVec2 uv = draw_list._Data->TexUvWhitePixel;

	// Reserve geometry:
	// - Inner fill uses (points_count - 2) triangles => (points_count - 2) * 3 indices
	// - AA fringe uses points_count quads => points_count * 6 indices
	// - For each outline point we emit 2 vertices (inner/outer)
	const float aa_size = draw_list._FringeScale;
	const int idx_count = (points_count - 2) * 3 + points_count * 6;
	const int vtx_count = points_count * 2;
	draw_list.PrimReserve(idx_count, vtx_count);

	const unsigned int vtx_inner_idx = draw_list._VtxCurrentIdx;
	const unsigned int vtx_outer_idx = draw_list._VtxCurrentIdx + 1;

	// 1) Fill interior (triangle fan using inner vertices)
	for (int i = 2; i < points_count; ++i)
	{
		draw_list._IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx);
		draw_list._IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + (i - 1) * 2);
		draw_list._IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx + i * 2);
		draw_list._IdxWritePtr += 3;
	}

	// 2) Compute normals per edge to create AA fringe.
	draw_list._Data->TempBuffer.reserve_discard(points_count);
	ImVec2* edge_normals = draw_list._Data->TempBuffer.Data;

	for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
	{
		const ImVec2& p0 = draw_list._Path[i0];
		const ImVec2& p1 = draw_list._Path[i1];

		float dx = p1.x - p0.x;
		float dy = p1.y - p0.y;
		Normalize2fOverZero(dx, dy);

		// Per-edge normal (perpendicular to direction)
		edge_normals[i0].x = dy;
		edge_normals[i0].y = -dx;
	}

	// 3) Emit vertices + AA fringe indices
	for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
	{
		const ImVec2& n0 = edge_normals[i0];
		const ImVec2& n1 = edge_normals[i1];

		// Average the adjacent edge normals to get a vertex normal.
		float nx = (n0.x + n1.x) * 0.5f;
		float ny = (n0.y + n1.y) * 0.5f;
		FixNormal2f(nx, ny);

		// Expand to inner/outer positions for AA fringe
		nx *= aa_size * 0.5f;
		ny *= aa_size * 0.5f;

		const ImVec2& p = draw_list._Path[i1];

		// Compute bilinear gradient coordinates
		const float u = (inv_w == 0.0f) ? 0.0f : (p.x - p_min.x) * inv_w;
		const float v = (inv_h == 0.0f) ? 0.0f : (p.y - p_min.y) * inv_h;

		const ImVec4 top = ImLerp(ul, ur, u);
		const ImVec4 bot = ImLerp(bl, br, u);
		const ImU32 col = ImGui::ColorConvertFloat4ToU32(ImLerp(top, bot, v));
		const ImU32 col_trans = col & ~IM_COL32_A_MASK;

		// Write inner + outer vertices
		ImDrawVert& vtx_inner = draw_list._VtxWritePtr[0];
		ImDrawVert& vtx_outer = draw_list._VtxWritePtr[1];

		vtx_inner.pos = ImVec2(p.x - nx, p.y - ny);
		vtx_inner.uv = uv;
		vtx_inner.col = col;

		vtx_outer.pos = ImVec2(p.x + nx, p.y + ny);
		vtx_outer.uv = uv;
		vtx_outer.col = col_trans;

		draw_list._VtxWritePtr += 2;

		// AA fringe quad indices (inner i0/i1 to outer i0/i1)
		draw_list._IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx + i1 * 2);
		draw_list._IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + i0 * 2);
		draw_list._IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx + i0 * 2);

		draw_list._IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx + i0 * 2);
		draw_list._IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx + i1 * 2);
		draw_list._IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx + i1 * 2);

		draw_list._IdxWritePtr += 6;
	}

	draw_list._VtxCurrentIdx += (ImDrawIdx)vtx_count;
	draw_list.PathClear();
}
