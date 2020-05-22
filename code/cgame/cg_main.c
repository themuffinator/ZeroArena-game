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
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"
#include "../ui/ui_public.h"

int forceModelModificationCount = -1;
#ifdef MISSIONPACK
int redTeamNameModificationCount = -1;
int blueTeamNameModificationCount = -1;
#endif

void CG_Init( connstate_t state, int maxSplitView, int playVideo );
void CG_Ingame_Init( int serverMessageNum, int serverCommandSequence, int maxSplitView, int playerNum0 );	// , int playerNum1, int playerNum2, int playerNum3 );
void CG_Shutdown( void );
void CG_Refresh( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback, connstate_t state, int realTime );
static char* CG_VoIPString( int localPlayerNum );
static int CG_MousePosition( int localPlayerNum );
static void CG_SetMousePosition( int localPlayerNum, int x, int y );
static void CG_UpdateGlconfig( qboolean initial );


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11 ) {

	switch ( command ) {
	case CG_GETAPINAME:
		return (intptr_t)CG_API_NAME;
	case CG_GETAPIVERSION:
		return (CG_API_MAJOR_VERSION << 16) | (CG_API_MINOR_VERSION & 0xFFFF);
	case CG_INIT:
		CG_Init( arg0, arg1, arg2 );
		return 0;
	case CG_INGAME_INIT:
		CG_Ingame_Init( arg0, arg1, arg2, arg3 );	// , arg4, arg5, arg6 );
		return 0;
	case CG_SHUTDOWN:
		UI_Shutdown();
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand( arg0, arg1 );
	case CG_REFRESH:
		CG_Refresh( arg0, arg1, arg2, arg3, arg4 );
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer( arg0 );
	case CG_LAST_ATTACKER:
		return CG_LastAttacker( arg0 );
	case CG_VOIP_STRING:
		return (intptr_t)CG_VoIPString( arg0 );
	case CG_KEY_EVENT:
		CG_DistributeKeyEvent( arg0, arg1, arg2, arg3, -1, 0 );
		return 0;
	case CG_CHAR_EVENT:
		CG_DistributeCharEvent( arg0, arg1 );
		return 0;
	case CG_MOUSE_EVENT:
		if ( cg.connected && (Key_GetCatcher() & KEYCATCH_CGAME) ) {
			CG_MouseEvent( arg0, arg1, arg2 );
		} else {
			UI_MouseEvent( arg0, arg1, arg2 );
		}
		return 0;
	case CG_MOUSE_POSITION:
		return CG_MousePosition( arg0 );
	case CG_SET_MOUSE_POSITION:
		CG_SetMousePosition( arg0, arg1, arg2 );
		return 0;
	case CG_SET_ACTIVE_MENU:
		UI_SetActiveMenu( arg0 );
		// stop cinematic when disconnect or start demo playback
		if ( arg0 == UIMENU_NONE && cg.cinematicPlaying ) {
			CG_StopCinematic_f();
		}
		return 0;
	case CG_JOYSTICK_AXIS_EVENT:
		CG_JoystickAxisEvent( arg0, arg1, arg2, arg3, arg4 );
		return 0;
	case CG_JOYSTICK_BUTTON_EVENT:
		CG_JoystickButtonEvent( arg0, arg1, arg2, arg3, arg4 );
		return 0;
	case CG_JOYSTICK_HAT_EVENT:
		CG_JoystickHatEvent( arg0, arg1, arg2, arg3, arg4 );
		return 0;
	case CG_CONSOLE_TEXT:
		CG_AddNotifyText( arg0, arg1 );
		return 0;
	case CG_CONSOLE_CLOSE:
		CG_CloseConsole();
		return 0;
	case CG_CREATE_USER_CMD:
		return (intptr_t)CG_CreateUserCmd( arg0, arg1, arg2, IntAsFloat( arg3 ), IntAsFloat( arg4 ), arg5 );
	case CG_UPDATE_GLCONFIG:
		CG_UpdateGlconfig( qfalse );
		return 0;
	case CG_CONSOLE_COMPLETEARGUMENT:
		return CG_ConsoleCompleteArgument( arg0, arg1, arg2 );
	default:
		CG_Error( "cgame vmMain: unknown command %i", command );
		break;
	}
	return -1;
}

cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES];
weaponInfo_t		cg_weapons[MAX_WEAPONS];
itemInfo_t			cg_items[MAX_ITEMS];

//vmCvar_t	cg_pmove_fixed;
vmCvar_t	cg_animSpeed;
vmCvar_t	cg_antiLag;
vmCvar_t	cg_atmosphericEffects;
vmCvar_t	cg_autoSwitch;
vmCvar_t	cg_blood;
vmCvar_t	cg_bobPitch;
vmCvar_t	cg_bobRoll;
vmCvar_t	cg_bobUp;
vmCvar_t	cg_brassTime;
vmCvar_t	cg_bubblesTrailStyle;
vmCvar_t	cg_cameraMode;
vmCvar_t	cg_centerPrintTime;
vmCvar_t	cg_color1;
vmCvar_t	cg_color2;
vmCvar_t	cg_coronaFarDist;
vmCvar_t	cg_coronas;
vmCvar_t	cg_crosshairHealth;
vmCvar_t	cg_crosshairSize;
vmCvar_t	cg_cyclePastGauntlet;
vmCvar_t	cg_debug;
vmCvar_t	cg_debugAnim;
vmCvar_t	cg_debugEvents;
vmCvar_t	cg_debugPosition;
vmCvar_t	cg_dedicated;
vmCvar_t	cg_deferPlayers;
vmCvar_t	cg_draw2D;
vmCvar_t	cg_draw3DIcons;
vmCvar_t	cg_drawAmmoWarning;
vmCvar_t	cg_drawAttacker;
vmCvar_t	cg_drawBBox;
vmCvar_t	cg_drawCrosshair;
vmCvar_t	cg_drawCrosshairNames;
vmCvar_t	cg_drawFPS;
vmCvar_t	cg_drawFriend;
vmCvar_t	cg_drawGrappleHook;
vmCvar_t	cg_drawGun;
vmCvar_t	cg_drawIcons;
vmCvar_t	cg_drawLagometer;
vmCvar_t	cg_drawItemPickups;
vmCvar_t	cg_drawRewards;
vmCvar_t	cg_drawScores;
vmCvar_t	cg_drawShaderInfo;
vmCvar_t	cg_drawSnapshot;
vmCvar_t	cg_drawStatus;
vmCvar_t	cg_drawTeamOverlay;
vmCvar_t	cg_drawTimer;
vmCvar_t	cg_enableBreath;
vmCvar_t	cg_enableDust;
vmCvar_t	cg_errorDecay;
vmCvar_t	cg_fadeExplosions;
vmCvar_t	cg_footSteps;
vmCvar_t	cg_forceBitmapFonts;
vmCvar_t	cg_fov;
vmCvar_t	cg_fovAspectAdjust;
vmCvar_t	cg_gibs;
vmCvar_t	cg_gun_frame;
vmCvar_t	cg_gun_x;
vmCvar_t	cg_gun_y;
vmCvar_t	cg_gun_z;
vmCvar_t	cg_handicap;
vmCvar_t	cg_hudFiles;
vmCvar_t	cg_hudFont;
vmCvar_t	cg_hudFontBorder;
vmCvar_t	cg_hudTextScale;
vmCvar_t	cg_impactMarks;
vmCvar_t	cg_noPlayerAnims;
vmCvar_t	cg_noPredict;
vmCvar_t	cg_noProjectileTrail;
vmCvar_t	cg_numberFont;
vmCvar_t	cg_numberFontBorder;
vmCvar_t	cg_paused;
vmCvar_t	cg_plasmaStyle;
vmCvar_t	cg_pmove_msec;
vmCvar_t	cg_predictItems;
vmCvar_t	cg_railCoreWidth;
vmCvar_t	cg_railSegmentLength;
vmCvar_t	cg_railStyle;
vmCvar_t	cg_railTrailTime;
vmCvar_t	cg_railWidth;
vmCvar_t	cg_rocketStyle;
vmCvar_t	cg_runPitch;
vmCvar_t	cg_runRoll;
vmCvar_t	cg_shadows;
vmCvar_t	cg_showMiss;
vmCvar_t	cg_simpleItems;
vmCvar_t	cg_singlePlayer;
vmCvar_t	cg_skyBox;
vmCvar_t	cg_smoothBodySink;
vmCvar_t	cg_splitviewTextScale;
vmCvar_t	cg_splitviewThirdEqual;
vmCvar_t	cg_splitviewVertical;
vmCvar_t	cg_swingSpeed;
vmCvar_t	cg_synchronousClients;
vmCvar_t	cg_teamChatsOnly;
vmCvar_t	cg_teamDmLeadAnnouncements;
vmCvar_t	cg_teamOverlayUserinfo;
vmCvar_t	cg_teamPref;
vmCvar_t	cg_thirdPerson;
vmCvar_t	cg_thirdPersonRange;
vmCvar_t	cg_thirdPersonAngle;
vmCvar_t	cg_thirdPersonHeight;
vmCvar_t	cg_thirdPersonSmooth;
vmCvar_t	cg_timescale;
vmCvar_t	cg_timescaleFadeEnd;
vmCvar_t	cg_timescaleFadeSpeed;
vmCvar_t	cg_tracerChance;
vmCvar_t	cg_tracerLength;
vmCvar_t	cg_tracerWidth;
vmCvar_t	cg_trueLightning;
vmCvar_t	cg_viewBobScale;
vmCvar_t	cg_viewSize;
vmCvar_t	cg_voipShowCrosshairMeter;
vmCvar_t	cg_voipShowMeter;
vmCvar_t	cg_weaponFov;
vmCvar_t	cg_zoomFov;

vmCvar_t 	cg_buildScript;
vmCvar_t 	cg_forceModel;
vmCvar_t 	cg_scorePlum;
vmCvar_t 	cg_smoothClients;
vmCvar_t 	cg_stats;
vmCvar_t 	cg_teamChatHeight;
vmCvar_t 	cg_teamChatTime;
vmCvar_t	con_autoChat;
vmCvar_t	con_autoClear;
vmCvar_t	con_font;
vmCvar_t	con_fontSize;
vmCvar_t	con_latency;
vmCvar_t	con_scrollSpeed;
vmCvar_t	pmove_fixed;
vmCvar_t	pmove_msec;
vmCvar_t	pmove_overBounce;

vmCvar_t	cg_cacheParticles;
vmCvar_t	cg_crosshairBrightness;
vmCvar_t	cg_crosshairColor;
vmCvar_t	cg_crosshairHitColor;
vmCvar_t	cg_crosshairHitStyle;
vmCvar_t	cg_crosshairHitPulseTime;
vmCvar_t	cg_crosshairOpacity;
vmCvar_t	cg_crosshairPickupPulse;
vmCvar_t	cg_crosshairRes;
vmCvar_t	cg_disableRGBA;
vmCvar_t	cg_drawGameNotify;
vmCvar_t	cg_drawGraphicalObits;
vmCvar_t	cg_drawPregameMessages;
vmCvar_t	cg_highResIcons;
vmCvar_t	cg_impactMarkTime;
vmCvar_t	cg_itemFX;
vmCvar_t	cg_kickScale;
vmCvar_t	cg_levelBrushContents;
vmCvar_t	cg_levelBrushSurfaces;
vmCvar_t	cg_placeholderSimpleItems;
vmCvar_t	cg_playIntros;
vmCvar_t	cg_switchOnEmpty;
vmCvar_t	cg_switchToEmpty;
vmCvar_t	cg_zoomOutOnDeath;
vmCvar_t	cg_zoomScaling;
vmCvar_t	cg_zoomSensitivity;
vmCvar_t	cg_zoomToggle;

vmCvar_t	pmove_q2;
vmCvar_t	pmove_q2slide;
vmCvar_t	pmove_q2air;

vmCvar_t	cg_introPlayed;
vmCvar_t	cg_joystickDebug;
vmCvar_t	ui_stretch;

#ifdef MISSIONPACK
vmCvar_t 	cg_redTeamName;
vmCvar_t 	cg_blueTeamName;
vmCvar_t	cg_recordSPDemo;
vmCvar_t	cg_recordSPDemoName;
#endif
vmCvar_t	cg_obeliskRespawnDelay;

vmCvar_t	cg_defaultModelGender;
vmCvar_t	cg_defaultMaleModel;
vmCvar_t	cg_defaultMaleHeadModel;
vmCvar_t	cg_defaultFemaleModel;
vmCvar_t	cg_defaultFemaleHeadModel;

typedef struct {
	vmCvar_t* vmCvar;
	char* cvarName;
	char* defaultString;
	int			cvarFlags;
	float		rangeMin;
	float		rangeMax;
	qboolean	rangeIntegral;
} cvarTable_t;

#define RANGE_ALL 0, 0, qfalse
#define RANGE_BOOL 0, 1, qtrue
#define RANGE_INT(min,max) min, max, qtrue
#define RANGE_FLOAT(min,max) min, max, qfalse

