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


typedef struct teamgame_s {
	float			last_flag_capture;
	team_t			last_capture_team;
#if 0
	flagStatus_t	redStatus;	// CTF
	flagStatus_t	blueStatus;	// CTF
	flagStatus_t	flagStatus;	// One Flag CTF
	int				redTakenTime;
	int				blueTakenTime;
	int				redObeliskAttackedTime;
	int				blueObeliskAttackedTime;
#endif
	flagStatus_t	flagStatus[TEAM_NUM_TEAMS];
	int				flagTakenTime[TEAM_NUM_TEAMS];
	int				obeliskAttackedTime[TEAM_NUM_TEAMS];
} teamgame_t;

teamgame_t teamgame;

gentity_t* neutralObelisk;

void Team_SetFlagStatus( team_t team, flagStatus_t status );

void Team_InitGame( void ) {
	memset( &teamgame, 0, sizeof teamgame );

	if ( GTF( GTF_CTF ) ) {
		int i;

		for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ ) {
			teamgame.flagStatus[i] = -1; // Invalid to force update
			Team_SetFlagStatus( i, FLAG_ATBASE );
		}
	} else if ( g_gameType.integer == GT_1FCTF ) {
		teamgame.flagStatus[TEAM_FREE] = -1; // Invalid to force update
		Team_SetFlagStatus( TEAM_FREE, FLAG_ATBASE );
	}
}

int OtherTeam( team_t team ) {
	if ( team == TEAM_RED )
		return TEAM_BLUE;
	else if ( team == TEAM_BLUE )
		return TEAM_RED;
	return team;
}

// NULL for everyone
static __attribute__( (format( printf, 2, 3 )) ) void QDECL PrintMsg( gentity_t* ent, const char* fmt, ... ) {
	char		msg[1024];
	va_list		argptr;
	char* p;

	va_start( argptr, fmt );
	if ( Q_vsnprintf( msg, sizeof( msg ), fmt, argptr ) >= sizeof( msg ) ) {
		G_Error( "PrintMsg overrun" );
	}
	va_end( argptr );

	// double quotes are bad
	while ( (p = strchr( msg, '"' )) != NULL )
		*p = '\'';

	trap_SendServerCommand( ((ent == NULL) ? -1 : ent - g_entities), va( "print \"%s\"", msg ) );
}

/*
==============
AddTeamScore

 for gametype GT_TEAM this is called in AddScore in g_combat.c (checks Frag Limit to win)
 for gametype > GT_TEAM this is called when gametype-specific scoring happens (checks captureLimit to win)
		(e.g. capture flag, kill obelisk, return skulls)
==============
*/
void AddTeamScore( vec3_t origin, team_t team, int score ) {
	int			eventParm;
	int			leadTeamNum, leadTeamScore;
	gentity_t* te;

	if ( score == 0 ) return;

	eventParm = -1;
	if ( team == level.sortedTeams[1] ) {
		leadTeamNum = level.sortedTeams[2];
	} else {
		leadTeamNum = level.sortedTeams[1];
	}
	leadTeamScore = level.teamScores[leadTeamNum];

	if ( level.teamScores[team] + score == leadTeamScore ) {
		//teams are tied sound
		eventParm = GTS_TEAMS_ARE_TIED;
	} else if ( level.teamScores[team] >= leadTeamScore &&
		level.teamScores[team] + score < leadTeamScore ) {
		// other team took the lead sound (negative score)
		eventParm = GTS_REDTEAM_TOOK_LEAD + leadTeamNum - FIRST_TEAM;
	} else if ( level.teamScores[team] <= leadTeamScore &&
		level.teamScores[team] + score > leadTeamScore ) {
		// this team took the lead sound
		eventParm = GTS_REDTEAM_TOOK_LEAD + team - FIRST_TEAM;
	} else if ( score > 0 && !GTF( GTF_TDM ) ) {
		// team scored sound
		eventParm = GTS_REDTEAM_SCORED + team - FIRST_TEAM;
	}

	if ( eventParm != -1 ) {
		te = G_TempEntity( origin, EV_GLOBAL_TEAM_SOUND );
		te->r.svFlags |= SVF_BROADCAST;
		te->s.eventParm = eventParm;
	}

	level.teamScores[team] += score;
}

/*
==============
OnSameTeam
==============
*/
qboolean OnSameTeam( gentity_t* ent1, gentity_t* ent2 ) {
	if ( !ent1 || !ent1->player || !ent2 || !ent2->player ) {
		return qfalse;
	}

	if ( !GTF( GTF_TEAMS ) ) {
		return qfalse;
	}

	if ( ent1->player->sess.sessionTeam == ent2->player->sess.sessionTeam ) {
		return qtrue;
	}

	return qfalse;
}

// based on flagStatus_t
//static char ctfFlagStatusRemap[] = { '0', '1', '2', '4', '5', '6', '7', '*', '*' };
static char ctfFlagStatusRemap[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8' };
static char oneFlagStatusRemap[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8' };

void Team_SetFlagStatus( team_t team, flagStatus_t status ) {
	qboolean modified = qfalse;

	if ( teamgame.flagStatus[team] != status ) {
		teamgame.flagStatus[team] = status;
		modified = qtrue;
	}

	if ( modified ) {
		char st[TEAM_NUM_TEAMS];
		if ( GTF( GTF_CTF ) ) {
			int i, j = 0;

			for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ ) {
				int stat = teamgame.flagStatus[i] < 0 ? 0 : teamgame.flagStatus[i];
				st[j] = ctfFlagStatusRemap[stat];
				j++;
			}

		} else {		// GT_1FCTF
			int stat = teamgame.flagStatus[TEAM_FREE] < 0 ? 0 : teamgame.flagStatus[TEAM_FREE];
			st[0] = oneFlagStatusRemap[stat];
			st[1] = 0;
		}
		trap_SetConfigstring( CS_FLAGSTATUS, st );
	}
}

void Team_CheckDroppedItem( gentity_t* dropped ) {
	Team_SetFlagStatus( dropped->item->giTag - PW_FLAGS_INDEX, FLAG_DROPPED );
}

/*
================
Team_ForceGesture
================
*/
void Team_ForceGesture( int team ) {
	int i;
	gentity_t* ent;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		ent = &g_entities[i];
		if ( !ent->inuse )
			continue;
		if ( !ent->player )
			continue;
		if ( ent->player->sess.sessionTeam != team )
			continue;
		//
		ent->flags |= FL_FORCE_GESTURE;
	}
}

