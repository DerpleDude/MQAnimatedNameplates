
#include "ConfigVariable.h"
#include "Widgets.h"

#include "spdlog/spdlog.h"
#include <fstream>

namespace Ui {

///////////////////////////////////////////////////////////////////////////////

ConfigContainerBase::ConfigContainerBase()
{
}

ConfigContainerBase::ConfigContainerBase(ConfigContainerBase&& other) noexcept
    : m_variables(std::move(other.m_variables))
{
    // update container pointer for moved variables
    for (ConfigVariableBase* var : m_variables)
    {
        var->m_container = this;
    }
}

ConfigContainerBase::~ConfigContainerBase()
{
    for (ConfigVariableBase* var : m_variables)
    {
        var->m_container = nullptr;
    }
}

ConfigContainerBase& ConfigContainerBase::operator=(ConfigContainerBase&& other) noexcept
{
    if (this != &other)
    {
        m_variables = std::move(other.m_variables);

        // update container pointer for moved variables
        for (ConfigVariableBase* var : m_variables)
        {
            var->m_container = this;
        }
    }

    return *this;
}

void ConfigContainerBase::RegisterVariable(ConfigVariableBase* variable)
{
    m_variables.push_back(variable);
}

void ConfigContainerBase::UnregisterVariable(ConfigVariableBase* variable)
{
    std::erase(m_variables, variable);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ConfigContainer::ConfigContainer()
{
}

ConfigContainer::ConfigContainer(ConfigContainer&& other) noexcept
    : ConfigContainerBase(std::move(other))
    , m_configNode(std::move(other.m_configNode))
    , m_dirty(other.m_dirty)
{
}

ConfigContainer& ConfigContainer::operator=(ConfigContainer&& other) noexcept
{
    if (this != &other)
    {
        ConfigContainerBase::operator=(std::move(other));
        m_configNode = std::move(other.m_configNode);
        m_dirty = other.m_dirty;
    }
    return *this;
}

void ConfigContainer::UpdateVariable(const ConfigVariableBase& variable)
{
    if (variable.Store(m_configNode))
        SetDirty(true);
}

YAML::Node ConfigContainer::GetNode()
{
    return m_configNode;
}

void ConfigContainer::SetDirty(bool dirty)
{
    m_dirty = dirty;
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

    m_dirty = false;
}

void ConfigContainer::LoadConfig(const std::string& fileName)
{
    try
    {
        m_configNode = YAML::LoadFile(fileName);

        for (ConfigVariableBase* var : m_variables)
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ConfigVariableBase::ConfigVariableBase(ConfigContainerBase* container, std::string keyName)
    : m_container(container)
    , m_keyName(std::move(keyName))
{
    m_container->RegisterVariable(this);
}

ConfigVariableBase::~ConfigVariableBase()
{
    if (m_container)
    {
        m_container->UnregisterVariable(this);
    }
}

ConfigVariableBase::ConfigVariableBase(ConfigVariableBase&& other) noexcept
    : m_container(std::exchange(other.m_container, nullptr))
    , m_keyName(std::move(other.m_keyName))
{
    // Move the registration from the other variable to this one.
    if (m_container)
    {
        m_container->UnregisterVariable(&other);
        m_container->RegisterVariable(this);
    }
}

ConfigVariableBase& ConfigVariableBase::operator=(ConfigVariableBase&& other) noexcept
{
    if (this != &other)
    {
        if (m_container)
            m_container->UnregisterVariable(this);

        m_container = other.m_container;
        m_keyName = std::move(other.m_keyName);

        if (m_container)
            m_container->RegisterVariable(this);
    }

    return *this;
}

///////////////////////////////////////////////////////////////////////////////

ConfigGroup::ConfigGroup(ConfigContainerBase& container, std::string keyName)
    : ConfigVariableBase(&container, std::move(keyName))
{
}

ConfigGroup::ConfigGroup(ConfigGroup&& other) noexcept
    : ConfigContainerBase(std::move(other))
    , ConfigVariableBase(std::move(other))
{
}

ConfigGroup::~ConfigGroup()
{
}

ConfigGroup& ConfigGroup::operator=(ConfigGroup&& other) noexcept
{
    if (this != &other)
    {
        ConfigContainerBase::operator=(std::move(other));
        ConfigVariableBase::operator=(std::move(other));
    }

    return *this;
}

void ConfigGroup::UpdateVariable(const ConfigVariableBase& variable)
{
    if (m_container)
    {
        YAML::Node node = GetNode();
        if (variable.Store(node))
            m_container->SetDirty(true);
    }
}

void ConfigGroup::SetDirty(bool dirty)
{
    if (m_container)
    {
        m_container->SetDirty(dirty);
    }
}

YAML::Node ConfigGroup::GetNode()
{
    if (m_container)
    {
        try
        {
            return m_container->GetNode()[m_keyName];
        }
        catch (const YAML::InvalidNode&)
        {
            return YAML::Node();
        }
    }

    return YAML::Node();
}

void ConfigGroup::Load(const YAML::Node& source)
{
    YAML::Node configNode = source[m_keyName];

    if (!configNode || !configNode.IsMap())
    {
        return;
    }

    for (ConfigVariableBase* var : m_variables)
    {
        var->Load(configNode);
    }
}

bool ConfigGroup::Store(YAML::Node& target) const
{
    return true;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace Ui