static cvarTable_t cgameCvarTable[] = {
	{ NULL, "cgameversion", PRODUCT_NAME " " PRODUCT_VERSION " " PLATFORM_STRING " " PRODUCT_DATE, CVAR_ROM, RANGE_ALL },

	{ &cg_animSpeed, "cg_animSpeed", "1", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_antiLag, "cg_antiLag", "0", CVAR_USERINFO_ALL | CVAR_ARCHIVE, RANGE_INT( 0, 2 ) },
	{ &cg_autoSwitch, "cg_autoSwitch", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_atmosphericEffects, "cg_atmosphericEffects", "1", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_bobPitch, "cg_bobPitch", "0.002", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_bobRoll, "cg_bobRoll", "0.002", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_bobUp, "cg_bobUp", "0.005", CVAR_CHEAT, RANGE_ALL },
	{ &cg_brassTime, "cg_brassTime", "5000", CVAR_ARCHIVE, RANGE_INT( 0, 60000 ) },
	{ &cg_bubblesTrailStyle, "cg_bubblesTrailStyle", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_centerPrintTime, "cg_centerPrintTime", "3000", CVAR_CHEAT, RANGE_INT( 0, 5000 ) },
	{ &cg_color1, "color1", XSTRING( DEFAULT_PLAYER_COLOR1 ), CVAR_USERINFO | CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_color2, "color2", XSTRING( DEFAULT_PLAYER_COLOR2 ), CVAR_USERINFO | CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_coronaFarDist, "cg_coronaFarDist", "1536", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_coronas, "cg_coronas", "1", CVAR_ARCHIVE, RANGE_INT( 0, 3 ) },
	{ &cg_crosshairHealth, "cg_crosshairHealth", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_crosshairSize, "cg_crosshairSize", "24", CVAR_ARCHIVE, RANGE_INT( 4, 64 ) },
	{ &cg_cyclePastGauntlet, "cg_cyclePastGauntlet", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_debug, "cg_debug", "0", CVAR_CHEAT, RANGE_ALL },
	{ &cg_debugAnim, "cg_debugAnim", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_debugEvents, "cg_debugEvents", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_debugPosition, "cg_debugPosition", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_dedicated, "dedicated", "0", 0, RANGE_ALL },
	{ &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_draw3DIcons, "cg_draw3DIcons", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawBBox, "cg_drawBBox", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE, RANGE_INT( 0, NUM_CROSSHAIRS ) },
	{ &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawGrappleHook, "cg_drawGrappleHook", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawGun, "cg_drawGun", "1", CVAR_USERINFO|CVAR_ARCHIVE, RANGE_INT( 0, 3 ) },
	{ &cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawLagometer, "cg_drawLagometer", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawItemPickups, "cg_drawItemPickups", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawScores, "cg_drawScores", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawShaderInfo, "cg_drawShaderInfo", "0", 0, RANGE_BOOL },
	{ &cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE, RANGE_INT( 0, 3 ) },
	{ &cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_enableBreath, "cg_enableBreath", "0", CVAR_ROM, RANGE_BOOL },
	{ &cg_enableDust, "cg_enableDust", "0", CVAR_ROM, RANGE_BOOL },
	{ &cg_errorDecay, "cg_errorDecay", "100", 0, RANGE_ALL },
	{ &cg_fadeExplosions, "cg_fadeExplosions", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_footSteps, "cg_footSteps", "1", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_forceBitmapFonts, "cg_forceBitmapFonts", "0", CVAR_ARCHIVE | CVAR_LATCH, RANGE_BOOL },
	{ &cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_fov, "cg_fov", "90", CVAR_ARCHIVE, RANGE_FLOAT( 80, 120 ) },
	{ &cg_fovAspectAdjust, "cg_fovAspectAdjust", "1", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_gun_x, "cg_gunX", "0", CVAR_CHEAT, RANGE_ALL },
	{ &cg_gun_y, "cg_gunY", "0", CVAR_CHEAT, RANGE_ALL },
	{ &cg_gun_z, "cg_gunZ", "0", CVAR_CHEAT, RANGE_ALL },
	{ &cg_gun_frame, "cg_gun_frame", "0", CVAR_CHEAT, RANGE_ALL },
	{ &cg_handicap, "handicap", "100", CVAR_USERINFO | CVAR_ARCHIVE, RANGE_INT( 1, 100 ) },
	{ &cg_hudFont, "cg_hudFont", "fonts/LiberationSans-Bold.ttf", CVAR_ARCHIVE | CVAR_LATCH, RANGE_ALL },
	{ &cg_hudFontBorder, "cg_hudFontBorder", "2", CVAR_ARCHIVE | CVAR_LATCH, RANGE_FLOAT( 0, 5 ) },
	{ &cg_hudTextScale, "cg_hudTextScale", "0.4", CVAR_ARCHIVE, RANGE_FLOAT( 0.1, 5 ) },
	{ &cg_impactMarks, "cg_impactMarks", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_joystickDebug, "in_joystickDebug", "0", CVAR_TEMP, RANGE_BOOL },
	{ &cg_noPlayerAnims, "cg_noPlayerAnims", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_noPredict, "cg_noPredict", "0", 0, RANGE_BOOL },
	{ &cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_numberFont, "cg_numberFont", "", CVAR_ARCHIVE | CVAR_LATCH, RANGE_ALL },
	{ &cg_numberFontBorder, "cg_numberFontBorder", "0", CVAR_ARCHIVE | CVAR_LATCH, RANGE_FLOAT( 0, 5 ) },
	{ &cg_plasmaStyle, "cg_plasmaStyle", "1", CVAR_ARCHIVE, RANGE_BOOL },
//	{ &cg_pmove_fixed, "cg_pmove_fixed", "0", CVAR_USERINFO | CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE | CVAR_USERINFO_ALL, RANGE_BOOL },
	{ &cg_railCoreWidth, "cg_railCoreWidth", "6", CVAR_ARCHIVE, RANGE_INT( 0, 32 ) },
	{ &cg_railSegmentLength, "cg_railSegmentLength", "32", CVAR_ARCHIVE, RANGE_INT( 1, 1000 ) },
	{ &cg_railStyle, "cg_railStyle", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE, RANGE_INT( 100, 1000 ) },
	{ &cg_railWidth, "cg_railWidth", "16", CVAR_ARCHIVE, RANGE_INT( 4, 32 ) },
	{ &cg_rocketStyle, "cg_rocketStyle", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_runPitch, "cg_runPitch", "0.002", CVAR_ARCHIVE, RANGE_FLOAT( 0, 1 ) },
	{ &cg_runRoll, "cg_runRoll", "0.005", CVAR_ARCHIVE, RANGE_FLOAT( 0, 1 ) },
	{ &cg_scorePlum, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_showMiss, "cg_showMiss", "0", CVAR_CHEAT, RANGE_INT( 0, 2 ) },
	{ &cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_skyBox, "cg_skyBox", "1", CVAR_ARCHIVE, RANGE_INT( 0, 2 ) },
	{ &cg_smoothBodySink, "cg_smoothBodySink", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_splitviewTextScale, "cg_splitviewTextScale", "2", CVAR_ARCHIVE, RANGE_FLOAT( 0.1, 5 ) },
	{ &cg_splitviewThirdEqual, "cg_splitviewThirdEqual", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_splitviewVertical, "cg_splitviewVertical", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_stats, "cg_stats", "0", 0, RANGE_ALL },
	{ &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT, RANGE_ALL },
	{ &cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE, RANGE_INT( 0, TEAMCHAT_HEIGHT ) },
	{ &cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE, RANGE_INT( 0, 10000 ) },
	{ &cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_teamDmLeadAnnouncements, "cg_teamDmLeadAnnouncements", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO_ALL, RANGE_ALL },
	{ &cg_teamPref, "teamPref", "", CVAR_USERINFO, RANGE_ALL },
	{ &cg_thirdPerson, "cg_thirdPerson", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_thirdPersonRange, "cg_thirdPersonRange", "40", CVAR_CHEAT, RANGE_ALL },
	{ &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT, RANGE_ALL },
	{ &cg_thirdPersonHeight, "cg_thirdPersonHeight", "24", 0, RANGE_INT( 0, 32 ) },
	{ &cg_thirdPersonSmooth, "cg_thirdPersonSmooth", "0", 0, RANGE_ALL }, // this cvar exists because it's behavior is too buggy to enable by default
	{ &cg_timescale, "timescale", "1", 0, RANGE_ALL },
	{ &cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0, RANGE_ALL },
	{ &cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0, RANGE_ALL },
	{ &cg_tracerChance, "cg_tracerChance", "0.4", CVAR_CHEAT, RANGE_ALL },
	{ &cg_tracerLength, "cg_tracerLength", "100", CVAR_CHEAT, RANGE_ALL },
	{ &cg_tracerWidth, "cg_tracerWidth", "1", CVAR_CHEAT, RANGE_ALL },
	{ &cg_trueLightning, "cg_trueLightning", "0.5", CVAR_ARCHIVE, RANGE_FLOAT( 0, 1 ) },
	{ &cg_viewBobScale, "cg_viewBobScale", "1.0", CVAR_ARCHIVE, RANGE_FLOAT( 0, 2 ) },
	{ &cg_viewSize, "cg_viewSize", "100", CVAR_ARCHIVE, RANGE_INT( 30, 100 ) },
	{ &cg_voipShowCrosshairMeter, "cg_voipShowCrosshairMeter", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_voipShowMeter, "cg_voipShowMeter", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_weaponFov, "cg_weaponFov", "90", CVAR_ARCHIVE, RANGE_FLOAT( 0, 160 ) },
	{ &cg_zoomFov, "cg_zoomFov", "22.5", CVAR_ARCHIVE, RANGE_FLOAT( 1, 160 ) },
	{ &con_autoChat, "con_autoChat", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &con_autoClear, "con_autoClear", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &con_font, "con_font", "fonts/LiberationMono-Regular.ttf", CVAR_ARCHIVE | CVAR_LATCH, RANGE_ALL },
	{ &con_fontSize, "con_fontSize", "8", CVAR_ARCHIVE | CVAR_LATCH, RANGE_INT( 4, 24 ) },
	{ &con_latency, "con_latency", "3000", 0, RANGE_INT( 0, 5000 ) },
	{ &con_scrollSpeed, "con_scrollSpeed", "3", 0, RANGE_FLOAT( 0, 10 ) },
	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, RANGE_BOOL },
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, RANGE_INT( 8, 33 ) },
	{ &pmove_overBounce, "pmove_overBounce", "0", CVAR_SYSTEMINFO, RANGE_BOOL },
	{ &ui_stretch, "ui_stretch", "0", CVAR_CHEAT, RANGE_BOOL },

	// the following variables are created in other parts of the system,
	// but we also reference them here
	{ &cg_buildScript, "com_buildScript", "0", 0, RANGE_ALL },	// force loading of all possible data amd error on failures
	{ &cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT, RANGE_ALL },
	{ &cg_introPlayed, "cg_introPlayed", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_paused, "cl_paused", "0", CVAR_ROM, RANGE_ALL },
	{ &cg_blood, "com_blood", "1", CVAR_ARCHIVE, RANGE_ALL },
	{ NULL,  "g_gameType", "1", CVAR_SERVERINFO, RANGE_INT( 0, GT_MAX_GAME_TYPE - 1 ) },
	{ &cg_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, RANGE_BOOL },
	{ &cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_SYSTEMINFO | CVAR_ROM, RANGE_ALL },
	{ &cg_obeliskRespawnDelay, "g_obeliskRespawnDelay", "20", CVAR_SYSTEMINFO, RANGE_ALL },

#ifdef MISSIONPACK
	{ &cg_redTeamName, "g_redTeamName", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE | CVAR_SYSTEMINFO, RANGE_ALL },
	{ &cg_blueTeamName, "g_blueTeamName", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE | CVAR_SYSTEMINFO, RANGE_ALL },
	{ &cg_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_recordSPDemoName, "ui_recordSPDemoName", "", CVAR_ARCHIVE, RANGE_ALL },
#else
	{ &cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE, RANGE_BOOL },
#endif

	{ &cg_defaultModelGender, "default_model_gender", DEFAULT_MODEL_GENDER, CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_defaultMaleModel, "default_male_model", DEFAULT_MODEL_MALE, CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_defaultMaleHeadModel, "default_male_headmodel", DEFAULT_HEAD_MALE, CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_defaultFemaleModel, "default_female_model", DEFAULT_MODEL_FEMALE, CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_defaultFemaleHeadModel, "default_female_headmodel", DEFAULT_HEAD_FEMALE, CVAR_ARCHIVE, RANGE_ALL },

	{ &cg_cacheParticles, "cg_cacheParticles", "0", CVAR_ARCHIVE, RANGE_INT( 0, 1 ) },
	{ &cg_crosshairBrightness, "cg_crosshairBrightness", "1.0", CVAR_ARCHIVE, RANGE_FLOAT( 0, 1 ) },
	{ &cg_crosshairColor, "cg_crosshairColor", "25", CVAR_ARCHIVE, RANGE_INT( 1, 26 ) },
	{ &cg_crosshairHitColor, "cg_crosshairHitColor", "1", CVAR_ARCHIVE, RANGE_INT( 1, 26 ) },
	{ &cg_crosshairHitStyle, "cg_crosshairHitStyle", "0", CVAR_ARCHIVE, RANGE_INT( 0, 7 ) },
	{ &cg_crosshairHitPulseTime, "cg_crosshairHitPulseTime", "200", CVAR_ARCHIVE, RANGE_INT( 0, 500 ) },
	{ &cg_crosshairOpacity, "cg_crosshairOpacity", "1.0", CVAR_ARCHIVE, RANGE_FLOAT( 0, 1 ) },
	{ &cg_crosshairPickupPulse, "cg_crosshairPickupPulse", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_crosshairRes, "cg_crosshairRes", "0", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_disableRGBA, "cg_disableRGBA", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_drawGameNotify, "cg_drawGameNotify", "2", CVAR_ARCHIVE, RANGE_INT( 0, MAX_NOTIFY_HISTORY ) },
	{ &cg_drawGraphicalObits, "cg_drawGraphicalObits", "1", CVAR_ARCHIVE, RANGE_INT( 0, MAX_GRAPHICAL_OBITS ) },
	{ &cg_drawPregameMessages, "cg_drawPregameMessages", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_highResIcons, "cg_highResIcons", "-1", CVAR_ARCHIVE, RANGE_INT( -1, 3 ) },
	{ &cg_impactMarkTime, "cg_impactMarkTime", "30000", CVAR_ARCHIVE, RANGE_INT( 0, 60000 ) },
	{ &cg_itemFX, "cg_itemFX", "0", CVAR_ARCHIVE, RANGE_INT( 0, 1 ) },
	{ &cg_placeholderSimpleItems, "cg_placeholderSimpleItems", "1", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_playIntros, "cg_playIntros", "1", CVAR_ARCHIVE, RANGE_INT( 0, 2 ) },
	{ &cg_kickScale, "cg_kickScale", "0.25", CVAR_ARCHIVE, RANGE_FLOAT( 0, 1 ) },
	{ &cg_levelBrushContents, "cg_levelBrushContents", "", CVAR_ROM, RANGE_ALL },
	{ &cg_levelBrushSurfaces, "com_levelBrushSurfaces", "", CVAR_ROM, RANGE_ALL },
	{ &cg_switchOnEmpty, "cg_switchOnEmpty", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_switchToEmpty, "cg_switchToEmpty", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_zoomOutOnDeath, "cg_zoomOutOnDeath", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_zoomScaling, "cg_zoomScaling", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_zoomSensitivity, "cg_zoomSensitivity", "1", CVAR_ARCHIVE, RANGE_FLOAT( 0.1, 5 ) },
	{ &cg_zoomToggle, "cg_zoomToggle", "0", CVAR_ARCHIVE, RANGE_BOOL },

	{ &pmove_q2, "pmove_q2", "0", CVAR_SYSTEMINFO | CVAR_CHEAT, RANGE_BOOL },
	{ &pmove_q2slide, "pmove_q2slide", "0", CVAR_SYSTEMINFO | CVAR_CHEAT, RANGE_BOOL },
	{ &pmove_q2air, "pmove_q2air", "0", CVAR_SYSTEMINFO | CVAR_CHEAT, RANGE_BOOL },
};