/*
================
Team_FragBonuses

Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumulative.  You get one, they are in importance
order.
================
*/
void Team_FragBonuses( gentity_t* targ, gentity_t* inflictor, gentity_t* attacker ) {
	int			i;
	gentity_t*	ent, * flag, * carrier = NULL;
	char*		c;
	vec3_t		v1, v2;
	team_t		targetTeam, attackerTeam, flagTeam;

	// no bonus for fragging yourself or targetTeam mates
	if ( !targ->player || !attacker->player || targ == attacker || OnSameTeam( targ, attacker ) )
		return;

	targetTeam = targ->player->sess.sessionTeam;
	attackerTeam = attacker->player->sess.sessionTeam;		//OtherTeam(targetTeam);

	if ( attackerTeam < 0 )
		return; // whoever died isn't on a team

	// same team, if the flag at base, check to he has the enemy flag
	if ( GTL( GTL_CAPTURES ) ) {
		flagTeam = BG_CarryingCapturableFlag( &targ->player->ps, g_gameType.integer );

		// did the attacker frag a flag carrier?
		if ( flagTeam >= 0 ) {
			// give bonus only if they were carrying the attacker's targetTeam's flag
			if ( flagTeam == attackerTeam ) {
				attacker->player->pers.teamState.lastfraggedcarrier = level.time;
				AddScore( attacker, targ->r.currentOrigin, CTF_FRAG_CARRIER_BONUS );
			} else {
				AddScore( attacker, targ->r.currentOrigin, 1 );
			}
			PrintMsg( NULL, "%s fragged %s's flag carrier!\n", PlayerName( attacker->player->pers ), G_PlayerTeamName( targetTeam ) );

			// the target had the flag, clear the hurt carrier
			// field on the other targetTeam
			for ( i = 0; i < g_maxClients.integer; i++ ) {
				ent = g_entities + i;
				if ( ent->inuse && ent->player->sess.sessionTeam == attackerTeam )
					ent->player->pers.teamState.lasthurtcarrier = 0;
			}
			return;
		}
	}

	// did the attacker frag a skull carrier?
	if ( g_gameType.integer == GT_HARVESTER && targ->player->ps.skulls ) {
		int skulls = targ->player->ps.skulls;

		attacker->player->pers.teamState.lastfraggedcarrier = level.time;
		AddScore( attacker, targ->r.currentOrigin, CTF_FRAG_CARRIER_BONUS * skulls );
		PrintMsg( NULL, "%s fragged %s's skull carrier!\n", PlayerName( attacker->player->pers ), G_PlayerTeamName( targetTeam ) );

		// the target had the flag, clear the hurt carrier
		// field on the other targetTeam
		for ( i = 0; i < g_maxClients.integer; i++ ) {
			ent = g_entities + i;
			if ( ent->inuse && ent->player->sess.sessionTeam == attackerTeam )
				ent->player->pers.teamState.lasthurtcarrier = 0;
		}
		return;
	}

	if ( targ->player->pers.teamState.lasthurtcarrier &&
		level.time - targ->player->pers.teamState.lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT &&
		BG_CarryingCapturableFlag( &attacker->player->ps, g_gameType.integer ) < 0 ) {
		// attacker is on the same targetTeam as the flag carrier and
		// fragged a guy who hurt our flag carrier
		AddScore( attacker, targ->r.currentOrigin, CTF_CARRIER_DANGER_PROTECT_BONUS );

		targ->player->pers.teamState.lasthurtcarrier = 0;

		attacker->player->ps.persistant[PERS_DEFEND_COUNT]++;
		// add the sprite over the player's head
		G_ClearMedals( &attacker->player->ps );
		attacker->player->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->player->rewardTime = level.time + REWARD_SPRITE_TIME;

		return;
	}

	// flag and flag carrier area defense bonuses

	// we have to find the flag and carrier entities
	if ( g_gameType.integer == GT_OVERLOAD ) {
		c = va( "team_%sobelisk", g_teamNamesLower[attackerTeam] );

	} else if ( g_gameType.integer == GT_HARVESTER ) {
		// center obelisk ent name
		c = "team_neutralobelisk";
	} else if ( g_gameType.integer == GT_1FCTF ) {
		c = va( "team_%sobelisk", g_teamNamesLower[attackerTeam] );

		// find attacker's targetTeam's flag carrier
		for ( i = 0; i < g_maxClients.integer; i++ ) {
			carrier = g_entities + i;
			if ( carrier->player->sess.sessionTeam != attackerTeam )
				continue;

			if ( carrier->inuse && BG_CarryingCapturableFlag( &carrier->player->ps, g_gameType.integer ) >= 0 )
				break;
			carrier = NULL;
		}
	} else if ( GTF( GTF_CTF ) ) {
		// flag ent name
		c = va( "team_CTF_%sflag", g_teamNamesLower[attackerTeam] );

		// find attacker's targetTeam's flag carrier
		for ( i = 0; i < g_maxClients.integer; i++ ) {
			carrier = g_entities + i;
			if ( carrier->player->sess.sessionTeam != attackerTeam )
				continue;

			if ( carrier->inuse && BG_CarryingCapturableFlag( &carrier->player->ps, g_gameType.integer ) >= 0 )
				break;
			carrier = NULL;
		}
	} else return;

	flag = NULL;
	while ( (flag = G_Find( flag, FOFS( classname ), c )) != NULL ) {
		if ( !(flag->s.eFlags & EF_DROPPED_ITEM) )
			break;
	}

	if ( !flag )
		return; // can't find attacker's flag

	// ok we have the attackers flag and a pointer to the carrier

	// check to see if we are defending the base's flag
	VectorSubtract( targ->r.currentOrigin, flag->r.currentOrigin, v1 );
	VectorSubtract( attacker->r.currentOrigin, flag->r.currentOrigin, v2 );

	if ( ((VectorLength( v1 ) < CTF_TARGET_PROTECT_RADIUS &&
		trap_InPVS( flag->r.currentOrigin, targ->r.currentOrigin )) ||
		(VectorLength( v2 ) < CTF_TARGET_PROTECT_RADIUS &&
			trap_InPVS( flag->r.currentOrigin, attacker->r.currentOrigin ))) &&
		attackerTeam != targetTeam ) {

		// we defended the base flag
		AddScore( attacker, targ->r.currentOrigin, CTF_FLAG_DEFENSE_BONUS );

		attacker->player->ps.persistant[PERS_DEFEND_COUNT]++;
		// add the sprite over the player's head
		G_ClearMedals( &attacker->player->ps );
		attacker->player->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->player->rewardTime = level.time + REWARD_SPRITE_TIME;

		return;
	}

	if ( carrier && carrier != attacker ) {
		VectorSubtract( targ->r.currentOrigin, carrier->r.currentOrigin, v1 );
		VectorSubtract( attacker->r.currentOrigin, carrier->r.currentOrigin, v2 );

		if ( ((VectorLength( v1 ) < CTF_ATTACKER_PROTECT_RADIUS &&
			trap_InPVS( carrier->r.currentOrigin, targ->r.currentOrigin )) ||
			(VectorLength( v2 ) < CTF_ATTACKER_PROTECT_RADIUS &&
				trap_InPVS( carrier->r.currentOrigin, attacker->r.currentOrigin ))) &&
			attackerTeam != targetTeam ) {
			AddScore( attacker, targ->r.currentOrigin, CTF_CARRIER_PROTECT_BONUS );

			attacker->player->ps.persistant[PERS_DEFEND_COUNT]++;
			// add the sprite over the player's head
			G_ClearMedals( &attacker->player->ps );
			attacker->player->ps.eFlags |= EF_AWARD_DEFEND;
			attacker->player->rewardTime = level.time + REWARD_SPRITE_TIME;

			return;
		}
	}
}

