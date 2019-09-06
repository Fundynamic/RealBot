/**
  * RealBot : Artificial Intelligence
  * Version : Work In Progress
  * Author  : Stefan Hendriks
  * Url     : http://realbot.bots-united.com
  **
  * DISCLAIMER
  *
  * History, Information & Credits: 
  * RealBot is based partially upon the HPB-Bot Template #3 by Botman
  * Thanks to Ditlew (NNBot), Pierre Marie Baty (RACCBOT), Tub (RB AI PR1/2/3)
  * Greg Slocum & Shivan (RB V1.0), Botman (HPB-Bot) and Aspirin (JOEBOT). And
  * everybody else who helped me with this project.
  * Storage of Visibility Table using BITS by Cheesemonster.
  *
  * Some portions of code are from other bots, special thanks (and credits) go
  * to (in no specific order):
  *
  * Pierre Marie Baty
  * Count-Floyd
  *  
  * !! BOTS-UNITED FOREVER !!
  *  
  * This project is open-source, it is protected under the GPL license;
  * By using this source-code you agree that you will ALWAYS release the
  * source-code with your project.
  *
  **/


#include <string.h>
#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>
#include <entity_state.h>

#include "bot.h"
#include "game.h"
#include "bot_weapons.h"
#include "bot_func.h"
#include "NodeMachine.h"

extern cBot bots[32];
extern edict_t *listenserver_edict;
extern bool bombplanted;
extern int mod_id;

extern int m_spriteTexture;

// defined in dll.cpp
extern FILE *fpRblog;

//
extern cNodeMachine NodeMachine;

// For taking cover decision

#define TOTAL_SCORE 16300       // 16000 money + 100 health + 100 fear + 100 camp desire

bool
VectorIsVisibleWithEdict(edict_t *pEdict, Vector dest, char *checkname) {
    TraceResult tr;

    Vector start = pEdict->v.origin + pEdict->v.view_ofs;

    // trace a line from bot's eyes to destination...
    UTIL_TraceLine(start, dest, ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

    // When our check string is not "none" and the traceline has a hit...
    if (strcmp("none", checkname) != 0 && tr.flFraction < 1.0) {
        // Check if the blocking entity is same as checkname..
        char entity_blocker[128];
        edict_t *pent = tr.pHit;  // Ok now retrieve the entity
        strcpy(entity_blocker, STRING(pent->v.classname));        // the classname

        if (strcmp(entity_blocker, checkname) == 0)
            return true;           // We are blocked by our string, this means its ok.
        else {
            return false;          // We are blocked, but by something differernt then 'checkname' its not ok
        }

    } else {
        // check if line of sight to object is not blocked (i.e. visible)
        if (tr.flFraction >= 1.0)
            return true;
        else
            return false;
    }

    return false;

}

bool VectorIsVisible(Vector start, Vector dest, char *checkname) {
    TraceResult tr;

    // trace a line from bot's eyes to destination...
    UTIL_TraceLine(start, dest, dont_ignore_monsters, NULL, &tr);

    // Als we geblokt worden EN we checken voor een naam
    if (strcmp("none", checkname) != 0 && tr.flFraction < 1.0) {
        // Check if the blocking entity is same as checkname..
        char entity_blocker[128];
        edict_t *pent = tr.pHit;  // Ok now retrieve the entity
        strcpy(entity_blocker, STRING(pent->v.classname));        // the classname

        if (strcmp(entity_blocker, checkname) == 0)
            return false;          // We worden geblokt door die naam..
        else
            return true;           // We worden NIET geblokt door die naam (dus we worden niet geblokt).

    } else {
        // check if line of sight to object is not blocked (i.e. visible)
        // Als er NONE wordt opgegeven dan checken we gewoon of we worden geblokt
        if (tr.flFraction >= 1.0)
            return TRUE;
        else
            return FALSE;

    }

}

float func_distance(Vector v1, Vector v2) {
    // Returns distance between 2 vectors
    if (v1 && v2)
        return (v1 - v2).Length();
    else
        return 0.0;
}

int FUNC_InFieldOfView(edict_t *pEntity, Vector dest) {
    // NOTE: Copy from Botman's BotInFieldOfView() routine.
    // find angles from source to destination...
    Vector entity_angles = UTIL_VecToAngles(dest);

    // make yaw angle 0 to 360 degrees if negative...
    if (entity_angles.y < 0)
        entity_angles.y += 360;

    // get bot's current view angle...
    float view_angle = pEntity->v.v_angle.y;

    // make view angle 0 to 360 degrees if negative...
    if (view_angle < 0)
        view_angle += 360;

    // return the absolute value of angle to destination entity
    // zero degrees means straight ahead,  45 degrees to the left or
    // 45 degrees to the right is the limit of the normal view angle

    // rsm - START angle bug fix
    int angle = abs((int) view_angle - (int) entity_angles.y);

    if (angle > 180)
        angle = 360 - angle;

    return angle;
    // rsm - END
}

// Draw waypoint beam?
void
WaypointDrawBeam(edict_t *pEntity, Vector start, Vector end, int width,
                 int noise, int red, int green, int blue, int brightness,
                 int speed) {
    MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pEntity);
    WRITE_BYTE(TE_BEAMPOINTS);
    WRITE_COORD(start.x);
    WRITE_COORD(start.y);
    WRITE_COORD(start.z);
    WRITE_COORD(end.x);
    WRITE_COORD(end.y);
    WRITE_COORD(end.z);
    WRITE_SHORT(m_spriteTexture);
    WRITE_BYTE(1);               // framestart
    WRITE_BYTE(10);              // framerate
    WRITE_BYTE(10);              // life in 0.1's
    WRITE_BYTE(width);           // width
    WRITE_BYTE(noise);           // noise

    WRITE_BYTE(red);             // r, g, b
    WRITE_BYTE(green);           // r, g, b
    WRITE_BYTE(blue);            // r, g, b

    WRITE_BYTE(brightness);      // brightness
    WRITE_BYTE(speed);           // speed
    MESSAGE_END();
}

