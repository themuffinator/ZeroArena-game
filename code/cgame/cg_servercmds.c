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
// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definitely
// be a valid snapshot this frame

#include "cg_local.h"



void CG_PrintPlayerWeaponStats( void ) {
#if 0
	//char pstr[1024];
	int i;
	playerStats_t *s = &cg.pstats;

	CG_Printf( "Weapon Stats (per weapon):\n" );
	//CG_Printf( "%c%c<weapon>: <dmgD> <dmgR> <DRatio> <shots> <hits> <accuracy>\n\n", Q_COLOR_ESCAPE, COLOR_CREAM );
	CG_Printf( "%c%c<weapon>: <dmgD> <dmgR> <shots> <hits>\n\n", Q_COLOR_ESCAPE, COLOR_CREAM );
	for ( i = WP_MACHINEGUN; i < WP_NUM_WEAPONS; i++ ) {
		if ( WP_GRAPPLING_HOOK ) continue;

		//Com_sprintf( pstr, sizeof(pstr), "%c%c%s%c%c: %i %i %.2f %i %i %.2f", Q_COLOR_ESCAPE, COLOR_YELLOW, weaponNamesShort[i], Q_COLOR_ESCAPE, COLOR_CREAM,
			//s->statsWeaponDmgD[i], s->statsWeaponDmgR[i], s->statsWeaponDamageRatio[i], s->statsWeaponShots[i], s->statsWeaponHits[i], s->statsWeaponAccuracy[i] );
		CG_Printf( "%c%c%s%c%c: %i %i %i %i", Q_COLOR_ESCAPE, COLOR_YELLOW, weaponNamesShort[i], Q_COLOR_ESCAPE, COLOR_CREAM,
			s->statsWeaponDmgD[i], s->statsWeaponDmgR[i], s->statsWeaponShots[i], s->statsWeaponHits[i] );
		//CG_Printf( "%s", pstr );
	}
#endif
#if 0
	CG_Printf( "\n" );
	CG_Printf( "Weapon Stats (totals):\n" );
	CG_Printf( "%c%cShots / Hits: %c%c%i / %i (%.2f %%)\n", Q_COLOR_ESCAPE, COLOR_CREAM, Q_COLOR_ESCAPE, COLOR_YELLOW, s->statsWeaponTotalShots, s->statsWeaponTotalHits, s->statsWeaponTotalAccuracy );
	CG_Printf( "%c%cDamage Delivered / Received: %c%c%i / %i (%.2f %%)\n", Q_COLOR_ESCAPE, COLOR_CREAM, Q_COLOR_ESCAPE, COLOR_YELLOW, s->statsWeaponTotalDmgD, s->statsWeaponTotalDmgR, s->statsWeaponTotalDamageRatio );
#endif
}

/*
=================
CG_ParsePlayerWeaponStats

=================
*/
static void CG_ParsePlayerWeaponStats( int start ) {
#if 0
	int				i, j;
	playerStats_t	*s = &cg.pstats;
	//i = WP_MACHINEGUN;
	j = 1;	// playernum not used for now
	for ( i = WP_MACHINEGUN; i < WP_NUM_WEAPONS; i++ ) {
		if ( i == WP_GRAPPLING_HOOK ) continue;
		s->statsWeaponShots[i] = atoi( CG_Argv( j++ + start ) );
		s->statsWeaponHits[i] = atoi( CG_Argv( j++ + start ) );
		s->statsWeaponDmgD[i] = atoi( CG_Argv( j++ + start ) );
		s->statsWeaponDmgR[i] = atoi( CG_Argv( j++ + start ) );

		//CG_Printf( "weap %i done\n", i );
	}
#endif
#if 0
		s->statsWeaponAccuracy[i] = s->statsWeaponHits[i] * 100 / s->statsWeaponShots[i];

		s->statsWeaponTotalAccuracy += s->statsWeaponAccuracy[i];
		s->statsWeaponTotalDmgD += s->statsWeaponDmgD[i];
		s->statsWeaponTotalDmgR += s->statsWeaponDmgR[i];
		s->statsWeaponTotalShots += s->statsWeaponShots[i];
		s->statsWeaponTotalHits += s->statsWeaponHits[i];

	//}
	s->statsWeaponTotalDamageRatio = (float)s->statsWeaponTotalDmgD / (float)s->statsWeaponTotalDmgR;
	s->statsWeaponTotalAccuracy = (float)s->statsWeaponTotalShots * 100.0f / (float)s->statsWeaponTotalHits;
#endif
	//CG_PrintPlayerWeaponStats();
}


