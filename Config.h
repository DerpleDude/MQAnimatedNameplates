#pragma once

#include "ConfigVariable.h"

#include "mq/base/Color.h"

#include <string>

namespace Ui {

enum HPBarStyle
{
    HPBarStyle_Invalid = -1,
    HPBarStyle_ConColor,
    HPBarStyle_ColorRange,
    HPBarStyle_Custom,
};

enum NameplateType
{
    NameplateType_Invalid = -1,
    NameplateType_Self,
    NameplateType_Target,
    NameplateType_Group,
    NameplateType_AutoHater,
    NameplateType_NPC,
    NameplateType_PC,
    NameplateType_Count
};

template <>
struct config_enum_traits<HPBarStyle>
{
    static const std::vector<std::pair<HPBarStyle, std::string>>& values()
    {
        static std::vector<std::pair<HPBarStyle, std::string>> value_map = {
            { HPBarStyle_ConColor, "Con Color" },
            { HPBarStyle_ColorRange, "Color Range" },
            { HPBarStyle_Custom, "Custom Color" },
        };

        return value_map;
    }
};

class TestConfigGroup : public ConfigGroup
{
public:
    TestConfigGroup(ConfigContainer& container, std::string name)
        : ConfigGroup(container, std::move(name))
    {
    }

    ConfigVariable<int> TestInt{ *this, "TestInt", 123, 0, 100 };
    ConfigVariable<bool> TestBool{ *this, "TestBool", false };
    ConfigVariable<float> TestFloat{ *this, "TestFloat", 1.0, 0.0, 10.0f };
    ConfigVariable<mq::MQColor> TestColor{ *this, "TestColor", mq::MQColor(255,255,255) };
};

class NameplateStyleDefinition : public ConfigGroup
{
public:
    NameplateStyleDefinition(ConfigContainerBase& container, std::string name)
        : ConfigGroup(container, std::move(name))
    {
    }

    ConfigVariable<HPBarStyle> HPBarStyle{ *this, "HPBarStyle", HPBarStyle_ColorRange };
    ConfigVariable<mq::MQColor> CustomColor{ *this, "Custom Color", mq::MQColor{ 255,255,255,255} };

    ConfigVariable<bool> ShowLevel{ *this, "ShowLevel", true };
    ConfigVariable<bool> ShowClass{ *this, "ShowClass", true };
    ConfigVariable<bool> ShortClassName{ *this, "ShortClassName", true };
    ConfigVariable<bool> ShowTargetIndicator{ *this, "ShowTargetIndicator", true };
    ConfigVariable<float> TargetIndicatorPadding{ *this, "TargetIndicatorPadding", 8.0f, 0.0f, 16.0f };
    ConfigVariable<float> TargetIndicatorBlinkSpeed{ *this, "TargetIndicatorBlinkSpeed", 0.75f, 0.0f, 4.0f };
    
    ConfigVariable<float> BarRounding{ *this, "BarRounding", 6.0f, 0.0f, 10.0f };
    ConfigVariable<bool> DrawBarBorders{ *this, "DrawBarBorders", true };
    ConfigVariable<mq::MQColor> BarBordersColor{ *this, "BarBordersColor", mq::MQColor(255,255,255) };
    ConfigVariable<float> BarBorderThickness{ *this, "BarBorderThickness", 2.5f, 0.0f, 5.0f };

    ConfigVariable<float> FontSize{ *this, "FontSize", 20.0f, 1.0f, 40.0f };
    ConfigVariable<float> IconSize{ *this, "IconSize", 20.0f, 10.0f, 40.0f };
    ConfigVariable<float> NameplateWidth{ *this, "NameplateWidth", 350.0f, 25.0f, 1500.0f };
    ConfigVariable<float> NameplateHeight{ *this, "NameplateHeight", 25.0f, 5.0f, 500.0f };
    ConfigVariable<int> HPTicks{ *this, "HPTicks", 10, 1, 25 };
    ConfigVariable<float> NameplateHeightOffset{ *this, "NameplateHeightOffset", 35.0f, -200.0f, 400.0f };
    ConfigVariable<float> ColorAlphaModifier{ *this, "ColorAlphaModifier", 1.0f, 0.1f, 1.0f };

