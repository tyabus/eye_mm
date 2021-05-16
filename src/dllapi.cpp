#include "extdll.h"
#include "sdk_util.h"
#include "dllapi.h"
#include "meta_api.h"
#include "entity_state.h"
#include "pm_defs.h"
#include "eye.h"

#include <string.h>

playerid_t players[33];
static char *pDotSpriteName     = (char*)"sprites/laserdot.spr";
static int   m_dotspriteTexture = 0;
static char szbuffer[1024];

/**************************************************************
*
* Removes view location entity
*
**************************************************************/
void RemoveEye(int i)
{
	if (!FNullEnt(players[i].Eye))
	{
		entvars_t *pev   = VARS(players[i].Eye);
		pev->flags      |= FL_KILLME;
		pev->targetname  = 0;
		pev->classname   = 0;
		players[i].Eye   = NULL;
	}
}


/**************************************************************
*
* Creates view location entity
*
**************************************************************/
static void CreateEye(int i, edict_t *pThis)
{
	players[i].Eye  = CREATE_NAMED_ENTITY(MAKE_STRING("func_wall")); //func_wall is in all mods

	entvars_t *pev  = VARS(players[i].Eye);

	pev->solid      = SOLID_NOT;
	pev->movetype   = MOVETYPE_NOCLIP;

	pev->rendermode = kRenderGlow;
	pev->renderfx   = kRenderFxNoDissipation;
	pev->renderamt  = 255;

	SET_MODEL(players[i].Eye, pDotSpriteName);

	pev->owner      = pThis;
}

/**************************************************************
*
* Updates view entity location and angles
*
**************************************************************/
static void UpdateEye(edict_t *pThis)
{
	int i = ENTINDEX2(pThis);

	if (FNullEnt(players[i].Eye))
	{
		CreateEye(i,pThis);
	}

	entvars_t *pev = VARS(players[i].Eye);

	pev->angles = pThis->v.v_angle;

	SET_ORIGIN(players[i].Eye, pThis->v.origin + pThis->v.view_ofs);


	// is this player EYEing something? Yes? Set the speed
	if (players[i].curPlayer != NULL)
	{
		if (players[i].next_time <= gpGlobals->time)
		{
			entvars_t *pevEye = VARS(players[i].curPlayer);

			float speed = pevEye->velocity.Length2D();
			float percent;
			if (pevEye->maxspeed > 0)
				percent = (speed/pevEye->maxspeed) * 100.0f;
			else
				percent = 0;

			sprintf(szbuffer, "%s (%d %d)\nH:%.0f\nA:%.0f\nS:%.0f (%.0f%%)",
				STRING(pevEye->netname), pevEye->playerclass, pevEye->team, pevEye->health, pevEye->armorvalue, speed, percent);
			UTIL_SendHudMessage(pThis, 1, szbuffer);

			players[i].next_time = gpGlobals->time + 0.7f;
		}
	}

}

/**************************************************************
* 
* Register client as it connects to server
*
**************************************************************/
static void ClientPutInServer(edict_t *pThis)
{
	int id = ENTINDEX2(pThis);

	if(players[id].IsConnected)
		RETURN_META(MRES_IGNORED);

	RemoveEye(id);

	players[id].IsConnected = true;
	players[id].IsAllowedToUse = false;
	players[id].old_in_jump = false;

	players[id].next_time = gpGlobals->time + 1.0f;
	players[id].plCount = 0;

	players[id].Eye     = NULL;
	players[id].curView = NULL;
	players[id].curPlayer = NULL;

	RETURN_META(MRES_IGNORED);
}