/*
=================
CG_ParseScores

=================
*/
static void CG_ParseScores( int start ) {
	int i, j = 1;

	cg.numScores = atoi( CG_Argv( j++ + start) );
	if ( cg.numScores > MAX_CLIENTS )
		cg.numScores = MAX_CLIENTS;

	for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ )
		cg.teamScores[i] = atoi( CG_Argv( j++ + start ) );

	//CG_Printf( "numscores = %i, red score = %i\n",cg.numScores, cg.teamScores[TEAM_RED] );
	memset( cg.scores, 0, sizeof( cg.scores ) );
	for ( i = 0 ; i < cg.numScores ; i++ ) {
		score_t* s = &cg.scores[i];
		int k = j, powerups;

		s->playerNum = atoi( CG_Argv( i * 15 + k++ + start) );
		s->score = atoi( CG_Argv( i * 15 + k++ + start) );
		s->ping = atoi( CG_Argv( i * 15 + k++ + start) );
		s->time = atoi( CG_Argv( i * 15 + k++ + start) );
		s->scoreFlags = atoi( CG_Argv( i * 15 + k++ + start) );
		powerups = atoi( CG_Argv( i * 15 + k++ + start) );
		s->accuracy = atoi(CG_Argv(i * 15 + k++ + start));
		s->impressiveCount = atoi(CG_Argv(i * 15 + k++ + start));
		s->excellentCount = atoi(CG_Argv(i * 15 + k++ + start));
		s->guantletCount = atoi(CG_Argv(i * 15 + k++ + start));
		s->defendCount = atoi(CG_Argv(i * 15 + k++ + start));
		s->assistCount = atoi(CG_Argv(i * 15 + k++ + start));
		s->perfect = atoi(CG_Argv(i * 15 + k++ + start));
		s->captures = atoi(CG_Argv(i * 15 + k++ + start));
		s->holyShitCount = atoi( CG_Argv( i * 15 + k++ + start ) );

		if ( s->playerNum < 0 || s->playerNum >= MAX_CLIENTS ) {
			s->playerNum = 0;
		}
		cgs.playerinfo[ s->playerNum ].score = s->score;
		cgs.playerinfo[ s->playerNum ].powerups = powerups;

		s->team = cgs.playerinfo[s->playerNum].team;

		//CG_Printf( "player %i score = %i\n", s->playerNum, s->score );
	}
}


/*
=================
CG_ParseTeamInfo

Format:
"tinfo" team numstrings string(there are numstrings strings)

Each string is "playerNum location health armor weapon powerups"

=================
*/
static void CG_ParseTeamInfo( int start ) {
	int		i;
	int		playerNum;
	int		team = atoi( CG_Argv( 1 + start ) );
	
	if ( team < FIRST_TEAM || team >= TEAM_NUM_TEAMS ) {
		CG_Error( "CG_ParseTeamInfo: team out of range (%d)", team );
		return;
	}

	sortedTeamPlayersTime[team] = cg.time;

	numSortedTeamPlayers[team] = atoi( CG_Argv( 2 + start ) );
	if( numSortedTeamPlayers[team] < 0 || numSortedTeamPlayers[team] > TEAM_MAXOVERLAY )
	{
		CG_Error( "CG_ParseTeamInfo: numSortedTeamPlayers out of range (%d)",
				numSortedTeamPlayers[team] );
		return;
	}

	for ( i = 0 ; i < numSortedTeamPlayers[team] ; i++ ) {
		playerNum = atoi( CG_Argv( i * 6 + 3 + start ) );
		if( playerNum < 0 || playerNum >= MAX_CLIENTS )
		{
			CG_Error( "CG_ParseTeamInfo: bad player number: %d", playerNum );
			return;
		}

		sortedTeamPlayers[team][i] = playerNum;

		cgs.playerinfo[ playerNum ].location = atoi( CG_Argv( i * 6 + 4 + start ) );
		cgs.playerinfo[ playerNum ].health = atoi( CG_Argv( i * 6 + 5 + start ) );
		cgs.playerinfo[ playerNum ].armor = atoi( CG_Argv( i * 6 + 6 + start ) );
		cgs.playerinfo[ playerNum ].curWeapon = atoi( CG_Argv( i * 6 + 7 + start ) );
		cgs.playerinfo[ playerNum ].powerups = atoi( CG_Argv( i * 6 + 8 + start ) );
	}
}


