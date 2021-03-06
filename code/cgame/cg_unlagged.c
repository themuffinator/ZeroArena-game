/*
===========================================================================
Copyright (C) 2006 Neil Toronto.

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

#include "cg_local.h"

#if 0

// we'll need this
#define MACHINEGUN_SPREAD	200

/*
=======================
CG_PredictWeaponEffects

Draws predicted effects for the railgun, shotgun, and machinegun.  The
lightning gun is done in CG_LightningBolt, since it was just a matter
of setting the right origin and angles.
=======================
*/
void CG_PredictWeaponEffects( centity_t *cent ) {
	vec3_t		muzzlePoint, forward, right, up;
	entityState_t *ent = &cent->currentState;

	// if the client isn't us, forget it
	if ( cent->currentState.number != cg.predictedPlayerState.clientNum ) {
		return;
	}

	// if it's not switched on server-side, forget it
	if ( !cgs.delagHitscan ) {
		return;
	}

	// get the muzzle point
	VectorCopy( cg.predictedPlayerState.origin, muzzlePoint );
	muzzlePoint[2] += cg.predictedPlayerState.viewheight;

	// get forward, right, and up
	AngleVectors( cg.predictedPlayerState.viewangles, forward, right, up );
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );

	// was it a rail attack?
	if ( ent->weapon == WP_RAILGUN ) {
		// do we have it on for the rail gun?
		if ( cg_delag.integer & 1 || cg_delag.integer & 16 ) {
			trace_t trace;
			vec3_t endPoint;

			// trace forward
			VectorMA( muzzlePoint, 8192, forward, endPoint );

			// THIS IS FOR DEBUGGING!
			// you definitely *will* want something like this to test the backward reconciliation
			// to make sure it's working *exactly* right
			if ( cg_debugDelag.integer ) {
				// trace forward
				CG_Trace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint, cent->currentState.number, CONTENTS_BODY|CONTENTS_SOLID );

				// did we hit another player?
				if ( trace.fraction < 1.0f && (trace.contents & CONTENTS_BODY) ) {
					// if we have two snapshots (we're interpolating)
					if ( cg.nextSnap ) {
						centity_t *c = &cg_entities[trace.entityNum];
						vec3_t origin1, origin2;

						// figure the two origins used for interpolation
						BG_EvaluateTrajectory( &c->currentState.pos, cg.snap->serverTime, origin1, cgs.gravity );
						BG_EvaluateTrajectory( &c->nextState.pos, cg.nextSnap->serverTime, origin2, cgs.gravity );

						// print some debugging stuff exactly like what the server does

						// it starts with "Int:" to let you know the target was interpolated
						CG_Printf("^3Int: time: %d, j: %d, k: %d, origin: %0.2f %0.2f %0.2f\n",
								cg.oldTime, cg.snap->serverTime, cg.nextSnap->serverTime,
								c->lerpOrigin[0], c->lerpOrigin[1], c->lerpOrigin[2]);
						CG_Printf("^5frac: %0.4f, origin1: %0.2f %0.2f %0.2f, origin2: %0.2f %0.2f %0.2f\n",
							cg.frameInterpolation, origin1[0], origin1[1], origin1[2], origin2[0], origin2[1], origin2[2]);
					}
					else {
						// we haven't got a next snapshot
						// the client clock has either drifted ahead (seems to happen once per server frame
						// when you play locally) or the client is using timenudge
						// in any case, CG_CalcEntityLerpPositions extrapolated rather than interpolated
						centity_t *c = &cg_entities[trace.entityNum];
						vec3_t origin1, origin2;

						c->currentState.pos.trTime = TR_LINEAR_STOP;
						c->currentState.pos.trTime = cg.snap->serverTime;
						c->currentState.pos.trDuration = 1000 / sv_fps.integer;

						BG_EvaluateTrajectory( &c->currentState.pos, cg.snap->serverTime, origin1, cgs.gravity );
						BG_EvaluateTrajectory( &c->currentState.pos, cg.snap->serverTime + 1000 / sv_fps.integer, origin2, cgs.gravity );

						// print some debugging stuff exactly like what the server does

						// it starts with "Ext:" to let you know the target was extrapolated
						CG_Printf("^3Ext: time: %d, j: %d, k: %d, origin: %0.2f %0.2f %0.2f\n",
								cg.oldTime, cg.snap->serverTime, cg.snap->serverTime,
								c->lerpOrigin[0], c->lerpOrigin[1], c->lerpOrigin[2]);
						CG_Printf("^5frac: %0.4f, origin1: %0.2f %0.2f %0.2f, origin2: %0.2f %0.2f %0.2f\n",
							cg.frameInterpolation, origin1[0], origin1[1], origin1[2], origin2[0], origin2[1], origin2[2]);
					}
				}
			}

			// find the rail's end point
			CG_Trace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint, cg.predictedPlayerState.clientNum, CONTENTS_SOLID );

			// do the magic-number adjustment
			VectorMA( muzzlePoint, 4, right, muzzlePoint );
			VectorMA( muzzlePoint, -1, up, muzzlePoint );

			// draw a rail trail
			CG_RailTrail( &cgs.clientinfo[cent->currentState.number], muzzlePoint, trace.endpos );
			//Com_Printf( "Predicted rail trail\n" );

			// explosion at end if not SURF_NOIMPACT
			if ( !(trace.surfaceFlags & SURF_NOIMPACT) ) {
				// predict an explosion
				CG_MissileHitWall( ent->weapon, cg.predictedPlayerState.clientNum, trace.endpos, trace.plane.normal, IMPACTSOUND_DEFAULT );
			}
		}
	}
	// was it a shotgun attack?
	else if ( ent->weapon == WP_SHOTGUN ) {
		// do we have it on for the shotgun?
		if ( cg_delag.integer & 1 || cg_delag.integer & 4 ) {
			int contents;
			vec3_t endPoint, v;

			// do everything like the server does

			SnapVector( muzzlePoint );

			VectorScale( forward, 4096, endPoint );
			SnapVector( endPoint );

			VectorSubtract( endPoint, muzzlePoint, v );
			VectorNormalize( v );
			VectorScale( v, 32, v );
			VectorAdd( muzzlePoint, v, v );

			if ( cgs.glconfig.hardwareType != GLHW_RAGEPRO ) {
				// ragepro can't alpha fade, so don't even bother with smoke
				vec3_t			up;

				contents = trap_CM_PointContents( muzzlePoint, 0 );
				if ( !( contents & CONTENTS_WATER ) ) {
					VectorSet( up, 0, 0, 8 );
					CG_SmokePuff( v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader );
				}
			}

			// do the shotgun pellets
			CG_ShotgunPattern( muzzlePoint, endPoint, cg.oldTime % 256, cg.predictedPlayerState.clientNum );
			//Com_Printf( "Predicted shotgun pattern\n" );
		}
	}
	// was it a machinegun attack?
	else if ( ent->weapon == WP_MACHINEGUN ) {
		// do we have it on for the machinegun?
		if ( cg_delag.integer & 1 || cg_delag.integer & 2 ) {
			// the server will use this exact time (it'll be serverTime on that end)
			int seed = cg.oldTime % 256;
			float r, u;
			trace_t tr;
			qboolean flesh;
			int fleshEntityNum;
			vec3_t endPoint;

			// do everything exactly like the server does

			r = Q_random(&seed) * M_PI * 2.0f;
			u = sin(r) * Q_crandom(&seed) * MACHINEGUN_SPREAD * 16;
			r = cos(r) * Q_crandom(&seed) * MACHINEGUN_SPREAD * 16;

			VectorMA( muzzlePoint, 8192*16, forward, endPoint );
			VectorMA( endPoint, r, right, endPoint );
			VectorMA( endPoint, u, up, endPoint );

			CG_Trace(&tr, muzzlePoint, NULL, NULL, endPoint, cg.predictedPlayerState.clientNum, MASK_SHOT );

			if ( tr.surfaceFlags & SURF_NOIMPACT ) {
				return;
			}

			// snap the endpos to integers, but nudged towards the line
			SnapVectorTowards( tr.endpos, muzzlePoint );

			// do bullet impact
			if ( tr.entityNum < MAX_CLIENTS ) {
				flesh = qtrue;
				fleshEntityNum = tr.entityNum;
			} else {
				flesh = qfalse;
			}

			// do the bullet impact
			CG_Bullet( tr.endpos, cg.predictedPlayerState.clientNum, tr.plane.normal, flesh, fleshEntityNum );
			//Com_Printf( "Predicted bullet\n" );
		}
	}
}