/*
================
Team_CheckHurtCarrier

Check to see if attacker hurt the flag carrier.  Needed when handing out bonuses for assistance to flag
carrier defense.
================
*/
void Team_CheckHurtCarrier( gentity_t* targ, gentity_t* attacker ) {

	if ( !targ->player || !attacker->player )
		return;

	// flags
	if ( GTL( GTL_CAPTURES ) ) {
		if ( BG_CarryingCapturableFlag( &targ->player->ps, g_gameType.integer ) >= 0
			&& targ->player->sess.sessionTeam != attacker->player->sess.sessionTeam )
			attacker->player->pers.teamState.lasthurtcarrier = level.time;
	} else if ( g_gameType.integer == GT_HARVESTER ) {
		// skulls
		if ( targ->player->ps.skulls &&
			targ->player->sess.sessionTeam != attacker->player->sess.sessionTeam )
			attacker->player->pers.teamState.lasthurtcarrier = level.time;
	}
}


gentity_t* Team_ResetFlag( team_t team ) {
	char* c;
	gentity_t* ent, * rent = NULL;

	c = va( "team_CTF_%sflag", g_teamNamesLower[team] );

	ent = NULL;
	while ( (ent = G_Find( ent, FOFS( classname ), c )) != NULL ) {
		if ( ent->s.eFlags & EF_DROPPED_ITEM )
			G_FreeEntity( ent );
		else {
			rent = ent;
			RespawnItem( ent );
		}
	}

	Team_SetFlagStatus( team, FLAG_ATBASE );

	return rent;
}

void Team_ResetFlags( team_t t1, team_t t2 ) {
	if ( GTF( GTF_CTF ) ) {
		Team_ResetFlag( t1 );
		Team_ResetFlag( t2 );
	} else if ( g_gameType.integer == GT_1FCTF ) {
		Team_ResetFlag( TEAM_FREE );
	}
}

void Team_ReturnFlagSound( gentity_t* ent, team_t team ) {
	gentity_t* te;

	if ( ent == NULL ) {
		G_Printf( "Warning:  NULL passed to Team_ReturnFlagSound\n" );
		return;
	}

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
	te->s.eventParm = GTS_RED_RETURN + team - FIRST_TEAM;

	te->r.svFlags |= SVF_BROADCAST;
}

void Team_TakeFlagSound( gentity_t* ent, team_t carrierTeam, team_t flagTeam ) {
	gentity_t* te;

	if ( ent == NULL ) {
		G_Printf( "Warning:  NULL passed to Team_TakeFlagSound\n" );
		return;
	}

	// only play sound when the flag was at the base
	// or not picked up the last 10 seconds
	if ( teamgame.flagStatus[flagTeam] != FLAG_ATBASE ) {
		if ( teamgame.flagTakenTime[flagTeam] > level.time - 10000 )
			return;
	}
	teamgame.flagTakenTime[flagTeam] = level.time;

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
	te->s.eventParm = GTS_RED_TAKEN - FIRST_TEAM + flagTeam;

	te->r.svFlags |= SVF_BROADCAST;
}

void Team_CaptureFlagSound( gentity_t* ent, team_t team ) {
	gentity_t* te;

	if ( ent == NULL ) {
		G_Printf( "Warning:  NULL passed to Team_CaptureFlagSound\n" );
		return;
	}

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
	te->s.eventParm = GTS_RED_CAPTURE + team - FIRST_TEAM;
	te->r.svFlags |= SVF_BROADCAST;
}

void Team_ReturnFlag( team_t team ) {
	Team_ReturnFlagSound( Team_ResetFlag( team ), team );
	if ( team == TEAM_FREE ) {
		PrintMsg( NULL, "The flag has returned!\n" );
	} else {
		PrintMsg( NULL, "%s's flag has returned!\n", G_PlayerTeamName( team ) );
	}
}

void Team_FreeEntity( gentity_t* ent ) {

	Team_ReturnFlag( ent->item->giTag - PW_FLAGS_INDEX );
}

/*
==============
Team_DroppedFlagThink

Automatically set in Launch_Item if the item is one of the flags

Flags are unique in that if they are dropped, the base flag must be respawned when they time out
==============
*/
void Team_DroppedFlagThink( gentity_t* ent ) {
	const team_t team = ent->item->giTag - PW_FLAGS_INDEX;

	Team_ReturnFlagSound( Team_ResetFlag( team ), team );
	// Reset Flag will delete this entity
}


