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
// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"

//==========================================================================

/*
===================
CG_PlaceString

Also called by scoreboard drawing
===================
*/
const char	*CG_PlaceString( int rank ) {
	static char	str[64];
	char	*s, *t;

	if ( rank & RANK_TIED_FLAG ) {
		rank &= ~RANK_TIED_FLAG;
		t = "Tied for ";
	} else {
		t = "";
	}

	if ( rank == 1 ) {
		s = S_COL24_DODGER_BLUE "1st" S_COLOR_WHITE;	// draw in blue
	} else if ( rank == 2 ) {
		s = S_COLOR_RED "2nd" S_COLOR_WHITE;			// draw in red
	} else if ( rank == 3 ) {
		s = S_COL24_AMBER "3rd" S_COLOR_WHITE;			// draw in yellow
	} else if ( rank == 11 ) {
		s = S_COLOR_WHITE "11th";
	} else if ( rank == 12 ) {
		s = S_COLOR_WHITE "12th";
	} else if ( rank == 13 ) {
		s = S_COLOR_WHITE "13th";
	} else if ( rank % 10 == 1 ) {
		s = va( S_COLOR_WHITE "%ist", rank);
	} else if ( rank % 10 == 2 ) {
		s = va( S_COLOR_WHITE "%ind", rank);
	} else if ( rank % 10 == 3 ) {
		s = va( S_COLOR_WHITE "%ird", rank);
	} else {
		s = va( S_COLOR_WHITE "%ith", rank);
	}

	Com_sprintf( str, sizeof( str ), "%s%s", t, s );
	return str;
}


/*
=============
CG_GraphObits_Add
=============
*/
static void CG_GraphObits_Add( int mod, int target, int attacker ) {
	char* s;

	if ( mod == MOD_SUICIDE_TEAM_CHANGE ) return;

	cg.obitNum = (cg.obitNum + 1) % MAX_GRAPHICAL_OBITS;

	cg.obitMOD[cg.obitNum] = mod;
	cg.obitTime[cg.obitNum] = cg.time;

	if ( attacker == target || attacker == ENTITYNUM_WORLD ) {
		Com_sprintf( cg.obitAttackerName[cg.obitNum], sizeof( cg.obitAttackerName[cg.obitNum] ), "" );
	} else {
		s = va( "%s", CG_ConfigString( CS_PLAYERS + attacker ) );
		if ( s ) {
			Com_sprintf( cg.obitAttackerName[cg.obitNum], sizeof( cg.obitAttackerName[cg.obitNum] ),
				"%s", Info_ValueForKey( s, "n" ) );
		}
	}
	s = va( "%s", CG_ConfigString( CS_PLAYERS + target ) );
	if ( s ) {
		Com_sprintf( cg.obitTargetName[cg.obitNum], sizeof( cg.obitTargetName[cg.obitNum] ),
			"%s", Info_ValueForKey( s, "n" ) );
	}

	cg.obitAttacker[cg.obitNum] = attacker;
	cg.obitTarget[cg.obitNum] = target;

	switch ( mod ) {
		case MOD_GAUNTLET:
			cg.obitIcon[cg.obitNum] = cg_weapons[WP_GAUNTLET].weaponIcon;
			break;
		case MOD_MACHINEGUN:
			cg.obitIcon[cg.obitNum] = cg_weapons[WP_MACHINEGUN].weaponIcon;
			break;
		case MOD_SHOTGUN:
			cg.obitIcon[cg.obitNum] = cg_weapons[WP_SHOTGUN].weaponIcon;
			break;
		case MOD_GRENADE:
		case MOD_GRENADE_SPLASH:
			cg.obitIcon[cg.obitNum] = cg_weapons[WP_GRENADE_LAUNCHER].weaponIcon;
			break;
		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
			cg.obitIcon[cg.obitNum] = cg_weapons[WP_ROCKET_LAUNCHER].weaponIcon;
			break;
		case MOD_LIGHTNING:
			cg.obitIcon[cg.obitNum] = cg_weapons[WP_LIGHTNING].weaponIcon;
			break;
		case MOD_RAILGUN:
			cg.obitIcon[cg.obitNum] = cg_weapons[WP_RAILGUN].weaponIcon;
			break;
		case MOD_PLASMA:
		case MOD_PLASMA_SPLASH:
			cg.obitIcon[cg.obitNum] = cg_weapons[WP_PLASMAGUN].weaponIcon;
			break;
		case MOD_BFG:
		case MOD_BFG_SPLASH:
			cg.obitIcon[cg.obitNum] = cg_weapons[WP_BFG].weaponIcon;
			break;
		case MOD_GRAPPLE:
			cg.obitIcon[cg.obitNum] = cg_weapons[WP_GRAPPLING_HOOK].weaponIcon;
			break;
#ifdef MISSIONPACK
		case MOD_NAIL:
			cg.obitIcon[cg.obitNum] = cg_weapons[WP_NAILGUN].weaponIcon;
			break;
		case MOD_PROXIMITY_MINE:
			cg.obitIcon[cg.obitNum] = cg_weapons[WP_PROX_LAUNCHER].weaponIcon;
			break;
		case MOD_CHAINGUN:
			cg.obitIcon[cg.obitNum] = cg_weapons[WP_CHAINGUN].weaponIcon;
			break;
#endif
#ifdef MISSIONPACK
		case MOD_KAMIKAZE: break;
		case MOD_JUICED: break;
#endif
		case MOD_WATER:
		case MOD_SLIME:
		case MOD_LAVA:
		case MOD_CRUSH:
		case MOD_TELEFRAG:
		case MOD_FALLING:
		case MOD_SUICIDE:
		case MOD_TARGET_LASER:
		case MOD_TRIGGER_HURT:
		case MOD_BLASTER:
		case MOD_TREASON:
			cg.obitIcon[cg.obitNum] = cgs.media.deathIcon;
			break;
		default:
			cg.obitIcon[cg.obitNum] = cgs.media.deathIcon;
			break;
	}
	//CG_Printf( "GraphObits_Add: num = %i, target = %i, attacker = %i, targetName = %s, attackerName = %s\n",
	//	cg.obitNum, cg.obitTarget[cg.obitNum], cg.obitAttacker[cg.obitNum], cg.obitTargetName[cg.obitNum], cg.obitAttackerName[cg.obitNum] );
}

