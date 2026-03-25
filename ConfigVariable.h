
#pragma once

#include "mq/base/Color.h"

#include "fmt/format.h"
#include "yaml-cpp/yaml.h"

#include <concepts>
#include <string>
#include <vector>

namespace Ui {

class ConfigVariableBase;

template <typename T>
struct config_enum_traits;

namespace detail
{
    template <typename T>
    concept arithmetic = (std::integral<T> || std::floating_point<T>) && !std::is_same_v<T, bool>;
}

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

///////////////////////////////////////////////////////////////////////////////

class ConfigContainerBase
{
public:
    ConfigContainerBase();
    virtual ~ConfigContainerBase();

    ConfigContainerBase(ConfigContainerBase&&) noexcept;
    ConfigContainerBase& operator=(ConfigContainerBase&&) noexcept;

    ConfigContainerBase(const ConfigContainerBase&) = delete;
    ConfigContainerBase& operator=(const ConfigContainerBase&) = delete;

    void RegisterVariable(ConfigVariableBase*);
    void UnregisterVariable(ConfigVariableBase*);

    virtual void UpdateVariable(const ConfigVariableBase& variable) = 0;
    virtual void SetDirty(bool dirty) = 0;
    virtual YAML::Node GetNode() = 0;

protected:
    std::vector<ConfigVariableBase*> m_variables;
};

///////////////////////////////////////////////////////////////////////////////

class ConfigContainer : public ConfigContainerBase
{
public:
    ConfigContainer();

    ConfigContainer(ConfigContainer&&) noexcept;
    ConfigContainer& operator=(ConfigContainer&&) noexcept;

    virtual void UpdateVariable(const ConfigVariableBase& variable) override;
    virtual YAML::Node GetNode() override;

    virtual void SetDirty(bool dirty) override;
    bool IsDirty() const { return m_dirty; }

    void SaveConfig(const std::string& fileName);
    void LoadConfig(const std::string& fileName);

private:
    YAML::Node m_configNode;
    bool m_dirty = false;
};

///////////////////////////////////////////////////////////////////////////////

class ConfigVariableBase
{
    friend class ConfigContainerBase;

public:
    ConfigVariableBase(ConfigContainerBase* container, std::string keyName);
    virtual ~ConfigVariableBase();

    ConfigVariableBase(ConfigVariableBase&&) noexcept;
    ConfigVariableBase& operator=(ConfigVariableBase&&) noexcept;

    ConfigVariableBase(const ConfigContainerBase&) = delete;
    ConfigVariableBase& operator=(const ConfigVariableBase&) = delete;

    virtual void Load(const YAML::Node& source) = 0;
    virtual bool Store(YAML::Node& target) const = 0;