/**************************************************************
* 
* Unregister client as it disconnects
*
**************************************************************/
static void ClientDisconnect_Post(edict_t *pThis)
{
	int id = ENTINDEX2(pThis);

	//Check for players that are 'eyeing' this player
	for(int i = 1, max = gpGlobals->maxClients; i <= max; i++)
	{
		if(i == id || players[i].curPlayer != pThis)
			continue;

		edict_t * pPlayer = INDEXENT2(i);
		if(FNullEnt(pPlayer))
			continue;

		UTIL_SendTextMsg(pPlayer, HUD_PRINTTALK, "[EYE] Reset back to normal! (Target disconnected from server)\n");

		SET_VIEW(pPlayer, pPlayer);

		players[i].curView = NULL;
		players[i].curPlayer = NULL;
	}

	//Remove eyeball
	RemoveEye(id);

	players[id].Eye     = NULL;
	players[id].curView = NULL;
	players[id].curPlayer = NULL;

	players[id].IsConnected = false;
	players[id].IsAllowedToUse = false;

	SET_VIEW(pThis, pThis);

	RETURN_META(MRES_IGNORED);
}


/**************************************************************
* 
* Update view entity location and angles
*
**************************************************************/
static void PlayerPostThink_Post( edict_t *pThis )
{
	int id = ENTINDEX2(pThis);
	if(!id) RETURN_META(MRES_HANDLED);
	
	if(!players[id].IsConnected)
	{
		//Fixes bots
		ClientPutInServer(pThis);
	}
	
	UpdateEye(pThis);

	RETURN_META(MRES_HANDLED);
}


/**************************************************************
* 
* Modify send entity data
*
**************************************************************/
static int AddToFullPack_Post(struct entity_state_s *state, int e, edict_t *ent, edict_t *pThis, int hostflags, int player, unsigned char *pSet)
{
	if (!gpGlobals->deathmatch) RETURN_META_VALUE(MRES_IGNORED,1);
	
	//Check if this entity has been stopped
	if (META_RESULT_STATUS>MRES_HANDLED) //override && supercede
	{
		if (!META_RESULT_OVERRIDE_RET(int))
		{
			//Stopped by other plugin .. 
			RETURN_META_VALUE(MRES_IGNORED,0);
		}
	}
	else if (!META_RESULT_ORIG_RET(int))
	{
		//Stopped by game dll .. 
		RETURN_META_VALUE(MRES_IGNORED,0);
	}
	
	//Get and check userid
	int id = ENTINDEX2(pThis);
	if (id==0) RETURN_META_VALUE(MRES_IGNORED,0);

	//Not send model of current player .. It just would block all vis
	if (!FNullEnt(players[id].curView) && players[id].curView->v.owner==ent) RETURN_META_VALUE(MRES_OVERRIDE,0);
	
	//pointer compare
	bool IsOurView = false;
	for(int i = 1, max = gpGlobals->maxClients; i <= max; i++)
	{
		if (players[i].Eye && players[i].Eye==ent)
		{
			IsOurView = true;
			
			state->modelindex = 0;
			state->eflags |= EFLAG_SLERP;
			
			break;
		}
	}
	
	if (!IsOurView)
	{
		RETURN_META_VALUE(MRES_IGNORED,0);
	}
	
	//It is our ent .. but it isn't used by this client .. don't send over network then
	if (players[id].curView!=ent)
	{
		RETURN_META_VALUE(MRES_OVERRIDE,0);
	}
		
	RETURN_META_VALUE(MRES_IGNORED,0);
}


