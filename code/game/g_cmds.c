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

// for checking if EC is used by match templates
#include "ai_chat_sys.h"

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t* ent ) {
	char		entry[1024];
	char		string[1000];
	//char		teamstr[1024];
	int			stringlength;
	int			i, j;
	gplayer_t* cl;
	int			numSorted, scoreFlags, perfect;

	// don't send scores to bots (bots don't parse them)
	if ( ent->r.svFlags & SVF_BOT ) {
		return;
	}

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;
#if 0
	Q_strcat( entry, sizeof( entry ), "scores" );
	for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ )
		Q_strcat( entry, sizeof( entry ), va( " %i", level.teamScores[i] ) );
#endif
	numSorted = level.numConnectedPlayers;
	for ( i = 0; i < numSorted; i++ ) {
		int		ping;

		cl = &level.players[level.sortedPlayers[i]];

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		perfect = (cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0) ? 1 : 0;

		//Q_strcat( entry, sizeof( entry ),
		Com_sprintf( entry, sizeof( entry ),
			" %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.sortedPlayers[i],
				cl->ps.persistant[PERS_SCORE], ping, (level.time - cl->pers.enterTime) / 60000,
				scoreFlags, g_entities[level.sortedPlayers[i]].s.powerups, G_WeaponsTotalAccuracy( cl ),
				cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
				cl->ps.persistant[PERS_EXCELLENT_COUNT],
				cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
				cl->ps.persistant[PERS_DEFEND_COUNT],
				cl->ps.persistant[PERS_ASSIST_COUNT],
				perfect,
				cl->ps.persistant[PERS_CAPTURES],
				cl->ps.persistant[PERS_HOLYSHIT_COUNT]
		);
		j = strlen( entry );
		if ( stringlength + j >= sizeof( string ) )
			break;
		strcpy( string + stringlength, entry );
		stringlength += j;
	}

	//AP( entry );
	AP( va( "scores %i %i %i %i %i %i %i%s", i,
		level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE],
		level.teamScores[TEAM_GREEN], level.teamScores[TEAM_YELLOW],
		level.teamScores[TEAM_TEAL], level.teamScores[TEAM_PINK],
		string ) );
}


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t* ent ) {
	DeathmatchScoreboardMessage( ent );
}




/*
==================
G_SendWeaponsPlayerStats

Send a client their weapon stats
==================
*/
void G_SendWeaponsPlayerStats( gplayer_t* cl, const int playerNum ) {
#if 0
	int		/*num, */stringlength;
	int		i, j;
	char	entry[WP_NUM_WEAPONS][96];
	char	string[1024];

	//num = 0;
	string[0] = 0;
	stringlength = 0;
	for ( i = WP_MACHINEGUN; i < WP_NUM_WEAPONS; i++ ) {
		if ( i == WP_GRAPPLING_HOOK ) continue;

		Com_sprintf( entry[i], sizeof( entry[i] ), "%i %i %i %i ", cl->statsWeaponShots[i], cl->statsWeaponHits[i], cl->statsWeaponDmgD[i], cl->statsWeaponDmgR[i] );
		j = strlen( entry[i] );
		if ( stringlength + j >= sizeof( string ) )
			break;
		strcpy( string + stringlength, entry[i] );
		stringlength += j;
	}

	trap_SendServerCommand( playerNum, va( "pwstats %i %s", playerNum, string ) );
#endif
}


