#include "Nameplate.h"
#include "Config.h"
#include "MaskedImage.h"

#include "eqlib/EQLib.h"
#include "imgui/imgui_internal.h"
#include "imgui/imanim/im_anim.h"
#include "mq/imgui/Widgets.h"
#include "mq/Plugin.h"

namespace Ui {

static MaskedImage g_maskedImage{ "ruler.png", "BarBorders\\blizzard-cast-bar-square-mask.png" };
static MaskedImage g_maskedImage2{ "fishface.png", "BarBorders\\blizzard-cast-bar-square-mask.png" };
static MaskedImage g_maskedImage3{ "energy_filler_gold.png", "BarBorders\\round-square-mask.png" };

static const ImGuiID pct_id = ImHashStr("pct_tween");
static const ImGuiID target_scale_id = ImHashStr("target_scale_tween");

ImU32 ReduceAlpha(ImU32 col, float factor)
{
    ImU32 a = (col >> 24) & 0xFF;   // extract alpha
    a = static_cast<ImU32>(a * factor);

    return (col & 0x00FFFFFF) | (a << 24);
}

Nameplate::Nameplate(const std::string& id, eqlib::PlayerClient* pSpawn)
    : m_id(id)
    , m_idHash(ImHashStr(id.c_str()))
    , m_pSpawn(pSpawn)
    , m_conColor(GetColorForChatColor(ConColor(pSpawn)))
    , m_targetPercent(GetSpawnPercentHP() / 100.f)
{
    InitIamAnim();
}

Nameplate::Nameplate(const std::string& id, eqlib::PlayerClient* pSpawn,
    const std::string& textureFrame, const std::string& textureBar)
    : m_id(id)
    , m_idHash(ImHashStr(id.c_str()))
    , m_pSpawn(pSpawn)
    , m_conColor(GetColorForChatColor(ConColor(pSpawn)))
    , m_targetPercent(GetSpawnPercentHP() / 100.f)
{
    m_pTextureFrame = mq::CreateTexturePtr(textureFrame);
    m_pTextureBar = mq::CreateTexturePtr(textureBar);

    InitIamAnim();
}

void Nameplate::InitIamAnim()
{
    float dt = ImGui::GetIO().DeltaTime;

    bool lazy_init_enabled = iam_is_lazy_init_enabled();
    iam_set_lazy_init(false);

    m_smoothPercent = iam_tween_float(ImHashStr(m_id.c_str()), pct_id, m_targetPercent * 100.0f, 0.5f,
        iam_ease_preset(iam_ease_linear), iam_policy_crossfade, dt, m_targetPercent * 100.0f) / 100.0f;

    iam_set_lazy_init(lazy_init_enabled);
}

ImDrawList* Nameplate::GetDrawList()
{
    return Ui::Config::Get().RenderToForeground ? ImGui::GetForegroundDrawList() : ImGui::GetBackgroundDrawList();
}

void Nameplate::Render(ImVec2& center_pos, const ImVec2& frameSize, float scale)
{
    // track the last render and clean up after 30s of non-usage.
    m_lastRenderTime = std::chrono::steady_clock::now();
    float percent = GetSpawnPercentHP();

    ImDrawList* drawList = Nameplate::GetDrawList();
    Ui::Config& config = Ui::Config::Get();

    if (config.DrawTestBar)
    {
        ImVec2 testBarPos{ 400, 450 };
        ImVec2 testBarSize = ImVec2{ 512, 64 } * ImVec2{ 2.0f, 2.0f };
        //ImVec4 margins{ 14,14,14,14 };
        ImVec4 margins{ 21,21,21,21 };
        //ImVec4 margins{ 17,17,17,17 };

        g_maskedImage.RenderNineSlice(drawList, testBarPos, testBarPos + testBarSize, ImVec2{ 42,42 }, margins);
        testBarPos += ImVec2{ 0, 150 };
        g_maskedImage2.Render(drawList, testBarPos, testBarPos + ImVec2{ 64,64 });
        testBarPos += ImVec2{ 0, 100 };
        g_maskedImage3.RenderNineSlice(drawList, testBarPos, testBarPos + ImVec2{ 512, 64 }, ImVec2{ 42,42 }, margins);
    }

    float finalScale = std::clamp(config.ScaleWithDistance ? (1.0f/scale) : 1.0f * config.ScaleFactor, config.MaxCalculatedScaleFactor.getMinValue(), config.MaxCalculatedScaleFactor.get());

    float targetScale = 1.0f;
    if (IsCurrentTarget())
    {
        targetScale = 1.1f;
    }

    float targetTween = iam_tween_float(m_idHash, target_scale_id, targetScale, 0.05f,
        iam_ease_preset(iam_ease_linear), iam_policy_crossfade, ImGui::GetIO().DeltaTime, 1.00f);
    finalScale *= targetTween;

    float dt = ImGui::GetIO().DeltaTime;

    ImVec2 scaledFameSize = frameSize * ImVec2(finalScale, finalScale);

    ImGui::PushFont(nullptr, config.FontSize );

    const ImVec2 padding = ImGui::GetStyle().FramePadding;
    const ImVec2 barSize{
        scaledFameSize.x - padding.x * 2,
        scaledFameSize.y - padding.y * 2
    };

    if (m_renderCount++ % 2 == 0)
    {
        m_lastPosition = center_pos;
    }

    center_pos = m_lastPosition;

    ImVec2 framePos = center_pos - (scaledFameSize / 2.0f);
    ImVec2 barPos   = center_pos - (barSize / 2.0f);
    ImVec2 topLeft  = framePos;
    ImVec2 botRight = center_pos + (barSize / 2.0f);

    m_targetPercent = std::clamp(percent, 0.0f, 100.0f) / 100.f;

    m_smoothPercent = iam_tween_float(m_idHash, pct_id, m_targetPercent * 100.0f, 0.5f,
        iam_ease_preset(iam_ease_linear), iam_policy_crossfade, dt, m_targetPercent * 100.0f) / 100.0f;

    if (m_pTextureBar && m_pTextureBar->IsValid())
    {
        drawList->PushClipRect(barPos, barPos + (barSize * ImVec2(m_smoothPercent, 1.0f)), true);
        drawList->AddImage(m_pTextureBar->GetTextureID(), barPos, barPos + barSize);
        drawList->PopClipRect();
    }
    else
    {
        ImU32 hpLow;
        ImU32 hpMid;
        ImU32 hpHigh;

        ImU32 highlightColor;

        GetNameplateColors(hpLow, hpMid, hpHigh, highlightColor);

        RenderAnimatedPercentageBar(center_pos, barSize, hpLow, hpMid, hpHigh, highlightColor);
    }

    if (m_pTextureFrame && m_pTextureFrame->IsValid())
    {
        drawList->AddImage(m_pTextureFrame->GetTextureID(), framePos, framePos + scaledFameSize);
    }

    ImVec2 textLinePos = center_pos - ImVec2(0, barSize.y / 2.0f + ImGui::GetTextLineHeightWithSpacing() + padding.y);
    ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);

