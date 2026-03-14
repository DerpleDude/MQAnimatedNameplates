#pragma once
#include "ConfigVariable.h"

#include "imgui.h"
#include "imgui/imanim/im_anim.h"
#include "imgui_internal.h"
#include <unordered_map>
#include <yaml-cpp/yaml.h>

namespace Ui
{

enum HPBarStyle
{
    HPBarStyle_SolidRed,
    HPBarStyle_ConColor,
    HPBarStyle_ColorRange
};

class Config
{
  public:
    void RegisterVariable(ConfigVariableBase* var) { m_registry.push_back(var); };

    void SaveSettings();
    void LoadSettings();

  private:
    // must come before the variables so that it's initialized when they register themselves in their constructors
    std::vector<ConfigVariableBase*> m_registry;

  public:
    // Rendering toggles
    ConfigVariable<bool> RenderForSelf;
    ConfigVariable<bool> RenderForGroup;
    ConfigVariable<bool> RenderForTarget;
    ConfigVariable<bool> RenderForAllHaters;

    // Optional display
    ConfigVariable<bool>  ShowGuild;
    ConfigVariable<bool>  ShowPurpose;
    ConfigVariable<bool>  ShowLevel;
    ConfigVariable<bool>  ShowClass;
    ConfigVariable<bool>  ShortClassName;
    ConfigVariable<bool>  ShowTargetIndicatorWings;
    ConfigVariable<float> TargetIndicatorWingLength;
    ConfigVariable<bool>  DrawBarBorders;

    // Rendering behavior
    ConfigVariable<bool> RenderToForeground;
    ConfigVariable<bool> RenderNoLOS;

    // Basic flags
    ConfigVariable<bool> ShowBuffIcons;
    ConfigVariable<bool> ShowDebugPanel;

    ConfigVariable<bool>  DrawTestBar;
    ConfigVariable<float> BarPercent;

    // Layout / sizes
    ConfigVariable<float> PaddingX;
    ConfigVariable<float> PaddingY;

    ConfigVariable<float> FontSize;
    ConfigVariable<float> IconSize;
    ConfigVariable<float> NameplateWidth;
    ConfigVariable<int>   HPTicks;
    ConfigVariable<float> NameplateHeightOffset;

    // Bar appearance
    ConfigVariable<float> BarRounding;
    ConfigVariable<float> BarBorderThickness;

    // HP bar style
    ConfigVariable<int> HPBarStyleSelf;
    ConfigVariable<int> HPBarStyleGroup;
    ConfigVariable<int> HPBarStyleTarget;
    ConfigVariable<int> HPBarStyleHaters;

    YAML::Node& GetConfigNode() { return m_configNode; }

    static Config& Get()
    {
        static Config instance;
        return instance;
    }

    Config(const Config&)            = delete;
    Config(Config&&)                 = delete;
    Config& operator=(const Config&) = delete;
    Config& operator=(Config&&)      = delete;

  private:
    std::string m_configFile;
    YAML::Node  m_configNode;

    Config()
        // clang-format off
        : RenderForSelf(this, "RenderForSelf", true),
		  RenderForGroup(this, "RenderForGroup", true),
          RenderForTarget(this, "RenderForTarget", true),
          RenderForAllHaters(this, "RenderForAllHaters", true),

          ShowGuild(this, "ShowGuild", false),
          ShowPurpose(this, "ShowPurpose", false),
          ShowLevel(this, "ShowLevel", true),
          ShowClass(this, "ShowClass", true),
          ShortClassName(this, "ShortClassName", true),
          ShowTargetIndicatorWings(this, "ShowTargetIndicatorWings", true),
          TargetIndicatorWingLength(this, "TargetIndicatorWingLength", 15.0f),
	      DrawBarBorders(this, "DrawBarImages", true),
        
		  RenderToForeground(this, "RenderToForeground", false),
          RenderNoLOS(this, "RenderNoLOS", false),
        
		  ShowBuffIcons(this, "ShowBuffIcons", true),
          ShowDebugPanel(this, "ShowDebugPanel", false),
          
	      PaddingX(this, "PaddingX", 8.0f),
          PaddingY(this, "PaddingY", 4.0f),

		  FontSize(this, "FontSize", 20.0f),
          IconSize(this, "IconSize", 20.0f),
          NameplateWidth(this, "NameplateWidth", 500.0f),
          HPTicks(this, "HPTicks", 10),
          NameplateHeightOffset(this, "NameplateHeightOffset", 35.0f),

		  BarRounding(this, "BarRounding", 6.0f),
          BarBorderThickness(this, "BarBorderThickness", 2.5f),

		  DrawTestBar(this, "DrawTestBar", false),
		  BarPercent(this, "BarPercent", 100.0f),

          HPBarStyleSelf(this, "HPBarStyleSelf", static_cast<int>(HPBarStyle_ColorRange)),
          HPBarStyleGroup(this, "HPBarStyleGroup", static_cast<int>(HPBarStyle_ColorRange)),
          HPBarStyleTarget(this, "HPBarStyleTarget", static_cast<int>(HPBarStyle_ColorRange)),
          HPBarStyleHaters(this, "HPBarStyleHaters", static_cast<int>(HPBarStyle_ColorRange))
    // clang-format on
    {
        LoadSettings();
    };
};
} // namespace Ui