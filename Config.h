#pragma once

#include "ConfigVariable.h"

#include <string>

namespace Ui {

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

    // Optional display
    ConfigVariable<bool> ShowGuild{ m_container, "ShowGuild", false };
    ConfigVariable<bool> ShowPurpose{ m_container, "ShowPurpose", false };
    ConfigVariable<bool> ShowLevel{ m_container, "ShowLevel", true };
    ConfigVariable<bool> ShowClass{ m_container, "ShowClass", true };
    ConfigVariable<bool> ShortClassName{ m_container, "ShortClassName", true };
    ConfigVariable<bool> ShowTargetIndicatorWings{ m_container, "ShowTargetIndicatorWings", true };
    ConfigVariable<float> TargetIndicatorWingLength{ m_container, "TargetIndicatorWingLength", 15.0f };
    ConfigVariable<bool> DrawBarBorders{ m_container, "DrawBarBorders", true };

    // Rendering behavior
    ConfigVariable<bool> RenderToForeground{ m_container, "RenderToForeground", false };
    ConfigVariable<bool> RenderNoLOS{ m_container, "RenderNoLOS", false };

    // Basic flags
    ConfigVariable<bool> ShowBuffIcons{ m_container, "ShowBuffIcons", true };
    ConfigVariable<bool> ShowDebugPanel{ m_container, "ShowDebugPanel", false };

    ConfigVariable<bool> DrawTestBar{ m_container, "DrawTestBar", false };
    ConfigVariable<float> BarPercent{ m_container, "BarPercent", 100.0f };

    // Layout / sizes
    ConfigVariable<float> PaddingX{ m_container, "PaddingX", 8.0f };
    ConfigVariable<float> PaddingY{ m_container, "PaddingY", 4.0f };

    ConfigVariable<float> FontSize{ m_container, "FontSize", 20.0f };
    ConfigVariable<float> IconSize{ m_container, "IconSize", 20.0f };
    ConfigVariable<float> NameplateWidth{ m_container, "NameplateWidth", 500.0f };
    ConfigVariable<int> HPTicks{ m_container, "HPTicks", 10 };
    ConfigVariable<float> NameplateHeightOffset{ m_container, "NameplateHeightOffset", 35.0f };

    // Bar appearance
    ConfigVariable<float> BarRounding{ m_container, "BarRounding", 6.0f };
    ConfigVariable<float> BarBorderThickness{ m_container, "BarBorderThickness", 2.5f };

    // HP bar style
    ConfigVariable<int> HPBarStyleSelf{ m_container, "HPBarStyleSelf", static_cast<int>(HPBarStyle_ColorRange) };
    ConfigVariable<int> HPBarStyleGroup{ m_container, "HPBarStyleGroup", static_cast<int>(HPBarStyle_ColorRange) };
    ConfigVariable<int> HPBarStyleTarget{ m_container, "HPBarStyleTarget", static_cast<int>(HPBarStyle_ColorRange) };
    ConfigVariable<int> HPBarStyleHaters{ m_container, "HPBarStyleHaters", static_cast<int>(HPBarStyle_ColorRange) };
};

} // namespace Ui