// Function takes care of near bot players, returns most 'closest in FOV' bot pointer
cBot *search_near_players(cBot *pBot) {
    // This function will look for players or bots near
    // Any bot or player near will make the bot slow down, and sometimes even wait
    // So bots don't interfere with their paths and dont mess up their learning

    edict_t *pEdict = pBot->pEdict;
    cBot *TheBot = NULL;

    int iClosestAngle = 180;

    // Loop through all clients
    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t *pPlayer = INDEXENT(i);

        // skip invalid players
        if ((pPlayer) && (!pPlayer->free) && (pPlayer != pEdict)) {
            // skip this player if not alive (i.e. dead or dying)
            if (!IsAlive(pPlayer))
                continue;

            // skip anything that is not a RealBot
            if (UTIL_GetBotPointer(pPlayer) == NULL)       // not using FL_FAKECLIENT here so it is multi-bot compatible
                continue;

            int iAngle = FUNC_InFieldOfView(pBot->pEdict,
                                            (pPlayer->v.origin -
                                             pBot->pEdict->v.origin));

            if (func_distance(pBot->pEdict->v.origin, pPlayer->v.origin) < 90
                && iAngle < iClosestAngle) {
                cBot *pBotPointer = UTIL_GetBotPointer(pPlayer);
                iClosestAngle = iAngle;

                if (pBotPointer != NULL)
                    TheBot = pBotPointer;    // set pointer
            }
        }
    }

    return TheBot;               // return result (either NULL or bot pointer)
}

// Return TRUE of any players are near that could block him
bool BOOL_search_near_players(cBot *pBot) {
    edict_t *pEdict = pBot->pEdict;
    int iClosestAngle = 180;

    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t *pPlayer = INDEXENT(i);

        // skip invalid players and skip self (i.e. this bot)
        if ((pPlayer) && (!pPlayer->free) && (pPlayer != pEdict)) {
            // skip this player if not alive (i.e. dead or dying)
            if (!IsAlive(pPlayer))
                continue;

            if (!((pPlayer->v.flags & FL_THIRDPARTYBOT)
                  || (pPlayer->v.flags & FL_FAKECLIENT)
                  || (pPlayer->v.flags & FL_CLIENT)))
                continue;

            int angle_to_player = FUNC_InFieldOfView(pBot->pEdict,
                                                     (pPlayer->v.origin -
                                                      pBot->pEdict->v.
                                                              origin));

            if (func_distance(pBot->pEdict->v.origin, pPlayer->v.origin) < 90
                && angle_to_player < iClosestAngle) {
                iClosestAngle = angle_to_player;
                return true;
            }

        }
    }
    return false;
}

