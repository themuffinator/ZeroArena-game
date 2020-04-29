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
//
// ui_team.c
//

#include "ui_local.h"


#define TEAMMAIN_FRAME	"menu/art/cut_frame"

#define ID_JOINRED		100
#define ID_JOINBLUE		101
#define ID_JOINGREEN	102
#define ID_JOINYELLOW	103
#define ID_JOINTEAL		104
#define ID_JOINPINK		105
#define ID_JOINGAME		106
#define ID_SPECTATE		107


typedef struct
{
	menuframework_s	menu;
	menubitmap_s	frame;
	menutext_s		joinTeam[TEAM_NUM_TEAMS];
	menutext_s		joingame;
	menutext_s		spectate;

	int				localPlayerNum;
} teammain_t;

static teammain_t	s_teammain;

/*
===============
TeamMain_MenuEvent
===============
*/
static void TeamMain_MenuEvent( void* ptr, int event ) {
	char *teamCmd;

	if( event != QM_ACTIVATED ) {
		return;
	}

	teamCmd = Com_LocalPlayerCvarName(s_teammain.localPlayerNum, "team");

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_JOINRED:
		trap_Cmd_ExecuteText( EXEC_APPEND, va("cmd %s red\n", teamCmd) );
		UI_ForceMenuOff();
		break;

	case ID_JOINBLUE:
		trap_Cmd_ExecuteText( EXEC_APPEND, va("cmd %s blue\n", teamCmd) );
		UI_ForceMenuOff();
		break;

	case ID_JOINGREEN:
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "cmd %s green\n", teamCmd ) );
		UI_ForceMenuOff();
		break;

	case ID_JOINYELLOW:
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "cmd %s yellow\n", teamCmd ) );
		UI_ForceMenuOff();
		break;

	case ID_JOINTEAL:
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "cmd %s teal\n", teamCmd ) );
		UI_ForceMenuOff();
		break;

	case ID_JOINPINK:
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "cmd %s pink\n", teamCmd ) );
		UI_ForceMenuOff();
		break;

	case ID_JOINGAME:
		trap_Cmd_ExecuteText( EXEC_APPEND, va("cmd %s free\n", teamCmd) );
		UI_ForceMenuOff();
		break;

	case ID_SPECTATE:
		trap_Cmd_ExecuteText( EXEC_APPEND, va("cmd %s spectator\n", teamCmd) );
		UI_ForceMenuOff();
		break;
	}
}


