// MQAnimatedNameplates.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup.

// By: Derple derple@ntsj.com

#include "Config.h"
#include "Nameplate.h"
#include "SettingsPanel.h"

#include "eqlib/graphics/CameraInterface.h"
#include "eqlib/graphics/Bones.h"
#include "eqlib/graphics/Actors.h"
#include "mq/Plugin.h"
#include "mq/imgui/ImGuiUtils.h"

#include "imgui/imgui.h"
#include "imgui/imanim/im_anim.h"
#include "imgui_internal.h"
#include "sol/sol.hpp"

#include <ranges>
#include <unordered_map>

PreSetup("MQAnimatedNameplates");
PLUGIN_VERSION(0.1);

iam_context* context = nullptr;

static std::unordered_map<uint32_t, Ui::Nameplate> s_nameplatesBySpawnId;
static std::vector<Ui::Nameplate*> s_nameplatesToRenderByDistance;
constexpr std::chrono::milliseconds SPAWN_SORT_INTERVAL{ 100 };
static std::chrono::steady_clock::time_point s_lastRenderTime{};

static std::chrono::steady_clock::time_point s_lastExpirationCheck{};

static constexpr std::chrono::seconds expiration_time{ 30 };
static constexpr std::chrono::seconds expiration_check_time{ 1 };

static bool GetPositionFromHeadBone(PlayerClient* pSpawn, glm::vec3& outPosition)
{
    if (CActorInterface* pActor = pSpawn->mActorClient.pActor)
    {
        if (pActor->GetBoneByIndex(eBoneHead) == nullptr)
            return false;

        pActor->GetBoneWorldPosition(eBoneHead, (CVector3*)&outPosition, false);
        return true;
    }

    return false;
}

static bool CanSeeNameplate(const CVector3& targetPosition)
{
    Ui::Config& config = Ui::Config::Get();

    if (config.RenderNoLOS)
        return true;


    CCamera* camera = static_cast<eqlib::CCamera*>(pDisplay->pCamera);
    if (!camera)
        return false;

    EQZoneInfo* zoneInfo = pWorldData->GetZone(pEverQuestInfo->ZoneID);
    if (!zoneInfo)
        return false;

    // Perform distance check
    float xdiff = targetPosition.X - camera->position.x;
    float ydiff = targetPosition.Y - camera->position.y;

    float squareDist = (xdiff * xdiff) + (ydiff * ydiff);
    if (squareDist > config.MaxDrawDistance * config.MaxDrawDistance)
        return false;

    uint64_t flags = zoneInfo->ZoneFlags;
    zoneInfo->ZoneFlags &= ~0x00400000; // Enable LOS checks

    // Perform line-of-sight test from camera to bone position. Its a line segment test,
    // but a larger volume test might be better to prevent nameplates from popping in and
    // out when the player is near cover.
    glm::vec3 startPosition(camera->position.x, camera->position.y, camera->position.z);
    int result = CastRayLoc(*reinterpret_cast<const CVector3*>(&startPosition),
        EQRace(0), targetPosition.X, targetPosition.Y, targetPosition.Z);

    if (result == 0)
    {
        // Maybe the player can see it?
        if (GetPositionFromHeadBone(pControlledPlayer, startPosition))
        {
            result = CastRayLoc(*reinterpret_cast<const CVector3*>(&startPosition),
                EQRace(0), targetPosition.X, targetPosition.Y, targetPosition.Z);
        }
    }

    zoneInfo->ZoneFlags = flags; // Restore original flags
    return result == 1;
}

static bool CanSeeNameplate(PlayerClient* pSpawn)
{
    Ui::Config& config = Ui::Config::Get();

    if (config.RenderNoLOS)
        return true;

    if (config.RenderTargetNoLOS && pSpawn == pTarget)
        return true;

    const CVector3 targetPos(pSpawn->Y, pSpawn->X, pSpawn->Z + pSpawn->Height);

    return CanSeeNameplate(targetPos);
}

static bool ShouldRenderNameplate(Ui::Nameplate* pNameplate)
{
    Ui::Config& config = Ui::Config::Get();

    switch (pNameplate->GetNameplateType())
    {
    case Ui::NameplateType_Self:
        return config.RenderForSelf;
    case Ui::NameplateType_Group:
        return config.RenderForGroup;
    case Ui::NameplateType_Target:
        return config.RenderForTarget;
    case Ui::NameplateType_AutoHater:
        return config.RenderForAllHaters;
    case Ui::NameplateType_NPC:
        return config.RenderForNPCs;
    default:
        return false;
    }
}