/*
================
CG_ParseServerinfo

This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
================
*/
void CG_ParseServerinfo( void ) {
	const char	*info;
	char	*mapname;

	info = CG_ConfigString( CS_SERVERINFO );
	Q_strncpyz( cgs.gametypeName, Info_ValueForKey( info, "sv_gametypeName" ), sizeof (cgs.gametypeName) );
	cgs.gameType = atoi( Info_ValueForKey( info, "g_gameType" ) );
	trap_Cvar_SetValue("g_gameType", cgs.gameType);
	cgs.dmFlags = atoi( Info_ValueForKey( info, "dmFlags" ) );
	cgs.scoreLimit = atoi( Info_ValueForKey( info, "scoreLimit" ) );
	cgs.timeLimit = atoi( Info_ValueForKey( info, "timeLimit" ) );
	cgs.maxplayers = atoi( Info_ValueForKey( info, "sv_maxClients" ) );
	cgs.bots_enabled = atoi( Info_ValueForKey( info, "bot_enable" ) );
	mapname = Info_ValueForKey( info, "mapName" );
	Com_sprintf( cgs.mapname, sizeof( cgs.mapname ), "maps/%s.bsp", mapname );
	cgs.gravity = atoi( Info_ValueForKey( info, "g_gravity" ) );
	cgs.armorRules = atoi( Info_ValueForKey( info, "g_armorRules" ) );
	cgs.teamSizeMin = atoi( Info_ValueForKey( info, "g_teamSize_min" ) );
	cgs.teamSizeMax = atoi( Info_ValueForKey( info, "g_teamSize_max" ) );
	cgs.forceWeaponColors = atoi( Info_ValueForKey( info, "g_forceWeaponColors" ) );
}


/*
==================
CG_ParseWarmup
==================
*/
static void CG_ParseWarmup( void ) {
	const char	*info;
	int			warmup;

	info = CG_ConfigString( CS_WARMUP );

	warmup = atoi( info );
	cg.warmupCount = -1;

	if ( warmup == 0 && cg.warmupTime ) {

	} else if ( warmup > 0 && cg.warmupTime <= 0 ) {
#ifdef MISSIONPACK
		if (GTF(GTF_TEAMS)) {
			trap_S_StartLocalSound( cgs.media.countPrepareTeamSound, CHAN_ANNOUNCER );
		} else
#endif
		{
			trap_S_StartLocalSound( cgs.media.countPrepareSound, CHAN_ANNOUNCER );
		}
	}

	cg.warmupTime = warmup;
}


/*
======================
CG_NewSortedTeams
======================
*/
void CG_NewSortedTeams( const char* configstring ) {
	int i;
	char str[1024];

	if ( !configstring[0] ) return;
	Q_strncpyz( str, configstring, sizeof( str ) );
	//CG_Printf( "CG_NewSortedTeams: str = %s\n", str );
	cgs.sortedTeams[0] = 0;	// ignore 0 to keep compatibility with team index
	for ( i = 0; i < strlen( str ) && i < TOTAL_TEAMS; i++ ) {
		int num = str[i] - '0';
		//CG_Printf( "CG_NewSortedTeams: num (%i) = %i\n", i, num );

		cgs.sortedTeams[i + FIRST_TEAM] = num;
	}
}

/*
================
CG_SetConfigValues

Called on load to set the initial values from configure strings
================
*/
void CG_SetConfigValues( void ) {
	const char *s;

	cgs.scores[0] = atoi( CG_ConfigString( CS_SCORES1 ) );
	cgs.scores[1] = atoi( CG_ConfigString( CS_SCORES2 ) );
	if ( GTF( GTF_TEAMS ) ) {
		cgs.scores[2] = atoi( CG_ConfigString( CS_SCORES3 ) );
		cgs.scores[3] = atoi( CG_ConfigString( CS_SCORES4 ) );
		cgs.scores[4] = atoi( CG_ConfigString( CS_SCORES5 ) );
		cgs.scores[5] = atoi( CG_ConfigString( CS_SCORES6 ) );
	}
	cgs.levelStartTime = atoi( CG_ConfigString( CS_LEVEL_START_TIME ) );

	CG_NewSortedTeams( CG_ConfigString( CS_SORTEDTEAMS ) );

	if ( GTF(GTF_CTF) ) {
		int i, j = 0;
		s = CG_ConfigString( CS_FLAGSTATUS );

		for ( i = FIRST_TEAM; i < strlen(s); i++ ) {
			if ( !s[j] ) break;
			cgs.flagStatus[i] = s[j] - '0';
			j++;
		}
	}
	else if( cgs.gameType == GT_1FCTF ) {
		s = CG_ConfigString( CS_FLAGSTATUS );
		cgs.flagStatus[TEAM_FREE] = s[0] - '0';
	}

	cg.warmupTime = atoi( CG_ConfigString( CS_WARMUP ) );
	cg.warmupState = atoi( CG_ConfigString( CS_WARMUP_STATE ) );
	cg.warmupVal = atoi( CG_ConfigString( CS_WARMUP_VAL ) );

	cgs.numTeams = atoi( CG_ConfigString( CS_NUMTEAMS ) );

	cgs.queueIndex = atoi( CG_ConfigString( CS_TOURNEY_QUEUEINDEX ) );

	cg.overTime = atoi( CG_ConfigString( CS_OVERTIME ) );
}

