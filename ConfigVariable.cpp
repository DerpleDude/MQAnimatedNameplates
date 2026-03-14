
#include "ConfigVariable.h"

#include "spdlog/spdlog.h"
#include <fstream>

namespace Ui {

ConfigContainer::ConfigContainer()
{
}

ConfigContainer::ConfigContainer(YAML::Node configNode)
    : m_configNode(configNode)
{
}

void ConfigContainer::Register(ConfigVariableBase* variable)
{
    m_registry.push_back(variable);
}

void ConfigContainer::Update(const ConfigVariableBase& variable)
{
    variable.Store(m_configNode);
    m_dirty = true;
}

void ConfigContainer::SaveConfig(const std::string& fileName)
{
    try
    {
        std::fstream file(fileName, std::ios::out);

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
        SPDLOG_ERROR("Failed to write settings file: {}", fileName);
    }
}

void ConfigContainer::LoadConfig(const std::string& fileName)
{
    try
    {
        m_configNode = YAML::LoadFile(fileName);

        for (ConfigVariableBase* var : m_registry)
        {
            var->Load(m_configNode);
        }
    }
    catch (const YAML::ParserException& ex)
    {
        // failed to parse, notify and return
        SPDLOG_ERROR("Failed to parse YAML in {}: {}", fileName, ex.what());
    }
    catch (const YAML::BadFile&)
    {
        // if we can't read the file, then try to write it with an empty config
        SaveConfig(fileName);
    }
}

//=============================================================================

ConfigVariableBase::ConfigVariableBase(ConfigContainer& container)
    : m_container(container)
{
    m_container.Register(this);
}

} // namespace Ui
