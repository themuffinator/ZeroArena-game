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
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

pmove_t* pm;
pml_t		pml;

// movement parameters
const float	pm_stopSpeed = 100.0f;
const float pm_ladderSpeed = 100.0f;
const float	pm_duckScale = 0.25f;
const float	pm_swimScale = 0.50f;

const float	pm_accelerate = 10.0f;
const float	pm_airAccelerate = 1.0f;
const float	pm_waterAccelerate = 4.0f;
const float	pm_flyAccelerate = 8.0f;

const float	pm_groundFriction = 6.0f;
const float	pm_waterFriction = 1.0f;
const float	pm_flightFriction = 3.0f;
const float	pm_ladderFriction = 14.0f;
const float	pm_spectatorFriction = 5.0f;

// quake 2 movement parameters
float	pmq2_stopspeed = 100;	//
float	pmq2_maxspeed = 300;
float	pmq2_duckspeed = 100;
float	pmq2_accelerate = 10;	//
float	pmq2_airaccelerate = 0;
float	pmq2_wateraccelerate = 10;
float	pmq2_friction = 6;	//
float	pmq2_waterfriction = 1;	//
float	pmq2_waterspeed = 400;

int		c_pmove = 0;

/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent( int newEvent ) {
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps );
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt( int entityNum ) {
	int		i;

	if ( entityNum == ENTITYNUM_WORLD ) {
		return;
	}
	if ( pm->numtouch == MAXTOUCH ) {
		return;
	}

	// see if it is already added
	for ( i = 0; i < pm->numtouch; i++ ) {
		if ( pm->touchents[i] == entityNum ) {
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
===================
PM_StartTorsoAnim
===================
*/
static void PM_StartTorsoAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
	pm->ps->torsoAnim = ((pm->ps->torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT)
		| anim;
}
static void PM_StartLegsAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}
	pm->ps->legsAnim = ((pm->ps->legsAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT)
		| anim;
}

static void PM_ContinueLegsAnim( int anim ) {
	if ( (pm->ps->legsAnim & ~ANIM_TOGGLEBIT) == anim ) {
		return;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}
	PM_StartLegsAnim( anim );
}

static void PM_ContinueTorsoAnim( int anim ) {
	if ( (pm->ps->torsoAnim & ~ANIM_TOGGLEBIT) == anim ) {
		return;
	}
	if ( pm->ps->torsoTimer > 0 ) {
		return;		// a high priority animation is running
	}
	PM_StartTorsoAnim( anim );
}

static void PM_ForceLegsAnim( int anim ) {
	pm->ps->legsTimer = 0;
	PM_StartLegsAnim( anim );
}


/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
#define	STOP_EPSILON	0.1
void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce ) {
	float	backoff;
	float	change;
	int		i;

	backoff = DotProduct( in, normal );

	if ( pm->pmove_q2 || backoff < 0 ) {
		backoff *= overbounce;
	} else {
		backoff /= overbounce;
	}

	for ( i = 0; i < 3; i++ ) {
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if ( pm->pmove_q2 && out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON )
			out[i] = 0;
	}
}


/*

input: origin, velocity, bounds, groundPlane, trace function

output: origin, velocity, impacts, stairup boolean

*/

/*
==================
PM_SlideMove

Returns qtrue if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES	5
qboolean	PM_SlideMove( qboolean gravity ) {
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity;
	vec3_t		clipVelocity;
	int			i, j, k;
	trace_t		trace;
	vec3_t		end;
	float		time_left;
	float		into;
	vec3_t		endVelocity;
	vec3_t		endClipVelocity;

	numbumps = 4;

	VectorCopy( pm->ps->velocity, primal_velocity );

	if ( !pm->pmove_q2slide && gravity ) {
		VectorCopy( pm->ps->velocity, endVelocity );
		endVelocity[2] -= pm->ps->gravity * pml.frametime;
		pm->ps->velocity[2] = (pm->ps->velocity[2] + endVelocity[2]) * 0.5;
		primal_velocity[2] = endVelocity[2];
		if ( pml.groundPlane ) {
			// slide along the ground plane
			PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal,
				pm->ps->velocity, OVERCLIP );
		}
	}

	time_left = pml.frametime;

	// never turn against the ground plane
	if ( pml.groundPlane ) {
		numplanes = 1;
		VectorCopy( pml.groundTrace.plane.normal, planes[0] );
	} else {
		numplanes = 0;
	}

	// never turn against original velocity
	VectorNormalize2( pm->ps->velocity, planes[numplanes] );
	numplanes++;

	for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ ) {

		// calculate position we are trying to move to
		VectorMA( pm->ps->origin, time_left, pm->ps->velocity, end );

		// see if we can make it there
		pm->trace( &trace, pm->ps->origin, pm->ps->mins, pm->ps->maxs, end, pm->ps->playerNum, pm->tracemask );

		if ( trace.allsolid ) {
			// entity is completely trapped in another solid
			pm->ps->velocity[2] = 0;	// don't build up falling damage, but allow sideways acceleration
			return qtrue;
		}

		if ( trace.fraction > 0 ) {
			// actually covered some distance
			VectorCopy( trace.endpos, pm->ps->origin );
		}

		if ( trace.fraction == 1 ) {
			break;		// moved the entire distance
		}

		// save entity for contact
		PM_AddTouchEnt( trace.entityNum );

		time_left -= time_left * trace.fraction;

		if ( numplanes >= MAX_CLIP_PLANES ) {
			// this shouldn't really happen
			VectorClear( pm->ps->velocity );
			return qtrue;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		if ( !pm->pmove_q2slide ) {
			for ( i = 0; i < numplanes; i++ ) {
				if ( DotProduct( trace.plane.normal, planes[i] ) > 0.99 ) {
					VectorAdd( trace.plane.normal, pm->ps->velocity, pm->ps->velocity );
					break;
				}
			}
			if ( i < numplanes ) {
				continue;
			}
		}
		VectorCopy( trace.plane.normal, planes[numplanes] );
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0; i < numplanes; i++ ) {
			if ( !pm->pmove_q2slide ) {
				into = DotProduct( pm->ps->velocity, planes[i] );
				if ( into >= 0.1 ) {
					continue;		// move doesn't interact with the plane
				}

				// see how hard we are hitting things
				if ( -into > pml.impactSpeed ) {
					pml.impactSpeed = -into;
				}
			}
			// slide along the plane
			PM_ClipVelocity( pm->ps->velocity, planes[i], clipVelocity, OVERCLIP );

			if ( !pm->pmove_q2slide && gravity ) {
				// slide along the plane
				PM_ClipVelocity( endVelocity, planes[i], endClipVelocity, OVERCLIP );
			}

			// see if there is a second plane that the new move enters
			if ( pm->pmove_q2slide ) {
				for ( j = 0; j < numplanes; j++ )
					if ( j != i ) {
						if ( DotProduct( clipVelocity, planes[j] ) < 0 )
							break;	// not ok
					}
				if ( j == numplanes )
					break;
			} else {
				for ( j = 0; j < numplanes; j++ ) {
					if ( !pm->pmove_q2slide ) {
						if ( j == i ) {
							continue;
						}
						if ( DotProduct( clipVelocity, planes[j] ) >= 0.1 ) {
							continue;		// move doesn't interact with the plane
						}
					}

					// try clipping the move to the plane
					PM_ClipVelocity( clipVelocity, planes[j], clipVelocity, OVERCLIP );

					if ( gravity ) {
						PM_ClipVelocity( endClipVelocity, planes[j], endClipVelocity, OVERCLIP );
					}

					// see if it goes back into the first clip plane
					if ( DotProduct( clipVelocity, planes[i] ) >= 0 ) {
						continue;
					}

					// slide the original velocity along the crease
					CrossProduct( planes[i], planes[j], dir );
					VectorNormalize( dir );
					d = DotProduct( dir, pm->ps->velocity );
					VectorScale( dir, d, clipVelocity );

					if ( !pm->pmove_q2slide && gravity ) {
						CrossProduct( planes[i], planes[j], dir );
						VectorNormalize( dir );
						d = DotProduct( dir, endVelocity );
						VectorScale( dir, d, endClipVelocity );
					}

					// see if there is a third plane the the new move enters
					for ( k = 0; k < numplanes; k++ ) {
						if ( k == i || k == j ) {
							continue;
						}
						if ( DotProduct( clipVelocity, planes[k] ) >= 0.1 ) {
							continue;		// move doesn't interact with the plane
						}

						// stop dead at a tripple plane interaction
						VectorClear( pm->ps->velocity );
						return qtrue;
					}
				}
			}

			if ( pm->pmove_q2slide ) {
				if ( i != numplanes ) {	// go along this plane
				} else {	// go along the crease
					if ( numplanes != 2 ) {
		//				Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
						VectorCopy( vec3_origin, clipVelocity );
						break;
					}
					CrossProduct( planes[0], planes[1], dir );
					d = DotProduct( dir, clipVelocity );
					VectorScale( dir, d, clipVelocity );
				}
				//
				// if velocity is against the original velocity, stop dead
				// to avoid tiny occilations in sloping corners
				//
				if ( DotProduct( clipVelocity, primal_velocity ) <= 0 ) {
					VectorCopy( vec3_origin, clipVelocity );
					break;
				}
			} else {
				// if we have fixed all interactions, try another move
				VectorCopy( clipVelocity, pm->ps->velocity );
				if ( gravity ) {
					VectorCopy( endClipVelocity, endVelocity );
				}
				break;
			}
		}
	}

	if ( !pm->pmove_q2slide && gravity ) {
		VectorCopy( endVelocity, pm->ps->velocity );
	}

	// don't change velocity if in a timer (FIXME: is this correct?)
	if ( pm->ps->pm_time ) {
		VectorCopy( primal_velocity, pm->ps->velocity );
	}

	return (bumpcount != 0);
}

/*
==================
PM_StepSlideMove

==================
*/
void PM_StepSlideMove( qboolean gravity ) {
	vec3_t		start_o, start_v;
	vec3_t		down_o, down_v;		//pm->pmove_q2slide
	trace_t		trace;
	float		down_dist, up_dist;		//pm->pmove_q2slide
//	vec3_t		delta, delta2;
	vec3_t		up, down;
	float		stepSize;

	VectorClear( down_o );
	VectorClear( down_v );
	VectorCopy( pm->ps->origin, start_o );
	VectorCopy( pm->ps->velocity, start_v );
	if ( pm->pmove_q2slide ) {
		PM_SlideMove( gravity );

		VectorCopy( pm->ps->origin, down_o );
		VectorCopy( pm->ps->velocity, down_v );
	} else {
		if ( !PM_SlideMove( gravity ) )
			return;		// we got exactly where we wanted to go first try

		VectorCopy( start_o, down );
		down[2] -= STEPSIZE;
		pm->trace( &trace, start_o, pm->ps->mins, pm->ps->maxs, down, pm->ps->playerNum, pm->tracemask );
		VectorSet( up, 0, 0, 1 );
		// never step up when you still have up velocity
		if ( pm->ps->velocity[2] > 0 && (trace.fraction == 1.0 ||
			DotProduct( trace.plane.normal, up ) < 0.7) ) {
			return;
		}
	}

	VectorCopy( start_o, up );
	up[2] += STEPSIZE;

	// test the player position if they were a stepheight higher
	pm->trace( &trace, start_o, pm->ps->mins, pm->ps->maxs, up, pm->ps->playerNum, pm->tracemask );
	if ( trace.allsolid ) {
		if ( pm->debugLevel ) {
			Com_Printf( "%i:bend can't step\n", c_pmove );
		}
		return;		// can't step up
	}

	stepSize = trace.endpos[2] - start_o[2];
	// try slidemove from this position
	VectorCopy( trace.endpos, pm->ps->origin );
	VectorCopy( start_v, pm->ps->velocity );

	PM_SlideMove( gravity );

	// push down the final amount
	VectorCopy( pm->ps->origin, down );
	down[2] -= stepSize;
	pm->trace( &trace, pm->ps->origin, pm->ps->mins, pm->ps->maxs, down, pm->ps->playerNum, pm->tracemask );
	if ( !trace.allsolid ) {
		VectorCopy( trace.endpos, pm->ps->origin );
	}
	if ( !pm->pmove_q2slide && trace.fraction < 1.0 ) {
		PM_ClipVelocity( pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP );
	}


	if ( pm->pmove_q2slide ) {
		VectorCopy( pm->ps->origin, up );

		// decide which one went farther
		down_dist = (down_o[0] - start_o[0]) * (down_o[0] - start_o[0])
			+ (down_o[1] - start_o[1]) * (down_o[1] - start_o[1]);
		up_dist = (up[0] - start_o[0]) * (up[0] - start_o[0])
			+ (up[1] - start_o[1]) * (up[1] - start_o[1]);

		if ( down_dist > up_dist || trace.plane.normal[2] < MIN_WALK_NORMAL ) {
			VectorCopy( down_o, pm->ps->origin );
			VectorCopy( down_v, pm->ps->velocity );
			return;
		}
		//!! Special case
		// if we were walking along a plane, then we need to copy the Z over
		pm->ps->velocity[2] = down_v[2];
	} else {
		// use the step move
		float	delta;

		delta = pm->ps->origin[2] - start_o[2];
		if ( delta > 2 ) {
			if ( delta < 7 ) {
				PM_AddEvent( EV_STEP_4 );
			} else if ( delta < 11 ) {
				PM_AddEvent( EV_STEP_8 );
			} else if ( delta < 15 ) {
				PM_AddEvent( EV_STEP_12 );
			} else {
				PM_AddEvent( EV_STEP_16 );
			}
		}
		if ( pm->debugLevel ) {
			Com_Printf( "%i:stepped\n", c_pmove );
		}
	}
}


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction( void ) {
	vec3_t	vec;
	float* vel;
	float	speed, newspeed, control;
	float	drop;

	vel = pm->ps->velocity;

	VectorCopy( vel, vec );
	if ( !pm->pmove_q2 && pml.walking ) {
		vec[2] = 0;	// ignore slope movement
	}

	speed = VectorLength( vec );
	if ( speed < 1 ) {
		vel[0] = 0;
		vel[1] = 0;		// allow sinking underwater
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// apply ground friction
	if ( pm->waterlevel <= 1 ) {
		if ( pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK) ) {
			// if getting knocked back, no friction
			if ( pm->pmove_q2 || !(pm->ps->pm_flags & PMF_TIME_KNOCKBACK) ) {
				control = speed < pm_stopSpeed ? pm_stopSpeed : speed;
				drop += control * pm_groundFriction * pml.frametime;
			}
		}
	}

	// apply water friction even if just wading
	if ( pm->waterlevel || ( pm->pmove_q2 && pm->waterlevel && !pml.ladder ) ) {		//no ladder added from pm->pmove_q2 code
		drop += speed * pm_waterFriction * pm->waterlevel * pml.frametime;
	}

	// apply flying friction
	if ( pm->ps->powerups[PW_FLIGHT] ) {
		drop += speed * pm_flightFriction * pml.frametime;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
		drop += speed * pm_spectatorFriction * pml.frametime;
	}

	// apply ladder strafe friction
	if ( !pm->pmove_q2 && pml.ladder ) {
		drop += speed * pm_ladderFriction * pml.frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if ( newspeed < 0 ) {
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel ) {
	// q2 style
	int			i;
	float		addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct( pm->ps->velocity, wishdir );
	addspeed = wishspeed - currentspeed;
	if ( addspeed <= 0 ) {
		return;
	}
	accelspeed = accel * pml.frametime * wishspeed;
	if ( accelspeed > addspeed ) {
		accelspeed = addspeed;
	}

	for ( i = 0; i < 3; i++ ) {
		pm->ps->velocity[i] += accelspeed * wishdir[i];
	}
}


void PMQ2_AirAccelerate( vec3_t wishdir, float wishspeed, float accel ) {
	int			i;
	float		addspeed, accelspeed, currentspeed, wishspd = wishspeed;

	if ( wishspd > 30 )
		wishspd = 30;
	currentspeed = DotProduct( pm->ps->velocity, wishdir );
	addspeed = wishspd - currentspeed;
	if ( addspeed <= 0 )
		return;
	accelspeed = accel * wishspeed * pml.frametime;
	if ( accelspeed > addspeed )
		accelspeed = addspeed;

	for ( i = 0; i < 3; i++ )
		pm->ps->velocity[i] += accelspeed * wishdir[i];
}


/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale( usercmd_t* cmd ) {
	int		max;
	float	total;
	float	scale;

	max = abs( cmd->forwardmove );
	if ( abs( cmd->rightmove ) > max ) {
		max = abs( cmd->rightmove );
	}
	if ( abs( cmd->upmove ) > max ) {
		max = abs( cmd->upmove );
	}
	if ( !max ) {
		return 0;
	}

	total = sqrt( cmd->forwardmove * cmd->forwardmove
		+ cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove );
	scale = (float)pm->ps->speed * max / (127.0 * total);

	return scale;
}


/*
================
PM_SetMovementDir

Determine the rotation of the legs relative
to the facing dir
================
*/
static void PM_SetMovementDir( void ) {
	if ( pm->cmd.forwardmove || pm->cmd.rightmove ) {
		if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 0;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 2;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 3;
		} else if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 4;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 5;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 6;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 7;
		}
	} else {
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if ( pm->ps->movementDir == 2 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->ps->movementDir == 6 ) {
			pm->ps->movementDir = 7;
		}
	}
}


/*
=============
PM_CheckJump
=============
*/
static qboolean PM_CheckJump( void ) {
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return qfalse;		// don't allow jump until all buttons are up
	}

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		return qfalse;
	}

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD ) {
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	if ( pm->pmove_q2 ) {
		if ( pm->ps->pm_type == PM_DEAD )
			return qfalse;

		if ( pm->waterlevel >= 2 ) {	// swimming, not jumping
			pm->ps->groundEntityNum = ENTITYNUM_NONE;
			pml.groundPlane = qfalse;
			pml.walking = qfalse;

			if ( pm->ps->velocity[2] <= -300 )
				return qfalse;

			if ( pm->watertype == CONTENTS_WATER )
				pm->ps->velocity[2] = 100;
			else if ( pm->watertype == CONTENTS_SLIME )
				pm->ps->velocity[2] = 80;
			else
				pm->ps->velocity[2] = 50;
			return qfalse;
		}

		if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
			return qfalse;		// in air, so no effect
	}

	pml.groundPlane = qfalse;		// jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	if ( pm->pmove_q2 ) {
		pm->ps->velocity[2] += JUMP_VELOCITY;
		if ( pm->ps->velocity[2] < JUMP_VELOCITY )
			pm->ps->velocity[2] = JUMP_VELOCITY;
	} else {
		pm->ps->velocity[2] = JUMP_VELOCITY;
	}
	PM_AddEvent( EV_JUMP );

	if ( pm->cmd.forwardmove >= 0 ) {
		PM_ForceLegsAnim( LEGS_JUMP );
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	} else {
		PM_ForceLegsAnim( LEGS_JUMPB );
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	return qtrue;
}

