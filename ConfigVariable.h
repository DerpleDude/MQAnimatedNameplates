#pragma once

#include <yaml-cpp/yaml.h>

namespace Ui
{
class Config;

class ConfigVariableBase
{
  public:
    virtual void Deserialize(const YAML::Node& configFile) = 0;
};

template <typename T> class ConfigVariable : public ConfigVariableBase
{
  public:
    ConfigVariable(Config* config, const char* key, const T& v);

    const T&           get() const noexcept { return m_value; }
    const std::string& getKey() const noexcept { return m_configKey; }

    void set(const T& v)
    {
        m_value                                         = v;
        m_pConfig->GetConfigNode()[m_configKey.c_str()] = v;
        m_pConfig->SaveSettings();
    }

    operator const T&() const noexcept { return m_value; }

    ConfigVariable& operator=(const T& v)
    {
        set(v);
        return *this;
    }

    void Deserialize(const YAML::Node& configFile) override
    {
        if (configFile[m_configKey])
        {
            m_value = configFile[m_configKey.c_str()].as<T>(m_value);
        }
    }

  private:
    T           m_value;
    std::string m_configKey;
    Config*     m_pConfig;
};
} // namespace Ui