    // if we will render guild/purpose text, move the text up a line to make room.
    if ((config.ShowGuild && eqlib::pGuild && m_pSpawn->GuildID > 0)
        || (config.ShowPurpose && GetSpawnType(m_pSpawn) == NameplateType_NPC && m_pSpawn->Lastname[0]))
    {
        //
        // Detail
        //

        std::string targetDetail;
        if (config.ShowPurpose && GetSpawnType(m_pSpawn) == NameplateType_NPC && m_pSpawn->Lastname[0])
        {
            targetDetail = fmt::format("({})", m_pSpawn->Lastname);
        }
        else if (config.ShowGuild && pGuild && m_pSpawn->GuildID > 0)
        {
            targetDetail = fmt::format("<{}>", pGuild->GetGuildName(m_pSpawn->GuildID));
        }

        if (!targetDetail.empty())
        {
            // center this text
            float detailTextWidth = 0.0f;
            RenderNameplateText(m_getTextPosition(TextPositioning::TopCenter, textLinePos, barSize.x, targetDetail.c_str(), detailTextWidth), textColor, targetDetail.c_str());
            textLinePos.y -= ImGui::GetTextLineHeightWithSpacing();
        }
    }

    // Render Text Elements
    // These render in reverse order so we can move them upward if we run out of space without impacting our bar position which should be static.

    // Calc the next 2 elements and see if we overlap.

    float nameTextWidth = 0.0f;
    ImVec2 nameTextPos = m_getTextPosition(TextPositioning::TopLeft, textLinePos, barSize.x, m_pSpawn->DisplayedName, nameTextWidth);