/*
==============
Team_TouchOurFlag
==============
*/
int Team_TouchOurFlag( gentity_t* ent, gentity_t* other, team_t friendlyTeam ) {
	int				i;
	gentity_t* player;
	gplayer_t* cl = other->player;
	const team_t	enemyTeam = BG_CarryingCapturableFlag( &cl->ps, g_gameType.integer );
	int				flagTime;
	char			*sFlagTime;
	char			str[256];

	if ( ent->s.eFlags & EF_DROPPED_ITEM ) {
		// hey, it's not home.  return it by teleporting it back
		PrintMsg( NULL, "%s returned %s's flag!\n", PlayerName( cl->pers ), G_PlayerTeamName( friendlyTeam ) );
		AddScore( other, ent->r.currentOrigin, CTF_RECOVERY_BONUS );

		//muff: reset timer
		level.miscTimer[friendlyTeam] = 0;

		other->player->pers.teamState.lastreturnedflag = level.time;
		//ResetFlag will remove this entity!  We must return zero
		Team_ReturnFlagSound( Team_ResetFlag( friendlyTeam ), friendlyTeam );
		return 0;
	}

	if ( enemyTeam < 0 ) return 0; // We don't have the flag

	if ( (flagTime = level.time - level.miscTimer[enemyTeam]) ) {
		int		mins, seconds;

		seconds = flagTime / 1000;
		flagTime -= seconds * 1000;
		mins = seconds / 60;
		seconds -= mins * 60;

		if ( mins ) {
			sFlagTime = va( "%i:%02d.%02d", mins, seconds, flagTime );
		} else {
			sFlagTime = va( "%i.%2d", seconds, flagTime );
		}
	} else {
		sFlagTime = "NONE";
	}

	// the flag is at home base.  if the player has the enemy
	// flag, he's just scored!
	if ( g_gameType.integer == GT_1FCTF ) {
		Com_sprintf( str, sizeof( str ), "print \"%s captured the flag! " S_COLOR_GREY "(timed: %s)\n\"", PlayerName( cl->pers ), sFlagTime );
	} else {
		Com_sprintf( str, sizeof( str ), "print \"%s captured %s's flag! " S_COLOR_GREY "(timed: %s)\n\"", PlayerName( cl->pers ), G_PlayerTeamName( enemyTeam ), sFlagTime );
	}
	AP( str );
	
	cl->ps.powerups[PW_FLAGS_INDEX + enemyTeam] = 0;

	teamgame.last_flag_capture = level.time;
	teamgame.last_capture_team = friendlyTeam;

	// increase the team's score
	AddTeamScore( ent->s.pos.trBase, other->player->sess.sessionTeam, 1 );
	Team_ForceGesture( other->player->sess.sessionTeam );

	// add the sprite over the player's head
	G_ClearMedals( &other->player->ps );
	other->player->ps.eFlags |= EF_AWARD_CAP;
	other->player->rewardTime = level.time + REWARD_SPRITE_TIME;
	other->player->ps.persistant[PERS_CAPTURES]++;

	// other gets another 10 frag bonus
	AddScore( other, ent->r.currentOrigin, CTF_CAPTURE_BONUS );

	Team_CaptureFlagSound( ent, friendlyTeam );

	// Ok, let's do the player loop, hand out the bonuses
	for ( i = 0; i < g_maxClients.integer; i++ ) {
		player = &g_entities[i];

		// also make sure we don't award assist bonuses to the flag carrier himself.
		if ( !player->inuse || player == other )
			continue;

		if ( player->player->sess.sessionTeam !=
			cl->sess.sessionTeam ) {
			player->player->pers.teamState.lasthurtcarrier = -5;
		} else if ( player->player->sess.sessionTeam ==
			cl->sess.sessionTeam ) {
			AddScore( player, ent->r.currentOrigin, CTF_TEAM_BONUS );
			// award extra points for capture assists
			if ( player->player->pers.teamState.lastreturnedflag +
				CTF_RETURN_FLAG_ASSIST_TIMEOUT > level.time ) {
				AddScore( player, ent->r.currentOrigin, CTF_RETURN_FLAG_ASSIST_BONUS );

				player->player->ps.persistant[PERS_ASSIST_COUNT]++;
				// add the sprite over the player's head
				G_ClearMedals( &player->player->ps );
				player->player->ps.eFlags |= EF_AWARD_ASSIST;
				player->player->rewardTime = level.time + REWARD_SPRITE_TIME;

			}
			if ( player->player->pers.teamState.lastfraggedcarrier +
				CTF_FRAG_CARRIER_ASSIST_TIMEOUT > level.time ) {
				AddScore( player, ent->r.currentOrigin, CTF_FRAG_CARRIER_ASSIST_BONUS );

				player->player->ps.persistant[PERS_ASSIST_COUNT]++;
				// add the sprite over the player's head
				G_ClearMedals( &player->player->ps );
				player->player->ps.eFlags |= EF_AWARD_ASSIST;
				player->player->rewardTime = level.time + REWARD_SPRITE_TIME;
			}
		}
	}
	Team_ResetFlags( friendlyTeam, enemyTeam );

	CalculateRanks();

	return 0; // Do not respawn this automatically
}

int Team_TouchEnemyFlag( gentity_t* ent, gentity_t* other, team_t flagTeam ) {
	gplayer_t* cl = other->player;

	if ( g_gameType.integer == GT_1FCTF ) {
		PrintMsg( NULL, "%s got the flag!\n", PlayerName( other->player->pers ) );

		cl->ps.powerups[PW_NEUTRALFLAG] = INT_MAX; // flags never expire
		Team_SetFlagStatus( TEAM_FREE, FLAG_TAKEN_RED + flagTeam );
	} else {
		PrintMsg( NULL, "%s got %s's flag!\n", PlayerName( other->player->pers ), G_PlayerTeamName( flagTeam ) );

		cl->ps.powerups[PW_FLAGS_INDEX + flagTeam] = INT_MAX; // flags never expire

		Team_SetFlagStatus( flagTeam, FLAG_TAKEN );
	}
	if ( !(ent->s.eFlags & EF_DROPPED_ITEM) ) {
		//muff: start a capture timer for this team flag, only timed from base pickup
		level.miscTimer[flagTeam] = level.time;
	}

	AddScore( other, ent->r.currentOrigin, CTF_FLAG_BONUS );
	Team_TakeFlagSound( ent, other->player->sess.sessionTeam, flagTeam );

	return -1; // Do not respawn this automatically, but do delete it if it was FL_DROPPED
}

