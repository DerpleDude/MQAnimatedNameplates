#include "SettingsPanel.h"
#include "Config.h"
#include "Widgets.h"
#include "Nameplate.h"

#include "imgui/ImGuiUtils.h"
#include "imgui/imgui_internal.h"
#include "imgui/imanim/im_anim.h"
#include "mq/Plugin.h"

#include <algorithm>
#include <cmath>

using namespace eqlib;

static const ImGuiID x_id = ImHashStr("x");
static const ImGuiID w_id = ImHashStr("w");


template <typename T> requires std::is_floating_point_v<T>
bool RenderOption(Ui::NumericConfigVariable<T>& variable, const char* labelText, float width = 0.0f, const char* format = "%.2f")
{
    T value = variable.get();
    if (Ui::AnimatedSlider(labelText, &value, variable.getMinValue(), variable.getMaxValue(), format, width))
    {
        variable.set(value);
        return true;
    }

    return false;
}

template <typename T> requires std::is_integral_v<T>
bool RenderOption(Ui::NumericConfigVariable<T>& variable, const char* labelText, float width = 0.0f, const char* format = "%d")
{
    T value = variable.get();
    if (Ui::AnimatedSlider(labelText, &value, variable.getMinValue(), variable.getMaxValue(), format, width))
    {
        variable.set(value);
        return true;
    }

    return false;
}

template <typename T> requires std::is_same_v<T, bool>
bool RenderOption(Ui::BasicConfigVariable<T>& variable, const char* labelText)
{
    bool value = variable.get();
    if (Ui::AnimatedCheckbox(labelText, &value))
    {
        variable.set(value);
        return true;
    }

    return false;
}

template <Ui::EnumTraitsConcept T>
bool RenderOption(Ui::ConfigVariable<T>& variable, const char* labelText)
{
    T value = variable.get();
    if (Ui::AnimatedEnumCombo(labelText, &value))
    {
        variable.set(value);
        return true;
    }

    return false;
}