    std::string targetLevel = fmt::format("{}", m_pSpawn->GetLevel());
    float levelTextWidth = 0.0f;
    ImVec2 levelTextPos = m_getTextPosition(TextPositioning::TopRight, textLinePos, barSize.x, targetLevel.c_str(), levelTextWidth);
    
    //
    // Class
    //
    if (config.ShowClass)
    {
        std::string classInfo;

        if (m_pSpawn->GetClass() < 1 || m_pSpawn->GetClass() > 16)
            classInfo = "???";
        else if (config.ShortClassName)
            classInfo = pEverQuest->GetClassThreeLetterCode(m_pSpawn->GetClass());
        else
            classInfo = GetClassDesc(m_pSpawn->GetClass());

        float classTextWidth = 0.0f;
        ImVec2 classTextPos = m_getTextPosition(TextPositioning::TopCenter, textLinePos, barSize.x, classInfo.c_str(), classTextWidth);
        RenderNameplateText(classTextPos, textColor, classInfo.c_str());

        if (classTextPos.x < nameTextPos.x + nameTextWidth || classTextPos.x > levelTextPos.x)
        {
            textLinePos.y -= ImGui::GetTextLineHeightWithSpacing();
            levelTextPos.y = textLinePos.y;
            nameTextPos.y = textLinePos.y;
        }
    }

    //
    // Name Text
    //

    RenderNameplateText(nameTextPos, textColor, m_pSpawn->DisplayedName);
    topLeft = nameTextPos;

    //
    // Level
    //

    if (Config::Get().ShowLevel)
        RenderNameplateText(levelTextPos, textColor, targetLevel.c_str());

    //
    // Buff Icons
    //

    // only render for target.
    if (config.ShowBuffIcons && pTarget == m_pSpawn)
    {
        int buffsPerRow = std::max(1, static_cast<int>(floorf(scaledFameSize.x / (config.IconSize + padding.x))));

        int buffCount = config.ShowBuffIcons ? GetCachedBuffCount(m_pSpawn) : 0;

        float numBuffRows = ceilf(buffCount / static_cast<float>(buffsPerRow));

        float verticalOffset = numBuffRows * (config.IconSize + padding.y);

        ImVec2 buffPos = topLeft - ImVec2(0, verticalOffset);
        float buffPosXStart = buffPos.x;
        topLeft = buffPos;

        // this draws above the nameplate so we can use a seperate cursor for it since it will not change the cursor for
        // the plate.
        int iconsDrawn = 0;
        for (int i = 0; i < buffCount; i++)
        {
            auto buff = GetCachedBuffAtSlot(m_pSpawn, i);

            if (buff.has_value())
            {
                if (EQ_Spell* spell = GetSpellByID(buff->spellId))
                {
                    RenderSpellIcon(buffPos, spell);
                    iconsDrawn += 1;
                    
                    if ((iconsDrawn) % buffsPerRow == 0)
                    {
                        buffPos.x = buffPosXStart;
                        buffPos.y += config.IconSize + padding.y;
                    }
                    else
                    {
                        buffPos.x += config.IconSize + padding.x;
                    }
                }
            }
        }
        
    }

    ImGui::PopFont();

    if (IsCurrentTarget() && Config::Get().ShowTargetIndicator)
    {
        /*
        // draw some wings or something if this is our target.
        float hue = (float)0.75;
        ImVec4 col_hsv(hue, 0.8f, 0.9f, .8f);
        ImVec4 col_rgb;
        ImGui::ColorConvertHSVtoRGB(col_hsv.x, col_hsv.y, col_hsv.z, col_rgb.x, col_rgb.y, col_rgb.z);
        col_rgb.w = .8f;

        // this was dumb needs replaced with some sort of scale or glow.
        //drawList->AddRectFilled(topLeft, botRight, IM_COL32((int)(col_rgb.x * 255), (int)(col_rgb.y * 255), (int)(col_rgb.z * 255), (int)(0.8 * 40)));
        
        static ImVec4 colorAmp(0.1f, .1f, .1f, .0f);
        
        static ImVec4 color{ m_conColor.Red / 255.0f, m_conColor.Green / 255.0f, m_conColor.Blue / 255.0f, m_conColor.Alpha / 255.0f };
        
        //ImVec4 oscColor = iam_oscillate_color(m_idHash, color, colorAmp, 2.0f, iam_wave_sine, 0.0f, iam_col_srgb, dt);
        */
        ImVec2 baseOffset{ config.TargetIndicatorPadding, config.TargetIndicatorPadding };
        int alphaOsc = 80 + iam_oscillate_int(m_idHash, 40, config.TargetIndicatorBlinkSpeed, iam_wave_sine, 0.0f, dt);
        drawList->AddRect(topLeft - baseOffset, botRight + baseOffset, ReduceAlpha(m_conColor.ToImU32(), alphaOsc / 255.0f), 4.0f, 0, 2.0f);
    }
    
