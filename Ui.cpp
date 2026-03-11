#include "Ui.h"
#include "eqlib/EQLib.h"
#include "imgui/ImGuiUtils.h"
#include "mq/imgui/Widgets.h"

#include <mq/Plugin.h>

#include <algorithm>
#include <cmath>
#include <fstream>

using namespace eqlib;

Ui::AnimatedNameplatesSettings Ui::Settings;

Ui::StateStruct Ui::State;

static float GetUIDeltaTime()
{
    float dt = ImGui::GetIO().DeltaTime;
    if (dt <= 0.0f)
        dt = 1.0f / 60.0f;

    if (dt > 0.1f)
        dt = 0.1f;
    return dt;
}

void Ui::RenderNamePlateText(CursorState& cursor, ImU32 color, const char* text)
{
    ImVec2 size = ImGui::CalcTextSize(text);

    ImDrawList* dl = Ui::GetDrawList();
    dl->AddText(cursor.GetPos(), color, text);

    cursor.Move(size);
}

void Ui::RenderNamePlateRect(CursorState& cursor, const ImVec2& size, ImU32 color, float rounding, float thickness,
                             bool filled)
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
    ImVec2        size(Settings.GetIconSize(), Settings.GetIconSize());
    ImVec2        max(cursorPos + size);

    ImDrawList* dl    = Ui::GetDrawList();
    ImVec2      mouse = ImGui::GetIO().MousePos;

    bool hovered = mouse.x >= cursorPos.x && mouse.x <= max.x && mouse.y >= cursorPos.y && mouse.y <= max.y;

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

    cursor.Move(size);
}

