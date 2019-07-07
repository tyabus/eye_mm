// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// sdk_util.cpp - utility routines from HL SDK util.cpp

// Selected portions of dlls/util.cpp from SDK 2.1.
// Functions copied from there as needed...

/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== util.cpp ========================================================

  Utility code.  Really not optional after all.

*/

#include <string.h>			// for strncpy(), etc

#include <extdll.h>
#include <sdk_util.h>
#include <meta_api.h>

#include "eye.h"


char* UTIL_VarArgs( char *format, ... )
{
	va_list		argptr;
	static char	string[1024];
	
	va_start  (argptr, format);
	_vsnprintf(string, 1024, format, argptr);
	va_end    (argptr);
	
	string[sizeof(string)-1] = 0;

	return string;
}
	

//=========================================================
// UTIL_LogPrintf - Prints a logged message to console.
// Preceded by LOG: ( timestamp ) < message >
//=========================================================
void UTIL_LogPrintf( char *fmt, ... )
{
	va_list			argptr;
	static char		string[1024];
	
	va_start  ( argptr, fmt );
	_vsnprintf( string, 1024, fmt, argptr );
	va_end    ( argptr );
	
	string[sizeof(string)-1] = 0;

	// Print to server console
	ALERT( at_logged, "%s", string );
}


/**************************************************************
* 
* Send message to client
*
**************************************************************/

short MyFixedSigned16( float value, float scale ){
	int output;
	output = int(value * scale);
	if ( output > 32767 )
		output = 32767;
	if ( output < -32768 )
		output = -32768;
	return (short)output;
}

unsigned short MyFixedUnsigned16( float value, float scale ){
	int output;
	output = int(value * scale);
	if ( output < 0 )
		output = 0;
	if ( output > 0xFFFF )
		output = 0xFFFF;
	return (unsigned short)output;
}


void UTIL_SendTextMsg(edict_t *client, int msg_dest, const char *msg_name)
{
	static int message_TextMsg = 0;
	
	if(!message_TextMsg)
		message_TextMsg = GET_USER_MSG_ID(PLID,"TextMsg",NULL);
	
	if (!message_TextMsg || !client || client->v.flags & FL_FAKECLIENT) return;
	
	MESSAGE_BEGIN( MSG_ONE, message_TextMsg, NULL, client );
		WRITE_BYTE( msg_dest );
		WRITE_STRING( msg_name );
	MESSAGE_END();
}

void UTIL_SendHudMessage(edict_t *pEntity, int msg_dest, char *pMessage)
{
	MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pEntity );
		WRITE_BYTE(29); 
		WRITE_BYTE(msg_dest & 0xFF); 
		WRITE_SHORT(MyFixedSigned16(0.03, (1<<13) ));
		WRITE_SHORT(MyFixedSigned16(0.1, (1<<13) ));
		WRITE_BYTE(0);
		WRITE_BYTE(255);
		WRITE_BYTE(255);
		WRITE_BYTE(255);
		WRITE_BYTE(0);
		WRITE_BYTE(255);
		WRITE_BYTE(255);
		WRITE_BYTE(250);
		WRITE_BYTE(0);
		WRITE_SHORT(MyFixedUnsigned16(0, (1<<8) ));
		WRITE_SHORT(MyFixedUnsigned16(1, (1<<8) ));
		WRITE_SHORT(MyFixedUnsigned16(5, (1<<8) ));
		WRITE_STRING(pMessage);
	MESSAGE_END();
}



/**************************************************************
* 
* Alternative INDEXENT function with error checking
*
**************************************************************/
edict_t * INDEXENT2( int playerIndex )
{
	edict_t * pPlayer = NULL;

	if ( playerIndex > 0 && playerIndex <= gpGlobals->maxClients ) 
	{
		pPlayer = INDEXENT( playerIndex );
	}
	
	return pPlayer;
}


/**************************************************************
* 
* Alternative ENTINDEX2 function with error checking
*
**************************************************************/
int ENTINDEX2(edict_t *pEdict)
{
	if(pEdict)
	{
		int index = ENTINDEX(pEdict);
		if( index > 0 && index <= gpGlobals->maxClients )
			return(index);
	}
	
	return(0);
}


/**************************************************************
* 
* Get player entity by string
*
**************************************************************/
edict_t *UTIL_GetView(const char *cmd)
{
	if (!cmd) return NULL;

	int pid=-1;
	
	if (cmd[0]=='#')
	{
		sscanf(&cmd[1],"%d",&pid);
		
		edict_t* pPlayer = INDEXENT2(pid);
		
		if (FNullEnt(pPlayer) || !players[ENTINDEX2(pPlayer)].is_connected) 
			return(NULL);

		return (players[ENTINDEX2(pPlayer)].Eye);
  	}
	
	for (int i=1;i<=gpGlobals->maxClients;i++)
  	{
		edict_t* pPlayer = INDEXENT2(i);
	
		if (FNullEnt(pPlayer) || !players[ENTINDEX2(pPlayer)].is_connected) 
			continue;
		if (strstr(STRING(pPlayer->v.netname),cmd)==NULL) 
			continue;

		return (players[ENTINDEX2(pPlayer)].Eye);
  	}
	
	return(NULL);
}

edict_t *UTIL_GetPlayerEdict(const char *cmd)
{
	if (!cmd) return NULL;

	int pid=-1;
	
	if (cmd[0]=='#')
	{
		sscanf(&cmd[1],"%d",&pid);
		
		edict_t* pPlayer = INDEXENT2(pid);
		
		if (FNullEnt(pPlayer) || !players[ENTINDEX2(pPlayer)].is_connected) 
			return(NULL);

		return (pPlayer);
  	}
	
	for (int i=1;i<=gpGlobals->maxClients;i++)
  	{
		edict_t* pPlayer = INDEXENT2(i);
	
		if (FNullEnt(pPlayer) || !players[ENTINDEX2(pPlayer)].is_connected) 
			continue;
		if (strstr(STRING(pPlayer->v.netname),cmd)==NULL) 
			continue;

		return (pPlayer);
  	}
	
	return(NULL);
}
