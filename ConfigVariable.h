
#pragma once

#include "yaml-cpp/yaml.h"

#include <concepts>
#include <string>
#include <vector>

namespace Ui {

class ConfigVariableBase;

template <typename T>
struct config_enum_traits;

// Requires a specialization of config_enum_traits that returns a value
// mapping representing association of values and names.
//
// Example:
//
// template <>
// struct config_enum_traits<HPBarStyle>
// {
//     static std::vector<std::pair<HPBarStyle, std::string>> values()
//     {
//         static std::vector<std::pair<HPBarStyle, std::string>> value_map = {
//             { HPBarStyle_SolidRed, "Solid Red" },
//             { HPBarStyle_ConColor, "Con Color" },
//             { HPBarStyle_ColorRange, "Color Range" }
//         };
// 
//         return value_map;
//     }
// };

template <typename T>
concept EnumTraitsConcept = requires(T v)
{
    std::is_enum_v<T>;
    { config_enum_traits<T>::values() } -> std::same_as<const std::vector<std::pair<T, std::string>>&>;
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
class BasicConfigVariable : public ConfigVariableBase
{
public:
    BasicConfigVariable(ConfigContainer& container, const char* key, const T& defaultValue)
        : ConfigVariableBase(container)
        , m_key(key)
        , m_value(defaultValue)
    {
    }

    BasicConfigVariable(const BasicConfigVariable&) = delete;
    BasicConfigVariable& operator=(const BasicConfigVariable&) = delete;

    BasicConfigVariable& operator=(const T& v)
    {
        set(v);
        return *this;
    }

    operator const T& () const noexcept { return m_value; }

    const T& get() const noexcept { return m_value; }
    const std::string& getKey() const noexcept { return m_key; }

    void set(const T& v)
    {
        if (set_internal(v))
        {
            m_container.Update(*this);
        }
    }

    virtual void Store(YAML::Node& yamlNode) const override
    {
        if constexpr (!std::is_enum_v<T>)
        {
            yamlNode[m_key] = m_value;
        }
        else
        {
            yamlNode[m_key] = static_cast<std::underlying_type_t<T>>(m_value);
        }
    }

    virtual void Load(const YAML::Node& yamlNode) override
    {
        if (auto node = yamlNode[m_key])
        {
            try
            {
                if constexpr (!std::is_enum_v<T>)
                {
                    set_internal(node.as<T>());
                }
                else
                {
                    set_internal(static_cast<T>(node.as<std::underlying_type_t<T>>()));
                }
            }
            catch (const YAML::BadConversion&)
            {
            }
        }
    }

protected:
    virtual bool set_internal(const T& v)
    {
        m_value = v;
        return true;
    }

    const std::string m_key;
    T m_value;
};

template <typename T> requires (std::is_integral_v<T> || std::is_floating_point_v<T>)
class NumericConfigVariable : public BasicConfigVariable<T>
{
public:
    NumericConfigVariable(ConfigContainer& container, const char* key, T defaultValue)
        : BasicConfigVariable<T>(container, key, defaultValue)
    {
    }

    NumericConfigVariable(ConfigContainer& container, const char* key, T defaultValue, T minValue, T maxValue)
        : BasicConfigVariable<T>(container, key, defaultValue)
        , m_minValue(minValue)
        , m_maxValue(maxValue)
    {
        if (m_minValue > m_maxValue)
        {
            std::swap(m_minValue, m_maxValue);
        }
    }

    T getMinValue() const { return m_minValue; }
    T getMaxValue() const { return m_maxValue; }

protected:
    virtual bool set_internal(const T& value) override
    {
        if (m_minValue != m_maxValue)
            this->m_value = std::clamp(value, m_minValue, m_maxValue);
        else
            this->m_value = value;

        return true;
    }

    T m_minValue{};
    T m_maxValue{};
};

template <typename T> requires EnumTraitsConcept<T>
class EnumConfigVariable : public BasicConfigVariable<T>
{
public:
    using BasicConfigVariable<T>::BasicConfigVariable;

    const std::vector<std::pair<T, std::string>>& getValueMapping() const
    {
        return config_enum_traits<T>::values();
    }

protected:
    virtual bool set_internal(const T& value) override
    {
        for (const auto& [enumValue, _] : getValueMapping())
        {
            if (enumValue == value)
            {
                this->m_value = value;
                return true;
            }
        }

        return false;
    }
};

template <typename T>
class ConfigVariable;

template <typename T> requires (std::is_integral_v<T> || std::is_floating_point_v<T>) && (!std::is_same_v<T, bool>)
class ConfigVariable<T> : public NumericConfigVariable<T>
{
    using NumericConfigVariable<T>::NumericConfigVariable;
};

template <typename T> requires std::is_same_v<T, bool>
class ConfigVariable<T> : public BasicConfigVariable<T>
{
    using BasicConfigVariable<T>::BasicConfigVariable;
};

template <typename T> requires std::is_enum_v<T>
class ConfigVariable<T> : public EnumConfigVariable<T>
{
    using EnumConfigVariable<T>::EnumConfigVariable;
};

} // namespace Ui