    const std::string& getKey() const noexcept { return m_keyName; }

protected:
    ConfigContainerBase* m_container;
    std::string m_keyName;
};

///////////////////////////////////////////////////////////////////////////////

class ConfigGroup 
    : public ConfigContainerBase
    , public ConfigVariableBase
{
public:
    ConfigGroup(ConfigContainerBase& container, std::string keyName);
    virtual ~ConfigGroup() override;

    ConfigGroup(ConfigGroup&&) noexcept;
    ConfigGroup& operator=(ConfigGroup&&) noexcept;

    // ConfigContainerBase
    virtual void UpdateVariable(const ConfigVariableBase& variable) override;
    virtual void SetDirty(bool dirty) override;
    virtual YAML::Node GetNode() override;

    // ConfigVariableBase
    virtual void Load(const YAML::Node& source) override;
    virtual bool Store(YAML::Node& target) const override;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Base traits for simple types that can be directly converted to/from YAML.
template <typename T, typename U = T>
struct BaseVariableTraits
{
    using ValueType = T;
    using StorageType = U;

    // if you need me, override me for correct behavior
    static StorageType convert_to_storage(const ValueType& v)
    {
        if constexpr (std::is_same_v<ValueType, StorageType>)
            return v;
        else if constexpr (std::is_convertible_v<ValueType, StorageType>)
            return static_cast<StorageType>(v);
        else
            return StorageType{};
    }

    // if you need me, override me for correct behavior
    static ValueType convert_to_value(const StorageType& s)
    {
        if constexpr (std::is_same_v<StorageType, ValueType>)
            return s;
        else if (std::is_convertible_v<StorageType, ValueType>)
            return static_cast<ValueType>(s);
        else
            return ValueType{};
    }

    static ValueType get_default()
    {
        return ValueType{};
    }

    static bool set_value(ValueType& existing_value, const ValueType& new_value)
    {
        existing_value = new_value;
        return true;
    }
};

// Traits for enum types that require mapping to/from strings.
template <EnumTraitsConcept T>
struct EnumVariableTraits : BaseVariableTraits<T, std::underlying_type_t<T>>
{
    using ValueType = T;
    using StorageType = std::underlying_type_t<T>;

    static StorageType convert_to_storage(const ValueType& v)
    {
        return static_cast<std::underlying_type_t<ValueType>>(v);
    }

    static ValueType convert_to_value(const StorageType& s)
    {
        return static_cast<T>(s);
    }

    static bool set_value(ValueType& existing_value, const ValueType& new_value)
    {
        for (const auto& [enumValue, _] : config_enum_traits<ValueType>::values())
        {
            if (enumValue == new_value)
            {
                existing_value = new_value;
                return true;
            }
        }

        return false;
    }
};

// Traits for mq::MQColor that convert to/from hex string representation.
struct ColorVariableTraits : BaseVariableTraits<mq::MQColor>
{
    using ValueType = mq::MQColor;
    using StorageType = std::string;

    static StorageType convert_to_storage(const ValueType& value)
    {
        return fmt::format("#{:06x}", value.ToARGB() & 0x00FFFFFF);
    }

    static ValueType convert_to_value(const StorageType& s)
    {
        try
        {
            return mq::MQColor(s.c_str());
        }
        catch (mq::detail::InvalidHexChar)
        {
            return mq::MQColor(0, 0, 0);
        }
    }
};

template <typename T>
struct VariableTraits : BaseVariableTraits<T> {};

template <typename T> requires EnumTraitsConcept<T>
struct VariableTraits<T> : EnumVariableTraits<T> {};

template <typename T> requires std::is_same_v<T, mq::MQColor>
struct VariableTraits<T> : ColorVariableTraits {};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename T, typename Traits = VariableTraits<T>>
class BasicConfigVariable : public ConfigVariableBase
{
public:
    using TraitsType = Traits;
    using ValueType = TraitsType::ValueType;
    using StorageType = TraitsType::StorageType;

    BasicConfigVariable(ConfigContainerBase& container, std::string keyName, const ValueType& defaultValue)
        : ConfigVariableBase(&container, std::move(keyName))
        , m_value(defaultValue)
    {
    }

    BasicConfigVariable& operator=(const ValueType& v)
    {
        set(v);
        return *this;
    }

    operator const ValueType& () const noexcept { return m_value; }

    const ValueType& get() const noexcept { return m_value; }

    void set(const ValueType& v)
    {
        if (set_internal(v))
        {
            m_container->UpdateVariable(*this);
        }
    }

    virtual bool Store(YAML::Node& yamlNode) const override final
    {
        yamlNode[m_keyName] = TraitsType::convert_to_storage(m_value);
        return true;
    }

    virtual void Load(const YAML::Node& yamlNode) override final
    {
        if (auto node = yamlNode[m_keyName])
        {
            try
            {
                set_internal(TraitsType::convert_to_value(node.template as<StorageType>()));
            }
            catch (const YAML::BadConversion&)
            {
                set_internal(TraitsType::get_default());
            }
        }
    }

protected:
    virtual bool set_internal(const ValueType& v)
    {
        return TraitsType::set_value(m_value, v);
    }

    ValueType m_value;
};

template <detail::arithmetic T>
class NumericConfigVariable : public BasicConfigVariable<T>
{
public:
    NumericConfigVariable(ConfigContainerBase& container, const char* key, T defaultValue)
        : BasicConfigVariable<T>(container, key, defaultValue)
    {
    }

    NumericConfigVariable(ConfigContainerBase& container, const char* key, T defaultValue, T minValue, T maxValue)
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

template <typename T>
class ConfigVariable : public BasicConfigVariable<T>
{
    using BasicConfigVariable<T>::BasicConfigVariable;
};

template <detail::arithmetic T>
class ConfigVariable<T> : public NumericConfigVariable<T>
{
    using NumericConfigVariable<T>::NumericConfigVariable;
};

} // namespace Ui
