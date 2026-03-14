#include "Config.h"

#include "mq/Plugin.h" // for gConfigPath

namespace Ui {

Config::Config()
{
    LoadSettings();
}

Config& Config::Get()
{
    static Config instance;
    return instance;
}

void Config::SaveSettings()
{
    if (m_container.IsDirty())
    {
        m_container.SaveConfig(m_configFile);
    }
}

void Config::LoadSettings()
{
    m_configFile = (std::filesystem::path(mq::gPathConfig) / "MQAnimatedNameplates.yaml").string();

    m_container.LoadConfig(m_configFile);
}

} // namespace Ui