/*
=============
CG_Obituary
=============
*/
static void CG_Obituary( entityState_t *ent ) {
	int			mod;
	int			target, attacker;
	char		*message;
	char		*message2;
	const char	*targetInfo;
	const char	*attackerInfo;
	char		targetName[MAX_NAME_LENGTH];
	char		attackerName[MAX_NAME_LENGTH];
	gender_t	gender;
	playerInfo_t	*pi;
	int				i;

	target = ent->otherEntityNum;
	attacker = ent->otherEntityNum2;
	mod = ent->eventParm;

	if ( target < 0 || target >= MAX_CLIENTS ) {
		CG_Error( "CG_Obituary: target out of range" );
	}
	pi = &cgs.playerinfo[target];

	if ( attacker < 0 || attacker >= MAX_CLIENTS ) {
		attacker = ENTITYNUM_WORLD;
		attackerInfo = NULL;
	} else {
		attackerInfo = CG_ConfigString( CS_PLAYERS + attacker );
	}

	targetInfo = CG_ConfigString( CS_PLAYERS + target );
	if ( !targetInfo ) {
		return;
	}
	//Q_strncpyz( targetName, Info_ValueForKey( targetInfo, "n" ), sizeof(targetName) - 2);
	//strcat( va( S_COLOR_WHITE "%s", targetName ), S_COLOR_CREAM );
	Com_sprintf( targetName, sizeof( targetName ), S_COLOR_WHITE "%s" S_COLOR_CREAM, Info_ValueForKey( targetInfo, "n" ) );
	message2 = "";

	CG_GraphObits_Add( mod, target, attacker );

	// check for single player messages

	switch( mod ) {
	case MOD_WATER:
		message = "sank like a rock";
		break;
	case MOD_SLIME:
		message = "melted";
		break;
	case MOD_LAVA:
		message = "does a back flip into the lava";
		break;
	case MOD_CRUSH:
		message = "was squished";
		break;
	case MOD_FALLING:
		message = "cratered";
		break;
	case MOD_SUICIDE:
		message = "suicides";
		break;
	case MOD_TARGET_LASER:
		message = "saw the light";
		break;
	case MOD_TRIGGER_HURT:
		message = "was in the wrong place";
		break;
	case MOD_BLASTER:
		message = "got blasted";
		break;
	default:
		message = NULL;
		break;
	}

	if (attacker == target) {
		gender = pi->gender;
		switch (mod) {
#ifdef MISSIONPACK
		case MOD_KAMIKAZE:
			message = "goes out with a bang";
			break;
#endif
		case MOD_GRENADE:
			if ( gender == GENDER_FEMALE )
				message = "caught her own grenade";
			else if ( gender == GENDER_NEUTER )
				message = "caught its own grenade";
			else
				message = "caught his own grenade";
			break;
		case MOD_GRENADE_SPLASH:
			if ( gender == GENDER_FEMALE )
				message = "tripped on her own grenade";
			else if ( gender == GENDER_NEUTER )
				message = "tripped on its own grenade";
			else
				message = "tripped on his own grenade";
			break;
		case MOD_ROCKET_SPLASH:
			if ( gender == GENDER_FEMALE )
				message = "blew herself up";
			else if ( gender == GENDER_NEUTER )
				message = "blew itself up";
			else
				message = "blew himself up";
			break;
		case MOD_PLASMA_SPLASH:
			if ( gender == GENDER_FEMALE )
				message = "melted herself";
			else if ( gender == GENDER_NEUTER )
				message = "melted itself";
			else
				message = "melted himself";
			break;
		case MOD_BFG_SPLASH:
			message = "should have used a smaller gun";
			break;
#ifdef MISSIONPACK
		case MOD_PROXIMITY_MINE:
			if( gender == GENDER_FEMALE ) {
				message = "found her prox mine";
			} else if ( gender == GENDER_NEUTER ) {
				message = "found its prox mine";
			} else {
				message = "found his prox mine";
			}
			break;
#endif
		case MOD_TREASON:
			message = "died from shame";
			break;
		default:
			if ( gender == GENDER_FEMALE )
				message = "killed herself";
			else if ( gender == GENDER_NEUTER )
				message = "killed itself";
			else
				message = "killed himself";
			break;
		}
	}

	if (message) {
		CG_Printf( "%s %s.\n", targetName, message);
		return;
	}

	// check for kill messages from the current playerNum
	if ( CG_LocalPlayerState(attacker) ) {
		char	*s;
		playerState_t	*ps;

		for (i = 0; i < CG_MaxSplitView(); i++) {
			if ( attacker != cg.snap->pss[i].playerNum ) {
				continue;
			}

			ps = &cg.snap->pss[i];

			if (GTF(GTF_TEAMS) || cg.warmupTime) {
				s = va("You fragged %s" S_COLOR_WHITE, targetName );
			} else {
				s = va("You fragged %s" S_COLOR_WHITE "\n%s place with %i", targetName, 
					CG_PlaceString( ps->persistant[PERS_RANK] + 1 ),
					ps->persistant[PERS_SCORE] );
			}
#ifdef MISSIONPACK
			if (!(cg_singlePlayer.integer && cg.localPlayers[i].cameraOrbit)) {
				CG_CenterPrint( i, s, SCREEN_HEIGHT * 0.30, 0.5, qfalse );
			} 
#else
			CG_CenterPrint( i, s, SCREEN_HEIGHT * 0.30, 0.5, qfalse );
#endif
		}

		// print the text message as well
	}

	// check for double player messages
	if ( !attackerInfo ) {
		attacker = ENTITYNUM_WORLD;
		strcpy( attackerName, "noname" );
	} else {
		//Q_strncpyz( attackerName, Info_ValueForKey( attackerInfo, "n" ), sizeof(attackerName) - 2);
		//strcat( va( S_COLOR_WHITE "%s", attackerName), S_COLOR_CREAM );
		Com_sprintf( attackerName, sizeof( attackerName ), S_COLOR_WHITE "%s" S_COLOR_CREAM, Info_ValueForKey( attackerInfo, "n" ) );
		// check for kill messages about the current playerNum
		for (i = 0; i < CG_MaxSplitView(); i++) {
			if ( target == cg.snap->pss[i].playerNum ) {
				Q_strncpyz( cg.localPlayers[i].killerName, attackerName, sizeof( cg.localPlayers[i].killerName ) );
			}
		}
	}

	if ( attacker != ENTITYNUM_WORLD ) {
		switch (mod) {
		case MOD_GRAPPLE:
			message = "was caught by";
			break;
		case MOD_GAUNTLET:
			message = "was pummeled by";
			break;
		case MOD_MACHINEGUN:
			message = "was machinegunned by";
			break;
		case MOD_SHOTGUN:
			message = "was gunned down by";
			break;
		case MOD_GRENADE:
			message = "ate";
			message2 = "'s grenade";
			break;
		case MOD_GRENADE_SPLASH:
			message = "was shredded by";
			message2 = "'s shrapnel";
			break;
		case MOD_ROCKET:
			message = "ate";
			message2 = "'s rocket";
			break;
		case MOD_ROCKET_SPLASH:
			message = "almost dodged";
			message2 = "'s rocket";
			break;
		case MOD_PLASMA:
			message = "was melted by";
			message2 = "'s plasmagun";
			break;
		case MOD_PLASMA_SPLASH:
			message = "was melted by";
			message2 = "'s plasmagun";
			break;
		case MOD_RAILGUN:
			message = "was railed by";
			break;
		case MOD_LIGHTNING:
			message = "was electrocuted by";
			break;
		case MOD_BFG:
		case MOD_BFG_SPLASH:
			message = "was blasted by";
			message2 = "'s BFG";
			break;
#ifdef MISSIONPACK
		case MOD_NAIL:
			message = "was nailed by";
			break;
		case MOD_CHAINGUN:
			message = "got lead poisoning from";
			message2 = "'s Chaingun";
			break;
		case MOD_PROXIMITY_MINE:
			message = "was too close to";
			message2 = "'s Prox Mine";
			break;
		case MOD_KAMIKAZE:
			message = "falls to";
			message2 = "'s Kamikaze blast";
			break;
		case MOD_JUICED:
			message = "was juiced by";
			break;
#endif
		case MOD_TELEFRAG:
			message = "tried to invade";
			message2 = "'s personal space";
			break;
		default:
			message = "was killed by";
			break;
		}
		
		if (message) {
			CG_Printf( "%s %s " S_COLOR_WHITE "%s%s\n", 
				targetName, message, attackerName, message2);
			return;
		}
	}

	// we don't know what it was
	CG_Printf( "%s died.\n", targetName );
}