/*
=============
PM_CheckWaterJump
=============
*/
static qboolean	PM_CheckWaterJump( void ) {
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;

	if ( pm->ps->pm_time ) {
		return qfalse;
	}

	// check for water jump
	if ( pm->waterlevel != 2 ) {
		return qfalse;
	}

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize( flatforward );

	VectorMA( pm->ps->origin, 30, flatforward, spot );
	spot[2] += 4;
	cont = pm->pointcontents( spot, pm->ps->playerNum );
	if ( !(cont & CONTENTS_SOLID) ) {
		return qfalse;
	}

	spot[2] += 16;
	cont = pm->pointcontents( spot, pm->ps->playerNum );
	if ( cont & (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY) ) {
		return qfalse;
	}

	// jump out of water
	VectorScale( pml.forward, 200, pm->ps->velocity );
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

//============================================================================


/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove( void ) {
	// waterjump has no control, but falls

	PM_StepSlideMove( qtrue );

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if ( pm->ps->velocity[2] < 0 ) {
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove( void ) {
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	float	scale;		//quake3
	float	vel;		//quake3

	if ( !pm->pmove_q2 ) {
		if ( PM_CheckWaterJump() ) {
			PM_WaterJumpMove();
			return;
		}

		PM_Friction();
	}

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = -60;		// sink towards bottom
	} else {
		for ( i = 0; i < 3; i++ )
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );

	if ( pm->pmove_q2 ) {
		if ( wishspeed > pmq2_maxspeed ) {
			VectorScale( wishvel, pmq2_maxspeed / wishspeed, wishvel );
			wishspeed = pmq2_maxspeed;
		}
		wishspeed *= 0.5;
	} else if ( wishspeed > pm->ps->speed * pm_swimScale ) {
		wishspeed = pm->ps->speed * pm_swimScale;
	}

	PM_Accelerate( wishdir, wishspeed, pm_waterAccelerate );

	// make sure we can go up slopes easily under water
	if ( !pm->pmove_q2 ) {
		if ( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0 ) {
			vel = VectorLength( pm->ps->velocity );
			// slide along the ground plane
			PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal,
				pm->ps->velocity, OVERCLIP );

			// don't decrease velocity when going up or down a slope
			if ( pm->pmove_overBounce || VectorLength( pm->ps->velocity ) > 1 ) {
				VectorNormalize( pm->ps->velocity );
				VectorScale( pm->ps->velocity, vel, pm->ps->velocity );
			}
		}
	}

	PM_SlideMove( qfalse );
}

#ifdef MISSIONPACK
/*
===================
PM_InvulnerabilityMove

Only with the invulnerability powerup
===================
*/
static void PM_InvulnerabilityMove( void ) {
	pm->cmd.forwardmove = 0;
	pm->cmd.rightmove = 0;
	pm->cmd.upmove = 0;
	VectorClear( pm->ps->velocity );
}
#endif

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove( void ) {
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	float	scale;

	// normal slowdown
	PM_Friction();

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;
	} else {
		for ( i = 0; i < 3; i++ ) {
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;
		}

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );

	PM_Accelerate( wishdir, wishspeed, pm_flyAccelerate );

	PM_StepSlideMove( qfalse );
}