template <typename T> requires std::is_same_v<T, mq::MQColor>
bool RenderOption(Ui::ConfigVariable<T>& variable, const char* labelText)
{
    ImVec4 color = variable.get().ToImColor();
    if (ImGui::ColorEdit3(labelText, &color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
    {
        variable.set(color);
        return true;
    }
    
    ImGui::SameLine();
    ImGui::TextUnformatted(labelText);

    return false;
}

class SettingsPanel
{
public:
    SettingsPanel()
    {
        tabs.emplace_back(0, "Targeting", [this]() { DrawTargetingTab(); });
        tabs.emplace_back(1, "Look and Feel", [this]() { DrawLookAndFeelTab(); });
        tabs.emplace_back(2, "Size and Positioning", [this]() { DrawSizeAndPositioningTab(); });
        tabs.emplace_back(3, "Dev and Debug", [this]() { DrawDevAndDebugTab(); });
    }

    void DrawTargetingTab()
    {
        Ui::Config& config = Ui::Config::Get();

        RenderOption(config.RenderForSelf, "Show For Self");
        RenderOption(config.RenderForGroup, "Show For Group");
        RenderOption(config.RenderForTarget, "Show For Target");
        RenderOption(config.RenderForAllHaters, "Show For All Haters");
        RenderOption(config.RenderForNPCs, "Show For All NPCs");
    }

    void DrawLookAndFeelTab()
    {
        Ui::Config& config = Ui::Config::Get();

        RenderOption(config.HPBarStyleSelf, "Self HP Bar Style");
        RenderOption(config.HPBarStyleTarget, "Target HP Bar Style");
        RenderOption(config.HPBarStyleGroup, "Group HP Bar Style");
        RenderOption(config.HPBarStyleHaters, "Haters HP Bar Style");
        RenderOption(config.HPBarStyleNPCs, "NPCs HP Bar Style");

        ImGui::NewLine();

        RenderOption(config.CustomColor1, "Custom Color 1");
        RenderOption(config.CustomColor2, "Custom Color 2");
        RenderOption(config.CustomColor3, "Custom Color 3");
        RenderOption(config.CustomColor4, "Custom Color 4");
        RenderOption(config.CustomColor5, "Custom Color 5");
        RenderOption(config.CustomColor6, "Custom Color 6");
        
        ImGui::NewLine();
        RenderOption(config.ColorAlphaModifier, "Color Alpha Modifier", 150, "%.2f");


        ImGui::NewLine();
        ImGui::Separator();
        ImGui::NewLine();

        RenderOption(config.ShowClass, "Show Class");
        ImGui::Indent();
        RenderOption(config.ShortClassName, "Short Class Name");
        ImGui::Unindent();

        RenderOption(config.ShowLevel, "Show Level");
        RenderOption(config.ShowGuild, "Show Guild");
        RenderOption(config.ShowPurpose, "Show Purpose");
        RenderOption(config.ShowBuffIcons, "Show Buff Icons");

        ImGui::NewLine();
        ImGui::Separator();
        ImGui::NewLine();
        
        RenderOption(config.ScaleWithDistance, "Scale With Distance");
        RenderOption(config.DrawBarBorders, "Draw Bar Borders");
        
        ImGui::NewLine();

        RenderOption(config.ShowTargetIndicator, "Show Target Indicator");
        RenderOption(config.TargetIndicatorPadding, "Indicator Padding", 150, "%.0f");
        RenderOption(config.TargetIndicatorBlinkSpeed, "Indicator Blink Speed", 150, "%.2f");
    }

    void DrawSizeAndPositioningTab()
    {
        Ui::Config& config = Ui::Config::Get();

        float sliderLabelWidth = ImGui::CalcTextSize("Nameplate Height Offset").x;

        RenderOption(config.NameplateWidth, "Nameplate Width", sliderLabelWidth, "%0.f");
        RenderOption(config.NameplateHeight, "Nameplate Height", sliderLabelWidth, "%0.f");

        RenderOption(config.NameplateHeightOffset, "Nameplate Height Offset", sliderLabelWidth, "%.0f");
        RenderOption(config.HPTicks, "HP Ticks Every [x]%", sliderLabelWidth);
        RenderOption(config.FontSize, "Font Size", sliderLabelWidth, "%.1f");
        RenderOption(config.IconSize, "Icon Size", sliderLabelWidth, "%.1f");
        RenderOption(config.BarRounding, "Bar Rounding", sliderLabelWidth, "%.1f");
        RenderOption(config.BarBorderThickness, "Bar Border Thickness", sliderLabelWidth, "%.1f");

        ImGui::NewLine();

        RenderOption(config.RenderToForeground, "Render To Foreground");
        RenderOption(config.RenderNoLOS, "Render Even When Occluded");
        RenderOption(config.RenderTargetNoLOS, "Render Target Even When Occluded");
        RenderOption(config.MaxDrawDistance, "Maximum Draw Distance");
    }

    void DrawDevAndDebugTab()
    {
        Ui::Config& config = Ui::Config::Get();
        float sliderLabelWidth = ImGui::CalcTextSize("Nameplate Height Adjustment").x;

        RenderOption(config.ShowDebugBounds, "Show Debug Bounding Box");
        RenderOption(config.ShowDebugText, "Show Debug Text");

        ImGui::NewLine();

        RenderOption(config.DrawTestBar, "Draw Test Bar");
        RenderOption(config.BarPercent, "Test Bar Percent", 0, "%0.f");

        ImGui::NewLine();

        RenderOption(config.ScaleFactorAdjustment, "Scale Factor Adjustment", sliderLabelWidth, "%.05f");
        RenderOption(config.NameplateHeightAdjust, "Nameplate Height Adjustment", sliderLabelWidth);
        RenderOption(config.NameplateHeightScaleCoeff, "Nameplate Scale Coefficient", sliderLabelWidth);

        ImGui::NewLine();
        RenderOption(config.ScaleFactor, "Overall Scale Factor", sliderLabelWidth, "%.2f");
        RenderOption(config.MaxCalculatedScaleFactor, "Max Scale Factor", sliderLabelWidth, "%.2f");
    }

    std::vector<Ui::AnimatedTabState> tabs;
};

void Ui::RenderSettingsPanel()
{
    ImGui::BeginChild(
        "##AnimatedNameplatesSettings",
        ImVec2(std::max(ImGui::GetContentRegionAvail().x, 400.0f), std::max(ImGui::GetContentRegionAvail().y, 250.0f)),
        ImGuiChildFlags_None, ImGuiWindowFlags_None);

    ImGui::PushFont(mq::imgui::LargeTextFont);
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "Nameplate Settings");
    ImGui::Separator();
    ImGui::PopFont();

    float dt = ImGui::GetIO().DeltaTime;
    ImDrawList* dl = ImGui::GetWindowDrawList();

    static int active_tab = 0;
    static SettingsPanel panel;

    ImVec2 tabs_pos = ImGui::GetCursorScreenPos();
    float tab_height = ImGui::GetTextLineHeightWithSpacing() * 1.5f;
    float total_width = ImGui::GetContentRegionAvail().x;
    float tab_width = floor(total_width / panel.tabs.size());

    // Draw tab background
    dl->AddRectFilled(tabs_pos, ImVec2(tabs_pos.x + total_width, tabs_pos.y + tab_height),
        ImGui::GetColorU32(ImGuiCol_TableHeaderBg), //(35, 38, 48, 255),
        2.0f, ImDrawFlags_RoundCornersTop);

    // Calculate target indicator position
    float target_x = tabs_pos.x;
    for (int i = 0; i < active_tab; i++)
        target_x += tab_width;
    float target_width = tab_width;

    // Animate indicator with spring
    ImGuiID id = ImGui::GetID("animnp_tab_indicator");
    float indicator_x = iam_tween_float(id, x_id, target_x, 0.3f,
        iam_ease_spring_desc(1.0f, 180.0f, 18.0f, 0.0f), iam_policy_crossfade, dt);
    float indicator_width = iam_tween_float(id, w_id, target_width, 0.25f,
        iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

    // Draw tabs
    float x = tabs_pos.x;
    for (const auto& tab : panel.tabs)
    {
        ImVec2 tab_min(x, tabs_pos.y);
        ImVec2 tab_max(x + tab_width, tabs_pos.y + tab_height);

        // Invisible button
        ImGui::SetCursorScreenPos(tab_min);
        ImGui::PushID(tab.idx);

        if (ImGui::InvisibleButton("##tab_btn", ImVec2(tab_width, tab_height)))
            active_tab = tab.idx;

        bool hovered = ImGui::IsItemHovered();

        // Text color animation
        ImGuiID tab_id = ImGui::GetID(tab.name.c_str());
        float target_alpha = (tab.idx == active_tab) ? 1.0f : (hovered ? 0.8f : 0.5f);

        // Draw text
        ImVec2 text_size = ImGui::CalcTextSize(tab.name.c_str());
        ImVec2 text_pos(x + (tab_width - text_size.x) * 0.5f, tabs_pos.y + (tab_height - text_size.y) * 0.5f);
        dl->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), tab.name.c_str());

        x += tab_width;

        ImGui::PopID();
    }

    // Draw animated indicator
    float indicator_y = tabs_pos.y + tab_height - 3.0f;
    dl->AddRectFilled(ImVec2(indicator_x + 8.0f, indicator_y),
        ImVec2(indicator_x + indicator_width - 8.0f, indicator_y + 3.0f),
        ImGui::GetColorU32(ImGuiCol_TabSelected), // IM_COL32(91, 194, 231, 255),
        4.0f);

    ImVec2 padding = ImGui::GetStyle().FramePadding;

    ImVec2 content_pos(tabs_pos.x, tabs_pos.y + tab_height + padding.y);
    ImVec2 content_size(total_width, ImGui::GetContentRegionAvail().y);

    ImGui::SetCursorScreenPos(ImVec2(tabs_pos.x, content_pos.y + padding.y * 2.0f));
    ImGui::Indent(padding.x);
    panel.tabs[active_tab].content();
    ImGui::Unindent(padding.x);

    ImGui::SetCursorScreenPos(ImVec2(tabs_pos.x, content_pos.y + content_size.y));
    ImGui::Dummy(ImVec2(0.0f, 0.0f));
    ImGui::EndChild();
}