static int  cgameCvarTableSize = ARRAY_LEN( cgameCvarTable );

/*
=================
CG_RegisterCvar
=================
*/
void CG_RegisterCvar( vmCvar_t* vmCvar, char* cvarName, char* defaultString, int cvarFlags, float rangeMin, float rangeMax, qboolean rangeIntegral ) {
	trap_Cvar_Register( vmCvar, cvarName, defaultString, cvarFlags );

	if ( rangeMin != 0 || rangeMax != 0 ) {
		trap_Cvar_CheckRange( cvarName, rangeMin, rangeMax, rangeIntegral );
	}
}

/*
=================
CG_RegisterCgameCvars
=================
*/
void CG_RegisterCgameCvars( void ) {
	cvarTable_t* cv;
	int			i;

	for ( i = 0, cv = cgameCvarTable; i < cgameCvarTableSize; i++, cv++ ) {
		CG_RegisterCvar( cv->vmCvar, cv->cvarName, cv->defaultString,
			cv->cvarFlags, cv->rangeMin, cv->rangeMax, cv->rangeIntegral );
	}
}


/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	char		var[MAX_TOKEN_CHARS];

	CG_RegisterCgameCvars();
	CG_RegisterInputCvars();

	// ZTM: TODO: Move cgs.localServer init somewhere else?
	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

	forceModelModificationCount = cg_forceModel.modificationCount;

	trap_Cvar_Register( NULL, "name", DEFAULT_PLAYER_NAME, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register( NULL, "clan", "", CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register( NULL, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register( NULL, "headModel", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
#ifdef MISSIONPACK
	redTeamNameModificationCount = cg_redTeamName.modificationCount;
	blueTeamNameModificationCount = cg_blueTeamName.modificationCount;
#endif
}

/*
===================
CG_ForceModelChange
===================
*/
static void CG_ForceModelChange( void ) {
	int		i;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		const char* playerInfo;

		playerInfo = CG_ConfigString( CS_PLAYERS + i );
		if ( !playerInfo[0] ) {
			continue;
		}
		CG_NewPlayerInfo( i );
	}
}

/*
=================
CG_UpdateCgameCvars
=================
*/
void CG_UpdateCgameCvars( void ) {
	int			i;
	cvarTable_t* cv;

	for ( i = 0, cv = cgameCvarTable; i < cgameCvarTableSize; i++, cv++ ) {
		if ( !cv->vmCvar ) {
			continue;
		}

		trap_Cvar_Update( cv->vmCvar );
	}
}


/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	CG_UpdateCgameCvars();
	CG_UpdateInputCvars();

	if ( !cg.connected ) {
		return;
	}

	// check for modications here

	// If team overlay is on, ask for updates from the server.  If it's off,
	// let the server know so we don't receive it
	if ( drawTeamOverlayModificationCount != cg_drawTeamOverlay.modificationCount ) {
		drawTeamOverlayModificationCount = cg_drawTeamOverlay.modificationCount;

		if ( cg_drawTeamOverlay.integer > 0 ) {
			trap_Cvar_SetValue( "teamoverlay", 1 );
		} else {
			trap_Cvar_SetValue( "teamoverlay", 0 );
		}
	}

#ifdef MISSIONPACK
	// if force model or a team name changed
	if ( forceModelModificationCount != cg_forceModel.modificationCount
		|| redTeamNameModificationCount != cg_redTeamName.modificationCount
		|| blueTeamNameModificationCount != cg_blueTeamName.modificationCount ) {
		forceModelModificationCount = cg_forceModel.modificationCount;
		redTeamNameModificationCount = cg_redTeamName.modificationCount;
		blueTeamNameModificationCount = cg_blueTeamName.modificationCount;
		CG_ForceModelChange();
	}
#else
	// if force model changed
	if ( forceModelModificationCount != cg_forceModel.modificationCount ) {
		forceModelModificationCount = cg_forceModel.modificationCount;
		CG_ForceModelChange();
	}
#endif
}

int CG_CrosshairPlayer( int localPlayerNum ) {
	if ( !cg.snap || localPlayerNum < 0 || localPlayerNum >= CG_MaxSplitView() ) {
		return -1;
	}

	if ( cg.time > ( cg.localPlayers[localPlayerNum].crosshairPlayerTime + 1000 ) ) {
		return -1;
	}

	return cg.localPlayers[localPlayerNum].crosshairPlayerNum;
}

int CG_LastAttacker( int localPlayerNum ) {
	if ( !cg.snap || localPlayerNum < 0 || localPlayerNum >= CG_MaxSplitView() ) {
		return -1;
	}

	if ( !cg.localPlayers[localPlayerNum].attackerTime || cg.localPlayers[localPlayerNum].playerNum == -1 ) {
		return -1;
	}

	return cg.snap->pss[localPlayerNum].persistant[PERS_ATTACKER];
}

/*
=================
CG_RemoveNotifyLine
=================
*/
void CG_RemoveNotifyLine( localPlayer_t* player ) {
	int i, offset, totalLength;

	if ( !player || player->numConsoleLines == 0 )
		return;

	offset = player->consoleLines[0].length;
	totalLength = strlen( player->consoleText ) - offset;

	// slide up consoleText
	for ( i = 0; i <= totalLength; i++ )
		player->consoleText[i] = player->consoleText[i + offset];

	// pop up the first consoleLine
	for ( i = 1; i < player->numConsoleLines; i++ )
		player->consoleLines[i - 1] = player->consoleLines[i];

	// clear last slot
	player->consoleLines[player->numConsoleLines - 1].length = 0;
	player->consoleLines[player->numConsoleLines - 1].time = 0;

	player->numConsoleLines--;
}

/*
=================
CG_AddNotifyText
=================
*/
void CG_AddNotifyText( int realTime, qboolean restoredText ) {
	char text[1024];
	char* buffer;
	int bufferLen;
	int i;
	localPlayer_t* player;
	int localPlayerBits;
	qboolean skipnotify = qfalse;

	trap_LiteralArgs( text, sizeof( text ) );

	if ( !text[0] ) {
		for ( i = 0; i < CG_MaxSplitView(); i++ ) {
			cg.localPlayers[i].consoleText[0] = '\0';
			cg.localPlayers[i].numConsoleLines = 0;
		}
		return;
	}

	buffer = text;

	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !Q_strncmp( buffer, "[skipnotify]", 12 ) ) {
		skipnotify = qtrue;
		buffer += 12;
	}

	CG_ConsolePrint( buffer );

	if ( skipnotify || restoredText || (Key_GetCatcher() & KEYCATCH_CONSOLE) ) {
		return;
	}

	// [player #] perfix for text that only shows up in notify area for one local player
	if ( !Q_strncmp( buffer, "[player ", 8 ) && buffer[8] >= '1' && buffer[8] < '1' + MAX_SPLITVIEW && buffer[9] == ']' ) {
		localPlayerBits = 1 << (buffer[8] - '1');

		buffer += 10;
	} else {
		localPlayerBits = ~0;
	}

	bufferLen = strlen( buffer );

	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		if ( !(localPlayerBits & (1 << i)) ) {
			continue;
		}

		player = &cg.localPlayers[i];

		// replace line
		if ( buffer[0] == '\r' ) {
			int j, length;

			length = 0;
			for ( j = 0; j < player->numConsoleLines - 1; j++ )
				length += player->consoleLines[j].length;

			player->consoleText[length] = '\0';

			if ( player->numConsoleLines > 0 ) {
				player->numConsoleLines--;
			}

			// free lines until there is enough space to fit buffer
			while ( strlen( player->consoleText ) + bufferLen > MAX_CONSOLE_TEXT ) {
				CG_RemoveNotifyLine( player );
			}

			// skip leading \r
			Q_strcat( player->consoleText, MAX_CONSOLE_TEXT, buffer + 1 );
			player->consoleLines[player->numConsoleLines].time = cg.time;
			player->consoleLines[player->numConsoleLines].length = bufferLen - 1;
			player->numConsoleLines++;
			continue;
		}

		// free lines until there is enough space to fit buffer
		while ( strlen( player->consoleText ) + bufferLen > MAX_CONSOLE_TEXT ) {
			CG_RemoveNotifyLine( player );
		}

		// append to existing line
		if ( player->numConsoleLines > 0 && player->consoleText[strlen( player->consoleText ) - 1] != '\n' ) {
			Q_strcat( player->consoleText, MAX_CONSOLE_TEXT, buffer );
			player->consoleLines[player->numConsoleLines - 1].time = cg.time;
			player->consoleLines[player->numConsoleLines - 1].length += bufferLen;
			continue;
		}

		if ( player->numConsoleLines == MAX_CONSOLE_LINES ) {
			CG_RemoveNotifyLine( player );
		}

		if ( player->numConsoleLines == MAX_CONSOLE_LINES ) {
			continue; // this shouldn't ever happen
		}

		Q_strcat( player->consoleText, MAX_CONSOLE_TEXT, buffer );
		player->consoleLines[player->numConsoleLines].time = cg.time;
		player->consoleLines[player->numConsoleLines].length = bufferLen;
		player->numConsoleLines++;
	}
}

/*
=================
CG_NotifyPrintf

Only printed in notify area for localPlayerNum (and client console)
=================
*/
void QDECL CG_NotifyPrintf( int localPlayerNum, const char* msg, ... ) {
	va_list		argptr;
	char		text[1024];
	int			prefixLen;

	Com_sprintf( text, sizeof( text ), "" );		// "[player %d]", localPlayerNum + 1 );
	prefixLen = strlen( text );

	va_start( argptr, msg );
	Q_vsnprintf( text + prefixLen, sizeof( text ) - prefixLen, msg, argptr );
	va_end( argptr );

	// switch order of [player %d][skipnotify] so skip is first
	if ( !Q_strncmp( text + prefixLen, "[skipnotify]", 12 ) ) {
		memmove( text + 12, text, prefixLen ); // "[player %d]"
		memcpy( text, "[skipnotify]", 12 );
	}

	trap_Print( text );
}

/*
=================
CG_NotifyBitsPrintf

Only printed in notify area for players specified in localPlayerBits (and client console)
=================
*/
void QDECL CG_NotifyBitsPrintf( int localPlayerBits, const char* msg, ... ) {
	va_list		argptr;
	char		text[1024];
	int i;

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		if ( localPlayerBits & (1 << i) ) {
			CG_NotifyPrintf( i, "%s", text );
		}
	}
}

void QDECL CG_DPrintf( const char* msg, ... ) {
	va_list		argptr;
	char		text[1024];
	char		var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer( "developer", var, sizeof( var ) );
	if ( !atoi( var ) ) {
		return;
	}

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	trap_Print( text );
}

void QDECL CG_Printf( const char* msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	trap_Print( text );
}

void QDECL CG_Error( const char* msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	trap_Error( text );
}

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

	CG_DPrintf( "%s", text );
}

/*
================
CG_Argv
================
*/
const char* CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}

/*
================
CG_Cvar_VariableString
================
*/
char* CG_Cvar_VariableString( const char* var_name ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer( var_name, buffer, sizeof( buffer ) );

	return buffer;
}

/*
================
CG_MaxSplitView
================
*/
int CG_MaxSplitView( void ) {
	return cgs.maxSplitView;
}

//========================================================================

/*
=================
CG_SetupDlightstyles
=================
*/
void CG_SetupDlightstyles( void ) {
	int i, j;
	char* str;
	char* token;
	int entnum;
	centity_t* cent;

	cg.lightstylesInited = qtrue;

	for ( i = 1; i < MAX_DLIGHT_CONFIGSTRINGS; i++ ) {
		str = (char*)CG_ConfigString( CS_DLIGHTS + i );
		if ( !strlen( str ) ) {
			break;
		}

		token = COM_Parse( &str );   // ent num
		entnum = atoi( token );

		if ( entnum < 0 || entnum >= MAX_GENTITIES ) {
			continue;
		}

		cent = &cg_entities[entnum];

		token = COM_Parse( &str );   // stylestring
		Q_strncpyz( cent->dl_stylestring, token, sizeof( cent->dl_stylestring ) );

		token = COM_Parse( &str );   // offset
		cent->dl_frame = atoi( token );
		cent->dl_oldframe = cent->dl_frame - 1;
		if ( cent->dl_oldframe < 0 ) {
			cent->dl_oldframe = strlen( cent->dl_stylestring );
		}

		token = COM_Parse( &str );   // sound id
		cent->dl_sound = atoi( token );

		token = COM_Parse( &str );   // attenuation
		cent->dl_atten = atoi( token );

		for ( j = 0; j < strlen( cent->dl_stylestring ); j++ ) {

			cent->dl_stylestring[j] += cent->dl_atten;  // adjust character for attenuation/amplification

			// clamp result
			if ( cent->dl_stylestring[j] < 'a' ) {
				cent->dl_stylestring[j] = 'a';
			}
			if ( cent->dl_stylestring[j] > 'z' ) {
				cent->dl_stylestring[j] = 'z';
			}
		}

		cent->dl_backlerp = 0.0;
		cent->dl_time = cg.time;
	}
}