bool BotShouldJump(cBot *pBot) {
    // WHen a bot should jump, something is blocking his way.
    // Most of the time it is a fence, or a 'half wall' that reaches from body to feet
    // However, the body most of the time traces above this wall.
    // What i do:
    // Get position of bot
    // Get position of legs
    // Trace
    // If blocked, then we SHOULD jump

    // FIX: Do not jump when defusing!
    if (pBot->f_defuse > gpGlobals->time)
        return false;

    TraceResult tr;
    Vector v_jump, v_source, v_dest;
    edict_t *pEdict = pBot->pEdict;

    // convert current view angle to vectors for TraceLine math...

    v_jump = FUNC_CalculateAngles(pBot);
    v_jump.x = 0;                // reset pitch to 0 (level horizontally)
    v_jump.z = 0;                // reset roll to 0 (straight up and down)

    UTIL_MakeVectors(v_jump);

    // use center of the body first...

    // maximum jump height is 45, so check one unit above that (MAX_JUMPHEIGHT)
    v_source = pEdict->v.origin + Vector(0, 0, -36 + (MAX_JUMPHEIGHT + 1));
    v_dest = v_source + gpGlobals->v_forward * 90;

    // trace a line forward at maximum jump height...
    UTIL_TraceHull(v_source, v_dest, dont_ignore_monsters, point_hull,
                   pEdict->v.pContainingEntity, &tr);

    //UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
    //              pEdict->v.pContainingEntity, &tr);

    // if trace hit something, return FALSE
    if (tr.flFraction < 1.0)
        return false;

    // Ok the body is clear
    v_source = pEdict->v.origin + Vector(0, 0, 34);
    v_dest = v_source + gpGlobals->v_forward * 90;

    // trace a line forward at maximum jump height...
    //  UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
    //                pEdict->v.pContainingEntity, &tr);
    UTIL_TraceHull(v_source, v_dest, dont_ignore_monsters, point_hull,
                   pEdict->v.pContainingEntity, &tr);

    if (tr.flFraction < 1.0)
        return false;

    // Ok the body is clear
    v_source = pEdict->v.origin + Vector(0, 0, -14);
    v_dest = v_source + gpGlobals->v_forward * 40;

    // trace a line forward at maximum jump height...
    //UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
    //              pEdict->v.pContainingEntity, &tr);
    UTIL_TraceHull(v_source, v_dest, dont_ignore_monsters, point_hull,
                   pEdict->v.pContainingEntity, &tr);

    if (tr.flFraction < 1.0) {
        //  if (BotCanJumpUp(pBot))
        return true;
    }

    return false;
}

// FUNCTION: Calculates angles as pEdict->v.v_angle should be when checking for body
Vector FUNC_CalculateAngles(cBot *pBot) {
    Vector v_body = Vector(0, 0, 0);

    // aim for the head and/or body
    Vector v_target = pBot->vBody - pBot->pEdict->v.origin;
    v_body = UTIL_VecToAngles(v_target);

    if (v_body.y > 180)
        v_body.y -= 360;

    // Paulo-La-Frite - START bot aiming bug fix
    if (v_body.x > 180)
        v_body.x -= 360;

    // adjust the view angle pitch to aim correctly (MUST be after body v.angles stuff)
    v_body.x = -v_body.x;

    // Paulo-La-Frite - END
    return v_body;
}

bool BotShouldDuck(cBot *pBot) {

    // temp
    return BotCanDuckUnder(pBot);

    // WHen a bot should jump, something is blocking his way.
    // Most of the time it is a fence, or a 'half wall' that reaches from body to feet
    // However, the body most of the time traces above this wall.
    // What i do:
    // Trace line from head
    // When head blocked and waist is free, then we should duck...

    TraceResult tr;
    Vector v_duck, v_source, v_dest;
    edict_t *pEdict = pBot->pEdict;

    // convert current view angle to vectors for TraceLine math...

    v_duck = FUNC_CalculateAngles(pBot);
    v_duck.x = 0;                // reset pitch to 0 (level horizontally)
    v_duck.z = 0;                // reset roll to 0 (straight up and down)

    UTIL_MakeVectors(v_duck);

    // Check if head is blocked
    v_source = pEdict->v.origin + Vector(0, 0, +37);
    v_dest = v_source + gpGlobals->v_forward * 24;

    // trace a line forward at maximum jump height...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

    if (tr.flFraction >= 1.0)
        return false;

    v_source = pEdict->v.origin;
    v_dest = v_source + gpGlobals->v_forward * 24;

    // trace a line forward at maximum jump height...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

    if (tr.flFraction < 1.0)
        return false;

    return true;
}