/*
=====================
CG_ShaderStateChanged
=====================
*/
void CG_ShaderStateChanged(void) {
	char originalShader[MAX_QPATH];
	char newShader[MAX_QPATH];
	char timeOffset[16];
	const char *o;
	char *n,*t;

	o = CG_ConfigString( CS_SHADERSTATE );
	while (o && *o) {
		n = strstr(o, "=");
		if (n && *n) {
			strncpy(originalShader, o, n-o);
			originalShader[n-o] = 0;
			n++;
			t = strstr(n, ":");
			if (t && *t) {
				strncpy(newShader, n, t-n);
				newShader[t-n] = 0;
			} else {
				break;
			}
			t++;
			o = strstr(t, "@");
			if (o) {
				strncpy(timeOffset, t, o-t);
				timeOffset[o-t] = 0;
				o++;
				trap_R_RemapShader( originalShader, newShader, timeOffset );
			}
		} else {
			break;
		}
	}

	// Only need to do this once, unless a shader is remapped to new shader with fogvars.
	trap_R_GetGlobalFog( &cgs.globalFogType, cgs.globalFogColor, &cgs.globalFogDepthForOpaque, &cgs.globalFogDensity, &cgs.globalFogFarClip );
}


/*
================
CG_ConfigStringModified

================
*/
static void CG_ConfigStringModified( void ) {
	const char	*str;
	int		num;

	num = atoi( CG_Argv( 1 ) );

	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState( &cgs.gameState );

	// look up the individual string that was modified
	str = CG_ConfigString( num );

	// do something with it if necessary
	if ( num == CS_MUSIC ) {
		CG_StartMusic();
	} else if ( num == CS_SERVERINFO ) {
		CG_ParseServerinfo();
	} else if ( num == CS_TOURNEY_QUEUEINDEX ) {
		cgs.queueIndex = atoi( str );
	} else if ( num == CS_OVERTIME ) {
		cg.overTime = atoi( str );
		cg.timePulse = cg.time;
	} else if ( num == CS_WARMUP ) {
		CG_ParseWarmup();
	} else if ( num == CS_WARMUP_STATE ) {
		cg.warmupState = atoi( str );
	} else if ( num == CS_WARMUP_VAL ) {
		int i, numshort = 0;
		cg.warmupVal = atoi( str );

		if ( GTF( GTF_TEAMS ) && cg.warmupState == WARMUP_SHORT_TEAMS ) {
			for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ ) {
				if ( cg.warmupVal & (1 << (i - FIRST_TEAM)) ) {
					cg.warmupShortTeams[i] = qtrue;
					numshort++;
				} else {
					cg.warmupShortTeams[i] = qfalse;
				}
			}
			cg.warmupNumShortTeams = numshort;
		}
	} else if ( num == CS_SCORES1 ) {
		cgs.scores[0] = atoi( str );
	} else if ( num == CS_SCORES2 ) {
		cgs.scores[1] = atoi( str );
	} else if ( num == CS_SCORES3 ) {
		cgs.scores[2] = atoi( str );
	} else if ( num == CS_SCORES4 ) {
		cgs.scores[3] = atoi( str );
	} else if ( num == CS_SCORES5 ) {
		cgs.scores[4] = atoi( str );
	} else if ( num == CS_SCORES6 ) {
		cgs.scores[5] = atoi( str );
	} else if ( num == CS_LEVEL_START_TIME ) {
		cgs.levelStartTime = atoi( str );
	} else if ( num == CS_VOTE_TIME ) {
		cgs.voteTime = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_YES ) {
		cgs.voteYes = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_NO ) {
		cgs.voteNo = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_STRING ) {
		Q_strncpyz( cgs.voteString, str, sizeof( cgs.voteString ) );
#ifdef MISSIONPACK
		trap_S_StartLocalSound( cgs.media.voteNow, CHAN_ANNOUNCER );
#endif //MISSIONPACK
	} else if ( num == CS_NUMTEAMS ) {
		cgs.numTeams = atoi( str );
	} else if ( num == CS_SORTEDTEAMS ) {
		CG_NewSortedTeams( str );
	} else if ( num == CS_INTERMISSION ) {
		cg.intermissionStarted = atoi( str );
	} else if ( num >= CS_MODELS && num < CS_MODELS+MAX_MODELS ) {
		cgs.gameModels[ num-CS_MODELS ] = trap_R_RegisterModel( str );
	} else if ( num >= CS_SOUNDS && num < CS_SOUNDS+MAX_SOUNDS ) {
		if ( str[0] != '*' ) {	// player specific sounds don't register here
			cgs.gameSounds[ num-CS_SOUNDS] = trap_S_RegisterSound( str, qfalse );
		}
	} else if ( num >= CS_PLAYERS && num < CS_PLAYERS+MAX_CLIENTS ) {
		CG_NewPlayerInfo( num - CS_PLAYERS );
#ifdef MISSIONPACK
		CG_BuildSpectatorString();
#endif
	} else if ( num >= CS_DLIGHTS && num < CS_DLIGHTS + MAX_DLIGHT_CONFIGSTRINGS ) {
		// FIXME - dlight changes ignored!
	} else if ( num == CS_FLAGSTATUS ) {
		if ( GTF( GTF_CTF ) ) {
			int i, j = 0;

			for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ ) {
				cgs.flagStatus[i] = str[j] - '0';
				j++;
			}
		} else if (cgs.gameType == GT_1FCTF) {
			cgs.flagStatus[TEAM_FREE] = str[0] - '0';
		}
	}
	else if ( num == CS_SHADERSTATE ) {
		CG_ShaderStateChanged();
	}
	else if ( num == CS_PLAYERS_READY ) {
		Com_ClientListParse( &cg.readyPlayers, str );
	}
		
}


