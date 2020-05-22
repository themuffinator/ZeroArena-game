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
// g_local.h -- local definitions for game module

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "g_public.h"
#include "ai_ea.h"

//==================================================================

#define BODY_QUEUE_SIZE		128			//64

#define SPAWNPOINT_DROPDIST		128

#define	FRAMETIME			100					// msec
#define	CARNAGE_REWARD_TIME	3000
#define REWARD_SPRITE_TIME	2000

#define	INTERMISSION_DELAY_TIME	1000
#define	SP_INTERMISSION_DELAY_TIME	5000

// gentity->flags
#define	FL_GODMODE				0x00000010
#define	FL_NOTARGET				0x00000020
#define	FL_TEAMSLAVE			0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK			0x00000800
//#define FL_DROPPED_ITEM			0x00001000
#define FL_NO_BOTS				0x00002000	// spawn point not for bot use
#define FL_NO_HUMANS			0x00004000	// spawn point just for bots
#define FL_FORCE_GESTURE		0x00008000	// force gesture on player

//wolfet
#define AP( x ) trap_SendServerCommand( -1, x )                 // Print to all
#define CP( x ) trap_SendServerCommand( ent - g_entities, x )     // Print to an ent
#define CPx( x, y ) trap_SendServerCommand( x, y )              // Print to id = x
//-wolfet

//muff
//#define GT( x ) gt[g_gameType.integer].gtFlags & (x)
#define NotGT( x ) gt[g_gameType.integer].gtFlags & ~(x)
#define GTx( x, y ) gt[x].gtFlags & (y)
//-muff

// movers are things like doors, plats, buttons, etc
typedef enum {
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1,
//muff: rotating doors
	ROTATOR_POS1,
	ROTATOR_POS2,
	ROTATOR_1TO2,
	ROTATOR_2TO1,
//-muff
//muff: trains
	//TRAIN_TOP,
//-muff
} moverState_t;

#define SP_PODIUM_MODEL		"models/mapobjects/podium/podium4.md3"

//============================================================================

typedef struct gentity_s gentity_t;
typedef struct gplayer_s gplayer_t;

struct gentity_s {
	entityShared_t	r;				// shared by both the server system and game
	entityState_t	s;				// communicated by server to clients

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	struct gplayer_s	*player;			// NULL if not a player

	qboolean	inuse;

	char		*classname;			// set in QuakeEd
	int			spawnflags;			// set in QuakeEd

	qboolean	neverFree;			// if true, FreeEntity will only unlink
									// bodyqueue uses this

	int			flags;				// FL_* variables

	char		*model;
	char		*model2;
	int			freetime;			// level.time when the object was freed
	
	int			eventTime;			// events will be cleared EVENT_VALID_MSEC after set
	qboolean	freeAfterEvent;
	qboolean	unlinkAfterEvent;

	qboolean	physicsObject;		// if true, it can be pushed by movers and fall off edges
									// all game items are physicsObjects, 
	float		physicsBounce;		// 1.0 = continuous bounce, 0.0 = no bounce
	int			clipmask;			// brushes with this content value will be collided against
									// when moving.  items and corpses do not collide against
									// players, for instance

	// movers
	moverState_t moverState;
	int			soundPos1;
	int			sound1to2;
	int			sound2to1;
	int			soundPos2;
	int			soundLoop;
	gentity_t	*parent;
	gentity_t	*nextTrain;
	gentity_t	*prevTrain;
	vec3_t		pos1, pos2;

	char		*message;

	int			timestamp;		// body queue sinking, etc

	char		*target;
	char		*targetname;
	char		*team;
	char		*targetShaderName;
	char		*targetShaderNewName;
	gentity_t	*target_ent;

	float		speed;
	vec3_t		movedir;