/**************************************************************
* 
* Checks if player has password set
*
**************************************************************/
static void ClientUserInfoChanged(edict_t *pEntity, char *infobuffer)
{
	int i = ENTINDEX2(pEntity);
	if(!i)  
		RETURN_META(MRES_IGNORED);

	if(!players[i].IsConnected)
		ClientPutInServer(pEntity);
	
	
	//Default return setting
	players[i].IsAllowedToUse = true;
	
	
	//Check if info key cvar is null
	const char * c_eye_setinfo = CVAR_GET_STRING("eye_setinfo");
	
	if(!c_eye_setinfo || !*c_eye_setinfo)
		RETURN_META(MRES_IGNORED);
	
	//not really needed     
	char eye_setinfo[256];
	strncpy(eye_setinfo, c_eye_setinfo, 256);
	eye_setinfo[255] = 0;
		
	
	//Check if info key value cvar is null
	const char * eye_password = CVAR_GET_STRING("eye_password");
	
	if(!eye_password || !*eye_password)
		RETURN_META(MRES_IGNORED);
	
	
	//Check if player's info key is null
	const char * userpassword = g_engfuncs.pfnInfoKeyValue(infobuffer, eye_setinfo);
	
	if(!userpassword || !*userpassword)
	{
		//Not equal
		players[i].IsAllowedToUse = false;
		
		RETURN_META(MRES_IGNORED);
	}
	
	
	//Compare passwords
	if(!strcmp(eye_password, userpassword))
	{
		//equal
		players[i].IsAllowedToUse = true;
	}
	else
	{
		//Not equal
		players[i].IsAllowedToUse = false;
	}

	RETURN_META(MRES_HANDLED);
}