/*
=======================
CG_AddToPlayerChatBox

=======================
*/
static qboolean CG_AddToPlayerChatBox( int localPlayerNum, const char *str ) {
	int len;
	char *p, *ls;
	int lastcolor;
	int chatHeight;
	localPlayer_t *player;

	player = &cg.localPlayers[localPlayerNum];

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT) {
		chatHeight = cg_teamChatHeight.integer;
	} else {
		chatHeight = TEAMCHAT_HEIGHT;
	}

	if (chatHeight <= 0 || cg_teamChatTime.integer <= 0) {
		// team chat disabled, dump into normal chat
		player->teamChatPos = player->teamLastChatPos = 0;
		return qfalse;
	}

	len = 0;

	p = player->teamChatMsgs[player->teamChatPos % chatHeight];
	*p = 0;

	lastcolor = '7';

	ls = NULL;
	while (*str) {
		if (len > TEAMCHAT_WIDTH - 1) {
			if (ls) {
				str -= (p - ls);
				str++;
				p -= (p - ls);
			}
			*p = 0;

			player->teamChatMsgTimes[player->teamChatPos % chatHeight] = cg.time;

			player->teamChatPos++;
			p = player->teamChatMsgs[player->teamChatPos % chatHeight];
			*p = 0;
			*p++ = Q_COLOR_ESCAPE;
			*p++ = lastcolor;
			len = 0;
			ls = NULL;
		}

		if ( Q_IsColorString( str ) ) {
			*p++ = *str++;
			lastcolor = *str;
			*p++ = *str++;
			continue;
		}
		if (*str == ' ') {
			ls = p;
		}
		*p++ = *str++;
		len++;
	}
	*p = 0;

	player->teamChatMsgTimes[player->teamChatPos % chatHeight] = cg.time;
	player->teamChatPos++;

	if (player->teamChatPos - player->teamLastChatPos > chatHeight)
		player->teamLastChatPos = player->teamChatPos - chatHeight;

	return qtrue;
}