	int			nextthink;
	void		(*think)(gentity_t *self);
	void		(*reached)(gentity_t *self);	// movers call this when hitting endpoint
	void		(*blocked)(gentity_t *self, gentity_t *other);
	void		(*touch)(gentity_t *self, gentity_t *other, trace_t *trace);
	void		(*use)(gentity_t *self, gentity_t *other, gentity_t *activator);
	void		(*pain)(gentity_t *self, gentity_t *attacker, int damage);
	void		(*die)(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, int damage, int mod);	//muff: add dir for directional gibs
	qboolean	(*snapshotCallback)(gentity_t *self, gentity_t *player);

	int			pain_debounce_time;
	int			fly_sound_debounce_time;	// wind tunnel

	int			health;

	qboolean	takedamage;

	int			damage;
	int			splashDamage;	// quad will increase this without increasing radius
	int			splashRadius;
	int			methodOfDeath;
	int			splashMethodOfDeath;

	int			count;

	gentity_t	*chain;
	gentity_t	*enemy;
	gentity_t	*activator;
	gentity_t	*teamchain;		// next entity in team
	gentity_t	*teammaster;	// master of the team

#ifdef MISSIONPACK
	int			kamikazeTime;
	int			kamikazeShockTime;
#endif

	int			watertype;
	int			waterlevel;

	int			noise_index;

	// timing variables
	float		wait;
	float		random;

	gitem_t		*item;			// for bonus items

//muff: rotating doors
	float		distance;
//-muff

//q2
	float		delay;
	char		*killtarget;
	char		*pathtarget;

	char		*sitem;			// for trigger_key

	float		volume;			// for target_speaker
	int			attenuation;	// for target_speaker
//-q2

	// dlights
	vec3_t		dl_color;
	char		*dl_stylestring;
	char		*dl_shader;
	int			dl_atten;

	// info for bots
	qboolean	botvalid;
	float		update_time;
	float		ltime;
	vec3_t		visorigin;
	vec3_t		lastvisorigin;
	vec3_t		lastAngles;
	vec3_t		lastMins;
	vec3_t		lastMaxs;
	int			areanum;
};