/**************************************************************
* 
* Handle commands
*
**************************************************************/
static void ClientCommand( edict_t *pThis )
{
	int id = ENTINDEX2(pThis);

	if (!id)
		RETURN_META(MRES_IGNORED);

	if (!players[id].IsAllowedToUse)
		RETURN_META(MRES_IGNORED);

	const char *pcmd = CMD_ARGV(0);

	if (!strcasecmp(pcmd,"eye"))
	{
		edict_t *pView;
		const char *param = CMD_ARGV(1);

		if (players[id].curView != NULL && players[id].curPlayer != NULL)
		{
			UTIL_SendTextMsg(pThis, HUD_PRINTTALK, "[EYE] Reset back to normal!\n");

			SET_VIEW(pThis, pThis);

			players[id].curView = NULL;
			players[id].curPlayer = NULL;

			RETURN_META(MRES_SUPERCEDE);
		}

		if (param==NULL || !*param)
		{
			UTIL_SendTextMsg(pThis, HUD_PRINTTALK, "[EYE] usage: eye <part_of_username>|#playerid\n");
			UTIL_SendTextMsg(pThis, HUD_PRINTTALK, "[EYE] usage: eye to stop spectating\n");

			RETURN_META(MRES_SUPERCEDE);
		}

		pView = UTIL_GetView(param);

		if (!FNullEnt(pView))
		{
			int pViewid = ENTINDEX2(pView->v.owner);

			if(id == pViewid)
			{
				UTIL_SendTextMsg(pThis, HUD_PRINTTALK, "[EYE] You cant spectate yourself!\n");

				RETURN_META(MRES_SUPERCEDE);
			}

			UTIL_SendTextMsg(
				pThis,
				HUD_PRINTTALK,
				UTIL_VarArgs(
					"[EYE] Setting view to '%s'!\n",
					STRING(pView->v.owner->v.netname)
				)
			);

			SET_VIEW(pThis,pView);

			players[id].curView = pView;
			players[id].curPlayer = UTIL_GetPlayerEdict(param);

			players[id].plCount = ENTINDEX2(players[id].curPlayer);
		}
		else
		{
			UTIL_SendTextMsg(pThis, HUD_PRINTTALK, "[EYE] Cant find specified player!\n");
		}

		RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
}


/**************************************************************
*
*
*
**************************************************************/
static void CmdStart(edict_t *pThis, struct usercmd_s *pCmd, unsigned int random_seed)
{       
	int id = ENTINDEX2(pThis);
	if(!id) 
		RETURN_META(MRES_IGNORED);
	
	if(!players[id].IsConnected)
		ClientPutInServer(pThis);
	
	//already pressed +jump?
	int new_in_jump = ((pCmd->buttons & IN_JUMP) != 0);
	if(players[id].old_in_jump)
	{
		players[id].old_in_jump = new_in_jump;
		RETURN_META(MRES_IGNORED);
	}
	
	//has not pressed +jump?
	players[id].old_in_jump = new_in_jump;
	if(!new_in_jump)
		RETURN_META(MRES_IGNORED);
	
	//not allowed?
	if(!players[id].IsAllowedToUse)
		RETURN_META(MRES_IGNORED);
	
	if(players[id].curView)
	{       
		players[id].plCount++;
		if (players[id].plCount > gpGlobals->maxClients)
			players[id].plCount = 1;
		
		// we can safely assume at least one person is connected
		edict_t* pPlayer = INDEXENT2(players[id].plCount);
		while (!players[ENTINDEX2(pPlayer)].IsConnected) {
			players[id].plCount++;
			if (players[id].plCount > gpGlobals->maxClients)
				players[id].plCount = 1;
			
			pPlayer = INDEXENT2(players[id].plCount);
		}
		
		edict_t *pView = players[players[id].plCount].Eye;
		
		if (!FNullEnt(pView))
		{
			UTIL_SendTextMsg(
				pThis,
				HUD_PRINTTALK,
				UTIL_VarArgs(
					"[EYE] Setting view to '%s'!\n",
					STRING(pView->v.owner->v.netname)
				)
			);
			
			SET_VIEW(pThis, pView);
		
			players[id].curView = pView;
			players[id].curPlayer = pPlayer;
		}
	}
	
	RETURN_META(MRES_HANDLED);
}


/**************************************************************
* 
* Clear data on map start
*
**************************************************************/
static void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{       
	m_dotspriteTexture = PRECACHE_MODEL(pDotSpriteName);
	
	float next_time = gpGlobals->time + 1.0f;
	for (int i = 0; i <= 32; i++)
	{
		players[i].next_time = next_time;
		players[i].IsAllowedToUse = false;
		players[i].IsConnected = false;
		
		players[i].plCount = 0;
		
		players[i].Eye     = NULL;
		players[i].curView = NULL;
		players[i].curPlayer = NULL;
	}
	
	RETURN_META(MRES_HANDLED);
}


/**************************************************************
* 
* Clear data on map close
*
**************************************************************/
static void ServerDeactivate_Post(void)
{       
	for(int i = 0; i <= 32; i++)
	{
		RemoveEye(i);

		players[i].Eye     = NULL;
		players[i].curView = NULL;
		players[i].curPlayer = NULL;
		
		players[i].IsConnected = false;
		players[i].IsAllowedToUse = false;
		players[i].old_in_jump = false;
	}
	
	RETURN_META(MRES_HANDLED);
}


/*************************************************************
*
**************************************************************
*
* Register DLLAPI Hooks
*
**************************************************************
*
**************************************************************/
int __GetEntityAPI2( DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
{
	if(!pFunctionTable) {
		return(FALSE);
	}
	else if(*interfaceVersion != INTERFACE_VERSION) {
		*interfaceVersion = INTERFACE_VERSION;
		return(FALSE);
	}
	
	memset( pFunctionTable, 0, sizeof( DLL_FUNCTIONS ) );
	
	pFunctionTable->pfnClientPutInServer     = ClientPutInServer;
	pFunctionTable->pfnClientCommand         = ClientCommand;
	pFunctionTable->pfnClientUserInfoChanged = ClientUserInfoChanged;
	pFunctionTable->pfnServerActivate        = ServerActivate;
	*(long*)pFunctionTable->pfnCmdStart      = (long)CmdStart;

	return(TRUE);
}


int __GetEntityAPI2_Post( DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
{
	if(!pFunctionTable) {
		return(FALSE);
	}
	else if(*interfaceVersion != INTERFACE_VERSION) {
		*interfaceVersion = INTERFACE_VERSION;
		return(FALSE);
	}
	
	memset( pFunctionTable, 0, sizeof( DLL_FUNCTIONS ) );
	
	pFunctionTable->pfnServerDeactivate = ServerDeactivate_Post;
	pFunctionTable->pfnClientDisconnect = ClientDisconnect_Post;
	pFunctionTable->pfnPlayerPostThink  = PlayerPostThink_Post;
	pFunctionTable->pfnAddToFullPack    = AddToFullPack_Post;
	
	return(TRUE);
}
