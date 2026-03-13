#include "ConfigVariable.h"
#include "Config.h"

namespace Ui
{

template <typename T>
ConfigVariable<T>::ConfigVariable(Config* config, const char* key, const T& v) : m_configKey(key), m_value(v)
{
    m_pConfig = config;
    m_pConfig->RegisterVariable(this);
}

template class ConfigVariable<bool>;
template class ConfigVariable<float>;
template class ConfigVariable<int>;

} // namespace Ui