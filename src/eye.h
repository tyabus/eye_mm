#ifndef __EYE_H__
#define __EYE_H__

#define VVERSION	"1.31"
#define VLOGTAG		"EYE"
#define VDATE		__DATE__
#define VNAME		"Eye " VVERSION " Metamod Plugin"
#define VAUTHOR		"hullu & disq & Buzzkill & tyabus"
#define VURL		"https://github.com/tyabus/eye_mm"

typedef struct
{
	BOOL IsConnected;
	BOOL IsAllowedToUse;

	int plCount;
	int old_in_jump;

	edict_t *Eye;
	edict_t *curView;
	edict_t *curPlayer;

	float next_time;
} playerid_t;

extern playerid_t players[33];

// dllapi.cpp:
extern void RemoveEye( int i );
extern int __GetEntityAPI2( DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion );
extern int __GetEntityAPI2_Post( DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion );

// sdk_util.cpp:
extern edict_t *UTIL_GetView( const char *cmd );
extern edict_t *UTIL_GetPlayerEdict( const char *cmd );
extern void UTIL_SendTextMsg( edict_t *client, int msg_dest, const char *msg_name );
extern void UTIL_SendHudMessage( edict_t *pEntity, int msg_dest, char *pMessage );
extern int ENTINDEX2( edict_t *pEdict );
extern edict_t *INDEXENT2( int playerIndex );

#endif /*__EYE_H__*/
