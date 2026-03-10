// MQAnimatedNameplates.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup.

// By: Derple derple@ntsj.com

#include "Ui.h"

#include "eqlib/graphics/CameraInterface.h"
#include "imgui/imgui.h"
#include "mq/imgui/ImGuiUtils.h"
#include "mq/Plugin.h"

#include "sol/sol.hpp"

PreSetup("MQAnimatedNameplates");
PLUGIN_VERSION(0.1);

void DrawNameplates(PSPAWNINFO pSpawn)
{
	if (!pSpawn)
		return;

	const CVector3 targetPos(pSpawn->Y, pSpawn->X, pSpawn->Z + pSpawn->Height);
	float targetNameplatePosX, targetNameplatePosY;

	pDisplay->pCamera->ProjectWorldCoordinatesToScreen(targetPos, targetNameplatePosX, targetNameplatePosY);
	bool open = true;

	ImGui::PushFont(nullptr, Ui::Settings.GetFontSize());

	const char* targetName = pSpawn->DisplayedName;
	float pctHP = pSpawn->HPMax == 0 ? 0 : pSpawn->HPCurrent * 100.0f / pSpawn->HPMax;
	std::string targetPctHPs = std::format("{:.0f}%", pctHP);
	int guildId = pSpawn->GuildID;
	std::string targetGuild;
	
	if (Ui::Settings.GetShowGuild() && pGuild && guildId > 0)
	{
		targetGuild = std::format("<{}>", pGuild->GetGuildName(guildId));
	}

	if (Ui::Settings.GetShowPurpose() && GetSpawnType(pSpawn) == NPC && strlen(pSpawn->Lastname) > 0)
	{
		targetGuild = std::format("({})", pSpawn->Lastname);
	}

	std::string classInfo = std::format("{} {}", pSpawn->GetLevel(), GetClassDesc(pSpawn->GetClass()));

	ImVec2 canvasSize(Ui::Settings.GetNameplateWidth(), 50);
	float baseHeadOffset = 35.0f;

	// only render for target.
	if (Ui::Settings.GetShowBuffIcons() && pTarget == pSpawn)
	{
		int buffsPerRow = static_cast<int>(floor(canvasSize.x / (Ui::Settings.GetIconSize() + Ui::Settings.GetPadding().x)));

		int buffCount = Ui::Settings.GetShowBuffIcons() ? GetCachedBuffCount(pSpawn) : 0;

		double numBuffRows = ceil(buffCount / static_cast<float>(buffsPerRow));

		float verticalOffset = static_cast<float>(numBuffRows) * (Ui::Settings.GetIconSize() + Ui::Settings.GetPadding().y);

		ImVec2 assumedHeadOffset(
			0,
			baseHeadOffset + verticalOffset
		);

		ImVec2 curPos(
			targetNameplatePosX - canvasSize.x * 0.5f - assumedHeadOffset.x,
			targetNameplatePosY - canvasSize.y * 0.5f - assumedHeadOffset.y
		);

		// this draws above the nameplate so we can use a seperate cursor for it since it will not change the cursor for the plate.
		CursorState cursor{ curPos };
		for (int i = 0; i < buffCount; i++)
		{
			auto buff = GetCachedBuffAtSlot(pSpawn, i);

			if (buff.has_value())
			{
				if (EQ_Spell* spell = GetSpellByID(buff->spellId))
				{
					Ui::DrawInspectableSpellIcon(cursor, spell);

					if (i == 0 || ((i+1) < buffCount) && ((i+1) % buffsPerRow) != 0)
						cursor.SameLine();
				}
			}
		}
	}

	CursorState cursor{
		ImVec2(
			targetNameplatePosX - canvasSize.x * 0.5f,
			targetNameplatePosY - canvasSize.y * 0.5f - baseHeadOffset
		) 
	};

	ImVec2 panelPos = cursor.GetPos();

	if (Ui::Settings.GetShowDebugPanel())
		Ui::RenderNamePlateRect(cursor, canvasSize, IM_COL32(153, 153, 153, 204), 2, 0, true);

	panelPos.x += Ui::Settings.GetPadding().x;

	cursor.SetPos(panelPos);

	ImU32 textColor = IM_COL32(255, 255, 255, 255);

	ImU32 conColor = ConColorToARGB(ConColor(pSpawn));

	ImVec2 curPos(cursor.GetPos());
	float startXPos = curPos.x;

	Ui::RenderNamePlateText(cursor, textColor, targetName);

	// right justify this text
	float hpWidth = ImGui::CalcTextSize(targetPctHPs.c_str()).x;
	curPos.x = (startXPos + canvasSize.x) -
		(hpWidth + Ui::Settings.GetPadding().x * 2);

	cursor.SetPos(curPos);
	Ui::RenderNamePlateText(cursor, textColor, targetPctHPs.c_str());

	// center this text
	float classWidth = ImGui::CalcTextSize(classInfo.c_str()).x;
	curPos.x = (startXPos + canvasSize.x / 2) -
		(classWidth / 2 + Ui::Settings.GetPadding().x * 2);

	cursor.SetPos(curPos);
	float LineLength = ImGui::CalcTextSize(targetName).x + ImGui::CalcTextSize(targetPctHPs.c_str()).x + ImGui::CalcTextSize(classInfo.c_str()).x + Ui::Settings.GetPadding().x * 4;

	if (LineLength >= Ui::Settings.GetNameplateWidth() * 0.9f)
		cursor.NewLine();

	Ui::RenderNamePlateText(cursor, textColor, classInfo.c_str());

	if (!targetGuild.empty())
	{
		// center this text
		curPos = cursor.GetPos();
		float guildWidth = ImGui::CalcTextSize(targetGuild.c_str()).x;
		curPos.x = (startXPos + canvasSize.x / 2) -
			(guildWidth / 2 + Ui::Settings.GetPadding().x * 2);
		
		cursor.SetPos(curPos);

		Ui::RenderNamePlateText(cursor, textColor, targetGuild.c_str());
	}

	cursor.SetPos(ImVec2(startXPos, cursor.GetPos().y));

	std::string hpBarID =
		std::string("TargetHPBar_") +
		std::to_string(pSpawn->GetId());

	Ui::RenderFancyHPBar(
		cursor,
		hpBarID,
		pctHP,
		ImGui::GetTextLineHeight() * 0.75f,
		canvasSize.x - Ui::Settings.GetPadding().x * 2,
		conColor,
		""
	);
	ImGui::PopFont();
}

PLUGIN_API void InitializePlugin()
{
	AddSettingsPanel("plugins/Nameplates", Ui::RenderSettingsPanel);
}

PLUGIN_API void ShutdownPlugin()
{
	RemoveSettingsPanel("plugins/Nameplates");
}

PLUGIN_API void OnUpdateImGui()
{
	if (GetGameState() == GAMESTATE_INGAME)
	{
		if (!pDisplay)
			return;

		if (Ui::Settings.GetRenderForTarget())
			DrawNameplates(pTarget);
		if (Ui::Settings.GetRenderForSelf())
			DrawNameplates(pLocalPC->pSpawn);
		if (Ui::Settings.GetRenderForGroup() && pLocalPC->pGroupInfo)
		{
			for (int i = 0; i < MAX_GROUP_SIZE; i++)
			{
				auto groupMember = pLocalPC->pGroupInfo->GetGroupMember(i);
				DrawNameplates(groupMember->GetPlayer());
			}
		}
	}
}

sol::object DoCreateModule(sol::this_state s)
{
	sol::state_view L(s);

	sol::table module = L.create_table();

	module["ProjectWorldCoordinatesToScreen"] = [](sol::this_state L, const float x, const float y, const float z) -> ImVec2
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