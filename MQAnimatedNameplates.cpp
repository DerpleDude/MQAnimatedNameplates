// MQAnimatedNameplates.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup.

// By: Derple derple@ntsj.com

#include "Config.h"
#include "Ui.h"

#include "eqlib/graphics/CameraInterface.h"
#include "eqlib/graphics/Bones.h"
#include "eqlib/graphics/Actors.h"
#include "mq/Plugin.h"
#include "mq/imgui/ImGuiUtils.h"

#include "imgui/imgui.h"
#include "imgui/imanim/im_anim.h"
#include "imgui_internal.h"
#include "sol/sol.hpp"

PreSetup("MQAnimatedNameplates");
PLUGIN_VERSION(0.1);

iam_context* context = nullptr;

static bool GetNameplatePositionFromSpawnPosition(PlayerClient* pSpawn, ImVec2& outCoords)
{
    const CVector3 targetPos(pSpawn->Y, pSpawn->X, pSpawn->Z + pSpawn->Height);
    float outPosX, outPosY;

    if (pDisplay->pCamera->ProjectWorldCoordinatesToScreen(targetPos, outPosX, outPosY))
    {
        outCoords = ImVec2{ outPosX, outPosY };
        return true;
    }

    return false;
}

static bool GetNameplatePositionFromBones(PlayerClient* pSpawn, ImVec2& outCoords)
{
    if (CActorInterface* pActor = pSpawn->mActorClient.pActor)
    {
        if (CEQBone* bone = static_cast<CEQBone*>(pActor->GetBoneByIndex(eBoneHead)))
        {
            // CNameStringSprite::SetAttachBone

            std::string_view tag = bone->GetTag();
            float multiplier = 1.55f;

            if (mq::string_equals("HEAD_NAME", tag))
            {
                multiplier = 0.0f;
            }
            else if (tag.length() > 5 && tag[5] == 'H')
            {
                if (tag[0] == 'O' && tag[1] == 'G')
                {
                    multiplier = 2.2f;
                }
                else
                {
                    multiplier = 2.1f;
                }
            }

            float scale = std::max(pActor->GetScaleFactor(), 1.0f);
            if (scale > 1.0f)
            {
                scale = sqrtf(scale);
            }

            const CMatrix44& mtx = bone->m_attachmentMatrix;
            CVector3 pos = CVector3(mtx.row[3].X, mtx.row[3].Y, mtx.row[3].Z + (scale * multiplier));
            float outPosX, outPosY;

            if (pDisplay->pCamera->ProjectWorldCoordinatesToScreen(pos, outPosX, outPosY))
            {
                outCoords = ImVec2{ outPosX, outPosY };
                return true;
            }
        }
    }

    return false;
}

