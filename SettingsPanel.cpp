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

void RenderTestGroup(Ui::TestConfigGroup& group)
{
    ImGui::PushID(&group);
    ImGui::Separator();
    ImGui::Text("Group Name: %s", group.getKey().c_str());
    RenderOption(group.TestInt, "Test int");
    RenderOption(group.TestBool, "Test bool");
    RenderOption(group.TestFloat, "Test float");
    RenderOption(group.TestColor, "Test color");
    ImGui::PopID();
}

void RenderNameplateConfigGroup(Ui::NameplateConfigGroup& group, const char* label)
{
    ImGui::PushID(&group);
    if (ImGui::CollapsingHeader(label))
    {
        ImGui::Indent();
        float sliderLabelWidth = 150;
        RenderOption(group.HPBarStyle, "HP Bar Style");
        
        ImGui::NewLine();
        ImGui::SeparatorText("Additional Info");
        ImGui::NewLine();

        RenderOption(group.ShowLevel, "Show Level");
        RenderOption(group.ShowClass, "Show Class");
        ImGui::Indent();
        ImGui::BeginDisabled(!group.ShowClass);
        RenderOption(group.ShortClassName, "Short Class Name");
        ImGui::EndDisabled();
        ImGui::Unindent();

        ImGui::NewLine();
        ImGui::SeparatorText("Target Indicator");
        ImGui::NewLine();

        RenderOption(group.ShowTargetIndicator, "Show Target Indicator");
        RenderOption(group.TargetIndicatorPadding, "Indicator Padding", sliderLabelWidth, "%.0f");
        RenderOption(group.TargetIndicatorBlinkSpeed, "Indicator Blink Speed", sliderLabelWidth, "%.2f");
        
        ImGui::NewLine();
        ImGui::SeparatorText("Style & Borders");
        ImGui::NewLine();

        RenderOption(group.BarRounding, "Bar Rounding", sliderLabelWidth, "%.1f");
        RenderOption(group.DrawBarBorders, "Draw Bar Borders");
        ImGui::SameLine();
        ImGui::BeginDisabled(!group.DrawBarBorders);
        RenderOption(group.BarBordersColor, "Bar Border Color");
        RenderOption(group.BarBorderThickness, "Bar Border Thickness", sliderLabelWidth, "%.1f");
        ImGui::EndDisabled();
        RenderOption(group.ColorAlphaModifier, "Bar Alpha Modifier", sliderLabelWidth, "%.2f");
        
        ImGui::NewLine();

        RenderOption(group.NameplateWidth, "Nameplate Width", sliderLabelWidth, "%0.f");
        RenderOption(group.NameplateHeight, "Nameplate Height", sliderLabelWidth, "%0.f");
        RenderOption(group.NameplateHeightOffset, "Nameplate Height Offset", sliderLabelWidth, "%.0f");

        ImGui::NewLine();

        RenderOption(group.HPTicks, "HP Ticks Every [x]%", sliderLabelWidth);
        RenderOption(group.FontSize, "Font Size", sliderLabelWidth, "%.1f");
        RenderOption(group.IconSize, "Icon Size", sliderLabelWidth, "%.1f");
        ImGui::NewLine();
        RenderOption(group.ScaleFactor, "Overall Scale Factor", sliderLabelWidth, "%.2f");
        RenderOption(group.MaxCalculatedScaleFactor, "Max Scale Factor", sliderLabelWidth, "%.2f");
        ImGui::Unindent();
    }
    ImGui::PopID();
}

class SettingsPanel
{
public:
    SettingsPanel()
    {
        tabs.emplace_back(0, "Targeting", [this]() { DrawTargetingTab(); });
        tabs.emplace_back(1, "Look and Feel", [this]() { DrawLookAndFeelTab(); });
        tabs.emplace_back(2, "Rendering Options", [this]() { DrawRenderingOptionsTab(); });
        tabs.emplace_back(3, "Dev and Debug", [this]() { DrawDevAndDebugTab(); });

        Ui::Config& config = Ui::Config::Get();

        config.TestGroups.emplace_back(config.GetContainer(), "TestGroup1");
        config.TestGroups.emplace_back(config.GetContainer(), "TestGroup1");
    }