typedef enum {
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

typedef enum {
	SPECTATOR_NOT,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW,
	SPECTATOR_SCOREBOARD
} spectatorState_t;

typedef enum {
	TEAM_BEGIN,		// Beginning a team game, spawn at base
	TEAM_ACTIVE		// Now actively playing
} playerTeamStateState_t;

typedef struct {
	playerTeamStateState_t	state;

	int			location;

	float		lasthurtcarrier;
	float		lastreturnedflag;
	float		lastfraggedcarrier;
} playerTeamState_t;

// player data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
typedef struct {
	team_t		sessionTeam;
	team_t		sessionPlayState;
	qboolean	queued;			// client is queuing in tourney
	int			queueNum;		// client number in queue to play in tourney
	spectatorState_t	spectatorState;
	int			spectatorPlayer;	// for chasecam and follow mode
	int			wins, losses;		// tournament stats
	qboolean	teamLeader;			// true when this player is a team leader
} playerSession_t;

//
#define MAX_NETNAME			36
#define MAX_NETCLAN			16
#define	MAX_VOTE_COUNT		3

// player data that stays across multiple respawns, but is cleared
// on each level change or team change at PlayerBegin()
typedef struct {
	int			connectionNum;		// index in level.connections
	int			localPlayerNum;		// client's local player number in range of 0 to MAX_SPLITVIEW-1
	clientConnected_t	connected;	
	usercmd_t	cmd;				// we would lose angles if not persistant
	qboolean	localClient;		// true if "ip" info key is "localhost"
	qboolean	initialSpawn;		// the first spawn should be at a cool location
	qboolean	predictItemPickup;	// based on cg_predictItems userinfo
	qboolean	pmoveFixed;			//
	int			antiLag;			// based on cg_antiLag userinfo
	char		netname[MAX_NETNAME];
	char		netclan[MAX_NETNAME];
	int			maxHealth;			// for handicapping
	int			enterTime;			// level.time the player entered the game
	playerTeamState_t teamState;	// status in teamplay games
	int			voteCount;			// to prevent people from constantly calling votes
	qboolean	teamInfo;			// send team overlay updates?
//muff
	qboolean	readyToBegin;		// ready to start match
	int			handedness;			// cg_drawGun value for muzzle origin
//-muff
} playerPersistant_t;

#define MAX_PLAYER_MARKERS 17

typedef struct {
	vec3_t		mins;
	vec3_t		maxs;
	vec3_t		origin;
	int			time;
} playerMarker_t;

// this structure is cleared on each PlayerSpawn(),
// except for 'player->pers' and 'player->sess'
struct gplayer_s {
	// ps MUST be the first element, because the server expects it
	playerState_t	ps;				// communicated by server to clients

	// the rest of the structure is private to game
	playerPersistant_t	pers;
	playerSession_t		sess;

	qboolean	readyToExit;		// wishes to leave the intermission

	qboolean	noClip;

	// history for backward reconcile
	int			topMarker;
	playerMarker_t	playerMarkers[MAX_PLAYER_MARKERS];
	playerMarker_t	backupMarker;

	int			frameOffset;		// an approximation of the actual server time we received this
									// command (not in 50ms increments)

	int			lastCmdServerTime;	// ucmd.serverTime from last usercmd_t

	int			lastCmdTime;		// level.time of last usercmd_t, for EF_CONNECTION
									// we can't just use pers.lastCommand.time, because
									// of the g_sycronousclients case
	int			buttons;
	int			oldbuttons;
	int			latched_buttons;

	vec3_t		oldOrigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int			damage_armor;		// damage absorbed by armor
	int			damage_blood;		// damage taken out of health
	int			damage_knockback;	// impact damage
	vec3_t		damage_from;		// origin for vector calculation
	qboolean	damage_fromWorld;	// if true, don't use the damage_from vector

	int			accurateCount;		// for "impressive" reward sound
#if 0
	int			statsWeaponShots[WP_NUM_WEAPONS];		// total number of shots per weapon
	int			statsWeaponHits[WP_NUM_WEAPONS];		// total number of hits per weapon
	int			statsWeaponDmgD[WP_NUM_WEAPONS];		// total damage dealt per weapon
	int			statsWeaponDmgR[WP_NUM_WEAPONS];		// total damage received per weapon
#endif
	//
	int			lastkilled_player;	// last player that this player killed
	int			lasthurt_player;	// last player that damaged this player
	int			lasthurt_mod;		// type of damage the player did

	// timers
	int			respawnTime;		// can respawn when time > this, force after g_forcerespwan
	int			respawnWishTime;	// player wants to respawn, bounded by min respawn delay
	int			inactivityTime;		// kick players when time > this
	qboolean	inactivityWarning;	// qtrue if the five seoond warning has been given
	int			rewardTime;			// clear the EF_AWARD_IMPRESSIVE, etc when time > this

	int			airOutTime;
//muff
	int			treasonDmg;
//-muff

	int			lastKillTime;		// for multiple kill rewards

	qboolean	fireHeld;			// used for hook
	gentity_t	*hook;				// grapple hook if out

	int			switchTeamTime;		// time the player switched teams

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int			timeResidual;

	gentity_t	*persistantPowerup;
	int			ammoTimes[WP_NUM_WEAPONS];
#ifdef MISSIONPACK
	int			portalID;
	int			invulnerabilityTime;
#endif

	char		*areabits;
};


// A single client can have multiple players, for splitscreen.
typedef struct gconnection_s {
	int			numLocalPlayers;				// for quick access, the players could be any indexes in localPlayers[].
	int			localPlayerNums[MAX_SPLITVIEW];
} gconnection_t;


//
// this structure is cleared as each map is entered
//
#define	MAX_SPAWN_VARS			64
#define	MAX_SPAWN_VARS_CHARS	4096

typedef struct {
	struct gplayer_s	*players;		// [maxplayers]

	struct gentity_s	*gentities;
	int			gentitySize;
	int			num_entities;		// MAX_CLIENTS <= num_entities <= ENTITYNUM_MAX_NORMAL

	gconnection_t	*connections;

	int			warmupTime;			// restart match at this time

	fileHandle_t	logFile;

	// store latched cvars here that we want to get at often
	int			maxplayers;
	int			maxconnections;

	int			framenum;
	int			time;					// in msec
	int			previousTime;			// so movers can back up when blocked

	int			startTime;				// level.time the map was started

	int			frameStartTime;			// time this server frame started

	int			teamScores[TEAM_NUM_TEAMS];
	int			lastTeamLocationTime;		// last time of client team location update

	qboolean	newSession;				// don't use any old session data, because
										// we changed gametype

	qboolean	restarted;				// waiting for a map_restart to fire

	int			numConnectedPlayers;
	int			numNonSpectatorPlayers;	// includes connecting players
	int			numQueuedPlayers;	// includes connecting players
	int			numPlayingPlayers;		// connected, non-spectators
	int			sortedPlayers[MAX_CLIENTS];		// sorted by score
	int			follow1, follow2;		// playerNums for auto-follow spectators

	int			snd_fry;				// sound index for standing in lava

	int			warmupState;				// warmup checks for validating match starting
	int			warmupOldState;				// check for changes in warmup state
	int			warmupVal;					// additional warmup info to send clients
	int			warmupOldVal;				// check for changes in warmup value

	int			warmupModificationCount;	// for detecting if g_warmupCountdownTime is changed
	int			botReportModificationCount;

	// voting state
	char		voteString[MAX_STRING_CHARS];
	char		voteDisplayString[MAX_STRING_CHARS];
	int			voteTime;				// level.time vote was called
	int			voteExecuteTime;		// time the vote is executed
	int			voteYes;
	int			voteNo;
	int			numVotingPlayers;		// set by CalculateRanks

	// spawn variables
	qboolean	spawning;				// the G_Spawn*() functions are valid
	int			numSpawnVars;
	char		*spawnVars[MAX_SPAWN_VARS][2];	// key / value pairs
	int			numSpawnVarChars;
	char		spawnVarChars[MAX_SPAWN_VARS_CHARS];
	int			spawnEntityOffset;

	// intermission state
	int			intermissionQueued;		// intermission was qualified, but
										// wait INTERMISSION_DELAY_TIME before
										// actually going there so the last
										// frag can be watched.  Disable future
										// kills during this delay
	int			intermissiontime;		// time the intermission was started
	char		*changemap;
	qboolean	readyToExit;			// at least one player wants to exit
	int			exitTime;
	vec3_t		intermission_origin;	// also used for spectator spawns
	vec3_t		intermission_angle;

	qboolean	locationLinked;			// target_locations get linked
	gentity_t	*locationHead;			// head of the location list
	int			bodyQueueIndex;			// dead bodies
	gentity_t	*bodyQueue[BODY_QUEUE_SIZE];
#ifdef MISSIONPACK
	int			portalSequence;
#endif

	//multiteam
	//int			flagCarrier[TEAM_NUM_TEAMS];	// client num for each flag's carrier
	int			map_teamBaseSpawns;		// detect which teams are supported in team base maps
	int			map_teamBaseCount;		// return a count of the above
	int			teams_max;				// maximum number of teams in team gametypes
//q2
	int			found_secrets;
	int			total_secrets;
//-q2

//muff
	float		miscTimer[TEAM_NUM_TEAMS];		// for gametype specific timing - flag capture times etc etc
	float		miscNum[TEAM_NUM_TEAMS];		// for gametype specific use - flag capture times etc etc

	int			overTime;
//-muff
	//map
	int			mapWeapons;

	// team checks
	int			sortedTeams[TEAM_NUM_TEAMS];	// teams sorted by score, always ignore 0 as it is TEAM_FREE
	int			numPlayingTeams;				// number of teams still with players
	int			numTeamPlayers[TEAM_NUM_TEAMS];	// count number of players per team
	int			shortTeams;						// bit flag of teams with less players than minimum
	int			numShortTeams;					// number of teams with less players than minimum
	int			smallestTeamCount;
	int			largestTeamCount;

	// checking setting changes requiring map_restart
	int			initGameType;					// gametype we have initialized
	qboolean	initRestart;					// just had a map_restart on same map

	// tourney queuing
	int			tourneyQueueEnd;
} level_locals_t;


//
// g_spawn.c
//
qboolean	G_SpawnString( const char *key, const char *defaultString, char **out );
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean	G_SpawnFloat( const char *key, const char *defaultString, float *out );
qboolean	G_SpawnInt( const char *key, const char *defaultString, int *out );
qboolean	G_SpawnVector( const char *key, const char *defaultString, float *out );
void		G_SpawnEntitiesFromString( void );
char *G_NewString( const char *string );

//
// g_cmds.c
//
void Cmd_Score_f (gentity_t *ent);
void StopFollowing( gentity_t *ent );
void BroadcastTeamChange( gplayer_t *player, team_t oldTeam, const qboolean forfeit );
void SetTeam( gentity_t *ent, const char *s, const qboolean forfeit );
void Cmd_FollowCycle_f( gentity_t *ent, int dir );
void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText );

//
// g_items.c
//
void G_CheckTeamItems( void );
void G_RunItem( gentity_t *ent );
void RespawnItem( gentity_t *ent );

void UseHoldableItem( gentity_t *ent );
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle );
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity );
void G_SpawnItem (gentity_t *ent, gitem_t *item);
void FinishSpawningItem( gentity_t *ent );
void Add_Ammo (gentity_t *ent, int weapon, int count);
void Touch_Item (gentity_t *ent, gentity_t *other, trace_t *trace);

void ClearRegisteredItems( void );
void RegisterItem( gitem_t *item );
void SaveRegisteredItems( void );

//
// g_utils.c
//
int		G_FindConfigstringIndex( char *name, int start, int max, qboolean create );
int		G_ModelIndex( char *name );
int		G_SoundIndex( char *name );
void	trap_SendServerCommand( int playerNum, char *cmd );
void	G_TeamCommand( team_t team, char *cmd );
void	G_KillBox (gentity_t *ent);
gentity_t *G_Find (gentity_t *from, int fieldofs, const char *match);
gentity_t *G_PickTarget (char *targetname);
void	G_UseTargets (gentity_t *ent, gentity_t *activator);
void	G_SetBrushModel( gentity_t *ent, const char *name );
void	G_SetMovedir ( vec3_t angles, vec3_t movedir);