bool FUNC_DoRadio(cBot *pBot) {

    if (pBot->fDoRadio > gpGlobals->time)
        return false;

    int iRadio = pBot->ipCreateRadio;

    return RANDOM_LONG(0, 100) < iRadio;
}

// DECIDE: Take cover or not
bool FUNC_ShouldTakeCover(cBot *pBot) {
    // Do not allow taking cover within 5 seconds again.
    if (pBot->f_cover_time + 3 > gpGlobals->time)
        return false;

    if (!pBot->hasEnemy())
        return false;

    // MONEY: The less we have, the more we want to take cover
    int vMoney = 16000 - pBot->bot_money;

    // HEALTH: The less we have, the more we want to take cover
    int vHealth = 100 - pBot->bot_health;

    // CAMP: The more we want, the more we want to take cover
    int vCamp = pBot->ipCampRate;

    if (RANDOM_LONG(0, TOTAL_SCORE) < (vMoney + vHealth + vCamp))
        return true;

    // Fix #1 on taking cover
    pBot->f_cover_time = gpGlobals->time;        // wait 3 seconds before deciding again

    return false;
}

int FUNC_BotEstimateHearVector(cBot *pBot, Vector v_sound) {
    // here we normally figure out where to look at when we hear an enemy, RealBot AI PR 2 lagged a lot on this so we need another approach

    return -1;
}

// Added Stefan
// 7 November 2001
int FUNC_PlayerSpeed(edict_t *pPlayer) {
    if (pPlayer != NULL)
        return (int) pPlayer->v.velocity.Length2D();      // Return speed of any edict given

    return 0;
}

bool FUNC_PlayerRuns(int speed) {
    if (speed < 200)
        return false;             // We make no sound
    else
        return true;              // We make sound
}

// return weapon type of edict.
// only when 'important enough'.
int FUNC_EdictHoldsWeapon(edict_t *pEdict) {
    // sniper guns
    if (strcmp("models/p_awp.mdl", STRING(pEdict->v.weaponmodel)) == 0)
        return CS_WEAPON_AWP;
    if (strcmp("models/p_scout.mdl", STRING(pEdict->v.weaponmodel)) == 0)
        return CS_WEAPON_SCOUT;

    // good weapons (ak, m4a1)
    if (strcmp("models/p_ak47.mdl", STRING(pEdict->v.weaponmodel)) == 0)
        return CS_WEAPON_AK47;
    if (strcmp("models/p_m4a1.mdl", STRING(pEdict->v.weaponmodel)) == 0)
        return CS_WEAPON_M4A1;

    // grenade types
    if (strcmp("models/p_smokegrenade.mdl", STRING(pEdict->v.weaponmodel))
        == 0)
        return CS_WEAPON_SMOKEGRENADE;
    if (strcmp("models/p_hegrenade.mdl", STRING(pEdict->v.weaponmodel)) ==
        0)
        return CS_WEAPON_HEGRENADE;
    if (strcmp("models/p_flashbang.mdl", STRING(pEdict->v.weaponmodel)) ==
        0)
        return CS_WEAPON_FLASHBANG;

    // shield
    if (strcmp("models/p_shield.mdl", STRING(pEdict->v.weaponmodel)) == 0)
        return CS_WEAPON_SHIELD;

    // unknown
    return -1;
}

// Function to let a bot react on some sound which he cannot see
void FUNC_HearingTodo(cBot *pBot) {
    // This is called every frame.
    if (pBot->f_hear_time > gpGlobals->time)
        return;                   // Do nothing, we need more time to think

    if (pBot->f_camp_time > gpGlobals->time)
        return;

    if (pBot->f_wait_time > gpGlobals->time)
        return;

    // I HEAR SOMETHING
    // More chance on getting to true
    int health = pBot->bot_health;

    int action = 0;
    int etime = 0;

    if (health < 25)
        action = 2;
    else if (health >= 25 && health < 75)
        action = 1;
    else
        action = -1;

    if (action == 0) {
        etime = RANDOM_LONG(2, 6);
        pBot->f_camp_time = gpGlobals->time + etime;
        pBot->forgetGoal();
    } else if (action == 1) {
        etime = RANDOM_LONG(1, 7);
        pBot->f_walk_time = gpGlobals->time + etime;
    } else if (action == 2) {
        etime = RANDOM_LONG(1, 5);
        pBot->f_hold_duck = gpGlobals->time + etime;
    }

    pBot->f_hear_time = gpGlobals->time + 6;     // Always keep a 6 seconds
    // think time.
}