int Pickup_Team( gentity_t* ent, gentity_t* other ) {
	team_t team = -1;
	gplayer_t* cl = other->player;
	int	i;

	if ( g_gameType.integer == GT_OVERLOAD ) {
		// there are no team items that can be picked up in Overload
		G_FreeEntity( ent );
		return 0;
	}
	
	if ( g_gameType.integer == GT_HARVESTER ) {
		// the only team items that can be picked up in harvester are the skulls
		if ( ent->s.team != cl->sess.sessionTeam ) {
			cl->ps.skulls += 1;
		}
		G_FreeEntity( ent );
		return 0;
	}

	// only capture-based games after this
	if ( !GTL( GTL_CAPTURES ) ) return 0;

	// figure out what team this flag is
	for ( i = 0; i < TEAM_NUM_TEAMS; i++ ) {
		char* s = va( "team_CTF_%sflag", g_teamNamesLower[i] );
		if ( !strcmp( ent->classname, s) ) {
			team = i;
			//PrintMsg( other, "flagname =%s, team=%i\n", s, i );
			break;
		}
	}
	if ( team < 0 ) {
		PrintMsg( other, "Don't know what team the flag is on.\n" );
		return 0;
	}

	if ( g_gameType.integer == GT_1FCTF ) {
		if ( team == TEAM_FREE ) {
			return Team_TouchEnemyFlag( ent, other, cl->sess.sessionTeam );
		}
		return 0;
#if 0
		if ( team != cl->sess.sessionTeam ) {
			return Team_TouchOurFlag( ent, other, cl->sess.sessionTeam );
		}
#endif
	} else if ( team == cl->sess.sessionTeam ) {
		return Team_TouchOurFlag( ent, other, team );
	}
	return Team_TouchEnemyFlag( ent, other, team );
}


/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
gentity_t* Team_GetLocation( gentity_t* ent ) {
	gentity_t* eloc, * best;
	float			bestlen, len;
	vec3_t			origin;

	if ( !ent ) {
		return NULL;
	}

	best = NULL;
	bestlen = 3 * 8192.0 * 8192.0;

	VectorCopy( ent->r.currentOrigin, origin );

	for ( eloc = level.locationHead; eloc; eloc = eloc->nextTrain ) {
		len = (origin[0] - eloc->r.currentOrigin[0]) * (origin[0] - eloc->r.currentOrigin[0])
			+ (origin[1] - eloc->r.currentOrigin[1]) * (origin[1] - eloc->r.currentOrigin[1])
			+ (origin[2] - eloc->r.currentOrigin[2]) * (origin[2] - eloc->r.currentOrigin[2]);

		if ( len > bestlen ) {
			continue;
		}

		if ( !trap_InPVS( origin, eloc->r.currentOrigin ) ) {
			continue;
		}

		bestlen = len;
		best = eloc;
	}

	return best;
}


/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
qboolean Team_GetLocationMsg( gentity_t* ent, char* loc, int loclen ) {
	gentity_t* best;

	best = Team_GetLocation( ent );

	if ( !best )
		return qfalse;

	if ( best->count ) {
		if ( best->count < 0 )
			best->count = 0;
		if ( best->count > 7 )
			best->count = 7;
		Com_sprintf( loc, loclen, "%c%c%s" S_COLOR_WHITE, Q_COLOR_ESCAPE, best->count + '0', best->message );
	} else
		Com_sprintf( loc, loclen, "%s", best->message );

	return qtrue;
}


/*---------------------------------------------------------------------------*/

/*
================
SelectRandomTeamSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_TEAM_SPAWN_POINTS	32
gentity_t* SelectRandomTeamSpawnPoint( int teamstate, team_t team ) {
	gentity_t* spot;
	int			count;
	int			selection;
	gentity_t* spots[MAX_TEAM_SPAWN_POINTS];
	char* classname;

	classname = va( "team_CTF_%s%s", g_teamNamesLower[team], teamstate == TEAM_BEGIN ? "player" : "spawn" );
	//if ( !IsValidTeam( team ) ) return G_Find( NULL, FOFS( classname ), classname );
	count = 0;
	spot = NULL;

	while ( (spot = G_Find( spot, FOFS( classname ), classname )) != NULL ) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		spots[count] = spot;
		if ( ++count == MAX_TEAM_SPAWN_POINTS )
			break;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS( classname ), classname );
	}

	selection = rand() % count;
	return spots[selection];
}


/*
===========
SelectTeamBaseSpawnPoint

============
*/
gentity_t* SelectTeamBaseSpawnPoint( team_t team, int teamState, vec3_t origin, vec3_t angles, qboolean isBot ) {
	gentity_t* spot = SelectRandomTeamSpawnPoint( teamState, team );

	if ( !spot ) return SelectSpawnPoint( vec3_origin, origin, angles, isBot );

	VectorCopy( spot->s.origin, origin );
	//origin[2] += 9;
	VectorCopy( spot->s.angles, angles );

	return spot;
}

/*---------------------------------------------------------------------------*/

static int QDECL SortPlayers( const void* a, const void* b ) {
	return *(int*)a - *(int*)b;
}


/*
==================
TeamplayLocationsMessage

Format:
	playerNum location health armor weapon powerups

==================
*/
void TeamplayInfoMessage( gentity_t* ent ) {
	char		entry[1024];
	char		string[8192];
	int			stringlength;
	int			i, j;
	gentity_t* player;
	int			cnt;
	int			h, a;
	int			players[TEAM_MAXOVERLAY];
	int			team;

	if ( !ent->player->pers.teamInfo ) return;

	// send team info to spectator for team of followed player
	if ( ent->player->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( ent->player->sess.spectatorState != SPECTATOR_FOLLOW
			|| ent->player->sess.spectatorPlayer < 0 ) {
			return;
		}
		team = g_entities[ent->player->sess.spectatorPlayer].player->sess.sessionTeam;
	} else {
		team = ent->player->sess.sessionTeam;
	}

	if ( !IsValidTeam( team ) ) return;

	// figure out what player should be on the display
	// we are limited to 8, but we want to use the top eight players
	// but in player order (so they don't keep changing position on the overlay)
	for ( i = 0, cnt = 0; i < g_maxClients.integer && cnt < TEAM_MAXOVERLAY; i++ ) {
		player = g_entities + level.sortedPlayers[i];
		if ( player->inuse && player->player->sess.sessionTeam == team ) {
			players[cnt++] = level.sortedPlayers[i];
		}
	}

	// We have the top eight players, sort them by playerNum
	qsort( players, cnt, sizeof( players[0] ), SortPlayers );

	// send the latest information on all players
	string[0] = 0;
	stringlength = 0;

	for ( i = 0, cnt = 0; i < g_maxClients.integer && cnt < TEAM_MAXOVERLAY; i++ ) {
		player = g_entities + i;
		if ( player->inuse && player->player->sess.sessionTeam == team ) {

			h = player->player->ps.stats[STAT_HEALTH];
			a = player->player->ps.stats[STAT_ARMOR];
			if ( h < 0 ) h = 0;
			else if ( h > 999 ) h = 999;
			if ( a < 0 ) a = 0;
			else if ( a > 999 ) a = 999;

			Com_sprintf( entry, sizeof( entry ),
					" %i %i %i %i %i %i",
					level.sortedPlayers[i], player->player->pers.teamState.location, h, a, 
//					i, player->player->pers.teamState.location, h, a,
					player->player->ps.weapon, player->s.powerups );
				j = strlen( entry );
				if ( stringlength + j >= sizeof( string ) )
					break;
			strcpy( string + stringlength, entry );
			stringlength += j;
			cnt++;
		}
	}

	trap_SendServerCommand( ent - g_entities, va( "tinfo %i %i%s", team, cnt, string ) );
}