void DrawNameplates(PlayerClient* pSpawn, Ui::HPBarStyle style)
{
    if (!pSpawn)
        return;

    if (!LineOfSight(pControlledPlayer, pSpawn))
        return;

    ImVec2 targetNameplatePos;
    if (!GetNameplatePositionFromBones(pSpawn, targetNameplatePos))
        return;

    Ui::Config& config = Ui::Config::Get();

    ImVec2 targetNameplateTopLeft{FLT_MAX, FLT_MAX};
    ImVec2 targetNameplateBottomRight{0.0f, 0.0f};

    ImGui::PushFont(nullptr, config.FontSize);

    ImVec2 canvasSize(config.NameplateWidth, 50);
    ImVec2 baseHeadOffset{0, config.NameplateHeightOffset};

    ImVec2 padding = ImGui::GetStyle().FramePadding;

    // only render for target.
    if (config.ShowBuffIcons && pTarget == pSpawn)
    {
        int buffsPerRow = std::max(1, static_cast<int>(floorf(canvasSize.x / (config.IconSize + padding.x))));

        int buffCount = config.ShowBuffIcons ? GetCachedBuffCount(pSpawn) : 0;

        float numBuffRows = ceilf(buffCount / static_cast<float>(buffsPerRow));

        float verticalOffset = numBuffRows * (config.IconSize + padding.y);

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
                std::max(targetNameplateBottomRight.x, cursor.GetPos().x + config.IconSize);
            targetNameplateBottomRight.y =
                std::max(targetNameplateBottomRight.y, cursor.GetPos().y + config.IconSize);
        }
    }

    CursorState cursor{targetNameplatePos - canvasSize * 0.5 - baseHeadOffset};

    ImVec2 panelPos = cursor.GetPos();

    panelPos.x += padding.x;

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
    curPos.x = (startXPos + canvasSize.x) - (levelWidth + padding.x * 2);

    if (config.ShowLevel)
    {
        cursor.SetPos(curPos);
        Ui::RenderNamePlateText(cursor, textColor, targetLevel.c_str());
    }

    //
    // Class
    //

    if (config.ShowClass)
    {
        std::string classInfo;

        if (pSpawn->GetClass() < 1 || pSpawn->GetClass() > 16)
            classInfo = "???";
        else if (config.ShortClassName)
            classInfo = pEverQuest->GetClassThreeLetterCode(pSpawn->GetClass());
        else
            classInfo = GetClassDesc(pSpawn->GetClass());

        float classWidth = ImGui::CalcTextSize(classInfo.c_str()).x;

        // center this text
        curPos.x = (startXPos + canvasSize.x / 2) - (classWidth / 2 + padding.x * 2);
        cursor.SetPos(curPos);

        if (curPos.x <= startXPos + displayNameWidth + padding.x * 2)
            cursor.NewLine();

        Ui::RenderNamePlateText(cursor, textColor, classInfo.c_str());
    }

    //
    // Detail
    //

    std::string targetDetail;
    if (config.ShowPurpose && GetSpawnType(pSpawn) == NPC && pSpawn->Lastname[0])
    {
        targetDetail = fmt::format("({})", pSpawn->Lastname);
    }
    else if (config.ShowGuild && pGuild && pSpawn->GuildID > 0)
    {
        targetDetail = fmt::format("<{}>", pGuild->GetGuildName(pSpawn->GuildID));
    }

    if (!targetDetail.empty())
    {
        // center this text
        curPos           = cursor.GetPos();
        float guildWidth = ImGui::CalcTextSize(targetDetail.c_str()).x;
        curPos.x = (startXPos + canvasSize.x / 2) - (guildWidth / 2 + padding.x * 2);

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
    ImVec2      barSize{canvasSize.x - padding.x * 2, ImGui::GetTextLineHeight() * 0.75f};
    Ui::RenderFancyHPBar(cursor, hpBarID, pctHP, barSize, conColor, pTarget == pSpawn, "", style);
    ImGui::PopFont();

    targetNameplateBottomRight.x =
        std::max(targetNameplateBottomRight.x, cursor.GetPos().x + canvasSize.x - padding.x * 2);
    targetNameplateBottomRight.y =
        std::max(targetNameplateBottomRight.y, cursor.GetPos().y + ImGui::GetTextLineHeight());

    cursor.SetPos(targetNameplateTopLeft);
    if (config.ShowDebugPanel)
    {
        Ui::RenderNamePlateRect(cursor, targetNameplateBottomRight - targetNameplateTopLeft,
            IM_COL32(40, 240, 40, 55), 3.0f, 1.0f, true);
    }

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

        Ui::Config& config = Ui::Config::Get();

        if (config.RenderForTarget)
            DrawNameplates(pTarget, config.HPBarStyleTarget.get());
        if (config.RenderForSelf)
            DrawNameplates(pLocalPlayer, config.HPBarStyleSelf.get());
        if (config.RenderForGroup && pLocalPC->pGroupInfo)
        {
            for (int i = 0; i < MAX_GROUP_SIZE; i++)
            {
                CGroupMember* pGroupMember = pLocalPC->pGroupInfo->GetGroupMember(i);
                if (pGroupMember && pGroupMember->GetPlayer()
                    && pGroupMember->GetPlayer()->SpawnID != pLocalPlayer->SpawnID)
                {
                    DrawNameplates(pGroupMember->GetPlayer(), config.HPBarStyleGroup.get());
                }
            }
        }

        if (config.RenderForNPCs)
        {
            PlayerClient* pSpawn = pSpawnManager->FirstSpawn;
            while (pSpawn)
            {
                if (GetSpawnType(pSpawn) == NPC)
                {
                    if (!config.RenderForTarget || !pTarget || pSpawn->SpawnID != pTarget->SpawnID)
                    {
                        DrawNameplates(pSpawn, config.HPBarStyleNPCs.get());
                    }
                }
                
                pSpawn = pSpawn->GetNext();
            }
        }
        else if (config.RenderForAllHaters)
        {
            if (pLocalPC)
            {
                ExtendedTargetList* xtm = pLocalPC->pExtendedTargetList;

                for (int i = 0; i < xtm->GetNumSlots(); i++)
                {
                    ExtendedTargetSlot* xts = xtm->GetSlot(i);

                    if (xts->SpawnID && xts->xTargetType == XTARGET_AUTO_HATER)
                    {
                        if (!config.RenderForTarget || !pTarget || xts->SpawnID != pTarget->SpawnID)
                        {
                            if (PlayerClient* pSpawn = GetSpawnByID(xts->SpawnID))
                            {
                                DrawNameplates(pSpawn, config.HPBarStyleHaters.get());
                            }
                        }
                    }
                }
            }
        }
    }
}

PLUGIN_API void OnPulse()
{
    Ui::Config::Get().SaveSettings();
}

PLUGIN_API void OnAddSpawn(PlayerClient* pSpawn)
{
}

PLUGIN_API void OnRemoveSpawn(PlayerClient* pSpawn)
{
}

sol::object DoCreateModule(sol::this_state s)
{
    sol::state_view L(s);

    sol::table module = L.create_table();

    module["ProjectWorldCoordinatesToScreen"] =
        [](sol::this_state L, const float x, const float y, const float z) -> ImVec2
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