static float GetBoneScaleOffset(CBoneInterface* bone)
{
    std::string_view tag = bone->GetTag();

    // Calculate scale offset
    float scaleOffset = 1.55f;
    if (mq::string_equals("HEAD_NAME", tag))
    {
        scaleOffset = 0.0f;
    }
    else if (tag.length() > 5 && tag[5] == 'H')
    {
        if (tag[0] == 'O' && tag[1] == 'G')
        {
            scaleOffset = 2.2f;
        }
        else
        {
            scaleOffset = 2.1f;
        }
    }

    return scaleOffset;
}

static float GetActorScaleFactor(CActorInterface* pActor)
{
    float scaleFactor = std::max(pActor->GetScaleFactor(), 1.0f);
    if (scaleFactor > 1.0f)
    {
        scaleFactor = sqrtf(scaleFactor);
    }

    return scaleFactor;
}

static bool GetNameplatePositionFromBones(PlayerClient* pSpawn, ImVec2& outCoords, float& outNameplateScale)
{
    CCamera* camera = static_cast<eqlib::CCamera*>(pDisplay->pCamera);
    if (!camera)
        return false;

    glm::vec3 bonePos;
    if (!GetPositionFromHeadBone(pSpawn, bonePos))
        return false;

    Ui::Config& config = Ui::Config::Get();

    CActorInterface* pActor = pSpawn->mActorClient.pActor;
    float scaleOffset = GetBoneScaleOffset(pActor->GetBoneByIndex(eBoneHead));
    float scaleFactor = GetActorScaleFactor(pActor) * config.ScaleFactorAdjustment;

    glm::vec3 eyeOffset;
    pGraphicsEngine->pRender->GetEyeOffset(*reinterpret_cast<CVector3*>(&eyeOffset));
    glm::vec3 worldPos = bonePos + eyeOffset;

    float Ez = glm::dot(worldPos, glm::vec3(camera->worldToEyeCoef[0][2], camera->worldToEyeCoef[1][2], camera->worldToEyeCoef[2][2]));
    if (Ez < 0.0f)     // Early out if out behind the camera
        return false;

    float additionalYOffset = scaleOffset * scaleFactor;

    float Ex = glm::dot(worldPos, camera->worldToEyeXAxisCot);
    float Ey = glm::dot(worldPos, camera->worldToEyeYAxisCotAspect);

    // Distance scale: value between 1.0 and 2.0 depending on how far away, up to 150 units
    float distance = glm::distance(glm::vec2(bonePos), glm::vec2(camera->position));
    float distanceScale = 1.0f + std::min(1.0f, (distance / (150.0f * scaleFactor)));
    float nameplateScaleCoeff = config.NameplateHeightScaleCoeff;
    outNameplateScale = scaleFactor * distanceScale * nameplateScaleCoeff;

    // FIXME: Use actual height of nameplate instead of config
    float scaledHeight = camera->cotAspectRatio * (additionalYOffset + config.NameplateHeightAdjust * scaleFactor * distanceScale * nameplateScaleCoeff);
    Ey += scaledHeight * 0.5f;

    float xoffset = camera->halfRenderWidth + camera->left;
    float yoffset = camera->halfRenderHeight + camera->top;
    float reci = 1.0f / Ez;

    float Wx1 = Ex * reci * camera->halfRenderWidth + xoffset;
    float Wy1 = -Ey * reci * camera->halfRenderHeight + yoffset;

    outCoords = ImVec2{ Wx1, Wy1 };
    return true;
}