/*
===================
PM_AirMove

===================
*/
static void PM_AirMove( const float gravity ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so players can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	for ( i = 0; i < 2; i++ ) {
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}
	wishvel[2] = 0;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

	if ( pm->pmove_q2air ) {
		float		maxspeed;
		//
		// clamp to server defined max speed
		//
		maxspeed = (pm->ps->pm_flags & PMF_DUCKED) ? pmq2_duckspeed : pmq2_maxspeed;

		if ( wishspeed > maxspeed ) {
			VectorScale( wishvel, maxspeed / wishspeed, wishvel );
			wishspeed = maxspeed;
		}

		if ( pml.ladder ) {
			PM_Accelerate( wishdir, wishspeed, pmq2_accelerate );
			if ( !wishvel[2] ) {
				if ( pm->ps->velocity[2] > 0 ) {
					pm->ps->velocity[2] -= gravity * pml.frametime;
					if ( pm->ps->velocity[2] < 0 )
						pm->ps->velocity[2] = 0;
				} else {
					pm->ps->velocity[2] += gravity * pml.frametime;
					if ( pm->ps->velocity[2] > 0 )
						pm->ps->velocity[2] = 0;
				}
			}
			PM_StepSlideMove( gravity );
		} else if ( pml.walking ) { //pm->groundentity ) {	// walking on ground
			pm->ps->velocity[2] = 0; //!!! this is before the accel
			PM_Accelerate( wishdir, wishspeed, pmq2_accelerate );

			// PGM	-- fix for negative trigger_gravity fields
			//		pm->ps->velocity[2] = 0;
			if ( gravity > 0 )
				pm->ps->velocity[2] = 0;
			else
				pm->ps->velocity[2] -= gravity * pml.frametime;
			// PGM

			if ( !pm->ps->velocity[0] && !pm->ps->velocity[1] )
				return;
			PM_SlideMove( DEFAULT_GRAVITY );	//PM_StepSlideMove();
		} else {	// not on ground, so little effect on velocity
			if ( pmq2_airaccelerate )
				PMQ2_AirAccelerate( wishdir, wishspeed, pmq2_accelerate );
			else
				PM_Accelerate( wishdir, wishspeed, 1 );
			// add gravity
			pm->ps->velocity[2] -= gravity * pml.frametime;
			PM_SlideMove( DEFAULT_GRAVITY );	//PM_StepSlideMove();
		}
	} else {
		// not on ground, so little effect on velocity
		PM_Accelerate( wishdir, wishspeed, pm_airAccelerate );

		// we may have a ground plane that is very steep, even
		// though we don't have a groundentity
		// slide along the steep plane
		if ( pml.groundPlane ) {
			PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal,
				pm->ps->velocity, OVERCLIP );
		}

		PM_StepSlideMove( qtrue );
	}
}


