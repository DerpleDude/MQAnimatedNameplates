#pragma once

#include "Config.h"

#include "mq/api/Textures.h"
#include "mq/base/Color.h"
#include "imgui/imgui.h"

#include <chrono>

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
    Nameplate(const std::string& id, eqlib::PlayerClient* pSpawn, mq::MQColor conColor);
    Nameplate(const std::string& id, eqlib::PlayerClient* pSpawn, mq::MQColor conColor,
        const std::string& textureFrame, const std::string& textureBar);

    ImDrawList* GetDrawList();

    void Render(ImVec2& center_pos, const ImVec2& frameSize, float scale, float percent, Ui::HPBarStyle style,
        bool currentTarget);

    void RenderAnimatedPercentageBar(const ImVec2& center_pos, const ImVec2& barSize, ImU32 colLow,
        ImU32 colMid, ImU32 colHigh, ImU32 colHighlight, bool currentTarget = false);
    void RenderNameplateText(const ImVec2& left_pos, ImU32 color, const char* text);

    void AddRectFilledMultiColorRounded(const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left,
        ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left, float rounding, ImDrawFlags flags);

    void RenderSpellIcon(const ImVec2& pos, eqlib::EQ_Spell* pSpell);
    
    void RenderDebugInfo(const ImVec2& min, const ImVec2& max, ImU32 color, float rounding, float scale, float finalScale);

    std::chrono::steady_clock::time_point GetLastRenderTime() const { return m_lastRenderTime; }

  private:
    ImVec2 m_getTextPosition(TextPositioning location, const ImVec2& center_pos, const float lineWidth, const char* text, float& textWidthOut);

    mq::MQTexturePtr m_pTextureFrame;
    mq::MQTexturePtr m_pTextureBar;

    float m_smoothPercent{0.0f};
    float m_targetPercent{0.0f};
    int   m_trendDirection{0};

    std::string m_id;
    ImGuiID m_idHash;
    mq::MQColor m_conColor;
    std::chrono::steady_clock::time_point m_lastRenderTime{};
    eqlib::PlayerClient* m_pSpawn;
    ImVec2 m_lastPosition{ 0.0f, 0.0f };
    int m_renderCount{ 0 };
};

} // namespace Ui