static bool DrawNameplate(Ui::Nameplate* pNameplate, bool alwaysVisible = false)
{
    if (!pDisplay)
        return false;

    if (!alwaysVisible && !CanSeeNameplate(pNameplate->GetSpawn()))
        return false;

    if (!ShouldRenderNameplate(pNameplate))
        return false;

    auto* pNameplateConfig = pNameplate->GetConfig();
    const ImVec2 canvasSize(pNameplateConfig->NameplateWidth, pNameplateConfig->NameplateHeight);

    float nameplateScale = 1.0f;
    ImVec2 targetNameplatePos;

    if (!GetNameplatePositionFromBones(pNameplate->GetSpawn(), targetNameplatePos, nameplateScale))
        return false;

    pNameplate->Render(targetNameplatePos, canvasSize, nameplateScale);

    return true;
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
    if (GetGameState() == GAMESTATE_INGAME)
    {
        iam_context_set_current(context);

        bool lazy_init_enabled = iam_is_lazy_init_enabled();
        iam_set_lazy_init(false);

        auto now = std::chrono::steady_clock::now();
        if (now - s_lastExpirationCheck > expiration_check_time)
        {
            s_lastExpirationCheck = now;

            // cleanup stale nameplates
            for (auto it = s_nameplatesBySpawnId.begin(); it != s_nameplatesBySpawnId.end();)
            {
                if (now - it->second.GetLastRenderTime() > expiration_time
                    || it->second.GetNameplateType() == Ui::NameplateType_Target && !it->second.IsCurrentTarget()
                    || it->second.GetNameplateType() == Ui::NameplateType_Group && !it->second.IsInGroup()
                    || it->second.GetNameplateType() == Ui::NameplateType_AutoHater && !it->second.IsAutoHater()
                    )
                {
                    std::erase(s_nameplatesToRenderByDistance, &it->second);
                    it = s_nameplatesBySpawnId.erase(it);
                }
                else
                    ++it;
            }
        }

        Ui::Config& config = Ui::Config::Get();

        // reverse order so that nameplates get the highest prioirity ID
        if (config.RenderForNPCs || config.RenderForPCs)
        {
            PlayerClient* pSpawn = pSpawnManager->FirstSpawn;
            while (pSpawn)
            {
                if ((config.RenderForNPCs && GetSpawnType(pSpawn) == NPC) 
                    || (config.RenderForPCs && GetSpawnType(pSpawn) == PC))
                {
                    auto [it, inserted] = s_nameplatesBySpawnId.try_emplace(pSpawn->SpawnID, pSpawn);
                    if (inserted)
                    {
                        s_nameplatesToRenderByDistance.push_back(&it->second);
                    }

                    it->second.SetNameplateType(Ui::NameplateType::NameplateType_NPC);
                }

                pSpawn = pSpawn->GetNext();
            }
        }

        if (config.RenderForAllHaters)
        {
            if (pLocalPC)
            {
                ExtendedTargetList* xtm = pLocalPC->pExtendedTargetList;

                for (int i = 0; i < xtm->GetNumSlots(); i++)
                {
                    ExtendedTargetSlot* xts = xtm->GetSlot(i);

                    if (xts->SpawnID && xts->xTargetType == XTARGET_AUTO_HATER)
                    {
                        if (PlayerClient* pSpawn = GetSpawnByID(xts->SpawnID))
                        {
                            auto [it, inserted] = s_nameplatesBySpawnId.try_emplace(pSpawn->SpawnID, pSpawn);
                            if (inserted)
                            {
                                s_nameplatesToRenderByDistance.push_back(&it->second);
                            }

                            it->second.SetNameplateType(Ui::NameplateType::NameplateType_AutoHater);
                        }
                    }
                }
            }
        }

        if (config.RenderForGroup && pLocalPC->pGroupInfo)
        {
            for (int i = 0; i < MAX_GROUP_SIZE; i++)
            {
                if (CGroupMember* pGroupMember = pLocalPC->pGroupInfo->GetGroupMember(i); pGroupMember && pGroupMember->GetPlayer())
                {
                    auto [it, inserted] = s_nameplatesBySpawnId.try_emplace(pGroupMember->GetPlayer()->SpawnID, pGroupMember->GetPlayer());
                    if (inserted)
                    {
                        s_nameplatesToRenderByDistance.push_back(&it->second);
                    }

                    it->second.SetNameplateType(Ui::NameplateType_Group);
                }
            }
        }

        if (config.RenderForTarget && pTarget)
        {
            auto [it, inserted] = s_nameplatesBySpawnId.try_emplace(pTarget->SpawnID, pTarget);
            if (inserted)
            {
                s_nameplatesToRenderByDistance.push_back(&it->second);
            }

            it->second.SetNameplateType(Ui::NameplateType_Target);
        }

        if (config.RenderForSelf)
        {
            auto [it, inserted] = s_nameplatesBySpawnId.try_emplace(pLocalPlayer->SpawnID, pLocalPlayer);
            if (inserted)
            {
                s_nameplatesToRenderByDistance.push_back(&it->second);
            }
            
            it->second.SetNameplateType(Ui::NameplateType_Self);
        }

        if (std::chrono::steady_clock::now() > s_lastRenderTime + SPAWN_SORT_INTERVAL)
        {
            // sort by furthest away
            std::ranges::sort(s_nameplatesToRenderByDistance.begin(), s_nameplatesToRenderByDistance.end(),
                [](Ui::Nameplate* a, Ui::Nameplate* b)
            {
                if (a->IsCurrentTarget())
                    return false;
                if (b->IsCurrentTarget())
                    return true;
                return a->GetDistplaceToPlayer() > b->GetDistplaceToPlayer();
            });
        }

        for (Ui::Nameplate* pNameplate : s_nameplatesToRenderByDistance)
        {
            if (!DrawNameplate(pNameplate))
                pNameplate->ResetRenderNameSpriteState();
        }

        iam_set_lazy_init(lazy_init_enabled);
    }
    else if (!s_nameplatesBySpawnId.empty())
    {
        s_nameplatesBySpawnId.clear();
        s_nameplatesToRenderByDistance.clear();
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
    auto it = s_nameplatesBySpawnId.find(pSpawn->SpawnID);
    if (it != s_nameplatesBySpawnId.end())
    {
        std::erase(s_nameplatesToRenderByDistance, &it->second);
        s_nameplatesBySpawnId.erase(it);
    }
}
