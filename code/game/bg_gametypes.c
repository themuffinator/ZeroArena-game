
#include "../qcommon/q_shared.h"
#include "bg_public.h"

bggametypes_t	gt[GT_MAX_GAME_TYPE] = {
	{
/* index */			GT_SINGLE_PLAYER,
/* longName */		"Campaign Mode",
/* shortName */		"CAM",
/* gtSpawnRef */	"single",
/* gtArenaRef */	"single",

/* gtFlags */		GTF_CAMPAIGN,
/* elimLives */		0,
/* gtGoal */		GTL_OBJ,
	},
	{
/* index */			GT_FFA,
/* longName */		"Free for All",
/* shortName */		"FFA",
/* gtSpawnRef */	"ffa",
/* gtArenaRef */	"ffa",

/* gtFlags */		0,
/* elimLives */		0,
/* gtGoal */		GTL_FRAGS,
	},
	{
/* index */			GT_DUEL,
/* longName */		"Duel",
/* shortName */		"DUEL",
/* gtSpawnRef */	"tournament",
/* gtArenaRef */	"tourney",

/* gtFlags */		GTF_TOURNEY | GTF_DUEL,
/* elimLives */		0,
/* gtGoal */		GTL_FRAGS,
	},
	{
/* index */			GT_TEAM,
/* longName */		"Team Deathmatch",
/* shortName */		"TDM",
/* gtSpawnRef */	"team",
/* gtArenaRef */	"team",

/* gtFlags */		GTF_TEAMS | GTF_TDM,
/* elimLives */		0,
/* gtGoal */		GTL_FRAGS,
	},
	{
/* index */			GT_CTF,
/* longName */		"Capture the Flag",
/* shortName */		"CTF",
/* gtSpawnRef */	"ctf",
/* gtArenaRef */	"ctf",

/* gtFlags */		GTF_CTF | GTF_TEAMS | GTF_TEAMBASES,
/* elimLives */		0,
/* gtGoal */		GTL_CAPTURES,
	},
	{
/* index */			GT_1FCTF,
/* longName */		"One Flag CTF",
/* shortName */		"1F",
/* gtSpawnRef */	"oneflag",
/* gtArenaRef */	"oneflag",

/* gtFlags */		GTF_CTF | GTF_TEAMS | GTF_TEAMBASES | GTF_NEUTOB,
/* elimLives */		0,
/* gtGoal */		GTL_CAPTURES,
	},
	{
/* index */			GT_OVERLOAD,
/* longName */		"Overload",
/* shortName */		"OVL",
/* gtSpawnRef */	"obelisk",
/* gtArenaRef */	"overload",

/* gtFlags */		GTF_TEAMS | GTF_TEAMBASES | GTF_BASEOB,
/* elimLives */		0,
/* gtGoal */		GTL_POINTS,
	},
	{
/* index */			GT_HARVESTER,
/* longName */		"Harvester",
/* shortName */		"HAR",
/* gtSpawnRef */	"harvester",
/* gtArenaRef */	"harvester",

/* gtFlags */		GTF_TEAMS | GTF_TEAMBASES | GTF_BASEOB | GTF_NEUTOB,
/* elimLives */		0,
/* gtGoal */		GTL_POINTS,
	},
};