/*
==================
CheatsOk
==================
*/
qboolean	CheatsOk( gentity_t* ent ) {
	if ( !g_cheats.integer ) {
		AP( "print \"Cheats are not enabled on this server.\n\"" );
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		AP( "print \"You must be alive to use this command.\n\"" );
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char* ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start; i < c; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}


/*
==================
StringIsInteger
==================
*/
qboolean StringIsInteger( const char* s ) {
	int			i;
	int			len;
	qboolean	foundDigit;

	len = strlen( s );
	foundDigit = qfalse;

	for ( i = 0; i < len; i++ ) {
		if ( !isdigit( s[i] ) ) {
			return qfalse;
		}

		foundDigit = qtrue;
	}

	return foundDigit;
}


/*
==================
PlayerNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int PlayerNumberFromString( gentity_t* to, char* s, qboolean checkNums, qboolean checkNames ) {
	gplayer_t* cl;
	int			idnum;
	char		cleanName[MAX_STRING_CHARS];

	if ( checkNums ) {
		// numeric values could be slot numbers
		if ( StringIsInteger( s ) ) {
			idnum = atoi( s );
			if ( idnum >= 0 && idnum < level.maxplayers ) {
				cl = &level.players[idnum];
				if ( cl->pers.connected == CON_CONNECTED ) {
					return idnum;
				}
			}
		}
	}

	if ( checkNames ) {
		// check for a name match
		for ( idnum = 0, cl = level.players; idnum < level.maxplayers; idnum++, cl++ ) {
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			Q_strncpyz( cleanName, cl->pers.netname, sizeof( cleanName ) );
			Q_CleanStr( cleanName );
			if ( !Q_stricmp( cleanName, s ) ) {
				return idnum;
			}
		}
	}

	trap_SendServerCommand( to - g_entities, va( "print \"User %s is not on the server\n\"", s ) );

	return -1;
}





/*
=================
Cmd_DropItem_f
=================
*/
void Cmd_DropItem_f( gentity_t* ent ) {
	gplayer_t* cl;
	gitem_t* ditem;
	gentity_t* dent;
	char	*name;
	int		i, count, dtype;
	qboolean stock, droppable;
	char* stype;

	if ( trap_Argc() < 2 ) {
		CP( va( "print \"Usage: drop [itemtype/itemname]\n\"" ) );
		return;
	}
	
	cl = ent->player;
	name = ConcatArgs( 1 );
	stock = droppable = qtrue;
	dtype = count = 0;
	ditem = BG_FindItem( name );
	stype = "";
	if ( ditem ) {
		dtype = ditem->giType;
	}

	// find item by item type
	if ( (!dtype && !Q_stricmp( name, "weapon" )) || dtype == IT_WEAPON ) {
		int weapon = dtype ? ditem->giTag : cl->ps.weapon;
		stype = "weapon";
		if ( !ditem ) ditem = BG_FindItemForWeapon( cl->ps.weapon );

		if ( weapon > 0 && weapon < WP_NUM_WEAPONS && weapon != WP_GAUNTLET ) {
			if ( cl->ps.stats[STAT_WEAPONS] & (1 << weapon) ) {
				if ( cl->ps.ammo[weapon] > 0 ) {
					if ( ditem ) {
						count = (ditem->quantity > cl->ps.ammo[cl->ps.weapon]) ? cl->ps.ammo[cl->ps.weapon] : ditem->quantity;
					}
				} else {
					CP( "print \"Cannot drop weapon without ammo.\n\"" );
					return;
				}
			} else {
				stock = qfalse;
			}
		} else {
			droppable = qfalse;
		}
	} else if ( (!dtype && !Q_stricmp( name, "ammo" ) ) || dtype == IT_AMMO ) {
		weapon_t weapon = dtype ? ditem->giTag : cl->ps.weapon;
		stype = "ammo";

		if ( !ditem ) ditem = BG_FindItemForAmmo( weapon );

		if ( weapon > 0 && weapon < WP_NUM_WEAPONS && weapon != WP_GAUNTLET && weapon != WP_GRAPPLING_HOOK ) {
			if ( cl->ps.ammo[weapon] > 0 ) {
				if ( ditem ) {
					count = (ditem->quantity > cl->ps.ammo[weapon]) ? cl->ps.ammo[weapon] : ditem->quantity;
				}
			} else {
				stock = qfalse;
			}
		} else {
			droppable = qfalse;
		}
	} else if ( (!dtype && !Q_stricmp( name, "holdable" )) || dtype == IT_HOLDABLE ) {
		if ( !cl->ps.stats[STAT_HOLDABLE_ITEM] ) {
			stock = qfalse;
		} else if ( !dtype ) {
			if ( !ditem ) ditem = BG_ItemForItemNum( cl->ps.stats[STAT_HOLDABLE_ITEM] );
			if ( !ditem ) stock = qfalse;
		} else if ( cl->ps.stats[STAT_HOLDABLE_ITEM] != BG_ItemNumForItem( ditem ) ) {
			stock = qfalse;
		}
	} else if ( (!dtype && !Q_stricmp( name, "powerup" )) || dtype == IT_POWERUP ) {
		stype = "powerup";
		if ( !dtype ) {
			for ( i = 0; i < MAX_POWERUPS; i++ ) {
				if ( cl->ps.powerups[i] ) {
					ditem = BG_FindItemForPowerup( i );
					if ( ditem ) {
						if ( ditem->giType != IT_POWERUP )
							continue;
					}
					break;
				}
			}
			if ( ditem ) {
				count = cl->ps.powerups[i];
				if ( !count ) stock = qfalse;
			} else {
				stock = qfalse;
			}
		} else if ( !cl->ps.powerups[ditem->giTag] ) {
			stock = qfalse;
		} else {
			count = cl->ps.powerups[ditem->giTag];
		}
	} else if ( (!dtype && !Q_stricmp( name, "key" ) ) || dtype == IT_KEY ) {
		stype = "key";
		if ( !dtype ) {
			for ( i = 0; i < MAX_UNLOCK_KEYS; i++ ) {
				if ( cl->ps.keys[i] ) {
					ditem = BG_FindItemForUnlockKey( i );
					break;
				}
			}
			if ( ditem ) {
				if ( !cl->ps.keys[i] ) {
					stock = qfalse;
				}
			} else {
				stock = qfalse;
			}
		} else if ( !cl->ps.keys[ditem->giTag] ) {
			stock = qfalse;
		}
	} else if ( ( !dtype && !Q_stricmp( name, "rune" ) ) || dtype == IT_RUNE ) {
		CP( "print \"Runes not currently droppable.\n\"" );	//FIXME: make it happen
		return;
#if 0
		stype = "rune";
		if ( !cl->ps.stats[STAT_RUNE] ) {
			stock = qfalse;
		} else if ( !dtype ) {
			ditem = BG_ItemForItemNum( cl->ps.stats[STAT_RUNE] );
		} else if ( cl->ps.stats[STAT_RUNE] != BG_ItemNumForItem( ditem ) ) {
			stock = qfalse;
		}
#endif
	} else if ( (!dtype && !Q_stricmp( name, "flag" ) ) || dtype == IT_TEAM ) {
		stype = "flag";
		if ( GTF( GTF_TEAMBASES ) ) {
			if ( !dtype ) {
				for ( i = PW_FLAGS_INDEX; i < PW_FLAGS_INDEX + TEAM_NUM_TEAMS; i++ ) {
					if ( cl->ps.powerups[i] ) {
						ditem = BG_FindItemForPowerup( i );
						break;
					}
				}
				if ( ditem ) {
					if ( !cl->ps.powerups[i] ) {
						stock = qfalse;
					}
				} else {
					stock = qfalse;
				}
			} else if ( !cl->ps.powerups[ditem->giTag] ) {
				stock = qfalse;
			}
		} else {
			return;
		}
	} else if ( (!dtype && !Q_stricmp( name, "health" ) ) || dtype == IT_HEALTH ) {
		stype = "health";
		if ( ditem && ditem->quantity != 5 ) {
			CP( "print \"Item is not droppable.\n\"" );
			return;
		}
		if ( cl->ps.stats[STAT_HEALTH] > 5 ) {
			ditem = BG_FindItem( "Small Health" );
			if ( ditem ) {
				count = 5;
			}
		} else {
			CP( "print \"Not enough health available to drop.\n\"" );
			return;
		}
	} else if ( ( !Q_stricmp( name, "armor" ) ) || dtype == IT_ARMOR ) {
		int num = bgarmor[g_armorRules.integer][ARMOR_SHARD].base_count;

		stype = "armor";
		if ( ditem && ditem->quantity != num ) {
			CP( "print \"Item is not droppable.\n\"" );
			return;
		}

		if ( cl->ps.stats[STAT_ARMOR] >= num ) {
			ditem = BG_FindItem( "Armor Shard" );
			if ( ditem ) {
				count = num;
			}
		} else {
			CP( "print \"Not enough armor available to drop.\n\"" );
			return;
		}
	} else {
		CP( "print \"Invalid item.\n\"" );
		return;
	}

	if ( !ditem && !stock ) {
		CP( va( "print \"No item to drop: %s.\n\"", name ) );
		return;
	}
	if ( !ditem ) {
		CP( va( "print \"Invalid item: %s.\n\"", name ) );
		return;
	}
	if ( !stock ) {
		CP( va( "print \"Out of item: %s.\n\"", ditem->pickup_name ) );
		return;
	}
	if ( !droppable ) {
		CP( va( "print \"Item not droppable: %s.\n\"", ditem->pickup_name ) );
		return;
	}
	if ( g_dropFlags.integer && g_dropFlags.integer & (1 << ditem->giType) ) {
		CP( va( "print \"The server has disabled %s drops.\n\"", stype ? stype : "item" ) );
		return;
	}

	// drop it like it's hot
	dent = Drop_Item( ent, ditem, 0 );
	if ( count > 0 ) dent->count = count;

	// take away dropped item from inventory
	switch ( ditem->giType ) {
	case IT_WEAPON:
		// remove weapon and a quantity of ammo
		cl->ps.stats[STAT_WEAPONS] &= ~(1 << ditem->giTag);
		cl->ps.ammo[ditem->giTag] -= count;

		// if dropping current weapon, change weapons now
		if ( cl->ps.weapon == ditem->giTag ) {
			for ( i = WP_NUM_WEAPONS - 1; i > 0; i-- ) {
				if ( cl->ps.stats[STAT_WEAPONS] & (1 << i) ) {
					cl->ps.weapon = i;
					break;
				}
			}
		}
		break;
	case IT_AMMO:
		cl->ps.ammo[ditem->giTag] -= count;
		break;
	case IT_HEALTH:
		ent->health -= count;
		cl->ps.stats[STAT_HEALTH] = ent->health;
		break;
	case IT_ARMOR:
		cl->ps.stats[STAT_ARMOR] -= count;
		break;
	case IT_POWERUP:
		cl->ps.powerups[ditem->giTag] = 0;
		break;
	case IT_KEY:
		cl->ps.keys[ditem->giTag] = 0;
		break;
	case IT_RUNE:
		cl->ps.stats[STAT_RUNE] = 0;
		break;
	case IT_TEAM:
		cl->ps.powerups[ditem->giTag] = 0;
		break;
	case IT_HOLDABLE:
		cl->ps.stats[STAT_HOLDABLE_ITEM] = 0;
		if ( ditem->giTag == HI_PSCREEN || ditem->giTag == HI_PSHIELD ) {
			cl->ps.stats[STAT_PARMOR_ACTIVE] = 0;
		}
		//TODO kamikaze
		break;
	default:
		break;
	}
}


/*
==================
Cmd_SpawnItem_f

Spawn and toss an item forward
==================
*/
void Cmd_SpawnItem_f( gentity_t* ent ) {
	gitem_t* it;

	if ( !CheatsOk( ent ) ) return;

	it = BG_FindItem( ConcatArgs( 1 ) );
	if ( !it ) return;

	RegisterItem( it );
	G_AddEvent( ent, EV_REGISTER_ITEM, BG_ItemNumForItem( it ) );
	Drop_Item( ent, it, 0 );
}


/*
==================
Cmd_Give_f

Give items to a player
==================
*/
void Cmd_Give_f( gentity_t* ent ) {
	char* name;
	gitem_t* it;
	int			i;
	qboolean	give_all;
	gentity_t* it_ent;
	trace_t		trace;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	name = ConcatArgs( 1 );

	if ( Q_stricmp( name, "all" ) == 0 )
		give_all = qtrue;
	else
		give_all = qfalse;

	if ( give_all || Q_stricmp( name, "health" ) == 0 ) {
		ent->health = ent->player->ps.stats[STAT_MAX_HEALTH];
		if ( !give_all )
			return;
	}

	if ( give_all || Q_stricmp( name, "weapons" ) == 0 ) {
		ent->player->ps.stats[STAT_WEAPONS] = (1 << WP_NUM_WEAPONS) - 1 -
			(1 << WP_GRAPPLING_HOOK) - (1 << WP_NONE);
		if ( !give_all )
			return;
	}

	if ( give_all || Q_stricmp( name, "ammo" ) == 0 ) {
		for ( i = 0; i < MAX_WEAPONS; i++ ) {
			ent->player->ps.ammo[i] = 999;
		}
		if ( !give_all )
			return;
	}

	if ( give_all || Q_stricmp( name, "armor" ) == 0 ) {
		ent->player->ps.stats[STAT_ARMOR] = 200;
		ent->player->ps.stats[STAT_ARMOR_TYPE] = ARMOR_BODY;

		if ( !give_all )
			return;
	}

	if ( Q_stricmp( name, "excellent" ) == 0 ) {
		ent->player->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if ( Q_stricmp( name, "impressive" ) == 0 ) {
		ent->player->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if ( Q_stricmp( name, "gauntletaward" ) == 0 ) {
		ent->player->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		return;
	}
	if ( Q_stricmp( name, "defend" ) == 0 ) {
		ent->player->ps.persistant[PERS_DEFEND_COUNT]++;
		return;
	}
	if ( Q_stricmp( name, "assist" ) == 0 ) {
		ent->player->ps.persistant[PERS_ASSIST_COUNT]++;
		return;
	}
	if ( Q_stricmp( name, "holyshit" ) == 0 ) {
		ent->player->ps.persistant[PERS_HOLYSHIT_COUNT]++;
		return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem( name );
		if ( !it ) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;
		G_SpawnItem( it_ent, it );
		FinishSpawningItem( it_ent );
		memset( &trace, 0, sizeof( trace ) );
		Touch_Item( it_ent, ent, &trace );
		if ( it_ent->inuse ) {
			G_FreeEntity( it_ent );
		}
	}
}


/*
==================
Cmd_God_f

Sets player to godmode

argv(0) god
==================
*/
void Cmd_God_f( gentity_t* ent ) {
	char* msg;

	if ( !CheatsOk( ent ) ) return;

	ent->flags ^= FL_GODMODE;
	msg = (ent->flags & FL_GODMODE) ? "ON" : "OFF";

	CP( va( "print \"%c%cgodMode %c%c%s\n\"", Q_COLOR_ESCAPE, COLOR_CREAM, Q_COLOR_ESCAPE, COLOR_WHITE, msg  ) );
}


/*
==================
Cmd_Notarget_f

Sets player to noTarget

argv(0) noTarget
==================
*/
void Cmd_Notarget_f( gentity_t* ent ) {
	char* msg;

	if ( !CheatsOk( ent ) ) return;

	ent->flags ^= FL_NOTARGET;
	msg = (ent->flags & FL_NOTARGET) ? "ON" : "OFF";

	CP( va( "print \"%c%cnoTarget %c%c%s\n\"", Q_COLOR_ESCAPE, COLOR_CREAM, Q_COLOR_ESCAPE, COLOR_WHITE, msg ) );
}


/*
==================
Cmd_Noclip_f

argv(0) noClip
==================
*/
void Cmd_Noclip_f( gentity_t* ent ) {
	char* msg;

	if ( !CheatsOk( ent ) ) return;

	ent->player->noClip = !ent->player->noClip;
	msg = ent->player->noClip ? "ON" : "OFF";

	CP( va( "print \"%c%cnoClip %c%c%s\n\"", Q_COLOR_ESCAPE, COLOR_CREAM, Q_COLOR_ESCAPE, COLOR_WHITE, msg ) );
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t* ent ) {
	char arg[MAX_TOKEN_CHARS];

	if ( !ent->player->pers.localClient ) {
		AP( "print \"The levelShot command must be executed by a local client\n\"" );
		return;
	}

	if ( !CheatsOk( ent ) ) return;

	// doesn't work in single player
	if ( g_singlePlayerActive.integer ) {
		AP( "print \"Must not be in singleplayer mode for levelShot\n\"" );
		return;
	}

	BeginIntermission();
	trap_Argv( 1, arg, sizeof( arg ) );
	trap_SendServerCommandEx( ent->player->pers.connectionNum, -1, va( "clientLevelShot %s", arg ) );
}


/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t* ent ) {
	if ( ent->player->sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}
	if ( ent->health <= 0 ) {
		return;
	}
	ent->flags &= ~FL_GODMODE;
	ent->player->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die( ent, ent, ent, NULL, 100000, MOD_SUICIDE );
}

/*
=================
BroadcastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gplayer_t* player, int oldTeam ) {
	if ( player->sess.sessionTeam == oldTeam )
		return;

	if ( player->sess.sessionTeam == TEAM_SPECTATOR ) {
		AP( va( "print \"%s joined the spectators.\n\"", PlayerName( player->pers ) ) );
	} else if ( player->sess.sessionTeam == TEAM_FREE ) {
		AP( va( "print \"%s joined the match.\n\"", PlayerName( player->pers ) ) );
	} else {	// if ( player->sess.sessionTeam >= FIRST_TEAM ) {	//IsValidTeam( player->sess.sessionTeam ) ) {
		//G_Printf( "max teams: %i, new player team index: %i\n", level.teams_max, player->sess.sessionTeam );
		AP( va( "print \"%s joined the %s.\n\"", PlayerName( player->pers ), G_PlayerTeamName( player->sess.sessionTeam ) ) );
	}
}

/*
=================
SetTeam
=================
*/
void SetTeam( gentity_t* ent, const char* s ) {
	int					team, oldTeam;
	gplayer_t* player;
	int					playerNum;
	spectatorState_t	specState;
	int					specPlayer;
	int					teamLeader;

	//
	// see what change is requested
	//
	player = ent->player;

	playerNum = player - level.players;
	specPlayer = 0;
	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( !Q_stricmp( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specPlayer = -1;
	} else if ( !Q_stricmp( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specPlayer = -2;
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) || !Q_stricmp( s, "spec" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	} else if ( !Q_stricmp( s, "a" ) || !Q_stricmp( s, "auto" ) ) {
		if ( GTF( GTF_TEAMS ) ) {
			team = PickTeam( playerNum );
		} else {
			team = TEAM_FREE;
		}
	} else if ( GTF( GTF_TEAMS ) ) {
		int i;
		// if running a team game, assign player to one of the teams
		team = TEAM_SPECTATOR;

		for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ ) {
			if ( !Q_stricmp( s, g_teamNamesLower[i] ) ) {
				team = i;
				break;
			}
		}
		if ( !IsValidTeam( team ) ) {
			for ( i = FIRST_TEAM; i < TEAM_NUM_TEAMS; i++ ) {
				if ( !Q_stricmp( s, g_teamNamesLetter[i] ) ) {
					team = i;
					break;
				}
			}
		}
		specState = SPECTATOR_NOT;
		if ( !IsValidTeam( team ) ) {
			trap_SendServerCommand( playerNum, va( "cp \"%s has been disabled in this server.\n\"", G_PlayerTeamName( i ) ) );
			return;	// team = PickTeam( playerNum );
		}

		if ( g_teamForceBalance.integer && !player->pers.localClient && !(ent->r.svFlags & SVF_BOT) ) {
			// We allow a spread of two
			if ( level.largestTeamCount > level.smallestTeamCount + 1 && level.numTeamPlayers[i] == level.largestTeamCount ) {
				trap_SendServerCommand( playerNum, va( "cp \"%s has too many players.\n\"", G_PlayerTeamName( i ) ) );
				return; // ignore the request
			}
			// It's ok, the team we are switching to has less or same number of players
		}

	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}


	// override decision if limiting the players
	if ( GTF( GTF_DUEL ) && level.numNonSpectatorPlayers >= 2 ) {
		team = TEAM_SPECTATOR;
	} else if ( g_maxGameClients.integer > 0 &&
			level.numNonSpectatorPlayers >= g_maxGameClients.integer ) {
		trap_SendServerCommand( playerNum, "cp \"Cannot join the match - player limit has been reached.\n\"" );
		team = TEAM_SPECTATOR;
	} else if ( GTF( GTF_TEAMS ) ) {
		if ( g_teamSize_max.integer > 0 &&
				level.numTeamPlayers[team] >= g_teamSize_max.integer ) {
			trap_SendServerCommand( playerNum, va("cp \"%s has too many players.\n\"", G_TeamName(team)) );
			team = TEAM_SPECTATOR;
		}
	}


	//
	// decide if we will allow the change
	//
	oldTeam = player->sess.sessionTeam;
	if ( team == oldTeam && team != TEAM_SPECTATOR ) {
		return;
	}

	//
	// execute the team change
	//

	// if the player was dead leave the body, but only if they're actually in game
	if ( player->ps.stats[STAT_HEALTH] <= 0 && player->pers.connected == CON_CONNECTED ) {
		CopyToBodyQueue( ent );
	}

	// he starts at 'base'
	player->pers.teamState.state = TEAM_BEGIN;
	if ( oldTeam != TEAM_SPECTATOR ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->player->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die( ent, ent, ent, NULL, 100000, MOD_SUICIDE_TEAM_CHANGE );
	}

	// they go to the end of the line for tournaments
	if ( team == TEAM_SPECTATOR && oldTeam != team )
		AddToTournamentQueue( player );

	player->sess.sessionTeam = team;
	player->sess.spectatorState = specState;
	player->sess.spectatorPlayer = specPlayer;

	player->sess.teamLeader = qfalse;
	if ( team >= FIRST_TEAM ) {
		teamLeader = TeamLeader( team );
		// if there is no team leader or the team leader is a bot and this player is not a bot
		if ( teamLeader == -1 || (!(g_entities[playerNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT)) ) {
			SetLeader( team, playerNum );
		}
	}
	// make sure there is a team leader on the team the player came from
	if ( oldTeam >= FIRST_TEAM ) {
		CheckTeamLeader( oldTeam );
	}

	// get and distribute relevant parameters
	PlayerUserinfoChanged( playerNum );

	// player hasn't spawned yet, they sent teamPref or g_teamAutoJoin is enabled
	if ( player->pers.connected != CON_CONNECTED ) {
		return;
	}

	BroadcastTeamChange( player, oldTeam );

	PlayerBegin( playerNum );
}

/*
=================
StopFollowing

If the player being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t* ent ) {
	ent->player->ps.persistant[PERS_TEAM] = TEAM_SPECTATOR;
	ent->player->sess.sessionTeam = TEAM_SPECTATOR;
	ent->player->sess.spectatorState = SPECTATOR_FREE;
	ent->player->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->player->ps.playerNum = ent - g_entities;

	SetPlayerViewAngle( ent, ent->player->ps.viewangles );

	// don't use dead view angles
	if ( ent->player->ps.stats[STAT_HEALTH] <= 0 ) {
		ent->player->ps.stats[STAT_HEALTH] = 1;
	}
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t* ent ) {
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	oldTeam = ent->player->sess.sessionTeam;

	if ( trap_Argc() != 2 && GTF(GTF_TEAMS) ) {
		AP( va( "print \"You are on %s.\n\"", G_PlayerTeamName( oldTeam ) ) );
		return;
	}

	if ( ent->player->switchTeamTime > level.time ) {
		if ( GTF( GTF_TEAMS ) ) {
			AP( "print \"You may not switch teams more than once per 5 seconds.\n\"" );
		} else {
			AP( "print \"You may not switch between playing and spectating more than once per 5 seconds.\n\"" );
		}
		return;
	}

	// if they are playing a tournament game, count as a loss
	if ( GTF( GTF_DUEL ) && ent->player->sess.sessionTeam == TEAM_FREE ) {
		ent->player->sess.losses++;
	}

	trap_Argv( 1, s, sizeof( s ) );

	SetTeam( ent, s );

	if ( oldTeam != ent->player->sess.sessionTeam ) {
		ent->player->switchTeamTime = level.time + 5000;
	}
}


/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t* ent ) {
	int		i;
	char	arg[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		if ( ent->player->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	i = PlayerNumberFromString( ent, arg, qtrue, qtrue );
	if ( i == -1 ) {
		return;
	}

	// can't follow self
	if ( &level.players[i] == ent->player ) {
		return;
	}

	// can't follow another spectator
	if ( level.players[i].sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

	// don't follow one of their local players
	if ( level.players[i].pers.connectionNum == ent->player->pers.connectionNum ) {
		return;
	}

	// if they are playing a tournament game, count as a loss
	if ( GTF( GTF_DUEL )
			&& ent->player->sess.sessionTeam == TEAM_FREE ) {
		ent->player->sess.losses++;
	}

	// first set them to spectator
	if ( ent->player->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator" );
	}

	ent->player->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->player->sess.spectatorPlayer = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t* ent, int dir ) {
	int		playerNum;
	int		original;

	// if they are playing a tournament game, count as a loss
	if ( GTF( GTF_DUEL )
		&& ent->player->sess.sessionTeam == TEAM_FREE ) {
		ent->player->sess.losses++;
	}
	// first set them to spectator
	if ( ent->player->sess.spectatorState == SPECTATOR_NOT ) {
		SetTeam( ent, "spectator" );
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	// if dedicated follow player, just switch between the two auto players
	if ( ent->player->sess.spectatorPlayer < 0 ) {
		if ( ent->player->sess.spectatorPlayer == -1 ) {
			ent->player->sess.spectatorPlayer = -2;
		} else if ( ent->player->sess.spectatorPlayer == -2 ) {
			ent->player->sess.spectatorPlayer = -1;
		}
		return;
	}

	playerNum = ent->player->sess.spectatorPlayer;
	original = playerNum;
	do {
		playerNum += dir;
		if ( playerNum >= level.maxplayers ) {
			playerNum = 0;
		}
		if ( playerNum < 0 ) {
			playerNum = level.maxplayers - 1;
		}

		// can only follow connected players
		if ( level.players[playerNum].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( level.players[playerNum].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		// don't follow one of their local players
		if ( level.players[playerNum].pers.connectionNum == ent->player->pers.connectionNum ) {
			continue;
		}

		// this is good, we can use it
		ent->player->sess.spectatorPlayer = playerNum;
		ent->player->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( playerNum != original );

	// leave it where it was
}


/*
==================
G_Say
==================
*/

static qboolean G_SayTo( gentity_t* ent, gentity_t* other, int mode ) {
	if ( !other ) {
		return qfalse;
	}
	if ( !other->inuse ) {
		return qfalse;
	}
	if ( !other->player ) {
		return qfalse;
	}
	if ( other->player->pers.connected != CON_CONNECTED ) {
		return qfalse;
	}
	if ( mode == SAY_TEAM && !OnSameTeam( ent, other ) ) {
		return qfalse;
	}
	// no chatting to players in tournaments
	if ( GTF( GTF_DUEL )
		&& other->player->sess.sessionTeam == TEAM_FREE
		&& ent && ent->player && ent->player->sess.sessionTeam != TEAM_FREE ) {
		return qfalse;
	}

	return qtrue;
}

// escape character for botfiles/match.c parsing
#define EC		"\x19"
#if 0
/*
===========
TokenizeTeamChat
============
*/
#define Q_CHATTOKEN_ESCAPE	'%'
#define Q_IsChatTokenString(p)	( p && *(p) == Q_CHATTOKEN_ESCAPE && *((p)+1) && *((p)+1) != Q_CHATTOKEN_ESCAPE )
static void TokeniseTeamChat( gentity_t* ent, const char* in, char* out, int outSize ) {
	int outpos = 0, tokenlessLen = 0, spaces = 0;

	// discard leading spaces
	for ( ; *in == ' '; in++ );

	for ( ; *in && outpos < outSize - 1; in++ ) {
		out[outpos] = *in;

		if ( *in == ' ' ) {
			// don't allow too many consecutive spaces
			if ( spaces > 2 )
				continue;

			spaces++;
		} else if ( outpos > 0 && out[outpos - 1] == Q_CHATTOKEN_ESCAPE ) {
			if ( Q_IsChatTokenString( &out[outpos - 1] ) ) {
				char* insert = NULL;
				tokenlessLen--;

				// health value
				if ( *in == 'H' || *in == 'h' ) {
					insert = va( "%i HP", ent->player->ps.stats[STAT_HEALTH] );
				}
				// armor value
				else if ( *in == 'A' || *in == 'a' ) {
					insert = va( "%i AP", ent->player->ps.stats[STAT_ARMOR] );
				}
				// current weapon name
				else if ( *in == 'W' || *in == 'w' ) {
					if ( ent->player->ps.weapon > 0 && ent->player->ps.weapon < WP_NUM_WEAPONS )
						insert = va( "%s", weaponNamesShort[ent->player->ps.weapon] );
				}
				// current holdable name
				else if ( *in == 'B' || *in == 'b' ) {
					int *holdstr = BG_ItemForItemNum( ent->player->ps.stats[STAT_HOLDABLE_ITEM] )->giTag;
					if ( holdstr ) insert = va( "%s", holdstr );
				}
				// current weapon ammo value
				else if ( *in == 'M' || *in == 'm' ) {
					if ( ent->player->ps.weapon > 0 && ent->player->ps.weapon < WP_NUM_WEAPONS
							&& ent->player->ps.weapon != WP_GAUNTLET && ent->player->ps.weapon != WP_GRAPPLING_HOOK ) {
						const char* names[WP_NUM_WEAPONS] = {
							"", "", "Bullets", "Shells", "Grenades", "Rockets",
							"Lightning Charge", "Slugs", "Cells", "BFG Cells", "",
						#ifdef MISSIONPACK
							"Nails", "Mines", "Chaingun Belt"
						#endif
						};
						insert = va( "%i %s", ent->player->ps.ammo[ent->player->ps.weapon], names[ent->player->ps.weapon] );
					}
				}
				// weapon ammo stock
				else if ( *in == 'U' || *in == 'u' ) {
					if ( ent->player->ps.weapon > 0 && ent->player->ps.weapon < WP_NUM_WEAPONS
						&& ent->player->ps.weapon != WP_GAUNTLET && ent->player->ps.weapon != WP_GRAPPLING_HOOK ) {
						int j;
						const char* names[WP_NUM_WEAPONS] = {
							"", "", "Bullets", "Shells", "Grenades", "Rockets",
							"Lightning Charge", "Slugs", "Cells", "BFG Cells", "",
						#ifdef MISSIONPACK
							"Nails", "Mines", "Chaingun Belt"
						#endif
						};
						for ( j = 0; j < WP_NUM_WEAPONS; j++ ) {
							if ( ent->player->ps.stats[STAT_WEAPONS] & (1 << j) ) {
								if ( insert ) strcat( insert, ", " );
								strcat( insert, va( "%i %s", ent->player->ps.ammo[j], names[j] ) );
							}
						}
					}
				}

				if ( insert ) {
					out[outpos] = *insert;
				}
			} else {
				spaces = 0;
				tokenlessLen++;
			}
		} else {
			spaces = 0;
			tokenlessLen++;
		}

		outpos++;
	}

	out[outpos] = '\0';

}
#endif
static void G_RemoveChatEscapeChar( char* text ) {
	int i, l;

	l = 0;
	for ( i = 0; text[i]; i++ ) {
		if ( text[i] == '\x19' )
			continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}

// ent is NULL for messages from dedicated server
void G_Say( gentity_t* ent, gentity_t* target, int mode, const char* chatText ) {
	static int	useChatEscapeCharacter = -1;
	int			i, j;
	gentity_t* other;
	int			color;
	char		name[64];
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	char		location[64];
	char* cmd, * str;	// , * netname;
	int			playerNum;
	char		extname[MAX_NAME_LENGTH + 16];

	if ( !GTF( GTF_TEAMS ) && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	if ( target && !G_SayTo( ent, target, mode ) ) {
		return;
	}
	if ( ent && ent->player ) {
		if ( ent->player->pers.netclan[0] != '\0' ) {
			Com_sprintf( extname, sizeof( extname ), "%c%c%s %c%c%s%c%c", Q_COLOR_ESCAPE, COLOR_WHITE, ent->player->pers.netclan, Q_COLOR_ESCAPE, COLOR_WHITE, ent->player->pers.netname, Q_COLOR_ESCAPE, COLOR_CREAM );
		} else {
			Com_sprintf( extname, sizeof( extname ), "%c%c%s%c%c", Q_COLOR_ESCAPE, COLOR_WHITE, ent->player->pers.netname, Q_COLOR_ESCAPE, COLOR_CREAM );
		}
		playerNum = ent->s.number;
	} else {
		Com_sprintf( extname, sizeof( extname ), "%c%cserver", Q_COLOR_ESCAPE, COLOR_CREAM );
		playerNum = CHATPLAYER_SERVER;
	}

	Q_strncpyz( text, chatText, sizeof( text ) );

	switch ( mode ) {
	default:
	case SAY_ALL:
		G_LogPrintf( "say: %s: %s\n", extname, text );
		Com_sprintf( name, sizeof( name ), "%s"EC": ", extname );
		color = COLOR_GREEN;
		cmd = "chat";
		break;
	case SAY_TEAM:
	{
		char *stok;
		G_LogPrintf( "sayteam: %s: %s\n", extname, text );
		if ( Team_GetLocationMsg( ent, location, sizeof( location ) ) )
			Com_sprintf( name, sizeof( name ), EC"(%s%c%c"EC") (%s)"EC": ",
				extname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		else
			Com_sprintf( name, sizeof( name ), EC"(%s%c%c"EC")"EC": ",
				extname, Q_COLOR_ESCAPE, COLOR_WHITE );

		color = COLOR_CYAN;
		cmd = "tchat";
		break;
	}
	case SAY_TELL:
		if ( target && target->player ) {
			G_LogPrintf( "tell: %s to %s: %s\n", extname, target->player->pers.netname, text );
		}
		if ( OnSameTeam( ent, target ) && Team_GetLocationMsg( ent, location, sizeof( location ) ) )
			Com_sprintf( name, sizeof( name ), EC"[%s%c%c"EC"] (%s)"EC": ", extname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		else
			Com_sprintf( name, sizeof( name ), EC"[%s%c%c"EC"]"EC": ", extname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_MAGENTA;
		cmd = "tell";
		break;
	}

	// if botfiles/match.c doesn't have EC (unpatched/demo Q3), it needs to be
	// removed for bots to be able to recognize the message.
	if ( useChatEscapeCharacter == -1 ) {
		useChatEscapeCharacter = BotMatchTemplatesContainsString( EC );
	}
	if ( !useChatEscapeCharacter ) {
		G_RemoveChatEscapeChar( name );
	}

	str = va( "%s \"%s%c%c%s\" %d", cmd, name, Q_COLOR_ESCAPE, color, text, playerNum );

	if ( target ) {
		trap_SendServerCommand( target - g_entities, str );

		// don't tell to the player self if it was already directed to this player
		// also don't send the chat back to a bot
		if ( ent && ent != target && !(ent->r.svFlags & SVF_BOT) ) {
			AP( str );
		}
		return;
	}

	// send to everyone on team
	if ( mode == SAY_TEAM && ent && ent->player ) {
		G_TeamCommand( ent->player->sess.sessionTeam, str );
		return;
	}

	// send it to all the appropriate clients
	for ( i = 0; i < level.maxconnections; i++ ) {
		for ( j = 0; j < MAX_SPLITVIEW; j++ ) {
			if ( level.connections[i].localPlayerNums[j] == -1 )
				continue;

			other = &g_entities[level.connections[i].localPlayerNums[j]];

			if ( !G_SayTo( ent, other, mode ) ) {
				break;
			}
		}

		if ( j == MAX_SPLITVIEW ) {
			trap_SendServerCommandEx( i, -1, str );
		}
	}
}

static void SanitizeChatText( char* text ) {
	int i;

	for ( i = 0; text[i]; i++ ) {
		if ( text[i] == '\n' || text[i] == '\r' ) {
			text[i] = ' ';
		}
	}
}


/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( gentity_t* ent, int mode, qboolean arg0 ) {
	char* p;

	if ( trap_Argc() < 2 && !arg0 ) {
		return;
	}

	if ( arg0 ) {
		p = ConcatArgs( 0 );
	} else {
		p = ConcatArgs( 1 );
	}

	SanitizeChatText( p );

	G_Say( ent, NULL, mode, p );
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f( gentity_t* ent ) {
	int			targetNum;
	gentity_t* target;
	char* p;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 3 ) {
		AP( "print \"Usage: tell <player id> <message>\n\"" );
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = PlayerNumberFromString( ent, arg, qtrue, qtrue );
	if ( targetNum == -1 ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target->inuse || !target->player ) {
		return;
	}

	p = ConcatArgs( 2 );

	SanitizeChatText( p );

	G_Say( ent, target, SAY_TELL, p );
}


static char* gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

static const int numgc_orders = ARRAY_LEN( gc_orders );

void Cmd_GameCommand_f( gentity_t* ent ) {
	int			targetNum;
	gentity_t* target;
	int			order;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 3 ) {
		AP( va( "print \"Usage: gc <player id> <order 0-%d>\n\"", numgc_orders - 1 ) );
		return;
	}

	trap_Argv( 2, arg, sizeof( arg ) );
	order = atoi( arg );

	if ( order < 0 || order >= numgc_orders ) {
		AP( va( "print \"Bad order: %i\n\"", order ) );
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = PlayerNumberFromString( ent, arg, qtrue, qtrue );
	if ( targetNum == -1 ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target->inuse || !target->player ) {
		return;
	}

	G_Say( ent, target, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t* ent ) {
	AP( va( "print \"%s\n\"", vtos( ent->r.currentOrigin ) ) );
}


/*
==================
Cmd_ReadyUp_f
==================
*/
void Cmd_ReadyUp_f( gentity_t* ent ) {
	gplayer_t* cl = ent->player;

	// must be during warmup
	if ( level.warmupTime >= 0 ) return;
	// readying must be enabled on the server
	if ( !g_doReady.integer ) return;
	// spectators cannot ready
	if ( cl->sess.sessionTeam == TEAM_SPECTATOR ) return;
	// connecting players are ignored
	if ( cl->pers.connected == CON_CONNECTING ) return;

	// check conditions for ready status
	switch ( level.warmupState ) {
	case WARMUP_DELAYED:
	case WARMUP_DEFAULT:
		if ( GTF(GTF_TEAMS) ) {
			CP( "print \"Players cannot ready up until teams are fully present.\n\"" );
		} else {
			CP( "print \"Cannot ready up until more players are present.\n\"" );
		}
		return;
	case WARMUP_SHORT_TEAMS:
	case WARMUP_IMBA:
		CP( va("print \"Players cannot ready up until teams are %s.\n\"", level.warmupState == WARMUP_SHORT_TEAMS ? "fully present" : "balanced" ) );
		return;
	case WARMUP_COUNTDOWN:
		CP( "print \"Readying has ended - match is starting.\n\"" );
		return;
	default: break;
	}

	cl->pers.readyToBegin ^= 1;

	AP( va( "cp \"%s is %s.\n\"", PlayerName( ent->player->pers ),
		(cl->pers.readyToBegin ? "Ready" : "Not Ready") ) );
}


/*
==================
Cmd_CallVote_f
==================
*/
#define CALLVOTE_ARG2_NONE 1
#define CALLVOTE_ARG2_INTREGAL 2
void Cmd_CallVote_f( gentity_t* ent ) {
	char* c;
	int		i;
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];
	int		arg2Flags, arg2RangeMin, arg2RangeMax;

	if ( !g_allowVote.integer ) {
		AP( "print \"Voting is not allowed here.\n\"" );
		return;
	}

	if ( level.voteTime ) {
		AP( "print \"A vote is already in progress.\n\"" );
		return;
	}
	if ( ent->player->pers.voteCount >= MAX_VOTE_COUNT ) {
		AP( "print \"You have called the maximum number of votes.\n\"" );
		return;
	}
	if ( ent->player->sess.sessionTeam == TEAM_SPECTATOR ) {
		AP( "print \"Not allowed to call a vote as spectator.\n\"" );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	trap_Argv( 2, arg2, sizeof( arg2 ) );

	// check for command separators in arg2
	for ( c = arg2; *c; ++c ) {
		switch ( *c ) {
		case '\n':
		case '\r':
		case ';':
			AP( "print \"Invalid vote string.\n\"" );
			return;
			break;
		}
	}

	arg2Flags = 0;
	arg2RangeMin = 0;
	arg2RangeMax = INT_MAX;

	if ( !Q_stricmp( arg1, "map_restart" ) ) {
		arg2Flags = CALLVOTE_ARG2_INTREGAL;
		arg2RangeMax = 60; // max 1 minute

		// default to 5 seconds if no argument was specified
		if ( !arg2[0] ) {
			Q_strncpyz( arg2, "5", sizeof( arg2 ) );
		}
	} else if ( !Q_stricmp( arg1, "nextMap" ) ) {
		arg2Flags = CALLVOTE_ARG2_NONE;
	} else if ( !Q_stricmp( arg1, "map" ) ) {
		// string
	} else if ( !Q_stricmp( arg1, "g_gameType" ) ) {
		arg2Flags = CALLVOTE_ARG2_INTREGAL;
		arg2RangeMax = GT_MAX_GAME_TYPE - 1;
	} else if ( !Q_stricmp( arg1, "kick" ) ) {
		// string
	} else if ( !Q_stricmp( arg1, "kickNum" ) ) {
		arg2Flags = CALLVOTE_ARG2_INTREGAL;
		arg2RangeMax = MAX_CLIENTS;
	} else if ( !Q_stricmp( arg1, "g_doWarmup" ) ) {
		arg2Flags = CALLVOTE_ARG2_INTREGAL;
		arg2RangeMax = 1;
	} else if ( !Q_stricmp( arg1, "timeLimit" ) ) {
		arg2Flags = CALLVOTE_ARG2_INTREGAL;
		arg2RangeMax = 240; // 4 hours
	} else if ( !Q_stricmp( arg1, "scoreLimit" ) ) {
		arg2Flags = CALLVOTE_ARG2_INTREGAL;
		arg2RangeMax = 999;
	} else if ( !Q_stricmp( arg1, "g_instaGib" ) ) {
		arg2Flags = CALLVOTE_ARG2_INTREGAL;
		arg2RangeMax = 1;
	} else {
		AP( "print \"Invalid vote string.\n\"" );
		AP( "print \"Vote commands are: map_restart, nextMap, map <mapname>, g_gameType <n>, kick <player>, kickNum <playernum>, g_doWarmup <boolean>, timeLimit <time>, scoreLimit <frags>, g_instaGib <boolean>.\n\"" );
		return;
	}

	if ( arg2Flags & CALLVOTE_ARG2_NONE ) {
		if ( arg2[0] != '\0' ) {
			AP( va( "print \"Vote command %s does not accept an argument.\n\"", arg1 ) );
			return;
		}
	} else if ( arg2[0] == '\0' ) {
		AP( va( "print \"Vote command %s requires an argument.\n\"", arg1 ) );
		return;
	}

	if ( arg2Flags & CALLVOTE_ARG2_INTREGAL ) {
		if ( !StringIsInteger( arg2 ) ) {
			AP( va( "print \"Vote command %s argument must be a number.\n\"", arg1 ) );
			return;
		}

		i = atoi( arg2 );
		if ( i < arg2RangeMin || i > arg2RangeMax ) {
			AP( va( "print \"Vote command %s argument must be %d to %d.\n\"", arg1, arg2RangeMin, arg2RangeMax ) );
			return;
		}
	}

	// if there is still a vote to be executed
	if ( level.voteExecuteTime ) {
		// don't start a vote when map change or restart is in progress
		if ( !Q_stricmpn( level.voteString, "map", 3 )
			|| !Q_stricmpn( level.voteString, "nextmap", 7 ) ) {
			AP( "print \"Vote after map change.\n\"" );
			return;
		}

		level.voteExecuteTime = 0;
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "%s\n", level.voteString ) );
	}

	// special case for g_gameType, check for bad values
	if ( !Q_stricmp( arg1, "g_gameType" ) ) {
		i = atoi( arg2 );
		if ( i == GT_CAMPAIGN || i < 0 || i >= GT_MAX_GAME_TYPE ) {
			AP( "print \"Invalid gametype.\n\"" );
			return;
		}

		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %d", arg1, i );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", arg1, gt[i].longName );
	} else if ( !Q_stricmp( arg1, "map" ) ) {
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char	s[MAX_STRING_CHARS];
		char	filename[MAX_QPATH];

		if ( strstr( arg2, ".." ) || strstr( arg2, "::" ) || strlen( arg2 ) + 9 >= MAX_QPATH ) {
			AP( "print \"Invalid map name.\n\"" );
			return;
		}
		Com_sprintf( filename, sizeof( filename ), "maps/%s.bsp", arg2 );
		if ( trap_FS_FOpenFile( filename, NULL, FS_READ ) <= 0 ) {
			AP( "print \"Map not found.\n\"" );
			return;
		}

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof( s ) );
		if ( *s ) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s; set nextmap \"%s\"", arg1, arg2, s );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
		}
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	} else if ( !Q_stricmp( arg1, "nextmap" ) ) {
		char	s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof( s ) );
		if ( !*s ) {
			AP( "print \"nextmap not set.\n\"" );
			return;
		}
		Com_sprintf( level.voteString, sizeof( level.voteString ), "vstr nextmap" );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	} else if ( !Q_stricmp( arg1, "kicknum" ) || !Q_stricmp( arg1, "kick" ) ) {
		i = PlayerNumberFromString( ent, arg2, !Q_stricmp( arg1, "kicknum" ), !Q_stricmp( arg1, "kick" ) );
		if ( i == -1 ) {
			return;
		}

		if ( level.players[i].pers.localClient ) {
			AP( "print \"Cannot kick host client.\n\"" );
			return;
		}

		Com_sprintf( level.voteString, sizeof( level.voteString ), "kicknum %d", i );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "kick %s", level.players[i].pers.netname );
	} else {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	}

	G_LogPrintf( "callVote: %s: %s\n", ent->player->pers.netname, level.voteDisplayString );

	AP( va( "print \"%s called a vote.\n\"", PlayerName( ent->player->pers ) ) );

	// start the voting, the caller automatically votes yes
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;

	for ( i = 0; i < level.maxplayers; i++ ) {
		level.players[i].ps.eFlags &= ~EF_VOTED;
	}
	ent->player->ps.eFlags |= EF_VOTED;

	trap_SetConfigstring( CS_VOTE_TIME, va( "%i", level.voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING, level.voteDisplayString );
	trap_SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes ) );
	trap_SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo ) );
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t* ent ) {
	char		msg[64];

	if ( !level.voteTime ) {
		AP( "print \"No vote in progress.\n\"" );
		return;
	}
	if ( ent->player->ps.eFlags & EF_VOTED ) {
		AP( "print \"Vote already cast.\n\"" );
		return;
	}
	if ( ent->player->sess.sessionTeam == TEAM_SPECTATOR ) {
		AP( "print \"Not allowed to vote as spectator.\n\"" );
		return;
	}

	AP( "print \"Vote cast.\n\"" );

	ent->player->ps.eFlags |= EF_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( tolower( msg[0] ) == 'y' || msg[0] == '1' ) {
		level.voteYes++;
		trap_SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes ) );
		G_LogPrintf( "vote: %s: yes\n", ent->player->pers.netname );
	} else {
		level.voteNo++;
		trap_SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo ) );
		G_LogPrintf( "vote: %s: no\n", ent->player->pers.netname );
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}


