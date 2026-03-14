#pragma once

#include "imgui.h"
#include "imgui/imanim/im_anim.h"
#include "imgui_internal.h"

// clang-format off
#include <memory>
#include "mq/api/Textures.h"
// clang-format on

namespace Ui
{
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