    // click handling
    if (ImGui::IsMouseHoveringRect(topLeft, botRight, false))
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            pTarget = m_pSpawn;
        }
    }

    // Render Debug Overlay
    RenderDebugInfo(topLeft, botRight, IM_COL32(40, 240, 40, 55), 3.0f, scale, finalScale);
}

void Nameplate::RenderNameplateText(const ImVec2& left_pos, ImU32 color, const char* text)
{
    ImDrawList* drawList = Nameplate::GetDrawList();
    drawList->AddText(left_pos, color, text);
}

void Nameplate::RenderAnimatedPercentageBar(const ImVec2& center_pos, const ImVec2& barSize,
                                            ImU32 colLow, ImU32 colMid, ImU32 colHigh, ImU32 colHighlight)
{
    ImDrawList* drawList = Nameplate::GetDrawList();
    Ui::Config& config = Ui::Config::Get();
    bool currentTarget = IsCurrentTarget();

    // FIXME: This should be accumulated time, not absolute time
    float dt = static_cast<float>(ImGui::GetTime());

    ImVec2 min = center_pos - (barSize / 2.0f);
    ImVec2 max = center_pos + (barSize / 2.0f);

    float barW = barSize.x;
    float barH = barSize.y;

    float fillWidth = barW * m_smoothPercent;
    float fillMaxX = min.x + fillWidth;
    float fillWithTarget = barW * m_targetPercent;
    float fillMaxTargetX = min.x + fillWithTarget;

    ImVec2 innerMin = min + ImVec2(1, +1);
    ImVec2 innerMax  = max - ImVec2(1, +1);
    ImVec2 fillMax  = ImVec2(fillMaxX, max.y);
    ImVec2 fillMaxTarget = ImVec2(fillMaxTargetX, max.y);
    ImVec2 innerFillMax = fillMax - ImVec2(1, 1);
    ImVec2 innerFillMaxTarget = fillMaxTarget - ImVec2(1, 1);

    ImU32 bgTop    = IM_COL32(28, 30, 41, 247 * config.ColorAlphaModifier);
    ImU32 bgBottom = IM_COL32(10, 13, 20, 247 * config.ColorAlphaModifier);

    // Dark background
    AddRectFilledMultiColorRounded(innerMin, innerMax, bgTop, bgTop, bgBottom, bgBottom, config.BarRounding, 0);

    drawList->AddRectFilled(innerMin, ImVec2(max.x - 1, min.y + std::max(2.0f, barH * 0.35f)),
        IM_COL32(255, 255, 255, 14), config.BarRounding);

    if (fillWidth > 0)
    {
        ImU32 edge;

        if (colHigh == colLow && colLow == colMid)
        {
            edge = colLow;
        }
        else
        {
            if (m_smoothPercent < 0.5f)
            {
                float t = m_smoothPercent / 0.5f;
                edge    = ImLerp(colLow, colMid, t);
            }
            else
            {
                float t = (m_smoothPercent - 0.5f) / 0.5f;
                edge    = ImLerp(colMid, colHigh, t);
            }
        }

        ImU32 topLeft = colLow;
        ImU32 topRight = edge;
        ImU32 bottomLeft = topLeft;
        ImU32 bottomRight = topRight;

        float fillRounding = std::min({ config.BarRounding.get(), barH * 0.5f, fillWidth * 0.5f });
        
        if (fillMax.x > innerMin.x && fillMax.y > innerMin.y)
        {
            // Draw one for where we are going and another for the smooth.
            if (m_smoothPercent > m_targetPercent)
            {
                // moving down
                AddRectFilledMultiColorRounded(innerMin, innerFillMax, ReduceAlpha(topLeft, 0.5f), ReduceAlpha(topRight,0.5f), ReduceAlpha(bottomRight, 0.5f), ReduceAlpha(bottomLeft,0.5f), config.BarRounding, 0);
                AddRectFilledMultiColorRounded(innerMin, innerFillMaxTarget, topLeft, topRight, bottomRight, bottomLeft, config.BarRounding, 0);
                    
            }
            else
            {
                // moving up
                AddRectFilledMultiColorRounded(innerMin, innerFillMaxTarget, ReduceAlpha(topLeft, 0.5f), ReduceAlpha(topRight, 0.5f), ReduceAlpha(bottomRight, 0.5f), ReduceAlpha(bottomLeft, 0.5f), config.BarRounding, 0);
                AddRectFilledMultiColorRounded(innerMin, innerFillMax, topLeft, topRight, bottomRight, bottomLeft, config.BarRounding, 0);
            }

            float glossMaxY = std::min(innerFillMax.y, min.y + std::max(2.0f, barH * 0.45f));

            if (glossMaxY > innerMin.y)
            {
                AddRectFilledMultiColorRounded(innerMin, ImVec2(innerFillMax.x, glossMaxY),
                    IM_COL32(255, 255, 255, 14), IM_COL32(255, 255, 255, 8),
                    IM_COL32(255, 255, 255, 2), IM_COL32(255, 255, 255, 8),
                    config.BarRounding, 0);
            }
        }
        else
        {
            drawList->AddRectFilled(min, fillMax, colLow, fillRounding);
        }
    }

    int hpTicks = 100 / Ui::Config::Get().HPTicks;

    for (int i = 1; i < hpTicks; ++i)
    {
        float tx = min.x + (barW * (i / static_cast<float>(hpTicks)));
        bool  reached = tx <= (min.x + fillWidth);

        drawList->AddLine(ImVec2(tx - 1, min.y + 1), ImVec2(tx - 1, max.y - 1),
            IM_COL32(0, 0, 0, static_cast<int>((reached ? 0.15 : 0.3) * 255)), 1.0f);
        drawList->AddLine(ImVec2(tx, min.y + 1), ImVec2(tx, max.y - 1),
            IM_COL32(255, 255, 255, static_cast<int>((reached ? 0.3 : 0.15) * 255)), 1.0f);
    }

    drawList->AddRect(min, max, colHighlight, Config::Get().BarRounding, 0, Config::Get().BarBorderThickness);

    std::string text = std::to_string(static_cast<int>(std::floor((m_targetPercent*100.f) + 0.5f))) + "%";

    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());

    float textX = min.x + ((max.x - min.x - textSize.x) * 0.5f);
    float textY = min.y + ((barH - ImGui::GetTextLineHeight()) * 0.5f);

    drawList->AddText(ImVec2(textX + 1, textY + 1), IM_COL32(0, 0, 0, 230), text.c_str());
    drawList->AddText(ImVec2(textX, textY), IM_COL32(255, 255, 255, 255), text.c_str());
}