/*
===================
PM_GrappleMove

===================
*/
static void PM_GrappleMove( void ) {
	vec3_t vel, v;
	float vlen;

	VectorScale( pml.forward, -16, v );
	VectorAdd( pm->ps->grapplePoint, v, v );
	VectorSubtract( v, pm->ps->origin, vel );
	vlen = VectorLength( vel );
	VectorNormalize( vel );

	if ( vlen <= 100 )
		VectorScale( vel, 10 * vlen, vel );
	else
		VectorScale( vel, 800, vel );

	VectorCopy( vel, pm->ps->velocity );

	pml.groundPlane = qfalse;
}

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove( void ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;
	float		accelerate;
	float		vel;

	if ( pm->waterlevel > 2 && DotProduct( pml.forward, pml.groundTrace.plane.normal ) > 0 ) {
		// begin swimming
		PM_WaterMove();
		return;
	}


	if ( PM_CheckJump() ) {
		// jumped away
		if ( pm->waterlevel > 1 ) {
			PM_WaterMove();
		} else {
			PM_AirMove( DEFAULT_GRAVITY );
		}
		return;
	}

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so players can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity( pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP );
	PM_ClipVelocity( pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP );
	//
	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	for ( i = 0; i < 3; i++ ) {
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}
	// when going up or down slopes the wish velocity should Not be zero
//	wishvel[2] = 0;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

	// clamp the speed lower if ducking
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		if ( wishspeed > pm->ps->speed * pm_duckScale ) {
			wishspeed = pm->ps->speed * pm_duckScale;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if ( pm->waterlevel ) {
		float	waterScale;

		waterScale = pm->waterlevel / 3.0;
		waterScale = 1.0 - (1.0 - pm_swimScale) * waterScale;
		if ( wishspeed > pm->ps->speed * waterScale ) {
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if ( (pml.groundTrace.surfaceFlags & SURF_SLICK) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		accelerate = pm_airAccelerate;
	} else {
		accelerate = pm_accelerate;
	}

	PM_Accelerate( wishdir, wishspeed, accelerate );

	//Com_Printf("velocity = %1.1f %1.1f %1.1f\n", pm->ps->velocity[0], pm->ps->velocity[1], pm->ps->velocity[2]);
	//Com_Printf("velocity1 = %1.1f\n", VectorLength(pm->ps->velocity));

	if ( (pml.groundTrace.surfaceFlags & SURF_SLICK) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	} else {
		// don't reset the z velocity for slopes
//		pm->ps->velocity[2] = 0;
	}

	vel = VectorLength( pm->ps->velocity );

	// slide along the ground plane
	PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal,
		pm->ps->velocity, OVERCLIP );

	// don't decrease velocity when going up or down a slope
	if ( pm->pmove_overBounce || VectorLength( pm->ps->velocity ) > 1 ) {
		VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, vel, pm->ps->velocity );
	}

	// don't do anything if standing still
	if ( !pm->ps->velocity[0] && !pm->ps->velocity[1] ) {
		return;
	}

	PM_StepSlideMove( qfalse );

	//Com_Printf("velocity2 = %1.1f\n", VectorLength(pm->ps->velocity));

}


/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove( void ) {
	float	forward;

	if ( !pml.walking ) {
		return;
	}

	// extra friction

	forward = VectorLength( pm->ps->velocity );
	forward -= 20;
	if ( forward <= 0 ) {
		VectorClear( pm->ps->velocity );
	} else {
		VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, forward, pm->ps->velocity );
	}
}


/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove( void ) {
	float	speed, drop, friction, control, newspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength( pm->ps->velocity );
	if ( speed < 1 ) {
		VectorCopy( vec3_origin, pm->ps->velocity );
	} else {
		drop = 0;

		friction = pm_groundFriction * 1.5;	// extra friction
		control = speed < pm_stopSpeed ? pm_stopSpeed : speed;
		drop += control * friction * pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if ( newspeed < 0 )
			newspeed = 0;
		newspeed /= speed;

		VectorScale( pm->ps->velocity, newspeed, pm->ps->velocity );
	}

	// accelerate
	scale = PM_CmdScale( &pm->cmd );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for ( i = 0; i < 3; i++ )
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	wishvel[2] += pm->cmd.upmove;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	VectorMA( pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin );
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number appropriate for the groundsurface
================
*/
static int PM_FootstepForSurface( void ) {
	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		return 0;
	} else if ( pml.groundTrace.surfaceFlags & SURF_METALSTEPS ) {
		return EV_FOOTSTEP_METAL;
	} else if ( pml.groundTrace.surfaceFlags & SURF_SNOW ) {
		return EV_FOOTSTEP_SNOW;
	} else if ( pml.groundTrace.surfaceFlags & SURF_WOOD ) {
		return EV_FOOTSTEP_WOOD;
	}
	return EV_FOOTSTEP;
}


/*
=================
PM_CrashLand

Check for hard landings that generate sound events
source: RTCW
=================
*/
static void PM_CrashLand( void ) {
	float		delta;
	float		dist;
	float		vel, acc;
	float		t;
	float		a, b, c, den;

	// decide which landing animation to use
	if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP ) {
		PM_ForceLegsAnim( LEGS_LANDB );
	} else {
		PM_ForceLegsAnim( LEGS_LAND );
	}

	pm->ps->legsTimer = TIMER_LAND;

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel = pml.previous_velocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den = b * b - 4 * a * c;
	if ( den < 0 ) {
		return;
	}
	t = (-b - sqrt( den )) / (2 * a);

	delta = vel + t * acc;
	delta = delta * delta * 0.0001;

	// ducking while falling doubles damage
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		delta *= 2;
	}

	// never take falling damage if completely underwater
	if ( pm->waterlevel == 3 ) {
		return;
	}

	// reduce falling damage if there is standing water
	if ( pm->waterlevel == 2 ) {
		delta *= 0.25;
	}
	if ( pm->waterlevel == 1 ) {
		delta *= 0.5;
	}

	if ( delta < 1 ) {
		return;
	}

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if ( !(pml.groundTrace.surfaceFlags & SURF_NODAMAGE) ) {
		if ( delta > 60 ) {
			PM_AddEvent( EV_FALL_FAR );
		} else if ( delta > 40 ) {
			// this is a pain grunt, so don't play it if dead
			if ( pm->ps->stats[STAT_HEALTH] > 0 ) {
				PM_AddEvent( EV_FALL_MEDIUM );
			}
		} else if ( delta > 7 ) {
			PM_AddEvent( EV_FALL_SHORT );
		} else {
			if ( !pm->ps->powerups[PW_SCOUT] )
				PM_AddEvent( PM_FootstepForSurface() );
		}
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
}