/*
=======================
CG_AddToTeamChat

Add message to team chat box if enabled otherwise add message to notify messages.
(for tell messages) Add message to notify messages if local player is on a different team.
=======================
*/
static void CG_AddToTeamChat( int localPlayerBits, team_t team, const char *text ) {
	int i, teamBits;
	qboolean firstTime;

	teamBits = CG_LocalPlayerBitsForTeam( team );
	firstTime = qtrue;

	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		if ( ! ( localPlayerBits & ( 1 << i ) ) ) {
			continue;
		}

		if ( ( teamBits & ( 1 << i ) ) && CG_AddToPlayerChatBox( i, text ) ) {
			if ( firstTime ) {
				//CG_NotifyPrintf( i, "[skipnotify]%s\n", text );
				CG_Printf( "[skipnotify][%s team]%s\n", CG_TeamName( team ), text );
				firstTime = qfalse;
			}
		} else {
			CG_NotifyPrintf( i, "%s\n", text );
		}
	}
}

/*
===============
CG_MapRestart

The server has issued a map_restart, so the next snapshot
is completely new and should not be interpolated to.

A tournament restart will clear everything, but doesn't
require a reload of all the media
===============
*/
static void CG_MapRestart( void ) {
	int	i;

	if ( cg_showMiss.integer ) {
		CG_Printf( "CG_MapRestart\n" );
	}

	CG_InitLocalEntities();
	CG_InitMarkPolys();
	CG_CacheParticles ();

	// make sure the "3 frags left" warnings play again
	cg.fraglimitWarnings = 0;

	cg.timelimitWarnings = 0;
	cg.intermissionStarted = qfalse;
	cg.levelShot = qfalse;

	cgs.voteTime = 0;

	cg.lightstylesInited = qfalse;

	cg.mapRestart = qtrue;

	cg.reloadAssets = qtrue;
	//FIXME check the kind of gametype change that has occured and only try loading what is needed
	CG_RegisterGraphics( qtrue );
	CG_RegisterSounds( qtrue );

	CG_StartMusic();

	trap_S_ClearLoopingSounds(qtrue);

	// we really should clear more parts of cg here and stop sounds

	// play the "fight" sound if this is a restart without warmup
	if ( cg.warmupTime == 0 ) {
		trap_S_StartLocalSound( cgs.media.countFightSound, CHAN_ANNOUNCER );
		if ( CG_NumLocalPlayers() > 1 ) {
			CG_GlobalCenterPrint( "FIGHT!", SCREEN_HEIGHT/2, 0.6 );
		} else {
			CG_GlobalCenterPrint( "FIGHT!", 112 + BIGCHAR_HEIGHT/2, 0.6 );
		}
	}
#ifdef MISSIONPACK
	if (cg_singlePlayer.integer) {
		trap_Cvar_SetValue("ui_matchStartTime", cg.time);
		if (cg_recordSPDemo.integer && *cg_recordSPDemoName.string) {
			trap_Cmd_ExecuteText(EXEC_APPEND, va("set g_synchronousclients 1 ; record %s \n", cg_recordSPDemoName.string));
		}
	}
#endif

	for (i = 0; i < CG_MaxSplitView(); i++) {
		cg.localPlayers[i].rewardTime = 0;
		cg.localPlayers[i].rewardStack = 0;

		cg.localPlayers[i].cameraOrbit = 0;
	}
}


/*
=================
CG_RemoveChatEscapeChar
=================
*/
static void CG_RemoveChatEscapeChar( char *text ) {
	int i, l;

	l = 0;
	for ( i = 0; text[i]; i++ ) {
		if (text[i] == '\x19')
			continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}

/*
=================
CG_LocalPlayerBitsForTeam
=================
*/
int CG_LocalPlayerBitsForTeam( team_t team ) {
	playerInfo_t	*pi;
	int				playerNum;
	int				bits;
	int				i;
	
	bits = 0;
	
	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		playerNum = cg.localPlayers[i].playerNum;
		if ( playerNum == -1 ) {
			continue;
		}
		
		pi = &cgs.playerinfo[playerNum];
		if ( !pi->infoValid ) {
			continue;
		}
		
		if ( team == -1 || pi->team == team ) {
			bits |= ( 1 << i );
		}
	}

	return bits;
}

void CG_ReplaceCharacter( char *str, char old, char new ) {
	char *p = strchr( str, old );

	while ( p != NULL )
	{
		*p = new;
		p = strchr( p + 1, old );
	}
}


/*
=================
CG_GameNotifyAdd

Add to notification text
=================
*/
void CG_GameNotifyAdd( const char* in ) {
	if ( !strlen(in) ) return;

	cg.notifyNum = (cg.notifyNum + 1) % MAX_NOTIFY_HISTORY;
	cg.notifyTime[cg.notifyNum] = cg.time;
	//CG_Printf( "CG_GameNotifyAdd: num = %i, text in = %s\n", cg.notifyNum, in );
	Q_strncpyz( cg.notifyText[cg.notifyNum], in, sizeof( cg.notifyText[cg.notifyNum] ) );
	CG_ReplaceCharacter( cg.notifyText[cg.notifyNum], '\n', ' ' );
}


