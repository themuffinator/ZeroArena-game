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
/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/


#include "ui_local.h"


qboolean UI_WantsBindKeys( void ) {
	return Controls_WantsBindKeys();
}

void UI_WindowResized( void ) {

}


/*
================
cvars
================
*/

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
} cvarTable_t;
/*
vmCvar_t	ui_ffa_fraglimit;
vmCvar_t	ui_ffa_timelimit;
vmCvar_t	ui_ffa_instagib;

vmCvar_t	ui_tourney_fraglimit;
vmCvar_t	ui_tourney_timelimit;
vmCvar_t	ui_tourney_instagib;

vmCvar_t	ui_team_fraglimit;
vmCvar_t	ui_team_timelimit;
vmCvar_t	ui_team_friendly;
vmCvar_t	ui_team_instagib;

vmCvar_t	ui_ctf_capturelimit;
vmCvar_t	ui_ctf_timelimit;
vmCvar_t	ui_ctf_friendly;
vmCvar_t	ui_ctf_instagib;

vmCvar_t	ui_1flag_capturelimit;
vmCvar_t	ui_1flag_timelimit;
vmCvar_t	ui_1flag_friendly;
vmCvar_t	ui_1flag_instagib;

vmCvar_t	ui_obelisk_capturelimit;
vmCvar_t	ui_obelisk_timelimit;
vmCvar_t	ui_obelisk_friendly;
vmCvar_t	ui_obelisk_instagib;

vmCvar_t	ui_harvester_capturelimit;
vmCvar_t	ui_harvester_timelimit;
vmCvar_t	ui_harvester_friendly;
vmCvar_t	ui_harvester_instagib;
*/

vmCvar_t	ui_gt_frags_free_scoreLimit;
vmCvar_t	ui_gt_frags_free_timeLimit;
vmCvar_t	ui_gt_frags_free_instaGib;
vmCvar_t	ui_gt_frags_teams_scoreLimit;
vmCvar_t	ui_gt_frags_teams_timeLimit;
vmCvar_t	ui_gt_frags_teams_friendlyFire;
vmCvar_t	ui_gt_frags_teams_instaGib;
vmCvar_t	ui_gt_1v1_scoreLimit;
vmCvar_t	ui_gt_1v1_timeLimit;
vmCvar_t	ui_gt_1v1_instaGib;
vmCvar_t	ui_gt_captures_teams_scoreLimit;
vmCvar_t	ui_gt_captures_teams_timeLimit;
vmCvar_t	ui_gt_captures_teams_friendlyFire;
vmCvar_t	ui_gt_captures_teams_instaGib;
vmCvar_t	ui_gt_elim_free_lives;
vmCvar_t	ui_gt_elim_free_timeLimit;
vmCvar_t	ui_gt_elim_free_instaGib;
vmCvar_t	ui_gt_elim_teams_lives;
vmCvar_t	ui_gt_elim_teams_timeLimit;
vmCvar_t	ui_gt_elim_teams_friendlyFire;
vmCvar_t	ui_gt_elim_teams_instaGib;
vmCvar_t	ui_gt_points_free_scoreLimit;
vmCvar_t	ui_gt_points_free_timeLimit;
vmCvar_t	ui_gt_points_free_instaGib;
vmCvar_t	ui_gt_points_teams_scoreLimit;
vmCvar_t	ui_gt_points_teams_timeLimit;
vmCvar_t	ui_gt_points_teams_friendlyFire;
vmCvar_t	ui_gt_points_teams_instaGib;
vmCvar_t	ui_gt_rounds_free_maxRounds;
vmCvar_t	ui_gt_rounds_free_maxRoundTime;
vmCvar_t	ui_gt_rounds_teams_maxRounds;
vmCvar_t	ui_gt_rounds_teams_maxRoundTime;

vmCvar_t	ui_publicServer;

vmCvar_t	ui_arenasFile;
vmCvar_t	ui_botsFile;
vmCvar_t	ui_spScores1;
vmCvar_t	ui_spScores2;
vmCvar_t	ui_spScores3;
vmCvar_t	ui_spScores4;
vmCvar_t	ui_spScores5;
vmCvar_t	ui_spAwards;
vmCvar_t	ui_spVideos;
vmCvar_t	ui_spSkill;

vmCvar_t	ui_spSelection;

vmCvar_t	ui_browserMaster;
vmCvar_t	ui_browserGame;
vmCvar_t	ui_browserGameType;
vmCvar_t	ui_browserSortKey;
vmCvar_t	ui_browserShowFull;
vmCvar_t	ui_browserShowEmpty;
vmCvar_t	ui_browserShowBots;
vmCvar_t	ui_browserSeparateMasters;