/*
===============
TeamMain_MenuInit
===============
*/
void TeamMain_MenuInit( int localPlayerNum ) {
	int				y;
	int				gametype;
	char			info[MAX_INFO_STRING];
	qboolean		teams;
	const int		maxTeams = (int)trap_Cvar_VariableValue( "g_teamTotal_max" );

	memset( &s_teammain, 0, sizeof(s_teammain) );

	s_teammain.localPlayerNum = localPlayerNum;

	TeamMain_Cache();

	trap_GetConfigString( CS_SERVERINFO, info, MAX_INFO_STRING );
	gametype = UI_RetrieveGametypeNumFromInfo( info );
	teams = gt[gametype].gtFlags & GTF_TEAMS;

	s_teammain.menu.wrapAround = qtrue;
	s_teammain.menu.fullscreen = qfalse;

	s_teammain.frame.generic.type   = MTYPE_BITMAP;
	s_teammain.frame.generic.flags	= QMF_INACTIVE;
	s_teammain.frame.generic.name   = TEAMMAIN_FRAME;
	s_teammain.frame.generic.x		= 142;
	s_teammain.frame.generic.y		= 118;
	s_teammain.frame.width			= 359;
	s_teammain.frame.height			= 256;

	//y = 194;
	y = 234 - ((teams ? (maxTeams + 1) / 2 : 1) * 20);
	if ( teams ) {
		s_teammain.joinTeam[TEAM_RED].generic.type = MTYPE_PTEXT;
		s_teammain.joinTeam[TEAM_RED].generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
		s_teammain.joinTeam[TEAM_RED].generic.id = ID_JOINRED;
		s_teammain.joinTeam[TEAM_RED].generic.callback = TeamMain_MenuEvent;
		s_teammain.joinTeam[TEAM_RED].generic.x = 320;
		s_teammain.joinTeam[TEAM_RED].generic.y = y;
		s_teammain.joinTeam[TEAM_RED].string = "JOIN RED";
		s_teammain.joinTeam[TEAM_RED].style = UI_CENTER | UI_SMALLFONT;
		s_teammain.joinTeam[TEAM_RED].color = colorRed;
		y += 20;

		s_teammain.joinTeam[TEAM_BLUE].generic.type = MTYPE_PTEXT;
		s_teammain.joinTeam[TEAM_BLUE].generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
		s_teammain.joinTeam[TEAM_BLUE].generic.id = ID_JOINBLUE;
		s_teammain.joinTeam[TEAM_BLUE].generic.callback = TeamMain_MenuEvent;
		s_teammain.joinTeam[TEAM_BLUE].generic.x = 320;
		s_teammain.joinTeam[TEAM_BLUE].generic.y = y;
		s_teammain.joinTeam[TEAM_BLUE].string = "JOIN BLUE";
		s_teammain.joinTeam[TEAM_BLUE].style = UI_CENTER | UI_SMALLFONT;
		s_teammain.joinTeam[TEAM_BLUE].color = colorBlue;
		y += 20;
		if ( maxTeams > 2 ) {
			s_teammain.joinTeam[TEAM_GREEN].generic.type = MTYPE_PTEXT;
			s_teammain.joinTeam[TEAM_GREEN].generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
			s_teammain.joinTeam[TEAM_GREEN].generic.id = ID_JOINGREEN;
			s_teammain.joinTeam[TEAM_GREEN].generic.callback = TeamMain_MenuEvent;
			s_teammain.joinTeam[TEAM_GREEN].generic.x = 320;
			s_teammain.joinTeam[TEAM_GREEN].generic.y = y;
			s_teammain.joinTeam[TEAM_GREEN].string = "JOIN GREEN";
			s_teammain.joinTeam[TEAM_GREEN].style = UI_CENTER | UI_SMALLFONT;
			s_teammain.joinTeam[TEAM_GREEN].color = colorGreen;
			y += 20;
		}
		if ( maxTeams > 3 ) {
			s_teammain.joinTeam[TEAM_YELLOW].generic.type = MTYPE_PTEXT;
			s_teammain.joinTeam[TEAM_YELLOW].generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
			s_teammain.joinTeam[TEAM_YELLOW].generic.id = ID_JOINYELLOW;
			s_teammain.joinTeam[TEAM_YELLOW].generic.callback = TeamMain_MenuEvent;
			s_teammain.joinTeam[TEAM_YELLOW].generic.x = 320;
			s_teammain.joinTeam[TEAM_YELLOW].generic.y = y;
			s_teammain.joinTeam[TEAM_YELLOW].string = "JOIN YELLOW";
			s_teammain.joinTeam[TEAM_YELLOW].style = UI_CENTER | UI_SMALLFONT;
			s_teammain.joinTeam[TEAM_YELLOW].color = colorYellow;
			y += 20;
		}
		if ( maxTeams > 4 ) {
			s_teammain.joinTeam[TEAM_TEAL].generic.type = MTYPE_PTEXT;
			s_teammain.joinTeam[TEAM_TEAL].generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
			s_teammain.joinTeam[TEAM_TEAL].generic.id = ID_JOINTEAL;
			s_teammain.joinTeam[TEAM_TEAL].generic.callback = TeamMain_MenuEvent;
			s_teammain.joinTeam[TEAM_TEAL].generic.x = 320;
			s_teammain.joinTeam[TEAM_TEAL].generic.y = y;
			s_teammain.joinTeam[TEAM_TEAL].string = "JOIN TEAL";
			s_teammain.joinTeam[TEAM_TEAL].style = UI_CENTER | UI_SMALLFONT;
			s_teammain.joinTeam[TEAM_TEAL].color = colorCyan;
		y += 20;
		}
		if ( maxTeams > 5 ) {
			s_teammain.joinTeam[TEAM_PINK].generic.type = MTYPE_PTEXT;
			s_teammain.joinTeam[TEAM_PINK].generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
			s_teammain.joinTeam[TEAM_PINK].generic.id = ID_JOINPINK;
			s_teammain.joinTeam[TEAM_PINK].generic.callback = TeamMain_MenuEvent;
			s_teammain.joinTeam[TEAM_PINK].generic.x = 320;
			s_teammain.joinTeam[TEAM_PINK].generic.y = y;
			s_teammain.joinTeam[TEAM_PINK].string = "JOIN PINK";
			s_teammain.joinTeam[TEAM_PINK].style = UI_CENTER | UI_SMALLFONT;
			s_teammain.joinTeam[TEAM_PINK].color = colorMagenta;
			y += 20;
		}
	} else {
		s_teammain.joingame.generic.type = MTYPE_PTEXT;
		s_teammain.joingame.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
		s_teammain.joingame.generic.id = ID_JOINGAME;
		s_teammain.joingame.generic.callback = TeamMain_MenuEvent;
		s_teammain.joingame.generic.x = 320;
		s_teammain.joingame.generic.y = y;
		s_teammain.joingame.string = "JOIN GAME";
		s_teammain.joingame.style = UI_CENTER | UI_SMALLFONT;
		s_teammain.joingame.color = colorYellow;
		y += 20;
	}

	s_teammain.spectate.generic.type     = MTYPE_PTEXT;
	s_teammain.spectate.generic.flags    = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_teammain.spectate.generic.id       = ID_SPECTATE;
	s_teammain.spectate.generic.callback = TeamMain_MenuEvent;
	s_teammain.spectate.generic.x        = 320;
	s_teammain.spectate.generic.y        = y;
	s_teammain.spectate.string           = "SPECTATE";
	s_teammain.spectate.style            = UI_CENTER|UI_SMALLFONT;
	s_teammain.spectate.color            = colorWhite;
#if 0
	// set initial states
	if ( teams ) {
		s_teammain.joingame.generic.flags |= QMF_GRAYED;
	} else {
		s_teammain.joinTeam[TEAM_RED].generic.flags |= QMF_GRAYED;
		s_teammain.joinTeam[TEAM_BLUE].generic.flags |= QMF_GRAYED;
	}
#endif

	Menu_AddItem( &s_teammain.menu, (void*) &s_teammain.frame );
	if ( teams ) {
		Menu_AddItem( &s_teammain.menu, (void*)&s_teammain.joinTeam[TEAM_RED] );
		Menu_AddItem( &s_teammain.menu, (void*)&s_teammain.joinTeam[TEAM_BLUE] );
		if ( maxTeams > 2 ) Menu_AddItem( &s_teammain.menu, (void*)&s_teammain.joinTeam[TEAM_GREEN] );
		if ( maxTeams > 3 ) Menu_AddItem( &s_teammain.menu, (void*)&s_teammain.joinTeam[TEAM_YELLOW] );
		if ( maxTeams > 4 ) Menu_AddItem( &s_teammain.menu, (void*)&s_teammain.joinTeam[TEAM_TEAL] );
		if ( maxTeams > 5 ) Menu_AddItem( &s_teammain.menu, (void*)&s_teammain.joinTeam[TEAM_PINK] );
	} else {
		Menu_AddItem( &s_teammain.menu, (void*)&s_teammain.joingame );
	}
	Menu_AddItem( &s_teammain.menu, (void*) &s_teammain.spectate );
}


/*
===============
TeamMain_Cache
===============
*/
void TeamMain_Cache( void ) {
	trap_R_RegisterShaderNoMip( TEAMMAIN_FRAME );
}


/*
===============
UI_TeamMainMenu
===============
*/
void UI_TeamMainMenu( int localPlayerNum ) {
	TeamMain_MenuInit( localPlayerNum );
	UI_PushMenu ( &s_teammain.menu );
}