void CheckTeamStatus( void ) {
	int i;
	gentity_t* loc, * ent;

	if ( level.time - level.lastTeamLocationTime > TEAM_LOCATION_UPDATE_TIME ) {

		level.lastTeamLocationTime = level.time;

		for ( i = 0; i < g_maxClients.integer; i++ ) {
			ent = g_entities + i;

			if ( ent->player->pers.connected != CON_CONNECTED ) {
				continue;
			}

			if ( ent->inuse && IsValidTeam( ent->player->sess.sessionTeam ) ) {
				loc = Team_GetLocation( ent );
				ent->player->pers.teamState.location = loc ? loc->health : 0;
			}
		}

		for ( i = 0; i < g_maxClients.integer; i++ ) {
			ent = g_entities + i;

			if ( ent->player->pers.connected != CON_CONNECTED ) {
				continue;
			}

			if ( ent->inuse ) {
				TeamplayInfoMessage( ent );
			}
		}
	}
}

/*-----------------------------------------------------------------*/

/*QUAKED team_CTF_redplayer (1 0 0) (-16 -16 -16) (16 16 32)
Only in team games with bases. Red players spawn here at game start.
*/
void SP_team_CTF_redplayer( gentity_t* ent ) {
	G_DropEntityToFloor( ent, SPAWNPOINT_DROPDIST );
}

/*QUAKED team_CTF_blueplayer (0 0 1) (-16 -16 -16) (16 16 32)
Only in team games with bases. Blue players spawn here at game start.
*/
void SP_team_CTF_blueplayer( gentity_t* ent ) {
	G_DropEntityToFloor( ent, SPAWNPOINT_DROPDIST );
}

/*QUAKED team_CTF_greenplayer (0 0 1) (-16 -16 -16) (16 16 32)
Only in team games with bases. Green players spawn here at game start.
*/
void SP_team_CTF_greenplayer( gentity_t* ent ) {
	G_DropEntityToFloor( ent, SPAWNPOINT_DROPDIST );
}

/*QUAKED team_CTF_yellowplayer (0 0 1) (-16 -16 -16) (16 16 32)
Only in team games with bases. Yellow players spawn here at game start.
*/
void SP_team_CTF_yellowplayer( gentity_t* ent ) {
	G_DropEntityToFloor( ent, SPAWNPOINT_DROPDIST );
}

/*QUAKED team_CTF_tealplayer (0 0 1) (-16 -16 -16) (16 16 32)
Only in team games with bases. Teal players spawn here at game start.
*/
void SP_team_CTF_tealplayer( gentity_t* ent ) {
	G_DropEntityToFloor( ent, SPAWNPOINT_DROPDIST );
}

/*QUAKED team_CTF_pinkplayer (0 0 1) (-16 -16 -16) (16 16 32)
Only in team games with bases. Pink players spawn here at game start.
*/
void SP_team_CTF_pinkplayer( gentity_t* ent ) {
	G_DropEntityToFloor( ent, SPAWNPOINT_DROPDIST );
}

/*QUAKED team_CTF_redspawn (1 0 0) (-16 -16 -24) (16 16 32)
Potential spawning position for Red Team in team games with bases.
Targets will be fired when someone spawns in on them.
*/
void SP_team_CTF_redspawn( gentity_t* ent ) {
	level.map_teamBaseSpawns |= (1 << TEAM_RED);
	G_DropEntityToFloor( ent, SPAWNPOINT_DROPDIST );
}

/*QUAKED team_CTF_bluespawn (0 0 1) (-16 -16 -24) (16 16 32)
Potential spawning position for Blue Team in team games with bases.
Targets will be fired when someone spawns in on them.
*/
void SP_team_CTF_bluespawn( gentity_t* ent ) {
	level.map_teamBaseSpawns |= (1 << TEAM_BLUE);
	G_DropEntityToFloor( ent, SPAWNPOINT_DROPDIST );
}

/*QUAKED team_CTF_greenspawn (0 0 1) (-16 -16 -24) (16 16 32)
Potential spawning position for Green Team in team games with bases.
Targets will be fired when someone spawns in on them.
*/
void SP_team_CTF_greenspawn( gentity_t* ent ) {
	level.map_teamBaseSpawns |= (1 << TEAM_GREEN);
	G_DropEntityToFloor( ent, SPAWNPOINT_DROPDIST );
}

/*QUAKED team_CTF_yellowspawn (0 0 1) (-16 -16 -24) (16 16 32)
Potential spawning position for Yellow Team in team games with bases.
Targets will be fired when someone spawns in on them.
*/
void SP_team_CTF_yellowspawn( gentity_t* ent ) {
	level.map_teamBaseSpawns |= (1 << TEAM_YELLOW);
	G_DropEntityToFloor( ent, SPAWNPOINT_DROPDIST );
}

/*QUAKED team_CTF_tealspawn (0 0 1) (-16 -16 -24) (16 16 32)
Potential spawning position for Teal Team in team games with bases.
Targets will be fired when someone spawns in on them.
*/
void SP_team_CTF_tealspawn( gentity_t* ent ) {
	level.map_teamBaseSpawns |= (1 << TEAM_TEAL);
	G_DropEntityToFloor( ent, SPAWNPOINT_DROPDIST );
}

/*QUAKED team_CTF_pinkspawn (0 0 1) (-16 -16 -24) (16 16 32)
Potential spawning position for Pink Team in team games with bases.
Targets will be fired when someone spawns in on them.
*/
void SP_team_CTF_pinkspawn( gentity_t* ent ) {
	level.map_teamBaseSpawns |= (1 << TEAM_PINK);
	G_DropEntityToFloor( ent, SPAWNPOINT_DROPDIST );
}

/*
================
Obelisks
================
*/