vmCvar_t	ui_brassTime;
vmCvar_t	ui_drawCrosshair;
vmCvar_t	ui_drawCrosshairNames;
vmCvar_t	ui_marks;

vmCvar_t	ui_server1;
vmCvar_t	ui_server2;
vmCvar_t	ui_server3;
vmCvar_t	ui_server4;
vmCvar_t	ui_server5;
vmCvar_t	ui_server6;
vmCvar_t	ui_server7;
vmCvar_t	ui_server8;
vmCvar_t	ui_server9;
vmCvar_t	ui_server10;
vmCvar_t	ui_server11;
vmCvar_t	ui_server12;
vmCvar_t	ui_server13;
vmCvar_t	ui_server14;
vmCvar_t	ui_server15;
vmCvar_t	ui_server16;

vmCvar_t	ui_ioq3;

vmCvar_t	ui_menuFont;
vmCvar_t	ui_menuFontProp;
vmCvar_t	ui_menuFontBanner;

static cvarTable_t		cvarTable[] = {
	/*
	{ &ui_ffa_fraglimit, "ui_ffa_fraglimit", "20", CVAR_ARCHIVE },
	{ &ui_ffa_timelimit, "ui_ffa_timelimit", "0", CVAR_ARCHIVE },
	{ &ui_ffa_instagib, "ui_ffa_instagib", "0", CVAR_ARCHIVE },

	{ &ui_tourney_fraglimit, "ui_tourney_fraglimit", "0", CVAR_ARCHIVE },
	{ &ui_tourney_timelimit, "ui_tourney_timelimit", "15", CVAR_ARCHIVE },
	{ &ui_tourney_instagib, "ui_tourney_instagib", "0", CVAR_ARCHIVE },

	{ &ui_team_fraglimit, "ui_team_fraglimit", "0", CVAR_ARCHIVE },
	{ &ui_team_timelimit, "ui_team_timelimit", "20", CVAR_ARCHIVE },
	{ &ui_team_friendly, "ui_team_friendly",  "1", CVAR_ARCHIVE },
	{ &ui_team_instagib, "ui_team_instagib", "0", CVAR_ARCHIVE },

	{ &ui_ctf_capturelimit, "ui_ctf_capturelimit", "8", CVAR_ARCHIVE },
	{ &ui_ctf_timelimit, "ui_ctf_timelimit", "30", CVAR_ARCHIVE },
	{ &ui_ctf_friendly, "ui_ctf_friendly",  "0", CVAR_ARCHIVE },
	{ &ui_ctf_instagib, "ui_ctf_instagib", "0", CVAR_ARCHIVE },

	{ &ui_1flag_capturelimit, "ui_1flag_capturelimit", "5", CVAR_ARCHIVE },
	{ &ui_1flag_timelimit, "ui_1flag_timelimit", "30", CVAR_ARCHIVE },
	{ &ui_1flag_friendly, "ui_1flag_friendly",  "0", CVAR_ARCHIVE },
	{ &ui_1flag_instagib, "ui_1flag_instagib", "0", CVAR_ARCHIVE },

	{ &ui_obelisk_capturelimit, "ui_obelisk_capturelimit", "5", CVAR_ARCHIVE },
	{ &ui_obelisk_timelimit, "ui_obelisk_timelimit", "20", CVAR_ARCHIVE },
	{ &ui_obelisk_friendly, "ui_obelisk_friendly",  "0", CVAR_ARCHIVE },
	{ &ui_obelisk_instagib, "ui_obelisk_instagib", "0", CVAR_ARCHIVE },

	{ &ui_harvester_capturelimit, "ui_harvester_capturelimit", "5", CVAR_ARCHIVE },
	{ &ui_harvester_timelimit, "ui_harvester_timelimit", "30", CVAR_ARCHIVE },
	{ &ui_harvester_friendly, "ui_harvester_friendly",  "0", CVAR_ARCHIVE },
	{ &ui_harvester_instagib, "ui_harvester_instagib", "0", CVAR_ARCHIVE },
	*/

	{ &ui_gt_frags_free_scoreLimit, "ui_gt_frags_free_scoreLimit", "30", CVAR_ARCHIVE },
	{ &ui_gt_frags_free_timeLimit, "ui_gt_frags_free_timeLimit", "20", CVAR_ARCHIVE },
	{ &ui_gt_frags_free_instaGib, "ui_gt_frags_free_instaGib", "0", CVAR_ARCHIVE },
	{ &ui_gt_frags_teams_scoreLimit, "ui_gt_frags_teams_scoreLimit", "200", CVAR_ARCHIVE },
	{ &ui_gt_frags_teams_timeLimit, "ui_gt_frags_teams_timeLimit", "20", CVAR_ARCHIVE },
	{ &ui_gt_frags_teams_friendlyFire, "ui_gt_frags_teams_friendlyFire", "1", CVAR_ARCHIVE },
	{ &ui_gt_frags_teams_instaGib, "ui_gt_frags_teams_instaGib", "0", CVAR_ARCHIVE },
	{ &ui_gt_1v1_scoreLimit, "ui_gt_1v1_scoreLimit", "30", CVAR_ARCHIVE },
	{ &ui_gt_1v1_timeLimit, "ui_gt_1v1_timeLimit", "10", CVAR_ARCHIVE },
	{ &ui_gt_1v1_instaGib, "ui_gt_1v1_instaGib", "0", CVAR_ARCHIVE },
	{ &ui_gt_captures_teams_scoreLimit, "ui_gt_captures_teams_scoreLimit", "8", CVAR_ARCHIVE },
	{ &ui_gt_captures_teams_timeLimit, "ui_gt_captures_teams_timeLimit", "20", CVAR_ARCHIVE },
	{ &ui_gt_captures_teams_friendlyFire, "ui_gt_captures_teams_friendlyFire", "1", CVAR_ARCHIVE },
	{ &ui_gt_captures_teams_instaGib, "ui_gt_captures_teams_instaGib", "0", CVAR_ARCHIVE },
	{ &ui_gt_elim_free_lives, "ui_gt_elim_free_lives", "3", CVAR_ARCHIVE },
	{ &ui_gt_elim_free_timeLimit, "ui_gt_elim_free_timeLimit", "20", CVAR_ARCHIVE },
	{ &ui_gt_elim_free_instaGib, "ui_gt_elim_free_instaGib", "0", CVAR_ARCHIVE },
	{ &ui_gt_elim_teams_lives, "ui_gt_elim_teams_lives", "3", CVAR_ARCHIVE },
	{ &ui_gt_elim_teams_timeLimit, "ui_gt_elim_teams_timeLimit", "20", CVAR_ARCHIVE },
	{ &ui_gt_elim_teams_friendlyFire, "ui_gt_elim_teams_friendlyFire", "1", CVAR_ARCHIVE },
	{ &ui_gt_elim_teams_instaGib, "ui_gt_elim_teams_instaGib", "0", CVAR_ARCHIVE },
	{ &ui_gt_points_free_scoreLimit, "ui_gt_points_free_scoreLimit", "30", CVAR_ARCHIVE },
	{ &ui_gt_points_free_timeLimit, "ui_gt_points_free_timeLimit", "20", CVAR_ARCHIVE },
	{ &ui_gt_points_free_instaGib, "ui_gt_points_free_instaGib", "0", CVAR_ARCHIVE },
	{ &ui_gt_points_teams_scoreLimit, "ui_gt_points_teams_scoreLimit", "30", CVAR_ARCHIVE },
	{ &ui_gt_points_teams_timeLimit, "ui_gt_points_teams_timeLimit", "20", CVAR_ARCHIVE },
	{ &ui_gt_points_teams_friendlyFire, "ui_gt_points_teams_friendlyFire", "1", CVAR_ARCHIVE },
	{ &ui_gt_points_teams_instaGib, "ui_gt_points_teams_instaGib", "0", CVAR_ARCHIVE },
	{ &ui_gt_rounds_free_maxRounds, "ui_gt_rounds_free_maxRounds", "5", CVAR_ARCHIVE },
	{ &ui_gt_rounds_free_maxRoundTime, "ui_gt_round_free_maxRoundTime", "4", CVAR_ARCHIVE },
	{ &ui_gt_rounds_teams_maxRounds, "ui_gt_rounds_teams_maxRounds", "8", CVAR_ARCHIVE },
	{ &ui_gt_rounds_teams_maxRoundTime, "ui_gt_round_teams_maxRoundTime", "3", CVAR_ARCHIVE },

	{ &ui_publicServer, "ui_publicServer", "1", CVAR_ARCHIVE },

	{ &ui_arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM },
	{ &ui_botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM },
	{ &ui_spScores1, "g_spScores1", "", CVAR_ARCHIVE },
	{ &ui_spScores2, "g_spScores2", "", CVAR_ARCHIVE },
	{ &ui_spScores3, "g_spScores3", "", CVAR_ARCHIVE },
	{ &ui_spScores4, "g_spScores4", "", CVAR_ARCHIVE },
	{ &ui_spScores5, "g_spScores5", "", CVAR_ARCHIVE },
	{ &ui_spAwards, "g_spAwards", "", CVAR_ARCHIVE },
	{ &ui_spVideos, "g_spVideos", "", CVAR_ARCHIVE },
	{ &ui_spSkill, "g_spSkill", "2", CVAR_ARCHIVE | CVAR_LATCH },

	{ &ui_spSelection, "ui_spSelection", "", CVAR_ROM },

	{ &ui_browserMaster, "ui_browserMaster", "1", CVAR_ARCHIVE },
	{ &ui_browserGame, "ui_browserGame", "", CVAR_ARCHIVE },
	{ &ui_browserGameType, "ui_browserGameType", "0", CVAR_ARCHIVE },
	{ &ui_browserSortKey, "ui_browserSortKey", "4", CVAR_ARCHIVE },
	{ &ui_browserShowFull, "ui_browserShowFull", "1", CVAR_ARCHIVE },
	{ &ui_browserShowEmpty, "ui_browserShowEmpty", "1", CVAR_ARCHIVE },
	{ &ui_browserShowBots, "ui_browserShowBots", "1", CVAR_ARCHIVE },
	{ &ui_browserSeparateMasters, "ui_browserSeparateMasters", "0", CVAR_ARCHIVE },

	{ &ui_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE },
	{ &ui_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE },
	{ &ui_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &ui_marks, "cg_marks", "1", CVAR_ARCHIVE },

	{ &ui_server1, "server1", "", CVAR_ARCHIVE },
	{ &ui_server2, "server2", "", CVAR_ARCHIVE },
	{ &ui_server3, "server3", "", CVAR_ARCHIVE },
	{ &ui_server4, "server4", "", CVAR_ARCHIVE },
	{ &ui_server5, "server5", "", CVAR_ARCHIVE },
	{ &ui_server6, "server6", "", CVAR_ARCHIVE },
	{ &ui_server7, "server7", "", CVAR_ARCHIVE },
	{ &ui_server8, "server8", "", CVAR_ARCHIVE },
	{ &ui_server9, "server9", "", CVAR_ARCHIVE },
	{ &ui_server10, "server10", "", CVAR_ARCHIVE },
	{ &ui_server11, "server11", "", CVAR_ARCHIVE },
	{ &ui_server12, "server12", "", CVAR_ARCHIVE },
	{ &ui_server13, "server13", "", CVAR_ARCHIVE },
	{ &ui_server14, "server14", "", CVAR_ARCHIVE },
	{ &ui_server15, "server15", "", CVAR_ARCHIVE },
	{ &ui_server16, "server16", "", CVAR_ARCHIVE },

	{ &ui_ioq3, "ui_ioq3", "1", CVAR_ROM },

	{ &ui_menuFont, "ui_menuFont", "fonts/LiberationSans-Bold.ttf", CVAR_ARCHIVE | CVAR_LATCH },
	{ &ui_menuFontProp, "ui_menuFontProp", "", CVAR_ARCHIVE | CVAR_LATCH },
	{ &ui_menuFontBanner, "ui_menuFontBanner", "", CVAR_ARCHIVE | CVAR_LATCH },
};

static int cvarTableSize = ARRAY_LEN( cvarTable );


/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
	}
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Update( cv->vmCvar );
	}
}


/*
=================
UI_RetrieveGametypeNum
=================
*/
int UI_RetrieveGametypeNum( void ) {
	int num = (int)trap_Cvar_VariableValue( "g_gameType" );
#if 0
	if ( num < 0 ) num = 0;
	else if ( num >= GT_MAX_GAME_TYPE ) num = GT_MAX_GAME_TYPE - 1;
#endif
	return num;
}


/*
=================
UI_RetrieveGametypeNumFromInfo
=================
*/
int UI_RetrieveGametypeNumFromInfo( char info[MAX_INFO_STRING] ) {
	int num = atoi( Info_ValueForKey( info, "g_gameType" ) );
#if 0
	if ( num < 0 ) num = 0;
	else if ( num >= GT_MAX_GAME_TYPE ) num = GT_MAX_GAME_TYPE - 1;
#endif
	return num;
}