//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
void CG_RegisterItemSounds( int itemNum ) {
	gitem_t* item;
	char			data[MAX_QPATH];
	char* s, * start;
	int				len;

	item = BG_ItemForItemNum( itemNum );

	if ( item->pickup_sound ) {
		cgs.media.itemPickupSounds[itemNum] = trap_S_RegisterSound( item->pickup_sound, qfalse );
	}

	if ( item->giType == IT_POWERUP ) {
		switch ( item->giTag ) {
		case PW_QUAD:
			cgs.media.quadSound = trap_S_RegisterSound( "sound/items/damage3.wav", qfalse );
			break;
		case PW_FLIGHT:
			cgs.media.flightSound = trap_S_RegisterSound( "sound/items/flight.wav", qfalse );
			break;
		case PW_BATTLESUIT:
		case PW_INVULN:
			cgs.media.regenSound = trap_S_RegisterSound( "sound/items/regen.wav", qfalse );
			cgs.media.protectSound = trap_S_RegisterSound( "sound/items/protect3.wav", qfalse );
			break;
		case PW_VAMPIRE:
			cgs.media.vampireSound = trap_S_RegisterSound( "sound/items/vampire.wav", qfalse );
			break;
		default: break;
		}
	} else if ( item->giType == IT_WEAPON ) {
		switch ( item->giTag ) {
		case WP_MACHINEGUN:
			cgs.media.tracerSound = trap_S_RegisterSound( "sound/weapons/machinegun/buletby1.wav", qfalse );
			cgs.media.sfx_ric[0] = trap_S_RegisterSound( "sound/weapons/machinegun/ric1.wav", qfalse );
			cgs.media.sfx_ric[1] = trap_S_RegisterSound( "sound/weapons/machinegun/ric2.wav", qfalse );
			cgs.media.sfx_ric[2] = trap_S_RegisterSound( "sound/weapons/machinegun/ric3.wav", qfalse );
			break;

		case WP_GRENADE_LAUNCHER:
			cgs.media.hgrenb1aSound = trap_S_RegisterSound( "sound/weapons/grenade/hgrenb1a.wav", qfalse );
			cgs.media.hgrenb2aSound = trap_S_RegisterSound( "sound/weapons/grenade/hgrenb2a.wav", qfalse );
			cgs.media.sfx_rockexp = trap_S_RegisterSound( "sound/weapons/rocket/rocklx1a.wav", qfalse );
			break;
		case WP_ROCKET_LAUNCHER:
			cgs.media.sfx_rockexp = trap_S_RegisterSound( "sound/weapons/rocket/rocklx1a.wav", qfalse );
			break;
		case WP_PLASMAGUN:
			cgs.media.sfx_plasmaexp = trap_S_RegisterSound( "sound/weapons/plasma/plasmx1a.wav", qfalse );
			break;
		default: break;
		}
	} else if ( item->giType == IT_HOLDABLE ) {
		switch ( item->giTag ) {
		case HI_MEDKIT:
			cgs.media.medkitSound = trap_S_RegisterSound( "sound/items/use_medkit.wav", qfalse );
			break;
		case HI_PSCREEN:
		case HI_PSHIELD:
			cgs.media.parmorOnSound = trap_S_RegisterSound( "sound/misc/power1.wav", qfalse );
			cgs.media.parmorOffSound = trap_S_RegisterSound( "sound/misc/power2.wav", qfalse );
			cgs.media.parmorSound = trap_S_RegisterSound( "sound/weapons/lashit.wav", qfalse );
			break;
		default: break;
		}
	} else if ( item->giType == IT_RUNE ) {
		switch ( item->giTag ) {
		case PW_SCOUT:
			cgs.media.scoutSound = trap_S_RegisterSound( "sound/runes/scout.wav", qfalse );
			break;
		case PW_ARMAMENT:
			cgs.media.armamentSound = trap_S_RegisterSound( "sound/runes/armament.wav", qfalse );
			break;
		case PW_STRENGTH:
			cgs.media.strengthSound = trap_S_RegisterSound( "sound/runes/strength.wav", qfalse );
			break;
		case PW_RESISTANCE:
			cgs.media.resistanceSound = trap_S_RegisterSound( "sound/runes/resistance.wav", qfalse );
			break;
		case PW_TENACITY:
			cgs.media.tenacitySound = trap_S_RegisterSound( "sound/runes/tenacity.wav", qfalse );
			break;
		case PW_PARASITE:
			cgs.media.parasiteSound = trap_S_RegisterSound( "sound/runes/parasite.wav", qfalse );
			break;
		default: break;
		}
	}
	
	// parse the space separated precache string for other media
	s = item->sounds;
	if ( !s || !s[0] )
		return;

	while ( *s ) {
		start = s;
		while ( *s && *s != ' ' ) {
			s++;
		}

		len = s - start;
		if ( len >= MAX_QPATH || len < 5 ) {
			CG_Error( "PrecacheItem: %s has bad precache string",
				item->classname );
			return;
		}
		memcpy( data, start, len );
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp( data + len - 3, "wav" ) ) {
			trap_S_RegisterSound( data, qfalse );
		}
	}
}


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
void CG_RegisterSounds( qboolean restart ) {
	int				i;
	char			items[MAX_ITEMS + 1];
	char			name[MAX_QPATH];
	const char* soundName;
	const qboolean	forceLoad = cg_buildScript.integer ? 1 : 0;

	if ( !restart ) {
		cgs.media.selectSound = trap_S_RegisterSound( "sound/weapons/change.wav", qfalse );
		cgs.media.wearOffSound = trap_S_RegisterSound( "sound/items/wearoff.wav", qfalse );
		cgs.media.useNothingSound = trap_S_RegisterSound( "sound/items/use_nothing.wav", qfalse );

		// announcer
		cgs.media.oneMinuteSound = trap_S_RegisterSound( "sound/feedback/1_minute.wav", qtrue );
		cgs.media.fiveMinuteSound = trap_S_RegisterSound( "sound/feedback/5_minute.wav", qtrue );
		cgs.media.suddenDeathSound = trap_S_RegisterSound( "sound/feedback/sudden_death.wav", qtrue );
		cgs.media.count3Sound = trap_S_RegisterSound( "sound/feedback/three.wav", qtrue );
		cgs.media.count2Sound = trap_S_RegisterSound( "sound/feedback/two.wav", qtrue );
		cgs.media.count1Sound = trap_S_RegisterSound( "sound/feedback/one.wav", qtrue );
		cgs.media.countFightSound = trap_S_RegisterSound( "sound/feedback/fight.wav", qtrue );
		cgs.media.countPrepareSound = trap_S_RegisterSound( "sound/feedback/prepare.wav", qtrue );
	}
	if ( GTL( GTL_FRAGS ) ) {
		cgs.media.oneFragSound = trap_S_RegisterSound( "sound/feedback/1_frag.wav", qtrue );
		cgs.media.twoFragSound = trap_S_RegisterSound( "sound/feedback/2_frags.wav", qtrue );
		cgs.media.threeFragSound = trap_S_RegisterSound( "sound/feedback/3_frags.wav", qtrue );
	}

	if ( GTF( GTF_TEAMS ) || forceLoad ) {
#ifdef MISSIONPACK
		cgs.media.countPrepareTeamSound = trap_S_RegisterSound( "sound/feedback/prepare_team.wav", qtrue );
#endif
		//cgs.media.captureAwardSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
		for ( i = FIRST_TEAM; i <= cgs.numTeams; i++ ) {
			cgs.media.teamScoredSound[i - FIRST_TEAM] = trap_S_RegisterSound( va( "sound/feedback/%sleads.wav", cg_teamNamesLower[i] ), qtrue );
			cgs.media.teamLeadsSound[i - FIRST_TEAM] = trap_S_RegisterSound( va( "sound/teamplay/voc_%s_scores.wav", cg_teamNamesLower[i] ), qtrue );
		}
		cgs.media.teamsTiedSound = trap_S_RegisterSound( "sound/feedback/teamstied.wav", qtrue );
		cgs.media.hitTeamSound = trap_S_RegisterSound( "sound/feedback/hit_teammate.wav", qtrue );

		if ( GTF( GTF_CTF ) || forceLoad ) {
			for ( i = FIRST_TEAM; i <= cgs.numTeams; i++ ) {
				cgs.media.flagReturnedSound[i] = trap_S_RegisterSound( va( "sound/teamplay/voc_%s_returned.wav", cg_teamNamesLower[i] ), qtrue );
			}
			cgs.media.enemyPickedUpFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_enemy_flag.wav", qtrue );
			cgs.media.yourTeamPickedUpFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_team_flag.wav", qtrue );

			cgs.media.captureYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
			cgs.media.captureOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_opponent.wav", qtrue );

			cgs.media.returnYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_yourteam.wav", qtrue );
			cgs.media.returnOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );

			cgs.media.takenYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_yourteam.wav", qtrue );
			cgs.media.takenOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_opponent.wav", qtrue );
		}
		if ( GTL( GTL_CAPTURES ) || forceLoad ) {
			cgs.media.youHaveFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_you_flag.wav", qtrue );
			cgs.media.holyShitSound = trap_S_RegisterSound( "sound/feedback/voc_holyshit.wav", qtrue );
		}

		if ( cgs.gameType == GT_1FCTF || forceLoad ) {
			// FIXME: get a replacement for this sound ?
			cgs.media.flagReturnedSound[TEAM_FREE] = 0; //trap_S_RegisterSound( "sound/teamplay/voc_red_returned.wav", qtrue );
			cgs.media.yourTeamPickedUpFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_team_1flag.wav", qtrue );
			cgs.media.enemyPickedUpFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_enemy_1flag.wav", qtrue );
		} else if ( cgs.gameType == GT_OVERLOAD || forceLoad ) {
			cgs.media.yourBaseIsUnderAttackSound = trap_S_RegisterSound( "sound/teamplay/voc_base_attack.wav", qtrue );
			cgs.media.obeliskHitSound1 = trap_S_RegisterSound( "sound/items/obelisk_hit_01.wav", qfalse );	//TODO asset
			cgs.media.obeliskHitSound2 = trap_S_RegisterSound( "sound/items/obelisk_hit_02.wav", qfalse );	//TODO asset
			cgs.media.obeliskHitSound3 = trap_S_RegisterSound( "sound/items/obelisk_hit_03.wav", qfalse );	//TODO asset
			cgs.media.obeliskRespawnSound = trap_S_RegisterSound( "sound/items/obelisk_respawn.wav", qfalse );	//TODO asset
		}
	}
	if ( !restart ) {
		if ( cg_gibs.integer || forceLoad ) {
			cgs.media.gibSound = trap_S_RegisterSound( "sound/player/gibsplt1.wav", qfalse );
			cgs.media.gibBounce1Sound = trap_S_RegisterSound( "sound/player/gibimp1.wav", qfalse );
			cgs.media.gibBounce2Sound = trap_S_RegisterSound( "sound/player/gibimp2.wav", qfalse );
			cgs.media.gibBounce3Sound = trap_S_RegisterSound( "sound/player/gibimp3.wav", qfalse );
		}
#ifdef MISSIONPACK
		cgs.media.useInvulnerabilitySound = trap_S_RegisterSound( "sound/items/invul_activate.wav", qfalse );
		cgs.media.invulnerabilityImpactSound1 = trap_S_RegisterSound( "sound/items/invul_impact_01.wav", qfalse );
		cgs.media.invulnerabilityImpactSound2 = trap_S_RegisterSound( "sound/items/invul_impact_02.wav", qfalse );
		cgs.media.invulnerabilityImpactSound3 = trap_S_RegisterSound( "sound/items/invul_impact_03.wav", qfalse );
		cgs.media.invulnerabilityJuicedSound = trap_S_RegisterSound( "sound/items/invul_juiced.wav", qfalse );

#endif

		cgs.media.teleInSound = trap_S_RegisterSound( "sound/world/telein.wav", qfalse );
		cgs.media.teleOutSound = trap_S_RegisterSound( "sound/world/teleout.wav", qfalse );
		cgs.media.respawnSound = trap_S_RegisterSound( "sound/items/respawn1.wav", qfalse );

		cgs.media.noAmmoSound = trap_S_RegisterSound( "sound/weapons/noammo.wav", qfalse );

		cgs.media.talkSound = trap_S_RegisterSound( "sound/player/talk.wav", qfalse );
		cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1.wav", qfalse );

		cgs.media.hitSound = trap_S_RegisterSound( "sound/feedback/hit.wav", qfalse );
#ifdef MISSIONPACK
		cgs.media.hitSoundHighArmor = trap_S_RegisterSound( "sound/feedback/hithi.wav", qfalse );
		cgs.media.hitSoundLowArmor = trap_S_RegisterSound( "sound/feedback/hitlo.wav", qfalse );
#endif

		cgs.media.impressiveSound = trap_S_RegisterSound( "sound/feedback/impressive.wav", qtrue );
		cgs.media.excellentSound = trap_S_RegisterSound( "sound/feedback/excellent.wav", qtrue );
		cgs.media.deniedSound = trap_S_RegisterSound( "sound/feedback/denied.wav", qtrue );
		cgs.media.humiliationSound = trap_S_RegisterSound( "sound/feedback/humiliation.wav", qtrue );
#ifdef MISSIONPACK
		cgs.media.firstImpressiveSound = trap_S_RegisterSound( "sound/feedback/first_impressive.wav", qtrue );
		cgs.media.firstExcellentSound = trap_S_RegisterSound( "sound/feedback/first_excellent.wav", qtrue );
		cgs.media.firstHumiliationSound = trap_S_RegisterSound( "sound/feedback/first_gauntlet.wav", qtrue );

		cgs.media.voteNow = trap_S_RegisterSound( "sound/feedback/vote_now.wav", qtrue );
		cgs.media.votePassed = trap_S_RegisterSound( "sound/feedback/vote_passed.wav", qtrue );
		cgs.media.voteFailed = trap_S_RegisterSound( "sound/feedback/vote_failed.wav", qtrue );
#endif


		cgs.media.jumpPadSound = trap_S_RegisterSound( "sound/world/jumppad.wav", qfalse );

		cgs.media.n_healthSound = trap_S_RegisterSound( "sound/items/n_health.wav", qfalse );

		for ( i = 0; i < 4; i++ ) {
			Com_sprintf( name, sizeof( name ), "sound/player/footsteps/step%i.wav", i + 1 );
			cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound( name, qfalse );

			Com_sprintf( name, sizeof( name ), "sound/player/footsteps/boot%i.wav", i + 1 );
			cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound( name, qfalse );

			Com_sprintf( name, sizeof( name ), "sound/player/footsteps/flesh%i.wav", i + 1 );
			cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound( name, qfalse );

			Com_sprintf( name, sizeof( name ), "sound/player/footsteps/mech%i.wav", i + 1 );
			cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound( name, qfalse );

			Com_sprintf( name, sizeof( name ), "sound/player/footsteps/energy%i.wav", i + 1 );
			cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound( name, qfalse );
		}
		{	//if ( cg_levelBrushContents.integer & CONTENTS_WATER ) {
			cgs.media.watrInSound = trap_S_RegisterSound( "sound/player/watr_in.wav", qfalse );
			cgs.media.watrOutSound = trap_S_RegisterSound( "sound/player/watr_out.wav", qfalse );
			cgs.media.watrUnSound = trap_S_RegisterSound( "sound/player/watr_un.wav", qfalse );

			for ( i = 0; i < 4; i++ ) {
				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/splash%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound( name, qfalse );
			}
		}
		{	//if ( (cg_levelBrushSurfaces.integer & SURF_METALSTEPS) || forceLoad ) {
			for ( i = 0; i < 4; i++ ) {
				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/clank%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound( name, qfalse );
			}
		}
		{	//if ( (cg_levelBrushSurfaces.integer & SURF_SNOW) || forceLoad ) {
			for ( i = 0; i < 4; i++ ) {
				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/snow%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_SNOW][i] = trap_S_RegisterSound( name, qfalse );
			}
		}
		{	//if ( (cg_levelBrushSurfaces.integer & SURF_WOOD) || forceLoad ) {
			for ( i = 0; i < 4; i++ ) {
				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/wood%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_WOOD][i] = trap_S_RegisterSound( name, qfalse );
			}
		}


