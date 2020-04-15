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

/*
=================
CG_ParseScores

=================
*/
static void CG_ParseScores( int start ) {
	int		i, powerups;

	cg.numScores = atoi( CG_Argv( 1 + start) );
	if ( cg.numScores > MAX_CLIENTS ) {
		cg.numScores = MAX_CLIENTS;
	}

	cg.teamScores[0] = atoi( CG_Argv( 2 + start) );
	cg.teamScores[1] = atoi( CG_Argv( 3 + start) );

	memset( cg.scores, 0, sizeof( cg.scores ) );
	for ( i = 0 ; i < cg.numScores ; i++ ) {
		//
		cg.scores[i].playerNum = atoi( CG_Argv( i * 14 + 4 + start) );
		cg.scores[i].score = atoi( CG_Argv( i * 14 + 5 + start) );
		cg.scores[i].ping = atoi( CG_Argv( i * 14 + 6 + start) );
		cg.scores[i].time = atoi( CG_Argv( i * 14 + 7 + start) );
		cg.scores[i].scoreFlags = atoi( CG_Argv( i * 14 + 8 + start) );
		powerups = atoi( CG_Argv( i * 14 + 9 + start) );
		cg.scores[i].accuracy = atoi(CG_Argv(i * 14 + 10 + start));
		cg.scores[i].impressiveCount = atoi(CG_Argv(i * 14 + 11 + start));
		cg.scores[i].excellentCount = atoi(CG_Argv(i * 14 + 12 + start));
		cg.scores[i].guantletCount = atoi(CG_Argv(i * 14 + 13 + start));
		cg.scores[i].defendCount = atoi(CG_Argv(i * 14 + 14 + start));
		cg.scores[i].assistCount = atoi(CG_Argv(i * 14 + 15 + start));
		cg.scores[i].perfect = atoi(CG_Argv(i * 14 + 16 + start));
		cg.scores[i].captures = atoi(CG_Argv(i * 14 + 17 + start));

		if ( cg.scores[i].playerNum < 0 || cg.scores[i].playerNum >= MAX_CLIENTS ) {
			cg.scores[i].playerNum = 0;
		}
		cgs.playerinfo[ cg.scores[i].playerNum ].score = cg.scores[i].score;
		cgs.playerinfo[ cg.scores[i].playerNum ].powerups = powerups;

		cg.scores[i].team = cgs.playerinfo[cg.scores[i].playerNum].team;
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
	int		team;

	team = atoi( CG_Argv( 1 + start ) );
	if( team < 0 || team >= TEAM_NUM_TEAMS )
	{
		CG_Error( "CG_ParseTeamInfo: team out of range (%d)",
				team );
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
	cgs.maxplayers = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cgs.mapname, sizeof( cgs.mapname ), "maps/%s.bsp", mapname );
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

	if ( warmup == 0 && cg.warmup ) {

	} else if ( warmup > 0 && cg.warmup <= 0 ) {
#ifdef MISSIONPACK
		if (GTF(GTF_TEAMS)) {
			trap_S_StartLocalSound( cgs.media.countPrepareTeamSound, CHAN_ANNOUNCER );
		} else
#endif
		{
			trap_S_StartLocalSound( cgs.media.countPrepareSound, CHAN_ANNOUNCER );
		}
	}

	cg.warmup = warmup;
}

/*
================
CG_SetConfigValues

Called on load to set the initial values from configure strings
================
*/
void CG_SetConfigValues( void ) {
	const char *s;

	cgs.scores1 = atoi( CG_ConfigString( CS_SCORES1 ) );
	cgs.scores2 = atoi( CG_ConfigString( CS_SCORES2 ) );
	cgs.levelStartTime = atoi( CG_ConfigString( CS_LEVEL_START_TIME ) );
	if ( GTF(GTF_CTF) ) {
		s = CG_ConfigString( CS_FLAGSTATUS );
		cgs.redflag = s[0] - '0';
		cgs.blueflag = s[1] - '0';
	}
	else if( cgs.gameType == GT_1FCTF ) {
		s = CG_ConfigString( CS_FLAGSTATUS );
		cgs.flagStatus = s[0] - '0';
	}

	cg.warmup = atoi( CG_ConfigString( CS_WARMUP ) );
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
	} else if ( num == CS_WARMUP ) {
		CG_ParseWarmup();
	} else if ( num == CS_SCORES1 ) {
		cgs.scores1 = atoi( str );
	} else if ( num == CS_SCORES2 ) {
		cgs.scores2 = atoi( str );
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
	} else if ( num >= CS_TEAMVOTE_TIME && num <= CS_TEAMVOTE_TIME + 1) {
		cgs.teamVoteTime[num-CS_TEAMVOTE_TIME] = atoi( str );
		cgs.teamVoteModified[num-CS_TEAMVOTE_TIME] = qtrue;
	} else if ( num >= CS_TEAMVOTE_YES && num <= CS_TEAMVOTE_YES + 1) {
		cgs.teamVoteYes[num-CS_TEAMVOTE_YES] = atoi( str );
		cgs.teamVoteModified[num-CS_TEAMVOTE_YES] = qtrue;
	} else if ( num >= CS_TEAMVOTE_NO && num <= CS_TEAMVOTE_NO + 1) {
		cgs.teamVoteNo[num-CS_TEAMVOTE_NO] = atoi( str );
		cgs.teamVoteModified[num-CS_TEAMVOTE_NO] = qtrue;
	} else if ( num >= CS_TEAMVOTE_STRING && num <= CS_TEAMVOTE_STRING + 1) {
		Q_strncpyz( cgs.teamVoteString[num-CS_TEAMVOTE_STRING], str, sizeof( cgs.teamVoteString[0] ) );
#ifdef MISSIONPACK
		trap_S_StartLocalSound( cgs.media.voteNow, CHAN_ANNOUNCER );
#endif
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
		if( GTF(GTF_CTF) ) {
			// format is rb where its red/blue, 0 is at base, 1 is taken, 2 is dropped
			cgs.redflag = str[0] - '0';
			cgs.blueflag = str[1] - '0';
		}
		else if( cgs.gameType == GT_1FCTF ) {
			cgs.flagStatus = str[0] - '0';
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

// copied from g_team.c
const char *CG_TeamName(int team)  {
	if (team==TEAM_RED)
		return "RED";
	else if (team==TEAM_BLUE)
		return "BLUE";
	else if (team==TEAM_SPECTATOR)
		return "SPECTATOR";
	return "FREE";
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

A tournement restart will clear everything, but doesn't
require a reload of all the media
===============
*/
static void CG_MapRestart( void ) {
	int	i;

	if ( cg_showmiss.integer ) {
		CG_Printf( "CG_MapRestart\n" );
	}

	CG_InitLocalEntities();
	CG_InitMarkPolys();
	CG_ClearParticles ();

	// make sure the "3 frags left" warnings play again
	cg.fraglimitWarnings = 0;

	cg.timelimitWarnings = 0;
	cg.intermissionStarted = qfalse;
	cg.levelShot = qfalse;

	cgs.voteTime = 0;

	cg.lightstylesInited = qfalse;

	cg.mapRestart = qtrue;

	CG_StartMusic();

	trap_S_ClearLoopingSounds(qtrue);

	// we really should clear more parts of cg here and stop sounds

	// play the "fight" sound if this is a restart without warmup
	if ( cg.warmup == 0 ) {
		trap_S_StartLocalSound( cgs.media.countFightSound, CHAN_ANNOUNCER );
		if ( CG_NumLocalPlayers() > 1 ) {
			CG_GlobalCenterPrint( "FIGHT!", SCREEN_HEIGHT/2, 2.0 );
		} else {
			CG_GlobalCenterPrint( "FIGHT!", 112 + GIANTCHAR_HEIGHT/2, 2.0 );
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

	// Commands for team
	if ( !Q_stricmp( cmd, "[RED]" ) ) {
		team = TEAM_RED;
		localPlayerBits = CG_LocalPlayerBitsForTeam( team );

		// Get command
		start++;
		cmd = CG_Argv(start);
	}
	else if ( !Q_stricmp( cmd, "[BLUE]" ) ) {
		team = TEAM_BLUE;
		localPlayerBits = CG_LocalPlayerBitsForTeam( team );

		// Get command
		start++;
		cmd = CG_Argv(start);
	}
	else if ( !Q_stricmp( cmd, "[SPECTATOR]" ) ) {
		team = TEAM_SPECTATOR;
		localPlayerBits = CG_LocalPlayerBitsForTeam( team );

		// Get command
		start++;
		cmd = CG_Argv(start);
	}
	else if ( !Q_stricmp( cmd, "[FREE]" ) ) {
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
				CG_CenterPrint( i, CG_Argv( start + 1 ), SCREEN_HEIGHT * 0.30, 0.5 );
			}
		}
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

		CG_Printf("%s", CG_Argv( start+1 ) );
		return;
	}

	if ( !strcmp( cmd, "print" ) ) {
		CG_NotifyBitsPrintf( localPlayerBits, "%s", CG_Argv( start+1 ) );
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
		return;
	}

	if ( !strcmp( cmd, "scores" ) ) {
		CG_ParseScores(start);
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

	// loaddeferred can be both a servercmd and a consolecmd
	if ( !strcmp( cmd, "loaddeferred" ) ) {
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