    ConfigVariable<float> ScaleFactor{ *this, "ScaleFactor", 1.0f, 0.1f, 10.0f };
    ConfigVariable<float> MaxCalculatedScaleFactor{ *this, "MaxCalculatedScaleFactor", 1.10f, 0.1f, 10.0f };
};

class NameplateStylesContainer : public ConfigGroup
{
public:    NameplateStylesContainer(ConfigContainerBase& container, std::string name)
    : ConfigGroup(container, std::move(name))
    {
    }
    
    ConfigVariable<uint32_t> StyleCount{ *this, "StyleCount", 0 };
    std::vector<NameplateStyleDefinition> StyleDefinitions;

    void AddNewStyle();

    virtual void OnLoaded() override;
};

class NameplateConfigGroup : public ConfigGroup
{
public:
    NameplateConfigGroup(ConfigContainer& container, std::string name)
        : ConfigGroup(container, std::move(name))
    {
    }

    ConfigVariable<bool> Render{ *this, "Render", true };
    ConfigVariable<uint32_t> NameplateConfigStyle{ *this, "NameplateConfigStyle", 0 };

    NameplateStyleDefinition& GetStyle();
};


class Config
{
    Config();

public:
    static Config& Get();

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    void SaveSettings();
    void LoadSettings();

    ConfigContainer& GetContainer() { return m_container; }

private:
    std::string m_configFile;

    // must come before the variables so that it's initialized when they register themselves in their constructors
    ConfigContainer m_container;

public:
    // Per-type options
    NameplateConfigGroup TargetNameplateOptions{ m_container, "TargetNameplateOptions" };
    NameplateConfigGroup SelfNameplateOptions{ m_container, "SelfNameplateOptions" };
    NameplateConfigGroup GroupNameplateOptions{ m_container, "GroupNameplateOptions" };
    NameplateConfigGroup HatersNameplateOptions{ m_container, "HatersNameplateOptions" };
    NameplateConfigGroup NPCNameplateOptions{ m_container, "NPCNameplateOptions" };
    NameplateConfigGroup PCNameplateOptions{ m_container, "PCNameplateOptions" };

    // Optional display
    ConfigVariable<bool> ShowGuild{ m_container, "ShowGuild", false };
    ConfigVariable<bool> ShowPurpose{ m_container, "ShowPurpose", false };

    // Rendering behavior
    ConfigVariable<bool> RenderToForeground{ m_container, "RenderToForeground", false };
    ConfigVariable<bool> RenderNoLOS{ m_container, "RenderNoLOS", false };
    ConfigVariable<bool> RenderTargetNoLOS{ m_container, "RenderTargetNoLOS", false };
    ConfigVariable<float> MaxDrawDistance{ m_container, "MaxDrawDistance", 200.0f, 100.0f, 1000.0f };
    ConfigVariable<bool> ScaleWithDistance{ m_container, "ScaleWithDistance", true };

    // Basic flags
    ConfigVariable<bool> ShowBuffIcons{ m_container, "ShowBuffIcons", true };
    ConfigVariable<bool> ShowDebugBounds{ m_container, "ShowDebugBounds", false };
    ConfigVariable<bool> ShowDebugText{ m_container, "ShowDebugText", false };

    ConfigVariable<bool> DrawTestBar{ m_container, "DrawTestBar", false };
    ConfigVariable<bool> UseTestPercentages{ m_container, "UseTestPercentages", false };
    ConfigVariable<float> BarPercent{ m_container, "BarPercent", 100.0f, 0.0f, 100.0f };
    ConfigVariable<float> ScaleFactorAdjustment{ m_container, "ScaleFactorAdjustment", 1.0f, 0.001f, 10.0f };
    ConfigVariable<float> NameplateHeightAdjust{ m_container, "NameplateHeightAdjust", 5.0f, 0.0f, 25.0f };
    ConfigVariable<float> NameplateHeightScaleCoeff{ m_container, "NameplateHeightScaleCoeff", 0.3125f, 0.01f, 1.0f };

    // Layout / sizes
    ConfigVariable<mq::MQColor> ColorRangeLow{ m_container, "ColorRangeLow", mq::MQColor(204,51,51) };
    ConfigVariable<mq::MQColor> ColorRangeMid{ m_container, "ColorRangeMid", mq::MQColor(230,179,51) };
    ConfigVariable<mq::MQColor> ColorRangeHigh{ m_container, "ColorRangeHigh", mq::MQColor(51,230,51) };

    // Styles
    NameplateStylesContainer NameplateStyles{ m_container, "NameplateStyles"};

    // Testing
    std::vector<TestConfigGroup> TestGroups;
};

} // namespace Ui