#ifdef MISSIONPACK
		cgs.media.sfx_proxexp = trap_S_RegisterSound( "sound/weapons/proxmine/wstbexpl.wav", qfalse );
		cgs.media.sfx_nghit = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpd.wav", qfalse );
		cgs.media.sfx_nghitflesh = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpl.wav", qfalse );
		cgs.media.sfx_nghitmetal = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpm.wav", qfalse );
		cgs.media.sfx_chghit = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpd.wav", qfalse );
		cgs.media.sfx_chghitflesh = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpl.wav", qfalse );
		cgs.media.sfx_chghitmetal = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpm.wav", qfalse );
		cgs.media.sfx_chgstop = trap_S_RegisterSound( "sound/weapons/vulcan/wvulwind.wav", qfalse );
		cgs.media.weaponHoverSound = trap_S_RegisterSound( "sound/weapons/weapon_hover.wav", qfalse );
		cgs.media.kamikazeExplodeSound = trap_S_RegisterSound( "sound/items/kam_explode.wav", qfalse );
		cgs.media.kamikazeImplodeSound = trap_S_RegisterSound( "sound/items/kam_implode.wav", qfalse );
		cgs.media.kamikazeFarSound = trap_S_RegisterSound( "sound/items/kam_explode_far.wav", qfalse );
		cgs.media.winnerSound = trap_S_RegisterSound( "sound/feedback/voc_youwin.wav", qfalse );
		cgs.media.loserSound = trap_S_RegisterSound( "sound/feedback/voc_youlose.wav", qfalse );

		cgs.media.wstbimplSound = trap_S_RegisterSound( "sound/weapons/proxmine/wstbimpl.wav", qfalse );
		cgs.media.wstbimpmSound = trap_S_RegisterSound( "sound/weapons/proxmine/wstbimpm.wav", qfalse );
		cgs.media.wstbimpdSound = trap_S_RegisterSound( "sound/weapons/proxmine/wstbimpd.wav", qfalse );
		cgs.media.wstbactvSound = trap_S_RegisterSound( "sound/weapons/proxmine/wstbactv.wav", qfalse );
#endif
	}
	if ( GTF( GTF_TEAMBASES ) || forceLoad ) {
		cgs.media.assistSound = trap_S_RegisterSound( "sound/feedback/assist.wav", qtrue );
		cgs.media.defendSound = trap_S_RegisterSound( "sound/feedback/defense.wav", qtrue );
	}
	if ( !GTF( GTF_TEAMS ) ) {
		cgs.media.takenLeadSound = trap_S_RegisterSound( "sound/feedback/takenlead.wav", qtrue );
		cgs.media.tiedLeadSound = trap_S_RegisterSound( "sound/feedback/tiedlead.wav", qtrue );
		cgs.media.lostLeadSound = trap_S_RegisterSound( "sound/feedback/lostlead.wav", qtrue );
	}


	// only register the items that the server says we need
	Q_strncpyz( items, CG_ConfigString( CS_ITEMS ), sizeof( items ) );

	for ( i = 1; i < BG_NumItems(); i++ ) {
		if ( items[ i ] == '1' || forceLoad ) {
			CG_RegisterItemSounds( i );
		}
	}

	for ( i = 1; i < MAX_SOUNDS; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS + i );
		if ( !soundName[0] ) {
			break;
		}
		if ( soundName[0] == '*' ) {
			continue;	// custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound( soundName, qfalse );
	}

}


//===================================================================================



/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t* itemInfo;
	gitem_t* item;

	if ( itemNum < 0 || itemNum >= BG_NumItems() ) {
		CG_Error( "CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, BG_NumItems() - 1 );
	}

	itemInfo = &cg_items[itemNum];
	if ( itemInfo->registered ) {
		return;
	}

	item = BG_ItemForItemNum( itemNum );

	memset( itemInfo, 0, sizeof( *itemInfo ) );
	itemInfo->registered = qtrue;

	itemInfo->models[0] = trap_R_RegisterModel( item->world_model[0] );
	//CG_Printf( "RegisterItemVisuals: icon=%s\n", item->icon );
	itemInfo->icon = CG_GetIconHandle( item->icon );

	if ( item->giType == IT_ARMOR ) {
		if ( item->giTag != ARMOR_SHARD && item->giTag != ARMOR_COMBAT ) {
			cgs.media.armorIcon[item->giTag - 1] = itemInfo->icon;
			cgs.media.armorModel[item->giTag - 1] = itemInfo->models[0];
		}
	} else if ( item->giType == IT_WEAPON ) {
		CG_RegisterWeapon( item->giTag );

	} else if ( item->giType == IT_POWERUP ) {
		switch ( item->giTag ) {
		case PW_QUAD:
			cgs.media.quadShader = trap_R_RegisterShader( "powerups/quad" );
			cgs.media.quadWeaponShader = trap_R_RegisterShader( "powerups/quadWeapon" );
			break;
		case PW_BATTLESUIT:
			cgs.media.battleSuitShader = trap_R_RegisterShader( "powerups/battleSuit" );
			cgs.media.battleWeaponShader = trap_R_RegisterShader( "powerups/battleWeapon" );
			break;
		case PW_INVIS:
			cgs.media.invisShader = trap_R_RegisterShader( "powerups/invisibility" );
			break;
		case PW_HASTE:
			cgs.media.hastePuffShader = trap_R_RegisterShader( "hasteSmokePuff" );
			break;
		case PW_REGEN:
			cgs.media.regenShader = trap_R_RegisterShader( "powerups/regen" );
			break;
		case PW_INVULN:
			cgs.media.invulnShader = trap_R_RegisterShader( "powerups/invuln" );
			cgs.media.invulnWeaponShader = trap_R_RegisterShader( "powerups/invulnWeapon" );
			break;
		default:
			break;
		}
	} else if ( item->giType == IT_HOLDABLE ) {
		switch ( item->giTag ) {
		case HI_PSCREEN:
		case HI_PSHIELD:
			cgs.media.powerShieldShader = trap_R_RegisterShader( "powerups/powerShield" );
			cgs.media.powerShieldWeaponShader = trap_R_RegisterShader( "powerups/powerShieldWeapon" );
			break;
		default: break;
		}
	} else if ( item->giType == IT_RUNE ) {
		switch ( item->giTag ) {
		case PW_SCOUT:
			cgs.media.scoutBandModel = trap_R_RegisterModel( "models/runes/band_scout.md3" );
			break;
		case PW_STRENGTH:
			cgs.media.strengthBandModel = trap_R_RegisterModel( "models/runes/band_strength.md3" );
			break;
		case PW_RESISTANCE:
			cgs.media.resistanceBandModel = trap_R_RegisterModel( "models/runes/band_resistance.md3" );
			break;
		case PW_TENACITY:
			cgs.media.tenacityBandModel = trap_R_RegisterModel( "models/runes/band_tenacity.md3" );
			break;
		case PW_PARASITE:
			cgs.media.parasiteBandModel = trap_R_RegisterModel( "models/runes/band_parasite.md3" );
			break;
		case PW_ARMAMENT:
			cgs.media.armamentBandModel = trap_R_RegisterModel( "models/runes/band_armament.md3" );
			break;
		default: break;
		}
	}

	//
	// powerups have an accompanying ring or sphere
	//
	if ( item->giType == IT_POWERUP || item->giType == IT_HEALTH ) {
		if ( item->world_model[1] ) {
			itemInfo->models[1] = trap_R_RegisterModel( item->world_model[1] );
		}
	}
}