static void ObeliskRegen( gentity_t* self ) {
	self->nextthink = level.time + g_obeliskRegenPeriod.integer * 1000;
	if ( self->health >= g_obeliskHealth.integer ) {
		return;
	}

	G_AddEvent( self, EV_POWERUP_REGEN, 0 );
	self->health += g_obeliskRegenAmount.integer;
	if ( self->health > g_obeliskHealth.integer ) {
		self->health = g_obeliskHealth.integer;
	}

	self->activator->s.modelindex2 = self->health * 0xff / g_obeliskHealth.integer;
	self->activator->s.frame = 0;
}


static void ObeliskRespawn( gentity_t* self ) {
	self->takedamage = qtrue;
	self->health = g_obeliskHealth.integer;

	self->think = ObeliskRegen;
	self->nextthink = level.time + g_obeliskRegenPeriod.integer * 1000;

	self->activator->s.frame = 0;
}


static void ObeliskDie( gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int mod ) {
	team_t attackerTeam;

	attackerTeam = attacker->player->sess.sessionTeam;
	AddTeamScore( self->s.pos.trBase, attackerTeam, 1 );
	Team_ForceGesture( attackerTeam );

	CalculateRanks();

	self->takedamage = qfalse;
	self->think = ObeliskRespawn;
	self->nextthink = level.time + g_obeliskRespawnDelay.integer * 1000;

	self->activator->s.modelindex2 = 0xff;
	self->activator->s.frame = 2;

	G_AddEvent( self->activator, EV_OBELISKEXPLODE, 0 );

	AddScore( attacker, self->r.currentOrigin, CTF_CAPTURE_BONUS );

	// add the sprite over the player's head
	G_ClearMedals( &attacker->player->ps );
	attacker->player->ps.eFlags |= EF_AWARD_CAP;
	attacker->player->rewardTime = level.time + REWARD_SPRITE_TIME;
	attacker->player->ps.persistant[PERS_CAPTURES]++;

	Team_CaptureFlagSound( self, self->spawnflags );

	teamgame.obeliskAttackedTime[self->spawnflags] = 0;
	teamgame.obeliskAttackedTime[attackerTeam] = 0;

	AP( va( "cp \"%s destroyed %s's obelisk.\n\"", PlayerName( attacker->player->pers ), G_PlayerTeamName( self->spawnflags ) ) );
}


static void BaseRecepticleTouch( gentity_t* self, gentity_t* other, trace_t* trace ) {
	team_t	playerTeam;

	if ( !other->player ) return;
	if ( !GTF( GTF_BASEOB ) ) return;

	playerTeam = other->player->sess.sessionTeam;
	if ( playerTeam == self->spawnflags ) return;

	if ( g_gameType.integer == GT_1FCTF ) {
		qboolean flag = other->player->ps.powerups[PW_NEUTRALFLAG];
		int		flagTime;
		char	*sFlagTime;
		char	str[256];

		if ( !flag ) return;

		if ( (flagTime = level.time - level.miscTimer[TEAM_FREE]) ) {
			int		mins, seconds;

			seconds = flagTime / 1000;
			flagTime -= seconds * 1000;
			mins = seconds / 60;
			seconds -= mins * 60;

			if ( mins ) {
				sFlagTime = va( "%i:%02d.%02d", mins, seconds, flagTime );
			} else {
				sFlagTime = va( "%i.%2d", seconds, flagTime );
			}
		} else {
			sFlagTime = "NONE";
		}

		// the flag is at home base.  if the player has the enemy
		// flag, he's just scored!
		Com_sprintf( str, sizeof( str ), "print \"%s captured the flag! " S_COLOR_GREY "(timed: %s)\n\"", PlayerName( other->player->pers ), sFlagTime );

		AP( str );

		other->player->ps.powerups[PW_NEUTRALFLAG] = 0;

		teamgame.last_flag_capture = level.time;
		teamgame.last_capture_team = playerTeam;

		// increase the team's score
		AddTeamScore( self->s.pos.trBase, playerTeam, 1 );
		Team_ForceGesture( playerTeam );

		// add the sprite over the player's head
		G_ClearMedals( &other->player->ps );
		other->player->ps.eFlags |= EF_AWARD_CAP;
		other->player->rewardTime = level.time + REWARD_SPRITE_TIME;
		other->player->ps.persistant[PERS_CAPTURES]++;

		// other gets another score bonus
		AddScore( other, self->r.currentOrigin, CTF_CAPTURE_BONUS );

		Team_CaptureFlagSound( self, playerTeam );

	} else {
		int			skulls;

		skulls = other->player->ps.skulls;
		if ( skulls <= 0 ) return;

		AP( va( "cp \"%s brought in %i %s.\n\"",
			PlayerName( other->player->pers ), skulls, (skulls == 1) ? "skull" : "skulls" ) );

		AddTeamScore( self->s.pos.trBase, playerTeam, skulls );
		Team_ForceGesture( other->player->sess.sessionTeam );

		AddScore( other, self->r.currentOrigin, CTF_CAPTURE_BONUS * skulls );

		// add the sprite over the player's head
		G_ClearMedals( &other->player->ps );
		other->player->ps.eFlags |= EF_AWARD_CAP;
		other->player->rewardTime = level.time + REWARD_SPRITE_TIME;
		other->player->ps.persistant[PERS_CAPTURES] += skulls;

		other->player->ps.skulls = 0;

		Team_CaptureFlagSound( self, self->spawnflags );
	}

	CalculateRanks();
}


static void ObeliskPain( gentity_t* self, gentity_t* attacker, int damage ) {
	int actualDamage = damage / 10;
	if ( actualDamage <= 0 ) {
		actualDamage = 1;
	}
	self->activator->s.modelindex2 = self->health * 0xff / g_obeliskHealth.integer;
	if ( !self->activator->s.frame ) {
		G_AddEvent( self, EV_OBELISKPAIN, 0 );
	}
	self->activator->s.frame = 1;
	AddScore( attacker, self->r.currentOrigin, actualDamage );
}

