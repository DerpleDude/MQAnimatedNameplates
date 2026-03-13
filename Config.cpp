#include "Config.h"

#include <fstream>
#include <mq/Plugin.h>
#include <yaml-cpp/yaml.h>

namespace Ui
{
void Ui::Config::LoadSettings()
{
    m_configFile = (std::filesystem::path(gPathConfig) / "MQAnimatedNameplates.yaml").string();

    try
    {
        m_configNode = YAML::LoadFile(m_configFile);

        for (Ui::ConfigVariableBase* var : m_registry)
        {
            var->Deserialize(m_configNode);
        }
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

void Ui::Config::SaveSettings()
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
} // namespace Ui