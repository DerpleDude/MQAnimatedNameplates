#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include <string>
#include <vector>

namespace Ui {

bool AnimatedCheckbox(const char* label, bool* value);
bool AnimatedCombo(const char* label, int* value, const std::vector<std::string>& items);

bool AnimatedSlider(const char* label, float* slider_value, float slider_min, float slider_max,
    const char* format = "%.2f", float width = 0.0f);
bool AnimatedSlider(const char* label, int* slider_value, int slider_min, int slider_max,
    const char* format = "%d", float width = 0.0f);
bool AnimatedSlider(const char* label, unsigned int* slider_value, unsigned int slider_min, unsigned int slider_max,
    const char* format = "%d", float width = 0.0f);

template <typename T> requires std::is_enum_v<T>
bool AnimatedEnumCombo(const char* label, T* value);

class AnimatedCheckmark
{
public:
    AnimatedCheckmark(bool value, const char* label)
        : m_newValue(value)
    {
        char id[32];
        sprintf_s(id, "%s_path1", label);
        m_animIdPath1 = ImHashStr(id);
        sprintf_s(id, "%s_path2", label);
        m_animIdPath2 = ImHashStr(id);

        Reset(value);
    }

    void Reset(bool newVal);
    void Render(ImDrawList* dl, const ImRect& check_bb, float box_size);

private:
    ImGuiID m_animIdPath1;
    ImGuiID m_animIdPath2;

    float m_path1Time = 0.0f;
    float m_path2Time = 0.0f;
    bool  m_newValue = false;
    bool  m_pathInitialized = false;
    bool  m_path1Complete = false;
    bool  m_path2Complete = false;
    float m_animSpeed = 6.0f;
};

struct AnimatedTabState
{
    int                   idx;
    std::string           name;
    std::function<void()> content;
};

struct AnimatedComboState
{
    bool  open = false;
    float open_time = 0.0f;
};

} // namespace Ui