/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
void CG_RegisterGraphics( qboolean restart ) {
	int				i;
	char			items[MAX_ITEMS + 1];
	const qboolean	bs = cg_buildScript.integer;

	if ( restart ) goto load;

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	CG_LoadingString( cgs.mapname );

	trap_R_LoadWorldMap( cgs.mapname );

	CG_LoadingString( "entities" );

	CG_ParseEntitiesFromString();
	
	// precache status bar pics
	CG_LoadingString( "game media" );

	if ( cgs.bots_enabled ) {
		cgs.media.botSkillShaders[0] = trap_R_RegisterShader( "menu/art/skill1.tga" );
		cgs.media.botSkillShaders[1] = trap_R_RegisterShader( "menu/art/skill2.tga" );
		cgs.media.botSkillShaders[2] = trap_R_RegisterShader( "menu/art/skill3.tga" );
		cgs.media.botSkillShaders[3] = trap_R_RegisterShader( "menu/art/skill4.tga" );
		cgs.media.botSkillShaders[4] = trap_R_RegisterShader( "menu/art/skill5.tga" );
	}
	if ( !restart ) {
		cgs.media.viewBloodShader = trap_R_RegisterShader( "viewBloodBlend" );

		cgs.media.deferShader = trap_R_RegisterShaderNoMip( "gfx/2d/defer.tga" );

		cgs.media.deathIcon = CG_GetIconHandle( "icons/hazard.tga" );

		cgs.media.scoreboardName = trap_R_RegisterShaderNoMip( "menu/tab/name.tga" );
		cgs.media.scoreboardPing = trap_R_RegisterShaderNoMip( "menu/tab/ping.tga" );
		cgs.media.scoreboardScore = trap_R_RegisterShaderNoMip( "menu/tab/score.tga" );
		cgs.media.scoreboardTime = trap_R_RegisterShaderNoMip( "menu/tab/time.tga" );

		cgs.media.smokePuffShader = trap_R_RegisterShader( "smokePuff" );
#ifdef MISSIONPACK
		cgs.media.nailPuffShader = trap_R_RegisterShader( "nailtrail" );
		cgs.media.blueProxMine = trap_R_RegisterModel( "models/weaphits/proxmineb.md3" );
#endif

		cgs.media.lagometerShader = trap_R_RegisterShader( "lagometer" );
		cgs.media.connectionShader = trap_R_RegisterShader( "disconnected" );

		cgs.media.tracerShader = trap_R_RegisterShader( "gfx/misc/tracer" );
		cgs.media.selectShader = trap_R_RegisterShader( "gfx/2d/select" );

		if ( cg_blood.integer ) {
			cgs.media.bloodTrailShader = trap_R_RegisterShader( "bloodTrail" );
		}
		{	//if ( cg_levelBrushContents.integer & CONTENTS_WATER ) {
			cgs.media.waterBubbleShader = trap_R_RegisterShader( "waterBubble" );
		}
		if ( cg_viewSize.integer < 100 ) {
			cgs.media.backTileShader = trap_R_RegisterShader( "gfx/2d/backtile" );
		}
		cgs.media.noammoShader = CG_GetIconHandle( "icons/noammo" );
	}
load:
	if ( cgs.gameType == GT_HARVESTER || bs ) {
		cgs.media.skullModel = trap_R_RegisterModel( "models/powerups/harvester/skull.md3" );
	}

	//if ( GTL(GTL_CAPTURES) || cgs.gameType == GT_HARVESTER || bs ) {
	if ( GTL( GTL_CAPTURES ) || bs ) {
		cgs.media.flagModel = trap_R_RegisterModel( "models/flags/flag.md3" );
		cgs.media.flagsShader[0] = CG_GetIconHandle( "gfx/teamplay/flagstatus1" );
		cgs.media.flagsShader[1] = CG_GetIconHandle( "gfx/teamplay/flagstatus2" );
		cgs.media.flagsShader[2] = CG_GetIconHandle( "gfx/teamplay/flagstatus3" );
//#ifdef MISSIONPACK
		//TODO assets
		cgs.media.flagPoleModel = trap_R_RegisterModel( "models/flag/pole.md3" );
		cgs.media.flagFlapModel = trap_R_RegisterModel( "models/flag/flap.md3" );
#if 0
		CG_RegisterSkin( "models/flag2/red.skin", &cgs.media.redFlagFlapSkin, qfalse );
		CG_RegisterSkin( "models/flag2/blue.skin", &cgs.media.blueFlagFlapSkin, qfalse );
		CG_RegisterSkin( "models/flag2/white.skin", &cgs.media.neutralFlagFlapSkin, qfalse );
#endif
	}
	if ( GTL( GTL_CAPTURES ) || cgs.gameType == GT_HARVESTER || bs ) {
#if 0
		cgs.media.redFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/red_base.md3" );
		cgs.media.blueFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/blue_base.md3" );
		cgs.media.neutralFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/ntrl_base.md3" );
#endif
		cgs.media.flagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/base.md3" );
//#endif
	}

//TODO assets
	if ( cgs.gameType == GT_1FCTF || bs ) {
#if 0
		cgs.media.neutralFlagModel = trap_R_RegisterModel( "models/flags/n_flag.md3" );
		cgs.media.flagStateShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral1" );
		cgs.media.flagStateShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.flagStateShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.flagStateShader[3] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral3" );
#endif
	}

	if ( cgs.gameType == GT_OVERLOAD || bs ) {
		cgs.media.rocketExplosionShader = trap_R_RegisterShader( "rocketExplosion" );
		cgs.media.overloadBaseModel = trap_R_RegisterModel( "models/powerups/overload_base.md3" );
		cgs.media.overloadTargetModel = trap_R_RegisterModel( "models/powerups/overload_target.md3" );
		cgs.media.overloadLightsModel = trap_R_RegisterModel( "models/powerups/overload_lights.md3" );
		cgs.media.overloadEnergyModel = trap_R_RegisterModel( "models/powerups/overload_energy.md3" );
	}

	if ( cgs.gameType == GT_HARVESTER || cgs.gameType == GT_1FCTF || bs ) {
#if 0
		cgs.media.harvesterModel = trap_R_RegisterModel( "models/powerups/harvester/harvester.md3" );
		CG_RegisterSkin( "models/powerups/harvester/red.skin", &cgs.media.harvesterRedSkin, qfalse );
		CG_RegisterSkin( "models/powerups/harvester/blue.skin", &cgs.media.harvesterBlueSkin, qfalse );
#endif
		cgs.media.baseRecepticleModel = trap_R_RegisterModel( "models/powerups/teambase/recepticle.md3" );
		//CG_RegisterSkin( "models/powerups/teambase/recepticle.skin", &cgs.media.baseRecepticleSkin, qfalse );
		cgs.media.skullGeneratorModel = trap_R_RegisterModel( "models/powerups/harvester/skullgen.md3" );
	}
	cgs.media.dustPuffShader = trap_R_RegisterShader( "hasteSmokePuff" );
#ifdef MISSIONPACK
	cgs.media.redKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikred" );
#endif
	if ( GTF( GTF_TEAMS ) || bs ) {
		cgs.media.friendShader = trap_R_RegisterShader( "sprites/foe" );
		cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" );
#ifdef MISSIONPACK
		cgs.media.blueKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikblu" );
#endif
	}
	if ( !restart ) {
		{
			gitem_t* it = BG_FindItemByClassname( "item_armor_combat" );
			if ( it ) {
				cgs.media.armorModel[1] = trap_R_RegisterModel( it->world_model[0] );
				cgs.media.armorIcon[1] = CG_GetIconHandle( it->icon );
			}
		}
#if 0
		if ( cg_draw3DIcons.integer ) {
			cgs.media.armorModel[0] = trap_R_RegisterModel( "models/powerups/armor/armor_grn.md3" );
			cgs.media.armorModel[1] = trap_R_RegisterModel( "models/powerups/armor/armor_yel.md3" );
			cgs.media.armorModel[2] = trap_R_RegisterModel( "models/powerups/armor/armor_red.md3" );
		} else {
			cgs.media.armorIcon[0] = CG_GetIconHandle( "icons/iconr_green" );
			cgs.media.armorIcon[1] = CG_GetIconHandle( "icons/iconr_yellow" );
			cgs.media.armorIcon[2] = CG_GetIconHandle( "icons/iconr_red" );
		}
#endif
		if ( cg_gibs.integer || bs ) {
			cgs.media.gibAbdomen = trap_R_RegisterModel( "models/gibs/abdomen.md3" );
			cgs.media.gibArm = trap_R_RegisterModel( "models/gibs/arm.md3" );
			cgs.media.gibChest = trap_R_RegisterModel( "models/gibs/chest.md3" );
			cgs.media.gibFist = trap_R_RegisterModel( "models/gibs/fist.md3" );
			cgs.media.gibFoot = trap_R_RegisterModel( "models/gibs/foot.md3" );
			cgs.media.gibForearm = trap_R_RegisterModel( "models/gibs/forearm.md3" );
			cgs.media.gibIntestine = trap_R_RegisterModel( "models/gibs/intestine.md3" );
			cgs.media.gibLeg = trap_R_RegisterModel( "models/gibs/leg.md3" );
			cgs.media.gibSkull = trap_R_RegisterModel( "models/gibs/skull.md3" );
			cgs.media.gibBrain = trap_R_RegisterModel( "models/gibs/brain.md3" );
		}
		if ( cg_blood.integer || bs ) {
			cgs.media.bloodExplosionShader = trap_R_RegisterShader( "bloodExplosion" );
		}
		cgs.media.balloonShader = trap_R_RegisterShader( "sprites/balloon3" );

		cgs.media.coronaShader = trap_R_RegisterShader( "flareShader" );

		cgs.media.bulletFlashModel = trap_R_RegisterModel( "models/weaphits/bullet.md3" );
		cgs.media.ringFlashModel = trap_R_RegisterModel( "models/weaphits/ring02.md3" );
		cgs.media.dishFlashModel = trap_R_RegisterModel( "models/weaphits/boom01.md3" );

		cgs.media.teleportEffectModel = trap_R_RegisterModel( "models/misc/telep.md3" );
		cgs.media.teleportEffectShader = trap_R_RegisterShader( "teleportEffect" );
#ifdef MISSIONPACK
		cgs.media.kamikazeEffectModel = trap_R_RegisterModel( "models/weaphits/kamboom2.md3" );
		cgs.media.kamikazeShockWave = trap_R_RegisterModel( "models/weaphits/kamwave.md3" );
		cgs.media.kamikazeHeadModel = trap_R_RegisterModel( "models/powerups/kamikazi.md3" );
		cgs.media.kamikazeHeadTrail = trap_R_RegisterModel( "models/powerups/trailtest.md3" );
		cgs.media.invulnerabilityImpactModel = trap_R_RegisterModel( "models/powerups/shield/impact.md3" );
		cgs.media.invulnerabilityJuicedModel = trap_R_RegisterModel( "models/powerups/shield/juicer.md3" );
		cgs.media.medkitUsageModel = trap_R_RegisterModel( "models/powerups/regen.md3" );
		cgs.media.heartShader = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/selectedhealth.tga" );
		cgs.media.invulnerabilityPowerupModel = trap_R_RegisterModel( "models/powerups/shield/shield.md3" );
#endif
	}
	if ( !GTF( GTF_CAMPAIGN ) ) {

		cgs.media.medalImpressive = trap_R_RegisterShaderNoMip( "medal_impressive" );
		cgs.media.medalExcellent = trap_R_RegisterShaderNoMip( "medal_excellent" );
		cgs.media.medalGauntlet = trap_R_RegisterShaderNoMip( "medal_gauntlet" );
	}
	if ( GTF( GTF_TEAMBASES ) ) {
		cgs.media.medalDefend = trap_R_RegisterShaderNoMip( "medal_defend" );
		cgs.media.medalAssist = trap_R_RegisterShaderNoMip( "medal_assist" );
		cgs.media.medalHolyShit = trap_R_RegisterShaderNoMip( "medal_holyshit" );
	}
	if ( GTL( GTL_CAPTURES ) ) {
		cgs.media.medalCapture = trap_R_RegisterShaderNoMip( "medal_capture" );
	}

	memset( cg_items, 0, sizeof( cg_items ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	// only register the items that the server says we need
	Q_strncpyz( items, CG_ConfigString( CS_ITEMS ), sizeof( items ) );

	for ( i = 1; i < BG_NumItems(); i++ ) {
		if ( items[i] == '1' || bs ) {
			if ( !restart ) CG_LoadingItem( i );
			CG_RegisterItemVisuals( i );
		}
	}
	if ( restart ) return;

	// crosshairs
	CG_RegisterCrosshair();

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader( "gfx/damage/bullet_mrk" );
	cgs.media.burnMarkShader = trap_R_RegisterShader( "gfx/damage/burn_med_mrk" );
	cgs.media.holeMarkShader = trap_R_RegisterShader( "gfx/damage/hole_lg_mrk" );
	cgs.media.energyMarkShader = trap_R_RegisterShader( "gfx/damage/plasma_mrk" );
	cgs.media.shadowMarkShader = trap_R_RegisterShader( "markShadow" );
	{	//if ( cg_levelBrushContents.integer & CONTENTS_WATER ) {
		cgs.media.wakeMarkShader = trap_R_RegisterShader( "wake" );
	}
	if ( cg_blood.integer || bs ) {
		cgs.media.bloodMarkShader = trap_R_RegisterShader( "bloodMark" );
	}
	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();

	if ( cgs.numInlineModels > MAX_SUBMODELS ) {
		CG_Error( "MAX_SUBMODELS (%d) exceeded by %d", MAX_SUBMODELS, cgs.numInlineModels - MAX_SUBMODELS );
	}

	for ( i = 1; i < cgs.numInlineModels; i++ ) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof( name ), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs, 0, 0, 0 );
		for ( j = 0; j < 3; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * (maxs[j] - mins[j]);
		}
	}

	// register all the server specified models
	for ( i = 1; i < MAX_MODELS; i++ ) {
		const char* modelName;

		modelName = CG_ConfigString( CS_MODELS + i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel( modelName );
	}

	CG_CacheParticles();
/*
	for (i=1; i<MAX_PARTICLES_AREAS; i++)
	{
		{
			int rval;

			rval = CG_NewParticleArea ( CS_PARTICLES + i);
			if (!rval)
				break;
		}
	}
*/
}


/*
==================
CG_LocalPlayerAdded
==================
*/
void CG_LocalPlayerAdded( int localPlayerNum, int playerNum ) {
	if ( playerNum < 0 || playerNum >= MAX_CLIENTS )
		return;

	cg.localPlayers[localPlayerNum].playerNum = playerNum;

	CG_LoadDeferredPlayers();
}

/*
==================
CG_LocalPlayerRemoved
==================
*/
void CG_LocalPlayerRemoved( int localPlayerNum ) {
	if ( cg.localPlayers[localPlayerNum].playerNum == -1 )
		return;

	Com_Memset( &cg.localPlayers[localPlayerNum], 0, sizeof( cg.localPlayers[0] ) );

	cg.localPlayers[localPlayerNum].playerNum = -1;
}

#ifdef MISSIONPACK
/*
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString( void ) {
	int i;
	cg.spectatorList[0] = 0;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( cgs.playerinfo[i].infoValid && cgs.playerinfo[i].team == TEAM_SPECTATOR ) {
			Q_strcat( cg.spectatorList, sizeof( cg.spectatorList ), va( "%s     ", cgs.playerinfo[i].name ) );
		}
	}
}
#endif


/*
===================
CG_RegisterPlayers
===================
*/
static void CG_RegisterPlayers( void ) {
	int		i;
	int		j;

	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		if ( cg.localPlayers[i].playerNum == -1 ) {
			continue;
		}
		CG_LoadingPlayer( cg.localPlayers[i].playerNum );
		CG_NewPlayerInfo( cg.localPlayers[i].playerNum );
	}

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		const char* playerInfo;

		for ( j = 0; j < CG_MaxSplitView(); j++ ) {
			if ( cg.localPlayers[j].playerNum == i ) {
				break;
			}
		}
		if ( j != CG_MaxSplitView() ) {
			continue;
		}

		playerInfo = CG_ConfigString( CS_PLAYERS + i );
		if ( !playerInfo[0] ) {
			continue;
		}
		CG_LoadingPlayer( i );
		CG_NewPlayerInfo( i );
	}
#ifdef MISSIONPACK
	CG_BuildSpectatorString();
#endif
	}

	//===========================================================================

	/*
	=================
	CG_ConfigString
	=================
	*/
const char* CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[index];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( void ) {
	char* s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char*)CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2, 1.0f, 1.0f );
}


/*
=================
CG_ClearState

Called at init and killing server from UI
=================
*/
void CG_ClearState( qboolean everything, int maxSplitView ) {
	int i;

	if ( everything ) {
		memset( &cgs, 0, sizeof( cgs ) );
		cgs.maxSplitView = Com_Clamp( 1, MAX_SPLITVIEW, maxSplitView );
	}
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof( cg_entities ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );
	memset( cg_items, 0, sizeof( cg_items ) );

	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		cg.localPlayers[i].playerNum = -1;
	}

	// get the rendering configuration from the client system
	CG_UpdateGlconfig( qtrue );
}

/*
=================
CG_SetConnectionState
=================
*/
void CG_SetConnectionState( connstate_t state ) {
	int i;

	if ( cg.connState == state ) {
		return;
	}

	cg.connState = state;
	cg.connected = (cg.connState > CA_CONNECTED && cg.connState != CA_CINEMATIC);

	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		CG_UpdateMouseState( i );
	}
}