void Nameplate::RenderDebugInfo(const ImVec2& min, const ImVec2& max, ImU32 color, float rounding, float scale, float finalScale)
{
    Ui::Config& config = Ui::Config::Get();
    ImDrawList* drawList = Nameplate::GetDrawList();

    if (config.ShowDebugText)
    {
        ImU32 pink = IM_COL32(240, 80, 240, 255);
        char debugText[128];
        sprintf_s(debugText, "CurrentTarget: %s Distance: %.2f\nScale: %.5f FinalScale: %.5f\nTargetPct: %.2f SmoothPct: %.2f", IsCurrentTarget() ? "true" : "false", GetDistplaceToPlayer(), 1.0f / scale, finalScale, m_targetPercent, m_smoothPercent);

        ImVec2 textPos{ min.x, max.y + ImGui::GetTextLineHeightWithSpacing() };
        ImVec2 textSize = ImGui::CalcTextSize(debugText);
        drawList->AddRectFilled(textPos + ImVec2{ -4,-4 }, textPos + textSize + ImVec2{ 4,4 }, IM_COL32(0, 0, 0, 150));
        drawList->AddText(textPos, pink, debugText);
    }

    if (config.ShowDebugBounds)
        drawList->AddRectFilled(min, max, color, rounding);
}

void Nameplate::RenderSpellIcon(const ImVec2& pos, eqlib::EQ_Spell* pSpell)
{
    ImVec2 size(Config::Get().IconSize, Config::Get().IconSize);
    ImVec2 max(pos + size);

    ImDrawList* drawList = Nameplate::GetDrawList();

    if (pSidlMgr)
    {
        if (CTextureAnimation* anim = pSidlMgr->FindAnimation("A_SpellGems"))
        {
            int iconID = pSpell->SpellIcon;
            anim->SetCurCell(iconID);
            mq::imgui::DrawTextureAnimation(drawList, anim, pos, size);
        }
    }
}

