#include "Widgets.h"
#include "ConfigVariable.h"
#include "Config.h"

#include "fmt/format.h"
#include "imgui/ImGuiUtils.h"
#include "imgui/imanim/im_anim.h"

namespace Ui {

static std::unordered_map<ImGuiID, AnimatedCheckmark> s_checkBoxAnims;
static std::unordered_map<ImU32, AnimatedComboState> s_comboAnimTimes;
static std::unordered_map<ImU32, AnimatedInlineConfirmState> s_confirmStates;

static const ImGuiID scale_id = ImHashStr("scale");
static const ImGuiID arrow_rot_id = ImHashStr("arrow_rot");
static const ImGuiID menu_height_id = ImHashStr("menu_height");

void AnimatedCheckmark::Reset(bool newVal)
{
    m_path1Time = 0.0f;
    m_path2Time = 0.0f;

    if (newVal != m_newValue)
    {
        m_newValue = newVal;

        m_pathInitialized = false;
        m_path1Complete = false;
        m_path2Complete = false;
    }
    else
    {
        // already in the correct state, so just mark the paths as complete
        m_pathInitialized = true;
        m_path1Complete = true;
        m_path2Complete = true;
    }
}

void AnimatedCheckmark::Render(ImDrawList* dl, const ImRect& check_bb, float box_size)
{
    float dt = ImGui::GetIO().DeltaTime;

    ImU32 check_col = ImGui::GetColorU32(ImGuiCol_CheckMark);

    const float pad = ImMax(1.0f, IM_TRUNC(box_size / 6.0f));
    float       sz = box_size - pad * 2.0f;

    float thickness = ImMax(sz / 5.0f, 1.0f);
    sz -= thickness * 0.5f;

    ImVec2 pos = check_bb.Min + ImVec2(pad, pad);
    pos += ImVec2(thickness * 0.25f, thickness * 0.25f);

    float third = sz / 3.0f;
    float bx = pos.x + third;
    float by = pos.y + sz - third * 0.5f;

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

bool AnimatedCheckbox(const char* label, bool* value)
{
    ImGui::PushID(label);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImVec2 box_pos = ImGui::GetCursorScreenPos();

    float        box_size = ImGui::GetFrameHeight();
    const ImRect check_bb(box_pos, box_pos + ImVec2(box_size, box_size));
    float        line_height = ImGui::GetTextLineHeight();

    ImVec2      label_size = ImGui::CalcTextSize(label);
    ImGuiStyle& style = ImGui::GetStyle();

    ImU32 anim_id = ImGui::GetID("##anim");

    auto [it, inserted] = s_checkBoxAnims.try_emplace(anim_id, *value, label);
    auto& animState = it->second;

    ImGui::SetCursorScreenPos(box_pos);

    // interactions
    bool         hovered, held;
    const ImRect total_bb(
        box_pos, box_pos + ImVec2(box_size + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f),
            label_size.y + style.FramePadding.y * 2.0f));

    const bool is_visible = ImGui::ItemAdd(total_bb, anim_id);
    bool pressed = ImGui::ButtonBehavior(total_bb, anim_id, &hovered, &held);

    if (pressed || inserted)
    {
        if (pressed)
            (*value) = !(*value);

        animState.Reset(*value);
    }

    // Box background
    ImU32 box_bg;

    if (held && hovered)
        box_bg = ImGui::GetColorU32(ImGuiCol_FrameBgActive);
    else if (hovered)
        box_bg = ImGui::GetColorU32(ImGuiCol_FrameBgHovered);
    else
        box_bg = ImGui::GetColorU32(ImGuiCol_FrameBg);
    const float border_size = style.FrameBorderSize;
    ImVec2 center = (check_bb.Min + check_bb.Max) * 0.5f;

    dl->AddRectFilled(check_bb.Min, check_bb.Max, box_bg, style.FrameRounding);
    dl->AddRect(check_bb.Min + ImVec2(1, 1), check_bb.Max + ImVec2(1, 1), ImGui::GetColorU32(ImGuiCol_BorderShadow),
        style.FrameRounding, 0, border_size);
    dl->AddRect(check_bb.Min, check_bb.Max, ImGui::GetColorU32(ImGuiCol_Border), style.FrameRounding);

    // Draw animated checkmark
    animState.Render(dl, check_bb, box_size);

    // Label
    const ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
    dl->AddText(label_pos, ImGui::GetColorU32(ImGuiCol_Text), label);

    ImGui::PopID();

    ImGui::SetCursorScreenPos(ImVec2(total_bb.Min.x, total_bb.Max.y));
    ImGui::Dummy(ImVec2(1, 1));

    return pressed;
}

template <typename T>
struct ImGuiDataTypeTraits {};
template <> struct ImGuiDataTypeTraits<int8_t> { static constexpr ImGuiDataType Type = ImGuiDataType_S8; };
template <> struct ImGuiDataTypeTraits<uint8_t> { static constexpr ImGuiDataType Type = ImGuiDataType_U8; };
template <> struct ImGuiDataTypeTraits<int16_t> { static constexpr ImGuiDataType Type = ImGuiDataType_S16; };
template <> struct ImGuiDataTypeTraits<uint16_t> { static constexpr ImGuiDataType Type = ImGuiDataType_U16; };
template <> struct ImGuiDataTypeTraits<int32_t> { static constexpr ImGuiDataType Type = ImGuiDataType_S32; };
template <> struct ImGuiDataTypeTraits<uint32_t> { static constexpr ImGuiDataType Type = ImGuiDataType_U32; };
template <> struct ImGuiDataTypeTraits<int64_t> { static constexpr ImGuiDataType Type = ImGuiDataType_S64; };
template <> struct ImGuiDataTypeTraits<uint64_t> { static constexpr ImGuiDataType Type = ImGuiDataType_U64; };
template <> struct ImGuiDataTypeTraits<float> { static constexpr ImGuiDataType Type = ImGuiDataType_Float; };
template <> struct ImGuiDataTypeTraits<double> { static constexpr ImGuiDataType Type = ImGuiDataType_Double; };


template <typename T> requires std::is_integral_v<T> || std::is_floating_point_v<T>
bool AnimatedSliderImpl(const char* label, T * slider_value, T slider_min, T slider_max, const char* format, float width)
{
    const ImGuiID id = ImGui::GetID(label);

    float dt = ImGui::GetIO().DeltaTime;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    bool changed = false;

    if (*slider_value > slider_max)
    {
        *slider_value = slider_max;
        changed = true;
    }
    if (*slider_value < slider_min)
    {
        *slider_value = slider_min;
        changed = true;
    }

    ImVec2      pos = ImGui::GetCursorScreenPos();
    ImGuiStyle& style = ImGui::GetStyle();

    float thumb_radius = 8.0f;

    ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);

