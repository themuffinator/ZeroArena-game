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

#define INVENTORY_NONE				0
//armor
#define INVENTORY_ARMOR				1
//weapons
#define INVENTORY_GAUNTLET			4
#define INVENTORY_MACHINEGUN		5
#define INVENTORY_SHOTGUN			6
#define INVENTORY_GRENADELAUNCHER	7
#define INVENTORY_ROCKETLAUNCHER	8
#define INVENTORY_LIGHTNING			9
#define INVENTORY_RAILGUN			10
#define INVENTORY_PLASMAGUN			11
#define INVENTORY_BFG10K			13
#define INVENTORY_GRAPPLINGHOOK		14
#define INVENTORY_NAILGUN			15
#define INVENTORY_PROXLAUNCHER		16
#define INVENTORY_CHAINGUN			17
//ammo
#define INVENTORY_BULLETS			18
#define INVENTORY_SHELLS			19
#define INVENTORY_GRENADES			20
#define INVENTORY_ROCKETS			21
#define INVENTORY_LIGHTNINGAMMO		22
#define INVENTORY_SLUGS				23
#define INVENTORY_CELLS				24
#define INVENTORY_BFGAMMO			25
#define INVENTORY_NAILS				26
#define INVENTORY_MINES				27
#define INVENTORY_BELT				28
//powerups
#define INVENTORY_HEALTH			29
#define INVENTORY_TELEPORTER		30
#define INVENTORY_MEDKIT			31
#define INVENTORY_KAMIKAZE			32
#define INVENTORY_PORTAL			33
#define INVENTORY_INVULNERABILITY	34
#define INVENTORY_QUAD				35
#define INVENTORY_ENVIRONMENTSUIT	36
#define INVENTORY_HASTE				37
#define INVENTORY_INVISIBILITY		38
#define INVENTORY_REGEN				39
#define INVENTORY_FLIGHT			40
#define INVENTORY_VAMPIRE			41
#define INVENTORY_INVULN			42
#define INVENTORY_BREATHER			43

#define INVENTORY_SCOUT				44
#define INVENTORY_RESISTANCE		45
#define INVENTORY_STRENGTH			46
#define INVENTORY_ARMAMENT			47
#define INVENTORY_TENACITY			48
#define INVENTORY_PARASITE			49

#define INVENTORY_NEUTRALFLAG		50
#define INVENTORY_REDFLAG			51
#define INVENTORY_BLUEFLAG			52
#define INVENTORY_GREENFLAG			53
#define INVENTORY_YELLOWFLAG		54
#define INVENTORY_TEALFLAG			55
#define INVENTORY_PINKFLAG			56

#define INVENTORY_REDSKULL			57
#define INVENTORY_BLUESKULL			58
#define INVENTORY_GREENSKULL		59
#define INVENTORY_YELLOWSKULL		60
#define INVENTORY_TEALSKULL			61
#define INVENTORY_PINKSKULL			62

//multiteam indexes
#define INV_FLAGS_INDEX				INVENTORY_NEUTRALFLAG
#define INV_SKULLS_INDEX			INVENTORY_REDSKULL

//enemy stuff
#define ENEMY_HORIZONTAL_DIST		200
#define ENEMY_HEIGHT				201
#define NUM_VISIBLE_ENEMIES			202
#define NUM_VISIBLE_TEAMMATES		203

//item numbers (make sure they are in sync with bg_itemlist in bg_misc.c)
#define MODELINDEX_ARMORSHARD		1
#define MODELINDEX_ARMORJACKET		2
#define MODELINDEX_ARMORCOMBAT		3
#define MODELINDEX_ARMORBODY		4
#define MODELINDEX_HEALTHSMALL		5
#define MODELINDEX_HEALTHMED		6
#define MODELINDEX_HEALTHLARGE		7
#define MODELINDEX_HEALTHMEGA		8

