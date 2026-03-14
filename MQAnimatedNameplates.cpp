// MQAnimatedNameplates.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup.

// By: Derple derple@ntsj.com

#include "Ui.h"

#include "eqlib/graphics/CameraInterface.h"
#include "mq/Plugin.h"
#include "mq/imgui/ImGuiUtils.h"

#include "imgui/imgui.h"
#include "imgui/imanim/im_anim.h"
#include "imgui_internal.h"
#include "sol/sol.hpp"

PreSetup("MQAnimatedNameplates");
PLUGIN_VERSION(0.1);

iam_context* context = nullptr;

void DrawNameplates(PlayerClient* pSpawn, Ui::HPBarStyle style)
{
    if (!pSpawn)
        return;

    if (!LineOfSight(pControlledPlayer, pSpawn))
        return;

    const CVector3 targetPos(pSpawn->Y, pSpawn->X, pSpawn->Z + pSpawn->Height);
    float          targetNameplatePosX, targetNameplatePosY;

    pDisplay->pCamera->ProjectWorldCoordinatesToScreen(targetPos, targetNameplatePosX, targetNameplatePosY);
    if (targetNameplatePosY <= 75)
        targetNameplatePosY = 75; // move off-screen if above a certain height so it doesn't draw in the middle of the
                                  // screen when looking up at something.
    const ImVec2 targetNameplatePos{targetNameplatePosX, targetNameplatePosY};

    ImVec2 targetNameplateTopLeft{FLT_MAX, FLT_MAX};
    ImVec2 targetNameplateBottomRight{0.0f, 0.0f};

    ImGui::PushFont(nullptr, Ui::Config::Get().FontSize);

    ImVec2 canvasSize(Ui::Config::Get().NameplateWidth, 50);
    ImVec2 baseHeadOffset{0, Ui::Config::Get().NameplateHeightOffset};

    // only render for target.
    if (Ui::Config::Get().ShowBuffIcons && pTarget == pSpawn)
    {
        int buffsPerRow =
            static_cast<int>(floorf(canvasSize.x / (Ui::Config::Get().IconSize + Ui::Config::Get().PaddingX)));

        int buffCount = Ui::Config::Get().ShowBuffIcons ? GetCachedBuffCount(pSpawn) : 0;

        float numBuffRows = ceilf(buffCount / static_cast<float>(buffsPerRow));

        float verticalOffset = numBuffRows * (Ui::Config::Get().IconSize + Ui::Config::Get().PaddingY);

        ImVec2 assumedHeadOffset(0, baseHeadOffset.y + verticalOffset);
        ImVec2 curPos = targetNameplatePos - canvasSize * 0.5f - assumedHeadOffset;

        // this draws above the nameplate so we can use a seperate cursor for it since it will not change the cursor for
        // the plate.
        CursorState cursor{curPos};
        int         iconsDrawn = 0;
        for (int i = 0; i < buffCount; i++)
        {
            auto buff = GetCachedBuffAtSlot(pSpawn, i);

            if (buff.has_value())
            {
                if (EQ_Spell* spell = GetSpellByID(buff->spellId))
                {
                    Ui::DrawInspectableSpellIcon(cursor, spell);

                    if (iconsDrawn == 0 || ((iconsDrawn + 1) < buffCount) && ((iconsDrawn + 1) % buffsPerRow) != 0)
                        cursor.SameLine();

                    iconsDrawn += 1;
                }
            }

            targetNameplateTopLeft.x = std::min(targetNameplateTopLeft.x, cursor.GetPos().x);
            targetNameplateTopLeft.y = std::min(targetNameplateTopLeft.y, cursor.GetPos().y);
            targetNameplateBottomRight.x =
                std::max(targetNameplateBottomRight.x, cursor.GetPos().x + Ui::Config::Get().IconSize);
            targetNameplateBottomRight.y =
                std::max(targetNameplateBottomRight.y, cursor.GetPos().y + Ui::Config::Get().IconSize);
        }
    }

    CursorState cursor{targetNameplatePos - canvasSize * 0.5 - baseHeadOffset};

    ImVec2 panelPos = cursor.GetPos();

    panelPos.x += Ui::Config::Get().PaddingX;

    cursor.SetPos(panelPos);

    targetNameplateTopLeft.x = std::min(targetNameplateTopLeft.x, cursor.GetPos().x);
    targetNameplateTopLeft.y = std::min(targetNameplateTopLeft.y, cursor.GetPos().y);

    //
    // Name Text
    //

    ImU32 textColor = IM_COL32(255, 255, 255, 255);
    ImU32 conColor  = GetColorForChatColor(ConColor(pSpawn)).ToImU32();

    ImVec2 curPos    = cursor.GetPos();
    float  startXPos = curPos.x;

    const char* displayName      = pSpawn->DisplayedName;
    float       displayNameWidth = ImGui::CalcTextSize(displayName).x;
    Ui::RenderNamePlateText(cursor, textColor, displayName);

    //
    // Level
    //

    std::string targetLevel = fmt::format("{}", pSpawn->GetLevel());

    // right justify this text
    float levelWidth = ImGui::CalcTextSize(targetLevel.c_str()).x;
    curPos.x         = (startXPos + canvasSize.x) - (levelWidth + Ui::Config::Get().PaddingX * 2);

    if (Ui::Config::Get().ShowLevel)
    {
        cursor.SetPos(curPos);
        Ui::RenderNamePlateText(cursor, textColor, targetLevel.c_str());
    }

    //
    // Class
    //
    std::string overRideClassName;
    if (pSpawn->GetClass() < 1 || pSpawn->GetClass() > 16)
        overRideClassName = "???";

    std::string classInfo      = Ui::Config::Get().ShowClass
                                     ? fmt::format("{}", Ui::Config::Get().ShortClassName
                                                             ? overRideClassName.length() > 0
                                                                   ? overRideClassName
                                                                   : pEverQuest->GetClassThreeLetterCode(pSpawn->GetClass())
                                                             : GetClassDesc(pSpawn->GetClass()))
                                     : "";
    float       classInfoWidth = ImGui::CalcTextSize(classInfo.c_str()).x;

    // center this text
    float classWidth = ImGui::CalcTextSize(classInfo.c_str()).x;
    curPos.x         = (startXPos + canvasSize.x / 2) - (classWidth / 2 + Ui::Config::Get().PaddingX * 2);
    cursor.SetPos(curPos);

    if (curPos.x <= startXPos + displayNameWidth + Ui::Config::Get().PaddingX * 2)
        cursor.NewLine();

    Ui::RenderNamePlateText(cursor, textColor, classInfo.c_str());

    //
    // Detail
    //

    std::string targetDetail;
    if (Ui::Config::Get().ShowPurpose && GetSpawnType(pSpawn) == NPC && pSpawn->Lastname[0])
    {
        targetDetail = fmt::format("({})", pSpawn->Lastname);
    }
    else if (Ui::Config::Get().ShowGuild && pGuild && pSpawn->GuildID > 0)
    {
        targetDetail = fmt::format("<{}>", pGuild->GetGuildName(pSpawn->GuildID));
    }

    if (!targetDetail.empty())
    {
        // center this text
        curPos           = cursor.GetPos();
        float guildWidth = ImGui::CalcTextSize(targetDetail.c_str()).x;
        curPos.x         = (startXPos + canvasSize.x / 2) - (guildWidth / 2 + Ui::Config::Get().PaddingX * 2);

        cursor.SetPos(curPos);

        Ui::RenderNamePlateText(cursor, textColor, targetDetail.c_str());
    }

    //
    // % HP
    //

    float       pctHP        = pSpawn->HPMax == 0 ? 0 : pSpawn->HPCurrent * 100.0f / pSpawn->HPMax;
    std::string targetPctHPs = fmt::format("{:.0f}%", pctHP);

    // Draw the rest

    cursor.SetPos(ImVec2(startXPos, cursor.GetPos().y));

    std::string hpBarID = fmt::format("TargetHPBar_{}", pSpawn->SpawnID);
    ImVec2      barSize{canvasSize.x - Ui::Config::Get().PaddingX * 2, ImGui::GetTextLineHeight() * 0.75f};
    Ui::RenderFancyHPBar(cursor, hpBarID, pctHP, barSize, conColor, pTarget == pSpawn, "", style);
    ImGui::PopFont();

    targetNameplateBottomRight.x =
        std::max(targetNameplateBottomRight.x, cursor.GetPos().x + canvasSize.x - Ui::Config::Get().PaddingX * 2);
    targetNameplateBottomRight.y =
        std::max(targetNameplateBottomRight.y, cursor.GetPos().y + ImGui::GetTextLineHeight());

    cursor.SetPos(targetNameplateTopLeft);
    if (Ui::Config::Get().ShowDebugPanel)
        Ui::RenderNamePlateRect(cursor, targetNameplateBottomRight - targetNameplateTopLeft, IM_COL32(40, 240, 40, 55),
                                3.0f, 1.0f, true);

    ImVec2 mouse   = ImGui::GetIO().MousePos;
    bool   hovered = mouse.x >= targetNameplateTopLeft.x && mouse.x <= targetNameplateBottomRight.x &&
                   mouse.y >= targetNameplateTopLeft.y && mouse.y <= targetNameplateBottomRight.y;

    bool clicked = hovered && ImGui::IsMouseClicked(0);

    if (clicked)
    {
        pTarget = pSpawn;
    }
}