bool Nameplate::IsCurrentTarget() const
{
    return pTarget == m_pSpawn;
}

bool Nameplate::IsInGroup() const
{
    if (pLocalPC->pGroupInfo)
        return false;

    return pLocalPC->pGroupInfo->GetGroupMember(m_pSpawn) != nullptr;
}

bool Nameplate::IsAutoHater() const
{
    if (pLocalPC)
        return false;

    ExtendedTargetList* xtm = pLocalPC->pExtendedTargetList;

    for (int i = 0; i < xtm->GetNumSlots(); i++)
    {
        ExtendedTargetSlot* xts = xtm->GetSlot(i);

        if (xts->SpawnID == m_pSpawn->SpawnID && xts->xTargetType == XTARGET_AUTO_HATER)
        {
            return true;
        }
    }

    return false;
}

float Nameplate::GetDistplaceToPlayer() const
{
    if (!m_pSpawn)
        return 0.0f;

    return GetDistance(m_pSpawn->Y, m_pSpawn->X, pControlledPlayer->Y, pControlledPlayer->X);
}

void Nameplate::GetNameplateColors(ImU32& lowOut, ImU32& midOut, ImU32& highOut, ImU32& highlightOut) const
{
    Ui::HPBarStyle style = GetBarStyle();
    Ui::Config& config = Ui::Config::Get();

    bool currentTarget = IsCurrentTarget();
    lowOut = IM_COL32(204, 51, 51, 255);
    midOut = IM_COL32(230, 179, 51, 255);
    highOut = IM_COL32(51, 230, 51, 255);

    highlightOut = m_conColor.ToImU32();

    switch (style)
    {
    case HPBarStyle_Custom1:
        lowOut = midOut = highOut = ReduceAlpha(config.CustomColor1.get().ToImU32(), config.ColorAlphaModifier);
        highlightOut = currentTarget ? IM_COL32(255, 128, 0, 255) : IM_COL32(240, 80, 240, 255);
        break;
    case HPBarStyle_Custom2:
        lowOut = midOut = highOut = ReduceAlpha(config.CustomColor2.get().ToImU32(), config.ColorAlphaModifier);
        highlightOut = currentTarget ? IM_COL32(255, 128, 0, 255) : IM_COL32(240, 80, 240, 255);
        break;
    case HPBarStyle_Custom3:
        lowOut = midOut = highOut = ReduceAlpha(config.CustomColor3.get().ToImU32(), config.ColorAlphaModifier);
        highlightOut = currentTarget ? IM_COL32(255, 128, 0, 255) : IM_COL32(240, 80, 240, 255);
        break;
    case HPBarStyle_Custom4:
        lowOut = midOut = highOut = ReduceAlpha(config.CustomColor4.get().ToImU32(), config.ColorAlphaModifier);
        highlightOut = currentTarget ? IM_COL32(255, 128, 0, 255) : IM_COL32(240, 80, 240, 255);
        break;
    case HPBarStyle_Custom5:
        lowOut = midOut = highOut = ReduceAlpha(config.CustomColor5.get().ToImU32(), config.ColorAlphaModifier);
        highlightOut = currentTarget ? IM_COL32(255, 128, 0, 255) : IM_COL32(240, 80, 240, 255);
        break;
    case HPBarStyle_Custom6:
        lowOut = midOut = highOut = ReduceAlpha(config.CustomColor6.get().ToImU32(), config.ColorAlphaModifier);
        highlightOut = currentTarget ? IM_COL32(255, 128, 0, 255) : IM_COL32(240, 80, 240, 255);
        break;
    case HPBarStyle_ConColor:
        lowOut = midOut = highOut = ReduceAlpha(m_conColor.ToImU32(), config.ColorAlphaModifier);
        highlightOut = currentTarget ? IM_COL32(255, 128, 0, 255) : IM_COL32(240, 80, 240, 255);
        break;
    case HPBarStyle_ColorRange:
        break;
    default: break;
    }
}