/*
================
PM_CheckLadderMove

Checks to see if we are on a ladder
source: RTCW
================
*/
qboolean ladderforward;
vec3_t laddervec;

void PM_CheckLadderMove( void ) {
	vec3_t spot;
	vec3_t flatforward;
	trace_t trace;
	float tracedist;
#define TRACE_LADDER_DIST   48.0
	qboolean wasOnLadder = qfalse;

	if ( pm->ps->pm_time ) {
		return;
	}

	if ( pml.walking ) {
		tracedist = 1.0;
	} else {
		tracedist = TRACE_LADDER_DIST;
	}

	wasOnLadder = ((pm->ps->pm_flags & PMF_LADDER) != 0);

	pml.ladder = qfalse;
	pm->ps->pm_flags &= ~PMF_LADDER;    // clear ladder bit
	ladderforward = qfalse;

	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check for ladder
	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize( flatforward );

	VectorMA( pm->ps->origin, tracedist, flatforward, spot );
	pm->trace( &trace, pm->ps->origin, pm->ps->mins, pm->ps->maxs, spot, pm->ps->playerNum, pm->tracemask );
	if ( (trace.fraction < 1) && (trace.surfaceFlags & SURF_LADDER) ) {
		pml.ladder = qtrue;
	}
	if ( pml.ladder ) {
		VectorCopy( trace.plane.normal, laddervec );
	}

	if ( pml.ladder && !pml.walking && (trace.fraction * tracedist > 1.0) ) {
		vec3_t mins;
		// if we are only just on the ladder, don't do this yet, or it may throw us back off the ladder
		pml.ladder = qfalse;
		VectorCopy( pm->ps->mins, mins );
		mins[2] = -1;
		VectorMA( pm->ps->origin, -tracedist, laddervec, spot );
		pm->trace( &trace, pm->ps->origin, mins, pm->ps->maxs, spot, pm->ps->playerNum, pm->tracemask );
		if ( (trace.fraction < 1) && (trace.surfaceFlags & SURF_LADDER) ) {
			// if AI, then be more stringent on their viewangles
#if 0
			if ( pm->ps->aiChar && (DotProduct( trace.plane.normal, pml.forward ) > -0.9) ) {
				pml.ladder = qfalse;
			} else {
#endif
				ladderforward = qtrue;
				pml.ladder = qtrue;
				pm->ps->pm_flags |= PMF_LADDER; // set ladder bit
#if 0
			}
#endif
		} else {
			pml.ladder = qfalse;
		}
	} else if ( pml.ladder ) {
		pm->ps->pm_flags |= PMF_LADDER; // set ladder bit
	}

	// create some up/down velocity if touching ladder
	if ( pml.ladder ) {
		if ( pml.walking ) {
			// we are currently on the ground, only go up and prevent X/Y if we are pushing forwards
			if ( pm->cmd.forwardmove <= 0 ) {
				pml.ladder = qfalse;
			}
		}
	}

	// if we have just dismounted the ladder at the top, play dismount
	if ( !pml.ladder && wasOnLadder && pm->ps->velocity[2] > 0 ) {
#if 0
		BG_AnimScriptEvent( pm->ps, ANIM_ET_CLIMB_DISMOUNT, qfalse, qfalse );
#endif
	}
	// if we have just mounted the ladder
	if ( pml.ladder && !wasOnLadder && pm->ps->velocity[2] < 0 ) {    // only play anim if going down ladder
#if 0
		//BG_AnimScriptEvent(pm->ps, ANIM_ET_CLIMB_MOUNT, qfalse, qfalse);
#endif
	}

}



/*
============
PM_LadderMove

source: RTCW
============
*/
void PM_LadderMove( void ) {
	float wishspeed, scale;
	vec3_t wishdir, wishvel;
	float upscale;

	if ( ladderforward ) {
		// move towards the ladder
		VectorScale( laddervec, -200.0, wishvel );
		pm->ps->velocity[0] = wishvel[0];
		pm->ps->velocity[1] = wishvel[1];
	}

	upscale = (pml.forward[2] + 0.5) * 2.5;
	if ( upscale > 1.0 ) {
		upscale = 1.0;
	} else if ( upscale < -1.0 ) {
		upscale = -1.0;
	}

	// forward/right should be horizontal only
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	// move depending on the view, if view is straight forward, then go up
	// if view is down more then X degrees, start going down
	// if they are back pedalling, then go in reverse of above
	scale = PM_CmdScale( &pm->cmd );
	VectorClear( wishvel );

	if ( pm->cmd.forwardmove ) {
#if 0
		if ( pm->ps->aiChar ) {
			wishvel[2] = 0.5 * upscale * scale * (float)pm->cmd.forwardmove;
		} else { // player speed
#endif
			wishvel[2] = 0.9 * upscale * scale * (float)pm->cmd.forwardmove;
#if 0
		}
#endif
	}
	//Com_Printf("wishvel[2] = %i, fwdmove = %i\n", (int)wishvel[2], (int)pm->cmd.forwardmove );

	if ( pm->cmd.rightmove ) {
		// strafe, so we can jump off ladder
		vec3_t ladder_right, ang;
		vectoangles( laddervec, ang );
		AngleVectors( ang, NULL, ladder_right, NULL );

		// if we are looking away from the ladder, reverse the right vector
		if ( DotProduct( laddervec, pml.forward ) > 0 ) {
			VectorInverse( ladder_right );
		}

		VectorMA( wishvel, 0.5 * scale * (float)pm->cmd.rightmove, pml.right, wishvel );
	}

	// do strafe friction
	PM_Friction();

	wishspeed = VectorNormalize2( wishvel, wishdir );

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );
	if ( !wishvel[2] ) {
		if ( pm->ps->velocity[2] > 0 ) {
			pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
			if ( pm->ps->velocity[2] < 0 ) {
				pm->ps->velocity[2] = 0;
			}
		} else {
			pm->ps->velocity[2] += pm->ps->gravity * pml.frametime;
			if ( pm->ps->velocity[2] > 0 ) {
				pm->ps->velocity[2] = 0;
			}
		}
	}

	//Com_Printf("vel[2] = %i\n", (int)pm->ps->velocity[2] );

	PM_StepSlideMove( qfalse );  // no gravity while going up ladder

	// always point legs forward
	pm->ps->movementDir = 0;
}


