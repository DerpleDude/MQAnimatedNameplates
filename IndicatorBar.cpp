#include "IndicatorBar.h"

#include "mq/Plugin.h"

namespace Ui
{
IndicatorBar::IndicatorBar(const std::string& textureFrame, const std::string& textureBar)
{
    m_pTextureFrame = CreateTexturePtr(textureFrame);
    m_pTextureBar   = CreateTexturePtr(textureBar);
}

void IndicatorBar::Render(const char* id, ImDrawList* dl, const ImVec2& center_pos, const ImVec2& frameSize,
                          const ImVec2& barSize, float percent, float dt)
{
    if (!m_pTextureFrame || !m_pTextureFrame->IsValid())
        return;

    ImVec2 framePos = center_pos - (frameSize / 2.0f);
    ImVec2 barPos   = center_pos - (barSize / 2.0f);

    if (m_pTextureBar && m_pTextureBar->IsValid())
    {
        float smoothPct = iam_tween_float(ImHashStr(id), ImHashStr("fill"), percent, 0.5f,
                                          iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt) /
                          100.0f;

        dl->PushClipRect(barPos, barPos + (barSize * ImVec2(smoothPct, 1.0f)), true);
        dl->AddImage(m_pTextureBar->GetTextureID(), barPos, barPos + barSize);
        dl->PopClipRect();
    }

    dl->AddImage(m_pTextureFrame->GetTextureID(), framePos, framePos + frameSize);
}

} // namespace Ui
