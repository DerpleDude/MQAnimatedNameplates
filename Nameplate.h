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
    Nameplate(eqlib::PlayerClient* pSpawn,
        const mq::MQTexturePtr& textureFrame = nullptr,
        const mq::MQTexturePtr& textureBar = nullptr);

    ImDrawList* GetDrawList();

    void Render(const ImVec2& center_pos, const ImVec2& frameSize, float scale);

    void RenderAnimatedPercentageBar(const ImVec2& center_pos, const ImVec2& barSize, ImU32 colLow,
        ImU32 colMid, ImU32 colHigh);

    void RenderNameplateText(const ImVec2& left_pos, ImU32 color, const char* text);

    void AddRectFilledMultiColorRounded(const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left,
        ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left, float rounding, ImDrawFlags flags);

    void RenderSpellIcon(const ImVec2& pos, eqlib::EQ_Spell* pSpell);
    
    void RenderDebugInfo(const ImVec2& min, const ImVec2& max, ImU32 color, float rounding, float scale, float finalScale);

    float GetSpawnPercentHP() const;

    bool IsCurrentTarget() const;
    bool IsInGroup() const;
    bool IsAutoHater() const;
    
    NameplateConfigGroup* GetConfig() const { return m_pConfigGroup; }

    void ResetRenderNameSpriteState();

    void SetNameplateType(Ui::NameplateType type);
    Ui::NameplateType GetNameplateType() const { return m_nameplateType; }
    void GetNameplateColors(ImU32& lowOut, ImU32& midOut, ImU32& highOut) const;

    std::chrono::steady_clock::time_point GetLastRenderTime() const { return m_lastRenderTime; }
    eqlib::PlayerClient* GetSpawn() const { return m_pSpawn; }
    float GetDistplaceToPlayer() const;

  private:
    ImVec2 m_getTextPosition(TextPositioning location, const ImVec2& center_pos, const float lineWidth, const char* text, float& textWidthOut);

    std::string m_id;
    ImGuiID m_idHash;

    std::chrono::steady_clock::time_point m_lastRenderTime{};
    eqlib::PlayerClient* m_pSpawn;
    mq::MQColor m_conColor;
    Ui::NameplateType m_nameplateType{ Ui::NameplateType::NameplateType_Invalid };

    mq::MQTexturePtr m_pTextureFrame;
    mq::MQTexturePtr m_pTextureBar;

    float m_smoothPercent{ 0.0f };
    float m_targetPercent{ 0.0f };

    bool m_originalDisplayNameplateState;

    NameplateConfigGroup* m_pConfigGroup;
};

} // namespace Ui
