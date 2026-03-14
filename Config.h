#pragma once

#include "ConfigVariable.h"
#include "Widgets.h"

#include <string>

namespace Ui {

enum HPBarStyle
{
    HPBarStyle_SolidRed,
    HPBarStyle_ConColor,
    HPBarStyle_ColorRange
};

template <>
struct config_enum_traits<HPBarStyle>
{
    static const std::vector<std::pair<HPBarStyle, std::string>>& values()
    {
        static std::vector<std::pair<HPBarStyle, std::string>> value_map = {
            { HPBarStyle_SolidRed, "Solid Red" },
            { HPBarStyle_ConColor, "Con Color" },
            { HPBarStyle_ColorRange, "Color Range" }
        };

        return value_map;
    }
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

private:
    std::string m_configFile;

    // must come before the variables so that it's initialized when they register themselves in their constructors
    ConfigContainer m_container;

public:
    // Rendering toggles
    ConfigVariable<bool> RenderForSelf{ m_container, "RenderForSelf", true };
    ConfigVariable<bool> RenderForGroup{ m_container, "RenderForGroup", true };
    ConfigVariable<bool> RenderForTarget{ m_container, "RenderForTarget", true };
    ConfigVariable<bool> RenderForAllHaters{ m_container, "RenderForAllHaters", true };
    ConfigVariable<bool> RenderForNPCs{ m_container, "RenderForNPCs", false };

    // Optional display
    ConfigVariable<bool> ShowGuild{ m_container, "ShowGuild", false };
    ConfigVariable<bool> ShowPurpose{ m_container, "ShowPurpose", false };
    ConfigVariable<bool> ShowLevel{ m_container, "ShowLevel", true };
    ConfigVariable<bool> ShowClass{ m_container, "ShowClass", true };
    ConfigVariable<bool> ShortClassName{ m_container, "ShortClassName", true };
    ConfigVariable<bool> ShowTargetIndicatorWings{ m_container, "ShowTargetIndicatorWings", true };
    ConfigVariable<float> TargetIndicatorWingLength{ m_container, "TargetIndicatorWingLength", 15.0f, 5.0f, 75.0f };
    ConfigVariable<bool> DrawBarBorders{ m_container, "DrawBarBorders", true };

    // Rendering behavior
    ConfigVariable<bool> RenderToForeground{ m_container, "RenderToForeground", false };
    ConfigVariable<bool> RenderNoLOS{ m_container, "RenderNoLOS", false };

    // Basic flags
    ConfigVariable<bool> ShowBuffIcons{ m_container, "ShowBuffIcons", true };
    ConfigVariable<bool> ShowDebugPanel{ m_container, "ShowDebugPanel", false };

    ConfigVariable<bool> DrawTestBar{ m_container, "DrawTestBar", false };
    ConfigVariable<float> BarPercent{ m_container, "BarPercent", 100.0f, 0.0f, 100.0f };

    // Layout / sizes
    ConfigVariable<float> FontSize{ m_container, "FontSize", 20.0f, 1.0f, 40.0f };
    ConfigVariable<float> IconSize{ m_container, "IconSize", 20.0f, 10.0f, 40.0f };
    ConfigVariable<float> NameplateWidth{ m_container, "NameplateWidth", 500.0f, 25.0f, 1500.0f };
    ConfigVariable<int> HPTicks{ m_container, "HPTicks", 10, 1, 25 };
    ConfigVariable<float> NameplateHeightOffset{ m_container, "NameplateHeightOffset", 35.0f, -100.0f, 400.0f };

    // Bar appearance
    ConfigVariable<float> BarRounding{ m_container, "BarRounding", 6.0f, 0.0f, 10.0f };
    ConfigVariable<float> BarBorderThickness{ m_container, "BarBorderThickness", 2.5f, 0.0f, 5.0f };

    // HP bar style
    ConfigVariable<HPBarStyle> HPBarStyleSelf{ m_container, "HPBarStyleSelf", HPBarStyle_ColorRange };
    ConfigVariable<HPBarStyle> HPBarStyleGroup{ m_container, "HPBarStyleGroup", HPBarStyle_ColorRange };
    ConfigVariable<HPBarStyle> HPBarStyleTarget{ m_container, "HPBarStyleTarget", HPBarStyle_ColorRange };
    ConfigVariable<HPBarStyle> HPBarStyleHaters{ m_container, "HPBarStyleHaters", HPBarStyle_ColorRange };
    ConfigVariable<HPBarStyle> HPBarStyleNPCs{ m_container, "HPBarStyleNPCs", HPBarStyle_ColorRange };

};

} // namespace Ui