    if (width > 0.0f)
        label_size.x = width;

    label_size.x += label_size.x > 0.0f ? thumb_radius : 0.0f;
    const float slider_width = ImGui::CalcItemWidth() - label_size.x;
    float       slider_height = label_size.y / 4.0f + style.FramePadding.y * 2.0f;

    char value_text[16];
    snprintf(value_text, sizeof(value_text), format, *slider_value);
    ImVec2 value_size = ImGui::CalcTextSize(value_text);

    const ImRect total_bb(pos, pos + ImVec2(slider_width + (value_size.x > 0.0f ? value_size.x + style.ItemInnerSpacing.x : 0.0f)
        + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));

    ImGui::PushID(label);

    ImVec2 label_pos = ImVec2(pos.x, pos.y + (((total_bb.Max.y - total_bb.Min.y) - label_size.y) / 2.0f));
    // Label
    dl->AddText(label_pos, ImGui::GetColorU32(ImGuiCol_Text), label);

    // Track position
    float track_x = pos.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f);
    float track_y = label_pos.y + ImGui::GetFontSize() * 0.5f - slider_height * 0.5f;

    // Track background
    dl->AddRectFilled(ImVec2(track_x, track_y), ImVec2(track_x + slider_width, track_y + slider_height),
        ImGui::GetColorU32(ImGuiCol_TextDisabled), slider_height * 0.5f);

    bool hovered = ImGui::IsItemHovered() || ImGui::IsItemActive();

    float range = static_cast<float>(slider_max) - static_cast<float>(slider_min);
    float relativeValue = (static_cast<float>(*slider_value) - static_cast<float>(slider_min)) / range;

    // Filled portion with glow
    float       fill_width = relativeValue * slider_width;
    const ImU32 frame_color = ImGui::GetColorU32(ImGui::GetCurrentContext()->ActiveId == id ? ImGuiCol_FrameBgActive
        : hovered ? ImGuiCol_FrameBgHovered
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
        float mouse_x = ImGui::GetIO().MousePos.x;
        float relativeValue2 = ImClamp((mouse_x - track_x) / slider_width, 0.0f, 1.0f);

        *slider_value = static_cast<T>(slider_min + relativeValue2 * (slider_max - slider_min));

        changed = true;

        fill_width = relativeValue2 * slider_width;
        thumb_x = track_x + fill_width;
        thumb_y = track_y + slider_height * 0.5f;
    }

    bool active = ImGui::IsItemActive();
    hovered = ImGui::IsItemHovered() || active;

    // Animate thumb scale
    float target_scale = hovered ? 1.3f : 1.0f;
    float thumb_scale = iam_tween_float(id, scale_id, target_scale, 0.15f,
        iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt, 1.0f);

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
    ImGui::SetCursorScreenPos(ImVec2(track_x + thumb_radius + slider_width + style.ItemInnerSpacing.x, label_pos.y));
    ImGui::SetNextItemWidth(75);
    if (ImGui::InputScalar("##input_value", ImGuiDataTypeTraits<T>::Type, slider_value, nullptr, nullptr, format))
        changed = true;

    ImGui::PopID();

    ImGui::SetCursorScreenPos(ImVec2(total_bb.Min.x, total_bb.Max.y));
    ImGui::Dummy(ImVec2(1, 1));

    return changed;
}