//==========================================================================

/*
===============
CG_UseItem
===============
*/
static void CG_UseItem( centity_t *cent ) {
	playerInfo_t *pi;
	int			itemNum, playerNum;
	gitem_t		*item;
	entityState_t *es;
	//int			i;

	es = &cent->currentState;
	
	itemNum = (es->event & ~EV_EVENT_BITS) - EV_USE_ITEM0;
	if ( itemNum < 0 || itemNum > HI_NUM_HOLDABLE ) {
		itemNum = 0;
	}
	
	// print a message if the local player
	if ( es->number == cg.cur_ps->playerNum ) {
		if ( !itemNum ) {
			CG_CenterPrint( 0, "No item to use", SCREEN_HEIGHT * 0.30, 0.5, qfalse );
		} else if ( (itemNum == HI_PSCREEN || itemNum == HI_PSHIELD) ) {
			// only use use item events for PA when out of ammo
			CG_CenterPrint( 0, "No Cells for Power Armor", SCREEN_HEIGHT * 0.30, 0.5, qfalse );
		} else {
			item = BG_FindItemForHoldable( itemNum );
			CG_CenterPrint( 0, va( "Use %s", item->pickup_name ), SCREEN_HEIGHT * 0.30, 0.5, qfalse );
		}
	}

	switch ( itemNum ) {
	default:
	case HI_NONE:
		trap_S_StartSound (NULL, es->number, CHAN_BODY, cgs.media.useNothingSound );
		break;

	case HI_TELEPORTER:
		break;

	case HI_MEDKIT:
		playerNum = cent->currentState.playerNum;
		if ( playerNum >= 0 && playerNum < MAX_CLIENTS ) {
			pi = &cgs.playerinfo[playerNum];
			pi->medkitUsageTime = cg.time;
			trap_S_StartSound (NULL, es->number, CHAN_BODY, cgs.media.medkitSound );
		}
		break;

#ifdef MISSIONPACK
	case HI_KAMIKAZE:
		break;

	case HI_PORTAL:
		break;
	case HI_INVULNERABILITY:
		trap_S_StartSound (NULL, es->number, CHAN_BODY, cgs.media.useInvulnerabilitySound );
		break;
#endif
	}

}

/*
================
CG_ItemPickup

A new item was picked up this frame
================
*/
static void CG_ItemPickup( int localPlayerNum, int itemNum ) {
	localPlayer_t *player = &cg.localPlayers[localPlayerNum];
	gitem_t *item = BG_ItemForItemNum( itemNum );

	player->itemPickup = itemNum;
	player->itemPickupTime = cg.time;
	player->itemPickupBlendTime = cg.time;
	// see if it should be the grabbed weapon
	if ( item->giType == IT_WEAPON ) {
		// select it immediately
		if ( cg_autoSwitch.integer && item->giTag != WP_MACHINEGUN ) {
			player->weaponSelectTime = cg.time;
			player->weaponSelect = bg_itemlist[itemNum].giTag;
		}
	}

}