/*
=================
CG_Init

Called after every cgame load, such as main menu, level change, or subsystem restart
=================
*/
void CG_Init( connstate_t state, int maxSplitView, int playVideo ) {
	Swap_Init();

	// clear everything
	CG_ClearState( qtrue, maxSplitView );

	CG_SetConnectionState( state );

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	cg.iconImageSize = (float)ICON_SIZE * ((float)cgs.glconfig.vidHeight / (float)SCREEN_HEIGHT);
	if ( cg.iconImageSize ) {
		int num;

		num = 32;
		while ( 1 ) {
			if ( cg.iconImageSize <= num * 1.5 ) {
				cg.iconImageSize = num;
				break;
			}
			num *= 2;
		}

		if ( cg.iconImageSize == ICON_SIZE )
			cg.iconImageSize = 0;
	}

	// load a few needed things before we do any screen updates
	cgs.media.whiteShader = trap_R_RegisterShader( "white" );
	cgs.media.consoleShader = trap_R_RegisterShader( "console" );
	cgs.media.nodrawShader = trap_R_RegisterShaderEx( "nodraw", LIGHTMAP_NONE, qtrue );
	cgs.media.whiteDynamicShader = trap_R_RegisterShaderEx( "white", LIGHTMAP_NONE, qtrue );

	cg.obitNum = 0;
	memset( cg.obitAttacker, 0, sizeof( cg.obitAttacker ) );
	memset( cg.obitTime, 0, sizeof( cg.obitTime ) );
	memset( cg.obitTarget, 0, sizeof( cg.obitTarget ) );
	memset( cg.obitMOD, 0, sizeof( cg.obitMOD ) );
	cg.notifyNum = 0;
	memset( cg.notifyTime, 0, sizeof( cg.notifyTime ) );
	memset( cg.notifyText, 0, sizeof( cg.notifyText ) );
	cg.notifyExpand = qfalse;

	CG_ConsoleInit();

	if ( cg_dedicated.integer ) {
		Key_SetCatcher( KEYCATCH_CONSOLE );
		return;
	}

	UI_Init( cg.connected, maxSplitView );

	// if the user didn't give any commands, run default action
	if ( playVideo == 1 ) {
		if ( cg_playIntros.integer ) {
			trap_Cmd_ExecuteText( EXEC_NOW, "cinematic idlogo.RoQ\n" );
			if ( !cg_introPlayed.integer || cg_playIntros.integer > 1 ) {
				trap_Cvar_SetValue( "cg_introPlayed", 1 );
				trap_Cvar_Set( "nextmap", "cinematic intro.RoQ" );
			}
		}
	}
}

#ifdef MISSIONPACK
void CG_InitTeamChat( void ) {
	memset( teamChat1, 0, sizeof( teamChat1 ) );
	memset( teamChat2, 0, sizeof( teamChat2 ) );
	memset( systemChat, 0, sizeof( systemChat ) );
}
#endif

/*
=================
CG_Ingame_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Ingame_Init( int serverMessageNum, int serverCommandSequence, int maxSplitView, int playerNum0 ) {	// , int playerNum1, int playerNum2, int playerNum3 ) {
	int	playerNums[MAX_SPLITVIEW];
	const char* s;
	int			i;

	cgs.maxSplitView = Com_Clamp( 1, MAX_SPLITVIEW, maxSplitView );
	cg.numViewports = 1;

	playerNums[0] = playerNum0;
#if 0
	playerNums[1] = playerNum1;
	playerNums[2] = playerNum2;
	playerNums[3] = playerNum3;
#endif

	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		// clear team preference if was previously set (only want it used for one game)
		trap_Cvar_Set( "teamPref", "" );

		if ( playerNums[i] < 0 || playerNums[i] >= MAX_CLIENTS ) {
			cg.localPlayers[i].playerNum = -1;
			continue;
		}

		trap_GetViewAngles( i, cg.localPlayers[i].viewangles );
		CG_LocalPlayerAdded( i, playerNums[i] );
	}

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		cg.localPlayers[i].weaponSelect = WP_MACHINEGUN;
	}

	for ( i = 0; i < TEAM_NUM_TEAMS; i++ ) {
		cgs.flagStatus[i] = -1;
	}

	// old servers

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	// check game protocol
	s = CG_ConfigString( CS_GAME_PROTOCOL );
	if ( strcmp( s, GAME_PROTOCOL ) ) {
		CG_Error( "Client/Server game mismatch: %s/%s", GAME_PROTOCOL, s );
	}

	CG_HudTextInit();

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	trap_SetMapTitle( CG_ConfigString( CS_MESSAGE ) );
	trap_SetNetFields( sizeof( entityState_t ), sizeof( entityState_t ) - sizeof( int ), bg_entityStateFields, bg_numEntityStateFields,
		sizeof( playerState_t ), 0, bg_playerStateFields, bg_numPlayerStateFields );


	CG_ParseServerinfo();

	// load the new map
	CG_LoadingString( "collision map" );

	trap_CM_LoadMap( cgs.mapname );

	cg.loading = qtrue;		// force players to load instead of defer

	CG_LoadingString( "sounds" );

	CG_RegisterSounds( qfalse );

	CG_LoadingString( "graphics" );

	CG_RegisterGraphics( qfalse );

	CG_LoadingString( "players" );

	CG_RegisterPlayers();		// if low on memory, some players will be deferred

	cg.loading = qfalse;	// future players will be deferred

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	cg.lightstylesInited = qfalse;

	CG_LoadingString( "" );

#ifdef MISSIONPACK
	CG_InitTeamChat();
#endif

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds( qtrue );

	CG_RestoreSnapshot();
}

/*
=================
CG_KillServer

Called by UI to kill local server
=================
*/
void CG_KillServer( void ) {
	if ( !cgs.localServer ) {
		return;
	}

	trap_SV_Shutdown( "Server was killed" );

	CG_ClearState( qfalse, cgs.maxSplitView );

	cgs.localServer = qfalse;
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void ) {
	int i;

	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		trap_SetViewAngles( i, cg.localPlayers[i].viewangles );
	}

	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}

/*
=================
CG_Refresh

Draw the frame
=================
*/
void CG_Refresh( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback, connstate_t state, int realTime ) {
	int i;

	CG_SetConnectionState( state );
	cg.realFrameTime = realTime - cg.realTime;
	cg.realTime = realTime;

	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		CG_UpdateMouseState( i );
	}

	// update cvars
	CG_UpdateCvars();

	if ( state == CA_CINEMATIC && cg.cinematicPlaying ) {
		float x, y, width, height;

		x = 0;
		y = 0;
		width = SCREEN_WIDTH;
		height = SCREEN_HEIGHT;
		CG_SetScreenPlacement( PLACE_CENTER, PLACE_CENTER );
		CG_AdjustFrom640( &x, &y, &width, &height );

		trap_CIN_SetExtents( cg.cinematicHandle, x, y, width, height );

		CG_ClearViewport();
		trap_CIN_DrawCinematic( cg.cinematicHandle );

		if ( trap_CIN_RunCinematic( cg.cinematicHandle ) == FMV_EOF ) {
			CG_StopCinematic_f();
		}
	}

	if ( !cg_dedicated.integer && state == CA_DISCONNECTED && !UI_IsFullscreen() ) {
		// if disconnected, bring up the menu
		trap_S_StopAllSounds();
		UI_SetActiveMenu( UIMENU_MAIN );
	}

	if ( state >= CA_LOADING && state != CA_CINEMATIC && !UI_IsFullscreen() ) {
		if ( !GTF( GTF_CAMPAIGN ) || state == CA_ACTIVE )
			CG_DrawActiveFrame( serverTime, stereoView, demoPlayback );
	}

	if ( (state > CA_DISCONNECTED && state <= CA_LOADING) || (Key_GetCatcher() & KEYCATCH_UI) ) {
		if ( ui_stretch.integer ) {
			CG_SetScreenPlacement( PLACE_STRETCH, PLACE_STRETCH );
		} else {
			CG_SetScreenPlacement( PLACE_CENTER, PLACE_CENTER );
		}
		if ( !GTF( GTF_CAMPAIGN ) || (Key_GetCatcher() & KEYCATCH_UI) )
			UI_Refresh( realTime );
	}

	// connecting clients will show the connection dialog
	if ( state >= CA_CONNECTING && state < CA_ACTIVE ) {
		UI_DrawConnectScreen( (state >= CA_LOADING) );
	}

	CG_RunConsole( state );
}

/*
===================
CG_BindUICommand

Returns qtrue if bind command should be executed while user interface is shown
===================
*/
static qboolean CG_BindUICommand( const char* cmd ) {
	if ( Key_GetCatcher() & KEYCATCH_CONSOLE )
		return qfalse;

	if ( !Q_stricmp( cmd, "toggleConsole" ) )
		return qtrue;
	if ( !Q_stricmp( cmd, "toggleMenu" ) )
		return qtrue;

	return qfalse;
}

/*
===================
CG_ParseBinding

Execute the commands in the bind string

key up events only perform actions if the game key binding is
a button command (leading + sign).  These will be processed even in
console mode and menu mode, to keep the character from continuing
an action started before a mode switch.

===================
*/
void CG_ParseBinding( int key, qboolean down, unsigned time, connstate_t state, int keyCatcher, int joystickNum, int axisNum ) {
	char buf[MAX_STRING_CHARS], * p = buf, * end;
	qboolean allCommands, allowUpCmds;

	if ( state == CA_DISCONNECTED && keyCatcher == 0 )
		return;

	trap_Key_GetBindingBuf( key, buf, sizeof( buf ) );

	if ( !buf[0] )
		return;

	// run all bind commands if console, ui, etc aren't reading keys
	allCommands = (keyCatcher == 0);

	// allow button up commands if in game even if key catcher is set
	allowUpCmds = (state != CA_DISCONNECTED);

	while ( 1 ) {
		while ( isspace( *p ) )
			p++;
		end = strchr( p, ';' );
		if ( end )
			*end = '\0';
		if ( *p == '+' ) {
			// button commands add keynum and time as parameters
			// so that multiple sources can be discriminated and
			// subframe corrected
			if ( allCommands || (allowUpCmds && !down) ) {
				char cmd[1024];
				Com_sprintf( cmd, sizeof( cmd ), "%c%s %d %d %d %d\n",
					(down) ? '+' : '-', p + 1, key, time, joystickNum, axisNum );
				trap_Cmd_ExecuteText( EXEC_APPEND, cmd );
			}
		} else if ( down ) {
			// normal commands only execute on key press
			if ( allCommands || CG_BindUICommand( p ) ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, p );
				trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
			}
		}
		if ( !end )
			break;
		p = end + 1;
	}
}

/*
================
Message_Key

In game talk message
================
*/
void Message_Key( int key, qboolean down ) {
	char	buffer[MAX_STRING_CHARS];

	if ( !down ) {
		return;
	}

	if ( key & K_CHAR_FLAG ) {
		key &= ~K_CHAR_FLAG;
		MField_CharEvent( &cg.messageField, key );
		return;
	}

	if ( key == K_ESCAPE ) {
		Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_MESSAGE );
		MField_Clear( &cg.messageField );
		return;
	}

	if ( key == K_ENTER || key == K_KP_ENTER ) {
		if ( cg.messageField.buffer[0] && cg.connected ) {
			Com_sprintf( buffer, sizeof( buffer ), "%s %s\n", cg.messageCommand, MField_Buffer( &cg.messageField ) );

			trap_SendClientCommand( buffer );
		}

		Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_MESSAGE );
		MField_Clear( &cg.messageField );
		return;
	}

	MField_KeyDownEvent( &cg.messageField, key );
}

/*
================
CG_DistributeKeyEvent
================
*/
void CG_DistributeKeyEvent( int key, qboolean down, unsigned time, connstate_t state, int joystickNum, int axisNum ) {
	int keyCatcher;

	CG_SetConnectionState( state );

	switch ( key ) {
	case K_KP_PGUP:
	case K_KP_EQUALS:
	case K_KP_5:
	case K_KP_LEFTARROW:
	case K_KP_UPARROW:
	case K_KP_RIGHTARROW:
	case K_KP_DOWNARROW:
	case K_KP_END:
	case K_KP_PGDN:
	case K_KP_INS:
	case K_KP_DEL:
	case K_KP_HOME:
		if ( !UI_WantsBindKeys() && trap_Key_GetNumLockMode() ) {
			return;
		}
		break;
	default:
		break;
	}

	// console key is hardcoded, so the user can never unbind it
	if ( key == K_CONSOLE || (key == K_ESCAPE && (trap_Key_IsDown( K_LEFTSHIFT ) || trap_Key_IsDown( K_RIGHTSHIFT ))) ) {
		if ( down ) {
			Con_ToggleConsole_f();
		}
		return;
	}

	keyCatcher = Key_GetCatcher();

	// keys can still be used for bound actions
	if ( (key < 128 || key == K_MOUSE1
		|| key == K_JOY_A || key == K_2JOY_A || key == K_3JOY_A || key == K_4JOY_A) &&
		(trap_GetDemoState() == DS_PLAYBACK || state == CA_CINEMATIC) && keyCatcher == 0 ) {

		if ( cg_cameraMode.integer == 0 ) {
			trap_Cvar_Set( "nextdemo", "" );
			key = K_ESCAPE;
		}
	}

	// escape is always handled special
	if ( key == K_ESCAPE ) {
		if ( down && !(keyCatcher & (KEYCATCH_UI | KEYCATCH_CGAME | KEYCATCH_MESSAGE)) ) {
			if ( state == CA_ACTIVE && trap_GetDemoState() != DS_PLAYBACK ) {
				UI_SetActiveMenu( UIMENU_INGAME );
			} else if ( state == CA_CINEMATIC ) {
				CG_StopCinematic_f();
			} else if ( state != CA_DISCONNECTED ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
			}

			return;
		}

		// skip console
		keyCatcher &= ~KEYCATCH_CONSOLE;
	} else {
		// send the bound action
		CG_ParseBinding( key, down, time, state, keyCatcher, joystickNum, axisNum );
	}

	// distribute the key down event to the apropriate handler
	if ( keyCatcher & KEYCATCH_CONSOLE ) {
		Console_Key( key, down );
	} else if ( keyCatcher & KEYCATCH_MESSAGE ) {
		Message_Key( key, down );
	} else if ( cg.connected && (keyCatcher & KEYCATCH_CGAME) ) {
		CG_KeyEvent( key, down );
	} else if ( keyCatcher & KEYCATCH_UI ) {
		UI_KeyEvent( key, down );
	} else if ( state == CA_DISCONNECTED ) {
		// console is drawn if disconnected and not KEYCATCH_UI
		Console_Key( key, down );
	}
}