/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid( trace_t* trace ) {
	int			i, j, k;
	vec3_t		point;

	if ( pm->debugLevel ) {
		Com_Printf( "%i:allsolid\n", c_pmove );
	}

	// jitter around
	for ( i = -1; i <= 1; i++ ) {
		for ( j = -1; j <= 1; j++ ) {
			for ( k = -1; k <= 1; k++ ) {
				VectorCopy( pm->ps->origin, point );
				point[0] += (float)i;
				point[1] += (float)j;
				point[2] += (float)k;
				pm->trace( trace, point, pm->ps->mins, pm->ps->maxs, point, pm->ps->playerNum, pm->tracemask );
				if ( !trace->allsolid ) {
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					pm->trace( trace, pm->ps->origin, pm->ps->mins, pm->ps->maxs, point, pm->ps->playerNum, pm->tracemask );
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}


/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void ) {
	trace_t		trace;
	vec3_t		point;

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
		// we just transitioned into freefall
		if ( pm->debugLevel ) {
			Com_Printf( "%i:lift\n", c_pmove );
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( pm->ps->origin, point );
		point[2] -= 64;

		pm->trace( &trace, pm->ps->origin, pm->ps->mins, pm->ps->maxs, point, pm->ps->playerNum, pm->tracemask );
		if ( trace.fraction == 1.0 ) {
			if ( pm->cmd.forwardmove >= 0 ) {
				PM_ForceLegsAnim( LEGS_JUMP );
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} else {
				PM_ForceLegsAnim( LEGS_JUMPB );
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}

		if ( trace.fraction == 1.0 && !(pm->ps->pm_flags & PMF_LADDER) ) {
			if ( pm->cmd.forwardmove >= 0 ) {
				//BG_AnimScriptEvent(pm->ps, ANIM_ET_JUMP, qfalse, qtrue);
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} else {
				//BG_AnimScriptEvent(pm->ps, ANIM_ET_JUMPBK, qfalse, qtrue);
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}


/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace( void ) {
	vec3_t		point;
	trace_t		trace;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25;

	pm->trace( &trace, pm->ps->origin, pm->ps->mins, pm->ps->maxs, point, pm->ps->playerNum, pm->tracemask );
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid ) {
		if ( !PM_CorrectAllSolid( &trace ) )
			return;
	}

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0 ) {
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity[2] > 0 && DotProduct( pm->ps->velocity, trace.plane.normal ) > 10 ) {
		if ( pm->debugLevel ) {
			Com_Printf( "%i:kickoff\n", c_pmove );
		}
		if ( !(pm->ps->pm_flags & PMF_LADDER) ) {
			// go into jump animation
			if ( pm->cmd.forwardmove >= 0 ) {
				PM_ForceLegsAnim( LEGS_JUMP );
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} else {
				PM_ForceLegsAnim( LEGS_JUMPB );
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// slopes that are too steep will not be considered onground
	if ( trace.plane.normal[2] < MIN_WALK_NORMAL ) {
		if ( pm->debugLevel ) {
			Com_Printf( "%i:steep\n", c_pmove );
		}
		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP ) {
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		// just hit the ground
		if ( pm->debugLevel ) {
			Com_Printf( "%i:Land\n", c_pmove );
		}

		PM_CrashLand();

		// don't do landing time if we were just going down a slope
		if ( pml.previous_velocity[2] < -200 ) {
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
//	pm->ps->velocity[2] = 0;

	PM_AddTouchEnt( trace.entityNum );
}


/*
=============
PM_SetWaterLevel	FIXME: avoid this twice?  certainly if not moving
=============
*/
static void PM_SetWaterLevel( void ) {
	vec3_t		point;
	int			cont;
	int			sample1;
	int			sample2;

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MINS_Z + 1;
	cont = pm->pointcontents( point, pm->ps->playerNum );

	if ( cont & MASK_WATER ) {
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + MINS_Z + sample1;
		cont = pm->pointcontents( point, pm->ps->playerNum );
		if ( cont & MASK_WATER ) {
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + MINS_Z + sample2;
			cont = pm->pointcontents( point, pm->ps->playerNum );
			if ( cont & MASK_WATER ) {
				pm->waterlevel = 3;
			}
		}
	}

}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck( void ) {
	trace_t	trace;
	float	bounds = pm->pmove_q2 ? 16 : 15;
#ifdef MISSIONPACK
	if ( pm->ps->powerups[PW_INVULNERABILITY] ) {
		if ( pm->ps->pm_flags & PMF_INVULEXPAND ) {
			// invulnerability sphere has a 42 units radius
			VectorSet( pm->ps->mins, -42, -42, -42 );
			VectorSet( pm->ps->maxs, 42, 42, 42 );
		} else {
			VectorSet( pm->ps->mins, -15, -15, MINS_Z );
			VectorSet( pm->ps->maxs, 15, 15, 16 );
		}
		pm->ps->pm_flags |= PMF_DUCKED;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
		return;
	}
	pm->ps->pm_flags &= ~PMF_INVULEXPAND;
#endif
	pm->ps->mins[0] = -bounds;
	pm->ps->mins[1] = -bounds;

	pm->ps->maxs[0] = bounds;
	pm->ps->maxs[1] = bounds;

	pm->ps->mins[2] = MINS_Z;

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
		return;
	}

	if ( pm->ps->pm_type == PM_DEAD ) {
		pm->ps->maxs[2] = -8;
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}

	if ( pm->cmd.upmove < 0 && !pml.ladder ) {	// duck
		pm->ps->pm_flags |= PMF_DUCKED;
	} else {	// stand up if possible
		if ( pm->ps->pm_flags & PMF_DUCKED ) {
			// try to stand up
			pm->ps->maxs[2] = 32;
			pm->trace( &trace, pm->ps->origin, pm->ps->mins, pm->ps->maxs, pm->ps->origin, pm->ps->playerNum, pm->tracemask );
			if ( !trace.allsolid )
				pm->ps->pm_flags &= ~PMF_DUCKED;
		}
	}

	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		pm->ps->maxs[2] = pm->pmove_q2 ? 4 : 16;
		pm->ps->viewheight = pm->pmove_q2 ? -2 : CROUCH_VIEWHEIGHT;
	} else {
		pm->ps->maxs[2] = 32;
		pm->ps->viewheight = pm->pmove_q2 ? 22 : DEFAULT_VIEWHEIGHT;
	}
}



//===================================================================


/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps( void ) {
	float		bobmove;
	int			old;
	qboolean	footstep;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = sqrt( pm->ps->velocity[0] * pm->ps->velocity[0]
		+ pm->ps->velocity[1] * pm->ps->velocity[1] );

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
#ifdef MISSIONPACK
		if ( pm->ps->powerups[PW_INVULNERABILITY] ) {
			PM_ContinueLegsAnim( LEGS_IDLECR );
		}
#endif
		// airborne leaves position in cycle intact, but doesn't advance
		if ( pm->waterlevel > 1 ) {
			PM_ContinueLegsAnim( LEGS_SWIM );
		}
		return;
	}

	// if not trying to move
	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove ) {
		if ( pm->xyspeed < 5 ) {
			pm->ps->bobCycle = 0;	// start at beginning of cycle again
			if ( pm->ps->pm_flags & PMF_DUCKED ) {
				PM_ContinueLegsAnim( LEGS_IDLECR );
			} else {
				PM_ContinueLegsAnim( LEGS_IDLE );
			}
		}
		return;
	}


	footstep = qfalse;

	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		bobmove = 0.5;	// ducked characters bob much faster
		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			PM_ContinueLegsAnim( LEGS_BACKCR );
		} else {
			PM_ContinueLegsAnim( LEGS_WALKCR );
		}
	} else {
		if ( !(pm->cmd.buttons & BUTTON_WALKING) ) {
			bobmove = 0.4f;	// faster speeds bob faster
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				PM_ContinueLegsAnim( LEGS_BACK );
			} else {
				PM_ContinueLegsAnim( LEGS_RUN );
			}
			footstep = qtrue;
		} else {
			bobmove = 0.3f;	// walking bobs slow
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				PM_ContinueLegsAnim( LEGS_BACKWALK );
			} else {
				PM_ContinueLegsAnim( LEGS_WALK );
			}
		}
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)(old + bobmove * pml.msec) & 255;

	// if we just crossed a cycle boundary, play an appropriate footstep event
	if ( ((old + 64) ^ (pm->ps->bobCycle + 64)) & 128 ) {
		if ( pm->waterlevel == 0 ) {
			// on ground will only play sounds if running
			if ( footstep && !pm->noFootsteps ) {
				PM_AddEvent( PM_FootstepForSurface() );
			}
		} else if ( pm->waterlevel == 1 ) {
			// splashing
			PM_AddEvent( EV_FOOTSPLASH );
		} else if ( pm->waterlevel == 2 ) {
			// wading / swimming at surface
			PM_AddEvent( EV_SWIM );
		} else if ( pm->waterlevel == 3 ) {
			// no sound when completely underwater

		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents( void ) {		// FIXME?
	//
	// if just entered a water volume, play a sound
	//
	if ( !pml.previous_waterlevel && pm->waterlevel ) {
		PM_AddEvent( EV_WATER_TOUCH );
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if ( pml.previous_waterlevel && !pm->waterlevel ) {
		PM_AddEvent( EV_WATER_LEAVE );
	}

	//
	// check for head just going under water
	//
	if ( pml.previous_waterlevel != 3 && pm->waterlevel == 3 ) {
		PM_AddEvent( EV_WATER_UNDER );
	}

	//
	// check for head just coming out of water
	//
	if ( pml.previous_waterlevel == 3 && pm->waterlevel != 3 ) {
		PM_AddEvent( EV_WATER_CLEAR );
	}
}


/*
===============
PM_BeginWeaponChange
===============
*/
static void PM_BeginWeaponChange( int weapon ) {
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		return;
	}

	if ( !(pm->ps->stats[STAT_WEAPONS] & (1 << weapon)) ) {
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		return;
	}

	PM_AddEvent( EV_CHANGE_WEAPON );
	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += 200;
	PM_StartTorsoAnim( TORSO_DROP );
}


/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange( void ) {
	int		weapon;

	BG_DecomposeUserCmdValue( pm->cmd.stateValue, &weapon );
	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		weapon = WP_NONE;
	}

	if ( !(pm->ps->stats[STAT_WEAPONS] & (1 << weapon)) ) {
		weapon = WP_NONE;
	}

	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	pm->ps->weaponTime += 250;
	PM_StartTorsoAnim( TORSO_RAISE );
}


/*
==============
PM_TorsoAnimation

==============
*/
static void PM_TorsoAnimation( void ) {
	if ( pm->ps->weaponstate == WEAPON_READY ) {
		if ( pm->ps->weapon == WP_GAUNTLET ) {
			PM_ContinueTorsoAnim( TORSO_STAND2 );
		} else {
			PM_ContinueTorsoAnim( TORSO_STAND );
		}
		return;
	}
}


/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
static void PM_Weapon( const int dmFlags, const int warmupState ) {
	int		addTime;
	int		newWeapon;

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return;
	}

	// ignore if spectator
	if ( pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	// check for dead player
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->ps->weapon = WP_NONE;
		return;
	}

	// check for item using
	if ( pm->cmd.buttons & BUTTON_USE_HOLDABLE ) {
		if ( !(pm->ps->pm_flags & PMF_USE_ITEM_HELD) ) {
			int itemnum = BG_ItemForItemNum( pm->ps->stats[STAT_HOLDABLE_ITEM] )->giTag;
			if ( itemnum == HI_MEDKIT && pm->ps->stats[STAT_HEALTH] >= (pm->ps->stats[STAT_MAX_HEALTH] + 25) ) {
				// don't use medkit if at max health
			} else if ( itemnum == HI_PSCREEN || itemnum == HI_PSHIELD ) {
				if ( !pm->ps->stats[STAT_PARMOR_ACTIVE] ) {
					if ( (pm->ps->ammo[WP_PLASMAGUN] || pm->ps->ammo[WP_BFG]) ) {	// turn on
						pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
						PM_AddEvent( (itemnum == HI_PSCREEN) ? EV_PARMOR_SCREEN_ON : EV_PARMOR_SHIELD_ON );
						pm->ps->stats[STAT_PARMOR_ACTIVE] = itemnum;
					} else {	// no ammo
						pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
						PM_AddEvent( EV_USE_ITEM0 + itemnum );
						pm->ps->stats[STAT_PARMOR_ACTIVE] = 0;
					}
				} else {	// turn it off
					pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
					PM_AddEvent( (itemnum == HI_PSCREEN) ? EV_PARMOR_SCREEN_OFF : EV_PARMOR_SHIELD_OFF );
					pm->ps->stats[STAT_PARMOR_ACTIVE] = 0;
				}
				
			} else {
				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
				PM_AddEvent( EV_USE_ITEM0 + itemnum );
				pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
			}
			return;
		}
	} else {
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
	}


	// make weapon function
	if ( pm->ps->weaponTime > 0 ) {
		pm->ps->weaponTime -= pml.msec;
	}

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if ( pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING ) {
		BG_DecomposeUserCmdValue( pm->cmd.stateValue, &newWeapon );
		if ( pm->ps->weapon != newWeapon ) {
			PM_BeginWeaponChange( newWeapon );
		}
	}

	if ( pm->ps->weaponTime > 0 ) {
		return;
	}

	// change weapon if time
	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		PM_FinishWeaponChange();
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_RAISING ) {
		pm->ps->weaponstate = WEAPON_READY;
		if ( pm->ps->weapon == WP_GAUNTLET ) {
			PM_StartTorsoAnim( TORSO_STAND2 );
		} else {
			PM_StartTorsoAnim( TORSO_STAND );
		}
		return;
	}

	// no firing during countdown time
	if ( warmupState == WARMUP_COUNTDOWN ) {
		return;
	}

	// check for fire
	if ( !(pm->cmd.buttons & BUTTON_ATTACK) ) {
		pm->ps->weaponTime = 0;
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	// start the animation even if out of ammo
	if ( pm->ps->weapon == WP_GAUNTLET ) {
		// the guantlet only "fires" when it actually hits something
		if ( !pm->gauntletHit ) {
			pm->ps->weaponTime = 0;
			pm->ps->weaponstate = WEAPON_READY;
			return;
		}
		PM_StartTorsoAnim( TORSO_ATTACK2 );
	} else {
		PM_StartTorsoAnim( TORSO_ATTACK );
	}

	pm->ps->weaponstate = WEAPON_FIRING;

	// check for out of ammo
	if ( !pm->ps->ammo[pm->ps->weapon] && !(dmFlags & DF_INFINITE_AMMO) ) {
		PM_AddEvent( EV_NOAMMO );
		pm->ps->weaponTime += 500;
		return;
	}

	// take an ammo away if not infinite
	if ( pm->ps->ammo[pm->ps->weapon] != AMMO_INFINITE && !(dmFlags & DF_INFINITE_AMMO) ) {
		pm->ps->ammo[pm->ps->weapon]--;
	}

	// fire weapon
	PM_AddEvent( EV_FIRE_WEAPON );

	switch ( pm->ps->weapon ) {
	default:
	case WP_GAUNTLET:
		addTime = 400;
		break;
	case WP_MACHINEGUN:
		addTime = 100;
		break;
	case WP_SHOTGUN:
		addTime = 1000;
		break;
	case WP_GRENADE_LAUNCHER:
		addTime = 800;
		break;
	case WP_ROCKET_LAUNCHER:
		addTime = 800;
		break;
	case WP_LIGHTNING:
		addTime = 50;
		break;
	case WP_RAILGUN:
		addTime = 1500;
		break;
	case WP_PLASMAGUN:
		addTime = 100;
		break;
	case WP_BFG:
		addTime = 200;
		break;
	case WP_GRAPPLING_HOOK:
		addTime = 400;
		break;
#ifdef MISSIONPACK
	case WP_NAILGUN:
		addTime = 1000;
		break;
	case WP_PROX_LAUNCHER:
		addTime = 800;
		break;
	case WP_CHAINGUN:
		addTime = 30;
		break;
#endif
	}

	if ( BG_ItemForItemNum( pm->ps->stats[STAT_RUNE] )->giTag == PW_ARMAMENT ) {
		addTime *= 0.75;
	}
	if ( pm->ps->powerups[PW_HASTE] ) {
		addTime *= 0.75;
	}

	pm->ps->weaponTime += addTime;
}

/*
================
PM_Animate
================
*/

static void PM_Animate( void ) {
	if ( pm->cmd.buttons & BUTTON_GESTURE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GESTURE );
			pm->ps->torsoTimer = TIMER_GESTURE;
			PM_AddEvent( EV_TAUNT );
		}
#ifdef MISSIONPACK
	} else if ( pm->cmd.buttons & BUTTON_GETFLAG ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GETFLAG );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_GUARDBASE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GUARDBASE );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_PATROL ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_PATROL );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_FOLLOWME ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_FOLLOWME );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_AFFIRMATIVE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_AFFIRMATIVE );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_NEGATIVE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_NEGATIVE );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