void	G_InitGentity( gentity_t *e );
gentity_t	*G_Spawn (void);
gentity_t *G_TempEntity( vec3_t origin, int event );
void	G_Sound( gentity_t *ent, int channel, int soundIndex );
void	G_FreeEntity( gentity_t *e );
qboolean	G_EntitiesFree( void );

void	G_TouchTriggers (gentity_t *ent);

float	*tv (float x, float y, float z);
char	*vtos( const vec3_t v );

float vectoyaw( const vec3_t vec );

void G_AddPredictableEvent( gentity_t *ent, int event, int eventParm );
void G_AddEvent( gentity_t *ent, int event, int eventParm );
void G_SetOrigin( gentity_t *ent, vec3_t origin );
void AddRemap(const char *oldShader, const char *newShader, float timeOffset);
const char *BuildShaderStateConfig( void );
extern qboolean GTF(const int gtFlags);
extern qboolean GTL(const int gtGoal);

extern char* g_teamNames[TEAM_NUM_TEAMS];
extern char* g_teamNamesLower[TEAM_NUM_TEAMS];
extern char* g_teamNamesLetter[TEAM_NUM_TEAMS];
extern char* g_teamShortNames[TEAM_NUM_TEAMS];
extern char* G_TeamName( team_t team );
extern qboolean	IsValidTeam( const team_t team );
extern char* PlayerName( playerPersistant_t p );
char* G_PlayerTeamName( team_t team );
int G_WeaponsTotalAccuracy( gplayer_t* cl );
void G_ClearMedals( playerState_t* ps );
void G_DropEntityToFloor( gentity_t* ent, const int traceDistance );
char* replace( const char* s, const char* old, const char* new );