    void DrawTargetingTab()
    {
        Ui::Config& config = Ui::Config::Get();

        RenderOption(config.RenderForSelf, "Show For Self");
        RenderOption(config.RenderForGroup, "Show For Group");
        RenderOption(config.RenderForPCs, "Show For All PCs");
        RenderOption(config.RenderForTarget, "Show For Target");
        RenderOption(config.RenderForAllHaters, "Show For All Haters");
        RenderOption(config.RenderForNPCs, "Show For All NPCs");
    }

    void DrawLookAndFeelTab()
    {
        Ui::Config& config = Ui::Config::Get();

        RenderNameplateConfigGroup(config.SelfNameplateOptions, "Nameplate Options for Self");
        RenderNameplateConfigGroup(config.TargetNameplateOptions, "Nameplate Options for Target");
        RenderNameplateConfigGroup(config.GroupNameplateOptions, "Nameplate Options for Group");
        RenderNameplateConfigGroup(config.HatersNameplateOptions, "Nameplate Options for Auto Haters");
        RenderNameplateConfigGroup(config.PCNameplateOptions, "Nameplate Options for PCs");
        RenderNameplateConfigGroup(config.NPCNameplateOptions, "Nameplate Options for NPCs");

        ImGui::NewLine();
        ImGui::SeparatorText("Color Range Colors");
        ImGui::NewLine();
        RenderOption(config.ColorRangeLow, "Color Low");
        RenderOption(config.ColorRangeMid, "Color Mid");
        RenderOption(config.ColorRangeHigh, "Color High");

        ImGui::NewLine();
        ImGui::SeparatorText("Custom Colors");
        ImGui::NewLine();

        RenderOption(config.CustomColor1, "Custom Color 1");
        RenderOption(config.CustomColor2, "Custom Color 2");
        RenderOption(config.CustomColor3, "Custom Color 3");
        RenderOption(config.CustomColor4, "Custom Color 4");
        RenderOption(config.CustomColor5, "Custom Color 5");
        RenderOption(config.CustomColor6, "Custom Color 6");
        
        ImGui::NewLine();


        ImGui::NewLine();
        ImGui::SeparatorText("Global Options");
        ImGui::NewLine();

        RenderOption(config.ShowGuild, "Show Guild Names");
        RenderOption(config.ShowPurpose, "Show Purpose");
        RenderOption(config.ShowBuffIcons, "Show Target Buff Icons");
    }

    void DrawRenderingOptionsTab()
    {
        Ui::Config& config = Ui::Config::Get();

        float sliderLabelWidth = 250;

        ImGui::NewLine();

        RenderOption(config.RenderToForeground, "Render To Foreground");
        RenderOption(config.RenderTargetNoLOS, "Render Target Even When Occluded");
        RenderOption(config.RenderNoLOS, "Render All Even When Occluded");
        RenderOption(config.MaxDrawDistance, "Maximum Draw Distance");

        ImGui::NewLine();

        RenderOption(config.ScaleWithDistance, "Scale With Distance");
    }

    void DrawDevAndDebugTab()
    {
        Ui::Config& config = Ui::Config::Get();
        float sliderLabelWidth = ImGui::CalcTextSize("Nameplate Height Adjustment").x;

        RenderOption(config.ShowDebugBounds, "Show Debug Bounding Box");
        RenderOption(config.ShowDebugText, "Show Debug Text");

        ImGui::NewLine();

        RenderOption(config.DrawTestBar, "Draw Test Bar");
        RenderOption(config.UseTestPercentages, "Use Test Percentages");
        RenderOption(config.BarPercent, "Test Bar Percent", 0, "%0.f");

        ImGui::NewLine();

        RenderOption(config.ScaleFactorAdjustment, "Scale Factor Adjustment", sliderLabelWidth, "%.05f");
        RenderOption(config.NameplateHeightAdjust, "Nameplate Height Adjustment", sliderLabelWidth);
        RenderOption(config.NameplateHeightScaleCoeff, "Nameplate Scale Coefficient", sliderLabelWidth);

        ImGui::NewLine();

        ImGui::Text("Test Settings Groups");

        for (Ui::TestConfigGroup& group : config.TestGroups)
        {
            RenderTestGroup(group);
        }

        if (ImGui::Button("Add New Group"))
        {
            config.TestGroups.emplace_back(config.GetContainer(), fmt::format("TestGroup{}", config.TestGroups.size() + 1));
        }
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