#endif
	}
}


/*
================
PM_DropTimers
================
*/
static void PM_DropTimers( void ) {
	// drop misc timing counter
	if ( pm->ps->pm_time ) {
		if ( pml.msec >= pm->ps->pm_time ) {
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		} else {
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if ( pm->ps->legsTimer > 0 ) {
		pm->ps->legsTimer -= pml.msec;
		if ( pm->ps->legsTimer < 0 ) {
			pm->ps->legsTimer = 0;
		}
	}

	if ( pm->ps->torsoTimer > 0 ) {
		pm->ps->torsoTimer -= pml.msec;
		if ( pm->ps->torsoTimer < 0 ) {
			pm->ps->torsoTimer = 0;
		}
	}
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated instead of a full move
================
*/
void PM_UpdateViewAngles( playerState_t* ps, const usercmd_t* cmd ) {
	short		temp;
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION ) {
		return;		// no view changes at all
	}

	if ( ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0 ) {
		return;		// no view changes at all
	}

	// circularly clamp the angles with deltas
	for ( i = 0; i < 3; i++ ) {
		temp = cmd->angles[i] + ps->delta_angles[i];
		if ( i == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) {
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			} else if ( temp < -16000 ) {
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE( temp );
	}

}


/*
================
PmoveSingle

================
*/
void PmoveSingle( pmove_t* pmove, const int dmFlags, const int warmupState ) {
	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint for the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->tracemask &= ~CONTENTS_BODY;	// corpses can fly through bodies
	}

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if ( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 ) {
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}

	// set the talk balloon flag
	if ( pm->cmd.buttons & BUTTON_TALK ) {
		pm->ps->eFlags |= EF_TALK;
	} else {
		pm->ps->eFlags &= ~EF_TALK;
	}

	// set the firing flag for continuous beam weapons
	if ( !(pm->ps->pm_flags & PMF_RESPAWNED) && pm->ps->pm_type != PM_INTERMISSION && warmupState != WARMUP_COUNTDOWN
		&& (pm->cmd.buttons & BUTTON_ATTACK) && ( pm->ps->ammo[pm->ps->weapon] || (dmFlags & DF_INFINITE_AMMO) ) ) {
		pm->ps->eFlags |= EF_FIRING;
	} else {
		pm->ps->eFlags &= ~EF_FIRING;
	}

	// clear the respawned flag if attack and use are cleared
	if ( pm->ps->stats[STAT_HEALTH] > 0 &&
		!(pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE)) ) {
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( pmove->cmd.buttons & BUTTON_TALK ) {
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset( &pml, 0, sizeof( pml ) );

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if ( pml.msec < 1 ) {
		pml.msec = 1;
	} else if ( pml.msec > 200 ) {
		pml.msec = 200;
	}
	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy( pm->ps->origin, pml.previous_origin );

	// save old velocity for crashlanding
	VectorCopy( pm->ps->velocity, pml.previous_velocity );

	pml.frametime = pml.msec * 0.001;

	// update the viewangles
	PM_UpdateViewAngles( pm->ps, &pm->cmd );

	AngleVectors( pm->ps->viewangles, pml.forward, pml.right, pml.up );

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
	if ( pm->cmd.forwardmove < 0 ) {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	} else if ( pm->cmd.forwardmove > 0 || (pm->cmd.forwardmove == 0 && pm->cmd.rightmove) ) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if ( pm->ps->pm_type >= PM_DEAD ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
		PM_CheckDuck();
		PM_FlyMove();
		PM_DropTimers();
		return;
	}

	if ( pm->ps->pm_type == PM_NOCLIP ) {
		PM_NoclipMove();
		PM_DropTimers();
		PM_Weapon( dmFlags, warmupState );
		return;
	}

	if ( pm->ps->pm_type == PM_FREEZE ) {
		return;		// no movement at all
	}

	if ( pm->ps->pm_type == PM_INTERMISSION || pm->ps->pm_type == PM_SPINTERMISSION ) {
		return;		// no movement at all
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	PM_CheckDuck();

	// set groundentity
	PM_GroundTrace();

	if ( pm->ps->pm_type == PM_DEAD ) {
		PM_DeadMove();
	}

	// Ridah, ladders
	PM_CheckLadderMove();

	PM_DropTimers();

#ifdef MISSIONPACK
	if ( pm->ps->powerups[PW_INVULNERABILITY] ) {
		PM_InvulnerabilityMove();
	} else
#endif
		if ( pm->ps->pm_flags & PMF_GRAPPLE_PULL ) {
			PM_GrappleMove();
			// We can wiggle a bit
			PM_AirMove( DEFAULT_GRAVITY );
		// Ridah, ladders
		} else if ( pml.ladder ) {
			PM_LadderMove();
			// done.
		} else if ( pm->ps->powerups[PW_FLIGHT] ) {
			// flight powerup doesn't allow jump and has different friction
			PM_FlyMove();
		} else if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP ) {
			PM_WaterJumpMove();
		} else if ( pm->waterlevel > 1 ) {
			// swimming
			PM_WaterMove();
		} else if ( pml.walking ) {
			// walking on ground
			PM_WalkMove();
		} else {
			// airborne
			PM_AirMove( DEFAULT_GRAVITY );
		}

		PM_Animate();

		// set groundentity, watertype, and waterlevel
		PM_GroundTrace();
		PM_SetWaterLevel();

		// weapons
		PM_Weapon( dmFlags, warmupState );

		// torso animation
		PM_TorsoAnimation();

		// footstep events / legs animations
		PM_Footsteps();

		// entering / leaving water splashes
		PM_WaterEvents();

		// snap some parts of playerstate to save network bandwidth
		trap_SnapVector( pm->ps->velocity );
}


/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove( pmove_t* pmove, const int dmFlags, const int warmupState ) {
	int			finalTime;

	finalTime = pmove->cmd.serverTime;

	if ( finalTime < pmove->ps->commandTime ) {
		return;	// should not happen
	}

	if ( finalTime > pmove->ps->commandTime + 1000 ) {
		pmove->ps->commandTime = finalTime - 1000;
	}

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount + 1) & ((1 << PS_PMOVEFRAMECOUNTBITS) - 1);

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while ( pmove->ps->commandTime != finalTime ) {
		int		msec;

		msec = finalTime - pmove->ps->commandTime;

		if ( pmove->pmove_fixed ) {
			if ( msec > pmove->pmove_msec ) {
				msec = pmove->pmove_msec;
			}
		} else {
			if ( msec > 66 ) {
				msec = 66;
			}
		}
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle( pmove, dmFlags, warmupState );

		if ( pmove->ps->pm_flags & PMF_JUMP_HELD ) {
			pmove->cmd.upmove = 20;
		}
	}

	//PM_CheckStuck();

}