/*
================
CG_WaterLevel

Returns waterlevel for entity origin
================
*/
int CG_WaterLevel(centity_t *cent) {
	vec3_t point;
	int contents, sample1, sample2, anim, waterlevel;
	int viewheight;

	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;

	if (anim == LEGS_WALKCR || anim == LEGS_IDLECR) {
		viewheight = CROUCH_VIEWHEIGHT;
	} else {
		viewheight = DEFAULT_VIEWHEIGHT;
	}

	//
	// get waterlevel, accounting for ducking
	//
	waterlevel = 0;

	point[0] = cent->lerpOrigin[0];
	point[1] = cent->lerpOrigin[1];
	point[2] = cent->lerpOrigin[2] + MINS_Z + 1;
	contents = CG_PointContents(point, -1);

	if (contents & MASK_WATER) {
		sample2 = viewheight - MINS_Z;
		sample1 = sample2 / 2;
		waterlevel = 1;
		point[2] = cent->lerpOrigin[2] + MINS_Z + sample1;
		contents = CG_PointContents(point, -1);

		if (contents & MASK_WATER) {
			waterlevel = 2;
			point[2] = cent->lerpOrigin[2] + MINS_Z + sample2;
			contents = CG_PointContents(point, -1);

			if (contents & MASK_WATER) {
				waterlevel = 3;
			}
		}
	}

	return waterlevel;
}

/*
================
CG_PainEvent

Also called by playerstate transition
================
*/
void CG_PainEvent( centity_t *cent, int health ) {
	char	*snd;

	// don't do more than two pain sounds a second
	if ( cg.time - cent->pe.painTime < 500 ) {
		return;
	}

	if ( health < 25 ) {
		snd = "*pain25_1.wav";
	} else if ( health < 50 ) {
		snd = "*pain50_1.wav";
	} else if ( health < 75 ) {
		snd = "*pain75_1.wav";
	} else {
		snd = "*pain100_1.wav";
	}
	// play a gurp sound instead of a normal pain sound
	if (CG_WaterLevel(cent) == 3) {
		if (rand()&1) {
			trap_S_StartSound(NULL, cent->currentState.number, CHAN_VOICE, CG_CustomSound(cent->currentState.number, "sound/player/gurp1.wav"));
		} else {
			trap_S_StartSound(NULL, cent->currentState.number, CHAN_VOICE, CG_CustomSound(cent->currentState.number, "sound/player/gurp2.wav"));
		}
	} else {
		trap_S_StartSound(NULL, cent->currentState.number, CHAN_VOICE, CG_CustomSound(cent->currentState.number, snd));
	}
	// save pain time for programitic twitch animation
	cent->pe.painTime = cg.time;
	cent->pe.painDirection ^= 1;
}



/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/
#define	DEBUGNAME(x) if(cg_debugEvents.integer){CG_Printf(x"\n");}
#define	DEBUGNAME2(x, y) if(cg_debugEvents.integer){CG_Printf(x"\n",(y));}
void CG_EntityEvent( centity_t *cent, vec3_t position ) {
	entityState_t	*es;
	int				event;
	vec3_t			dir;
	const char		*s;
	int				playerNum;
	playerInfo_t	*pi;
	int				i;

	es = &cent->currentState;
	event = es->event & ~EV_EVENT_BITS;

	if ( cg_debugEvents.integer ) {
		CG_Printf( "ent:%3i  event:%3i ", es->number, event );
	}

	if ( !event ) {
		DEBUGNAME("ZEROEVENT");
		return;
	}

	playerNum = es->playerNum;
	if ( playerNum < 0 || playerNum >= MAX_CLIENTS ) {
		playerNum = 0;
	}
	pi = &cgs.playerinfo[ playerNum ];

	switch ( event ) {
	//
	// movement generated events
	//
	case EV_FOOTSTEP:
		DEBUGNAME("EV_FOOTSTEP");
		if (cg_footSteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ pi->footsteps ][rand()&3] );
		}
		break;
	case EV_FOOTSTEP_METAL:
		DEBUGNAME("EV_FOOTSTEP_METAL");
		if (cg_footSteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ FOOTSTEP_METAL ][rand()&3] );
		}
		break;
	case EV_FOOTSTEP_SNOW:
		DEBUGNAME("EV_FOOTSTEP_SNOW");
		if (cg_footSteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ FOOTSTEP_SNOW ][rand()&3] );
		}
		break;
	case EV_FOOTSTEP_WOOD:
		DEBUGNAME("EV_FOOTSTEP_WOOD");
		if (cg_footSteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ FOOTSTEP_WOOD ][rand()&3] );
		}
		break;
	case EV_FOOTSPLASH:
		DEBUGNAME("EV_FOOTSPLASH");
		if (cg_footSteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		}
		break;
	case EV_FOOTWADE:
		DEBUGNAME("EV_FOOTWADE");
		if (cg_footSteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		}
		break;
	case EV_SWIM:
		DEBUGNAME("EV_SWIM");
		if (cg_footSteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		}
		break;


	case EV_FALL_SHORT:
		DEBUGNAME("EV_FALL_SHORT");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.landSound );
		for (i = 0; i < CG_MaxSplitView(); i++) {
			if ( playerNum == cg.snap->pss[i].playerNum ) {
				// smooth landing z changes
				cg.localPlayers[i].landChange = -8;
				cg.localPlayers[i].landTime = cg.time;
			}
		}
		break;
	case EV_FALL_MEDIUM:
		DEBUGNAME("EV_FALL_MEDIUM");
		// use normal pain sound
		trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*pain100_1.wav" ) );
		for (i = 0; i < CG_MaxSplitView(); i++) {
			if ( playerNum == cg.snap->pss[i].playerNum ) {
				// smooth landing z changes
				cg.localPlayers[i].landChange = -16;
				cg.localPlayers[i].landTime = cg.time;
			}
		}
		break;
	case EV_FALL_FAR:
		DEBUGNAME("EV_FALL_FAR");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*fall1.wav" ) );
		cent->pe.painTime = cg.time;	// don't play a pain sound right after this
		for (i = 0; i < CG_MaxSplitView(); i++) {
			if ( playerNum == cg.snap->pss[i].playerNum ) {
				// smooth landing z changes
				cg.localPlayers[i].landChange = -24;
				cg.localPlayers[i].landTime = cg.time;
			}
		}
		break;

	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:		// smooth out step up transitions
		DEBUGNAME("EV_STEP");
	{
		float	oldStep;
		int		delta;
		int		step;
		localPlayer_t *player;
		playerState_t *ps;

		for (i = 0; i < CG_MaxSplitView(); i++) {
			player = &cg.localPlayers[i];
			ps = &cg.snap->pss[i];

			if ( playerNum != ps->playerNum ) {
				continue;
			}

			// if we are interpolating, we don't need to smooth steps
			if ( cg.demoPlayback || (ps->pm_flags & PMF_FOLLOW) ||
				cg_noPredict.integer || cg_synchronousClients.integer ) {
				continue;
			}
			// check for stepping up before a previous step is completed
			delta = cg.time - player->stepTime;
			if (delta < STEP_TIME) {
				oldStep = player->stepChange * (STEP_TIME - delta) / STEP_TIME;
			} else {
				oldStep = 0;
			}

			// add this amount
			step = 4 * (event - EV_STEP_4 + 1 );
			player->stepChange = oldStep + step;
			if ( player->stepChange > MAX_STEP_CHANGE ) {
				player->stepChange = MAX_STEP_CHANGE;
			}
			player->stepTime = cg.time;
		}
		break;
	}

	case EV_JUMP_PAD:
		DEBUGNAME("EV_JUMP_PAD");