Ui::HPBarStyle Nameplate::GetBarStyle() const
{
    Ui::Config& config = Ui::Config::Get();

    switch (m_nameplateType)
    {
        case Ui::NameplateType::NameplateType_Self:
            return config.HPBarStyleSelf;
        case Ui::NameplateType::NameplateType_Group:
            return config.HPBarStyleGroup;
        case Ui::NameplateType::NameplateType_Target:
            return config.HPBarStyleTarget;
        case Ui::NameplateType::NameplateType_AutoHater:
            return config.HPBarStyleHaters;
        case Ui::NameplateType::NameplateType_NPC:
            return config.HPBarStyleNPCs;
            default:
                return Ui::HPBarStyle_Invalid;
    }
}   

void Nameplate::SetNameplateType(Ui::NameplateType type)
{
    m_nameplateType = type;
}

float Nameplate::GetSpawnPercentHP() const
{
    Ui::Config& config = Ui::Config::Get();

    if (config.DrawTestBar)
        return config.BarPercent;

    if (!m_pSpawn)
        return 0.0f;

    return m_pSpawn->HPMax == 0 ? 0 : m_pSpawn->HPCurrent * 100.0f / m_pSpawn->HPMax;
}

ImVec2 Nameplate::m_getTextPosition(TextPositioning location, const ImVec2& center_pos, const float lineWidth, const char* text, float& textWidthOut)
{
    ImVec2 ret = center_pos;
    textWidthOut = ImGui::CalcTextSize(text).x;

    switch (location)
    {
    case TextPositioning::TopLeft:
        ret.x = (center_pos.x - lineWidth / 2.0f);
        break;
    case TextPositioning::TopCenter:
        ret.x = (center_pos.x) - (textWidthOut / 2.0f);
        break;
    case TextPositioning::TopRight:
        ret.x = (center_pos.x + lineWidth / 2.0f) - textWidthOut;
        break;
    }

    return ret;
}

//============================================================================

static void Normalize2fOverZero(float& x, float& y)
{
    if (const float d2 = x * x + y * y; d2 > 0.0f)
    {
        const float inv_len = ImRsqrt(d2);
        x *= inv_len;
        y *= inv_len;
    }
}

// Helper used by ImGui's AA fringe generation: scales by inv_len^2 and clamps
// to avoid extremely large normals on nearly-degenerate segments.
static void FixNormal2f(float& x, float& y)
{
    if (const float d2 = x * x + y * y; d2 > 0.000001f)
    {
        // This mirrors the intent in upstream imgui_draw.cpp (see ImGui issues #4053, #3366).
        constexpr float FixNormalMaxInvLen2 = 100.0f;

        float inv_len2 = 1.0f / d2;
        inv_len2       = std::min(inv_len2, FixNormalMaxInvLen2);

        x *= inv_len2;
        y *= inv_len2;
    }
}