/*
 *	FUNC    : FUNC_ClearEnemyPointer(edict)
 *	Author  : Stefan Hendriks
 *  Function: Removes all pointers to a certain edict.
 *  Created : 16/11/2001
 *	Changed : 16/11/2001
 */
void FUNC_ClearEnemyPointer(edict_t *pPtr) {
    // Go through all bots and remove their enemy pointer that matches the given
    // pointer pPtr
    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t *pPlayer = INDEXENT(i);

        // Skip invalid players.
        if ((pPlayer) && (!pPlayer->free)) {

            // skip this player if not alive (i.e. dead or dying)
            if (!IsAlive(pPlayer))
                continue;

            // skip human players
            if (!(pPlayer->v.flags & FL_THIRDPARTYBOT))
                continue;

            // check if it is a bot known to us (ie, not another metamod supported bot)
            cBot *botpointer = UTIL_GetBotPointer(pPlayer);

            if (botpointer &&                   // Is a bot managed by us
                botpointer->hasEnemy(pPtr) // and has the pointer we want to get rid of
                    ) {
                botpointer->forgetEnemy();    // Clear its pointer
            }
        }

    }
}

// Returns true/false if an entity is on a ladder
bool FUNC_IsOnLadder(edict_t *pEntity) {
    if (pEntity == NULL)
        return false;

    if (pEntity->v.movetype == MOVETYPE_FLY)
        return true;

    return false;
}

/**
 * Returns true when hostage is not marked as being rescued by any other alive bot.
 *
 * @param pBot
 * @param pHostage
 * @return
 */
bool isHostageFree(cBot *pBot, edict_t *pHostage) {
    if (pHostage == NULL) return false;
    if (pBot == NULL) return false;

    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t *pPlayer = INDEXENT(i);

        if ((!pPlayer) || // NULL
            (pPlayer && pPlayer->free)) // free - ie no client
            continue; // next

        // skip this player if not alive (i.e. dead or dying)
        if (!IsAlive(pPlayer))
            continue;

        // not a bot
        if (!(pPlayer->v.flags & FL_THIRDPARTYBOT))
            continue;

        // Only check other bots (do not check self)
        cBot *botpointer = UTIL_GetBotPointer(pPlayer);
        if (botpointer && // a bot
            (botpointer != pBot) && // not self
            !botpointer->isDead()) { // not dead

            // other bot uses hostage, so hostage is not 'free'
            if (botpointer->isUsingHostage(pHostage)) {
                return false;
            }
        }
    }

    return true;
}

void TryToGetHostageTargetToFollowMe(cBot *pBot) {
    if (pBot->hasEnemy()) {
        return;                   // enemy, do not check
    }

    edict_t *pHostage = NULL;

    if (pBot->hasHostageToRescue()) {
        pBot->rprint("Has hostage to rescue");
        pHostage = pBot->getHostageToRescue();
    } else {
        pBot->rprint("No hostage to rescue, finding one");
        pHostage = pBot->findHostageToRescue();
    }

    if (pHostage == NULL) {
        pBot->rprint("No hostage found to rescue");
        return; // nothing to do
    }

    // Whenever we have a hostage to go after
    bool isRescueable = isHostageRescueable(pBot, pHostage);

    if (!isRescueable) {
        pBot->rprint("Hostage found, but not rescueable, forgetting...");
        pBot->forgetHostage(pHostage);
        return;
    } else {
        pBot->rprint("Remembering hostage to rescue");
        pBot->rememberWhichHostageToRescue(pHostage);
    }

    // Prevent bots getting to close here
    float distanceToHostage = func_distance(pBot->pEdict->v.origin, pHostage->v.origin);

    if (distanceToHostage < 80) {
        pBot->rprint("Getting really close to hostage now");
        pBot->f_move_speed = 10.0;      // do not get to close ;)
    }

    // From here, we should get the hostage when still visible
    if (pBot->canSeeEntity(pHostage)) {
        pBot->rprint("Can see hostage!");
        // set body to hostage!
        pBot->vBody = pBot->vHead = pHostage->v.origin + Vector(0, 0, 36);
        // by default run
        pBot->f_move_speed = pBot->f_max_speed;

        if (distanceToHostage <= 80) {
            pBot->rprint("Can see hostage AND really close!");
            pBot->f_move_speed = 0.0; // too close, do not move

            // only use hostage when facing
            int angle_to_hostage = FUNC_InFieldOfView(pBot->pEdict, (pBot->vBody - pBot->pEdict->v.origin));

            if (angle_to_hostage <= 30
                && (pBot->f_use_timer < gpGlobals->time)) {
                pBot->rprint("Can see hostage AND really REALLY close AND facing!");
                // within FOV and we assume we can now press the USE key. In order to make sure we press it once
                // and not multiple times we set the timer.
                pBot->f_use_timer = gpGlobals->time + 0.7f;

                UTIL_BotPressKey(pBot, IN_USE);

                // assuming it worked, remember bot is rescueing this hostage
                pBot->rememberHostageIsFollowingMe(pHostage);
                pBot->clearHostageToRescueTarget();
                REALBOT_PRINT(pBot, "HostageNear()", "I pressed USE and assume i have used the hostage");
                pBot->f_wait_time = gpGlobals->time + 0.5f;
            }
        }

        pBot->forgetGoal();
    }
}