//		CG_Printf( "EV_JUMP_PAD w/effect #%i\n", es->eventParm );
		{
			vec3_t			up = {0, 0, 1};


			CG_SmokePuff( cent->lerpOrigin, up, 
						  32, 
						  1, 1, 1, 0.33f,
						  1000, 
						  cg.time, 0,
						  LEF_PUFF_DONT_SCALE, 
						  cgs.media.smokePuffShader );
		}

		// boing sound at origin, jump sound on player
		trap_S_StartSound ( cent->lerpOrigin, -1, CHAN_VOICE, cgs.media.jumpPadSound );
		trap_S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*jump1.wav" ) );
		break;

	case EV_JUMP:
		DEBUGNAME("EV_JUMP");
		trap_S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*jump1.wav" ) );
		break;
	case EV_TAUNT:
		DEBUGNAME("EV_TAUNT");
		trap_S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*taunt.wav" ) );
		break;
	case EV_WATER_TOUCH:
		DEBUGNAME("EV_WATER_TOUCH");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrInSound );
		break;
	case EV_WATER_LEAVE:
		DEBUGNAME("EV_WATER_LEAVE");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound );
		break;
	case EV_WATER_UNDER:
		DEBUGNAME("EV_WATER_UNDER");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrUnSound );
		break;
	case EV_WATER_CLEAR:
		DEBUGNAME("EV_WATER_CLEAR");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*gasp.wav" ) );
		break;

	case EV_ITEM_PICKUP:
		DEBUGNAME("EV_ITEM_PICKUP");
		{
			gitem_t	*item;
			int		index;

			index = es->eventParm;		// player predicted

			if ( index < 1 || index >= BG_NumItems() ) {
				break;
			}
			item = BG_ItemForItemNum( index );

			// powerups and team items will have a separate global sound, this one
			// will be played at prediction time
			if ( item->giType == IT_POWERUP || item->giType == IT_TEAM) {
				trap_S_StartSound (NULL, es->number, CHAN_AUTO,	cgs.media.n_healthSound );
			} else if (item->giType == IT_RUNE) {
				switch (item->giTag ) {
					case PW_SCOUT:
						trap_S_StartSound (NULL, es->number, CHAN_AUTO,	cgs.media.scoutSound );
					break;
					case PW_RESISTANCE:
						trap_S_StartSound (NULL, es->number, CHAN_AUTO,	cgs.media.resistanceSound );
					break;
					case PW_STRENGTH:
						trap_S_StartSound (NULL, es->number, CHAN_AUTO,	cgs.media.strengthSound );
					break;
					case PW_ARMAMENT:
						trap_S_StartSound (NULL, es->number, CHAN_AUTO,	cgs.media.armamentSound );
					break;
					case PW_TENACITY:
						trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.tenacitySound );
						break;
					case PW_PARASITE:
						trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.parasiteSound );
						break;
				}
			} else {
				trap_S_StartSound (NULL, es->number, CHAN_AUTO,	cgs.media.itemPickupSounds[ index ] );
			}

			// show icon and name on status bar
			for (i = 0; i < CG_MaxSplitView(); i++) {
				if ( es->number == cg.snap->pss[i].playerNum ) {
					CG_ItemPickup( i, index );
				}
			}
		}
		break;

	case EV_GLOBAL_ITEM_PICKUP:
		DEBUGNAME("EV_GLOBAL_ITEM_PICKUP");
		{
			int		index;

			index = es->eventParm;		// player predicted

			if ( index < 1 || index >= BG_NumItems() ) {
				break;
			}
			// powerup pickups are global
			trap_S_StartLocalSound( cgs.media.itemPickupSounds[ index ], CHAN_AUTO );

			// show icon and name on status bar
			for (i = 0; i < CG_MaxSplitView(); i++) {
				if ( es->number == cg.snap->pss[i].playerNum ) {
					CG_ItemPickup( i, index );
				}
			}
		}
		break;

	//
	// weapon events
	//
	case EV_NOAMMO:
		DEBUGNAME("EV_NOAMMO");
//		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.noAmmoSound );
		for (i = 0; i < CG_MaxSplitView(); i++) {
			if ( es->number == cg.snap->pss[i].playerNum ) {
				CG_OutOfAmmoChange(i);
			}
		}
		break;
	case EV_CHANGE_WEAPON:
		DEBUGNAME("EV_CHANGE_WEAPON");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.selectSound );
		break;
	case EV_FIRE_WEAPON:
		DEBUGNAME("EV_FIRE_WEAPON");
		CG_FireWeapon( cent );
		break;

	case EV_USE_ITEM0:
	case EV_USE_ITEM1:
	case EV_USE_ITEM2:
	case EV_USE_ITEM3:
	case EV_USE_ITEM4:
	case EV_USE_ITEM5:
	case EV_USE_ITEM6:
	case EV_USE_ITEM7:
	case EV_USE_ITEM8:
	case EV_USE_ITEM9:
	case EV_USE_ITEM10:
	case EV_USE_ITEM11:
	case EV_USE_ITEM12:
	case EV_USE_ITEM13:
	case EV_USE_ITEM14:
	case EV_USE_ITEM15:
		DEBUGNAME2("EV_USE_ITEM%d", event - EV_USE_ITEM0);
		CG_UseItem( cent );
		break;

	//=================================================================

	//
	// other events
	//
	case EV_PLAYER_TELEPORT_IN:
		DEBUGNAME("EV_PLAYER_TELEPORT_IN");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.teleInSound );
		CG_SpawnEffect( position);
		break;

	case EV_PLAYER_TELEPORT_OUT:
		DEBUGNAME("EV_PLAYER_TELEPORT_OUT");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.teleOutSound );
		CG_SpawnEffect(  position);
		break;

	case EV_ITEM_POP:
		DEBUGNAME("EV_ITEM_POP");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.respawnSound );
		break;
	case EV_ITEM_RESPAWN:
		DEBUGNAME("EV_ITEM_RESPAWN");
		cent->miscTime = cg.time;	// scale up from this
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.respawnSound );
		break;

	case EV_GRENADE_BOUNCE:
		DEBUGNAME("EV_GRENADE_BOUNCE");
		if ( rand() & 1 ) {
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.hgrenb1aSound );
		} else {
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.hgrenb2aSound );
		}
		break;