bool AnimatedSlider(const char* label, float* slider_value, float slider_min, float slider_max,
    const char* format, float width)
{
    return AnimatedSliderImpl(label, slider_value, slider_min, slider_max, format, width);
}

bool AnimatedSlider(const char* label, int* slider_value, int slider_min, int slider_max,
    const char* format, float width)
{
    return AnimatedSliderImpl(label, slider_value, slider_min, slider_max, format, width);
}

bool AnimatedSlider(const char* label, unsigned int* slider_value, unsigned int slider_min, unsigned int slider_max,
    const char* format, float width)
{
    return AnimatedSliderImpl(label, slider_value, slider_min, slider_max, format, width);
}

template <typename T>
bool AnimatedComboImpl(const char* label, T* value, int item_count,
    std::function<T(int)>&& values_getter, std::function<const char* (int)>&& labels_getter)
{
    bool changed = false;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    if (ImGui::BeginCombo(label, labels_getter(*value)))
    {
        for (int i = 0; i < item_count; i++)
        {
            bool is_selected = (*value == values_getter(i));
            if (ImGui::Selectable(labels_getter(i), is_selected))
            {
                *value = values_getter(i);
                changed = true;
                break;
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::PopStyleVar();
    return changed;
}

bool AnimatedCombo(const char* label, int* value, const std::vector<std::string>& items)
{
    return AnimatedComboImpl<int>(label, value, static_cast<int>(items.size()),
        [](int index) { return index; }, [&items](int index) { return items[index].c_str(); });
}

template <typename T> requires std::is_enum_v<T>
bool AnimatedEnumCombo(const char* label, T* value)
{
    static const std::vector<std::pair<T, std::string>>& valuesMapping = config_enum_traits<T>::values();

    return AnimatedComboImpl<T>(label, value, static_cast<int>(valuesMapping.size()),
        [](int index) { return valuesMapping[index].first; },
        [](int index) { return valuesMapping[index].second.c_str(); });
}

template bool AnimatedEnumCombo<HPBarStyle>(const char*, HPBarStyle*);

bool InlineConfirmButton(const char* buttonNormalText, const char* buttonAcceptText, const char* buttonCancelText, const float buttonHeight, char* inputText, size_t inputTextSize, ImU32 buttonColor, ImU32 buttonHighlightColor)
{
    const ImGuiID id = ImHashStr(buttonNormalText);

    auto [state, inserted] = s_confirmStates.try_emplace(id);

    auto& style = ImGui::GetStyle();
    float dt = ImGui::GetIO().DeltaTime;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    bool accepted = false;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    float padding = style.FramePadding.x;
    float gap = 15;  // Gap between Confirm and Cancel

    // Calculate text sizes for proper button widths
    ImVec2 delete_size = ImGui::CalcTextSize(buttonNormalText);
    ImVec2 confirm_size = ImGui::CalcTextSize(buttonAcceptText);
    ImVec2 cancel_size = ImGui::CalcTextSize(buttonCancelText);

    float collapsed_width = delete_size.x + padding * 2;
    float confirm_btn_width = confirm_size.x + padding * 2;
    float cancel_btn_width = cancel_size.x + padding * 2;
    float expanded_total = confirm_btn_width + gap + cancel_btn_width;

    ImVec4 textColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);

    // Width animation
    float target_width = state->second.confirming ? expanded_total : collapsed_width;
    float animated_width = iam_tween_float(ImGui::GetID("conf_w"), ImHashStr("cw"),
        target_width, 0.2f, iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt, collapsed_width);

    // Background
    ImU32 bg_col = state->second.confirming ? buttonHighlightColor : buttonColor;
    
    if (!state->second.confirming)
    {
        dl->AddRectFilled(pos, ImVec2(pos.x + animated_width, pos.y + buttonHeight), bg_col, 4);

        // Delete button hover detection
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("delete_btn", ImVec2(animated_width, buttonHeight));
        bool del_hovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) state->second.confirming = true;

        // Animate delete button hover
        state->second.delete_hover = iam_tween_float(ImGui::GetID("del_h"), ImHashStr("dh"),
            del_hovered ? 1.0f : 0.0f, 0.15f, iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

        // Hover highlight
        if (state->second.delete_hover > 0.01f)
        {
            dl->AddRectFilled(pos, ImVec2(pos.x + animated_width, pos.y + buttonHeight),
                IM_COL32(textColor.x, textColor.y, textColor.z, static_cast<int>(30 * state->second.delete_hover)), 4);
        }

        // Center "Delete" text in button
        float text_x = pos.x + (animated_width - delete_size.x) * 0.5f;
        dl->AddText(ImVec2(text_x, pos.y + (buttonHeight - delete_size.y) * 0.5f),
            textColor.ToImU32(), buttonNormalText);
    }
    else
    {
        ImVec2 mouse = ImGui::GetMousePos();
        // input text box.
        int inputLen = 0;
        float inputWidth = 0.0f;

        if (inputText)
        {
            inputWidth = ImGui::GetContentRegionAvail().x * .45f;
            ImGui::SetNextItemWidth(inputWidth);
            ImGui::InputText("##animatedInlineConfirmText", inputText, inputTextSize);
            inputLen = strlen(inputText);
        }

        pos.x += inputWidth;
        float cancel_start_x = pos.x + confirm_btn_width + gap;

        dl->AddRectFilled(pos, ImVec2(pos.x + animated_width, pos.y + buttonHeight), bg_col, 4);

        // Check hover for each button
        bool confirm_hovered = (mouse.x >= pos.x && mouse.x < pos.x + confirm_btn_width &&
            mouse.y >= pos.y && mouse.y < pos.y + buttonHeight);
        bool cancel_hovered = (mouse.x >= cancel_start_x && mouse.x < cancel_start_x + cancel_btn_width &&
            mouse.y >= pos.y && mouse.y < pos.y + buttonHeight);

        // Animate hover states
        state->second.confirm_hover = iam_tween_float(ImGui::GetID("conf_h"), ImHashStr("ch"),
            confirm_hovered ? 1.0f : 0.0f, 0.15f, iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);
        state->second.cancel_hover = iam_tween_float(ImGui::GetID("canc_h"), ImHashStr("cah"),
            cancel_hovered ? 1.0f : 0.0f, 0.15f, iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

        // Confirm button hover highlight
        if (state->second.confirm_hover > 0.01f)
        {
            dl->AddRectFilled(pos, ImVec2(pos.x + confirm_btn_width, pos.y + buttonHeight),
                IM_COL32(textColor.x, textColor.y, textColor.z, static_cast<int>(40 * state->second.confirm_hover)), 4, ImDrawFlags_RoundCornersLeft);
        }

        // Cancel button hover highlight
        if (state->second.cancel_hover > 0.01f)
        {
            dl->AddRectFilled(ImVec2(cancel_start_x, pos.y),
                ImVec2(cancel_start_x + cancel_btn_width, pos.y + buttonHeight),
                IM_COL32(textColor.x, textColor.y, textColor.z, static_cast<int>(40 * state->second.cancel_hover)), 4, ImDrawFlags_RoundCornersRight);
        }

        // Center "Confirm" text in its section
        float confirm_text_x = pos.x + (confirm_btn_width - confirm_size.x) * 0.5f;
        dl->AddText(ImVec2(confirm_text_x, pos.y + (buttonHeight - confirm_size.y) * 0.5f),
            (inputLen > 0 || inputTextSize == 0) ? textColor.ToImU32() : ImGui::GetColorU32(ImGuiCol_TextDisabled), buttonAcceptText);

        // Center "Cancel" text in its section
        float cancel_text_x = cancel_start_x + (cancel_btn_width - cancel_size.x) * 0.5f;
        dl->AddText(ImVec2(cancel_text_x, pos.y + (buttonHeight - cancel_size.y) * 0.5f),
            textColor.ToImU32(), buttonCancelText);

        // Handle clicks
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("conf_area", ImVec2(animated_width, buttonHeight));
        if (ImGui::IsItemClicked())
        {
            if (mouse.x < pos.x + confirm_btn_width)
            {
                // ok only valid with input.
                if (inputLen > 0 || inputTextSize == 0)
                {
                    accepted = true;
                    state->second.confirming = false;
                }
            }
            else
            {
                // cancel always valid
                state->second.confirming = false;
            }
        }
    }
    
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + buttonHeight + style.ItemSpacing.y * 2.0f));
    ImGui::Dummy(ImVec2{ 1.0f, 1.0f });

    return accepted;
    
    return false;
}

} // namespace Ui