bool isHostageRescued(edict_t *pHostage) {
    if (pHostage == NULL) return false;
//    char msg[255];
//    memset(msg, 0, sizeof(msg));
//    sprintf(msg, "v.name = %s v.effects for hostage is [%hhx]", pHostage->v.classname, pHostage->v.effects);
//    REALBOT_PRINT("isHostageRescued()", msg);

    if (FBitSet(pHostage->v.effects, EF_NODRAW)) {
        REALBOT_PRINT("isHostageRescued()", "Hostage is rescued");
        return true;
    }
    REALBOT_PRINT("isHostageRescued()", "Hostage is NOT rescued");
    return false;
}

bool isHostageRescueable(cBot *pBot, edict_t *pHostage) {
    pBot->rprint("isHostageRescueable");
    if (pHostage == NULL) return false;

    // Already rescued?
    if (isHostageRescued(pHostage)) return false;
    // dead
    if (pHostage->v.health < 1) return false;
    // Already moving? (used by human player?)
    if (FUNC_PlayerSpeed(pHostage) > 2) return false;
    // Already used by this bot?
    if (pBot->isUsingHostage(pHostage)) return false;
    // Is the hostage not used by *any other* bot?
    if (!isHostageFree(pBot, pHostage)) return false;
    // yes we can rescue this hostage
    return true;
}

// HostageNear()

bool FUNC_BotHoldsZoomWeapon(cBot *pBot) {
    // Check if the bot holds a weapon that can zoom, but is not a sniper gun.
    return pBot->isHoldingWeapon(CS_WEAPON_AUG) || pBot->isHoldingWeapon(CS_WEAPON_SG552);
}

void FUNC_BotChecksFalling(cBot *pBot) {
    // This routine should never be filled with code.
    // - Bots should simply never fall
    // - If bots do fall, check precalculation routine.
}

// New function to display a message on the center of the screen
void CenterMessage(char *buffer) {
    //DebugOut("waypoint: CenterMessage():\n");
    //DebugOut(buffer);
    //DebugOut("\n");
    UTIL_ClientPrintAll(HUD_PRINTCENTER, buffer);
}

// Bot Takes Cover
bool BOT_DecideTakeCover(cBot *pBot) {
    /*
           UTIL_ClientPrintAll( HUD_PRINTCENTER, "DECISION TO TAKE COVER\n" );

        int iNodeEnemy = NodeMachine.getCloseNode(pBot->pBotEnemy->v.origin, NODE_ZONE);
        int iNodeHere  = NodeMachine.getCloseNode(pBot->pEdict->v.origin, NODE_ZONE);
        int iCoverNode = NodeMachine.node_cover(iNodeHere, iNodeEnemy, pBot->pEdict);

        if (iCoverNode > -1)
        {
            // TODO TODO TODO make sure the cover code works via iGoalNode
    //		pBot->v_cover = NodeMachine.node_vector(iCoverNode);
            pBot->f_cover_time = gpGlobals->time + RANDOM_LONG(3,5);
            pBot->f_camp_time = gpGlobals->time;
            return true;
        }
        */

    return false;
}

// logs into a file
void rblog(char *txt) {
    // output to stdout
    printf(txt);

    // and to reallog file
    if (fpRblog) {
        fprintf(fpRblog, "%s", txt);        // print the text into the file
    }
}
