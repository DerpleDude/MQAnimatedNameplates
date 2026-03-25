#include "SettingsPanel.h"
#include "Config.h"
#include "Widgets.h"

#include "imgui/ImGuiUtils.h"
#include "imgui/imgui_internal.h"
#include "imgui/imanim/im_anim.h"
#include "mq/Plugin.h"

#include <algorithm>
#include <ranges>
#include <cmath>

using namespace eqlib;

static const ImGuiID x_id = ImHashStr("x");
static const ImGuiID w_id = ImHashStr("w");
static std::vector<std::string> s_NameplateStyleLabels;

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

bool RenderVectorDropdown(Ui::ConfigVariable<uint32_t>& variable, const char* label, const std::vector<std::string>& options)
{
    int currentIndex = variable.get();
    if (Ui::AnimatedCombo(label, &currentIndex, options))
    {
        variable.set(currentIndex);
        return true;
    }
    
    return false;
}

void ResolveNameplateLabels()
{
    // setup the new vector of labels
    s_NameplateStyleLabels.clear();
    for (auto& style : Ui::Config::Get().NameplateStyles.StyleDefinitions)
    {
        s_NameplateStyleLabels.push_back(style.getKey());
    }

    // validate all nameplates for valid indexes and reset to 0 if invalid
    Ui::Config& config = Ui::Config::Get();
    for (auto* nameplateOptions : { &config.TargetNameplateOptions, &config.SelfNameplateOptions, &config.GroupNameplateOptions, &config.HatersNameplateOptions, &config.NPCNameplateOptions, &config.PCNameplateOptions })
    {
        if (nameplateOptions->NameplateConfigStyle.get() >= s_NameplateStyleLabels.size())
            nameplateOptions->NameplateConfigStyle.set(0);
    }
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
    if (s_NameplateStyleLabels.empty())
        ResolveNameplateLabels();

    ImGui::PushID(&group);
    if (ImGui::CollapsingHeader(label))
    {
        ImGui::Indent();
        RenderOption(group.Render, "Render Nameplate");
        RenderVectorDropdown(group.NameplateConfigStyle, "Nameplate Config Style", s_NameplateStyleLabels);
        ImGui::Unindent();
    }
    ImGui::PopID();
}

void RenderNameplateStyleConfigGroup(Ui::NameplateStyleDefinition& group, const char* label)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(52,52,52,52));
    ImGui::PushID(&group);
    if (ImGui::CollapsingHeader(label))
    {
        ImGui::BeginChild("StyleChild", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiWindowFlags_AlwaysAutoResize);
        float sliderLabelWidth = 150;
        RenderOption(group.HPBarStyle, "HP Bar Style");
        if (group.HPBarStyle.get() == Ui::HPBarStyle_Custom)
            RenderOption(group.CustomColor, "Custom Color");
        
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
        RenderOption(group.ColorAlphaModifier, "Bar Alpha Modifier", sliderLabelWidth, "%.2f");
        RenderOption(group.DrawBarBorders, "Draw Bar Borders");
        ImGui::BeginDisabled(!group.DrawBarBorders);
        ImGui::Indent();
        RenderOption(group.BarBordersColor, "Bar Border Color");
        RenderOption(group.BarBorderThickness, "Bar Border Thickness", sliderLabelWidth, "%.1f");
        ImGui::Unindent();
        ImGui::EndDisabled();
        
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
        ImGui::EndChild();

        if (Ui::InlineConfirmButton("Delete Style", "Confirm", "Cancel", ImGui::GetTextLineHeightWithSpacing(), nullptr, 0, IM_COL32(220, 52, 52, 100), IM_COL32(220, 52, 52, 220)))
        {
            Ui::Config::Get().NameplateStyles.DeleteStyle(label);
        }
    }
    ImGui::PopID();
    ImGui::PopStyleColor();
}

Ui::NameplateStyleDefinition& Ui::NameplateConfigGroup::GetStyle()
{
    auto& config = Config::Get();

    if (NameplateConfigStyle >= config.NameplateStyles.StyleDefinitions.size())
    {
        NameplateConfigStyle.set(0);
        return config.NameplateStyles.StyleDefinitions[0];
    }

    return config.NameplateStyles.StyleDefinitions[NameplateConfigStyle];
}

void Ui::NameplateStylesContainer::Load(const YAML::Node& source)
{
    ConfigGroup::Load(source);

    StyleDefinitions.emplace_back(*this, "Default");
    StyleDefinitions.back().Load(GetNode());
    // now load in all the custom configs.
    if (GetNode().IsMap())
    {
        for (auto pair : GetNode())
        {
            std::string key = pair.first.as<std::string>();
            if (key.compare("Default") != 0)
            {
                StyleDefinitions.emplace_back(*this, key);
                StyleDefinitions.back().Load(GetNode());
            }

        }
    }
}

void Ui::NameplateStylesContainer::AddNewStyle(const char* name)
{
    // put this in first becuase 0 == Default when size == 1
    StyleDefinitions.emplace_back(*this, name);
    StyleDefinitions.back().GetNode() = YAML::Node(YAML::NodeType::Map);
    SetDirty(true);

    Config::Get().SaveSettings();

    ResolveNameplateLabels();
}

void Ui::NameplateStylesContainer::DeleteStyle(const char* name)
{
    // make a local copy because we will delete the thing that owns name.
    std::string nameString{ name };
    // put this in first becuase 0 == Default when size == 1
    std::erase_if(StyleDefinitions, [nameString](const auto& item) {
        return item.getKey().compare(nameString) == 0; // Replace with your condition
        });    
    
    GetNode().remove(nameString.c_str());
    SetDirty(true);
    Config::Get().SaveSettings();

    ResolveNameplateLabels();
}

class SettingsPanel
{
public:
    SettingsPanel()
    {
        tabs.emplace_back(0, "Look and Feel", [this]() { DrawLookAndFeelTab(); });
        tabs.emplace_back(1, "Sytle Editor", [this]() { DrawStyleEditorTab(); });
        tabs.emplace_back(2, "Rendering Options", [this]() { DrawRenderingOptionsTab(); });
        tabs.emplace_back(3, "Dev and Debug", [this]() { DrawDevAndDebugTab(); });
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

    void DrawStyleEditorTab()
    {
        Ui::Config& config = Ui::Config::Get();

        for (auto& style : config.NameplateStyles.StyleDefinitions)
        {
            RenderNameplateStyleConfigGroup(style, style.getKey().c_str());
        }

        char inputText[512] = {0};

        if (Ui::InlineConfirmButton("New Style", "Accept", "Cancel", ImGui::GetTextLineHeightWithSpacing(), inputText, sizeof(inputText)))
        {
            config.NameplateStyles.AddNewStyle(inputText);
        }
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
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
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
    ImGui::PopStyleVar(2);
}