/*
=================
CG_ServerCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
static void CG_ServerCommand( void ) {
	const char	*cmd;
	char		text[MAX_SAY_TEXT];
	int			start = 0;
	team_t		team = -1;
	int			localPlayerBits = -1;
	int			i;
	int			chatPlayerNum;

	cmd = CG_Argv(start);

	if ( !cmd[0] ) {
		// server claimed the command
		return;
	}

	for ( i = FIRST_TEAM; i <= cgs.numTeams; i++ ) {
		if ( !Q_stricmp( cmd, va( "[%s]", cg_teamNames[i] ) ) ) {
			team = i;
		}
	}

	if ( team >= FIRST_TEAM ) {
		localPlayerBits = CG_LocalPlayerBitsForTeam( team );

		// Get command
		start++;
		cmd = CG_Argv( start );
	} else if ( !Q_stricmp( cmd, "[SPECTATOR]" ) ) {
		team = TEAM_SPECTATOR;
		localPlayerBits = CG_LocalPlayerBitsForTeam( team );

		// Get command
		start++;
		cmd = CG_Argv(start);
	} else if ( !Q_stricmp( cmd, "[FREE]" ) ) {
		team = TEAM_FREE;
		localPlayerBits = CG_LocalPlayerBitsForTeam( team );

		// Get command
		start++;
		cmd = CG_Argv(start);
	}
	// Commands for specific player begin "lc# "
	else if ( cmd[0] == 'l' && cmd[1] =='c' && isdigit(cmd[2]) ) {
		int num = atoi( &cmd[2] );

		if ( num < 0 || num > CG_MaxSplitView() ) {
			return;
		}

		team = cgs.playerinfo[ cg.localPlayers[num].playerNum ].team;
		localPlayerBits = ( 1 << num );

		// Get command
		start++;
		cmd = CG_Argv(start);
	}

	if ( !strcmp( cmd, "cp" ) ) {
		// print to console as a single line
		Q_strncpyz( text, CG_Argv( start+1 ), sizeof ( text ) );
		if ( strlen(text) > 1 && text[strlen(text) - 1] == '\n' ) {
			text[strlen(text) - 1] = '\0';
		}
		CG_ReplaceCharacter( text, '\n', ' ' );
		CG_Printf("[skipnotify]%s\n", text );

		for ( i = 0; i < CG_MaxSplitView(); i++ ) {
			if ( localPlayerBits == -1 || ( localPlayerBits & ( 1 << i ) ) ) {
				CG_CenterPrint( i, CG_Argv( start + 1 ), SCREEN_HEIGHT * 0.30, 0.5, qfalse );
			}
		}
		return;
	}

	// priority center print
	if ( !strcmp( cmd, "pcp" ) ) {
		CG_CenterPrint( i, CG_Argv( 1 ), SCREEN_HEIGHT * 0.30, 0.5, qtrue );
		return;
	}

	if ( !strcmp( cmd, "cs" ) ) {
		if ( localPlayerBits != -1 ) {
			return;
		}

		CG_ConfigStringModified();
		return;
	}

	// global print to all players
	if ( !strcmp( cmd, "print" ) && localPlayerBits == -1 ) {
#ifdef MISSIONPACK
		cmd = CG_Argv(start+1);			// yes, this is obviously a hack, but so is the way we hear about
									// votes passing or failing
		if ( !Q_stricmpn( cmd, "vote failed", 11 ) || !Q_stricmpn( cmd, "team vote failed", 16 )) {
			trap_S_StartLocalSound( cgs.media.voteFailed, CHAN_ANNOUNCER );
		} else if ( !Q_stricmpn( cmd, "vote passed", 11 ) || !Q_stricmpn( cmd, "team vote passed", 16 ) ) {
			trap_S_StartLocalSound( cgs.media.votePassed, CHAN_ANNOUNCER );
		}
#endif
		CG_Printf("%s", CG_Argv( start + 1 ) );
		CG_GameNotifyAdd( va( "%s", CG_Argv( start + 1 )) );
		return;
	}

	if ( !strcmp( cmd, "print" ) ) {
		CG_NotifyBitsPrintf( localPlayerBits, "%s", CG_Argv( start+1 ) );
		CG_GameNotifyAdd( va( "%s", CG_Argv( start + 1 ) ) );
		return;
	}

	if ( !strcmp( cmd, "chat" ) ) {
		if ( trap_Argc() > start+2 ) {
			chatPlayerNum = atoi(CG_Argv(start+2));
		} else {
			// message is from a pre-Spearmint 0.5 server or demo
			chatPlayerNum = CHATPLAYER_UNKNOWN;
		}

		// allow disabling non-team chat but always show server chat
		if (GTF(GTF_TEAMS) && cg_teamChatsOnly.integer && chatPlayerNum != CHATPLAYER_SERVER ) {
			return;
		}

		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );

		Q_strncpyz( text, CG_Argv(start+1), MAX_SAY_TEXT );

		CG_RemoveChatEscapeChar( text );
		CG_GameNotifyAdd( text );
		CG_Printf( "%s\n", text );
		return;
	}

	if ( !strcmp( cmd, "tell" ) ) {
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );

		Q_strncpyz( text, CG_Argv(start+1), MAX_SAY_TEXT );

		if ( trap_Argc() > start+2 ) {
			chatPlayerNum = atoi(CG_Argv(start+2));
		} else {
			// message is from a pre-Spearmint 0.5 server or demo
			chatPlayerNum = CHATPLAYER_UNKNOWN;
		}

		CG_RemoveChatEscapeChar( text );
		if ( chatPlayerNum >= 0 && chatPlayerNum < MAX_CLIENTS ) {
			CG_AddToTeamChat( localPlayerBits, cgs.playerinfo[chatPlayerNum].team, text );
			CG_GameNotifyAdd( text );
		} else {
			CG_NotifyBitsPrintf( localPlayerBits, "%s\n", text );
		}
		return;
	}

	if ( !strcmp( cmd, "tchat" ) ) {
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );

		Q_strncpyz( text, CG_Argv(start+1), MAX_SAY_TEXT );

		if ( trap_Argc() > start+2 ) {
			chatPlayerNum = atoi(CG_Argv(start+2));
		} else {
			// message is from a pre-Spearmint 0.5 server or demo
			chatPlayerNum = CHATPLAYER_UNKNOWN;
		}

		CG_RemoveChatEscapeChar( text );
		CG_AddToTeamChat( localPlayerBits, team, text );
		CG_GameNotifyAdd( text );
		return;
	}

	if ( !strcmp( cmd, "scores" ) ) {
		CG_ParseScores(start);
		return;
	}

	if ( !strcmp( cmd, "pwstats" ) ) {
		CG_ParsePlayerWeaponStats( start );
		return;
	}

	if ( !strcmp( cmd, "tinfo" ) ) {
		CG_ParseTeamInfo(start);
		return;
	}

	if ( !strcmp( cmd, "map_restart" ) ) {
		if ( localPlayerBits != -1 ) {
			return;
		}

		CG_MapRestart();
		return;
	}

	if ( Q_stricmp (cmd, "remapShader") == 0 )
	{
		if ( localPlayerBits != -1 ) {
			return;
		}

		if (trap_Argc() == 4)
		{
			char shader1[MAX_QPATH];
			char shader2[MAX_QPATH];
			char shader3[MAX_QPATH];

			Q_strncpyz(shader1, CG_Argv(start+1), sizeof(shader1));
			Q_strncpyz(shader2, CG_Argv(start+2), sizeof(shader2));
			Q_strncpyz(shader3, CG_Argv(start+3), sizeof(shader3));

			trap_R_RemapShader(shader1, shader2, shader3);
		}
		
		return;
	}

	// loadDeferred can be both a servercmd and a consolecmd
	if ( !strcmp( cmd, "loadDeferred" ) ) {
		if ( localPlayerBits != -1 ) {
			return;
		}

		CG_LoadDeferredPlayers();
		return;
	}

	// clientLevelShot is sent before taking a special screenshot for
	// the menu system during development
	if ( !strcmp( cmd, "clientLevelShot" ) ) {
		cg.levelShot = qtrue;
		return;
	}

	CG_Printf( "Unknown client game command: %s\n", cmd );
}


/*
====================
CG_ExecuteNewServerCommands

Execute all of the server commands that were received along
with this this snapshot.
====================
*/
void CG_ExecuteNewServerCommands( int latestSequence ) {
	while ( cgs.serverCommandSequence < latestSequence ) {
		if ( trap_GetServerCommand( ++cgs.serverCommandSequence ) ) {
			CG_ServerCommand();
		}
	}
}
