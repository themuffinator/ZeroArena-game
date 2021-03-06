/*
===========================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of Spearmint Source Code.

Spearmint Source Code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Spearmint Source Code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Spearmint Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, Spearmint Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following
the terms and conditions of the GNU General Public License.  If not, please
request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional
terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/
//

#include "g_local.h"

level_locals_t	level;

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
	int			gameFlags;
	float		rangeMin;
	float		rangeMax;
	qboolean	rangeIntegral;
	char		*description;

	int			modificationCount;  // for tracking changes
} cvarTable_t;

// game cvar flags
#define GCF_TRACK_CHANGE	1	// track this variable, and announce if changed
#define GCF_TEAM_SHADER		2	// if changed, update shader state
#define GCF_DO_RESTART		4	// if changed, require fresh map load in map_restart

#define RANGE_ALL 0, 0, qfalse
#define RANGE_BOOL 0, 1, qtrue
#define RANGE_INT(min,max) min, max, qtrue
#define RANGE_FLOAT(min,max) min, max, qfalse

int gravity_modcount = -1;

gentity_t		g_entities[MAX_GENTITIES];
gplayer_t		g_players[MAX_CLIENTS];
gconnection_t	g_connections[MAX_CLIENTS];

vmCvar_t	g_allowVote;
vmCvar_t	g_banIPs;
vmCvar_t	g_cheats;
vmCvar_t	g_debugDamage;
vmCvar_t	g_debugMove;
vmCvar_t	g_dedicated;
vmCvar_t	g_dmFlags;
vmCvar_t	g_doWarmup;
vmCvar_t	g_filterBan;
vmCvar_t	g_friendlyFire;
vmCvar_t	g_gameType;
vmCvar_t	g_gravity;
vmCvar_t	g_harvester_skullTimeout;
vmCvar_t	g_inactivity;
vmCvar_t	g_knockback;
vmCvar_t	g_listEntity;
vmCvar_t	g_logFile;
vmCvar_t	g_logFileSync;
vmCvar_t	g_maxClients;
vmCvar_t	g_maxGameClients;
vmCvar_t	g_motd;
vmCvar_t	g_needPassword;
vmCvar_t	g_obeliskHealth;
vmCvar_t	g_obeliskRegenAmount;
vmCvar_t	g_obeliskRegenPeriod;
vmCvar_t	g_obeliskRespawnDelay;
vmCvar_t	g_password;
vmCvar_t	g_podiumDist;
vmCvar_t	g_podiumDrop;
vmCvar_t	g_quadFactor;
vmCvar_t	g_rankings;
vmCvar_t	g_restarted;
vmCvar_t	g_scoreLimit;
vmCvar_t	g_singlePlayerActive;
vmCvar_t	g_smoothClients;
vmCvar_t	g_speed;
vmCvar_t	g_synchronousClients;
vmCvar_t	g_teamAutoJoin;
vmCvar_t	g_teamForceBalance;
vmCvar_t	g_timeLimit;
vmCvar_t	g_warmupCountdownTime;
vmCvar_t	g_weaponRespawn;
vmCvar_t	g_weaponTeamRespawn;
vmCvar_t	pmove_fixed;
vmCvar_t	pmove_msec;
vmCvar_t	pmove_overBounce;
#ifdef MISSIONPACK
vmCvar_t	g_redTeamName;
vmCvar_t	g_blueTeamName;
vmCvar_t	g_proxMineTimeout;
#endif
vmCvar_t	g_playerCapsule;
vmCvar_t	g_instaGib;

vmCvar_t	g_botEnable;
vmCvar_t	g_dropFlags;

vmCvar_t	gt_frags_limit;
vmCvar_t	gt_frags_timelimit;
vmCvar_t	gt_duel_frags_limit;
vmCvar_t	gt_duel_frags_timelimit;
vmCvar_t	gt_teams_frags_limit;
vmCvar_t	gt_teams_frags_mercylimit;
vmCvar_t	gt_teams_frags_timelimit;
vmCvar_t	gt_teams_captures_limit;
vmCvar_t	gt_teams_captures_timelimit;
vmCvar_t	gt_elimination_lives_limit;
vmCvar_t	gt_elimination_timelimit;
vmCvar_t	gt_points_limit;
vmCvar_t	gt_points_timelimit;
vmCvar_t	gt_rounds_limit;
vmCvar_t	gt_rounds_timelimit;

vmCvar_t	g_teamSize_max;
vmCvar_t	g_teamSize_min;
vmCvar_t	g_teamTotal_max;
vmCvar_t	g_teamTotal_min;

vmCvar_t	g_treasonDamage;

vmCvar_t	g_ammoRules;
vmCvar_t	g_armorRules;
vmCvar_t	g_classicItemRespawns;

vmCvar_t	g_forceWeaponColors;

vmCvar_t	g_doReady;

vmCvar_t	g_forceRespawn_delayMax;
vmCvar_t	g_forceRespawn_delayMin;

vmCvar_t	g_overTime;

vmCvar_t	g_warmupDelay;
vmCvar_t	g_warmupReadyPercentage;
vmCvar_t	g_warmupWeaponSet;

vmCvar_t	g_intermissionForceExitTime;
vmCvar_t	g_gunyoffset;
vmCvar_t	g_gunzoffset;

vmCvar_t	pmove_q2;
vmCvar_t	pmove_q2slide;
vmCvar_t	pmove_q2air;

static cvarTable_t		gameCvarTable[] = {
	// don't override the cheat state set by the system
	{ &g_cheats, "sv_cheats", "", 0, 0, RANGE_ALL },

	// noset vars
	{ NULL, "gameVersion", PRODUCT_NAME " " PRODUCT_VERSION " " PLATFORM_STRING " " PRODUCT_DATE, CVAR_SERVERINFO | CVAR_ROM, 0, RANGE_ALL },
	{ NULL, "gameProtocol", GAME_PROTOCOL, CVAR_SERVERINFO | CVAR_ROM, 0, RANGE_ALL },
	{ &g_restarted, "g_restarted", "0", CVAR_ROM, 0, RANGE_ALL },

	// latched vars
	{ &g_gameType, "g_gameType", "1", CVAR_SERVERINFO, 0, RANGE_INT( 0, GT_MAX_GAME_TYPE - 1 )  },
	{ &g_instaGib, "g_instaGib", "0", CVAR_LATCH, GCF_DO_RESTART, RANGE_BOOL },

	{ &g_maxClients, "sv_maxClients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, RANGE_ALL, "maximum number of clients connected to server" },
	{ &g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, RANGE_INT( 0, MAX_CLIENTS - 1 ) },

	// change anytime vars
	{ &g_dmFlags, "dmFlags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, GCF_TRACK_CHANGE, RANGE_ALL, "deathmatch flags to alter game rules" },
	{ &g_scoreLimit, "scoreLimit", "20", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, INT_MAX ), "score to win the match" },
	{ &g_timeLimit, "timeLimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, INT_MAX / 60000 ), "maximum duration of the match" },

	{ &g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, 0, RANGE_BOOL },

	{ &g_friendlyFire, "g_friendlyFire", "1", CVAR_ARCHIVE, GCF_TRACK_CHANGE, RANGE_BOOL, "team members can damage each other" },

	{ &g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE, 0, RANGE_BOOL, "players can automatically join a team" },
	{ &g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE, 0, RANGE_BOOL, "try to balance teams by player numbers" },

	{ &g_warmupCountdownTime, "g_warmupCountdownTime", "20", CVAR_ARCHIVE, GCF_TRACK_CHANGE, RANGE_ALL, "countdown time to match start" },
	{ &g_doWarmup, "g_doWarmup", "1", CVAR_ARCHIVE, GCF_TRACK_CHANGE, RANGE_BOOL, "enable warmup phase before match" },
	{ &g_logFile, "g_logFile", "games.log", CVAR_ARCHIVE, 0, RANGE_ALL, "log game events to file" },
	{ &g_logFileSync, "g_logFileSync", "0", CVAR_ARCHIVE, 0, RANGE_ALL },

	{ &g_password, "g_password", "", CVAR_USERINFO, 0, RANGE_ALL },

	{ &g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, 0, RANGE_ALL },
	{ &g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, 0, RANGE_ALL },

	{ &g_needPassword, "g_needPassword", "0", CVAR_SERVERINFO | CVAR_ROM, 0, RANGE_BOOL },

	{ &g_dedicated, "dedicated", "0", 0, 0, RANGE_ALL },

	{ &g_speed, "g_speed", "320", 0, GCF_TRACK_CHANGE, RANGE_ALL },
	{ &g_gravity, "g_gravity", "800", CVAR_SERVERINFO, GCF_TRACK_CHANGE, RANGE_ALL },
	{ &g_knockback, "g_knockback", "1000", 0, GCF_TRACK_CHANGE, RANGE_ALL },
	{ &g_quadFactor, "g_quadFactor", "4", 0, GCF_TRACK_CHANGE, RANGE_ALL },
	{ &g_weaponRespawn, "g_weaponRespawn", "5", 0, GCF_TRACK_CHANGE, RANGE_ALL },
	{ &g_weaponTeamRespawn, "g_weaponTeamRespawn", "30", 0, GCF_TRACK_CHANGE, RANGE_ALL },
	{ &g_inactivity, "g_inactivity", "0", 0, GCF_TRACK_CHANGE, RANGE_BOOL },
	{ &g_debugMove, "g_debugMove", "0", 0, 0, RANGE_BOOL },
	{ &g_debugDamage, "g_debugDamage", "0", 0, 0, RANGE_BOOL },
	{ &g_motd, "g_motd", "", 0, 0, RANGE_ALL },

	{ &g_podiumDist, "g_podiumDist", "80", 0, 0, RANGE_ALL },
	{ &g_podiumDrop, "g_podiumDrop", "70", 0, 0, RANGE_ALL },

	{ &g_allowVote, "g_allowVote", "1", CVAR_ARCHIVE, 0, RANGE_BOOL },
	{ &g_listEntity, "g_listEntity", "0", 0, 0, RANGE_ALL },

	{ &g_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_SYSTEMINFO | CVAR_ROM, 0, RANGE_ALL },

	{ &g_obeliskHealth, "g_obeliskHealth", "2500", 0, 0, RANGE_ALL },
	{ &g_obeliskRegenPeriod, "g_obeliskRegenPeriod", "1", 0, 0, RANGE_ALL },
	{ &g_obeliskRegenAmount, "g_obeliskRegenAmount", "15", 0, 0, RANGE_ALL },
	{ &g_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", CVAR_SYSTEMINFO, 0, RANGE_ALL },

	{ &g_harvester_skullTimeout, "g_harvester_skullTimeout", "30", 0, 0, RANGE_ALL },
#ifdef MISSIONPACK
	{ &g_redTeamName, "g_redTeamName", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE | CVAR_SYSTEMINFO, GCF_TRACK_CHANGE | GCF_TEAM_SHADER, RANGE_ALL },
	{ &g_blueTeamName, "g_blueTeamName", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE | CVAR_SYSTEMINFO, GCF_TRACK_CHANGE | GCF_TEAM_SHADER, RANGE_ALL },

	{ &g_proxMineTimeout, "g_proxMineTimeout", "20000", 0, 0, RANGE_ALL },
#endif
	{ &g_playerCapsule, "g_playerCapsule", "0", 0, 0, RANGE_BOOL },
	{ &g_smoothClients, "g_smoothClients", "1", 0, 0, RANGE_BOOL },
	{ &pmove_overBounce, "pmove_overBounce", "0", CVAR_SYSTEMINFO, 0, RANGE_BOOL },
	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, 0, RANGE_BOOL },
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, 0, RANGE_ALL },

	{ &g_botEnable, "bot_enable", "", CVAR_SERVERINFO | CVAR_ROM, 0, RANGE_ALL },

	{ &g_dropFlags, "g_dropFlags", "0", CVAR_ARCHIVE, GCF_TRACK_CHANGE, RANGE_ALL },

	{ &gt_frags_limit, "gt_frags_limit", "50", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 10000 ) },
	{ &gt_frags_timelimit, "gt_frags_timelimit", "15", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 1000 ) },
	{ &gt_duel_frags_limit, "gt_duel_frags_limit", "0", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 10000 ) },
	{ &gt_duel_frags_timelimit, "gt_duel_frags_timelimit", "10", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 10000 ) },
	{ &gt_teams_frags_limit, "gt_teams_frags_limit", "200", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 1000 ) },
	{ &gt_teams_frags_mercylimit, "gt_teams_frags_mercylimit", "50", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 1000 ) },
	{ &gt_teams_frags_timelimit, "gt_teams_frags_timelimit", "20", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 1000 ) },
	{ &gt_teams_captures_limit, "gt_teams_captures_limit", "8", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 10000 ) },
	{ &gt_teams_captures_timelimit, "gt_teams_captures_timelimit", "20", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 10000 ) },
	{ &gt_elimination_lives_limit, "gt_elimination_lives_limit", "3", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 1000 ) },
	{ &gt_elimination_timelimit, "gt_elimination_timelimit", "20", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 1000 ) },
	{ &gt_points_limit, "gt_points_limit", "20", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 10000 ) },
	{ &gt_points_timelimit, "gt_points_timelimit", "20", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 1000 ) },
	{ &gt_rounds_limit, "gt_rounds_limit", "8", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 1000 ) },
	{ &gt_rounds_timelimit, "gt_rounds_timelimit", "3", CVAR_ARCHIVE | CVAR_NORESTART, GCF_TRACK_CHANGE, RANGE_INT( 0, 1000 ) },

	{ &g_teamSize_max, "g_teamSize_max", "4", CVAR_SERVERINFO | CVAR_LATCH, GCF_DO_RESTART, RANGE_INT( 2, TOTAL_TEAMS ) },
	{ &g_teamSize_min, "g_teamSize_min", "2", CVAR_SERVERINFO | CVAR_LATCH, GCF_DO_RESTART, RANGE_INT( 2, TOTAL_TEAMS ) },
	{ &g_teamTotal_max, "g_teamTotal_max", "2", CVAR_SERVERINFO | CVAR_LATCH, GCF_DO_RESTART, RANGE_INT( 2, TOTAL_TEAMS ) },
	{ &g_teamTotal_min, "g_teamTotal_min", "2", CVAR_SERVERINFO | CVAR_LATCH, GCF_DO_RESTART, RANGE_INT( 2, TOTAL_TEAMS ) },

	{ &g_treasonDamage, "g_treasonDamage", "0.5", CVAR_SERVERINFO, GCF_TRACK_CHANGE, RANGE_FLOAT( 0, 2 ) },

	{ &g_ammoRules, "g_ammoRules", "0", CVAR_SERVERINFO, 0, RANGE_INT( 0, ARR_COUNT-1 ) },
	{ &g_armorRules, "g_armorRules", "3", CVAR_SERVERINFO, 0, RANGE_INT( 0, WPR_COUNT-1 ) },
	{ &g_classicItemRespawns, "g_classicItemRespawns", "0", 0, 0, RANGE_BOOL },

	{ &g_forceWeaponColors, "g_forceWeaponColors", "0", CVAR_SERVERINFO, 0, RANGE_INT( 0, 3 ) }, //0 = user defined, &1 = force default colors, &2 = force team colors in team games

	{ &g_doReady, "g_doReady", "1", 0, 0, RANGE_BOOL },

	{ &g_forceRespawn_delayMax, "g_forceRespawn_delayMax", "3000", 0, GCF_TRACK_CHANGE, RANGE_INT( 50, 60000 ) },	// q3 = 20000, ql = 2400
	{ &g_forceRespawn_delayMin, "g_forceRespawn_delayMin", "1500", 0, GCF_TRACK_CHANGE, RANGE_INT( 50, 5000 ) },	// q3 = 1700, ql = 2000

	{ &g_overTime, "g_overTime", "2", CVAR_SERVERINFO, GCF_TRACK_CHANGE, RANGE_INT( 0, 10 ) },

	{ &g_warmupDelay, "g_warmupDelay", "10", 0, 0, RANGE_INT( 0, 60 ) },	//ql = 30
	{ &g_warmupReadyPercentage, "g_warmupReadyPercentage", "0.51", 0, 0, RANGE_FLOAT( 0.0f, 1.0f ) },
	{ &g_warmupWeaponSet, "g_warmupWeaponSet", "1", 0, 0, RANGE_BOOL },

	{ &g_intermissionForceExitTime, "g_intermissionForceExitTime", "20", 0, 0, RANGE_INT( 0, 60 ) },
	{ &g_gunyoffset, "g_gunyoffset", "0", CVAR_CHEAT, 0, RANGE_ALL },
	{ &g_gunzoffset, "g_gunzoffset", "-8", CVAR_CHEAT, 0, RANGE_ALL },

	{ &pmove_q2, "pmove_q2", "0", CVAR_SYSTEMINFO|CVAR_CHEAT, 0, RANGE_BOOL },
	{ &pmove_q2slide, "pmove_q2slide", "0", CVAR_SYSTEMINFO | CVAR_CHEAT, 0, RANGE_BOOL },
	{ &pmove_q2air, "pmove_q2air", "0", CVAR_SYSTEMINFO | CVAR_CHEAT, 0, RANGE_BOOL },

	{ &g_rankings, "g_rankings", "0", 0, 0, RANGE_ALL }

};

static int gameCvarTableSize = ARRAY_LEN( gameCvarTable );


void G_InitGame( int levelTime, int randomSeed, int restart );
void G_RunFrame( int levelTime );
void G_ShutdownGame( int restart );
qboolean G_SnapshotCallback( int entityNum, int playerNum );
void G_VidRestart( void );
int G_MapRestart( int levelTime, int restartTime );
void CheckExitRules( void );


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11 ) {
	switch ( command ) {
	case GAME_GETAPINAME:
		return (intptr_t)GAME_API_NAME;
	case GAME_GETAPIVERSION:
		return (GAME_API_MAJOR_VERSION << 16) | (GAME_API_MINOR_VERSION & 0xFFFF);
	case GAME_INIT:
		G_InitGame( arg0, arg1, arg2 );
		return 0;
	case GAME_SHUTDOWN:
		G_ShutdownGame( arg0 );
		return 0;
	case GAME_PLAYER_CONNECT:
		return (intptr_t)PlayerConnect( arg0, arg1, arg2, arg3, arg4 );
	case GAME_PLAYER_THINK:
		PlayerThink( arg0 );
		return 0;
	case GAME_PLAYER_USERINFO_CHANGED:
		PlayerUserinfoChanged( arg0 );
		return 0;
	case GAME_PLAYER_DISCONNECT:
		return PlayerDisconnect( arg0, arg1 );
	case GAME_PLAYER_BEGIN:
		PlayerBegin( arg0 );
		return 0;
	case GAME_CLIENT_COMMAND:
		ClientCommand( arg0 );
		return 0;
	case GAME_RUN_FRAME:
		G_RunFrame( arg0 );
		return 0;
	case GAME_CONSOLE_COMMAND:
		return G_ConsoleCommand();
	case GAME_SNAPSHOT_CALLBACK:
		return G_SnapshotCallback( arg0, arg1 );
	case GAME_VID_RESTART:
		G_VidRestart();
		return 0;
	case GAME_MAP_RESTART:
		return G_MapRestart( arg0, arg1 );
	case BOTAI_START_FRAME:
		return BotAIStartFrame( arg0 );
	case GAME_CONSOLE_COMPLETEARGUMENT:
		return G_ConsoleCompleteArgument( arg0 );
	default:
		G_Error( "game vmMain: unknown command %i", command );
		break;
	}

	return -1;
}


void QDECL G_DPrintf( const char* fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	if ( !trap_Cvar_VariableIntegerValue( "developer" ) ) {
		return;
	}

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	trap_Print( text );
}

void QDECL G_Printf( const char* fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	trap_Print( text );
}

void QDECL G_Error( const char* fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	trap_Error( text );
}

/*
================
G_FindTeamedItems

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeamedItems( void ) {
	gentity_t* e, * e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for ( i = MAX_CLIENTS, e = g_entities + i; i < level.num_entities; i++, e++ ) {
		if ( !e->inuse )
			continue;
		if ( !e->team )
			continue;
		if ( e->flags & FL_TEAMSLAVE )
			continue;
		e->teammaster = e;
		c++;
		c2++;
		for ( j = i + 1, e2 = e + 1; j < level.num_entities; j++, e2++ ) {
			if ( !e2->inuse )
				continue;
			if ( !e2->team )
				continue;
			if ( e2->flags & FL_TEAMSLAVE )
				continue;
			if ( !strcmp( e->team, e2->team ) ) {
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if ( e2->targetname ) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}

	G_DPrintf( "%i teams with %i entities\n", c, c2 );
}

void G_RemapTeamShaders( void ) {
#ifdef MISSIONPACK
	char string[1024];
	float f = level.time * 0.001;
	Com_sprintf( string, sizeof( string ), "team_icon/%s_red", g_redTeamName.string );
	AddRemap( "textures/ctf2/redteam01", string, f );
	AddRemap( "textures/ctf2/redteam02", string, f );
	Com_sprintf( string, sizeof( string ), "team_icon/%s_blue", g_blueTeamName.string );
	AddRemap( "textures/ctf2/blueteam01", string, f );
	AddRemap( "textures/ctf2/blueteam02", string, f );
	trap_SetConfigstring( CS_SHADERSTATE, BuildShaderStateConfig() );
#endif
}


/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars( void ) {
	int			i;
	cvarTable_t* cv;
	qboolean remapped = qfalse;

	for ( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );

		if ( cv->rangeMin != 0 || cv->rangeMax != 0 ) {
			trap_Cvar_CheckRange( cv->cvarName, cv->rangeMin, cv->rangeMax, cv->rangeIntegral );
		}

		if ( cv->vmCvar ) {
			cv->modificationCount = cv->vmCvar->modificationCount;
		} else {
			cv->modificationCount = 0;
		}

		if ( cv->gameFlags & GCF_TEAM_SHADER ) {
			remapped = qtrue;
		}
	}

	if ( remapped ) {
		G_RemapTeamShaders();
	}

	//muff: allow co-op mode...
#if 0
	// Don't allow single player gametype to be used in multiplayer.
	if ( g_gameType.integer == GT_CAMPAIGN && !g_singlePlayerActive.integer ) {
		trap_Cvar_SetValue( "g_gameType", DEFAULT_GAMETYPE );
		trap_Cvar_Update( &g_gameType );
	}
#endif
	// Don't allow instagib in single player mode.
	if ( ( g_singlePlayerActive.integer || GTF(GTF_CAMPAIGN) ) && g_instaGib.integer ) {
		trap_Cvar_SetValue( "g_instaGib", 0 );
		trap_Cvar_Update( &g_instaGib );
	}

	if ( g_instaGib.integer ) {
		trap_Cvar_Set( "sv_gametypeName", va( "Instagib %s", gt[g_gameType.integer].longName ) );
		trap_Cvar_Set( "sv_gametypeNetName", va( "Insta%s", gt[g_gameType.integer].shortName ) );
	} else {
		trap_Cvar_Set( "sv_gametypeName", gt[g_gameType.integer].longName );
		trap_Cvar_Set( "sv_gametypeNetName", gt[g_gameType.integer].shortName );
	}

	level.warmupModificationCount = g_warmupCountdownTime.modificationCount;
}

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars( void ) {
	int			i;
	cvarTable_t* cv;
	qboolean remapped = qfalse;

	for ( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ ) {
		if ( cv->vmCvar ) {
			trap_Cvar_Update( cv->vmCvar );

			if ( cv->modificationCount != cv->vmCvar->modificationCount ) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				if ( cv->gameFlags & GCF_TRACK_CHANGE ) {
					AP( va( "print \"Server: %s changed to %s\n\"",
						cv->cvarName, cv->vmCvar->string ) );
				}

				if ( cv->gameFlags & GCF_TEAM_SHADER ) {
					remapped = qtrue;
				}

				if ( !Q_stricmp( cv->cvarName, "g_gameType" ) ) {
					AP( va( "pcp \"" S_COLOR_CYAN "%s" S_COLOR_WHITE "\n\"", va( "%s", gt[g_gameType.integer].longName ) ) );
					level.newSession = qtrue;
					//G_Printf( "Gametype changed, clearing session data.\n" );

					trap_Cmd_ExecuteText( EXEC_APPEND, "map_restart 0\n" );
					level.changemap = NULL;
					level.intermissiontime = 0;
				}
			}
		}
	}

	if ( remapped ) {
		G_RemapTeamShaders();
	}
}

/*
============
G_InitGame

============
*/
void G_InitGame( int levelTime, int randomSeed, int restart ) {
	int	i, j, initGameType;

	G_DPrintf( "------- Game Initialization -------\n" );
	G_DPrintf( "gameVersion: %s\n", PRODUCT_NAME " " PRODUCT_VERSION " " PLATFORM_STRING " " PRODUCT_DATE );
	G_DPrintf( "gameProtocol: %s\n", GAME_PROTOCOL );

	srand( randomSeed );

	Swap_Init();

	G_BotInitBotLib();

	G_RegisterCvars();

	G_ProcessIPBans();

	// tell server entity and player state size and network field info
	trap_SetNetFields( sizeof( entityState_t ), sizeof( entityState_t ) - sizeof( int ), bg_entityStateFields, bg_numEntityStateFields,
		sizeof( playerState_t ), 0, bg_playerStateFields, bg_numPlayerStateFields );

	// set some level globals
	if ( restart ) initGameType = level.initGameType;
	else initGameType = -1;

	memset( &level, 0, sizeof( level ) );
	level.time = levelTime;
	level.startTime = levelTime;
	level.initGameType = initGameType;
	level.initRestart = restart;

	level.snd_fry = G_SoundIndex( "sound/player/fry.wav" );	// FIXME standing in lava / slime

	gravity_modcount = g_gravity.modificationCount;

	if ( !g_singlePlayerActive.integer && g_logFile.string[0] ) {
		if ( g_logFileSync.integer ) {
			trap_FS_FOpenFile( g_logFile.string, &level.logFile, FS_APPEND_SYNC );
		} else {
			trap_FS_FOpenFile( g_logFile.string, &level.logFile, FS_APPEND );
		}
		if ( !level.logFile ) {
			G_Printf( "WARNING: Couldn't open logfile: %s\n", g_logFile.string );
		} else {
			char	serverinfo[MAX_INFO_STRING];

			trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

			G_LogPrintf( "------------------------------------------------------------\n" );
			G_LogPrintf( "InitGame: %s\n", serverinfo );
		}
	} else {
		G_Printf( "Not logging to disk.\n" );
	}

	G_InitWorldSession();

	G_RegisterCommands();

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof( g_entities[0] ) );
	level.gentities = g_entities;

	// initialize all client connections for this game
	level.maxconnections = g_maxClients.integer;
	memset( g_connections, 0, MAX_CLIENTS * sizeof( g_connections[0] ) );
	level.connections = g_connections;

	// clear local player nums
	for ( i = 0; i < level.maxconnections; i++ ) {
		for ( j = 0; j < MAX_SPLITVIEW; j++ ) {
			level.connections[i].localPlayerNums[j] = -1;
		}
	}

	// initialize all players for this game
	level.maxplayers = g_maxClients.integer;
	memset( g_players, 0, MAX_CLIENTS * sizeof( g_players[0] ) );
	level.players = g_players;

	// setup player entity fields
	for ( i = 0; i < level.maxplayers; i++ ) {
		g_entities[i].player = level.players + i;
	}
	CountPopulation();

	// always leave room for the max number of players,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but players
	level.num_entities = MAX_CLIENTS;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		g_entities[i].classname = "playerslot";
	}

	// let the server system know where the entites are
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ),
		&level.players[0].ps, sizeof( level.players[0] ) );

	// reserve some spots for dead player bodies
	InitBodyQueue();

	// set score limit
	{
		int scorelimit = 20;
		int timelimit = 0;

		switch ( gt[g_gameType.integer].gtGoal ) {
		case GTL_FRAGS:
			if ( GTF( GTF_TEAMS ) ) {
				scorelimit = gt_teams_frags_limit.integer;
				timelimit = gt_teams_frags_timelimit.integer;
			} else if ( GTF( GTF_DUEL ) ) {
				scorelimit = gt_duel_frags_limit.integer;
				timelimit = gt_duel_frags_timelimit.integer;
			} else {
				scorelimit = gt_frags_limit.integer;
				timelimit = gt_frags_timelimit.integer;
			}
			break;
		case GTL_CAPTURES:
			if ( GTF( GTF_TEAMS ) ) {
				scorelimit = gt_teams_captures_limit.integer;
				timelimit = gt_teams_captures_timelimit.integer;
			}
			break;
		case GTL_ELIM:
			scorelimit = gt_elimination_lives_limit.integer;
			timelimit = gt_elimination_timelimit.integer;
			break;
		case GTL_OBJ:
			break;
		case GTL_POINTS:
			scorelimit = gt_points_limit.integer;
			timelimit = gt_points_timelimit.integer;
			break;
		case GTL_TIME:
			break;
		default: break;
		}
		if ( scorelimit != g_scoreLimit.integer )
			trap_Cvar_SetValue( "scoreLimit", scorelimit );
		if ( timelimit != g_timeLimit.integer )
			trap_Cvar_SetValue( "timeLimit", timelimit );
	}

	ClearRegisteredItems();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// general initialization
	G_FindTeamedItems();

	level.teams_max = g_teamTotal_max.integer;

	// make sure we have flags for CTF, etc
	level.map_teamBaseCount = 0;
	if ( GTF( GTF_TEAMBASES ) ) {
		int i;

		G_CheckTeamItems();

		for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ ) {
			if ( level.map_teamBaseSpawns & (1 << i) ) {
				level.map_teamBaseCount++;
			} else break; //muff: for now, multiteam support relies on filling consecutive team slots
			// eg: can't have team base spawn count of 2 with green and teal, it must be red and blue
		}

		if ( level.map_teamBaseCount < 2 ) {
			G_Printf( S_COLOR_YELLOW "WARNING: map is not suitable for base gametypes (less than two team bases detected)\n" );
			//multiteam TODO: force change gametype to TDM
		} else if ( level.teams_max > level.map_teamBaseCount ) {
			level.teams_max = level.map_teamBaseCount;
			G_Printf( S_COLOR_YELLOW "WARNING: max teams lowered to %i due to map limitations\n", level.teams_max );
		}
	}

	trap_SetConfigstring( CS_NUMTEAMS, va( "%i", level.teams_max ) );

	SaveRegisteredItems();

	G_DPrintf( "-----------------------------------\n" );

	if ( ( g_singlePlayerActive.integer && !GTF(GTF_TEAMS) && !GTF(GTF_CAMPAIGN) ) || trap_Cvar_VariableIntegerValue( "com_buildScript" ) ) {
		G_ModelIndex( SP_PODIUM_MODEL );
	}

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAISetup( restart );
		BotAILoadMap( restart );
		G_InitBots( restart );
	}

	G_RemapTeamShaders();

	level.overTime = 0;
	trap_SetConfigstring( CS_OVERTIME, va( "%i", level.overTime ) );

	// clear ready players from intermission
	trap_SetConfigstring( CS_PLAYERS_READY, "" );
	trap_SetConfigstring( CS_INTERMISSION, "" );
}