PLUGIN_API void InitializePlugin()
{
    context = iam_context_create();
    AddSettingsPanel("plugins/Nameplates", Ui::RenderSettingsPanel);
}

PLUGIN_API void ShutdownPlugin()
{
    iam_context_destroy(context);
    RemoveSettingsPanel("plugins/Nameplates");
}

PLUGIN_API void OnUpdateImGui()
{
    iam_context_set_current(context);

    if (GetGameState() == GAMESTATE_INGAME)
    {
        if (!pDisplay)
            return;

        if (Ui::Config::Get().RenderForTarget)
            DrawNameplates(pTarget, static_cast<Ui::HPBarStyle>(Ui::Config::Get().HPBarStyleTarget.get()));
        if (Ui::Config::Get().RenderForSelf)
            DrawNameplates(pLocalPlayer, static_cast<Ui::HPBarStyle>(Ui::Config::Get().HPBarStyleSelf.get()));
        if (Ui::Config::Get().RenderForGroup && pLocalPC->pGroupInfo)
        {
            for (int i = 0; i < MAX_GROUP_SIZE; i++)
            {
                CGroupMember* pGroupMember = pLocalPC->pGroupInfo->GetGroupMember(i);
                if (pGroupMember && pGroupMember->GetPlayer() &&
                    pGroupMember->GetPlayer()->SpawnID != pLocalPlayer->SpawnID)
                    DrawNameplates(pGroupMember->GetPlayer(),
                                   static_cast<Ui::HPBarStyle>(Ui::Config::Get().HPBarStyleGroup.get()));
            }
        }
        if (Ui::Config::Get().RenderForAllHaters)
        {
            if (pLocalPC)
            {
                ExtendedTargetList* xtm = pLocalPC->pExtendedTargetList;

                for (int i = 0; i < xtm->GetNumSlots(); i++)
                {
                    ExtendedTargetSlot* xts = xtm->GetSlot(i);

                    if (xts->SpawnID && xts->xTargetType == XTARGET_AUTO_HATER)
                    {
                        if (!Ui::Config::Get().RenderForTarget || !pTarget || xts->SpawnID != pTarget->SpawnID)
                        {
                            if (PlayerClient* pSpawn = GetSpawnByID(xts->SpawnID))
                            {
                                DrawNameplates(pSpawn,
                                               static_cast<Ui::HPBarStyle>(Ui::Config::Get().HPBarStyleHaters.get()));
                            }
                        }
                    }
                }
            }
        }
    }
}

sol::object DoCreateModule(sol::this_state s)
{
    sol::state_view L(s);

    sol::table module = L.create_table();

    module["ProjectWorldCoordinatesToScreen"] = [](sol::this_state L, const float x, const float y,
                                                   const float z) -> ImVec2
    {
        if (!pDisplay || !pDisplay->pCamera)
            return ImVec2(0, 0);

        float outX, outY;

        pDisplay->pCamera->ProjectWorldCoordinatesToScreen(CVector3(y, x, z), outX, outY);

        return ImVec2(outX, outY);
    };

    return sol::make_object(L, module);
}

PLUGIN_API bool CreateLuaModule(sol::this_state s, sol::object& object)
{
    object = DoCreateModule(s);
    return true;
}