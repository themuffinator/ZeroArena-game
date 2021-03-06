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
// cg_info.c -- display information while data is being loading

#include "cg_local.h"
#include "../ui/ui_public.h"

#define MAX_LOADING_PLAYER_ICONS	16
#define MAX_LOADING_ITEM_ICONS		26

static int			loadingPlayerIconCount;
static int			loadingItemIconCount;
static qhandle_t	loadingPlayerIcons[MAX_LOADING_PLAYER_ICONS];
static qhandle_t	loadingItemIcons[MAX_LOADING_ITEM_ICONS];


/*
===================
CG_DrawLoadingIcons
===================
*/
static void CG_DrawLoadingIcons( void ) {
#if 0
	int		n;
	int		x, y;

	for ( n = 0; n < loadingPlayerIconCount; n++ ) {
		x = 16 + n * 78;
		y = 324 - 40;
		CG_DrawPic( x, y, 64, 64, loadingPlayerIcons[n] );
	}

	for ( n = 0; n < loadingItemIconCount; n++ ) {
		y = 400 - 40;
		if ( n >= 13 ) {
			y += 40;
		}
		x = 16 + n % 13 * 48;
		CG_DrawPic( x, y, 32, 32, loadingItemIcons[n] );
	}
#endif
}


/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString( const char* s ) {
	Q_strncpyz( cg.infoScreenText, s, sizeof( cg.infoScreenText ) );

	trap_UpdateScreen();
}

/*
===================
CG_LoadingItem
===================
*/
void CG_LoadingItem( int itemNum ) {
	gitem_t* item;

	item = BG_ItemForItemNum( itemNum );

	if ( item->icon && loadingItemIconCount < MAX_LOADING_ITEM_ICONS ) {
		loadingItemIcons[loadingItemIconCount++] = CG_GetIconHandle( item->icon );
	}

	CG_LoadingString( item->pickup_name );
}

/*
===================
CG_LoadingPlayer
===================
*/
void CG_LoadingPlayer( int playerNum ) {
	const char* info;
	char* skin;
	char			personality[MAX_QPATH];
	char			model[MAX_QPATH];
	char			iconName[MAX_QPATH];
	const char* defaultModel;

	info = CG_ConfigString( CS_PLAYERS + playerNum );

	if ( loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS ) {
		Q_strncpyz( model, Info_ValueForKey( info, "model" ), sizeof( model ) );
		skin = strrchr( model, '/' );
		if ( skin ) {
			*skin++ = '\0';
		} else {
			skin = "bright";
		}

		Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", model, skin );
		loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );

		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			defaultModel = cg_defaultModelGender.string[0] == 'f' ? cg_defaultFemaleModel.string : cg_defaultMaleModel.string;

			Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", defaultModel, "bright" );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( loadingPlayerIcons[loadingPlayerIconCount] ) {
			loadingPlayerIconCount++;
		}
	}

	Q_strncpyz( personality, Info_ValueForKey( info, "n" ), sizeof( personality ) );
	Q_CleanStr( personality );
	
	if ( cg_singlePlayer.integer && !GTF(GTF_TEAMS) && !GTF(GTF_CAMPAIGN) ) {
		trap_S_RegisterSound( va( "sound/player/announce/%s.wav", personality ), qtrue );
	}

	CG_LoadingString( personality );
}


/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawInformation( void ) {
	const char* s;
	const char* info;
	const char* sysInfo;
	int			y;
	//int			value;
	qhandle_t	levelShot;
	qhandle_t	detail;
	char		buf[1024];

	info = CG_ConfigString( CS_SERVERINFO );
	sysInfo = CG_ConfigString( CS_SYSTEMINFO );

	s = Info_ValueForKey( info, "mapName" );
	levelShot = trap_R_RegisterShaderNoMip( va( "levelshots/16-9/%s", s ) );
	if ( !levelShot ) {
		levelShot = trap_R_RegisterShaderNoMip( va( "levelshots/%s", s ) );
		if ( !levelShot ) {
			levelShot = trap_R_RegisterShaderNoMip( "menu/art/unknownmap" );
		}
	}
	trap_R_SetColor( NULL );
	trap_R_DrawStretchPic( 0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight, 0, 0, 1, 1, levelShot );

	// blend a detail texture over it
	detail = trap_R_RegisterShader( "levelShotDetail" );
	trap_R_DrawStretchPic( 0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight, 0, 0, 2.5, 2, detail );

	CG_SetScreenPlacement( PLACE_CENTER, PLACE_BOTTOM );

	// draw the icons of things as they are loaded
	CG_DrawLoadingIcons();

	// the first 150 rows are reserved for the client connection
	// screen to write into
	if ( cg.infoScreenText[0] ) {
		UI_DrawProportionalString( 320, 128 - 32, va( "Loading... %s", cg.infoScreenText ),
			UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
	} else {
		UI_DrawProportionalString( 320, 128 - 32, "Awaiting snapshot...",
			UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
	}

	// draw info string information

	y = 180 - 32;

	// don't print server lines if playing a local game
	trap_Cvar_VariableStringBuffer( "sv_running", buf, sizeof( buf ) );
	if ( !atoi( buf ) ) {
		// server hostname
		Q_strncpyz( buf, Info_ValueForKey( info, "sv_hostname" ), 1024 );
		Q_CleanStr( buf );
		UI_DrawProportionalString( 320, y, buf,
			UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
		y += PROP_HEIGHT;

		// pure server
		s = Info_ValueForKey( sysInfo, "sv_pure" );
		if ( s[0] == '1' ) {
			UI_DrawProportionalString( 320, y, "Pure Server",
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
			y += PROP_HEIGHT;
		}

		// server-specific message of the day
		s = CG_ConfigString( CS_MOTD );
		if ( s[0] ) {
			UI_DrawProportionalString( 320, y, s,
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
			y += PROP_HEIGHT;
		}

		// some extra space after hostname and motd
		y += 10;
	}

	// cheats warning
	s = Info_ValueForKey( sysInfo, "sv_cheats" );
	if ( s[0] == '1' ) {
		UI_DrawProportionalString( 320, y, "CHEATS ARE ENABLED",
			UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
		y += PROP_HEIGHT;
	}

	// game type
	if ( cgs.gameType != GT_CAMPAIGN )
		UI_DrawProportionalString( 320, y, cgs.gametypeName,
			UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
	y += PROP_HEIGHT;

	// map-specific message (long map name)
	s = CG_ConfigString( CS_MESSAGE );
	if ( s[0] ) {
		y = 390;
		CG_DrawString( SCREEN_WIDTH / 2, 390, s, UI_CENTER | UI_DROPSHADOW | UI_BIGFONT, NULL );
		y += CG_DrawStringLineHeight( UI_BIGFONT );
	}

	// map author(s)
	s = CG_ConfigString( CS_MAPAUTHOR1 );
	if ( s[0] ) {
		const char* t = CG_ConfigString( CS_MAPAUTHOR2 );
		CG_DrawString( SCREEN_WIDTH / 2, y, t[0] ? va( "%s | %s", s, t ) : s, UI_CENTER | UI_DROPSHADOW | UI_BIGFONT, NULL );

	}
}