/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame( int restart ) {
	G_DPrintf( "==== ShutdownGame ====\n" );

	if ( level.logFile ) {
		G_LogPrintf( "ShutdownGame:\n" );
		G_LogPrintf( "------------------------------------------------------------\n" );
		trap_FS_FCloseFile( level.logFile );
		level.logFile = 0;
	}

	// write all the player session data so we can get it back
	G_WriteSessionData();

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAIShutdown( restart );
	}
}


/*
=================
G_SnapshotCallback
=================
*/
qboolean G_SnapshotCallback( int entityNum, int playerNum ) {
	gentity_t* ent;

	ent = &g_entities[entityNum];

	if ( ent->snapshotCallback ) {
		return ent->snapshotCallback( ent, &g_entities[playerNum] );
	}

	return qtrue;
}

/*
=================
G_VidRestart

Model handles are no longer valid, re-register all models.
=================
*/
void G_VidRestart( void ) {

}

/*
=================
G_FullMapRestart

Check if full map restart is needed for some cvars to take affect.
=================
*/
qboolean G_NeedFullMapRestart( void ) {
	int			i;
	cvarTable_t* cv;
	char		value[MAX_CVAR_VALUE_STRING];
	char		latched[MAX_CVAR_VALUE_STRING];

	for ( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ ) {
		if ( cv->gameFlags & GCF_DO_RESTART ) {
			trap_Cvar_VariableStringBuffer( cv->cvarName, value, sizeof( value ) );
			trap_Cvar_LatchedVariableStringBuffer( cv->cvarName, latched, sizeof( latched ) );

			if ( strcmp( value, latched ) ) {
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
=================
G_MapRestart

This is called by the server when map_restart command is issued and when restart time is hit.

return restart time, -1 to do a full map reload, or INT_MAX to prevent restart.
=================
*/
int G_MapRestart( int levelTime, int restartTime ) {
	int			delay;
	char		buf[12];

	if ( trap_Argc() > 1 ) {
		trap_Argv( 1, buf, sizeof( buf ) );
		delay = atoi( buf );
	} else if ( g_doWarmup.integer ) {
		// warmup delays using g_warmupCountdownTime after map restart
		delay = 0;
	} else {
		delay = 5;
	}

	// restart time hit
	if ( (restartTime && levelTime >= restartTime) || (!restartTime && delay <= 0) ) {
		if ( G_NeedFullMapRestart() ) {
			return -1;
		}

		return 0;
	}

	// don't let user change restart time
	if ( restartTime ) {
		return restartTime;
	}

	restartTime = levelTime + delay * 1000;

	trap_SetConfigstring( CS_WARMUP, va( "%i", restartTime ) );
	return restartTime;
}

//===================================================================

void QDECL Com_Error( int level, const char* error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start( argptr, error );
	Q_vsnprintf( text, sizeof( text ), error, argptr );
	va_end( argptr );

	trap_Error( text );
}

void QDECL Com_Printf( const char* msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	trap_Print( text );
}

void QDECL Com_DPrintf( const char* msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	G_DPrintf( "%s", text );
}

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
Tournament_RemoveFromQueue

Remove a client from the queue when he joins the game or leaves server, update queue accordingly
=============
*/
void Tournament_RemoveFromQueue( gplayer_t* player, const qboolean silent ) {
	if ( !GTF( GTF_DUEL ) ) return;

	if ( !player ) return;
	if ( !player->sess.queued ) return;

	player->sess.queued = qfalse;
	player->sess.queueNum = 0;
	PlayerUserinfoChanged( (int)(player - level.players) );
	if ( !silent )
		trap_SendServerCommand( (int)(player - level.players), "cp \"You are no longer in the queue.\n\"" );
	level.numQueuedPlayers--;
}


/*
=============
Tournament_AddToQueue

Add a client to the queue
=============
*/
void Tournament_AddToQueue( gplayer_t* player, const qboolean silent ) {

	if ( !GTF( GTF_DUEL ) ) return;
	if ( !player ) return;
	if ( player->sess.queued ) return;

	player->sess.queued = qtrue;
	player->sess.queueNum = level.tourneyQueueEnd;
	PlayerUserinfoChanged( (int)(player - level.players) );
	if ( !silent )
		trap_SendServerCommand( (int)(player - level.players), "cp \"You are in the queue to play.\n\"" );
	//G_Printf( "Tournament_AddToQueue: num=%i\n", level.tourneyQueueEnd );
	level.tourneyQueueEnd = (level.tourneyQueueEnd + 1) % MAX_CLIENTS;
	trap_SetConfigstring( CS_TOURNEY_QUEUEINDEX, va( "%i", level.tourneyQueueEnd ) );
	level.numQueuedPlayers++;
}


/*
=============
Tournament_AddPlayer

Add the next waiting client to play
=============
*/
void Tournament_AddPlayer( void ) {
	int			i;
	gplayer_t* player, *nextInLine;

	if ( level.numPlayingPlayers >= 2 ) return;
	if ( !level.numQueuedPlayers ) return;
	if ( level.intermissiontime ) return;

	player = NULL;
	nextInLine = NULL;
	for ( i = level.tourneyQueueEnd; i < (level.tourneyQueueEnd + MAX_CLIENTS); i++ ) {
		player = g_entities[i % MAX_CLIENTS].player;
		if ( !player ) continue;
		if ( player->pers.connected != CON_CONNECTED ) continue;
		if ( player->sess.sessionTeam != TEAM_SPECTATOR ) continue;
		if ( player->sess.queued ) {
			nextInLine = player;
			break;
		}
	}

	if ( !nextInLine ) return;
	G_Printf( "Tournament_AddPlayer: playerNum=%i queued=%i queueNum=%i\n",
		(int)(nextInLine - level.players), nextInLine->sess.queued, nextInLine->sess.queueNum );

	level.warmupTime = -1;
	nextInLine->sess.queued = qfalse;
	nextInLine->sess.queueNum = 0;
	Tournament_RemoveFromQueue( nextInLine, qtrue );
	SetTeam( &g_entities[(int)(nextInLine - level.players)], "f", qfalse );	//player - level.players
}


/*
=======================
Tournament_RemoveLoser

Make the loser a spectator at the back of the line
=======================
*/
void Tournament_RemoveLoser( void ) {
	gentity_t* ent;

	if ( level.numPlayingPlayers != 2 ) return;

	ent = &g_entities[level.sortedPlayers[1]];

	if ( ent->player->pers.connected != CON_CONNECTED ) {
		return;
	}

	SetTeam( ent, "q", qfalse );
}

/*
=======================
Tournament_RemoveWinner
=======================
*/
void Tournament_RemoveWinner( void ) {
	gentity_t* ent;

	if ( level.numPlayingPlayers != 2 ) return;

	ent = &g_entities[level.sortedPlayers[0]];

	if ( ent->player->pers.connected != CON_CONNECTED ) {
		return;
	}

	SetTeam( ent, "q", qfalse );
}

/*
=======================
Tournament_AdjustScores
=======================
*/
void Tournament_AdjustScores( void ) {
	int			playerNum;

	playerNum = level.sortedPlayers[0];
	if ( level.players[playerNum].pers.connected == CON_CONNECTED ) {
		level.players[playerNum].sess.wins++;
		PlayerUserinfoChanged( playerNum );
	}

	playerNum = level.sortedPlayers[1];
	if ( level.players[playerNum].pers.connected == CON_CONNECTED ) {
		level.players[playerNum].sess.losses++;
		PlayerUserinfoChanged( playerNum );
	}

}

/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const void* a, const void* b ) {
	gplayer_t* ca, * cb;

	ca = &level.players[*(int*)a];
	cb = &level.players[*(int*)b];

	// sort special players last
	if ( ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorPlayer < 0 ) {
		return 1;
	}
	if ( cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorPlayer < 0 ) {
		return -1;
	}

	// then connecting players
	if ( ca->pers.connected == CON_CONNECTING ) {
		return 1;
	}
	if ( cb->pers.connected == CON_CONNECTING ) {
		return -1;
	}


	// then spectators
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( ca->sess.queueNum < cb->sess.queueNum ) {
			return -1;
		}
		if ( ca->sess.queueNum > cb->sess.queueNum ) {
			return 1;
		}
		return 0;
	}
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR ) {
		return 1;
	}
	if ( cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		return -1;
	}

	// then sort by score
	if ( ca->ps.persistant[PERS_SCORE]
		> cb->ps.persistant[PERS_SCORE] ) {
		return -1;
	}
	if ( ca->ps.persistant[PERS_SCORE]
		< cb->ps.persistant[PERS_SCORE] ) {
		return 1;
	}
	return 0;
}


/*
============
SortTeamRanks

Sorts all team scores in descending order

muff: my own sorting function until a more efficient method is found
============
*/

void SortTeamRanks( void ) {
	int		i, high;
	int		ts[TEAM_NUM_TEAMS];
	int		sort;

	// load array
	for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ ) {
		ts[i] = (i > level.teams_max) ? SCORE_NOT_PRESENT : level.teamScores[i];
		//G_Printf( "SortTeamRanks: ts[%i] = %i\n", i, ts[i] );
	}

	// clear sorted array
	for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ )
		level.sortedTeams[i] = 0;

	sort = FIRST_TEAM;
	while ( sort < TEAM_NUM_TEAMS ) {
		high = SCORE_NOT_PRESENT;
		// find highest score value
		for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ ) {
			if ( ts[i] == SCORE_NOT_PRESENT ) continue;
			if ( high < ts[i] ) high = ts[i];
		}

		// no more to sort
		if ( high == SCORE_NOT_PRESENT ) break;

		// save highest team(s) with highest score
		for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ ) {
			if ( ts[i] == SCORE_NOT_PRESENT ) continue;
			if ( ts[i] < high ) continue;

			ts[i] = SCORE_NOT_PRESENT;
			level.sortedTeams[sort] = i;
			//G_Printf( "SortTeamRanks: level.sortedTeams[%i] = %i\n", sort, level.sortedTeams[sort] );
			sort++;
		}
	}

}


