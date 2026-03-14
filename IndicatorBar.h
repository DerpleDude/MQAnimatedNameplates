#pragma once

#include "mq/api/Textures.h"
#include "imgui/imgui.h"

#include <memory>

namespace Ui {

class IndicatorBar
{
public:
    IndicatorBar(const std::string& textureFrame, const std::string& textureBar);
    void Render(const char* id, ImDrawList* dl, const ImVec2& center_pos, const ImVec2& frameSize,
                const ImVec2& barSize, float percent, float dt);

private:
    mq::MQTexturePtr m_pTextureFrame;
    mq::MQTexturePtr m_pTextureBar;
};

} // namespace Ui