#endif

/*
=================
CG_DrawBBox

Draws a bounding box around an entity.
=================
*/
void CG_DrawBBox( centity_t *cent, float *color ) {
	polyVert_t verts[4];
	int i;
	vec3_t mins;
	vec3_t maxs;
	float extx, exty, extz;
	vec3_t corners[8];

	if ( !cg_drawBBox.integer ) {
		return;
	}

	VectorCopy( cent->currentState.mins, mins );
	VectorCopy( cent->currentState.maxs, maxs );

	// get the extents (size)
	extx = maxs[0] - mins[0];
	exty = maxs[1] - mins[1];
	extz = maxs[2] - mins[2];

	// set the polygon's texture coordinates
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;

	// set the polygon's vertex colors
	for ( i = 0; i < 4; i++ ) {
		verts[i].modulate[0] = 0xFF * color[0];
		verts[i].modulate[1] = 0xFF * color[1];
		verts[i].modulate[2] = 0xFF * color[2];
		verts[i].modulate[3] = 0xFF * color[3];
	}

	VectorAdd( cent->lerpOrigin, maxs, corners[3] );

	VectorCopy( corners[3], corners[2] );
	corners[2][0] -= extx;

	VectorCopy( corners[2], corners[1] );
	corners[1][1] -= exty;

	VectorCopy( corners[1], corners[0] );
	corners[0][0] += extx;

	for ( i = 0; i < 4; i++ ) {
		VectorCopy( corners[i], corners[i + 4] );
		corners[i + 4][2] -= extz;
	}

	// top
	VectorCopy( corners[0], verts[0].xyz );
	VectorCopy( corners[1], verts[1].xyz );
	VectorCopy( corners[2], verts[2].xyz );
	VectorCopy( corners[3], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.whiteDynamicShader, 4, verts, 0, 0 );

	// bottom
	VectorCopy( corners[7], verts[0].xyz );
	VectorCopy( corners[6], verts[1].xyz );
	VectorCopy( corners[5], verts[2].xyz );
	VectorCopy( corners[4], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.whiteDynamicShader, 4, verts, 0, 0 );

	// top side
	VectorCopy( corners[3], verts[2].xyz );
	VectorCopy( corners[2], verts[3].xyz );
	VectorCopy( corners[6], verts[0].xyz );
	VectorCopy( corners[7], verts[1].xyz );
	trap_R_AddPolyToScene( cgs.media.whiteDynamicShader, 4, verts, 0, 0 );

	// left side
	VectorCopy( corners[2], verts[2].xyz );
	VectorCopy( corners[1], verts[3].xyz );
	VectorCopy( corners[5], verts[0].xyz );
	VectorCopy( corners[6], verts[1].xyz );
	trap_R_AddPolyToScene( cgs.media.whiteDynamicShader, 4, verts, 0, 0 );

	// right side
	VectorCopy( corners[0], verts[2].xyz );
	VectorCopy( corners[3], verts[3].xyz );
	VectorCopy( corners[7], verts[0].xyz );
	VectorCopy( corners[4], verts[1].xyz );
	trap_R_AddPolyToScene( cgs.media.whiteDynamicShader, 4, verts, 0, 0 );

	// bottom side
	VectorCopy( corners[1], verts[2].xyz );
	VectorCopy( corners[0], verts[3].xyz );
	VectorCopy( corners[4], verts[0].xyz );
	VectorCopy( corners[5], verts[1].xyz );
	trap_R_AddPolyToScene( cgs.media.whiteDynamicShader, 4, verts, 0, 0 );
}