void Ui::RenderAnimatedPercentage(CursorState& cursor, const std::string& id, const float barPct, const float height,
                                  const float width, ImU32 colLow, ImU32 colMid, ImU32 colHigh, ImU32 colHighlight,
                                  const std::string& label /* = "" */)
{
    float targetPct = std::clamp(barPct, 0.0f, 100.0f);

    // FIXME: This should be accumulated time, not absolute time.
    float       now      = static_cast<float>(ImGui::GetTime());
    ImDrawList* drawList = Ui::GetDrawList();

    AnimState& animState = State.ProgBarAnimState[id];

    if (animState.lastTarget == 0)
        animState.lastTarget = targetPct;

    if (targetPct != animState.lastTarget)
        animState.lastTarget = targetPct;

    float dt             = ImGui::GetIO().DeltaTime;
    float pct            = animState.lastTarget + (targetPct - animState.lastTarget) * (dt * 8.0f);
    animState.lastTarget = pct;

    float fraction = pct / 100.0f;

    TrendState& trend = State.ProgBarTrendState[id];

    if (trend.lastPct == 0)
    {
        trend.lastPct   = pct;
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

    ImU32 bgTop    = IM_COL32(28, 30, 41, 247);
    ImU32 bgBottom = IM_COL32(10, 13, 20, 247);

    AddRectFilledMultiColorRounded(*drawList, ImVec2(minX + 1, minY + 1), ImVec2(maxX - 1, maxY - 1), bgTop, bgTop,
                                   bgBottom, bgBottom, Settings.GetBarRounding(), 0);

    drawList->AddRectFilled(ImVec2(minX + 1, minY + 1), ImVec2(maxX - 1, minY + std::max(2.0f, barH * 0.35f)),
                            IM_COL32(255, 255, 255, 14), Settings.GetBarRounding());

    float fillWidth = barW * fraction;

    if (fillWidth > 0)
    {
        ImU32 edge;

        if (colHigh == colLow && colLow == colMid)
        {
            edge = colLow;
        }
        else
        {
            if (fraction < 0.5f)
            {
                float t = fraction / 0.5f;
                edge    = ImLerp(colLow, colMid, t);
            }
            else
            {
                float t = (fraction - 0.5f) / 0.5f;
                edge    = ImLerp(colMid, colHigh, t);
            }
        }

        ImU32 topLeft     = colLow;
        ImU32 topRight    = edge;
        ImU32 bottomLeft  = topLeft;
        ImU32 bottomRight = topRight;

        float fillMaxX = minX + fillWidth;

        float fillRounding = std::min(Settings.GetBarRounding(), std::min(barH * 0.5f, fillWidth * 0.5f));

        drawList->AddRectFilled(ImVec2(minX, minY), ImVec2(fillMaxX, maxY), colLow, fillRounding);

        float innerMinX = minX + 1;
        float innerMaxX = fillMaxX - 1;
        float innerMinY = minY + 1;
        float innerMaxY = maxY - 1;

        if (innerMaxX > innerMinX && innerMaxY > innerMinY)
        {
            AddRectFilledMultiColorRounded(*drawList, ImVec2(innerMinX, innerMinY), ImVec2(innerMaxX, innerMaxY),
                                           topLeft, topRight, bottomRight, bottomLeft, Settings.GetBarRounding(), 0);

            float glossMaxY = std::min(innerMaxY, minY + std::max(2.0f, barH * 0.45f));

            if (glossMaxY > innerMinY)
            {
                AddRectFilledMultiColorRounded(*drawList, ImVec2(innerMinX, innerMinY), ImVec2(innerMaxX, glossMaxY),
                                               IM_COL32(255, 255, 255, 14), IM_COL32(255, 255, 255, 8),
                                               IM_COL32(255, 255, 255, 2), IM_COL32(255, 255, 255, 8),
                                               Settings.GetBarRounding(), 0);
            }
        }

        if (fillWidth > 12)
        {
            bool isAnimating = fabs(targetPct - pct) > 0.5f;

            float sweepSpeed = isAnimating ? 1.2f : 0.65f;
            float sweepBase  = fmodf(now * sweepSpeed, 1.0f);

            float sweep = (isAnimating || trend.direction < 0) ? (1.0f - sweepBase) : sweepBase;

            float sheenCenter = minX + (fillWidth * sweep);
            float sheenHalf   = std::min(16.0f, fillWidth * 0.22f);

            float sheenLeft  = std::max(minX + 1, sheenCenter - sheenHalf);
            float sheenRight = std::min(fillMaxX - 1, sheenCenter + sheenHalf);

            if (sheenRight > sheenLeft)
            {
                float sheenMid = (sheenLeft + sheenRight) * 0.5f;

                float sheenAlpha = isAnimating ? 0.25f : 0.18f;

                AddRectFilledMultiColorRounded(*drawList, ImVec2(sheenLeft, minY), ImVec2(sheenMid, maxY),
                                               IM_COL32(255, 255, 255, 0),
                                               IM_COL32(255, 255, 255, static_cast<int>(sheenAlpha * 255)),
                                               IM_COL32(255, 255, 255, static_cast<int>((sheenAlpha * 0.55f) * 255)),
                                               IM_COL32(255, 255, 255, 0), Settings.GetBarRounding(), 0);

                AddRectFilledMultiColorRounded(*drawList, ImVec2(sheenMid, minY), ImVec2(sheenRight, maxY),
                                               IM_COL32(255, 255, 255, static_cast<int>(sheenAlpha * 255)),
                                               IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0),
                                               IM_COL32(255, 255, 255, static_cast<int>((sheenAlpha * 0.55f) * 255)),
                                               Settings.GetBarRounding(), 0);
            }
        }
    }

    for (int i = 1; i < 10; ++i)
    {
        float tx      = minX + (barW * (i / 10.0f));
        bool  reached = tx <= (minX + fillWidth);

        float a = reached ? 0.64f : 0.34f;

        drawList->AddLine(ImVec2(tx, minY + 1), ImVec2(tx, maxY - 1),
                          IM_COL32(255, 255, 255, static_cast<int>(a * 255)), 1.0f);
    }

    drawList->AddRect(ImVec2(minX, minY), ImVec2(maxX, maxY), colHighlight, Settings.GetBarRounding(), 0,
                      Settings.GetBarBorderThickness());

    std::string text = label.empty() ? std::to_string((int)std::floor(pct + 0.5f)) + "%" : label;

    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());

    float textX = minX + ((maxX - minX - textSize.x) * 0.5f);
    float textY = minY + ((barH - ImGui::GetTextLineHeight()) * 0.5f);

    drawList->AddText(ImVec2(textX + 1, textY + 1), IM_COL32(0, 0, 0, 230), text.c_str());
    drawList->AddText(ImVec2(textX, textY), IM_COL32(255, 255, 255, 255), text.c_str());

    // Ui::DrawDragonWing(drawList, ImVec2(minX, minY), (maxY-minY)*8.0f, (maxX-minX), -1.0f, 1.0f);
}

void Ui::RenderFancyHPBar(CursorState& cursor, const std::string& id, float hpPct, float height, float width,
                          ImU32 conColor, bool currentTarget, const std::string& label)
{
    ImU32 hpLow  = IM_COL32(floor(0.8f * 255), floor(0.2f * 255), floor(0.2f * 255), 255);
    ImU32 hpMid  = IM_COL32(floor(0.9f * 255), floor(0.7f * 255), floor(0.2f * 255), 255);
    ImU32 hpHigh = IM_COL32(floor(0.2f * 255), floor(0.9f * 255), floor(0.2f * 255), 255);

    ImU32 highlightColor = conColor;

    switch (Settings.GetHPBarStyle())
    {
    case AnimatedNameplatesSettings::HPBarStyle_SolidRed:
        hpLow = hpMid = hpHigh = IM_COL32(floor(0.8f * 255), floor(0.2f * 255), floor(0.2f * 255), 255);
        highlightColor         = currentTarget ? IM_COL32(255, 128, 0, 255) : IM_COL32(240, 80, 240, 255);
        break;
    case AnimatedNameplatesSettings::HPBarStyle_ConColor:
        hpLow = hpMid = hpHigh = conColor;
        highlightColor         = currentTarget ? IM_COL32(255, 128, 0, 255) : IM_COL32(240, 80, 240, 255);
        break;
    case AnimatedNameplatesSettings::HPBarStyle_ColorRange:
    default:
        break;
    }

    RenderAnimatedPercentage(cursor, id, hpPct, height, width, hpLow, hpMid, hpHigh, highlightColor, label);
}