// spawn invisible damagable obelisk entity / harvester base trigger.
gentity_t* SpawnObelisk( vec3_t origin, vec3_t mins, vec3_t maxs, int team ) {
	gentity_t* ent = G_Spawn();

	VectorCopy( origin, ent->s.origin );
	VectorCopy( origin, ent->s.pos.trBase );
	VectorCopy( origin, ent->r.currentOrigin );

	VectorCopy( mins, ent->s.mins );
	VectorCopy( maxs, ent->s.maxs );

	ent->s.eType = ET_GENERAL;
	ent->flags = FL_NO_KNOCKBACK;

	if ( g_gameType.integer == GT_OVERLOAD ) {
		ent->s.contents = CONTENTS_SOLID;
		ent->takedamage = qtrue;
		ent->health = g_obeliskHealth.integer;
		ent->die = ObeliskDie;
		ent->pain = ObeliskPain;
		ent->think = ObeliskRegen;
		ent->nextthink = level.time + g_obeliskRegenPeriod.integer * 1000;
	} else if ( g_gameType.integer == GT_HARVESTER || g_gameType.integer == GT_1FCTF ) {
		ent->s.contents = CONTENTS_TRIGGER;
		ent->touch = BaseRecepticleTouch;
	}

	G_SetOrigin( ent, ent->s.origin );

	ent->spawnflags = team;

	trap_LinkEntity( ent );

	return ent;
}


// setup entity for team base model / obelisk model.
void ObeliskInit( gentity_t* ent ) {
	trace_t		tr;
	vec3_t		dest;

	ent->s.eType = ET_TEAM;

	VectorSet( ent->s.mins, -15, -15, 0 );
	VectorSet( ent->s.maxs, 15, 15, 87 );

	if ( ent->spawnflags & 1 ) {
		// suspended
		G_SetOrigin( ent, ent->s.origin );
	} else {
		// mappers like to put them exactly on the floor, but being coplanar
		// will sometimes show up as starting in solid, so lif it up one pixel
		ent->s.origin[2] += 1;

		// drop to floor
		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
		trap_Trace( &tr, ent->s.origin, ent->s.mins, ent->s.maxs, dest, ent->s.number, MASK_SOLID );
		if ( tr.startsolid ) {
			ent->s.origin[2] -= 1;
			G_Printf( "SpawnObelisk: %s startsolid at %s\n", ent->classname, vtos( ent->s.origin ) );

			ent->s.groundEntityNum = ENTITYNUM_NONE;
			G_SetOrigin( ent, ent->s.origin );
		} else {
			// allow to ride movers
			ent->s.groundEntityNum = tr.entityNum;
			G_SetOrigin( ent, tr.endpos );
		}
	}
}

void SpawnTeamObelisk( gentity_t* ent, const team_t entityTeam ) {
	gentity_t* obelisk;

	if ( !GTF( GTF_BASEOB ) ) {
		G_FreeEntity( ent );
		return;
	}
	ObeliskInit( ent );
	if ( g_gameType.integer == GT_OVERLOAD ) {
		obelisk = SpawnObelisk( ent->s.origin, ent->s.mins, ent->s.maxs, entityTeam );
		obelisk->activator = ent;
		// initial obelisk health value
		ent->s.modelindex2 = 0xff;
		ent->s.frame = 0;
	} else if ( g_gameType.integer == GT_HARVESTER || g_gameType.integer == GT_1FCTF ) {
		obelisk = SpawnObelisk( ent->s.origin, ent->s.mins, ent->s.maxs, entityTeam );
		obelisk->activator = ent;
	}

	G_DropEntityToFloor( ent, 0 );

	ent->s.modelindex = entityTeam;
	trap_LinkEntity( ent );
}

/*QUAKED team_redobelisk (1 0 0) (-16 -16 0) (16 16 8)
*/
void SP_team_redobelisk( gentity_t* ent ) {
	SpawnTeamObelisk( ent, TEAM_RED );
}

/*QUAKED team_blueobelisk (0 0 1) (-16 -16 0) (16 16 88)
*/
void SP_team_blueobelisk( gentity_t* ent ) {
	SpawnTeamObelisk( ent, TEAM_BLUE );
}

/*QUAKED team_greenobelisk (0 0 1) (-16 -16 0) (16 16 88)
*/
void SP_team_greenobelisk( gentity_t* ent ) {
	SpawnTeamObelisk( ent, TEAM_GREEN );
}

/*QUAKED team_yellowobelisk (0 0 1) (-16 -16 0) (16 16 88)
*/
void SP_team_yellowobelisk( gentity_t* ent ) {
	SpawnTeamObelisk( ent, TEAM_YELLOW );
}

/*QUAKED team_tealobelisk (0 0 1) (-16 -16 0) (16 16 88)
*/
void SP_team_tealobelisk( gentity_t* ent ) {
	SpawnTeamObelisk( ent, TEAM_TEAL );
}

/*QUAKED team_pinkobelisk (0 0 1) (-16 -16 0) (16 16 88)
*/
void SP_team_pinkobelisk( gentity_t* ent ) {
	SpawnTeamObelisk( ent, TEAM_PINK );
}

/*QUAKED team_neutralobelisk (0 0 1) (-16 -16 0) (16 16 88)
*/
void SP_team_neutralobelisk( gentity_t* ent ) {
	if ( !GTF( GTF_NEUTOB ) ) {
		G_FreeEntity( ent );
		return;
	}
	ObeliskInit( ent );
	if ( g_gameType.integer == GT_HARVESTER ) {
		neutralObelisk = SpawnObelisk( ent->s.origin, ent->s.mins, ent->s.maxs, TEAM_FREE );
		neutralObelisk->activator = ent;
	}
	ent->s.modelindex = TEAM_FREE;
	trap_LinkEntity( ent );
}

/*
================
CheckObeliskAttack
================
*/
qboolean CheckObeliskAttack( gentity_t* obelisk, gentity_t* attacker ) {
	gentity_t* te;

	// if this really is an obelisk
	if ( obelisk->die != ObeliskDie ) {
		return qfalse;
	}

	// if the attacker is a player
	if ( !attacker->player ) {
		return qfalse;
	}

	// if the obelisk is on the same team as the attacker then don't hurt it
	if ( obelisk->spawnflags == attacker->player->sess.sessionTeam ) {
		return qtrue;
	}

	// obelisk may be hurt

	// if not played any sounds recently
	if ( teamgame.obeliskAttackedTime[obelisk->spawnflags] < level.time - OVERLOAD_ATTACK_BASE_SOUND_TIME ) {
		// tell which obelisk is under attack
		te = G_TempEntity( obelisk->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
		te->s.eventParm = GTS_REDOBELISK_ATTACKED + obelisk->spawnflags - FIRST_TEAM;
		teamgame.obeliskAttackedTime[obelisk->spawnflags] = level.time;
		te->r.svFlags |= SVF_BROADCAST;
	}

	return qfalse;
}
