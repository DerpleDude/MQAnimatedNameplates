#pragma once
#include "Config.h"
// must come before Textures.h because Textures.h doesn't include memory even though it uses it. FOR SHAME.
#include <memory>

#include "mq/api/Textures.h"
#include "imgui/imgui.h"

namespace eqlib
{
class EQ_Spell;
class PlayerClient;
} // namespace eqlib

namespace Ui {

enum TextPositioning
{
    TopLeft,
    TopRight,
    TopCenter,
};

class Nameplate
{
public:
    Nameplate(const std::string& id, const std::string& textureFrame, const std::string& textureBar, eqlib::PlayerClient* pSpawn, ImU32 conColor);

    ImDrawList* GetDrawList();
    void Render(const ImVec2& center_pos, const ImVec2& frameSize, float percent, Ui::HPBarStyle style, bool currentTarget);

    void RenderAnimatedPercentageBar(const ImVec2& center_pos, const ImVec2& barSize, ImU32 colLow, ImU32 colMid, ImU32 colHigh,
                                     ImU32 colHighlight, bool currentTarget = false);

    void RenderNameplateText(const ImVec2& left_pos, ImU32 color, const char* text);

    void AddRectFilledMultiColorRounded(const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left,
                                        ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left,
                                        float rounding, ImDrawFlags flags);

    void RenderSpellIcon(const ImVec2& pos, eqlib::EQ_Spell* pSpell);
    
    void RenderDebugNameplateRect(const ImVec2& min, const ImVec2& max, ImU32 color, float rounding);

    time_t GetLastRenderTime() const { return m_lastRenderTime; }

  private:

    ImVec2 m_getTextPosition(TextPositioning location, const ImVec2& center_pos, const float lineWidth, const char* text, float& textWidthOut);

    mq::MQTexturePtr m_pTextureFrame;
    mq::MQTexturePtr m_pTextureBar;

    float m_smoothPercent{0.0f};
    float m_targetPercent{0.0f};
    int   m_trendDirection{0};

    std::string          m_id;
    ImU32                m_conColor{0};
    time_t               m_lastRenderTime{0};
    eqlib::PlayerClient* m_pSpawn;
};

} // namespace Ui