#ifdef MISSIONPACK
	case EV_PROXIMITY_MINE_STICK:
		DEBUGNAME("EV_PROXIMITY_MINE_STICK");
		if( es->eventParm & SURF_FLESH ) {
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.wstbimplSound );
		} else 	if( es->eventParm & SURF_METALSTEPS ) {
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.wstbimpmSound );
		} else {
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.wstbimpdSound );
		}
		break;

	case EV_PROXIMITY_MINE_TRIGGER:
		DEBUGNAME("EV_PROXIMITY_MINE_TRIGGER");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.wstbactvSound );
		break;
	case EV_KAMIKAZE:
		DEBUGNAME("EV_KAMIKAZE");
		CG_KamikazeEffect( cent->lerpOrigin );
		break;
	case EV_INVUL_IMPACT:
		DEBUGNAME("EV_INVUL_IMPACT");
		CG_InvulnerabilityImpact( cent->lerpOrigin, cent->currentState.angles );
		break;
	case EV_JUICED:
		DEBUGNAME("EV_JUICED");
		CG_InvulnerabilityJuiced( cent->lerpOrigin );
		break;
	case EV_LIGHTNINGBOLT:
		DEBUGNAME("EV_LIGHTNINGBOLT");
		CG_LightningBoltBeam(es->origin2, es->pos.trBase);
		break;
#endif
	case EV_OBELISKEXPLODE:
		DEBUGNAME( "EV_OBELISKEXPLODE" );
		CG_ObeliskExplode( cent->lerpOrigin, es->eventParm );
		break;
	case EV_OBELISKPAIN:
		DEBUGNAME( "EV_OBELISKPAIN" );
		CG_ObeliskPain( cent->lerpOrigin );
		break;
//muff
	case EV_REGISTER_ITEM:
		DEBUGNAME( "EV_REGISTER_ITEM" );
		CG_RegisterItemVisuals( es->eventParm );
		CG_RegisterItemSounds( es->eventParm );
		break;