void Nameplate::AddRectFilledMultiColorRounded(const ImVec2& p_min, const ImVec2& p_max,
                                        ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right,
                                        ImU32 col_bot_left, float rounding, ImDrawFlags flags)
{
    ImDrawList* drawList = Nameplate::GetDrawList();

    // All corners fully transparent => nothing to draw
    if (((col_upr_left | col_upr_right | col_bot_right | col_bot_left) & IM_COL32_A_MASK) == 0)
        return;

    // If no rounding requested, use built-in multicolor rect and exit.
    if (rounding <= 0.0f)
    {
        drawList->AddRectFilledMultiColor(p_min, p_max, col_upr_left, col_upr_right, col_bot_right, col_bot_left);
        return;
    }

    // Build the rounded outline path.
    drawList->PathRect(p_min, p_max, rounding, flags);
    const int points_count = drawList->_Path.Size;
    if (points_count < 3)
    {
        drawList->PathClear();
        return;
    }

    // Corner colors as float4 for lerping
    const ImVec4 ul = ImGui::ColorConvertU32ToFloat4(col_upr_left);
    const ImVec4 ur = ImGui::ColorConvertU32ToFloat4(col_upr_right);
    const ImVec4 br = ImGui::ColorConvertU32ToFloat4(col_bot_right);
    const ImVec4 bl = ImGui::ColorConvertU32ToFloat4(col_bot_left);

    const float w     = (p_max.x - p_min.x);
    const float h     = (p_max.y - p_min.y);
    const float inv_w = (w != 0.0f) ? (1.0f / w) : 0.0f;
    const float inv_h = (h != 0.0f) ? (1.0f / h) : 0.0f;

    const ImVec2 uv = drawList->_Data->TexUvWhitePixel;

    // Reserve geometry:
    // - Inner fill uses (points_count - 2) triangles => (points_count - 2) * 3 indices
    // - AA fringe uses points_count quads => points_count * 6 indices
    // - For each outline point we emit 2 vertices (inner/outer)
    const float aa_size   = drawList->_FringeScale;
    const int   idx_count = (points_count - 2) * 3 + points_count * 6;
    const int   vtx_count = points_count * 2;
    drawList->PrimReserve(idx_count, vtx_count);

    const unsigned int vtx_inner_idx = drawList->_VtxCurrentIdx;
    const unsigned int vtx_outer_idx = drawList->_VtxCurrentIdx + 1;

    // 1) Fill interior (triangle fan using inner vertices)
    for (int i = 2; i < points_count; ++i)
    {
        drawList->_IdxWritePtr[0] = static_cast<ImDrawIdx>(vtx_inner_idx);
        drawList->_IdxWritePtr[1] = static_cast<ImDrawIdx>(vtx_inner_idx + (i - 1) * 2);
        drawList->_IdxWritePtr[2] = static_cast<ImDrawIdx>(vtx_inner_idx + i * 2);
        drawList->_IdxWritePtr += 3;
    }

    // 2) Compute normals per edge to create AA fringe.
    drawList->_Data->TempBuffer.reserve_discard(points_count);
    ImVec2* edge_normals = drawList->_Data->TempBuffer.Data;

    for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
    {
        const ImVec2& p0 = drawList->_Path[i0];
        const ImVec2& p1 = drawList->_Path[i1];

        float dx = p1.x - p0.x;
        float dy = p1.y - p0.y;
        Normalize2fOverZero(dx, dy);

        // Per-edge normal (perpendicular to direction)
        edge_normals[i0].x = dy;
        edge_normals[i0].y = -dx;
    }

    // 3) Emit vertices + AA fringe indices
    for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
    {
        const ImVec2& n0 = edge_normals[i0];
        const ImVec2& n1 = edge_normals[i1];

        // Average the adjacent edge normals to get a vertex normal.
        float nx = (n0.x + n1.x) * 0.5f;
        float ny = (n0.y + n1.y) * 0.5f;
        FixNormal2f(nx, ny);

        // Expand to inner/outer positions for AA fringe
        nx *= aa_size * 0.5f;
        ny *= aa_size * 0.5f;

        const ImVec2& p = drawList->_Path[i1];

        // Compute bilinear gradient coordinates
        const float u = (inv_w == 0.0f) ? 0.0f : (p.x - p_min.x) * inv_w;
        const float v = (inv_h == 0.0f) ? 0.0f : (p.y - p_min.y) * inv_h;

        const ImVec4 top       = ImLerp(ul, ur, u);
        const ImVec4 bot       = ImLerp(bl, br, u);
        const ImU32  col       = ImGui::ColorConvertFloat4ToU32(ImLerp(top, bot, v));
        const ImU32  col_trans = col & ~IM_COL32_A_MASK;

        // Write inner + outer vertices
        ImDrawVert& vtx_inner = drawList->_VtxWritePtr[0];
        ImDrawVert& vtx_outer = drawList->_VtxWritePtr[1];

        vtx_inner.pos = ImVec2(p.x - nx, p.y - ny);
        vtx_inner.uv  = uv;
        vtx_inner.col = col;

        vtx_outer.pos = ImVec2(p.x + nx, p.y + ny);
        vtx_outer.uv  = uv;
        vtx_outer.col = col_trans;

        drawList->_VtxWritePtr += 2;

        // AA fringe quad indices (inner i0/i1 to outer i0/i1)
        drawList->_IdxWritePtr[0] = static_cast<ImDrawIdx>(vtx_inner_idx + i1 * 2);
        drawList->_IdxWritePtr[1] = static_cast<ImDrawIdx>(vtx_inner_idx + i0 * 2);
        drawList->_IdxWritePtr[2] = static_cast<ImDrawIdx>(vtx_outer_idx + i0 * 2);

        drawList->_IdxWritePtr[3] = static_cast<ImDrawIdx>(vtx_outer_idx + i0 * 2);
        drawList->_IdxWritePtr[4] = static_cast<ImDrawIdx>(vtx_outer_idx + i1 * 2);
        drawList->_IdxWritePtr[5] = static_cast<ImDrawIdx>(vtx_inner_idx + i1 * 2);

        drawList->_IdxWritePtr += 6;
    }

    drawList->_VtxCurrentIdx += static_cast<ImDrawIdx>(vtx_count);
    drawList->PathClear();
}

} // namespace Ui