void Ui::AnimatedCheckmark::Reset(bool newVal)
{
    m_path1Time = 0.0f;
    m_path2Time = 0.0f;

    if (newVal != m_newValue)
    {
        m_newValue = newVal;

        m_pathInitialized = false;
        m_path1Complete   = false;
        m_path2Complete   = false;
    }
    else
    {
        // already in the correct state, so just mark the paths as complete
        m_pathInitialized = true;
        m_path1Complete   = true;
        m_path1Complete   = true;
    }
}

void Ui::AnimatedCheckmark::Render(ImDrawList* dl, const ImRect& check_bb, float box_size)
{
    float dt = ImGui::GetIO().DeltaTime;

    ImU32 check_col = ImGui::GetColorU32(ImGuiCol_CheckMark);

    const float pad = ImMax(1.0f, IM_TRUNC(box_size / 6.0f));
    float       sz  = box_size - pad * 2.0f;

    float thickness = ImMax(sz / 5.0f, 1.0f);
    sz -= thickness * 0.5f;

    ImVec2 pos = check_bb.Min + ImVec2(pad, pad);
    pos += ImVec2(thickness * 0.25f, thickness * 0.25f);

    float third = sz / 3.0f;
    float bx    = pos.x + third;
    float by    = pos.y + sz - third * 0.5f;

    ImVec2 p1(bx - third, by - third);
    ImVec2 p2(bx, by);
    ImVec2 p3(bx + third * 2.0f, by - third * 2.0f);

    if (!m_pathInitialized)
    {
        m_pathInitialized = true;

        if (m_newValue)
        {
            iam_path::begin(m_animIdPath1, p1).line_to(p2).end();

            iam_path::begin(m_animIdPath2, p2).line_to(p3).end();
        }
        else
        {
            iam_path::begin(m_animIdPath1, p3).line_to(p2).end();

            iam_path::begin(m_animIdPath2, p2).line_to(p1).end();
        }
    }

    if (!m_path1Complete)
    {
        m_path1Time += dt * m_animSpeed;

        auto ap1 = iam_path_evaluate(m_animIdPath1, m_path1Time);

        if (m_newValue)
            dl->AddLine(p1, ap1, check_col, thickness);
        else
            dl->AddLine(p2, ap1, check_col, thickness);

        m_path1Complete = m_path1Time > 1.0f;
    }
    else
    {
        if (m_newValue)
            dl->AddLine(p1, p2, check_col, thickness);
    }

    if (m_path1Complete && !m_path2Complete)
    {
        m_path2Time += dt * m_animSpeed;

        auto ap2 = iam_path_evaluate(m_animIdPath2, m_path2Time);

        if (m_newValue)
            dl->AddLine(p2, ap2, check_col, thickness);
        else
            dl->AddLine(p1, ap2, check_col, thickness);

        m_path2Complete = m_path2Time > 1.0f;
    }
    else
    {
        if (m_path2Complete && m_newValue)
            dl->AddLine(p2, p3, check_col, thickness);

        if (!m_path1Complete && !m_newValue)
            dl->AddLine(p2, p1, check_col, thickness);
    }
}

std::map<ImU32, Ui::AnimatedCheckmark> checkBoxAnims;

bool Ui::AnimatedCheckbox(const std::string& label, bool* value)
{
    ImU32 animId = ImHashStr(label.c_str());

    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImVec2 box_pos = ImGui::GetCursorScreenPos();

    float        box_size = ImGui::GetFrameHeight();
    const ImRect check_bb(box_pos, box_pos + ImVec2(box_size, box_size));
    float        line_height = ImGui::GetTextLineHeight();

    ImVec2      label_size = ImGui::CalcTextSize(label.c_str());
    ImGuiStyle& style      = ImGui::GetStyle();

    auto [it, inserted] = checkBoxAnims.try_emplace(
        animId, Ui::AnimatedCheckmark(*value, ImHashStr(fmt::format("{}_path1", label).c_str()),
                                      ImHashStr(fmt::format("{}_path2", label).c_str())));
    auto& animState  = it->second;
    bool  valueStart = *value;

    ImGui::PushID(label.c_str());

    ImGui::SetCursorScreenPos(box_pos);

    // interactions
    bool         hovered, held;
    const ImRect total_bb(
        box_pos, box_pos + ImVec2(box_size + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f),
                                  label_size.y + style.FramePadding.y * 2.0f));
    const ImGuiID id         = ImGui::GetID(label.c_str());
    const bool    is_visible = ImGui::ItemAdd(total_bb, id);
    bool          pressed    = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);

    if (pressed || inserted)
    {
        if (pressed)
            (*value) = !(*value);

        animState.Reset(*value);
    }

    // Box background
    ImU32       box_bg      = ImGui::GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive
                                                 : hovered         ? ImGuiCol_FrameBgHovered
                                                                   : ImGuiCol_FrameBg);
    const float border_size = style.FrameBorderSize;
    ImVec2      center      = (check_bb.Min + check_bb.Max) * 0.5f;

    dl->AddRectFilled(check_bb.Min, check_bb.Max, box_bg, style.FrameRounding);
    dl->AddRect(check_bb.Min + ImVec2(1, 1), check_bb.Max + ImVec2(1, 1), ImGui::GetColorU32(ImGuiCol_BorderShadow),
                style.FrameRounding, 0, border_size);
    dl->AddRect(check_bb.Min, check_bb.Max, ImGui::GetColorU32(ImGuiCol_Border), style.FrameRounding);

    // Draw animated checkmark
    animState.Render(dl, check_bb, box_size);

    // Label
    const ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
    dl->AddText(label_pos, ImGui::GetColorU32(ImGuiCol_Text), label.c_str());

    ImGui::PopID();

    ImGui::SetCursorScreenPos(ImVec2(total_bb.Min.x, total_bb.Max.y));
    ImGui::Dummy(ImVec2(1, 1));

    return pressed;
}

