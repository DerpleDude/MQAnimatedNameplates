
#pragma once

#include "yaml-cpp/yaml.h"
#include <string>
#include <vector>

namespace Ui {

class ConfigVariableBase;

enum HPBarStyle
{
    HPBarStyle_SolidRed,
    HPBarStyle_ConColor,
    HPBarStyle_ColorRange
};

class ConfigContainer
{
public:
    ConfigContainer();
    ConfigContainer(YAML::Node configNode);

    void Register(ConfigVariableBase* variable);

    void Update(const ConfigVariableBase& variable);
    bool IsDirty() const { return m_dirty; }

    void SaveConfig(const std::string& fileName);
    void LoadConfig(const std::string& fileName);

    template <typename T>
    void Store(const std::string& key, const T& value)
    {
        m_configNode[key] = value;
    }

    template <typename T>
    bool Load(const std::string& key, T& value)
    {
        if (auto node = m_configNode[key])
        {
            try
            {
                value = node.as<T>();
                return true;
            }
            catch (const YAML::BadConversion&)
            {
                return false;
            }
        }

        return false;
    }

private:
    std::vector<ConfigVariableBase*> m_registry;
    YAML::Node m_configNode;
    bool m_dirty = false;
};

class ConfigVariableBase
{
public:
    ConfigVariableBase(ConfigContainer& container);

    virtual ~ConfigVariableBase() = default;

    virtual void Load(const YAML::Node& source) = 0;
    virtual void Store(YAML::Node& target) const = 0;

protected:
    ConfigContainer& m_container;
};

template <typename T>
class ConfigVariable : public ConfigVariableBase
{
public:
    ConfigVariable(ConfigContainer& container, const char* key, const T& v)
        : ConfigVariableBase(container)
        , m_key(key)
        , m_value(v)
    {
    }

    ConfigVariable(const ConfigVariable&) = delete;
    ConfigVariable& operator=(const ConfigVariable&) = delete;

    ConfigVariable& operator=(const T& v)
    {
        set(v);
        return *this;
    }

    operator const T& () const noexcept { return m_value; }

    const T& get() const noexcept { return m_value; }
    const std::string& getKey() const noexcept { return m_key; }

    void set(const T& v)
    {
        m_value = v;
        m_container.Update(*this);
    }

    virtual void Store(YAML::Node& yamlNode) const override
    {
        yamlNode[m_key] = m_value;
    }

    virtual void Load(const YAML::Node& yamlNode) override
    {
        if (auto node = yamlNode[m_key])
        {
            try
            {
                m_value = node.as<T>();
            }
            catch (const YAML::BadConversion&)
            {
            }
        }
    }

private:
    std::string m_key;
    T m_value;
};

} // namespace Ui
