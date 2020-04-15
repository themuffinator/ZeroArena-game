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

/*****************************************************************************
 * name:		ai_vcmd.c
 *
 * desc:		Quake3 bot AI
 *
 * $Archive: /MissionPack/code/game/ai_vcmd.c $
 *
 *****************************************************************************/

#include "g_local.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas.h"
//
#include "ai_char.h"
#include "ai_chat_sys.h"
#include "ai_ea.h"
#include "ai_gen.h"
#include "ai_goal.h"
#include "ai_move.h"
#include "ai_weap.h"
#include "ai_weight.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_vcmd.h"
#include "ai_dmnet.h"
#include "ai_team.h"
//
#include "chars.h"				//characteristics
#include "inv.h"				//indexes into the inventory
#include "syn.h"				//synonyms
#include "match.h"				//string matching types and vars

// for the voice chats
#include "../../ui/menudef.h"

/*
==================
//BotVoiceChat_GetFlag
==================
*/
void BotVoiceChat_GetFlag(bot_state_t *bs, int playernum, int mode) {

}

/*
==================
//BotVoiceChat_Offense
==================
*/
void BotVoiceChat_Offense(bot_state_t *bs, int playernum, int mode) {

}

/*
==================
//BotVoiceChat_Defend
==================
*/
void BotVoiceChat_Defend(bot_state_t *bs, int playernum, int mode) {

}

/*
==================
//BotVoiceChat_DefendFlag
==================
*/
void BotVoiceChat_DefendFlag(bot_state_t *bs, int playernum, int mode) {

}

/*
==================
//BotVoiceChat_Patrol
==================
*/
void BotVoiceChat_Patrol(bot_state_t *bs, int playernum, int mode) {

}

/*
==================
//BotVoiceChat_Camp
==================
*/
void BotVoiceChat_Camp(bot_state_t *bs, int playernum, int mode) {

}

/*
==================
//BotVoiceChat_FollowMe
==================
*/
void BotVoiceChat_FollowMe(bot_state_t *bs, int playernum, int mode) {

}

/*
==================
//BotVoiceChat_FollowFlagCarrier
==================
*/
void BotVoiceChat_FollowFlagCarrier(bot_state_t *bs, int playernum, int mode) {

}

/*
==================
//BotVoiceChat_ReturnFlag
==================
*/
void BotVoiceChat_ReturnFlag(bot_state_t *bs, int playernum, int mode) {

}

/*
==================
//BotVoiceChat_StartLeader
==================
*/
void BotVoiceChat_StartLeader(bot_state_t *bs, int playernum, int mode) {

}

/*
==================
//BotVoiceChat_StopLeader
==================
*/
void BotVoiceChat_StopLeader(bot_state_t *bs, int playernum, int mode) {

}

/*
==================
//BotVoiceChat_WhoIsLeader
==================
*/
void BotVoiceChat_WhoIsLeader(bot_state_t *bs, int playernum, int mode) {

}

/*
==================
//BotVoiceChat_WantOnDefense
==================
*/
void BotVoiceChat_WantOnDefense(bot_state_t *bs, int playernum, int mode) {

}

/*
==================
//BotVoiceChat_WantOnOffense
==================
*/
void BotVoiceChat_WantOnOffense(bot_state_t *bs, int playernum, int mode) {

}

void BotVoiceChat_Dummy(bot_state_t *bs, int playernum, int mode) {
}

int BotVoiceChatCommand(bot_state_t *bs, int mode, char *voiceChat) {
    return 0;
}