/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t* ent ) {
	vec3_t		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( !g_cheats.integer ) {
		CP( va( "print \"Cheats are not enabled on this server.\n\"" ) );
		return;
	}

	if ( trap_Argc() < 4 || trap_Argc() > 6 ) {
		CP( va( "print \"Usage: setViewPos <x> <y> <z> [pitch] [yaw]\n\"" ) );
		return;
	}

	VectorClear( angles );
	for ( i = 0; i < 3; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	if ( trap_Argc() > 4 ) {
		trap_Argv( 4, buffer, sizeof( buffer ) );
		angles[PITCH] = atof( buffer );
	}
	if ( trap_Argc() > 5 ) {
		trap_Argv( 5, buffer, sizeof( buffer ) );
		angles[YAW] = atof( buffer );
	}
	/*
	if ( trap_Argc() > 6 ) {
		trap_Argv( 6, buffer, sizeof(buffer) );
		angles[ROLL] = atof( buffer );
	}
	*/

	TeleportPlayer( ent, origin, angles, qtrue, qfalse, qfalse );
}


/*
=================
ClientCommand
=================
*/
void ClientCommand( int connectionNum ) {
	gentity_t* ent;
	gconnection_t* connection;
	int		playerNum;
	char	cmd[MAX_TOKEN_CHARS];

	connection = &level.connections[connectionNum];

	trap_Argv( 0, cmd, sizeof( cmd ) );

	playerNum = connection->localPlayerNums[0];

	ent = g_entities + playerNum;
	if ( !ent->player || ent->player->pers.connected != CON_CONNECTED ) {
		return;		// not fully in game yet
	}

	if ( Q_stricmp( cmd, "say" ) == 0 ) {
		Cmd_Say_f( ent, SAY_ALL, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "say_team" ) == 0 ) {
		Cmd_Say_f( ent, SAY_TEAM, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "tell" ) == 0 ) {
		Cmd_Tell_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "score" ) == 0 ) {
		Cmd_Score_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "pwstat" ) == 0 ) {
		G_SendWeaponsPlayerStats( ent->player, playerNum );
		return;
	}

	// ignore all other commands when at intermission
	if ( level.intermissiontime ) {
		trap_SendServerCommand( playerNum, va( "print \"Command not allowed during intermission: %s\n\"", cmd ) );
		return;
	}

	if ( Q_stricmp( cmd, "give" ) == 0 )
		Cmd_Give_f( ent );
	else if ( Q_stricmp( cmd, "god" ) == 0 )
		Cmd_God_f( ent );
	else if ( Q_stricmp( cmd, "noTarget" ) == 0 )
		Cmd_Notarget_f( ent );
	else if ( Q_stricmp( cmd, "noClip" ) == 0 )
		Cmd_Noclip_f( ent );
	else if ( Q_stricmp( cmd, "kill" ) == 0 )
		Cmd_Kill_f( ent );
	else if ( Q_stricmp( cmd, "levelShot" ) == 0 )
		Cmd_LevelShot_f( ent );
	else if ( Q_stricmp( cmd, "follow" ) == 0 )
		Cmd_Follow_f( ent );
	else if ( Q_stricmp( cmd, "followNext" ) == 0 )
		Cmd_FollowCycle_f( ent, 1 );
	else if ( Q_stricmp( cmd, "followPrevious" ) == 0 )
		Cmd_FollowCycle_f( ent, -1 );
	else if ( Q_stricmp( cmd, "team" ) == 0 )
		Cmd_Team_f( ent );
	else if ( Q_stricmp( cmd, "where" ) == 0 )
		Cmd_Where_f( ent );
	else if ( !Q_stricmp( cmd, "readyUp" ) )
		Cmd_ReadyUp_f( ent );
	else if ( Q_stricmp( cmd, "callVote" ) == 0 )
		Cmd_CallVote_f( ent );
	else if ( Q_stricmp( cmd, "vote" ) == 0 )
		Cmd_Vote_f( ent );
	else if ( Q_stricmp( cmd, "gc" ) == 0 )
		Cmd_GameCommand_f( ent );
	else if ( Q_stricmp( cmd, "setViewPos" ) == 0 )
		Cmd_SetViewpos_f( ent );
	else if ( Q_stricmp( cmd, "drop" ) == 0 )
		Cmd_DropItem_f( ent );
	if ( Q_stricmp( cmd, "spawn" ) == 0 )
		Cmd_SpawnItem_f( ent );
	else
		trap_SendServerCommand( playerNum, va( "print \"Unknown command: %s\n\"", cmd ) );
}