/*
============
SendTeamRanks

============
*/
void SendTeamRanks( void ) {
	char* s = va( "%1i%1i%1i%1i%1i%1i", level.sortedTeams[1], level.sortedTeams[2], level.sortedTeams[3],
		level.sortedTeams[4], level.sortedTeams[5], level.sortedTeams[6] );
#if 0
	char s[16];
	int i;

	for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ ) {
		Q_strcat( s, sizeof( s ), va( "%1i", level.sortedTeams[i] ) );
		//G_Printf( "SendTeamRanks: s = %s\n", s );
	}
	trap_SetConfigstring( CS_SORTEDTEAMS, s );
#endif
	trap_SetConfigstring( CS_SORTEDTEAMS, s );
	//G_Printf( "SendTeamRanks: s = %s\n", s );
}


/*
============
CountPopulation

Creates info on player load - should be run every time a client connects, begins, and changes teams
============
*/
void CountPopulation( void ) {
	int i;
	gplayer_t* cl;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedPlayers = 0;
	level.numNonSpectatorPlayers = 0;
	level.numPlayingPlayers = 0;
	level.numVotingPlayers = 0;		// don't count bots
	level.numQueuedPlayers = 0;

	for ( i = 0; i < level.maxplayers; i++ ) {
		cl = &level.players[i];
		if ( cl->pers.connected != CON_DISCONNECTED ) {
			level.sortedPlayers[level.numConnectedPlayers] = i;
			level.numConnectedPlayers++;

			if ( cl->sess.sessionTeam != TEAM_SPECTATOR ) {
				level.numNonSpectatorPlayers++;

				// decide if this should be auto-followed
				if ( cl->pers.connected == CON_CONNECTED ) {
					level.numPlayingPlayers++;
					if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
						level.numVotingPlayers++;
					}
					if ( level.follow1 == -1 ) {
						level.follow1 = i;
					} else if ( level.follow2 == -1 ) {
						level.follow2 = i;
					}
				}
			} else if ( GTF( GTF_DUEL ) && cl->sess.queued ) {
				level.numQueuedPlayers++;
			}
		}
	}

	if ( GTF( GTF_TEAMS ) ) {
		int numTeams, shortTeams, numShortTeams, tmin, tmax;
		int tps[TEAM_NUM_TEAMS];

		numTeams = shortTeams = numShortTeams = 0;
		tmin = tmax = -1;

		for ( i = FIRST_TEAM; i <= level.teams_max; i++ ) {
			tps[i] = TeamCount( -1, i );
			if ( tps[i] > 0 )
				numTeams++;
			if ( tps[i] < g_teamSize_min.integer ) {
				shortTeams |= (1 << i);
				numShortTeams++;
			}
			if ( tmax < 0 || tmax < tps[i] ) tmax = level.numTeamPlayers[i];
			if ( tmin < 0 || tmin > tps[i] ) tmin = level.numTeamPlayers[i];
		}
		memcpy( level.numTeamPlayers, tps, sizeof( level.numTeamPlayers ) );
		level.shortTeams = shortTeams;
		level.numShortTeams = numShortTeams;
		level.numPlayingTeams = numTeams;
		level.smallestTeamCount = tmin;
		level.largestTeamCount = tmax;
	}
}