//
// g_combat.c
//
qboolean CanDamage (gentity_t *targ, vec3_t origin);
void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod, weapon_t weapon );
qboolean G_RadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, const int dFlags, const int mod, const weapon_t weapon );
int G_InvulnerabilityEffect( gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir );
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, int damage, int meansOfDeath );
void TossPlayerItems( gentity_t *self );
void TossPlayerGametypeItems( gentity_t *self );
#ifdef MISSIONPACK
void ResetPlayerRune( gentity_t *self );
#endif
void TossPlayerSkulls( gentity_t *self );

//
// g_missile.c
//
void G_RunMissile( gentity_t *ent );

gentity_t *fire_plasma (gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_grenade (gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_rocket (gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_bfg (gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_grapple (gentity_t *self, vec3_t start, vec3_t dir);
#ifdef MISSIONPACK
gentity_t *fire_nail( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up );
gentity_t *fire_prox( gentity_t *self, vec3_t start, vec3_t aimdir );
#endif
void fire_blaster( gentity_t* self, vec3_t start, vec3_t dir, int damage, int speed );

//
// g_mover.c
//
void G_RunMover( gentity_t *ent );
void Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace );

//
// g_trigger.c
//
void trigger_teleporter_touch (gentity_t *self, gentity_t *other, trace_t *trace );
void AimAtTarget( gentity_t* self );

//
// g_misc.c
//
void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles, const qboolean freezeVelocity, const qboolean saveAngles, const qboolean effect );
#ifdef MISSIONPACK
void DropPortalSource( gentity_t *ent );
void DropPortalDestination( gentity_t *ent );
#endif


//
// g_weapon.c
//
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker );
void CalcMuzzlePoint ( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint );
qboolean CheckGauntletAttack( gentity_t *ent );
void Weapon_HookFree (gentity_t *ent);
void Weapon_HookThink (gentity_t *ent);


//
// g_client.c
//
int TeamCount( int ignorePlayerNum, team_t team );
int TeamLeader( int team );
team_t PickTeam( int ignorePlayerNum );
void SetPlayerViewAngle( gentity_t *ent, vec3_t angle );
gentity_t *SelectSpawnPoint (vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot);
void CopyToBodyQueue( gentity_t *ent );
void PlayerRespawn(gentity_t *ent);
void BeginIntermission (void);
void InitBodyQueue (void);
void PlayerSpawn( gentity_t *ent );
void player_die (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, int damage, int mod);
void AddScore( gentity_t *ent, vec3_t origin, int score );
void CountPopulation( void );
void CalculateRanks( void );
qboolean SpotWouldTelefrag( gentity_t *spot );

//
// g_unlagged.c
//
void G_ResetHistory( gentity_t *ent );
void G_StoreHistory( gentity_t *ent );
void G_TimeShiftAllClients( int time, gentity_t *skip );
void G_UnTimeShiftAllClients( gentity_t *skip );
void G_DoTimeShiftFor( gentity_t *ent );
void G_UndoTimeShiftFor( gentity_t *ent );
void G_UnTimeShiftClient( gentity_t *client );
void G_PredictPlayerMove( gentity_t *ent, float frametime );

//
// g_svcmds.c
//
qboolean	G_ConsoleCommand( void );
qboolean	G_ConsoleCompleteArgument( int completeArgument );
void G_RegisterCommands( void );
void G_ProcessIPBans(void);
qboolean G_FilterPacket (char *from);

//
// g_weapon.c
//
void FireWeapon( gentity_t *ent );
#ifdef MISSIONPACK
void G_StartKamikaze( gentity_t *ent );
#endif

//
// g_cmds.c
//
void DeathmatchScoreboardMessage( gentity_t *ent );
char *ConcatArgs( int start );
qboolean StringIsInteger( const char * s );

//
// g_main.c
//
void MovePlayerToIntermission( gentity_t *ent );
void FindIntermissionPoint( void );
void SetLeader(int team, int player);
void CheckTeamLeader( int team );
void G_RunThink (gentity_t *ent);
void Tournament_RemoveFromQueue( gplayer_t* player, const qboolean silent );
void Tournament_AddToQueue( gplayer_t *player, const qboolean silent );
void QDECL G_LogPrintf( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
void SendScoreboardMessageToAllClients( void );
void QDECL G_DPrintf( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
void QDECL G_Printf( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
void QDECL G_Error( const char *fmt, ... ) __attribute__ ((noreturn, format (printf, 1, 2)));

//
// g_client.c
//
char *PlayerConnect( int playerNum, qboolean firstTime, qboolean isBot, int connectionNum, int localPlayerNum );
void PlayerUserinfoChanged( int playerNum );
qboolean PlayerDisconnect( int playerNum, qboolean force );
void PlayerBegin( int playerNum );
void ClientCommand( int connectionNum );
float PlayerHandicap( gplayer_t *player );

//
// g_active.c
//
void PlayerThink( int playerNum );
void PlayerEndFrame( gentity_t *ent );
void G_RunPlayer( gentity_t *ent );

//
// g_team.c
//
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 );
void Team_CheckDroppedItem( gentity_t *dropped );
qboolean CheckObeliskAttack( gentity_t *obelisk, gentity_t *attacker );

//
// g_session.c
//
void G_ReadSessionData( gplayer_t *player );
void G_InitSessionData( gplayer_t *player, char *userinfo );

void G_InitWorldSession( void );
void G_WriteSessionData( void );

//
// g_arenas.c
//
void SendSPPostGameInfo( void );
void SpawnModelsOnVictoryPads( void );
void Svcmd_AbortPodium_f( void );

//
// g_bot.c
//
void G_InitBots( qboolean restart );
char *G_GetBotInfoByNumber( int num );
char *G_GetBotInfoByName( const char *name );
void G_CheckBotSpawn( void );
void G_RemoveQueuedBotBegin( int playerNum );
qboolean G_BotConnect( int playerNum, qboolean restart );
void Svcmd_AddBot_f( void );
void Svcmd_AddBotComplete( char *args, int argNum );
void Svcmd_BotList_f( void );
void BotInterbreedEndMatch( void );

//
// g_botlib.c
//
void G_BotInitBotLib(void);

//
// ai_main.c
//
#define MAX_FILEPATH			144

//bot settings
typedef struct bot_settings_s
{
	char characterfile[MAX_FILEPATH];
	float skill;
} bot_settings_t;

int BotAISetup( int restart );
int BotAIShutdown( int restart );
int BotAILoadMap( int restart );
int BotAISetupPlayer(int playernum, struct bot_settings_s *settings, qboolean restart);
int BotAIShutdownPlayer( int playernum, qboolean restart );
int BotAIStartFrame( int time );
void BotTestAAS(vec3_t origin);
void Svcmd_BotTeamplayReport_f( void );

#include "g_team.h" // teamplay specific stuff
#include "g_syscalls.h"

extern	level_locals_t	level;
extern	gentity_t		g_entities[MAX_GENTITIES];

#define	FOFS(x) ((size_t)&(((gentity_t *)0)->x))

extern	vmCvar_t	g_allowVote;
extern	vmCvar_t	g_banIPs;
extern	vmCvar_t	g_blueTeamName;
extern	vmCvar_t	g_cheats;
extern	vmCvar_t	g_debugDamage;
extern	vmCvar_t	g_debugMove;
extern	vmCvar_t	g_dedicated;
extern	vmCvar_t	g_dmFlags;
extern	vmCvar_t	g_doWarmup;
extern	vmCvar_t	g_filterBan;
extern	vmCvar_t	g_friendlyFire;
extern	vmCvar_t	g_gameType;
extern	vmCvar_t	g_gravity;
extern	vmCvar_t	g_greenTeamName;
extern	vmCvar_t	g_harvester_skullTimeout;
extern	vmCvar_t	g_inactivity;
extern	vmCvar_t	g_instaGib;
extern	vmCvar_t	g_knockback;
extern	vmCvar_t	g_maxClients;			// allow this many total, including spectators
extern	vmCvar_t	g_maxGameClients;		// allow this many active
extern	vmCvar_t	g_motd;
extern	vmCvar_t	g_needPassword;
extern	vmCvar_t	g_obeliskHealth;
extern	vmCvar_t	g_obeliskRegenAmount;
extern	vmCvar_t	g_obeliskRegenPeriod;
extern	vmCvar_t	g_obeliskRespawnDelay;
extern	vmCvar_t	g_password;
extern	vmCvar_t	g_pinkTeamName;
extern	vmCvar_t	g_playerCapsule;
extern	vmCvar_t	g_proxMineTimeout;
extern	vmCvar_t	g_quadFactor;
extern	vmCvar_t	g_rankings;
extern	vmCvar_t	g_redTeamName;
extern	vmCvar_t	g_restarted;
extern	vmCvar_t	g_scoreLimit;
extern	vmCvar_t	g_singlePlayerActive;
extern	vmCvar_t	g_smoothClients;
extern	vmCvar_t	g_speed;
extern	vmCvar_t	g_synchronousClients;
extern	vmCvar_t	g_tealTeamName;
extern	vmCvar_t	g_teamAutoJoin;
extern	vmCvar_t	g_teamForceBalance;
extern	vmCvar_t	g_timeLimit;
extern	vmCvar_t	g_warmupCountdownTime;
extern	vmCvar_t	g_weaponRespawn;
extern	vmCvar_t	g_weaponTeamRespawn;
extern	vmCvar_t	g_yellowTeamName;
extern	vmCvar_t	pmove_fixed;
extern	vmCvar_t	pmove_msec;
extern	vmCvar_t	pmove_overBounce;

extern	vmCvar_t	g_dropFlags;

extern	vmCvar_t	gt_frags_limit;
extern	vmCvar_t	gt_frags_timelimit;
extern	vmCvar_t	gt_duel_frags_limit;
extern	vmCvar_t	gt_duel_frags_timelimit;
extern	vmCvar_t	gt_teams_frags_limit;
extern	vmCvar_t	gt_teams_frags_mercylimit;
extern	vmCvar_t	gt_teams_frags_timelimit;
extern	vmCvar_t	gt_teams_captures_limit;
extern	vmCvar_t	gt_teams_captures_timelimit;
extern	vmCvar_t	gt_elimination_lives_limit;
extern	vmCvar_t	gt_elimination_timelimit;
extern	vmCvar_t	gt_points_limit;
extern	vmCvar_t	gt_points_timelimit;
extern	vmCvar_t	gt_rounds_limit;
extern	vmCvar_t	gt_rounds_timelimit;

extern	vmCvar_t	g_teamSize_max;
extern	vmCvar_t	g_teamSize_min;
extern	vmCvar_t	g_teamTotal_max;
extern	vmCvar_t	g_teamTotal_min;

extern	vmCvar_t	g_treasonDamage;

extern	vmCvar_t	g_ammoRules;
extern	vmCvar_t	g_armorRules;
extern	vmCvar_t	g_classicItemRespawns;

extern	vmCvar_t	g_forceWeaponColors;

extern	vmCvar_t	g_doReady;

extern	vmCvar_t	g_forceRespawn_delayMax;
extern	vmCvar_t	g_forceRespawn_delayMin;

extern	vmCvar_t	g_overTime;

extern	vmCvar_t	g_warmupDelay;
extern	vmCvar_t	g_warmupReadyPercentage;
extern	vmCvar_t	g_warmupWeaponSet;

extern	vmCvar_t	g_intermissionForceExitTime;
extern	vmCvar_t	g_gunyoffset;
extern	vmCvar_t	g_gunzoffset;

extern	vmCvar_t	pmove_q2;
extern	vmCvar_t	pmove_q2slide;
extern	vmCvar_t	pmove_q2air;