#define MODELINDEX_GAUNTLET			9
#define MODELINDEX_MACHINEGUN		10
#define MODELINDEX_SHOTGUN			11
#define MODELINDEX_GRENADELAUNCHER	12
#define MODELINDEX_ROCKETLAUNCHER	13
#define MODELINDEX_LIGHTNING		14
#define MODELINDEX_RAILGUN			15
#define MODELINDEX_PLASMAGUN		16
#define MODELINDEX_BFG10K			17
#define MODELINDEX_GRAPPLINGHOOK	18

#define MODELINDEX_BULLETS			19
#define MODELINDEX_SHELLS			20
#define MODELINDEX_GRENADES			21
#define MODELINDEX_ROCKETS			22
#define MODELINDEX_LIGHTNINGAMMO	23
#define MODELINDEX_SLUGS			24
#define MODELINDEX_CELLS			25
#define MODELINDEX_BFGAMMO			26

#define MODELINDEX_TELEPORTER		27
#define MODELINDEX_MEDKIT			28
#define MODELINDEX_PSCREEN			29
#define MODELINDEX_PSHIELD			30
#define MODELINDEX_QUAD				31
#define MODELINDEX_ENVIRONMENTSUIT	32
#define MODELINDEX_HASTE			33
#define MODELINDEX_INVISIBILITY		34
#define MODELINDEX_REGEN			35
#define MODELINDEX_FLIGHT			36
#define MODELINDEX_VAMPIRE			37
#define MODELINDEX_INVULN			38
#define MODELINDEX_BREATHER			39

#define MODELINDEX_ANCIENT			40
#define MODELINDEX_ADREN			41
#define MODELINDEX_BANDO			42
#define MODELINDEX_AMMOPACK			43

#define MODELINDEX_KEY_BLUE			44
#define MODELINDEX_KEY_RED			45
#define MODELINDEX_KEY_DATACD		46
#define MODELINDEX_KEY_POWER		47
#define MODELINDEX_KEY_PYRAMID		48
#define MODELINDEX_KEY_SPINNER		49
#define MODELINDEX_KEY_PASS			50
#define MODELINDEX_KEY_GOLD			51
#define MODELINDEX_KEY_SILVER		52

#define MODELINDEX_SCOUT			53
#define MODELINDEX_RESISTANCE		54
#define MODELINDEX_STRENGTH			55
#define MODELINDEX_ARMAMENT			56
#define MODELINDEX_TENACITY		57
#define MODELINDEX_PARASITE			58

#define MODELINDEX_NEUTRALFLAG		59
#define MODELINDEX_REDFLAG			60
#define MODELINDEX_BLUEFLAG			61
#define MODELINDEX_GREENFLAG		62
#define MODELINDEX_YELLOWFLAG		63
#define MODELINDEX_TEALFLAG			64
#define MODELINDEX_PINKFLAG			65

#define MODELINDEX_REDSKULL			66
#define MODELINDEX_BLUESKULL		67
#define MODELINDEX_GREENSKULL		68
#define MODELINDEX_YELLOWSKULL		69
#define MODELINDEX_TEALSKULL		70
#define MODELINDEX_PINKSKULL		71

// mission pack only defines

#define MODELINDEX_KAMIKAZE			72
#define MODELINDEX_PORTAL			73
#define MODELINDEX_INVULNERABILITY	74

#define MODELINDEX_NAILS			75
#define MODELINDEX_MINES			76
#define MODELINDEX_BELT				77

#define MODELINDEX_NAILGUN			78
#define MODELINDEX_PROXLAUNCHER		79
#define MODELINDEX_CHAINGUN			80

//
#define WEAPONINDEX_GAUNTLET			1
#define WEAPONINDEX_MACHINEGUN			2
#define WEAPONINDEX_SHOTGUN				3
#define WEAPONINDEX_GRENADE_LAUNCHER	4
#define WEAPONINDEX_ROCKET_LAUNCHER		5
#define WEAPONINDEX_LIGHTNING			6
#define WEAPONINDEX_RAILGUN				7
#define WEAPONINDEX_PLASMAGUN			8
#define WEAPONINDEX_BFG					9
#define WEAPONINDEX_GRAPPLING_HOOK		10
#define WEAPONINDEX_NAILGUN				11
#define WEAPONINDEX_PROXLAUNCHER		12
#define WEAPONINDEX_CHAINGUN			13