/*
================
CG_DistributeCharEvent
================
*/
void CG_DistributeCharEvent( int character, connstate_t state ) {
	int key, keyCatcher;

	CG_SetConnectionState( state );

	key = (character | K_CHAR_FLAG);

	keyCatcher = Key_GetCatcher();

	// distribute the character event to the apropriate handler
	if ( keyCatcher & KEYCATCH_CONSOLE ) {
		Console_Key( key, qtrue );
	} else if ( keyCatcher & KEYCATCH_MESSAGE ) {
		Message_Key( key, qtrue );
	} else if ( cg.connected && (keyCatcher & KEYCATCH_CGAME) ) {
		CG_KeyEvent( key, qtrue );
	} else if ( keyCatcher & KEYCATCH_UI ) {
		UI_KeyEvent( key, qtrue );
	} else if ( state == CA_DISCONNECTED ) {
		// console is drawn if disconnected and not KEYCATCH_UI
		Console_Key( key, qtrue );
	}
}

/*
====================
CG_UpdateMouseState
====================
*/
void CG_UpdateMouseState( int localPlayerNum ) {
	int state = 0;

	if ( Key_GetCatcher() & KEYCATCH_CONSOLE ) {
		// no grab, show system cursor
		state |= MOUSE_SYSTEMCURSOR;
	}

	// controling UI mouse cursor
	if ( Key_GetCatcher() & KEYCATCH_UI ) {
		// call mouse move event, no grab, hide system cursor
		state |= MOUSE_CGAME;
	}
	// not controlling view angles
	else if ( cg.demoPlayback || cg.connState != CA_ACTIVE
		|| (cg.snap && (cg.snap->pss[localPlayerNum].pm_flags & (PMF_FOLLOW | PMF_SCOREBOARD))) ) {
	// no grab, show system cursor
		state |= MOUSE_SYSTEMCURSOR;
	}
	// if console isn't open, not UI, and not other non-view angle modes
	else if ( state == 0 ) {
		// change viewangles, grab mouse, hide system cursor
		state = MOUSE_CLIENT;
	}

	trap_Mouse_SetState( localPlayerNum, state );
}

static int keyCatchers = 0;

/*
====================
Key_GetCatcher
====================
*/
int Key_GetCatcher( void ) {
	return keyCatchers;
}

/*
====================
Key_SetCatcher
====================
*/
void Key_SetCatcher( int catcher ) {
	// If the catcher state is changing, clear all key states
	if ( catcher != keyCatchers ) {
		trap_Key_ClearStates();

		// If catcher is 0, disable held key repeating so binds don't repeat
		trap_Key_SetRepeat( catcher != 0 );
	}

	keyCatchers = catcher;

	CG_UpdateMouseState( 0 );
}

/*
==================
CG_EventHandling
==================
 type 0 - no event handling
	  1 - team menu
	  2 - hud editor

*/
void CG_EventHandling( int type ) {
}

void CG_KeyEvent( int key, qboolean down ) {
}

void CG_MouseEvent( int localPlayerNum, int x, int y ) {
}

/*
=================
CG_MousePosition

returns bitshifted combo of x and y in window coords
=================
*/
static int CG_MousePosition( int localPlayerNum ) {
	float ax, ay, aw, ah, xbias, ybias;
	int cursorx, cursory;
	int	x, y;

	if ( ui_stretch.integer ) {
		CG_SetScreenPlacement( PLACE_STRETCH, PLACE_STRETCH );
	} else {
		CG_SetScreenPlacement( PLACE_CENTER, PLACE_CENTER );
	}

	ax = 0;
	ay = 0;
	aw = 1;
	ah = 1;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	xbias = ax / aw;
	ybias = ay / ah;

	UI_GetCursorPos( localPlayerNum, &cursorx, &cursory );

	x = (cursorx + xbias) / (SCREEN_WIDTH + xbias * 2) * cgs.glconfig.vidWidth;
	y = (cursory + ybias) / (SCREEN_HEIGHT + ybias * 2) * cgs.glconfig.vidHeight;

	return x | (y << 16);
}

/*
=================
CG_SetMousePosition

x and y are in window coords
=================
*/
static void CG_SetMousePosition( int localPlayerNum, int x, int y ) {
	float ax, ay, aw, ah, xbias, ybias;
	int cursorx, cursory;

	if ( ui_stretch.integer ) {
		CG_SetScreenPlacement( PLACE_STRETCH, PLACE_STRETCH );
	} else {
		CG_SetScreenPlacement( PLACE_CENTER, PLACE_CENTER );
	}

	ax = 0;
	ay = 0;
	aw = 1;
	ah = 1;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	xbias = ax / aw;
	ybias = ay / ah;

	cursorx = (float)x / cgs.glconfig.vidWidth * (SCREEN_WIDTH + xbias * 2) - xbias;
	cursory = (float)y / cgs.glconfig.vidHeight * (SCREEN_HEIGHT + ybias * 2) - ybias;

	UI_SetCursorPos( localPlayerNum, cursorx, cursory );
}

/*
=================
CG_JoystickEvent

Internal function for general joystick event handling (not called for up events).
=================
*/
void CG_JoystickEvent( int localPlayerNum, const joyevent_t* joyevent ) {
	if ( cg_joystickDebug.integer ) {
		char str[32];

		trap_JoyEventToString( joyevent, str, sizeof( str ) );
		CG_Printf( "Player %d pressed %s\n", localPlayerNum + 1, str );
	}
}

/*
=================
CG_JoystickAxisEvent

Joystick values stay set until changed
=================
*/
void CG_JoystickAxisEvent( int localPlayerNum, int axis, int value, unsigned time, connstate_t state ) {
	joyevent_t negEvent, posEvent;
	int negKey, posKey;
	int oldvalue;

	if ( localPlayerNum < 0 || localPlayerNum >= MAX_SPLITVIEW ) {
		return;
	}
	if ( axis < 0 || axis >= MAX_JOYSTICK_AXIS ) {
		CG_Error( "CG_JoystickEvent: bad axis %i", axis );
	}

	negEvent.type = JOYEVENT_AXIS;
	negEvent.value.axis.num = axis;
	negEvent.value.axis.sign = -1;
	negKey = trap_GetKeyForJoyEvent( localPlayerNum, &negEvent );

	posEvent.type = JOYEVENT_AXIS;
	posEvent.value.axis.num = axis;
	posEvent.value.axis.sign = 1;
	posKey = trap_GetKeyForJoyEvent( localPlayerNum, &posEvent );

	oldvalue = cg.localPlayers[localPlayerNum].joystickAxis[axis];
	cg.localPlayers[localPlayerNum].joystickAxis[axis] = value;

	// stick released or switched pos/neg
	if ( value == 0 || !!(value < 0) != !!(oldvalue < 0) ) {
		if ( oldvalue < 0 ) {
			if ( negKey != -1 ) {
				CG_DistributeKeyEvent( negKey, qfalse, time, state, localPlayerNum, -(axis + 1) );
			}
		} else if ( oldvalue > 0 ) {
			if ( posKey != -1 ) {
				CG_DistributeKeyEvent( posKey, qfalse, time, state, localPlayerNum, axis + 1 );
			}
		}
	}

	// move in new pos or neg direction
	if ( value < 0 && oldvalue >= 0 ) {
		CG_JoystickEvent( localPlayerNum, &negEvent );
		if ( negKey != -1 ) {
			CG_DistributeKeyEvent( negKey, qtrue, time, state, localPlayerNum, -(axis + 1) );
		}
	} else if ( value > 0 && oldvalue <= 0 ) {
		CG_JoystickEvent( localPlayerNum, &posEvent );
		if ( posKey != -1 ) {
			CG_DistributeKeyEvent( posKey, qtrue, time, state, localPlayerNum, axis + 1 );
		}
	}
}

/*
=================
CG_JoystickButtonEvent
=================
*/
void CG_JoystickButtonEvent( int localPlayerNum, int button, qboolean down, unsigned time, connstate_t state ) {
	joyevent_t joyevent;
	int key;

	if ( localPlayerNum < 0 || localPlayerNum >= MAX_SPLITVIEW ) {
		return;
	}
	if ( button < 0 || button >= MAX_JOYSTICK_BUTTONS ) {
		CG_Error( "CG_JoystickButtonEvent: bad button %i", button );
	}

	joyevent.type = JOYEVENT_BUTTON;
	joyevent.value.button = button;
	key = trap_GetKeyForJoyEvent( localPlayerNum, &joyevent );

	if ( down ) {
		CG_JoystickEvent( localPlayerNum, &joyevent );
	}

	if ( key != -1 ) {
		CG_DistributeKeyEvent( key, down, time, state, localPlayerNum, 0 );
	}
}

/*
=================
CG_JoystickHatEvent
=================
*/
void CG_JoystickHatEvent( int localPlayerNum, int hat, int value, unsigned time, connstate_t state ) {
	joyevent_t hatEvent[4];
	int hatKeys[4];
	int oldvalue;
	int i;

	if ( localPlayerNum < 0 || localPlayerNum >= MAX_SPLITVIEW ) {
		return;
	}
	if ( hat < 0 || hat >= MAX_JOYSTICK_HATS ) {
		CG_Error( "CG_JoystickHatEvent: bad hat %i", hat );
	}

	hatEvent[0].type = JOYEVENT_HAT;
	hatEvent[0].value.hat.num = hat;
	hatEvent[0].value.hat.mask = HAT_UP;
	hatKeys[0] = trap_GetKeyForJoyEvent( localPlayerNum, &hatEvent[0] );

	hatEvent[1].type = JOYEVENT_HAT;
	hatEvent[1].value.hat.num = hat;
	hatEvent[1].value.hat.mask = HAT_RIGHT;
	hatKeys[1] = trap_GetKeyForJoyEvent( localPlayerNum, &hatEvent[1] );

	hatEvent[2].type = JOYEVENT_HAT;
	hatEvent[2].value.hat.num = hat;
	hatEvent[2].value.hat.mask = HAT_DOWN;
	hatKeys[2] = trap_GetKeyForJoyEvent( localPlayerNum, &hatEvent[2] );

	hatEvent[3].type = JOYEVENT_HAT;
	hatEvent[3].value.hat.num = hat;
	hatEvent[3].value.hat.mask = HAT_LEFT;
	hatKeys[3] = trap_GetKeyForJoyEvent( localPlayerNum, &hatEvent[3] );

	oldvalue = cg.localPlayers[localPlayerNum].joystickHats[hat];
	cg.localPlayers[localPlayerNum].joystickHats[hat] = value;

	// released
	for ( i = 0; i < 4; i++ ) {
		if ( (oldvalue & (1 << i)) && !(value & (1 << i)) ) {
			if ( hatKeys[i] != -1 ) {
				CG_DistributeKeyEvent( hatKeys[i], qfalse, time, state, localPlayerNum, 0 );
			}
		}
	}

	switch ( value ) {
	case HAT_RIGHTUP:
	case HAT_RIGHTDOWN:
	case HAT_LEFTUP:
	case HAT_LEFTDOWN:
		if ( UI_WantsBindKeys() ) {
			return;
		}
	default:
		break;
	}

	// pressed
	for ( i = 0; i < 4; i++ ) {
		if ( !(oldvalue & (1 << i)) && (value & (1 << i)) ) {
			CG_JoystickEvent( localPlayerNum, &hatEvent[i] );
			if ( hatKeys[i] != -1 ) {
				CG_DistributeKeyEvent( hatKeys[i], qtrue, time, state, localPlayerNum, 0 );
			}
		}
	}
}

/*
================
CG_VoIPString
================
*/
static char* CG_VoIPString( int localPlayerNum ) {
	// a generous overestimate of the space needed for 0,1,2...61,62,63
	static char voipString[MAX_CLIENTS * 4];
	char voipSendTarget[MAX_CVAR_VALUE_STRING];

	if ( localPlayerNum < 0 || localPlayerNum > CG_MaxSplitView() || cg.localPlayers[localPlayerNum].playerNum == -1 ) {
		return NULL;
	}

	trap_Argv( 0, voipSendTarget, sizeof( voipSendTarget ) );

	if ( Q_stricmpn( voipSendTarget, "team", 4 ) == 0 ) {
		int i, slen, nlen;
		for ( slen = i = 0; i < cgs.maxplayers; i++ ) {
			if ( !cgs.playerinfo[i].infoValid || i == cg.localPlayers[localPlayerNum].playerNum )
				continue;
			if ( cgs.playerinfo[i].team != cgs.playerinfo[cg.localPlayers[localPlayerNum].playerNum].team )
				continue;

			nlen = Com_sprintf( &voipString[slen], sizeof( voipString ) - slen,
				"%s%d", (slen > 0) ? "," : "", i );
			if ( slen + nlen + 1 >= sizeof( voipString ) ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: voipString overflowed\n" );
				break;
			}

			slen += nlen;
		}

		// Notice that if the Com_sprintf was truncated, slen was not updated
		// so this will remove any trailing commas or partially-completed numbers
		voipString[slen] = '\0';
	} else if ( Q_stricmpn( voipSendTarget, "crosshair", 9 ) == 0 )
		Com_sprintf( voipString, sizeof( voipString ), "%d",
			CG_CrosshairPlayer( localPlayerNum ) );
	else if ( Q_stricmpn( voipSendTarget, "attacker", 8 ) == 0 )
		Com_sprintf( voipString, sizeof( voipString ), "%d",
			CG_LastAttacker( localPlayerNum ) );
	else
		return NULL;

	return voipString;
}

/*
================
CG_UpdateGlconfig
================
*/
static void CG_UpdateGlconfig( qboolean initial ) {
	int oldWidth, oldHeight;
	qboolean resized;

	oldWidth = cgs.glconfig.vidWidth;
	oldHeight = cgs.glconfig.vidHeight;

	trap_GetGlconfig( &cgs.glconfig );

	resized = !initial && (oldWidth != cgs.glconfig.vidWidth
		|| oldHeight != cgs.glconfig.vidHeight);

	if ( initial || (resized && cg.connState != CA_ACTIVE) ) {
		// Viewport scale and offset
		cg.viewport = 0;
		cg.numViewports = 1;
		CG_CalcVrect();
	}

	if ( resized ) {
		CG_ConsoleResized();
		UI_WindowResized();
	}
}

