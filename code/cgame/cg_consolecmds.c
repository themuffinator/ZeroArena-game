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
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#include "../ui/ui_public.h"
#if 0
#ifdef MISSIONPACK
#include "../ui/ui_shared.h"
#endif
#endif


void CG_TargetCommand_f( int localPlayerNum ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer( localPlayerNum );
	if ( targetNum == -1 ) {
		return;
	}

	trap_Argv( 1, test, 4 );
	trap_SendClientCommand( va( "%s %i %i", Com_LocalPlayerCvarName( localPlayerNum, "gc" ), targetNum, atoi( test ) ) );
}



/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f (void) {
	// manually clamp here so cvar range warning isn't shown
	trap_Cvar_SetValue("cg_viewSize", Com_Clamp( 30, 100, (int)(cg_viewSize.integer+10) ) );
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f (void) {
	// manually clamp here so cvar range warning isn't shown
	trap_Cvar_SetValue("cg_viewSize", Com_Clamp( 30, 100, (int)(cg_viewSize.integer-10) ) );
}

/*
================
CG_MessageMode_f
================
*/
void CG_MessageMode_f( void ) {
	Q_strncpyz( cg.messageCommand, "say", sizeof (cg.messageCommand) );
	Q_strncpyz( cg.messagePrompt, "Say: ", sizeof (cg.messagePrompt) );
	MField_Clear( &cg.messageField );
	cg.messageField.widthInChars = 30;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
CG_MessageMode2_f
================
*/
void CG_MessageMode2_f( void ) {
	Q_strncpyz( cg.messageCommand, "say_team", sizeof (cg.messageCommand) );
	Q_strncpyz( cg.messagePrompt, "Team Say: ", sizeof (cg.messagePrompt) );
	MField_Clear( &cg.messageField );
	cg.messageField.widthInChars = 25;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
CG_MessageMode3_f
================
*/
void CG_MessageMode3_f( void ) {
	int playerNum = CG_CrosshairPlayer( 0 );
	if ( playerNum < 0 || playerNum >= MAX_CLIENTS ) {
		return;
	}
	Com_sprintf( cg.messageCommand, sizeof (cg.messageCommand), "tell %d", playerNum );
	Com_sprintf( cg.messagePrompt, sizeof (cg.messagePrompt), "Tell %s: ", cgs.playerinfo[ playerNum ].name );
	MField_Clear( &cg.messageField );
	cg.messageField.widthInChars = 30;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
CG_MessageMode4_f
================
*/
void CG_MessageMode4_f( void ) {
	int playerNum = CG_LastAttacker( 0 );
	if ( playerNum < 0 || playerNum >= MAX_CLIENTS ) {
		return;
	}
	Com_sprintf( cg.messageCommand, sizeof (cg.messageCommand), "tell %d", playerNum );
	Com_sprintf( cg.messagePrompt, sizeof (cg.messagePrompt), "Tell %s: ", cgs.playerinfo[ playerNum ].name );
	MField_Clear( &cg.messageField );
	cg.messageField.widthInChars = 30;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}


/*
===================
CG_CenterEcho_f
===================
*/
void CG_CenterEcho_f( int localPlayerNum ) {
	char text[1024];

	trap_Args( text, sizeof( text ) );

	CG_ReplaceCharacter( text, '\\', '\n' );

	CG_CenterPrint( localPlayerNum, text, SCREEN_HEIGHT * 0.30, 0.5 );
}

/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f(int localPlayerNum) {
	CG_Printf(
		"(x:%i y:%i z:%i) : (pt:%i yw:%i rl:%i)\n",
		(int)cg.refdef.vieworg[0],
		(int)cg.refdef.vieworg[1],
		(int)cg.refdef.vieworg[2],
		(int)cg.refdefViewAngles[PITCH],
		(int)cg.refdefViewAngles[YAW],
		(int)cg.refdefViewAngles[ROLL]
		);
}

/*
=============
CG_ScoresDown
=============
*/
static void CG_ScoresDown_f(int localPlayerNum) {
	localPlayer_t *player = &cg.localPlayers[localPlayerNum];

	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !CG_AnyScoreboardShowing() ) {
			cg.numScores = 0;
		}

		player->showScores = qtrue;
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		player->showScores = qtrue;
	}
}


/*
=============
CG_ScoresUp_f
=============
*/
static void CG_ScoresUp_f( int localPlayerNum ) {
	localPlayer_t *player = &cg.localPlayers[localPlayerNum];

	if ( player->showScores ) {
		player->showScores = qfalse;
		player->scoreFadeTime = cg.time;
	}
}



/*
=============
CG_NotifyDown_f
=============
*/
static void CG_NotifyDown_f( int localPlayerNum ) {
	cg.notifyExpand = qtrue;
}


/*
=============
CG_NotifyUp_f
=============
*/
static void CG_NotifyUp_f( int localPlayerNum ) {
	cg.notifyExpand = qfalse;
}


/*
=============
CG_SetModel_f
=============
*/
void CG_SetModel_f( int localPlayerNum ) {
	const char	*arg;
	char	name[256];
	char	cvarName[32];

	Q_strncpyz( cvarName, Com_LocalPlayerCvarName( localPlayerNum, "model"), sizeof (cvarName) );

	arg = CG_Argv( 1 );
	if ( arg[0] ) {
		trap_Cvar_Set( cvarName, arg );
		trap_Cvar_Set( Com_LocalPlayerCvarName( localPlayerNum, "headModel"), arg );
	} else {
		trap_Cvar_VariableStringBuffer( cvarName, name, sizeof(name) );
		Com_Printf("%s is set to %s\n", cvarName, name);
	}
}

/*
=============
CG_SetHeadmodel_f
=============
*/
void CG_SetHeadmodel_f( int localPlayerNum ) {
	const char	*arg;
	char	name[256];
	char	cvarName[32];

	Q_strncpyz( cvarName, Com_LocalPlayerCvarName( localPlayerNum, "headModel"), sizeof (cvarName) );

	arg = CG_Argv( 1 );
	if ( arg[0] ) {
		trap_Cvar_Set( cvarName, arg );
	} else {
		trap_Cvar_VariableStringBuffer( cvarName, name, sizeof(name) );
		Com_Printf("%s is set to %s\n", cvarName, name);
	}
}


/*
==================
CG_Field_CompletePlayerModel
==================
*/
static void CG_Field_CompletePlayerModel( int argNum, qboolean lookingForHead, char *lookingForSkin, team_t lookingForTeam ) {
	int		numdirs;
	int		numfiles;
	char	dirlist[2048];
	char	filelist[2048];
	char	skinname[MAX_QPATH];
	int		skinnameLength;
	char*	dirptr;
	char*	fileptr;
	char*	skinptr;
	int		i;
	int		j;
	int		dirlen;
	int		filelen;
	char	list[8192];
	int		listTotalLength;
	char	completeModel[MAX_QPATH];
	int		completeModelLength;
	char	teamName[MAX_QPATH];
	int		teamNameLength;
	char	*skinPrefix;
	int		skinPrefixLength;
	char	*skinTeamSuffix;
	int		skinTeamSuffixLength;

	trap_Argv( argNum - 1, completeModel, sizeof( completeModel ) );

	// remove skin
	completeModelLength = 0;
	while ( completeModel[completeModelLength] != '\0' && completeModel[completeModelLength] != '/' ) {
		completeModelLength++;
	}
	completeModel[completeModelLength] = '\0';

	teamName[0] = 0;
#ifdef MISSIONPACK
	if ( lookingForTeam != TEAM_FREE ) {
		if ( lookingForTeam == TEAM_BLUE ) {
			Q_strncpyz( teamName, cg_blueTeamName.string, sizeof( teamName ) );
		} else {
			Q_strncpyz( teamName, cg_redTeamName.string, sizeof( teamName ) );
		}
	}
	if ( teamName[0] ) {
		strcat( teamName, "/" );
	}
#endif
	teamNameLength = strlen( teamName );

	skinPrefix = ( lookingForHead ) ? "head_" : "upper_";
	skinPrefixLength = ( lookingForHead ) ? 5 : 6;

	skinTeamSuffix = "_bright";
	skinTeamSuffixLength = 8;

	// ZTM: FIXME: have to clear whole list because BG_AddStringToList doesn't properly terminate list
	memset( list, 0, sizeof( list ) );
	listTotalLength = 0;

	if ( !lookingForHead || completeModelLength == 0 || completeModel[0] != '*' ) {
		// iterate directory of all player models
		numdirs = trap_FS_GetFileList("models/players", "/", dirlist, 2048 );
		dirptr  = dirlist;
		for (i=0; i<numdirs; i++,dirptr+=dirlen+1)
		{
			dirlen = strlen(dirptr);

			if ( dirlen == 0 )
				continue;

			if (dirptr[dirlen-1]=='/')
				dirptr[dirlen-1]='\0';

			if (!strcmp(dirptr,".") || !strcmp(dirptr,"..") || !strcmp(dirptr,"heads"))
				continue;

			// not a partial match
			if ( completeModelLength > 0 && Q_stricmpn( completeModel, dirptr, completeModelLength ) != 0 ) {
				continue;
			}

			// iterate all skin files in directory
			numfiles = trap_FS_GetFileList( va("models/players/%s",dirptr), "skin", filelist, 2048 );
			fileptr  = filelist;
			for (j=0; j<numfiles;j++,fileptr+=filelen+1)
			{
				filelen = strlen(fileptr);
				skinptr = fileptr;

				// models/players/example/stroggs/upper_lily_red.skin
				if ( teamNameLength > 0 && Q_stricmpn( skinptr, teamName, teamNameLength ) == 0 ) {
					skinptr += teamNameLength;
				}

				// look for upper_???? or head_????
				if ( Q_stricmpn( skinptr, skinPrefix, skinPrefixLength ) != 0 ) {
					continue;
				}

				COM_StripExtension( skinptr + skinPrefixLength, skinname, sizeof( skinname ) );

				if ( Q_stricmp( skinname, lookingForSkin ) == 0 ) {
					// models/players/example/upper_default.skin
					// add default skin as just the model name
					// for team models this is red or blue
					BG_AddStringToList( list, sizeof( list ), &listTotalLength, dirptr );
				} else if ( lookingForTeam != TEAM_FREE ) {
					// models/players/example/upper_lily_red.skin
					// for team model add lily_red skin as lily
					skinnameLength = strlen( skinname );

					if ( skinnameLength - skinPrefixLength > skinTeamSuffixLength
							&& COM_CompareExtension( skinname, skinTeamSuffix ) ) {
						// remove _red
						skinname[skinnameLength - 1 - skinTeamSuffixLength] = '\0';
						BG_AddStringToList( list, sizeof( list ), &listTotalLength, va( "%s/%s", dirptr, skinname ) );
					}
				} else {
					// models/players/example/upper_lily.skin
					// misc ffa skins
					BG_AddStringToList( list, sizeof( list ), &listTotalLength, va( "%s/%s", dirptr, skinname ) );
				}
			}
		}
	}

	if ( lookingForHead && ( completeModelLength == 0 || completeModel[0] == '*' ) ) {
		// iterate directory of all head models
		numdirs = trap_FS_GetFileList("models/players/heads", "/", dirlist, 2048 );
		dirptr  = dirlist;
		for (i=0; i<numdirs; i++,dirptr+=dirlen+1)
		{
			dirlen = strlen(dirptr);

			if ( dirlen == 0 )
				continue;

			if (dirptr[dirlen-1]=='/')
				dirptr[dirlen-1]='\0';

			if (!strcmp(dirptr,".") || !strcmp(dirptr,".."))
				continue;

			// not a partial match
			// completeModel[0] is '*' which means heads directory
			if ( completeModelLength > 1 && Q_stricmpn( completeModel+1, dirptr, completeModelLength-1 ) != 0 ) {
				continue;
			}

			// iterate all skin files in directory
			numfiles = trap_FS_GetFileList( va("models/players/heads/%s",dirptr), "skin", filelist, 2048 );
			fileptr  = filelist;
			for (j=0; j<numfiles;j++,fileptr+=filelen+1)
			{
				filelen = strlen(fileptr);
				skinptr = fileptr;

				// models/players/heads/example/stroggs/head_lily_red.skin
				if ( teamNameLength > 0 && Q_stricmpn( skinptr, teamName, teamNameLength ) == 0 ) {
					skinptr += teamNameLength;
				}

				// look for upper_???? or head_????
				if ( Q_stricmpn( skinptr, skinPrefix, skinPrefixLength ) != 0 ) {
					continue;
				}

				COM_StripExtension( skinptr + skinPrefixLength, skinname, sizeof( skinname ) );

				if ( Q_stricmp( skinname, lookingForSkin ) == 0 ) {
					// models/players/heads/example/head_default.skin
					// add default skin as just the model name
					// for team models this is red or blue
					BG_AddStringToList( list, sizeof( list ), &listTotalLength, va( "*%s", dirptr ) );
				} else if ( lookingForTeam != TEAM_FREE ) {
					// models/players/heads/example/head_lily_red.skin
					// for team model add lily_red skin as lily
					skinnameLength = strlen( skinname );

					if ( skinnameLength - skinPrefixLength > skinTeamSuffixLength
							&& COM_CompareExtension( skinname, skinTeamSuffix ) ) {
						// remove _red
						skinname[skinnameLength - 1 - skinTeamSuffixLength] = '\0';
						BG_AddStringToList( list, sizeof( list ), &listTotalLength, va( "*%s/%s", dirptr, skinname ) );
					}
				} else {
					// models/players/heads/example/head_lily.skin
					// misc ffa skins
					BG_AddStringToList( list, sizeof( list ), &listTotalLength, va( "*%s/%s", dirptr, skinname ) );
				}
			}
		}
	}

	if ( listTotalLength > 0 ) {
		list[listTotalLength++] = 0;
		trap_Field_CompleteList( list );
	}
}

/*
==================
CG_ModelComplete
==================
*/
static void CG_ModelComplete( int localPlayerNum, char *args, int argNum ) {
	if ( argNum == 2 ) {
		CG_Field_CompletePlayerModel( argNum, qfalse, "bright", TEAM_FREE );
	}
}

/*
==================
CG_HeadmodelComplete
==================
*/
static void CG_HeadmodelComplete( int localPlayerNum, char *args, int argNum ) {
	if ( argNum == 2 ) {
		CG_Field_CompletePlayerModel( argNum, qtrue, "bright", TEAM_FREE );
	}
}


static void CG_CameraOrbit( int localPlayerNum, float speed ) {
	localPlayer_t *player;

	player = &cg.localPlayers[localPlayerNum];

	player->cameraOrbit = speed;
	player->cameraOrbitAngle = 0;
	player->cameraOrbitRange = 100;
}

#ifdef MISSIONPACK
static void CG_spWin_f( void) {
	CG_CameraOrbit( 0, 30 );
	CG_AddBufferedSound(cgs.media.winnerSound);
	//trap_S_StartLocalSound(cgs.media.winnerSound, CHAN_ANNOUNCER);
	CG_GlobalCenterPrint("YOU WIN!", SCREEN_HEIGHT/2, 1.0);
}

static void CG_spLose_f( void) {
	CG_CameraOrbit( 0, 30 );
	CG_AddBufferedSound(cgs.media.loserSound);
	//trap_S_StartLocalSound(cgs.media.loserSound, CHAN_ANNOUNCER);
	CG_GlobalCenterPrint("YOU LOSE...", SCREEN_HEIGHT/2, 1.0);
}

#endif

static void CG_TellTarget_f( int localPlayerNum ) {
	int		playerNum;
	char	command[MAX_SAY_TEXT + 16];
	char	message[MAX_SAY_TEXT];

	playerNum = CG_CrosshairPlayer( localPlayerNum );
	if ( playerNum == -1 ) {
		return;
	}

	trap_Args( message, sizeof( message ) );
	Com_sprintf( command, sizeof( command ), "%s %i %s", Com_LocalPlayerCvarName( localPlayerNum, "tell" ), playerNum, message );
	trap_SendClientCommand( command );
}

static void CG_TellAttacker_f( int localPlayerNum ) {
	int		playerNum;
	char	command[MAX_SAY_TEXT + 16];
	char	message[MAX_SAY_TEXT];

	playerNum = CG_LastAttacker( localPlayerNum );
	if ( playerNum == -1 ) {
		return;
	}

	trap_Args( message, sizeof( message ) );
	Com_sprintf( command, sizeof( command ), "%s %i %s", Com_LocalPlayerCvarName( localPlayerNum, "tell" ), playerNum, message );
	trap_SendClientCommand( command );
}


/*
==================
CG_VstrDown_f
==================
*/
static void CG_VstrDown_f( void ) {
	const char *cvarName;

	if ( trap_Argc() < 3 ) {
		Com_Printf( "+vstr <press variable name> <release variable name> : execute a variable command on key press and release\n" );
		return;
	}

	cvarName = CG_Argv( 1 );
	if ( *cvarName ) {
		trap_Cmd_ExecuteText( EXEC_NOW, va( "vstr %s\n", cvarName ) );
	}
}

/*
==================
CG_VstrUp_f
==================
*/
static void CG_VstrUp_f( void ) {
	const char *cvarName;

	if ( trap_Argc() < 3 ) {
		Com_Printf( "-vstr <press variable name> <release variable name> : execute a variable command on key press and release\n" );
		return;
	}

	cvarName = CG_Argv( 2 );
	if ( *cvarName ) {
		trap_Cmd_ExecuteText( EXEC_NOW, va( "vstr %s\n", cvarName ) );
	}
}

/*
==================
CG_VstrComplete

complete cvar name for second and third arguments for +vstr and -vstr
==================
*/
static void CG_VstrComplete( char *args, int argNum ) {
	if ( argNum == 2 || argNum == 3 ) {
		// Skip "<cmd> "
		char *p = Com_SkipTokens( args, argNum - 1, " " );

		if( p > args )
			trap_Field_CompleteCommand( p, qfalse, qtrue );
	}
}

/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f( void ) {
	int i;

	for ( i = 0; i < MAX_SPLITVIEW; i++ ) {
		if (cg.localPlayers[i].cameraOrbit != 0) {
			CG_CameraOrbit( i, 0 );
		} else {
			CG_CameraOrbit( i, 30 );
		}
	}
}

/*
static void CG_Camera_f( void ) {
	char name[1024];
	trap_Argv( 1, name, sizeof(name));
	if (trap_loadCamera(name)) {
		cg.cameraMode = qtrue;
		trap_startCamera(cg.time);
	} else {
		CG_Printf ("Unable to load camera %s\n",name);
	}
}
*/


void CG_GenerateTracemap(void)
{
	bgGenTracemap_t gen;

	if ( !cg.mapcoordsValid ) {
		CG_Printf( "Need valid mapcoords in the worldspawn to be able to generate a tracemap.\n" );
		return;
	}

	gen.trace = CG_Trace;
	gen.pointcontents = CG_PointContents;

	BG_GenerateTracemap(cgs.mapname, cg.mapcoordsMins, cg.mapcoordsMaxs, &gen);
}

/*
==================
CG_RemapShader_f
==================
*/
static void CG_RemapShader_f( void ) {
	char shader1[MAX_QPATH];
	char shader2[MAX_QPATH];
	char timeoffset[MAX_QPATH];

	if ( cg.connected && trap_Cvar_VariableIntegerValue( "sv_pure" ) ) {
		CG_Printf( "%s command cannot be used on pure servers.\n", CG_Argv( 0 ) );
		return;
	}

	if (trap_Argc() < 2 || trap_Argc() > 4) {
		CG_Printf( "Usage: %s <original shader> [new shader] [time offset]\n", CG_Argv( 0 ) );
		return;
	}

	Q_strncpyz( shader1, CG_Argv( 1 ), sizeof(shader1) );
	Q_strncpyz( shader2, CG_Argv( 2 ), sizeof( shader2 ) );

	if ( !strlen( shader2 ) ) {
		// reset shader remap
		Q_strncpyz( shader2, shader1, sizeof(shader2) );
	}

	Q_strncpyz( timeoffset, CG_Argv( 3 ), sizeof( timeoffset ) );

	trap_R_RemapShader( shader1, shader2, timeoffset );
}

/*
=================
CG_Play_f
=================
*/
void CG_Play_f( void ) {
	int 		i;
	int			c;
	sfxHandle_t	h;

	c = trap_Argc();

	if( c < 2 ) {
		Com_Printf ("Usage: play <sound filename> [sound filename] [sound filename] ...\n");
		return;
	}

	for( i = 1; i < c; i++ ) {
		h = trap_S_RegisterSound( CG_Argv( i ), qfalse );

		if( h ) {
			trap_S_StartLocalSound( h, CHAN_LOCAL_SOUND );
		}
	}
}

/*
=================
CG_PlayComplete
=================
*/
static void CG_PlayComplete( char *args, int argNum ) {
	trap_Field_CompleteFilename( "", "$sounds", qfalse, qfalse );
}

/*
=================
CG_Music_f
=================
*/
void CG_Music_f( void ) {
	int		c;
	char	intro[MAX_QPATH];
	char	loop[MAX_QPATH];
	float	volume;
	float	loopVolume;

	c = trap_Argc();

	if ( c < 2 || c > 5 ) {
		Com_Printf ("Usage: music <musicfile> [loopfile] [volume] [loopvolume]\n");
		return;
	}

	trap_Argv( 1, intro, sizeof( intro ) );
	trap_Argv( 2, loop, sizeof( loop ) );

	volume = loopVolume = 1.0f;

	if ( c > 3 ) {
		volume = loopVolume = atof( CG_Argv( 3 ) );
	}
	if ( c > 4 ) {
		loopVolume = atof( CG_Argv( 4 ) );
	}

	trap_S_StartBackgroundTrack( intro, loop, volume, loopVolume );
}

/*
=================
CG_MusicComplete
=================
*/
static void CG_MusicComplete( char *args, int argNum ) {
	if ( argNum == 2 || argNum == 3 ) {
		trap_Field_CompleteFilename( "", "$sounds", qfalse, qfalse );
	}
}

/*
=================
CG_StopMusic_f
=================
*/
void CG_StopMusic_f( void ) {
	trap_S_StopBackgroundTrack();
}

/*
=================
CG_StopCinematic_f
=================
*/
void CG_StopCinematic_f( void ) {
	if ( !cg.cinematicPlaying ) {
		return;
	}

	trap_S_StopAllSounds();
	trap_CIN_StopCinematic( cg.cinematicHandle );

	cg.cinematicHandle = 0;
	cg.cinematicPlaying = qfalse;
}

/*
=================
CG_Cinematic_f
=================
*/
void CG_Cinematic_f( void ) {
	char	arg[MAX_QPATH];
	char	s[6];
	float	x, y, width, height;
	int		bits = CIN_system;
	int		c;

	c = trap_Argc();

	if ( c < 2 || c > 3 ) {
		Com_Printf( "Usage: cinematic <videofile> [hold|loop]\n" );
		return;
	}

	if ( cg.cinematicPlaying ) {
		CG_StopCinematic_f();
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	trap_Argv( 2, s, sizeof( s ) );

	if (s[0] == '1' || Q_stricmp(s,"hold")==0 || Q_stricmp(arg,"demoend.roq")==0 || Q_stricmp(arg,"end.roq")==0) {
		bits |= CIN_hold;
	}
	if (s[0] == '2' || Q_stricmp(s,"loop")==0) {
		bits |= CIN_loop;
	}

	trap_S_StopAllSounds();

	x = 0;
	y = 0;
	width = SCREEN_WIDTH;
	height = SCREEN_HEIGHT;
	CG_SetScreenPlacement( PLACE_CENTER, PLACE_CENTER );
	CG_AdjustFrom640( &x, &y, &width, &height );

	cg.cinematicHandle = trap_CIN_PlayCinematic( arg, x, y, width, height, bits );
	if ( cg.cinematicHandle >= 0 ) {
		cg.cinematicPlaying = qtrue;
	}
}

/*
=================
CG_CinematicComplete
=================
*/
static void CG_CinematicComplete( char *args, int argNum ) {
	if ( argNum == 2 ) {
		trap_Field_CompleteFilename( "", "$videos", qfalse, qfalse );
	}
}

/*
===================
CG_ToggleMenu_f
===================
*/
void CG_ToggleMenu_f( void ) {
	CG_DistributeKeyEvent( K_ESCAPE, qtrue, trap_Milliseconds(), cg.connState, -1, 0 );
	CG_DistributeKeyEvent( K_ESCAPE, qfalse, trap_Milliseconds(), cg.connState, -1, 0 );
}

/*
===================
CG_ForwardToServer_f
===================
*/
static void CG_ForwardToServer_f( int localPlayerNum ) {
	char		buffer[BIG_INFO_STRING];

	if ( cg.connected && trap_GetDemoState() != DS_PLAYBACK ) {
		// the game server will interpret these commands
		trap_LiteralArgs( buffer, sizeof ( buffer ) );
		trap_SendClientCommand( buffer );
	}
}

/*
=================
CG_Field_CompletePlayerName
=================
*/
static void CG_Field_CompletePlayerName( int team, qboolean excludeTeam, qboolean excludeLocalPlayers ) {
	int		i;
	playerInfo_t	*pi;
	char name[MAX_QPATH];
	char list[MAX_CLIENTS * MAX_QPATH];
	int listTotalLength;

	if ( !cg.snap ) {
		return;
	}

	// ZTM: FIXME: have to clear whole list because BG_AddStringToList doesn't properly terminate list
	memset( list, 0, sizeof( list ) );
	listTotalLength = 0;

	for ( i = 0 ; i < cgs.maxplayers ; i++ ) {
		pi = &cgs.playerinfo[ i ];
		if ( !pi->infoValid ) {
			continue;
		}

		if ( team != -1 && ( ( excludeTeam && pi->team == team ) || ( !excludeTeam && pi->team != team ) ) ) {
			continue;
		}

		if ( excludeLocalPlayers && CG_LocalPlayerState( i ) ) {
			continue;
		}

		Q_strncpyz( name, pi->name, sizeof ( name ) );
		Q_CleanStr( name );

		// Use quotes if there is a space in the name
		if ( strchr( name, ' ' ) != NULL ) {
			BG_AddStringToList( list, sizeof( list ), &listTotalLength, va( "\"%s\"", name ) );
		} else {
			BG_AddStringToList( list, sizeof( list ), &listTotalLength, name );
		}
	}

	if ( listTotalLength > 0 ) {
		list[listTotalLength++] = 0;
		trap_Field_CompleteList( list );
	}
}

/*
=================
CG_TellComplete
=================
*/
static void CG_TellComplete( int localPlayerNum, char *args, int argNum ) {
	if ( argNum == 2 ) {
		CG_Field_CompletePlayerName( -1, qfalse, qfalse );
	}
}


/*
=================
CG_GiveComplete

Game VM combines all arguments but completion works on separate words.
This is kind of an ugly hack to avoid having to quote item names containing spaces.
=================
*/
static void CG_GiveComplete( int localPlayerNum, char *args, int argNum ) {
	char builtinNames[] = "all\0ammo\0armor\0assist\0defend\0excellent\0gauntletaward\0health\0impressive\0weapons\0";
	char list[2048], *name, *typedName;
	int i, j, listTotalLength, typedNameLength;
	gitem_t *item;

	// ZTM: FIXME: have to clear whole list because BG_AddStringToList doesn't properly terminate list
	memset( list, 0, sizeof( list ) );
	listTotalLength = 0;

	if ( argNum == 2 ) {
		memcpy( list, builtinNames, ARRAY_LEN( builtinNames ) - 1 );
		listTotalLength = ARRAY_LEN( builtinNames ) - 1;
	}

	// skip "give " or NULL if no space character
	typedName = strchr( args, ' ' );
	if ( typedName ) {
		typedName++;
		typedNameLength = strlen( typedName );
	} else {
		typedNameLength = 0;
	}

	for ( i = 1; i < BG_NumItems(); i++ ) {
		item = BG_ItemForItemNum( i );
		name = item->pickup_name;

		//
		// complete item names with spaces across multiple arguments instead of adding quotes
		//

		// check if matches previously typed words
		if ( typedName && Q_stricmpn( typedName, name, typedNameLength ) ) {
			continue;
		}

		// find current word for argument
		for ( j = 0; j < argNum - 2 && name; j++ ) {
			name = strchr( name, ' ' );
			if ( name ) {
				name++;
			}
		}

		if ( name && *name ) {
			BG_AddStringToList( list, sizeof( list ), &listTotalLength, name );
		}
	}

	if ( listTotalLength > 0 ) {
		list[listTotalLength++] = 0;
		trap_Field_CompleteList( list );
	}
}

/*
=================
CG_FollowComplete
=================
*/
static void CG_FollowComplete( int localPlayerNum, char *args, int argNum ) {
	if ( argNum == 2 ) {
		CG_Field_CompletePlayerName( TEAM_SPECTATOR, qtrue, qtrue );
	}
}

/*
=================
CG_TeamComplete
=================
*/
static void CG_TeamComplete( int localPlayerNum, char *args, int argNum ) {
	if ( argNum == 2 ) {
		trap_Field_CompleteList( "blue\0follow1\0follow2\0free\0red\0scoreboard\0spectator\0" );
	}
}

/*
=================
CG_CallVoteComplete
=================
*/
static void CG_CallVoteComplete( int localPlayerNum, char *args, int argNum ) {
	if ( argNum == 2 ) {
		trap_Field_CompleteList( "scoreLimit\0g_doWarmup\0g_gameType\0g_instagib\0kick\0kickNum\0map\0map_restart\0nextMap\0timeLimit\0" );
	}
	if ( argNum == 3 && !Q_stricmp( CG_Argv( 1 ), "kick" ) ) {
		CG_Field_CompletePlayerName( -1, qfalse, qfalse );
	}
	if ( argNum == 3 && !Q_stricmp( CG_Argv( 1 ), "map" ) ) {
		trap_Field_CompleteFilename( "maps", ".bsp", qtrue, qfalse );
	}
}

/*
=================
CG_VoteComplete
=================
*/
static void CG_VoteComplete( int localPlayerNum, char *args, int argNum ) {
	if ( argNum == 2 ) {
		trap_Field_CompleteList( "no\0yes\0" );
	}
}

static consoleCommand_t	cg_commands[] = {
	{ "+vstr", CG_VstrDown_f, 0, CG_VstrComplete },
	{ "-vstr", CG_VstrUp_f, 0, CG_VstrComplete },
	{ "testGun", CG_TestGun_f, CMD_INGAME, CG_TestModelComplete },
	{ "testModel", CG_TestModel_f, CMD_INGAME, CG_TestModelComplete },
	{ "nextFrame", CG_TestModelNextFrame_f, CMD_INGAME },
	{ "previousFrame", CG_TestModelPrevFrame_f, CMD_INGAME },
	{ "nextSkin", CG_TestModelNextSkin_f, CMD_INGAME },
	{ "previousSkin", CG_TestModelPrevSkin_f, CMD_INGAME },
	{ "sizeUp", CG_SizeUp_f, 0 },
	{ "sizeDown", CG_SizeDown_f, 0 },
#ifdef MISSIONPACK
	{ "spWin", CG_spWin_f, CMD_INGAME },
	{ "spLose", CG_spLose_f, CMD_INGAME },
#endif
	{ "startOrbit", CG_StartOrbit_f, CMD_INGAME },
	//{ "camera", CG_Camera_f, CMD_INGAME },
	{ "loadDeferred", CG_LoadDeferredPlayers, CMD_INGAME },
	{ "generateTracemap", CG_GenerateTracemap, CMD_INGAME },
	{ "remapShader", CG_RemapShader_f, 0 },
	{ "play", CG_Play_f, 0, CG_PlayComplete },
	{ "music", CG_Music_f, 0, CG_MusicComplete },
	{ "stopMusic", CG_StopMusic_f, 0 },
	{ "cinematic", CG_Cinematic_f, 0, CG_CinematicComplete },
	{ "stopCinematic", CG_StopCinematic_f, 0 },
	{ "messageMode", CG_MessageMode_f },
	{ "messageMode2", CG_MessageMode2_f },
	{ "messageMode3", CG_MessageMode3_f },
	{ "messageMode4", CG_MessageMode4_f },
	{ "clear", Con_ClearConsole_f },
	{ "toggleConsole", Con_ToggleConsole_f },
	{ "toggleMenu", CG_ToggleMenu_f }
};

static int cg_numCommands = ARRAY_LEN( cg_commands );

typedef struct {
	char	*cmd;
	void	(*function)(int);
	int		flags;
	void	(*complete)(int, char *, int);
} playerConsoleCommand_t;

static playerConsoleCommand_t	playerCommands[] = {
	{ "+attack", IN_Button0Down, 0 },
	{ "-attack", IN_Button0Up, 0 },
	{ "+back",IN_BackDown, 0 },
	{ "-back",IN_BackUp, 0 },
	{ "+button0", IN_Button0Down, 0 },
	{ "-button0", IN_Button0Up, 0 },
	{ "+button10", IN_Button10Down, 0 },
	{ "-button10", IN_Button10Up, 0 },
	{ "+button11", IN_Button11Down, 0 },
	{ "-button11", IN_Button11Up, 0 },
	{ "+button12", IN_Button12Down, 0 },
	{ "-button12", IN_Button12Up, 0 },
	{ "+button13", IN_Button13Down, 0 },
	{ "-button13", IN_Button13Up, 0 },
	{ "+button14", IN_Button14Down, 0 },
	{ "-button14", IN_Button14Up, 0 },
	{ "+button1", IN_Button1Down, 0 },
	{ "-button1", IN_Button1Up, 0 },
	{ "+button2", IN_Button2Down, 0 },
	{ "-button2", IN_Button2Up, 0 },
	{ "+button3", IN_Button3Down, 0 },
	{ "-button3", IN_Button3Up, 0 },
	{ "+button4", IN_Button4Down, 0 },
	{ "-button4", IN_Button4Up, 0 },
	{ "+button5", IN_Button5Down, 0 },
	{ "-button5", IN_Button5Up, 0 },
	{ "+button6", IN_Button6Down, 0 },
	{ "-button6", IN_Button6Up, 0 },
	{ "+button7", IN_Button7Down, 0 },
	{ "-button7", IN_Button7Up, 0 },
	{ "+button8", IN_Button8Down, 0 },
	{ "-button8", IN_Button8Up, 0 },
	{ "+button9", IN_Button9Down, 0 },
	{ "-button9", IN_Button9Up, 0 },
	{ "+forward",IN_ForwardDown, 0 },
	{ "-forward",IN_ForwardUp, 0 },
	{ "+gesture",IN_Button3Down, 0 },
	{ "-gesture",IN_Button3Up, 0 },
	{ "+left",IN_LeftDown, 0 },
	{ "-left",IN_LeftUp, 0 },
	{ "+lookDown", IN_LookdownDown, 0 },
	{ "-lookDown", IN_LookdownUp, 0 },
	{ "+lookUp", IN_LookupDown, 0 },
	{ "-lookUp", IN_LookupUp, 0 },
	{ "+mouseLook", IN_MLookDown, 0 },
	{ "-mouseLook", IN_MLookUp, 0 },
	{ "+moveDown",IN_DownDown, 0 },
	{ "-moveDown",IN_DownUp, 0 },
	{ "+moveLeft", IN_MoveleftDown, 0 },
	{ "-moveLeft", IN_MoveleftUp, 0 },
	{ "+moveRight", IN_MoverightDown, 0 },
	{ "-moveRight", IN_MoverightUp, 0 },
	{ "+moveUp",IN_UpDown, 0 },
	{ "-moveUp",IN_UpUp, 0 },
	{ "+notify", CG_NotifyDown_f, CMD_INGAME },
	{ "-notify", CG_NotifyUp_f, CMD_INGAME },
	{ "+right",IN_RightDown, 0 },
	{ "-right",IN_RightUp, 0 },
	{ "+scores", CG_ScoresDown_f, CMD_INGAME },
	{ "-scores", CG_ScoresUp_f, CMD_INGAME },
	{ "+speed", IN_SpeedDown, 0 },
	{ "-speed", IN_SpeedUp, 0 },
	{ "+strafe", IN_StrafeDown, 0 },
	{ "-strafe", IN_StrafeUp, 0 },
	{ "+useItem",IN_Button2Down, 0 },
	{ "-useItem",IN_Button2Up, 0 },
	{ "+zoom", CG_ZoomDown_f, CMD_INGAME },
	{ "-zoom", CG_ZoomUp_f, CMD_INGAME },
	{ "centerEcho", CG_CenterEcho_f, CMD_INGAME },
	{ "centerView", IN_CenterView, 0 },
	{ "headModel", CG_SetHeadmodel_f, 0, CG_HeadmodelComplete },
	{ "targetCommand", CG_TargetCommand_f, CMD_INGAME },
	{ "tell_target", CG_TellTarget_f, CMD_INGAME },
	{ "tell_attacker", CG_TellAttacker_f, CMD_INGAME },
	{ "model", CG_SetModel_f, 0, CG_ModelComplete },
	{ "viewPos", CG_Viewpos_f, CMD_INGAME },
	{ "weaponNext", CG_NextWeapon_f, CMD_INGAME },
	{ "weaponPrevious", CG_PrevWeapon_f, CMD_INGAME },
	{ "weapon", CG_Weapon_f, CMD_INGAME },
	{ "weaponToggle", CG_WeaponToggle_f, CMD_INGAME },

	//
	// These commands will be forwarded to the server and interpret by the Game VM.
	//
	{ "say", CG_ForwardToServer_f, CMD_INGAME },
	{ "say_team", CG_ForwardToServer_f, CMD_INGAME },
	{ "tell", CG_ForwardToServer_f, CMD_INGAME, CG_TellComplete },
	{ "give", CG_ForwardToServer_f, CMD_INGAME, CG_GiveComplete },
	{ "god", CG_ForwardToServer_f, CMD_INGAME },
	{ "noTarget", CG_ForwardToServer_f, CMD_INGAME },
	{ "noClip", CG_ForwardToServer_f, CMD_INGAME },
	{ "where", CG_ForwardToServer_f, CMD_INGAME },
	{ "kill", CG_ForwardToServer_f, CMD_INGAME },
	{ "teamTask", CG_ForwardToServer_f, CMD_INGAME },
	{ "levelShot", CG_ForwardToServer_f, CMD_INGAME },
	{ "follow", CG_ForwardToServer_f, CMD_INGAME, CG_FollowComplete },
	{ "followNext", CG_ForwardToServer_f, CMD_INGAME },
	{ "followPrevious", CG_ForwardToServer_f, CMD_INGAME },
	{ "team", CG_ForwardToServer_f, CMD_INGAME, CG_TeamComplete },
	{ "readyUp", CG_ForwardToServer_f, CMD_INGAME },
	{ "callVote", CG_ForwardToServer_f, CMD_INGAME, CG_CallVoteComplete },
	{ "vote", CG_ForwardToServer_f, CMD_INGAME, CG_VoteComplete },
	{ "setViewPos", CG_ForwardToServer_f, CMD_INGAME },
	{ "stats", CG_ForwardToServer_f, CMD_INGAME }
};

static int numPlayerCommands = ARRAY_LEN( playerCommands );

/*
=================
CG_CheckCmdFlags
=================
*/
static qboolean CG_CheckCmdFlags( const char *cmd, int flags ) {
	if ( ( flags & CMD_INGAME ) && !cg.snap ) {
		CG_Printf("Must be in game to use command \"%s\"\n", cmd);
		return qtrue;
	}

	if ( ( flags & CMD_MENU ) && cg.connected ) {
		CG_Printf("Cannot use command \"%s\" while in game\n", cmd);
		return qtrue;
	}

	return qfalse;
}

/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( connstate_t state, int realTime ) {
	const char	*cmd;
	int		i;
	int		localPlayerNum;
	const char	*baseCmd;

	cg.connState = state;

	// update UI frame time
	UI_ConsoleCommand( realTime );

	cmd = CG_Argv(0);

	localPlayerNum = Com_LocalPlayerForCvarName( cmd );
	baseCmd = Com_LocalPlayerBaseCvarName( cmd );

	for ( i = 0 ; i < numPlayerCommands ; i++ ) {
		if ( !Q_stricmp( baseCmd, playerCommands[i].cmd )) {
			if ( CG_CheckCmdFlags( cmd, playerCommands[i].flags ) ) {
				return qtrue;
			}
			playerCommands[i].function( localPlayerNum );
			return qtrue;
		}
	}

	if ( localPlayerNum == 0 ) {
		for ( i = 0 ; i < cg_numCommands ; i++ ) {
			if ( !Q_stricmp( cmd, cg_commands[i].cmd )) {
				if ( CG_CheckCmdFlags( cmd, cg_commands[i].flags ) ) {
					return qtrue;
				}
				cg_commands[i].function();
				return qtrue;
			}
		}

		for ( i = 0 ; i < ui_numCommands ; i++ ) {
			if ( !Q_stricmp( cmd, ui_commands[i].cmd )) {
				if ( CG_CheckCmdFlags( cmd, ui_commands[i].flags ) ) {
					return qtrue;
				}
				ui_commands[i].function();
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
=================
CG_ConsoleCompleteArgument

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCompleteArgument( connstate_t state, int realTime, int completeArgument ) {
	char	args[BIG_INFO_STRING];
	char	cmd[MAX_TOKEN_CHARS];
	int		i;
	int		localPlayerNum;
	const char	*baseCmd;

	cg.connState = state;

	trap_Argv( 0, cmd, sizeof( cmd ) );
	trap_LiteralArgs( args, sizeof ( args ) );

	localPlayerNum = Com_LocalPlayerForCvarName( cmd );
	baseCmd = Com_LocalPlayerBaseCvarName( cmd );

	for ( i = 0 ; i < numPlayerCommands ; i++ ) {
		if ( playerCommands[i].complete && !Q_stricmp( baseCmd, playerCommands[i].cmd ) ) {
			playerCommands[i].complete( localPlayerNum, args, completeArgument );
			return qtrue;
		}
	}

	if ( localPlayerNum == 0 ) {
		for ( i = 0 ; i < cg_numCommands ; i++ ) {
			if ( cg_commands[i].complete && !Q_stricmp( cmd, cg_commands[i].cmd ) ) {
				cg_commands[i].complete( args, completeArgument );
				return qtrue;
			}
		}

		for ( i = 0 ; i < ui_numCommands ; i++ ) {
			if ( ui_commands[i].complete && !Q_stricmp( cmd, ui_commands[i].cmd ) ) {
				ui_commands[i].complete( args, completeArgument );
				return qtrue;
			}
		}
	}

	return qfalse;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	int		i, j;

	for ( i = 0 ; i < cg_numCommands ; i++ ) {
		trap_AddCommand( cg_commands[i].cmd );
	}

	for ( i = 0 ; i < ui_numCommands ; i++ ) {
		trap_AddCommand( ui_commands[i].cmd );
	}

	for ( i = 0 ; i < numPlayerCommands ; i++ ) {
		for ( j = 0; j < CG_MaxSplitView(); j++ ) {
			trap_AddCommand( Com_LocalPlayerCvarName( j, playerCommands[i].cmd ) );
		}
	}
}