//-muff
	case EV_SCOREPLUM:
		DEBUGNAME("EV_SCOREPLUM");
		CG_ScorePlum( cent->currentState.otherEntityNum, cent->lerpOrigin, cent->currentState.time );
		break;

	//
	// missile impacts
	//
	case EV_MISSILE_HIT:
		DEBUGNAME("EV_MISSILE_HIT");
		ByteToDir( es->eventParm, dir );
		CG_MissileHitPlayer( es->weapon, es->ownerNum, position, dir, es->otherEntityNum );
		break;

	case EV_MISSILE_MISS:
		DEBUGNAME("EV_MISSILE_MISS");
		ByteToDir( es->eventParm, dir );
		CG_MissileHitWall( es->weapon, es->ownerNum, position, dir, IMPACTSOUND_DEFAULT );
		break;

	case EV_MISSILE_MISS_METAL:
		DEBUGNAME("EV_MISSILE_MISS_METAL");
		ByteToDir( es->eventParm, dir );
		CG_MissileHitWall( es->weapon, es->ownerNum, position, dir, IMPACTSOUND_METAL );
		break;

	case EV_RAILTRAIL:
		DEBUGNAME("EV_RAILTRAIL");
		cent->currentState.weapon = WP_RAILGUN;
		//FIXME: correct rail origin accurately to muzzle in both 1st and 3rd person
		if ( es->playerNum >= 0 && es->playerNum < MAX_CLIENTS ) {
			for (i = 0; i < CG_MaxSplitView(); i++) {
				if ( es->playerNum == cg.snap->pss[i].playerNum
					&& !cg.localPlayers[i].renderingThirdPerson)
				{
					if ( cg_drawGun.integer == 2 )
						VectorMA(es->origin2, 8, cg.refdef.viewaxis[1], es->origin2);
					else if( cg_drawGun.integer == 3 )
						VectorMA(es->origin2, 4, cg.refdef.viewaxis[1], es->origin2);
					break;
				}
			}
		}

		CG_RailTrail(pi, es->origin2, es->pos.trBase);

		// if the end was on a nomark surface, don't make an explosion
		if ( es->eventParm != 255 ) {
			ByteToDir( es->eventParm, dir );
			CG_MissileHitWall( es->weapon, playerNum, position, dir, IMPACTSOUND_DEFAULT );
		}
		break;

	case EV_BULLET_HIT_WALL:
		DEBUGNAME("EV_BULLET_HIT_WALL");
		ByteToDir( es->eventParm, dir );
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD );
		break;

	case EV_BULLET_HIT_FLESH:
		DEBUGNAME("EV_BULLET_HIT_FLESH");
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm );
		break;

	case EV_SHOTGUN:
		DEBUGNAME("EV_SHOTGUN");
		CG_ShotgunFire( es );
		break;

	case EV_GENERAL_SOUND:
		DEBUGNAME("EV_GENERAL_SOUND");
		if ( cgs.gameSounds[ es->eventParm ] ) {
			trap_S_StartSound (NULL, es->number, CHAN_VOICE, cgs.gameSounds[ es->eventParm ] );
		} else {
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );
			trap_S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, s ) );
		}
		break;

	case EV_GLOBAL_SOUND:	// play as a local sound so it never diminishes
		DEBUGNAME("EV_GLOBAL_SOUND");
		if ( cgs.gameSounds[ es->eventParm ] ) {
			trap_S_StartLocalSound( cgs.gameSounds[ es->eventParm ], CHAN_AUTO );
		} else {
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );
			trap_S_StartLocalSound( CG_CustomSound( es->number, s ), CHAN_AUTO );
		}
		break;

	case EV_GLOBAL_TEAM_SOUND:	// play as a local sound so it never diminishes
		DEBUGNAME("EV_GLOBAL_TEAM_SOUND");
		{
			team_t	team = cg.cur_ps->persistant[PERS_TEAM];

			switch( es->eventParm ) {
				case GTS_RED_CAPTURE:
				case GTS_BLUE_CAPTURE:
				case GTS_GREEN_CAPTURE:
				case GTS_YELLOW_CAPTURE:
				case GTS_TEAL_CAPTURE:
				case GTS_PINK_CAPTURE:
					CG_AddBufferedSound( (team == (es->eventParm - GTS_RED_CAPTURE + FIRST_TEAM)) ? cgs.media.captureYourTeamSound : cgs.media.captureOpponentSound );
					break;
				case GTS_RED_RETURN:
				case GTS_BLUE_RETURN:
				case GTS_GREEN_RETURN:
				case GTS_YELLOW_RETURN:
				case GTS_TEAL_RETURN:
				case GTS_PINK_RETURN:
					CG_AddBufferedSound( (team == (es->eventParm - GTS_RED_RETURN + FIRST_TEAM)) ? cgs.media.returnYourTeamSound : cgs.media.returnOpponentSound );
					break;
				case GTS_RED_TAKEN:
				case GTS_BLUE_TAKEN:
				case GTS_GREEN_TAKEN:
				case GTS_YELLOW_TAKEN:
				case GTS_TEAL_TAKEN:
				case GTS_PINK_TAKEN:
					// if this player picked up the flag then a sound is played in CG_CheckLocalSounds
					if ( (team == (es->eventParm - GTS_RED_TAKEN + FIRST_TEAM)) ) {
						CG_AddBufferedSound( (team == (es->eventParm - GTS_RED_TAKEN + FIRST_TEAM)) ? cgs.media.yourTeamPickedUpFlagSound : cgs.media.enemyPickedUpFlagSound );
					} else {
						CG_AddBufferedSound( (team == (es->eventParm - GTS_RED_TAKEN + FIRST_TEAM)) ? cgs.media.enemyPickedUpFlagSound : cgs.media.yourTeamPickedUpFlagSound );
					}
					break;
				case GTS_REDOBELISK_ATTACKED:
				case GTS_BLUEOBELISK_ATTACKED:
				case GTS_GREENOBELISK_ATTACKED:
				case GTS_YELLOWOBELISK_ATTACKED:
				case GTS_PINKOBELISK_ATTACKED:
				case GTS_TEALOBELISK_ATTACKED:
					if ( (team == (es->eventParm - GTS_REDOBELISK_ATTACKED + FIRST_TEAM))  ) {
						CG_AddBufferedSound( cgs.media.yourBaseIsUnderAttackSound );
					}
					break;
				case GTS_REDTEAM_SCORED:
				case GTS_BLUETEAM_SCORED:
				case GTS_GREENTEAM_SCORED:
				case GTS_YELLOWTEAM_SCORED:
				case GTS_TEALTEAM_SCORED:
				case GTS_PINKTEAM_SCORED:
					CG_AddBufferedSound(cgs.media.teamScoredSound[es->eventParm - GTS_REDTEAM_SCORED]);
					break;
				case GTS_REDTEAM_TOOK_LEAD:
				case GTS_BLUETEAM_TOOK_LEAD:
				case GTS_GREENTEAM_TOOK_LEAD:
				case GTS_YELLOWTEAM_TOOK_LEAD:
				case GTS_TEALTEAM_TOOK_LEAD:
				case GTS_PINKTEAM_TOOK_LEAD:
					if ( GTF(GTF_TDM) && cg_teamDmLeadAnnouncements.integer )
						CG_AddBufferedSound(cgs.media.teamScoredSound[es->eventParm - GTS_REDTEAM_TOOK_LEAD] );
					break;
				case GTS_TEAMS_ARE_TIED:
					if ( GTF(GTF_TDM) && cg_teamDmLeadAnnouncements.integer )
						CG_AddBufferedSound( cgs.media.teamsTiedSound );
					break;
#ifdef MISSIONPACK
				case GTS_KAMIKAZE:
					trap_S_StartLocalSound(cgs.media.kamikazeFarSound, CHAN_ANNOUNCER);
					break;
#endif
				default:
					break;
			}
			break;
		}

	case EV_PAIN:
		// local player sounds are triggered in CG_CheckLocalSounds,
		// so ignore events on the player
		DEBUGNAME("EV_PAIN");
		if ( !CG_LocalPlayerState( es->number ) ) {
			CG_PainEvent( cent, es->eventParm );
		}
		break;

	case EV_DEATH1:
	case EV_DEATH2:
	case EV_DEATH3:
		DEBUGNAME2("EV_DEATH%d", event - EV_DEATH1 + 1);

		// check if gibbed
		// eventParm 1 = living player gibbed
		// eventParm 2 = corpse gibbed
		if ( es->eventParm >= 1 ) {
			CG_GibPlayer( cent->lerpOrigin, cent->currentState.dir, cent->currentState.dmg );
			
			if ( cg_blood.integer && cg_gibs.integer ) {
				// don't play gib sound when using the kamikaze because it interferes
				// with the kamikaze sound, downside is that the gib sound will also
				// not be played when someone is gibbed while just carrying the kamikaze
				if ( !(es->eFlags & EF_KAMIKAZE) ) {
					trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.gibSound );
				}

				// don't play death sound
				break;
			}

			// don't play death sound if already dead
			if ( es->eventParm == 2 ) {
				break;
			}
		}

		if (CG_WaterLevel(cent) == 3) {
			trap_S_StartSound(NULL, es->number, CHAN_VOICE, CG_CustomSound(es->number, "*drown.wav"));
		} else {
			trap_S_StartSound(NULL, es->number, CHAN_VOICE, CG_CustomSound(es->number, va("*death%i.wav", event - EV_DEATH1 + 1)));
		}

		break;

	case EV_OBITUARY:
		DEBUGNAME("EV_OBITUARY");
		CG_Obituary( es );
		break;

	//
	// powerup events
	//
	case EV_POWERUP_QUAD:
		DEBUGNAME("EV_POWERUP_QUAD");
		for (i = 0; i < CG_MaxSplitView(); i++) {
			if ( es->number == cg.snap->pss[i].playerNum ) {
				cg.localPlayers[i].powerupActive = PW_QUAD;
				cg.localPlayers[i].powerupTime = cg.time;
			}
		}
		trap_S_StartSound (NULL, es->number, CHAN_ITEM, cgs.media.quadSound );
		break;
	case EV_POWERUP_BATTLESUIT:
		DEBUGNAME("EV_POWERUP_BATTLESUIT");
		for (i = 0; i < CG_MaxSplitView(); i++) {
			if ( es->number == cg.snap->pss[i].playerNum ) {
				cg.localPlayers[i].powerupActive = PW_BATTLESUIT;
				cg.localPlayers[i].powerupTime = cg.time;
			}
		}
		trap_S_StartSound (NULL, es->number, CHAN_ITEM, cgs.media.protectSound );
		break;
	case EV_POWERUP_INVULN:
		DEBUGNAME( "EV_POWERUP_INVULN" );
		for ( i = 0; i < CG_MaxSplitView(); i++ ) {
			if ( es->number == cg.snap->pss[i].playerNum ) {
				cg.localPlayers[i].powerupActive = PW_INVULN;
				cg.localPlayers[i].powerupTime = cg.time;
			}
		}
		trap_S_StartSound( NULL, es->number, CHAN_ITEM, cgs.media.protectSound );
		break;
	case EV_POWERUP_REGEN:
		DEBUGNAME("EV_POWERUP_REGEN");
		for (i = 0; i < CG_MaxSplitView(); i++) {
			if ( es->number == cg.snap->pss[i].playerNum ) {
				cg.localPlayers[i].powerupActive = PW_REGEN;
				cg.localPlayers[i].powerupTime = cg.time;
			}
		}
		trap_S_StartSound (NULL, es->number, CHAN_ITEM, cgs.media.regenSound );
		break;
	case EV_POWERUP_VAMPIRE:
		DEBUGNAME( "EV_POWERUP_VAMPIRE" );
		for ( i = 0; i < CG_MaxSplitView(); i++ ) {
			if ( es->number == cg.snap->pss[i].playerNum ) {
				cg.localPlayers[i].powerupActive = PW_VAMPIRE;
				cg.localPlayers[i].powerupTime = cg.time;
			}
		}
		trap_S_StartSound( NULL, es->number, CHAN_ITEM, cgs.media.vampireSound );
		break;
	case EV_PARMOR_SCREEN:
		DEBUGNAME( "EV_PARMOR_SCREEN" );
		for ( i = 0; i < CG_MaxSplitView(); i++ ) {
			if ( es->number == cg.snap->pss[i].playerNum ) {
				cg.localPlayers[i].powerupActive = PW_PSCREEN;
				cg.localPlayers[i].powerupTime = cg.time;
			}
		}
		trap_S_StartSound( NULL, es->number, CHAN_ITEM, cgs.media.parmorSound );
		break;
	case EV_PARMOR_SHIELD:
		DEBUGNAME( "EV_PARMOR_SHIELD" );
		for ( i = 0; i < CG_MaxSplitView(); i++ ) {
			if ( es->number == cg.snap->pss[i].playerNum ) {
				cg.localPlayers[i].powerupActive = PW_PSHIELD;
				cg.localPlayers[i].powerupTime = cg.time;
			}
		}
		trap_S_StartSound( NULL, es->number, CHAN_ITEM, cgs.media.parmorSound );
		break;
	case EV_PARMOR_SCREEN_ON:
	case EV_PARMOR_SHIELD_ON:
		DEBUGNAME( "EV_PARMOR_SCREEN_ON" );
		trap_S_StartSound( NULL, es->number, CHAN_ITEM, cgs.media.parmorOnSound );
		break;
	case EV_PARMOR_SCREEN_OFF:
	case EV_PARMOR_SHIELD_OFF:
		DEBUGNAME( "EV_PARMOR_SCREEN_OFF" );
		trap_S_StartSound( NULL, es->number, CHAN_ITEM, cgs.media.parmorOffSound );
		break;
	case EV_STOPLOOPINGSOUND:
		DEBUGNAME("EV_STOPLOOPINGSOUND");
		trap_S_StopLoopingSound( es->number );
		es->loopSound = 0;
		break;

	case EV_DEBUG_LINE:
		DEBUGNAME("EV_DEBUG_LINE");
		CG_Beam( cent );
		break;

	default:
		DEBUGNAME("UNKNOWN");
		CG_Error( "Unknown event: %i", event );
		break;
	}

}


/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents( centity_t *cent ) {
	// check for event-only entities
	if ( cent->currentState.eType > ET_EVENTS ) {
		if ( cent->previousEvent ) {
			return;	// already fired
		}
		// if this is a player event set the entity number of the player entity number
		if ( cent->currentState.eFlags & EF_PLAYER_EVENT ) {
			cent->currentState.number = cent->currentState.otherEntityNum;
		}

		cent->previousEvent = 1;

		cent->currentState.event = cent->currentState.eType - ET_EVENTS;
	} else {
		// check for events riding with another entity
		if ( cent->currentState.event == cent->previousEvent ) {
			return;
		}
		cent->previousEvent = cent->currentState.event;
		if ( ( cent->currentState.event & ~EV_EVENT_BITS ) == 0 ) {
			return;
		}
	}

	// calculate the position at exactly the frame time
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin, cgs.gravity );
	CG_SetEntitySoundPosition( cent );

	CG_EntityEvent( cent, cent->lerpOrigin );
}