bool Ui::AnimatedSlider(const std::string& label, float* slider_value, float slider_min, float slider_max,
                        const char* format, float labelWidthOverride)
{
    const ImGuiID id = ImGui::GetID(label.c_str());

    float       dt      = ImGui::GetIO().DeltaTime;
    ImDrawList* dl      = ImGui::GetWindowDrawList();
    bool        changed = false;

    ImVec2      pos   = ImGui::GetCursorScreenPos();
    ImGuiStyle& style = ImGui::GetStyle();

    float thumb_radius = 8.0f;

    ImVec2 label_size = ImGui::CalcTextSize(label.c_str(), NULL, true);
    bool   AnimatedSlider(const std::string& label, float* slider_value, float slider_min, float slider_max,
                          const char* format = "%.2f", float labelWidthOverride = 0.0f);
    if (labelWidthOverride > 0.0f)
        label_size.x = labelWidthOverride;

    label_size.x += label_size.x > 0.0f ? thumb_radius : 0.0f;
    const float slider_width  = ImGui::CalcItemWidth() - label_size.x;
    float       slider_height = label_size.y / 4.0f + style.FramePadding.y * 2.0f;

    char value_text[16];
    snprintf(value_text, sizeof(value_text), format, (*slider_value));
    ImVec2 value_size = ImGui::CalcTextSize(value_text);

    const ImRect frame_bb(pos, pos + ImVec2(slider_width, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(pos, pos + ImVec2(slider_width +
                                                (value_size.x > 0.0f ? value_size.x + style.ItemInnerSpacing.x : 0.0f) +
                                                (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f),
                                            label_size.y + style.FramePadding.y * 2.0f));

    ImGui::PushID(label.c_str());

    ImVec2 label_pos = ImVec2(pos.x, pos.y + (((total_bb.Max.y - total_bb.Min.y) - label_size.y) / 2.0f));
    // Label
    dl->AddText(label_pos, ImGui::GetColorU32(ImGuiCol_Text), label.c_str());

    // Track position
    float track_x = pos.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f);
    float track_y = label_pos.y + ImGui::GetFontSize() * 0.5f - slider_height * 0.5f;

    // Track background
    dl->AddRectFilled(ImVec2(track_x, track_y), ImVec2(track_x + slider_width, track_y + slider_height),
                      ImGui::GetColorU32(ImGuiCol_TextDisabled), slider_height * 0.5f);

    bool hovered = ImGui::IsItemHovered() || ImGui::IsItemActive();

    float relativeValue = (*slider_value) / slider_max;

    // Filled portion with glow
    float       fill_width  = relativeValue * slider_width;
    const ImU32 frame_color = ImGui::GetColorU32(ImGui::GetCurrentContext()->ActiveId == id ? ImGuiCol_FrameBgActive
                                                 : hovered                                  ? ImGuiCol_FrameBgHovered
                                                                                            : ImGuiCol_FrameBg);
    dl->AddRectFilled(ImVec2(track_x, track_y), ImVec2(track_x + fill_width, track_y + slider_height), frame_color,
                      slider_height * 0.5f);

    // Thumb position
    float thumb_x = track_x + fill_width;
    float thumb_y = track_y + slider_height * 0.5f;

    ImGui::SetCursorScreenPos(ImVec2(track_x - thumb_radius, track_y - thumb_radius));
    ImGui::InvisibleButton("##slider", ImVec2(slider_width + thumb_radius * 1.5f, slider_height + thumb_radius * 2));

    if (ImGui::IsItemActive())
    {
        float mouse_x       = ImGui::GetIO().MousePos.x;
        float relativeValue = ImClamp((mouse_x - track_x) / slider_width, 0.0f, 1.0f);

        *slider_value = slider_min + relativeValue * (slider_max - slider_min);

        changed = true;

        fill_width = relativeValue * slider_width;
        thumb_x    = track_x + fill_width;
        thumb_y    = track_y + slider_height * 0.5f;
    }
    bool active = ImGui::IsItemActive();

    hovered = ImGui::IsItemHovered() || active;

    // Animate thumb scale
    float target_scale = hovered ? 1.3f : 1.0f;
    float thumb_scale  = iam_tween_float(id, ImHashStr("scale"), target_scale, 0.15f,
                                         iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

    // Draw thumb glow when hovered
    if (thumb_scale > 1.1f)
    {
        dl->AddCircleFilled(ImVec2(thumb_x, thumb_y), thumb_radius * thumb_scale * 1.5f, IM_COL32(255, 255, 255, 30));
    }

    // Thumb
    dl->AddCircleFilled(ImVec2(thumb_x, thumb_y), thumb_radius * thumb_scale,
                        ImGui::GetColorU32(active ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab));
    dl->AddCircle(ImVec2(thumb_x, thumb_y), thumb_radius * thumb_scale, frame_color, 0, 2.0f);

    // Value text
    dl->AddText(ImVec2(track_x + thumb_radius + slider_width + style.ItemInnerSpacing.x, label_pos.y),
                ImGui::GetColorU32(ImGuiCol_Text), value_text);

    ImGui::PopID();

    ImGui::SetCursorScreenPos(ImVec2(total_bb.Min.x, total_bb.Max.y));
    ImGui::Dummy(ImVec2(1, 1));

    return changed;
}

std::map<ImU32, AnimatedComboState> comboAnimTimes;

bool Ui::AnimatedCombo(const std::string& label, int* value, std::vector<std::string> items)
{
    ImU32 animId  = ImHashStr(label.c_str());
    bool  changed = false;

    float       dt  = ImGui::GetIO().DeltaTime;
    ImDrawList* dl  = ImGui::GetWindowDrawList();
    ImDrawList* tdl = ImGui::GetForegroundDrawList();

    ImGuiStyle& style = ImGui::GetStyle();

    int item_count = items.size();

    auto [it, inserted] = comboAnimTimes.try_emplace(animId, AnimatedComboState());
    auto& animState     = it->second;
    bool  valueStart    = *value;

    if (animState.open)
        animState.open_time += dt;
    else
        animState.open_time = 0.0f;

    ImVec2 pos         = ImGui::GetCursorScreenPos();
    float  btn_width   = ImGui::CalcItemWidth();
    float  item_height = ImGui::GetTextLineHeight();
    float  btn_height  = item_height + style.FramePadding.y * 2.0f;
    ImVec2 label_size  = ImGui::CalcTextSize(label.c_str());

    const ImRect total_bb(
        pos, pos + ImVec2(btn_width + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f),
                          label_size.y + style.FramePadding.y * 2.0f));

    ImGui::PushID(label.c_str());
    // Dropdown button
    ImGui::InvisibleButton("dropdown_btn", ImVec2(btn_width, btn_height));
    if (ImGui::IsItemClicked())
        animState.open = !animState.open;

    dl->AddRectFilled(pos, ImVec2(pos.x + btn_width, pos.y + btn_height), IM_COL32(60, 65, 75, 255), 4);
    dl->AddText(ImVec2(pos.x + 10, pos.y + (btn_height - ImGui::GetFontSize()) * 0.5f), IM_COL32(255, 255, 255, 255),
                items[*value].c_str());

    // Arrow
    float arrow_x   = pos.x + btn_width - 20;
    float arrow_y   = pos.y + btn_height * 0.5f;
    float arrow_rot = iam_tween_float(animId, ImHashStr("ar"), animState.open ? 3.14159f : 0.0f, 0.2f,
                                      iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);
    dl->AddTriangleFilled(ImVec2(arrow_x - 5, arrow_y - 3 * (animState.open ? -1 : 1)),
                          ImVec2(arrow_x + 5, arrow_y - 3 * (animState.open ? -1 : 1)),
                          ImVec2(arrow_x, arrow_y + 5 * (animState.open ? -1 : 1)), IM_COL32(180, 180, 190, 255));

    // Dropdown menu
    float menu_height = iam_tween_float(animId, ImHashStr("mh"), animState.open ? item_count * item_height : 0.0f,
                                        0.25f, iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

    if (menu_height > 1.0f)
    {
        ImVec2 menu_pos(pos.x, pos.y + btn_height + 2);
        tdl->AddRectFilled(menu_pos, ImVec2(menu_pos.x + btn_width, menu_pos.y + menu_height),
                           IM_COL32(50, 55, 65, 255), 4);

        for (int i = 0; i < item_count; i++)
        {
            float item_y = menu_pos.y + i * item_height;
            if (item_y + item_height > menu_pos.y + menu_height)
                break;

            // Staggered fade in
            float item_alpha  = ImClamp((animState.open_time - i * 0.05f) * 5.0f, 0.0f, 1.0f);
            float item_offset = (1.0f - item_alpha) * 10;

            ImVec2 item_pos(menu_pos.x + 10 + item_offset, item_y + (item_height - ImGui::GetFontSize()) * 0.5f);
            ImGui::SetCursorScreenPos(item_pos);
            ImGui::InvisibleButton(fmt::format("##menuitem{}", i).c_str(), ImVec2(btn_width, item_height));

            bool active = ImGui::IsItemActive();

            tdl->AddText(item_pos, IM_COL32(200, 200, 210, (int)(item_alpha * 255)), items[i].c_str());

            if (active && ImGui::IsMouseClicked(0))
            {
                *value         = i;
                changed        = true;
                animState.open = false;
                break;
            }
        }
    }

    // Label  text
    dl->AddText(ImVec2(pos.x + btn_width + style.ItemInnerSpacing.x, pos.y), ImGui::GetColorU32(ImGuiCol_Text),
                label.c_str());

    ImGui::PopID();

    ImGui::SetCursorScreenPos(ImVec2(total_bb.Min.x, total_bb.Max.y));
    ImGui::Dummy(ImVec2(1, 1));

    return changed;
}

void Ui::RenderSettingsPanel()
{
    ImGui::PushFont(mq::imgui::LargeTextFont);
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "Nameplate Settings");
    ImGui::Separator();
    ImGui::PopFont();

    float       dt = GetUIDeltaTime();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    static int active_tab = 0;

    struct TabData
    {
        int                   idx;
        std::string           name;
        std::function<void()> content;
    };

    std::vector<TabData> tabs = {
        {0, "Targeting",
         []()
         {
             bool renderForSelf = Settings.GetRenderForSelf();
             if (Ui::AnimatedCheckbox("Render For Self", &renderForSelf))
                 Settings.SetRenderForSelf(renderForSelf);

             bool renderForGroup = Settings.GetRenderForGroup();
             if (Ui::AnimatedCheckbox("Render For Group", &renderForGroup))
                 Settings.SetRenderForGroup(renderForGroup);

             bool renderForTarget = Settings.GetRenderForTarget();
             if (Ui::AnimatedCheckbox("Render For Target", &renderForTarget))
                 Settings.SetRenderForTarget(renderForTarget);

             bool renderForAllHaters = Settings.GetRenderForAllHaters();
             if (Ui::AnimatedCheckbox("Render For All Haters", &renderForAllHaters))
                 Settings.SetRenderForAllHaters(renderForAllHaters);
         }},
        {1, "Look and Feel",
         []()
         {
             Ui::AnimatedNameplatesSettings::HPBarStyle hpBarStyle = Settings.GetHPBarStyle();
             if (Ui::AnimatedCombo("HP Bar Style", reinterpret_cast<int*>(&hpBarStyle),
                                   {"Solid Red", "Con Color", "Color Range"}))
                 Settings.SetHPBarStyle(hpBarStyle);

             ImGui::NewLine();

             bool showClass = Settings.GetShowClass();
             if (Ui::AnimatedCheckbox("Show Class", &showClass))
                 Settings.SetShowClass(showClass);

             bool shortClassName = Settings.GetShortClassName();
             if (Ui::AnimatedCheckbox("Short Class Name", &shortClassName))
                 Settings.SetShortClassName(shortClassName);

             bool showLevel = Settings.GetShowLevel();
             if (Ui::AnimatedCheckbox("Show Level", &showLevel))
                 Settings.SetShowLevel(showLevel);

             bool showGuild = Settings.GetShowGuild();
             if (Ui::AnimatedCheckbox("Show Guild", &showGuild))
                 Settings.SetShowGuild(showGuild);

             bool showPurpose = Settings.GetShowPurpose();
             if (Ui::AnimatedCheckbox("Show Purpose", &showPurpose))
                 Settings.SetShowPurpose(showPurpose);

             bool showBuffIcons = Settings.GetShowBuffIcons();
             if (Ui::AnimatedCheckbox("Show Buff Icons", &showBuffIcons))
                 Settings.SetShowBuffIcons(showBuffIcons);
         }},
        {2, "Size & Positioning",
         []()
         {
             float       sliderLabelWidth      = ImGui::CalcTextSize("Nameplate Height Offset").x;
             const char* valueFormat           = "%.1f";
             float       nameplateHeightOffset = Settings.GetNameplateHeightOffset();
             if (Ui::AnimatedSlider("Nameplate Height Offset", &nameplateHeightOffset, 0.0f, 300.0f, valueFormat,
                                    sliderLabelWidth))
                 Settings.SetNameplateHeightOffset(nameplateHeightOffset);

             float nameplateWidth = Settings.GetNameplateWidth();
             if (Ui::AnimatedSlider("Nameplate Width", &nameplateWidth, 25.0f, 800.0f, valueFormat, sliderLabelWidth))
                 Settings.SetNameplateWidth(nameplateWidth);

             float fontSize = Settings.GetFontSize();
             if (Ui::AnimatedSlider("Font Size", &fontSize, 1.0f, 40.0f, valueFormat, sliderLabelWidth))
                 Settings.SetFontSize(fontSize);

             float iconSize = Settings.GetIconSize();
             if (Ui::AnimatedSlider("Icon Size", &iconSize, 10.0f, 40.0f, valueFormat, sliderLabelWidth))
                 Settings.SetIconSize(iconSize);

             float barRounding = Settings.GetBarRounding();
             if (Ui::AnimatedSlider("Bar Rounding", &barRounding, 0.0f, 10.0f, valueFormat, sliderLabelWidth))
                 Settings.SetBarRounding(barRounding);

             float barBorderThickness = Settings.GetBarBorderThickness();
             if (Ui::AnimatedSlider("Bar Border Thickness", &barBorderThickness, 0.0f, 5.0f, valueFormat,
                                    sliderLabelWidth))
                 Settings.SetBarBorderThickness(barBorderThickness);

             ImGui::NewLine();

             bool renderToForeground = Settings.GetRenderToForeground();
             if (Ui::AnimatedCheckbox("Always on Top", &renderToForeground))
                 Settings.SetRenderToForeground(renderToForeground);

             bool renderNoLOS = Settings.GetRenderNoLOS();
             if (Ui::AnimatedCheckbox("Render Even When Occluded", &renderNoLOS))
                 Settings.SetRenderNoLOS(renderNoLOS);
         }},
        {3, "Dev & Debug",
         []()
         {
             bool showDebugPanel = Settings.GetShowDebugPanel();
             if (Ui::AnimatedCheckbox("Show Debug Panel", &showDebugPanel))
                 Settings.SetShowDebugPanel(showDebugPanel);
         }},
    };

    ImVec2 tabs_pos    = ImGui::GetCursorScreenPos();
    float  tab_height  = 36.0f;
    float  total_width = ImGui::GetContentRegionAvail().x;
    float  tab_width   = floor(total_width / tabs.size());

    // Draw tab background
    dl->AddRectFilled(tabs_pos, ImVec2(tabs_pos.x + total_width, tabs_pos.y + tab_height), IM_COL32(35, 38, 48, 255),
                      4.0f, ImDrawFlags_RoundCornersTop);

    // Calculate target indicator position
    float target_x = tabs_pos.x;
    for (int i = 0; i < active_tab; i++)
        target_x += tab_width;
    float target_width = tab_width;

    // Animate indicator with spring
    ImGuiID id              = ImGui::GetID("animnp_tab_indicator");
    float   indicator_x     = iam_tween_float(id, ImHashStr("x"), target_x, 0.3f,
                                              iam_ease_spring_desc(1.0f, 180.0f, 18.0f, 0.0f), iam_policy_crossfade, dt);
    float   indicator_width = iam_tween_float(id, ImHashStr("w"), target_width, 0.25f,
                                              iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

    // Draw tabs
    float x = tabs_pos.x;
    for (auto tab : tabs)
    {
        ImVec2 tab_min(x, tabs_pos.y);
        ImVec2 tab_max(x + tab_width, tabs_pos.y + tab_height);

        // Invisible button
        ImGui::SetCursorScreenPos(tab_min);
        std::string btn_id = fmt::format("##animnp_tab{}", tab.name);
        if (ImGui::InvisibleButton(btn_id.c_str(), ImVec2(tab_width, tab_height)))
            active_tab = tab.idx;

        bool hovered = ImGui::IsItemHovered();

        // Text color animation
        ImGuiID tab_id       = ImGui::GetID(tab.name.c_str());
        float   target_alpha = (tab.idx == active_tab) ? 1.0f : (hovered ? 0.8f : 0.5f);
        float   text_alpha   = iam_tween_float(tab_id, ImHashStr("animnp_alpha"), target_alpha, 0.15f,
                                               iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

        // Draw text
        ImVec2 text_size = ImGui::CalcTextSize(tab.name.c_str());
        ImVec2 text_pos(x + (tab_width - text_size.x) * 0.5f, tabs_pos.y + (tab_height - text_size.y) * 0.5f);
        dl->AddText(text_pos, IM_COL32(255, 255, 255, (int)(text_alpha * 255)), tab.name.c_str());

        x += tab_width;
    }

    // Draw animated indicator
    float indicator_y = tabs_pos.y + tab_height - 3.0f;
    dl->AddRectFilled(ImVec2(indicator_x + 8.0f, indicator_y),
                      ImVec2(indicator_x + indicator_width - 8.0f, indicator_y + 3.0f), IM_COL32(91, 194, 231, 255),
                      2.0f);

    // Content area with fade
    ImVec2 content_pos(tabs_pos.x, tabs_pos.y + tab_height + Ui::Settings.GetPadding().y);
    ImVec2 content_size(total_width, ImGui::GetContentRegionAvail().y - tab_height);

    dl->AddRectFilled(content_pos, ImVec2(content_pos.x + content_size.x, content_pos.y + content_size.y),
                      IM_COL32(30, 32, 40, 255), 4.0f);

    // Animate content alpha
    float content_alpha = iam_tween_float(id, ImHashStr("animmp_content"), 1.0f, 0.2f,
                                          iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

    ImGui::SetCursorScreenPos(ImVec2(tabs_pos.x, content_pos.y + Ui::Settings.GetPadding().y * 2.0f));
    ImGui::Indent(Ui::Settings.GetPadding().x);
    tabs[active_tab].content();
    ImGui::Unindent(Ui::Settings.GetPadding().x);

    ImGui::SetCursorScreenPos(ImVec2(tabs_pos.x, content_pos.y + content_size.y + Ui::Settings.GetPadding().y));
    ImGui::Dummy(ImVec2(0.0f, 0.0f));
}

void Ui::AnimatedNameplatesSettings::LoadSettings()
{
    m_configFile = (std::filesystem::path(gPathConfig) / "MQAnimatedNameplates.yaml").string();

    try
    {
        m_configNode = YAML::LoadFile(m_configFile);

        m_showBuffIcons         = m_configNode["ShowBuffIcons"].as<bool>(m_showBuffIcons);
        m_showDebugPanel        = m_configNode["ShowDebugPanel"].as<bool>(m_showDebugPanel);
        m_fontSize              = m_configNode["FontSize"].as<float>(m_fontSize);
        m_iconSize              = m_configNode["IconSize"].as<float>(m_iconSize);
        m_barRounding           = m_configNode["BarRounding"].as<float>(m_barRounding);
        m_barBorderThickness    = m_configNode["BarBorderThickness"].as<float>(m_barBorderThickness);
        m_renderForSelf         = m_configNode["RenderForSelf"].as<bool>(m_renderForSelf);
        m_renderForTarget       = m_configNode["RenderForTarget"].as<bool>(m_renderForTarget);
        m_renderForGroup        = m_configNode["RenderForGroup"].as<bool>(m_renderForGroup);
        m_renderForAllHaters    = m_configNode["RenderForAllHaters"].as<bool>(m_renderForAllHaters);
        m_renderNoLOS           = m_configNode["RenderNoLOS"].as<bool>(m_renderNoLOS);
        m_nameplateWidth        = m_configNode["NameplateWidth"].as<float>(m_nameplateWidth);
        m_showGuild             = m_configNode["ShowGuild"].as<bool>(m_showGuild);
        m_showPurpose           = m_configNode["ShowPurpose"].as<bool>(m_showPurpose);
        m_renderToForeground    = m_configNode["RenderToForeground"].as<bool>(m_renderToForeground);
        m_shortClassName        = m_configNode["ShortClassName"].as<bool>(m_shortClassName);
        m_showClass             = m_configNode["ShowClass"].as<bool>(m_showClass);
        m_showLevel             = m_configNode["ShowLevel"].as<bool>(m_showLevel);
        m_nameplateHeightOffset = m_configNode["NameplateHeightOffset"].as<float>(m_nameplateHeightOffset);
        m_hpBarStyle = static_cast<HPBarStyle>(m_configNode["HPBarStyle"].as<int>(static_cast<int>(m_hpBarStyle)));

        m_padding =
            ImVec2(m_configNode["PaddingX"].as<float>(m_padding.x), m_configNode["PaddingY"].as<float>(m_padding.y));
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
void Ui::AddRectFilledMultiColorRounded(ImDrawList& draw_list, const ImVec2& p_min, const ImVec2& p_max,
                                        ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right,
                                        ImU32 col_bot_left, float rounding, ImDrawFlags flags)
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

    const float w     = (p_max.x - p_min.x);
    const float h     = (p_max.y - p_min.y);
    const float inv_w = (w != 0.0f) ? (1.0f / w) : 0.0f;
    const float inv_h = (h != 0.0f) ? (1.0f / h) : 0.0f;

    const ImVec2 uv = draw_list._Data->TexUvWhitePixel;

    // Reserve geometry:
    // - Inner fill uses (points_count - 2) triangles => (points_count - 2) * 3 indices
    // - AA fringe uses points_count quads => points_count * 6 indices
    // - For each outline point we emit 2 vertices (inner/outer)
    const float aa_size   = draw_list._FringeScale;
    const int   idx_count = (points_count - 2) * 3 + points_count * 6;
    const int   vtx_count = points_count * 2;
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

        const ImVec4 top       = ImLerp(ul, ur, u);
        const ImVec4 bot       = ImLerp(bl, br, u);
        const ImU32  col       = ImGui::ColorConvertFloat4ToU32(ImLerp(top, bot, v));
        const ImU32  col_trans = col & ~IM_COL32_A_MASK;

        // Write inner + outer vertices
        ImDrawVert& vtx_inner = draw_list._VtxWritePtr[0];
        ImDrawVert& vtx_outer = draw_list._VtxWritePtr[1];

        vtx_inner.pos = ImVec2(p.x - nx, p.y - ny);
        vtx_inner.uv  = uv;
        vtx_inner.col = col;

        vtx_outer.pos = ImVec2(p.x + nx, p.y + ny);
        vtx_outer.uv  = uv;
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