/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every player connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void ) {
	int		i;
	int		rank;
	int		score;
	int		newScore;
	gplayer_t* cl;

	qsort( level.sortedPlayers, level.numConnectedPlayers,
		sizeof( level.sortedPlayers[0] ), SortRanks );

	// set the rank value for all players that are connected and not spectators
	if ( GTF( GTF_TEAMS ) ) {
		int j;

		SortTeamRanks();
#if 0
		qsort( level.sortedTeams, TOTAL_TEAMS,
			sizeof( level.sortedTeams ) - FIRST_TEAM, SortTeamRanks );
#endif
		SendTeamRanks();

		// in team games, rank is number the team is ranking in score
		//multiteam FIXME tied teams not considered yet
		for ( i = 0; i < level.numConnectedPlayers; i++ ) {
			cl = &level.players[level.sortedPlayers[i]];

			for ( j = FIRST_TEAM; j < TEAM_NUM_TEAMS; j++ ) {
				if ( cl->sess.sessionTeam == level.sortedTeams[j] ) {
					cl->ps.persistant[PERS_RANK] = j;
				}
			}
		}

#if 0
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for ( i = 0; i < level.numConnectedPlayers; i++ ) {
			cl = &level.players[level.sortedPlayers[i]];
			if ( level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 2;
			} else if ( level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 0;
			} else {
				cl->ps.persistant[PERS_RANK] = 1;
			}
		}
#endif
	} else {
		rank = -1;
		score = 0;
		for ( i = 0; i < level.numPlayingPlayers; i++ ) {
			cl = &level.players[level.sortedPlayers[i]];
			newScore = cl->ps.persistant[PERS_SCORE];
			if ( i == 0 || newScore != score ) {
				rank = i;
				// assume we aren't tied until the next player is checked
				level.players[level.sortedPlayers[i]].ps.persistant[PERS_RANK] = rank;
			} else {
				// we are tied with the previous player
				level.players[level.sortedPlayers[i - 1]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.players[level.sortedPlayers[i]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if ( g_singlePlayerActive.integer && level.numPlayingPlayers == 1 ) {
				level.players[level.sortedPlayers[i]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if ( GTF( GTF_TEAMS ) ) {
		for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ ) {
			if ( i > level.teams_max ) {
				trap_SetConfigstring( CS_SCORES1 + i - FIRST_TEAM, va( "%i", SCORE_NOT_PRESENT ) );
			} else {
				trap_SetConfigstring( CS_SCORES1 + i - FIRST_TEAM, va( "%i", level.teamScores[i] ) );
			}
		}
	} else {
		if ( level.numConnectedPlayers == 0 ) {
			trap_SetConfigstring( CS_SCORES1, va( "%i", SCORE_NOT_PRESENT ) );
			trap_SetConfigstring( CS_SCORES2, va( "%i", SCORE_NOT_PRESENT ) );
		} else if ( level.numConnectedPlayers == 1 ) {
			trap_SetConfigstring( CS_SCORES1, va( "%i", level.players[level.sortedPlayers[0]].ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SCORES2, va( "%i", SCORE_NOT_PRESENT ) );
		} else {
			trap_SetConfigstring( CS_SCORES1, va( "%i", level.players[level.sortedPlayers[0]].ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SCORES2, va( "%i", level.players[level.sortedPlayers[1]].ps.persistant[PERS_SCORE] ) );
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission, send the new info to everyone
	if ( level.intermissiontime ) {
		SendScoreboardMessageToAllClients();
	}
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients( void ) {
	int		i;

	for ( i = 0; i < level.maxplayers; i++ ) {
		if ( level.players[i].pers.connected == CON_CONNECTED ) {
			DeathmatchScoreboardMessage( g_entities + i );
		}
	}
}

/*
========================
MovePlayerToIntermission

When the intermission starts, this will be called for all players.
If a new player connects, this will be called after the spawn function.
========================
*/
void MovePlayerToIntermission( gentity_t* ent ) {
	// take out of follow mode if needed
	if ( ent->player->sess.spectatorState == SPECTATOR_FOLLOW ) {
		StopFollowing( ent );
	}

	FindIntermissionPoint();
	// move to the spot
	VectorCopy( level.intermission_origin, ent->s.origin );
	VectorCopy( level.intermission_origin, ent->player->ps.origin );
	VectorCopy( level.intermission_angle, ent->player->ps.viewangles );
	ent->player->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset( ent->player->ps.powerups, 0, sizeof( ent->player->ps.powerups ) );

	ent->player->ps.eFlags = 0;
	ent->player->ps.contents = 0;
	ent->s.eFlags = 0;
	ent->s.contents = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint( void ) {
	gentity_t* ent, * target;
	vec3_t		dir;

	// find the intermission spot
	ent = G_Find( NULL, FOFS( classname ), "info_player_intermission" );
	if ( !ent ) {	// the map creator forgot to put in an intermission point...
		SelectSpawnPoint( vec3_origin, level.intermission_origin, level.intermission_angle, qfalse );
	} else {
		VectorCopy( ent->s.origin, level.intermission_origin );
		VectorCopy( ent->s.angles, level.intermission_angle );
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( target->s.origin, level.intermission_origin, dir );
				vectoangles( dir, level.intermission_angle );
			}
		}
	}

}

/*
==================
BeginIntermission
==================
*/
void BeginIntermission( void ) {
	int			i;
	gentity_t* player;

	if ( level.intermissiontime ) {
		return;		// already active
	}

	// if in tournament mode, change the wins / losses
	if ( GTF( GTF_DUEL ) ) {
		Tournament_AdjustScores();
	}

	level.intermissiontime = level.time;
	// move all players to the intermission point
	for ( i = 0; i < level.maxplayers; i++ ) {
		player = g_entities + i;
		if ( !player->inuse )
			continue;
		// respawn if dead
		if ( player->health <= 0 ) {
			PlayerRespawn( player );
		}
		MovePlayerToIntermission( player );
		trap_UnlinkEntity( player );
	}
#ifdef MISSIONPACK
	if ( g_singlePlayerActive.integer ) {
		trap_Cvar_SetValue( "ui_singlePlayerActive", 0 );
		SendSPPostGameInfo();
	}
#else
	// if single player game
	if ( g_singlePlayerActive.integer && !GTF( GTF_TEAMS ) && !GTF( GTF_CAMPAIGN ) ) {
		SendSPPostGameInfo();
		SpawnModelsOnVictoryPads();
	}
#endif
	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();

}


/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar

=============
*/
void ExitLevel( void ) {
	int		i;
	gplayer_t* cl;
	char nextmap[MAX_STRING_CHARS];
	char d1[MAX_STRING_CHARS];

	//bot interbreeding
	BotInterbreedEndMatch();

	// if we are running a tournament map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if ( GTF( GTF_DUEL ) ) {
		if ( !level.restarted ) {
			Tournament_RemoveLoser();
			trap_Cmd_ExecuteText( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			level.changemap = NULL;
			level.intermissiontime = 0;
		}
		return;
	}

	trap_Cvar_VariableStringBuffer( "nextmap", nextmap, sizeof( nextmap ) );
	trap_Cvar_VariableStringBuffer( "d1", d1, sizeof( d1 ) );

	if ( !Q_stricmp( nextmap, "map_restart 0" ) && Q_stricmp( d1, "" ) ) {
		trap_Cvar_Set( "nextmap", "vstr d2" );
		trap_Cmd_ExecuteText( EXEC_APPEND, "vstr d1\n" );
	} else {
		trap_Cmd_ExecuteText( EXEC_APPEND, "vstr nextmap\n" );
	}

	level.changemap = NULL;
	level.intermissiontime = 0;

	// reset all the scores so we don't enter the intermission again
	if ( GTF( GTF( GTF_TEAMS ) ) ) {
		for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ ) {
			level.teamScores[i] = 0;
		}
	}

	for ( i = 0; i < g_maxClients.integer; i++ ) {
		cl = level.players + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.persistant[PERS_SCORE] = 0;
	}

	// we need to do this here before changing to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for ( i = 0; i < g_maxClients.integer; i++ ) {
		if ( level.players[i].pers.connected == CON_CONNECTED ) {
			level.players[i].pers.connected = CON_CONNECTING;
		}
	}

}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf( const char* fmt, ... ) {
	va_list		argptr;
	char		string[1024];
	int			min, tens, sec;

	sec = (level.time - level.startTime) / 1000;

	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	Com_sprintf( string, sizeof( string ), "%3i:%i%i ", min, tens, sec );

	va_start( argptr, fmt );
	Q_vsnprintf( string + 7, sizeof( string ) - 7, fmt, argptr );
	va_end( argptr );

	if ( g_dedicated.integer ) {
		G_Printf( "%s", string + 7 );
	}

	if ( !level.logFile ) {
		return;
	}

	trap_FS_Write( string, strlen( string ), level.logFile );
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit( const char* string ) {
	int				i, numSorted;
	gplayer_t* cl;
#ifdef MISSIONPACK
	//multiteam TODO
	qboolean won = qtrue;
	team_t team = FIRST_TEAM;
#endif
	G_LogPrintf( "Exit: %s\n", string );

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring( CS_INTERMISSION, "1" );

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedPlayers;
	if ( numSorted > 32 ) {
		numSorted = 32;
	}

	if ( GTF( GTF_TEAMS ) ) {
		char str[1024];
		int i;
		for ( i = FIRST_TEAM; i <= level.teams_max; i++ ) {
			Q_strcat( str, sizeof( str ), va( "%s:%i ", g_teamNamesLower[i], level.teamScores[i] ) );
		}

		G_LogPrintf( "%s\n", str );
	}

	for ( i = 0; i < numSorted; i++ ) {
		int		ping;

		cl = &level.players[level.sortedPlayers[i]];

		if ( cl->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		if ( cl->pers.connected == CON_CONNECTING ) {
			continue;
		}

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		G_LogPrintf( "score: %i  ping: %i  player: %i %s\n", cl->ps.persistant[PERS_SCORE], ping, level.sortedPlayers[i], cl->pers.netname );
#ifdef MISSIONPACK
		if ( g_singlePlayerActive.integer && !(g_entities[cl - level.players].r.svFlags & SVF_BOT) ) {
			team = cl->sess.sessionTeam;
		}
		if ( g_singlePlayerActive.integer && !GTF( GTF_TEAMS ) ) {
			if ( g_entities[cl - level.players].r.svFlags & SVF_BOT && cl->ps.persistant[PERS_RANK] == 0 ) {
				won = qfalse;
			}
		}
#endif

	}

#ifdef MISSIONPACK
	if ( g_singlePlayerActive.integer ) {
		if ( GTF( GTF_TEAMS ) ) {

			if ( GTF( GTF_TEAMS ) ) {
				int i, topScore = 0;	// , topTeam = -1;

				for ( i = FIRST_TEAM; i <= level.teams_max; i++ ) {
					if ( i > FIRST_TEAM && level.teamScores[i] > topScore ) {
						topScore = level.teamScores[i];
						//topTeam = i;
					}
				}

				won = (team == i);
			}
		}
		trap_Cmd_ExecuteText( EXEC_APPEND, (won) ? "spWin\n" : "spLose\n" );
	}
#endif

}


/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit( void ) {
	int			ready, notReady, playerCount;
	int			i;
	gplayer_t* cl;
	clientList_t	readyList;

	if ( g_singlePlayerActive.integer ) return;

	// see which players are ready
	ready = notReady = 0;
	playerCount = 0;
	Com_ClientListClear( &readyList );
	for ( i = 0; i < g_maxClients.integer; i++ ) {
		cl = level.players + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( g_entities[i].r.svFlags & SVF_BOT ) {
			continue;
		}
		//muff: exclude spectators from readying
		if ( cl->sess.sessionTeam == TEAM_SPECTATOR )
			continue;

		playerCount++;
		if ( cl->readyToExit ) {
			ready++;
			Com_ClientListAdd( &readyList, i );
		} else {
			notReady++;
		}
	}

	// update configstring so it can be displayed on the scoreboard
	trap_SetConfigstring( CS_PLAYERS_READY, Com_ClientListString( &readyList ) );

	// never exit in less than five seconds
	if ( level.time < level.intermissiontime + 5000 ) {
		return;
	}

	// only test ready status when there are real players present
	if ( playerCount > 0 ) {
		// if nobody wants to go, clear timer
		if ( !ready ) {
			level.readyToExit = qfalse;
			return;
		}

		// if everyone wants to go, go now
		if ( !notReady ) {
			ExitLevel();
			return;
		}
	}

	// the first person to ready starts the ten second timeout
	if ( !level.readyToExit ) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	//muff: force exit after a certain time
	} else if ( g_intermissionForceExitTime.integer && level.intermissiontime + (g_intermissionForceExitTime.integer * 1000) < level.time ) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if ( level.time < level.exitTime + 10000 ) {
		return;
	}

	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied( void ) {
	int		a, b;

	if ( level.numPlayingPlayers < 2 ) {
		return qfalse;
	}

	if ( GTF( GTF_TEAMS ) ) {
		int i, topScore = SCORE_NOT_PRESENT;

		for ( i = FIRST_TEAM; i <= level.teams_max; i++ ) {
			if ( level.teamScores[i] > topScore ) {
				topScore = level.teamScores[i];
			}
		}

		for ( i = FIRST_TEAM; i <= level.teams_max; i++ ) {
			if ( level.teamScores[i] > topScore ) {
				return qfalse;
			}
		}
	}

	a = level.players[level.sortedPlayers[0]].ps.persistant[PERS_SCORE];
	b = level.players[level.sortedPlayers[1]].ps.persistant[PERS_SCORE];

	return a == b;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
void CheckExitRules( void ) {
	int			i;
	gplayer_t* cl;
	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if ( level.intermissiontime ) {
		CheckIntermissionExit();
		return;
	}

	if ( level.intermissionQueued ) {
#ifdef MISSIONPACK
		int time = (g_singlePlayerActive.integer) ? SP_INTERMISSION_DELAY_TIME : INTERMISSION_DELAY_TIME;
		if ( level.time - level.intermissionQueued >= time ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
#else
		if ( level.time - level.intermissionQueued >= INTERMISSION_DELAY_TIME ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
#endif
		return;
	}

	// campaigns don't consider score
	if ( GTF( GTF_CAMPAIGN ) ) {
		return;
	}

	// don't do during warmup
	if ( level.warmupTime < 0 ) {
		return;
	}

	// give a 1 sec delay at start before exiting is considered
	if ( level.time - level.startTime < 1000 ) {
		return;
	}

	if ( GTF( GTF_TEAMS ) && level.numPlayingTeams < 2 ) {
		int i;

		for ( i = 0; i <= level.teams_max; i++ ) {
			if ( !(level.shortTeams & (1 << (i + FIRST_TEAM))) ) {
				if ( level.warmupTime > 0 ) {
					level.warmupTime = -1;
					level.warmupState = WARMUP_DEFAULT;
					AP( "print \"Countdown cancelled: not enough teams\n\"" );
				} else {
					AP( va( "print \"%s wins!" S_COLOR_GREY " (Last remaining team)\n\"", G_PlayerTeamName( level.sortedTeams[1] ) ) );
					LogExit( "Not enough teams left." );
				}
				return;
			}
		}
	} else if ( level.numPlayingPlayers < 2 ) {
		int i;

		for ( i = 0; i <= level.maxplayers; i++ ) {
			cl = level.players + i;
			if ( cl->pers.connected != CON_CONNECTED ) continue;
			if ( cl->sess.sessionTeam == TEAM_FREE ) {
				if ( level.warmupTime > 0 ) {
					level.warmupTime = -1;
					level.warmupState = WARMUP_DEFAULT;
					AP( "print \"Countdown cancelled: not enough players\n\"" );
				} else {
					AP( va( "print \"%s wins!" S_COLOR_GREY " (Last remaining player)\n\"", PlayerName( cl->pers ) ) );
					LogExit( "Not enough players left." );
				}
				return;
			}
		}
	}

	if ( level.warmupTime > 0 ) {
		return;
	}

	// check for sudden death
	if ( ScoreIsTied() ) {
		// always wait for sudden death
		if ( GTF(GTF_DUEL) && g_timeLimit.integer
				&& (level.time - level.startTime >= (g_timeLimit.integer * 60000) + level.overTime) ) {
			level.overTime += g_overTime.integer * 60000;
			AP( va( "pcp \"" S_COLOR_CREAM "Overtime! " S_COLOR_WHITE "%i " S_COLOR_CREAM "minute%s added.\n\"", g_overTime.integer, g_overTime.integer == 1 ? "" : "s" ) );
			trap_SetConfigstring( CS_OVERTIME, va( "%i", level.overTime ) );
		}
		return;
	}

	if ( g_timeLimit.integer && !level.warmupTime ) {
		if ( level.time - level.startTime >= (g_timeLimit.integer * 60000) + level.overTime ) {
			if ( GTF( GTF_TEAMS ) ) {
				AP( va( "print \"%s wins!" S_COLOR_GREY " (Time Limit hit)\n\"", G_PlayerTeamName( level.sortedTeams[1] ) ) );
			} else {
				AP( va( "print \"%s wins!" S_COLOR_GREY " (Time Limit hit)\n\"", PlayerName( g_entities[level.sortedPlayers[0]].player->pers ) ) );
			}
			LogExit( "Time Limit hit." );
			return;
		}
	}
	
	if ( GTL( GTL_FRAGS ) && g_scoreLimit.integer ) {
		if ( GTF( GTF_TEAMS ) ) {
			if ( level.teamScores[level.sortedTeams[1]] >= g_scoreLimit.integer ) {
				AP( va( "print \"%s " S_COLOR_CREAM "wins!" S_COLOR_GREY " (hit the Frag Limit)\n\"", G_PlayerTeamName( level.sortedTeams[1] ) ) );
				LogExit( "Frag Limit hit." );
				return;
			}
			if ( gt_teams_frags_mercylimit.integer > 0
					&& level.teamScores[level.sortedTeams[1]] >= (level.teamScores[level.sortedTeams[2]] + gt_teams_frags_mercylimit.integer) ) {
				AP( va( "print \"%s " S_COLOR_CREAM "wins!" S_COLOR_GREY " (hit the Mercy Limit: +%i frags).\n\"", G_PlayerTeamName( level.sortedTeams[1] ), gt_teams_frags_mercylimit.integer ) );
				LogExit( "Mercy Limit hit." );
				return;
			}
		} else {
			for ( i = 0; i < g_maxClients.integer; i++ ) {
				cl = level.players + i;
				if ( cl->pers.connected != CON_CONNECTED ) {
					continue;
				}
				if ( cl->sess.sessionTeam != TEAM_FREE ) {
					continue;
				}

				if ( cl->ps.persistant[PERS_SCORE] >= g_scoreLimit.integer ) {
					AP( va( "print \"%s wins!" S_COLOR_GREY " (hit the Frag Limit)\n\"", PlayerName( cl->pers ) ) );
					LogExit( "Frag Limit hit." );
					return;
				}
			}
		}
	}

	if ( GTF( GTF_TEAMS ) && GTL( GTL_CAPTURES ) && g_scoreLimit.integer ) {
		int i;

		for ( i = FIRST_TEAM; i <= level.teams_max; i++ ) {
			if ( level.teamScores[i] >= g_scoreLimit.integer ) {
				AP( va( "print \"%s " S_COLOR_CREAM "wins!" S_COLOR_GREY " (hit the Capture Limit)\n\"", G_PlayerTeamName( i ) ) );
				LogExit( "Capture Limit hit." );
				return;
			}
		}
	}
#if 0
	// no longer have the required number of players on a team
	if ( GTF( GTF_TEAMS ) && g_doWarmup.integer && !level.warmupTime && level.numPlayingTeams < g_teamTotal_min.integer ) {
		//multiteam TODO: force players to team with no players / alternative server options?
		AP( va( "print \"Match ended: not enough teams left for play.\n\"" ) );
		LogExit( "Match cancelled." );
		return;
	}
#endif
}


/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/


/*
=================
CheckReadyStart

Handles conditions to exit warmup stage and progress into the match.
Considers player ready statuses and communicates them to all clients for scoreboard drawing.
=================
*/
qboolean CheckReadyStart( void ) {
	gplayer_t	*cl;
	int			ready, playerCount, botCount;
	int			readyMask;
	int			i;

	// stop if in mission mode or single player
	if ( GTF(GTF_CAMPAIGN) || g_singlePlayerActive.integer ) return qtrue;
	// don't check ready players when disabled
	if ( !g_doReady.integer ) return qtrue;

	//G_Printf( "CheckReadyStart start\n" );
	// see which players are ready
	ready = 0;
	readyMask = 0;
	playerCount = 0;
	botCount = 0;
	for ( i = 0; i < level.maxplayers; i++ ) {
		cl = level.players + i;
		// must be connected to the match
		if ( cl->pers.connected != CON_CONNECTED ) continue;
		// spectators can fuck riiiight off
		if ( cl->sess.sessionTeam == TEAM_SPECTATOR ) continue;
		// bots are handled differently
		if ( g_entities[cl->ps.playerNum].r.svFlags & SVF_BOT ) {
			botCount++;
			continue;
		}

		playerCount++;
		if ( cl->pers.readyToBegin ) {
			ready++;
			if ( i < level.maxplayers ) {
				readyMask |= (1 << i);
			}
		}
	}

	// if no human players, only bots, then we're ready
	if ( !playerCount && botCount ) return qtrue;
	// if there's no players or bots then not ready
	if ( !playerCount && !botCount ) return qfalse;
	//G_Printf( "CheckReadyStart: playerCount = %i, botCount = %i, ready = %i\n", playerCount, botCount, ready );

	// copy the readyMask to each player's stats so
	// it can be displayed on the scoreboard
	for ( i = 0; i < level.maxplayers; i++ ) {
		cl = level.players + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
	}

	// check if a sufficient percentage of players are ready to begin
	if ( ready && playerCount )
		if ( ((float)ready / (float)playerCount) > g_warmupReadyPercentage.value ) return qtrue;
	//G_Printf( "CheckReadyStart end\n" );
	return qfalse;
}


/*
=============
PassWarmupDuel

Check if warmup can end in duels
=============
*/
qboolean PassWarmupDuel( void ) {
	// pull in a queued player if needed
	if ( level.numPlayingPlayers < 2 )
		Tournament_AddPlayer();

	// if we don't have two players, go back to "waiting for players"
	level.warmupState = (level.numPlayingPlayers == 2) ? WARMUP_READYUP : WARMUP_DEFAULT;

	if ( level.warmupState == WARMUP_DEFAULT ) {
		level.warmupVal = level.numPlayingPlayers;
		return qfalse;
	}

	// check if players are ready to begin
	if ( !CheckReadyStart() ) return qfalse;

	// ready to begin!
	level.warmupState = WARMUP_COUNTDOWN;
	level.warmupVal = 0;

	return qtrue;
}


/*
=============
PassWarmupGeneric

Sets warmup state and returns true if match should start
=============
*/
qboolean PassWarmupGeneric( void ) {
	int state = -1, val = 0;

	// check there are enough players
	if ( level.numPlayingPlayers < 2 ) {
		state = WARMUP_DEFAULT;
	} else if ( GTF(GTF_TEAMS) ) {
		if ( level.numShortTeams > 1 ) {
		   // multiple teams are short of players - tell clients which teams
			state = WARMUP_DEFAULT;
			val = level.shortTeams;
		} else if ( level.numShortTeams == 1 ) {
			// one team is short of players - tell clients how many are needed
			state = WARMUP_SHORT_TEAMS;
			val = g_teamSize_min.integer - level.smallestTeamCount;
		} else if ( g_teamForceBalance.integer && (level.largestTeamCount - level.smallestTeamCount > 1)  ) {
			G_Printf( "largestTeamCount=%i, smallestTeamCount=%i, numPlayingTeams=%i\n", level.largestTeamCount, level.smallestTeamCount, level.numPlayingTeams );
			// teams have enough players but teams are imbalanced
			state = WARMUP_IMBA;
		}

	}
	level.warmupState = state;
	level.warmupVal = val;

	if ( state != -1 ) return qfalse;

	level.warmupState = WARMUP_READYUP;

	// check if enough players are ready to begin
	if ( !CheckReadyStart() ) return qfalse;

	// ready to begin!
	level.warmupState = WARMUP_COUNTDOWN;

	return qtrue;
}


void WarmupReadyReset( void ) {
	gplayer_t* cl;
	int			i;

	for ( i = 0; i < level.maxplayers; i++ ) {
		cl = level.players + i;

		if ( cl->pers.connected != CON_CONNECTED ) continue;

		cl->ps.stats[STAT_CLIENTS_READY] = 0;
		cl->pers.readyToBegin = qfalse;
	}
	level.warmupTime = -1;
}


/*
=============
CheckWarmup

Once a frame, check for changes in player state during warmup
=============
*/
void CheckWarmup( void ) {
	// stop if match has begun or there is no warmup
	if ( !level.warmupTime ) return;

	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	if ( !level.numPlayingPlayers ) return;

	// if the warmup is changed at the console, restart it
	if ( g_warmupCountdownTime.modificationCount != level.warmupModificationCount ) {
		level.warmupModificationCount = g_warmupCountdownTime.modificationCount;
		level.warmupTime = -1;
		level.warmupState = WARMUP_DEFAULT;
		level.warmupVal = 0;
		trap_SetConfigstring( CS_WARMUP, va( "%i", level.warmupTime ) );
		trap_SetConfigstring( CS_WARMUP_STATE, va( "%i", level.warmupState ) );
		trap_SetConfigstring( CS_WARMUP_VAL, va( "%i", g_warmupDelay.integer * 1000 ) );
	}

	// return if minimum delay hasn't been reached yet
	if ( (g_warmupDelay.integer * 1000) > level.time ) {
		if ( level.warmupState != WARMUP_DELAYED ) {
			level.warmupTime = -1;
			level.warmupState = WARMUP_DELAYED;
			trap_SetConfigstring( CS_WARMUP, va( "%i", level.warmupTime ) );
			trap_SetConfigstring( CS_WARMUP_STATE, va( "%i", level.warmupState ) );
			trap_SetConfigstring( CS_WARMUP_VAL, va( "%i", g_warmupDelay.integer * 1000 ) );
		}
		return;
	}

	// check we have the required players
	if ( GTF( GTF_DUEL ) ) {
		PassWarmupDuel();
	} else {
		PassWarmupGeneric();
	}

	// check for changes in warmup state, and update the config strings accordingly
	if ( level.warmupOldState != level.warmupState ) {

		// check for cancelling countdown
		if ( level.warmupOldState == WARMUP_COUNTDOWN && level.warmupState != WARMUP_COUNTDOWN ) {
			level.warmupTime = -1;
			trap_SetConfigstring( CS_WARMUP, va( "%i", level.warmupTime ) );
		}

		// if we're going backwards from readyup state, reset clients ready statuses
		if ( level.warmupOldState == WARMUP_READYUP && level.warmupState < WARMUP_READYUP ) {
			WarmupReadyReset();
		}

		trap_SetConfigstring( CS_WARMUP_STATE, va( "%i", level.warmupState ) );
		level.warmupOldState = level.warmupState;

		if ( level.warmupOldVal == level.warmupVal ) {
			trap_SetConfigstring( CS_WARMUP_VAL, va( "%i", level.warmupVal ) );
			level.warmupOldVal = level.warmupVal;
		}
		//G_Printf( "CheckWarmup: new state = %i\n", level.warmupState );
	}
	if ( level.warmupState != WARMUP_COUNTDOWN ) return;

	// fudge by -1 to account for extra delays
	if ( level.warmupTime < 0 ) {
		level.warmupTime = level.time + (g_warmupCountdownTime.integer - 1) * 1000;
		trap_SetConfigstring( CS_WARMUP, va( "%i", level.warmupTime ) );
	}

	// if the warmup time has counted down, restart
	if ( level.time > level.warmupTime ) {
		level.warmupTime += g_warmupCountdownTime.integer;
		level.warmupState = WARMUP_DEFAULT;
		trap_Cvar_Set( "g_restarted", "1" );
		trap_Cmd_ExecuteText( EXEC_APPEND, "map_restart 0\n" );
		level.restarted = qtrue;
		level.changemap = NULL;	//muff: don't change map here
	}
}


/*
==================
CheckVote
==================
*/
void CheckVote( void ) {
	if ( level.voteExecuteTime && level.voteExecuteTime < level.time ) {
		level.voteExecuteTime = 0;
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "%s\n", level.voteString ) );
	}
	if ( !level.voteTime ) {
		return;
	}
	
	if ( level.time - level.voteTime >= VOTE_TIME ) {
		AP( "print \"Vote failed.\n\"" );
	} else {
		// ATVI Q3 1.32 Patch #9, WNF
		if ( level.voteYes > level.numVotingPlayers / 2 ) {
			// execute the command, then remove the vote
			AP( "print \"Vote passed.\n\"" );
			level.voteExecuteTime = level.time + 3000;
		} else if ( level.voteNo >= level.numVotingPlayers / 2 ) {
			// same behavior as a timeout
			AP( "print \"Vote failed.\n\"" );
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.voteTime = 0;
	trap_SetConfigstring( CS_VOTE_TIME, "" );

}

/*
==================
PrintTeam
==================
*/
void PrintTeam( int team, char* message ) {
	G_TeamCommand( team, message );
}

/*
==================
SetLeader
==================
*/
void SetLeader( int team, int playerNum ) {
	gplayer_t* player;
	int i;

	player = &level.players[playerNum];

	if ( player->pers.connected == CON_DISCONNECTED ) {
		PrintTeam( team, va( "print \"%s is not connected\n\"", PlayerName( player->pers ) ) );
		return;
	}
	if ( player->sess.sessionTeam != team ) {
		PrintTeam( team, va( "print \"%s is not on the team anymore\n\"", PlayerName( player->pers ) ) );
		return;
	}
	for ( i = 0; i < level.maxplayers; i++ ) {
		if ( level.players[i].sess.sessionTeam != team )
			continue;
		if ( level.players[i].sess.teamLeader ) {
			level.players[i].sess.teamLeader = qfalse;
			PlayerUserinfoChanged( i );
		}
	}
	player->sess.teamLeader = qtrue;
	PlayerUserinfoChanged( playerNum );
	//PrintTeam( team, va( "print \"%s is the new team leader.\n\"", PlayerName( player->pers ) ) );
}

/*
==================
CheckTeamLeader
==================
*/
void CheckTeamLeader( int team ) {
	int i;

	for ( i = 0; i < level.maxplayers; i++ ) {
		if ( level.players[i].sess.sessionTeam != team )
			continue;
		if ( level.players[i].sess.teamLeader )
			break;
	}
	if ( i >= level.maxplayers ) {
		for ( i = 0; i < level.maxplayers; i++ ) {
			if ( level.players[i].sess.sessionTeam != team )
				continue;
			if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
				level.players[i].sess.teamLeader = qtrue;
				break;
			}
		}

		if ( i >= level.maxplayers ) {
			for ( i = 0; i < level.maxplayers; i++ ) {
				if ( level.players[i].sess.sessionTeam != team )
					continue;
				level.players[i].sess.teamLeader = qtrue;
				break;
			}
		}
	}
}


/*
==================
CheckCvars
==================
*/
void CheckCvars( void ) {
	static int lastMod = -1;
	static int gameTypeLastMod = -1;

	if ( g_password.modificationCount != lastMod ) {
		lastMod = g_password.modificationCount;
		if ( *g_password.string && Q_stricmp( g_password.string, "none" ) ) {
			trap_Cvar_SetValue( "g_needPassword", 1 );
		} else {
			trap_Cvar_SetValue( "g_needPassword", 0 );
		}
	}

	if ( gameTypeLastMod < 0 ) {
		gameTypeLastMod = g_gameType.modificationCount;
	}

	// invoke gametype change adapting
	if ( g_gameType.modificationCount != gameTypeLastMod ) {

		gameTypeLastMod = g_gameType.modificationCount;
	}
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink( gentity_t* ent ) {
	int	thinktime;

	thinktime = ent->nextthink;
	if ( thinktime <= 0 ) {
		return;
	}
	if ( thinktime > level.time ) {
		return;
	}

	ent->nextthink = 0;
	if ( !ent->think ) {
		G_Error( "NULL ent->think" );
	}
	ent->think( ent );
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void G_RunFrame( int levelTime ) {
	int			i;
	gentity_t* ent;

	// if we are waiting for the level to restart, do nothing
	if ( level.restarted ) {
		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;

	// get any cvar changes
	G_UpdateCvars();

	//
	// go through all allocated objects
	//
	ent = &g_entities[0];
	for ( i = 0; i < level.num_entities; i++, ent++ ) {
		if ( !ent->inuse ) {
			continue;
		}

		// clear events that are too old
		if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) {
			if ( ent->s.event ) {
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if ( ent->player ) {
					ent->player->ps.externalEvent = 0;
					// predicted events should never be set to zero
					//ent->player->ps.events[0] = 0;
					//ent->player->ps.events[1] = 0;
				}
			}
			if ( ent->freeAfterEvent ) {
				// tempEntities or dropped items completely go away after their event
				G_FreeEntity( ent );
				continue;
			} else if ( ent->unlinkAfterEvent ) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap_UnlinkEntity( ent );
			}
		}

		// muff: update push triggers if gravity has changed
		if ( ent->s.eType == ET_PUSH_TRIGGER ) {
			if ( gravity_modcount != g_gravity.modificationCount ) {
				ent->think = AimAtTarget;
				ent->nextthink = level.time + FRAMETIME;
			}
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( !ent->r.linked && ent->neverFree ) {
			continue;
		}

		if ( ent->s.eType == ET_MISSILE ) {
			G_RunMissile( ent );
			continue;
		}

		if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
			G_RunItem( ent );
			continue;
		}

		if ( ent->s.eType == ET_MOVER ) {
			G_RunMover( ent );
			continue;
		}

		if ( i < MAX_CLIENTS ) {
			G_RunPlayer( ent );
			continue;
		}

		G_RunThink( ent );
	}
	gravity_modcount = g_gravity.modificationCount;

	// perform final fixups on the players
	ent = &g_entities[0];
	for ( i = 0; i < level.maxplayers; i++, ent++ ) {
		if ( ent->inuse ) {
			PlayerEndFrame( ent );
		}
	}

	// check for warmup status
	CheckWarmup();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// for tracking changes
	CheckCvars();

	if ( g_listEntity.integer ) {
		for ( i = 0; i < MAX_GENTITIES; i++ ) {
			G_Printf( "%4i: %s\n", i, g_entities[i].classname );
		}
		trap_Cvar_SetValue( "g_listEntity", 0 );
	}

	// record the time at the end of this frame - it should be about
	// the time the next frame begins - when the server starts
	// accepting commands from connected clients
	level.frameStartTime = trap_Milliseconds();
}
