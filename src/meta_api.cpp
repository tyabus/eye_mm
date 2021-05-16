// meta_api.cpp - minimal implementation of metamod's plugin interface
// This is intended to illustrate the (more or less) bare minimum code
// required for a valid metamod plugin, and is targeted at those who want
// to port existing HL/SDK DLL code to run as a metamod plugin.

/*
 * Copyright (c) 2001-2002 Will Day <willday@hpgx.net>
 *
 *    This file is part of Metamod.
 *
 *    Metamod is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Metamod is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Metamod; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#include "extdll.h" // always
#include "meta_api.h" // of course
#include "sdk_util.h" // UTIL_LogPrintf, etc
#include "eye.h"

// From SDK dlls/h_export.cpp:

//! Holds engine functionality callbacks
enginefuncs_t g_engfuncs;
globalvars_t  *gpGlobals;

static cvar_t eye_cvars[] = {
	{"eye_version", VVERSION, FCVAR_EXTDLL | FCVAR_SERVER , 0, NULL},
	{"eye_setinfo", "_eye_pw", FCVAR_EXTDLL , 0, NULL},
	{"eye_password", "", FCVAR_EXTDLL , 0, NULL},
	{NULL,NULL,0,0,NULL},
};

// Receive engine function table from engine.
// This appears to be the _first_ DLL routine called by the engine, so we
// do some setup operations here.
C_DLLEXPORT void WINAPI GiveFnptrsToDll( enginefuncs_t* pengfuncsFromEngine, globalvars_t *pGlobals )
{
	memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
	gpGlobals = pGlobals;
}


// Description of plugin
plugin_info_t Plugin_info = {
	META_INTERFACE_VERSION, // ifvers
	VNAME,			// name
	VVERSION,		// version
	VDATE,			// date
	VAUTHOR,		// author
	VURL,			// url
	VLOGTAG,		// logtag
	PT_CHANGELEVEL,		// loadable
	PT_CHANGELEVEL,		// unloadable
};

// Global vars from metamod:
meta_globals_t *gpMetaGlobals;		// metamod globals
gamedll_funcs_t *gpGamedllFuncs;	// gameDLL function tables
mutil_funcs_t *gpMetaUtilFuncs;		// metamod utility functions

// Metamod requesting info about this plugin:
//  ifvers			(given) interface_version metamod is using
//  pPlugInfo		(requested) struct with info about plugin
//  pMetaUtilFuncs	(given) table of utility functions provided by metamod
C_DLLEXPORT int Meta_Query(char *ifvers, plugin_info_t **pPlugInfo,
		mutil_funcs_t *pMetaUtilFuncs) 
{
	if(ifvers);	// to satisfy gcc -Wunused
	// Give metamod our plugin_info struct
	Plugin_info.ifvers = strdup(ifvers);
	*pPlugInfo=&Plugin_info;
	// Get metamod utility function table.
	gpMetaUtilFuncs=pMetaUtilFuncs;
	return(TRUE);
}

// Metamod attaching plugin to the server.
//  now				(given) current phase, ie during map, during changelevel, or at startup
//  pFunctionTable	(requested) table of function tables this plugin catches
//  pMGlobals		(given) global vars from metamod
//  pGamedllFuncs	(given) copy of function tables from game dll
C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME now, META_FUNCTIONS *pFunctionTable, 
		meta_globals_t *pMGlobals, gamedll_funcs_t *pGamedllFuncs) 
{
	if(now);	// to satisfy gcc -Wunused
	if(!pMGlobals) {
		return(FALSE);
	}
	gpMetaGlobals=pMGlobals;
	if(!pFunctionTable) {
		return(FALSE);
	}

	memset(pFunctionTable, 0, sizeof(META_FUNCTIONS));
	pFunctionTable->pfnGetEntityAPI2      = __GetEntityAPI2;
	pFunctionTable->pfnGetEntityAPI2_Post = __GetEntityAPI2_Post;

	gpGamedllFuncs=pGamedllFuncs;

	int i;
	for(i=0;eye_cvars[i].name;i++) CVAR_REGISTER(&eye_cvars[i]);

	for (i=0;i<=32;i++)
	{
		players[i].IsConnected = false;
		players[i].IsAllowedToUse = false;

		players[i].Eye     = NULL;
		players[i].curView = NULL;
	}

	return(TRUE);
}

// Metamod detaching plugin from the server.
// now		(given) current phase, ie during map, etc
// reason	(given) why detaching (refresh, console unload, forced unload, etc)
C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason) {
	if(now && reason);	// to satisfy gcc -Wunused

	for(int i = 0;i<=32;i++)
	{
		RemoveEye(i);

		players[i].Eye     = NULL;
		players[i].curView = NULL;
	}

	return(TRUE);
}
