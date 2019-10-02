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
  * Count - Floyd
  *  
  * !! BOTS-UNITED FOREVER !! 
  *  
  * This project is open-source, it is protected under the GPL license;
  * By using this source-code you agree that you will ALWAYS release the
  * source-code with your project.
  *
  **/


/*

//=========================================================
// Returns if enemy can be shoot through some obstacle
//=========================================================
bool CBaseBot::IsShootableThruObstacle(Vector vecDest)
{
 if (!WeaponShootsThru(m_iCurrentWeapon))
    return FALSE;

 Vector vecSrc = EyePosition();
 Vector vecDir = (vecDest - vecSrc).Normalize();  // 1 unit long
 Vector vecPoint = g_vecZero;
 int iThickness = 0;
 int iHits = 0;

 edict_t *pentIgnore = pev->pContainingEntity;
 TraceResult tr;
 UTIL_TraceLine(vecSrc, vecDest, ignore_monsters, ignore_glass, pentIgnore, &tr);

 while (tr.flFraction != 1.0 && iHits < 3)
 {
    iHits++;
    iThickness++;
    vecPoint = tr.vecEndPos + vecDir;
    while (POINT_CONTENTS(vecPoint) == CONTENTS_SOLID && iThickness < 64)
    {
       vecPoint = vecPoint + vecDir;
       iThickness++;
    }
    UTIL_TraceLine(vecPoint, vecDest, ignore_monsters, ignore_glass, pentIgnore, &tr);
 }

 if (iHits < 3 && iThickness < 64)
 {
    if (LengthSquared(vecDest - vecPoint) < 12544)
       return TRUE;
 }

 return FALSE;
}

*/

#include <string.h>
#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>
#include <entity_state.h>

#include "bot.h"
#include "bot_weapons.h"
#include "bot_func.h"

#include "game.h"
#include "NodeMachine.h"
#include "ChatEngine.h"

#include <sys/types.h>
#include <sys/stat.h>

extern edict_t *pHostEdict;
extern int mod_id;
extern bool internet_play;
extern cGame Game;
extern cNodeMachine NodeMachine;
extern cChatEngine ChatEngine;
extern int counterstrike;
//static FILE *fp;
extern bool autoskill;

/* Radio issue
   Credit by Ditlew (NNBOT - Rest In Peace) */
bool radio_message = false;
char *message = (char *) malloc(64 * sizeof(char));
char radio_messenger[30];

// random boundries
extern int random_max_skill;
extern int random_min_skill;
cBot bots[32];                  // max of 32 bots in a game

// External added variables
extern bool end_round;          // End round

#ifndef _WIN32
#define _snprintf snprintf
#endif

cBot::cBot() {
    pBotHostage = NULL;
    fMoveToNodeTime = -1;
    clearHostages();
}

/******************************************************************************
 Function purpose: Initializes bot vars on spawn
 ******************************************************************************/
void cBot::SpawnInit() {
    rprint_trace("SpawnInit()", "START");

    // ------------------------
    // TIMERS
    // ------------------------
    fUpdateTime = gpGlobals->time;
    fLastRunPlayerMoveTime = gpGlobals->time - 0.1f;
    fButtonTime = gpGlobals->time;
    fChatTime = gpGlobals->time + RANDOM_FLOAT(0.5, 5);
    fMemoryTime = gpGlobals->time;
    fDoRadio = gpGlobals->time;
    float freezeTimeCVAR = CVAR_GET_FLOAT("mp_freezetime");
    fNotStuckTime = gpGlobals->time + freezeTimeCVAR + 0.5f;
    f_shoot_wait_time = gpGlobals->time;
    f_goback_time = gpGlobals->time;
    f_may_jump_time = gpGlobals->time;
    fCheckHostageStatusTimer = gpGlobals->time;
    f_defuse = gpGlobals->time;
    f_allow_keypress = gpGlobals->time;
    f_use_timer = gpGlobals->time;
    f_light_time = gpGlobals->time;
    f_sec_weapon = gpGlobals->time;
    f_prim_weapon = gpGlobals->time;
    f_gren_time = gpGlobals->time;
    f_walk_time = gpGlobals->time;
    f_hear_time = gpGlobals->time;
    freezeTime = gpGlobals->time - 1;
    f_cover_time = gpGlobals->time;
    f_c4_time = gpGlobals->time;
    f_update_weapon_time = gpGlobals->time;
    f_follow_time = gpGlobals->time;
    f_jump_time = 0.0;
    f_hold_duck = gpGlobals->time;
    f_camp_time = gpGlobals->time;
    f_wait_time = gpGlobals->time;
    f_bot_see_enemy_time = gpGlobals->time;
    f_bot_find_enemy_time = gpGlobals->time;
    f_shoot_time = gpGlobals->time;
    fMoveToNodeTime = -1;
    nodeTimeIncreasedAmount = 0;
    distanceMovedTimer = gpGlobals->time;
    distanceMoved = 0;
    fBlindedTime = gpGlobals->time;
    f_console_timer = gpGlobals->time + RANDOM_FLOAT(0.1, 0.9);
    fWanderTime = gpGlobals->time;
    f_strafe_time = gpGlobals->time;

    // Personality Related (these gets changed when loading personality file)
    fpXOffset = 0.0;
    fpYOffset = 0.0;
    fpZOffset = 0.0;
    fpMinReactTime = 0.0;
    fpMaxReactTime = 0.0;

    // ------------------------
    // POINTERS
    // ------------------------
    pButtonEdict = NULL;
    pBotHostage = NULL;
    clearHostages();
    pEnemyEdict = NULL;

    // chat
    memset(chChatSentence, 0, sizeof(chChatSentence));


    // ------------------------
    // INTEGERS
    // ------------------------
    iGoalNode = -1;
    goalIndex = -1;
    iPreviousGoalNode = -1;
    iCloseNode = -1;
    iDiedNode = -1;

    iTeam = -1;
    bot_class = -1;
    i_camp_style = 0;
    iPrimaryWeapon = -1;
    iSecondaryWeapon = -1;
    zoomed = ZOOM_NONE;
    play_rounds = RANDOM_LONG(Game.GetMinPlayRounds(), Game.GetMaxPlayRounds());
    bot_health = 0;
    prev_health = 0;
    bot_armor = 0;
    bot_weapons = 0;
    bot_use_special = 0 + RANDOM_LONG(0, 2);
    console_nr = 0;
    pathIndex = -1;
    iPathFlags = PATH_DANGER;

    // Smarter Stuck stuff
    iDuckTries = 0;
    iJumpTries = 0;

    // ------------------------
    // BOOLEANS
    // ------------------------
    vip = UTIL_IsVip(pEdict);
    bWalkKnife = false;
    buy_ammo_primary = true;
    buy_ammo_secondary = true;
    buy_primary = (Game.bPistols ? false : true);        //30/07/04: Josh, handle the pistols only mode
    buy_secondary = (Game.bPistols ? true : false);
    buy_armor = false;
    buy_defusekit = false;
    bFirstOutOfSight = false;
    buy_grenade = false;
    buy_smokegrenade = false;

    buy_flashbang = 0;
    if (RANDOM_LONG(0, 100) < ipWalkWithKnife) {
        bWalkKnife = true;
    }

    if (UTIL_GetTeam(pEdict) == 1) {
        if (RANDOM_LONG(0, 100) < ipBuyDefuseKit) {
            buy_defusekit = true;
        }
    }

    if (RANDOM_LONG(0, 100) < ipBuyGrenade) {
        buy_grenade = true;
    }

    // 31.08.04 Frashman added Support for Smoke Grenade
    if (RANDOM_LONG(0, 100) < ipBuySmokeGren) {
        buy_smokegrenade = true;
    }

    if (RANDOM_LONG(0, 100) < ipBuyFlashBang) {
        buy_flashbang = 2;

    }

    if (RANDOM_LONG(0, 100) < 15 || Game.bPistols)
        buy_secondary = true;

    // ------------------------
    // HUD
    // ------------------------
    bHUD_C4_plantable = false;   // Get's init'ed anyway...  // BERKED

    // ------------------------
    // FLOATS
    // ------------------------
    f_strafe_speed = 0.0;
    f_max_speed = CVAR_GET_FLOAT("sv_maxspeed");

    // ------------------------
    // VECTORS
    // ------------------------
    prevOrigin = Vector(9999.0, 9999.0, 9999.0);
    lastSeenEnemyVector = Vector(0, 0, 0);
    vEar = Vector(9999, 9999, 9999);

    // ------------------------
    // CHAR
    // ------------------------
    arg1[0] = 0;
    arg2[0] = 0;
    arg3[0] = 0;
    memset(&(current_weapon), 0, sizeof(current_weapon));
    memset(&(m_rgAmmo), 0, sizeof(m_rgAmmo));

    rprint_trace("SpawnInit()", "END");
}

/******************************************************************************
 Function purpose: Initializes bot vars on new round
 ******************************************************************************/
void cBot::NewRound() {
    rprint_trace("NewRound()", "START");

    // ------------------------
    // TIMERS
    // ------------------------
    fUpdateTime = gpGlobals->time;
    fLastRunPlayerMoveTime = gpGlobals->time;
    fCheckHostageStatusTimer = gpGlobals->time;
    fButtonTime = gpGlobals->time;
    fChatTime = gpGlobals->time + RANDOM_FLOAT(0.5, 5);
    fMemoryTime = gpGlobals->time;
    fDoRadio = gpGlobals->time;
    float freezeTimeCVAR = CVAR_GET_FLOAT("mp_freezetime");
    fNotStuckTime = gpGlobals->time + freezeTimeCVAR + 0.5f;
    f_shoot_wait_time = gpGlobals->time;
    f_goback_time = gpGlobals->time;
    f_may_jump_time = gpGlobals->time;
    f_defuse = gpGlobals->time;
    f_allow_keypress = gpGlobals->time;
    f_use_timer = gpGlobals->time;
    f_light_time = gpGlobals->time;
    f_sec_weapon = gpGlobals->time;
    f_prim_weapon = gpGlobals->time;
    f_gren_time = gpGlobals->time;
    f_walk_time = gpGlobals->time;
    f_hear_time = gpGlobals->time;
    freezeTime = gpGlobals->time - 1;
    f_cover_time = gpGlobals->time;
    f_c4_time = gpGlobals->time;
    f_update_weapon_time = gpGlobals->time;
    f_follow_time = gpGlobals->time;
    f_jump_time = 0.0;
    f_hold_duck = gpGlobals->time - 1;
    f_camp_time = gpGlobals->time;
    f_wait_time = gpGlobals->time;
    f_bot_see_enemy_time = gpGlobals->time;
    f_bot_find_enemy_time = gpGlobals->time;
    f_shoot_time = gpGlobals->time;
    fMoveToNodeTime = -1;
    nodeTimeIncreasedAmount = 0;
    distanceMovedTimer = gpGlobals->time;
    distanceMoved = 0;
    fBlindedTime = gpGlobals->time;
    f_console_timer = gpGlobals->time + RANDOM_FLOAT(0.1, 0.9);
    fWanderTime = gpGlobals->time;
    f_strafe_time = gpGlobals->time;

    // ------------------------
    // POINTERS
    // ------------------------
    pButtonEdict = NULL;
    pBotHostage = NULL;
    clearHostages();
    pEnemyEdict = NULL;

    // ------------------------
    // INTEGERS
    // ------------------------
    i_camp_style = 0;
    iPrimaryWeapon = -1;
    iSecondaryWeapon = -1;
    zoomed = ZOOM_NONE;
    bot_health = 0;
    prev_health = 0;
    bot_armor = 0;
//   bot_weapons = 0; // <- stefan: prevent from buying new stuff every round!
    console_nr = 0;
    pathIndex = -1;
    iGoalNode = -1;
    goalIndex = -1;
    iPreviousGoalNode = -1;
    iCloseNode = -1;


    // Smarter Stuck stuff
    iDuckTries = 0;
    iJumpTries = 0;

    if (RANDOM_LONG(0, 100) < ipFearRate)
        iPathFlags = PATH_DANGER;
    else
        iPathFlags = PATH_NONE;

    // ------------------------
    // BOOLEANS
    // ------------------------

    // chat
    memset(chChatSentence, 0, sizeof(chChatSentence));

    vip = UTIL_IsVip(pEdict);

    // Every round consider
    bWalkKnife = false;

    if (RANDOM_LONG(0, 100) < ipWalkWithKnife)
        bWalkKnife = true;

    // Buying
    buy_ammo_primary = true;
    buy_ammo_secondary = true;
    buy_primary = (Game.bPistols ? false : true);
    buy_grenade = false;
    buy_smokegrenade = false;
    buy_flashbang = 0;
    buy_secondary = (Game.bPistols ? true : false);
    buy_armor = false;
    buy_defusekit = false;

    if (UTIL_GetTeam(pEdict) == 1)
        if (RANDOM_LONG(0, 100) < ipBuyDefuseKit)
            buy_defusekit = true;

    if (RANDOM_LONG(0, 100) < ipBuyArmour)
        buy_armor = true;

    if (RANDOM_LONG(0, 100) < ipBuyGrenade)
        buy_grenade = true;

    if (RANDOM_LONG(0, 100) < ipBuySmokeGren)
        buy_smokegrenade = true;

    if (RANDOM_LONG(0, 100) < ipBuyFlashBang)
        buy_flashbang = 2;


    bFirstOutOfSight = false;


    f_strafe_speed = 0.0;

    // ------------------------
    // VECTORS
    // ------------------------
    prevOrigin = Vector(9999.0, 9999.0, 9999.0);
    lastSeenEnemyVector = Vector(0, 0, 0);
    vEar = Vector(9999, 9999, 9999);

    // ------------------------
    // CHAR
    // ------------------------
    arg1[0] = 0;
    arg2[0] = 0;
    arg3[0] = 0;

    // initalize a few other stuff
    NodeMachine.path_clear(iBotIndex);
    iPathFlags = PATH_NONE;

    played_rounds++;

    // hello dudes
    if (played_rounds == 1) {
        // do some chatting
        if (RANDOM_LONG(0, 100) < (ipChatRate + 10)) {
            // we should say something now?
            int iMax = -1;

            for (int tc = 0; tc < 50; tc++) {
                if (ChatEngine.ReplyBlock[98].sentence[tc][0] != '\0')
                    iMax++;
            }

            int the_c = RANDOM_LONG(0, iMax);

            if (the_c > -1 && iMax > -1) {
                char chSentence[80];
                memset(chSentence, 0, sizeof(chSentence));
                sprintf(chSentence, "%s ",
                        ChatEngine.ReplyBlock[98].sentence[the_c]);
                PrepareChat(chSentence);
            }
        }
    }

    clearHostages();
    clearHostageToRescueTarget();

    rprint("NewRound", "Initialization new round finished");
}

/******************************************************************************
 Function purpose: Returns a random chat sentence and stores it into 'sentence'
 ******************************************************************************/
void cBot::PrepareChat(char sentence[128]) {
    if (Game.iProducedSentences <= Game.iMaxSentences) {
        // makes bot chat away
        fChatTime = gpGlobals->time + RANDOM_FLOAT(0.1, 2.0);
        strcpy(chChatSentence, sentence); // copy this
        Game.iProducedSentences++;
    }
}

/******************************************************************************
 Function purpose: Return reaction time based upon skill
 ******************************************************************************/
float cBot::ReactionTime(int iSkill) {
    float time = RANDOM_FLOAT(fpMinReactTime, fpMaxReactTime);
    if (Game.messageVerbosity > 1) {
        char msg[255];
        sprintf(msg, "minReactTime %f, maxReactTime %f, skill %d, results into %f", fpMinReactTime, fpMaxReactTime, iSkill, time);
        rprint_trace("ReactionTime()", msg);
    }
    return time;
}

/******************************************************************************
 Function purpose: Finds a (new) enemy
 ******************************************************************************/
int cBot::FindEnemy() {
    // When on ladder, do not search for enemies
    if (isOnLadder())
        return -1;

    // When blinded we cannot search for enemies
    if (fBlindedTime > gpGlobals->time)
        return -1;
    float fNearestDistance = 9999;       // Nearest distance
    edict_t *pNewEnemy = NULL;   // New enemy found

    // SEARCH PLAYERS FOR ENEMIES
    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t *pPlayer = INDEXENT(i);

        // skip invalid players and skip self (i.e. this bot)
        if ((pPlayer) && (!pPlayer->free) && (pPlayer != pEdict)) {

            // skip this player if not alive (i.e. dead or dying)
            if (!IsAlive(pPlayer))
                continue;

            Vector vVecEnd = pPlayer->v.origin + pPlayer->v.view_ofs;

            // if bot can see the player...
            if (FInViewCone(&vVecEnd, pEdict) && FVisible(vVecEnd, pEdict)) {
                int player_team = UTIL_GetTeam(pPlayer);
                int bot_team = UTIL_GetTeam(pEdict);
                if (player_team == bot_team)
                    continue;        // do not target teammates

                // Its not a friend, track enemy
                float fDistance =
                        (pPlayer->v.origin - pEdict->v.origin).Length();
                bool bCanSee = true;

                // The further away, the less chance we see this enemy
                //if (RANDOM_FLOAT(0,1.0) < (fDistance/4096))
                //    bCanSee=false;
                if (CarryWeaponType() == SNIPER)
                    bCanSee = true;
                if (fDistance < fNearestDistance && bCanSee) {
                    fNearestDistance = fDistance;
                    pNewEnemy = pPlayer;
                }
                continue;
            }
        }                         // valid player
    }                            // FOR

    // We found a new enemy & the new enemy is different then previous pointer
    if (pNewEnemy && pNewEnemy != pEnemyEdict) {
        int iCurrentNode = determineCurrentNode();

        // Add 'contact' data
        if (iCurrentNode > -1) {
            NodeMachine.contact(iCurrentNode, UTIL_GetTeam(pEdict));
        }

        // We have a reaction time to this new enemy
        rememberEnemyFound();
        f_shoot_time = gpGlobals->time + ReactionTime(bot_skill);
        pEnemyEdict = pNewEnemy; // Update pointer

        // We did not have an enemy before
        if (pEnemyEdict == NULL) {
            rprint_trace("FindEnemy()", "Found new enemy");

            // RADIO: When we found a NEW enemy but NOT via a friend
            if (FUNC_DoRadio(this)) {
                UTIL_BotRadioMessage(this, 3, "2", "");
            }

            // We found a new enemy
            return 0;
        } else {
            // we found an enemy that is newer/more dangerous then previous
            rprint_trace("FindEnemy()", "Found 'newer' enemy");
            return 3;
        }
    }

    // nothing found
    return -1;              // return result
}

void cBot::rememberEnemyFound() {
    f_bot_find_enemy_time = gpGlobals->time + REMEMBER_ENEMY_TIME;
}

/******************************************************************************
 Function purpose: Sets vHead to aim at vector
 ******************************************************************************/
void cBot::setHeadAiming(Vector vTarget) {
    vHead = vTarget;
}

/**
 * Returns true / false wether enemy is alive.
 * @return
 */
bool cBot::isEnemyAlive() {
    return IsAlive(pEnemyEdict);
}

bool cBot::isSeeingEnemy() {
    if (!hasEnemy()) {
        this->rprint("canSeeEnemy called without having enemy?");
        return false;
    }

    if (isBlindedByFlashbang()) {
        return false;
    }

    Vector vBody = pEnemyEdict->v.origin;
    Vector vHead = pEnemyEdict->v.origin + pEnemyEdict->v.view_ofs;

    bool bodyInFOV = FInViewCone(&vBody, pEdict) && FVisible(vBody, pEdict);
    bool headInFOV = FInViewCone(&vHead, pEdict) && FVisible(vHead, pEdict);
    if (bodyInFOV || headInFOV) {
        return true;
    }
    return false;
}

/******************************************************************************
 Function purpose: Aims at enemy, only when valid. Based upon skill how it 'aims'
 ******************************************************************************/
void cBot::AimAtEnemy() {
    if (!hasEnemy())
        return;

    // We cannot see our enemy? -> bail out
    if (isSeeingEnemy()) {
        setHeadAiming(lastSeenEnemyVector); // look at last known vector of enemy
        return;
    }

    // ------------------------ we can see enemy -------------------------
    float fDistance;

    // Distance to enemy
    fDistance = (pEnemyEdict->v.origin - pEdict->v.origin).Length() + 1; // +1 to make sure we never divide by zero

    // factor in distance, the further away the more deviation - which is based on skill
    int skillReversed = (10 - bot_skill) + 1;
    float fScale = 0.5 + (fDistance / (64 *
                                       skillReversed)); // a good skilled bot is less impacted by distance than a bad skilled bot

    if (CarryWeaponType() == SNIPER) fScale *= 0.80; // sniping improves aiming

    // Set target here
    Vector vTarget;
    if (bot_skill <= 1)
        vTarget = pEnemyEdict->v.origin + pEnemyEdict->v.view_ofs * RANDOM_FLOAT(-0.5, 1.1); // aim for the head
    else if (bot_skill > 1 && bot_skill < 4)
        vTarget = pEnemyEdict->v.origin +
                  pEnemyEdict->v.view_ofs * RANDOM_FLOAT(-2.5, 2.5); // aim for the head more fuzzy
    else
        vTarget = pEnemyEdict->v.origin; // aim for body

    // Based upon how far, we make this fuzzy
    float fDx, fDy, fDz;
    fDx = fDy = fDz = ((bot_skill + 1) * fScale);

    // Example 1:
    // Super skilled bot (bot_skill 1), with enemy of 2048 units away. Results into:
    // skillReversed = (10 - 0 + 1) == 11
    // fScale = 2048 / (128 * 11) -> 2048 / 1408 => 1.454545
    // fd* = 0.5 + 1 * 1,95

    // Example 2, less skilled bot (skill = 3) same enemy
    // skillReversed = (10 - 3 + 1) == 8
    // fScale = 2048 / (128 * 8) -> 2048 / 1024 => 2
    // fd* = 3 * 2

    vTarget = vTarget + Vector(
            RANDOM_FLOAT(-fDx, fDx),
            RANDOM_FLOAT(-fDy, fDy),
            RANDOM_FLOAT(-fDz, fDz)
    );

    // Add Offset
    fDx = fpXOffset;
    fDy = fpYOffset;
    fDz = fpZOffset;

    // increase offset with personality x,y,z offsets randomly
    vTarget = vTarget + Vector(
            RANDOM_FLOAT(-fDx, fDx),
            RANDOM_FLOAT(-fDy, fDy),
            RANDOM_FLOAT(-fDz, fDz)
    );

    if (isHoldingGrenadeOrFlashbang()) {
        // aim a bit higher
        vTarget = vTarget + Vector(0, 0, 50);
    }

    setHeadAiming(vTarget);
}

bool cBot::isBlindedByFlashbang() const {
    return fBlindedTime > gpGlobals->time;
}

bool cBot::isHoldingGrenadeOrFlashbang() const {
    return current_weapon.iId == CS_WEAPON_HEGRENADE || current_weapon.iId == CS_WEAPON_FLASHBANG;
}

/******************************************************************************
 Function purpose: Perform fighting actions
 ******************************************************************************/
void cBot::FightEnemy() {
    // We can see our enemy
    if (!isBlindedByFlashbang() && isSeeingEnemy()) {

        // GET OUT OF CAMP MODE
        if (f_camp_time > gpGlobals->time) {
            f_camp_time = gpGlobals->time;
        }

        // Next time our enemy gets out of sight, it will be the 'first' time
        // of all 'frame times'.
        bFirstOutOfSight = false;

        // Remember last seen enemy position
        lastSeenEnemyVector = pEnemyEdict->v.origin;    // last seen enemy position

        // FIXME: Fix the darn zoom bug
        // zoom in with sniper gun
        if (CarryWeaponType() == SNIPER) {
            if (zoomed < ZOOM_TWICE && f_allow_keypress < gpGlobals->time) {
                UTIL_BotPressKey(this, IN_ATTACK2);
                f_allow_keypress = gpGlobals->time + 0.7;
                zoomed++;

                if (zoomed > ZOOM_TWICE)
                    zoomed = ZOOM_NONE;
            }
        } else if (FUNC_BotHoldsZoomWeapon(this)) {
            if (zoomed < ZOOM_ONCE && f_allow_keypress < gpGlobals->time) {
                UTIL_BotPressKey(this, IN_ATTACK2);
                f_allow_keypress = gpGlobals->time + 0.7;
                zoomed++;
            }
        }

        // NOT blinded by flashbang, try to find cover?
        if (f_cover_time < gpGlobals->time) {
            // COVER: Not taking cover now, fight using fightstyles.

            // when vip, we always take cover.
            if (vip) {
                // Camp, take cover, etc.
                BOT_DecideTakeCover(this);

                if (FUNC_DoRadio(this)) {
                    UTIL_BotRadioMessage(this, 3, "3", "");  // need backup
                }
            } else {
                // DECIDE: Should we take cover or not.
                if (FUNC_ShouldTakeCover(this)) {
                    FindCover();
                }
            }
        } else {

        }

        // Keep timer updated for enemy
        f_bot_find_enemy_time = gpGlobals->time + REMEMBER_ENEMY_TIME;
    }
    else                       // ---- CANNOT SEE ENEMY
    {
        if (f_bot_find_enemy_time < gpGlobals->time) {
            pEnemyEdict = NULL;
            lastSeenEnemyVector = Vector(0, 0, 0);
            rprint_trace("FightEnemy()", "Lost enemy out of sight, forgetting path and goal");
            forgetPath();
            forgetGoal();
        } else {

            // When we have the enemy for the first time out of sight
            // we calculate a path to the last seen position
            if (!bFirstOutOfSight) {
                rprint_trace("FightEnemy()", "Enemy out of sight, calculating path towards it.");
                // Only change path when we update our information here
                int iGoal = NodeMachine.getClosestNode(lastSeenEnemyVector, NODE_ZONE, pEdict);
                if (iGoal > -1) {
                    setGoalNode(iGoal);
                    forgetPath();
                }

                bFirstOutOfSight = true;
            } else {
                if (!hasGoal()) {
                    rprint("Enemy out of sight and no goal, forgetting enemy");
                    forgetEnemy();
                }
            }
        }
    }                            // visible
}

void cBot::pickWeapon(int weaponId) {
    UTIL_SelectItem(pEdict, UTIL_GiveWeaponName(weaponId));
    f_c4_time = gpGlobals->time - 1;    // reset C4 timer data
    // give Counter-Strike time to switch weapon (animation, update state, etc)
    f_update_weapon_time = gpGlobals->time + 0.7;
}

bool cBot::ownsFavoritePrimaryWeapon() {
    return hasFavoritePrimaryWeaponPreference() && isOwningWeapon(ipFavoPriWeapon);
}

bool cBot::ownsFavoriteSecondaryWeapon() {
    return hasFavoriteSecondaryWeaponPreference() && isOwningWeapon(ipFavoSecWeapon);
}

/**
 * Returns true if bot has weapon (id) in possession
 * @param weaponId
 * @return
 */
bool cBot::isOwningWeapon(int weaponId) {
    return bot_weapons & (1 << weaponId);
}

/**
 * Returns true if bot carries weapon right now
 * @param weaponId
 * @return
 */
bool cBot::isHoldingWeapon(int weaponId) {
    return (current_weapon.iId == weaponId);
}

bool cBot::hasFavoritePrimaryWeaponPreference() {
    return ipFavoPriWeapon > -1;
}

bool cBot::hasFavoriteSecondaryWeaponPreference() {
    return ipFavoSecWeapon > -1;
}

bool cBot::canAfford(int price) {
    return this->bot_money > price;
}

/******************************************************************************
 Function purpose: Based upon several events pick the best weapon
 ******************************************************************************/
void cBot::PickBestWeapon() {
    // does Timer allow to change weapon? (only when f_update_weapon_time < gpGlobals->time
    if (f_update_weapon_time > gpGlobals->time)
        return;

    // Distance to enemy
    float fDistance = func_distance(pEdict->v.origin, lastSeenEnemyVector);

    float knifeDistance = 300;

    // ----------------------------
    // In this function all we do is decide what weapon to pick
    // if we don't pick another weapon the current weapon is okay
    // ----------------------------

    // First we handle situations which are bad, no matter the distance
    // or any other circumstance.

    // BAD: Carrying C4 or knife
    if (CarryWeapon(CS_WEAPON_C4) ||    // carrying C4
        (CarryWeapon(CS_WEAPON_KNIFE) && fDistance > knifeDistance)) { // carrying knife and too far
        if (hasPrimaryWeaponEquiped()) {
            pickWeapon(iPrimaryWeapon);
            return;
        } else if (hasSecondaryWeaponEquiped()) {
            pickWeapon(iSecondaryWeapon);
            return;
        }
    }

    // At this point we do not update weapon information. And we did not 'switch back' to primary / secondary
    if (hasEnemy() && !isSeeingEnemy()) {
        // decision to pull HE grenade
        if (isOwningWeapon(CS_WEAPON_HEGRENADE) &&       // we have a grenade
            func_distance(pEdict->v.origin, lastSeenEnemyVector) < 900 &&     // we are close
            func_distance(pEdict->v.origin, lastSeenEnemyVector) > 200 &&     // but not to close
            RANDOM_LONG(0, 100) < 10 &&   // only randomly we pick a grenade in the heat of the battle
            current_weapon.iId != CS_WEAPON_HEGRENADE && current_weapon.iId != CS_WEAPON_FLASHBANG &&
            f_gren_time + 15 < gpGlobals->time) // and dont hold it yet
        {
            UTIL_SelectItem(pEdict, "weapon_hegrenade");   // select grenade
            f_wait_time = gpGlobals->time + 1;     // wait 1 second (stand still 1 sec)
            f_gren_time =
                    gpGlobals->time + (1.0 + RANDOM_FLOAT(0.5, 1.5));        // and determine how long we should hold it
            zoomed = ZOOM_NONE;    // Counter-Strike resets zooming when choosing another weapon
            return;
        }
        // OR we pull a flashbang?
        if (isOwningWeapon(CS_WEAPON_FLASHBANG) &&       // we have a grenade
            func_distance(pEdict->v.origin, lastSeenEnemyVector) < 200 &&     // we are close
            func_distance(pEdict->v.origin, lastSeenEnemyVector) > 300 &&     // but not to close
            RANDOM_LONG(0, 100) < 15 &&   // only randomly we pick a grenade in the heat of the battle
            current_weapon.iId != CS_WEAPON_FLASHBANG && current_weapon.iId != CS_WEAPON_HEGRENADE &&
            f_gren_time + 15 < gpGlobals->time) // and dont hold it yet
        {
            UTIL_SelectItem(pEdict, "weapon_flashbang");   // select grenade
            f_wait_time = gpGlobals->time + 1;     // wait 1 second (stand still 1 sec)
            f_gren_time =
                    gpGlobals->time + (1.0 + RANDOM_FLOAT(0.5, 1.5));        // and determine how long we should hold it
            zoomed = ZOOM_NONE;    // Counter-Strike resets zooming when choosing another weapon
            return;
        }
    }

    // When we are here, we did not decide to switch to grenade/flashbang. Now we look
    // if the bot has to reload or switch weapon based upon ammo.

    // ----------------------------------------
    // More complex bad things that can happen:
    // ----------------------------------------
    int iTotalAmmo = current_weapon.iAmmo1;
    int iCurrentAmmo = current_weapon.iClip;

    //char msg[80];
    //sprintf(msg, "BOT: ICLIP %d, TOTALAMMO %d\n", iCurrentAmmo, iTotalAmmo);

    // Clip is out of ammo
    if (iCurrentAmmo < 1
        && (CarryWeaponType() == PRIMARY || CarryWeaponType() == SECONDARY)) {
        // Camp, take cover, etc.
        BOT_DecideTakeCover(this);

        // We still have ammo!
        if (iTotalAmmo > 0) {
            UTIL_BotPressKey(this, IN_RELOAD);
            f_update_weapon_time = gpGlobals->time + 0.7;  // update timer
            return;
        } else {
            // Thanks to dstruct2k for easy ctrl-c/v, i optimized the code
            // a bit though. Btw, distance 600 is too far for slashing :)

            // at here the bot does not have ammo of the current weapon, so
            // switch to another weapon.
            if (iPrimaryWeapon > -1 &&     // we have a primary
                current_weapon.iId != iPrimaryWeapon &&    // that's not the current, empty gun
                func_distance(pEdict->v.origin, lastSeenEnemyVector) > 300)    // and we are not close enough to knife
            {
                // select primary weapon
                UTIL_SelectItem(pEdict, UTIL_GiveWeaponName(iPrimaryWeapon));       // select the primary
                return;
            } else {

                if (iSecondaryWeapon > -1 && current_weapon.iId != iSecondaryWeapon &&
                    // that's not the current, empty gun
                    func_distance(pEdict->v.origin, lastSeenEnemyVector) > 300) // and we are not close enough to knife
                {
                    UTIL_SelectItem(pEdict, UTIL_GiveWeaponName(iSecondaryWeapon));  // select the secondary
                    return;
                } else {
                    if (isOwningWeapon(CS_WEAPON_KNIFE) &&  // we have a knife (for non-knife maps)
                        !isHoldingWeapon(CS_WEAPON_KNIFE))  // but we do not carry it
                    {
                        UTIL_SelectItem(pEdict, "weapon_knife");
                        return;
                    }
                }
            }                      // end if
        }                         // no ammo
    }
}

/******************************************************************************
 Function purpose: Fire weapon (burst; or do not fire when not allowed)
 ******************************************************************************/
void cBot::FireWeapon() {
    // We may not shoot!
    if (f_shoot_time > gpGlobals->time ||
        f_update_weapon_time > gpGlobals->time)
        return;

    if (!isSeeingEnemy()) {
        return;
    }

    // ------------------------------------------------------------
    float fDistance = 50;

    if (hasEnemy()) {
        fDistance = func_distance(pEdict->v.origin, pEnemyEdict->v.origin);
    }

    // Depending on weapon type
    if (CarryWeaponType() == SECONDARY) {
        // We may shoot, use shooting rate.
        // TODO TODO TODO; Add shooting rates in BUYTABLE.INI

        if (f_sec_weapon < gpGlobals->time) {
            UTIL_BotPressKey(this, IN_ATTACK);
            f_sec_weapon = gpGlobals->time + RANDOM_FLOAT(0.05, 0.2);
        }

    } else if (CarryWeaponType() == PRIMARY) {
        // We may shoot, use shooting rate.
        // TODO TODO TODO: Add shooting rates in BUYTABLE.INI
        if (f_prim_weapon < gpGlobals->time) {
            UTIL_BotPressKey(this, IN_ATTACK);     // Hold fire
            // All other weapons, the more distance, the more time we add to holding weapon
            if (f_shoot_wait_time < gpGlobals->time) {
                // AK, COLT, STEYR AUG, only when enough skill!
                if ((CarryWeapon(CS_WEAPON_AK47)
                     || CarryWeapon(CS_WEAPON_M4A1)
                     || CarryWeapon(CS_WEAPON_AUG)) && (bot_skill < 3)) {
                    float f_burst = 0.1;
                    f_burst = (2048 / fDistance) + 0.1;
                    if (f_burst < 0.1)
                        f_burst = 0.1;
                    if (f_burst > 0.4)
                        f_burst = 0.4;

                    // CS 1.6 less burst
                    if (counterstrike == 1)
                        if (f_burst > 0.3)
                            f_burst = 0.3;

                    f_prim_weapon = gpGlobals->time + f_burst;

                    f_shoot_wait_time = gpGlobals->time + (f_burst * 3);
                } else              // other weapons
                {
                    float f_burst = 0.1;
                    if (fDistance > 300 && bot_skill < 6) {
                        f_burst = ((fDistance - 300) / 550);
                        if (f_burst < 0.1)
                            f_burst = 0.0;
                        if (f_burst > 0.7)
                            f_burst = 0.7;

                        // CS 1.6 less burst
                        if (counterstrike == 1)
                            if (f_burst > 0.2)
                                f_burst = 0.2;
                        if (f_prim_weapon < gpGlobals->time)
                            f_prim_weapon = gpGlobals->time + f_burst;
                    }
                    f_shoot_wait_time =
                            gpGlobals->time + f_burst + RANDOM_FLOAT(0.2, 0.7);
                }
            }
        }                         // give the bot alteast 0.3 seconds to fire its weapon
    }                            // PRIMARY
    else if (CarryWeaponType() == GRENADE) {
        if (f_gren_time > gpGlobals->time) {
            UTIL_BotPressKey(this, IN_ATTACK);     // Hold fire
            setMoveSpeed(f_max_speed / 2);

            // Set new goal when holding flashbang!
            if (current_weapon.iId == CS_WEAPON_FLASHBANG) {

                //tonode ?
                // COVER: Take cover, using tracelines all the time!
                FindCover();
            }
        } else if (f_gren_time + 0.5 < gpGlobals->time) {
            // NOTE: Should not happen, a bot cannot 'forget' this...
            f_gren_time = gpGlobals->time + 1;
        }
    }                            // GRENADE
    else if (CarryWeaponType() == KNIFE) {
        setMoveSpeed(f_max_speed);
        UTIL_BotPressKey(this, IN_ATTACK);        // Hold fire
    }                            // KNIFE
    else if (CarryWeaponType() == SNIPER) {
        setMoveSpeed(f_max_speed / 2);
        UTIL_BotPressKey(this, IN_ATTACK);        // Hold fire
        f_shoot_time = gpGlobals->time + 1.0;
    }                            // SNIPER
    else if (CarryWeaponType() == SHIELD) {
        if (fDistance > 550) {
            if (hasShieldDrawn()) {
                // when the enemy is far away, we keep it
            } else {
                // draw shield!
                UTIL_BotPressKey(this, IN_ATTACK2); // secondary attack makes shield draw
                f_allow_keypress = gpGlobals->time + 0.7;
            }
        } else {
            // get weapon here.
            if (hasShieldDrawn() && f_allow_keypress < gpGlobals->time) {
                rblog
                        ("BOT: Enemy is close enough, i should withdraw shield to attack this enemy\n");
                UTIL_BotPressKey(this, IN_ATTACK2);
                f_allow_keypress = gpGlobals->time + 0.7;
            }
        }
    } else {
        // debug print
        REALBOT_PRINT(this, "FireWeapon()", "Unknown weapon");
    }
}

/******************************************************************************
 Function purpose: The combat brain of the bot ( called by Think() )
 ******************************************************************************/
void cBot::Combat() {
    if (!hasEnemy()) {
        rprint("Unexpected call to Combat because bot has no enemy!");
        return;
    }

    // Bot is on ladder
    if (isOnLadder()) {
        // TODO: Bot fights when on ladder
        return;
    }

    // We have an enemy and it is now dead
    if (!isEnemyAlive()) {

        // radio (Enemy down)
        if (FUNC_DoRadio(this)) {
            UTIL_BotRadioMessage(this, 3, "9", "");
        }

        // get bot pointer
        cBot *checkpointer = UTIL_GetBotPointer(pEnemyEdict);

        // This bot killed a human; adjust skill when 'autoskill' is on.
        if (checkpointer == NULL) {

            // increase bot_skill value when autoskill enabled (making bot weaker)
            if (autoskill && bot_skill < 10) {
                bot_skill++;
            }

            if (Game.iDeathsBroadcasting != BROADCAST_DEATHS_NONE) {
                // This is a human, we will tell this human he has been killed
                // by a bot.
                int r = RANDOM_LONG(150, 255);
                int g = RANDOM_LONG(30, 155);
                int b = RANDOM_LONG(30, 155);
                char msg[128];
                if (Game.iDeathsBroadcasting == BROADCAST_DEATHS_FULL) {
                    sprintf(msg, "A RealBot has killed you!\n\nName:%s\nSkill:%d\n", name, bot_skill);
                } else {
                    sprintf(msg, "A RealBot named %s has killed you!", name);
                }

                HUD_DrawString(r, g, b, msg, pEnemyEdict);
            }
        }

        // clear the pointer for this and other bots that might have the same pEnemyEdict
        FUNC_ClearEnemyPointer(pEnemyEdict);

        // from here react after kill...
        forgetGoal();
        forgetPath();

        if (lastSeenEnemyVector != Vector(0, 0, 0)) {
            vHead = lastSeenEnemyVector;
        }

        lastSeenEnemyVector = Vector(0, 0, 0);

        // random waiting
        f_wait_time = gpGlobals->time + (1 + RANDOM_FLOAT(0.0, 0.4));

        // keep on walking when afraid (perhaps there are more enemies)
        if (RANDOM_LONG(0, 100) < ipFearRate)
            f_walk_time = gpGlobals->time + (1 + RANDOM_FLOAT(0.0, 2.0));

        InteractWithPlayers();    // check any new enemy here immediately

        return;
    }

    // ----------- combat

    // STEP 1: Pick best weapon to fight with
    PickBestWeapon();

    // STEP 2: Decide how to move to make us a harder target
    FightEnemy();

    // STEP 3: Aim at enemy (skill-based)
    AimAtEnemy();

    // STEP 4: Fire!
    FireWeapon();
}

/******************************************************************************
 Function purpose: Find cover
 Note: Using tracelines to get a cover node.
 ******************************************************************************/
void cBot::FindCover() {
    TraceResult tr;
    Vector dest = lastSeenEnemyVector;
    //  Vector start = pEdict->v.origin;
    //  Vector end;
    Vector cover_vect = Vector(9999, 9999, 9999);

    // When vector is visible, then look from that vector to the threat, if then NOT
    // Visible, then its cover.
    Vector v_src, v_right, v_left;

    // TraceLines in 2 directions to find which way to go...
    UTIL_MakeVectors(pEdict->v.v_angle);
    v_src = pEdict->v.origin + pEdict->v.view_ofs;
    v_right = v_src + gpGlobals->v_right * 90;
    v_left = v_src + gpGlobals->v_right * -90;

    // We have now our first 'left' and 'right'

    // First check the right..
    UTIL_TraceLine(v_src, v_right, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

    if (tr.flFraction >= 1.0) {
        // We can see it
        // Now trace from that vector to our threat
        UTIL_TraceLine(v_right, dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

        // If this is blocking.. then its a good wpt
        if (tr.flFraction < 1.0)
            cover_vect = v_right;
    }

    // Now check at the left
    UTIL_TraceLine(v_src, v_left, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

    if (tr.flFraction >= 1.0) {
        // We can see it
        // Now trace from that vector to our threat
        UTIL_TraceLine(v_left, dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

        // If this is blocking.. then its a good wpt
        if (tr.flFraction < 1.0) {
            // If we already found a wpt, then randomly pick this one
            if (cover_vect != Vector(9999, 9999, 9999)) {
                if (RANDOM_LONG(0, 100) < 50)
                    cover_vect = v_left;
            } else
                cover_vect = v_left;
        }
    }
    // Now update the V_left and V_right and do the checks again.
    //  Vector old_right = v_right;
    //  Vector old_left = v_left;
    v_right = v_src + gpGlobals->v_right * 180;
    v_left = v_src + gpGlobals->v_right * -180;

    // Now check at the right again
    UTIL_TraceLine(v_src, v_right, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

    if (tr.flFraction >= 1.0) {
        // We can see it
        // Now trace from that vector to our threat
        UTIL_TraceLine(v_right, dest, dont_ignore_monsters,
                       pEdict->v.pContainingEntity, &tr);

        // If this is blocking.. then its a good wpt
        if (tr.flFraction < 1.0) {
            // If we already found a wpt, then randomly pick this one
            if (cover_vect != Vector(9999, 9999, 9999)) {
                if (RANDOM_LONG(0, 100) < 50)
                    cover_vect = v_right;
            } else
                cover_vect = v_right;
        }
    }

    // Now check at the left
    UTIL_TraceLine(v_src, v_left, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);
    if (tr.flFraction >= 1.0) {
        // We can see it
        // Now trace from that vector to our threat
        UTIL_TraceLine(v_left, dest, dont_ignore_monsters,
                       pEdict->v.pContainingEntity, &tr);

        // If this is blocking.. then its a good wpt
        if (tr.flFraction < 1.0) {
            // If we already found a wpt, then randomly pick this one
            if (cover_vect != Vector(9999, 9999, 9999)) {
                if (RANDOM_LONG(0, 100) < 50)
                    cover_vect = v_left;
            } else
                cover_vect = v_left;
        }
    }

    int iNodeEnemy = NodeMachine.getClosestNode(pEnemyEdict->v.origin, 60, pEnemyEdict);
    int iNodeFrom = NodeMachine.getClosestNode(pEdict->v.origin, NODE_ZONE, pEdict);

    // --------------
    // TEST TEST TEST
    // --------------
    int iCoverNode = NodeMachine.node_cover(iNodeFrom, iNodeEnemy, pEdict);
    bool bTakenCover = false;

    if (iCoverNode > -1) {
        rprint("FindCover()", "cover node found (node based)");
        setGoalNode(iCoverNode);
        forgetPath();

        // Calculate a path to this position and get the heck there.
        createPath(iCoverNode);
        f_cover_time = gpGlobals->time + 8;
        bTakenCover = true;
    } else {

        // --------------------------------------------------
        // If cover_vect is found, we find a node close to it
        // --------------------------------------------------
        if (cover_vect != Vector(9999, 9999, 9999)) {
            rprint("FindCover()", "cover node found (cover_vect based)");
            int iNodeCover = NodeMachine.getClosestNode(cover_vect, 60, pEdict);
            if (iNodeCover > -1) {
                setGoalNode(iNodeCover);
                forgetPath();

                // Calculate a path to this position and get the heck there.
                rprint("createPath -> find cover node");
                NodeMachine.createPath(iNodeFrom, iNodeCover, iBotIndex, this, PATH_NONE);
                f_cover_time = gpGlobals->time + 8;
                bTakenCover = true;
            }
        }
    }

    // when we have taken cover, and we are leader, command our team to get
    // into our position to cover area
    if (bTakenCover) {
        // do something...
    }

} // FindCover()

void cBot::InteractWithFriends() {


    // TODO TODO TODO; make this thing really work
    return;

    // We interact with our players in some way
    //
    // When a bot is camping, another bot can choose to say 'go go go' for example.
    //
    //

    for (int i = 1; i <= gpGlobals->maxClients; i++) {

        edict_t *pPlayer = INDEXENT(i);

        // skip invalid players and skip self (i.e. this bot)
        if ((pPlayer) && (!pPlayer->free) && (pPlayer != pEdict)) {
            // skip this player if not alive (i.e. dead or dying)
            if (!IsAlive(pPlayer))
                continue;

            // skip enemies
            if (UTIL_GetTeam(pPlayer) != UTIL_GetTeam(pEdict))
                continue;

            bool bCanSeePlayer = false;
            bool bClose = false;

            Vector vVecEnd = pPlayer->v.origin + pPlayer->v.view_ofs;

            if (func_distance(pPlayer->v.origin, pEdict->v.origin) < 450)
                bClose = true;

            if (FInViewCone(&vVecEnd, pEdict) && FVisible(vVecEnd, pEdict))
                bCanSeePlayer = true;

            // there are tons of cases
            cBot *pBotPointer = UTIL_GetBotPointer(pPlayer);

            // It is a fellow bot
            if (pBotPointer != NULL) {
                if (bClose) {
                    if (pBotPointer->f_camp_time > gpGlobals->time
                        && pBotPointer->f_camp_time - 10 < gpGlobals->time
                        && pBotPointer->pEnemyEdict == NULL
                        && (RANDOM_LONG(0, 100) < ipCampRate
                            && FUNC_DoRadio(this))) {
                        // issue go go go
                        UTIL_BotRadioMessage(this, 2, "1", "");       // go go go!
                    }
                }

                if (bCanSeePlayer) {}
            } else                 // it is a teammate, but it is human (or a different bot)
            {
                // when firing

            }

            // any player:
            if (bClose) {
                // some one is close,  need backup?
                if (RANDOM_LONG(0, 100) < ipFearRate && pEnemyEdict != NULL)
                    if (FUNC_DoRadio(this)) {
                        UTIL_BotRadioMessage(this, 3, "3", "");       // need backup
                    }
            }
        }
    }

}

// BOT: Interact with Players ('find enemy, and how to react upon them')
void cBot::InteractWithPlayers() {

    // friends are important, we are a team dudes!
    InteractWithFriends();

    int result = FindEnemy();

    // -------------------------------
    // RESULT < 0; NO ENEMY FOUND
    // -------------------------------

    // No enemy found, unzoom
    if (result < 0) {
        // Keep f_prim_weapon updated, else we do burst immidiatly
        if (CarryWeaponType() == SNIPER) {

            // Unzoom (for sniper guns)
            if (zoomed > ZOOM_NONE && f_allow_keypress < gpGlobals->time) {
                UTIL_BotPressKey(this, IN_ATTACK2);
                f_allow_keypress = gpGlobals->time + 0.7;
                zoomed++;
            }
            if (zoomed > ZOOM_TWICE)
                zoomed = ZOOM_NONE;
        } else if (FUNC_BotHoldsZoomWeapon(this)) {

            // Unzoom (for other guns with only 1 zoom)
            if (zoomed > ZOOM_NONE && f_allow_keypress < gpGlobals->time) {
                UTIL_BotPressKey(this, IN_ATTACK2);
                f_allow_keypress = gpGlobals->time + 0.7;
                zoomed = ZOOM_NONE;
            }
        } else {

            // For any weapon that has a silencer (the colt for example), use it if we want that.
            if (isHoldingWeapon(CS_WEAPON_M4A1))
                if (bot_use_special == 0 && zoomed == ZOOM_NONE
                    && f_allow_keypress < gpGlobals->time) {
                    UTIL_BotPressKey(this, IN_ATTACK2);
                    zoomed = ZOOM_ONCE;
                }
        }
    }
    // ------------------------------------------------
    // RESULT > -1 ; ENEMY FOUND / NO SPECIFIC REACTION
    // ------------------------------------------------
    if (result > -1 && result < 4) {

        // VIP: When we found an enemy, we have a problem.
        if (vip) {

            // We do not forget our enemy, but we will try to get the heck out of here.
            // TODO TODO TODO: code something here?
        }
        // Whenever we hold a knife, get our primary weapon
        if (CarryWeapon(CS_WEAPON_KNIFE)) {

            // switch back to primary
            if (iPrimaryWeapon > -1)
                UTIL_SelectItem(pEdict, UTIL_GiveWeaponName(iPrimaryWeapon));

            else                   // pick secondary
                UTIL_SelectItem(pEdict, UTIL_GiveWeaponName(iSecondaryWeapon));

            f_update_weapon_time = gpGlobals->time + 0.7;
        }
    }
    // ------------------------------------------------
    // RESULT = 1 ; ENEMY FOUND, VIA FRIEND!
    // ------------------------------------------------

    // When we have found an enemy via a friend, we simply build a path to it.
    if (result == 1) {

        /*
           f_prim_weapon = gpGlobals->time;

           // DECIDE:
           // Do we go into battle, or do we wait first a few seconds?

           // HEALTH: The less we have, the more we want to wait
           int vHealth = 100-bot_health;

           // CAMP: The more we want to camp, the more we want to wait.
           int vCamp = ipCampRate;

           if (RANDOM_LONG(0,200) < (vHealth+vCamp))
           {
           // depending on how much we want, the longer we wait
           float fWaitTime = ((200/(vHealth+vCamp))*5);
           f_wait_time = gpGlobals->time + fWaitTime;

           // TODO TODO TODO; we might not even want to wait, but also take 'cover'?
           }

           // INITIALIZATION:
           int iGoal = NodeMachine.getCloseNode(pBotEnemy->v.origin, NODE_ZONE, pBotEnemy);
           if (iGoal > -1)
           {
           iGoalNode = iGoal;
           pathNodeIndex = -1;
           }
         */
    }
    // ------------------------------------------------
    // RESULT = 0 ; NEW ENEMY FOUND
    // ------------------------------------------------
    if (result == 0) {
        // First Encounter
        //f_prim_weapon = gpGlobals->time;
        if (CarryWeaponType() == SNIPER) {
            if (zoomed < ZOOM_TWICE && f_allow_keypress < gpGlobals->time) {
                UTIL_BotPressKey(this, IN_ATTACK2);
                f_allow_keypress = gpGlobals->time + 0.7;
                zoomed++;
            }
        }

        // INITIALIZATION:
        int iGoal = NodeMachine.getClosestNode(pEnemyEdict->v.origin, NODE_ZONE, pEnemyEdict);
        if (iGoal > -1) {
            rprint_trace("InteractWithPlayers()", "Found a new enemy, setting goal and forgetting path");
            setGoalNode(iGoal);
            forgetPath();
        }

        // Speed our enemy runs
        // int run_speed = FUNC_PlayerSpeed(pBot->pBotEnemy);
        // Distance between Us and Enemy.
        // float f_distance = func_distance(pBot->pEdict->v.origin,
        // pBot->pBotEnemy->v.origin);

        // Does our enemy (when a bot) has focus on us?
        bool focused = false;
        cBot *playerbot = UTIL_GetBotPointer(pEnemyEdict);
        if (playerbot) {
            if (playerbot->pEnemyEdict == pEdict)
                focused = true;
        } else                      // Its a human
        {

            // When we are in his 'sight' of 25 degrees , we are pretty
            // much focussed for a first encounter.
            if (FUNC_InFieldOfView
                        (pEdict, (pEnemyEdict->v.origin - pEdict->v.origin)) < 25)
                focused = true;
        }

        /******************************
        At this moment we know:
        - The distance between us and enemy
        - The focus (are we targetted too?)
        - The speed of the enemy (running, standing still? etc)
        *******************************/
    }                            // We have a first encounter

    // ------------------------------------------------
    // RESULT = 3 ; NEWER ENEMY FOUND
    // ------------------------------------------------
    if (result == 3) {
        //
        // Newer enemy found, update goals and such, but thats all!
        //

        // INITIALIZATION:
        int iGoal = NodeMachine.getClosestNode(pEnemyEdict->v.origin, NODE_ZONE, pEnemyEdict);

        if (iGoal > -1) {
            rprint_trace("InteractWithPlayers()", "Found a *newer* enemy, so picking goal node to that");
            setGoalNode(iGoal);
            forgetPath();
        }
    }
}

// BOT: INTERACT WITH PLAYERS
void cBot::JoinTeam() {
    if (mod_id != CSTRIKE_DLL) return;
    // When bot plays Counter-Strike (only Counter-Strike is supported)

    char c_team[32];
    char c_class[32];

    // Choose team first
    if (start_action == MSG_CS_TEAM_SELECT) {
        start_action = MSG_CS_IDLE;    // switch back to idle

        // in case of bad state/input fall-back to 'pick one for me'
        if ((iTeam != 1) && (iTeam != 2) && (iTeam != 5)) {
            iTeam = 5;
        }

        // select the team the bot wishes to join...
        if (iTeam == 1) {
            strcpy(c_team, "1");
        } else if (iTeam == 2) {
            strcpy(c_team, "2");
        } else {
            strcpy(c_team, "5");
        }

        // choose
        FakeClientCommand(this->pEdict, "menuselect", c_team, NULL);

        return;
    }

    // counter terrorist menu, which class/outfit?
    if (start_action == MSG_CS_CT_SELECT) {
        start_action = MSG_CS_IDLE;    // switch back to idle

        if ((bot_class < 1) || (bot_class > 4))
            bot_class = 5;      // use random if invalid

        // Since cs 1.6 does not give us pretty random models
        // we do it ourselves
        if (bot_class == 5) {
            bot_class = RANDOM_LONG(1, 4);
        }

        // select the class the bot wishes to use...
        if (bot_class == 1)
            strcpy(c_class, "1");
        else if (bot_class == 2)
            strcpy(c_class, "2");
        else if (bot_class == 3)
            strcpy(c_class, "3");
        else if (bot_class == 4)
            strcpy(c_class, "4");
        else
            strcpy(c_class, "5");       // random

        FakeClientCommand(this->pEdict, "menuselect", c_class, NULL);

        // bot has now joined a team
        hasJoinedTeam = true;

        return;
    }

    // terrorist select
    if (start_action == MSG_CS_T_SELECT) {
        start_action = MSG_CS_IDLE;    // switch back to idle

        if ((bot_class < 1) || (bot_class > 4))
            bot_class = 5;      // use random if invalid

        // Since cs 1.6 does not give us pretty random models
        // we do it ourselves
        if (bot_class == 5)
            bot_class = RANDOM_LONG(1, 4);

        // select the class the bot wishes to use...
        if (bot_class == 1)
            strcpy(c_class, "1");
        else if (bot_class == 2)
            strcpy(c_class, "2");
        else if (bot_class == 3)
            strcpy(c_class, "3");
        else if (bot_class == 4)
            strcpy(c_class, "4");
        else
            strcpy(c_class, "5");       // random

        FakeClientCommand(this->pEdict, "menuselect", c_class, NULL);

        // bot has now joined the game (doesn't need to be started)
        hasJoinedTeam = true;

        return;
    }
}

int cBot::ReturnTurnedAngle(float speed, float current, float ideal) {

    // hope this fix the unnescesary turning of bots.
    // how? we save the values here, andc alculate the new value.
    // this part is copied from botchangeyaw/pitch so it SHOULD work :)
    float current_180;           // current +/- 180 degrees
    float diff;

    // turn from the current v_angle pitch to the idealpitch by selecting
    // the quickest way to turn to face that direction

    // find the difference in the current and ideal angle
    diff = fabs(current - ideal);

    // check if the bot is already facing the idealpitch direction...
    if (diff <= 1.0)
        return (int) current;     // return number of degrees turned

    // check if difference is less than the max degrees per turn
    if (diff < speed)
        speed = diff;             // just need to turn a little bit (less than max)

    // here we have four cases, both angle positive, one positive and
    // the other negative, one negative and the other positive, or
    // both negative.  handle each case separately...
    if ((current >= 0.0) && (ideal >= 0.0))      // both positive
    {
        if (current > ideal)
            current -= speed;

        else
            current += speed;
    } else if ((current >= 0.0) && (ideal < 0.0)) {
        current_180 = current - 180.0;
        if (current_180 > ideal)
            current += speed;

        else
            current -= speed;
    } else if ((current < 0) && (ideal >= 0)) {
        current_180 = current + 180;
        if (current_180 > ideal)
            current += speed;

        else
            current -= speed;
    } else                         // (current < 0) && (ideal < 0)  both negative
    {
        if (current > ideal)
            current -= speed;

        else
            current += speed;
    }

    // check for wrap around of angle...
    if (current > 180)
        current -= 360;
    if (current < -180)
        current += 360;
    return (int) current;        // return what it should be
}

// BOT: sub-function (DEFUSE) for ACT()
bool cBot::Defuse() {
    if (!isCounterTerrorist()) // non-Counter-Terrorists have no business here
        return false;

    // this bot is defusing
    if (shouldActWithC4() && keyPressed(IN_USE)) {
        setTimeToMoveToNode(3);
        return true;
    }

    // What i do, i search for the c4 timer, store its origin and check
    // if this bot is close. If so, the bot should be defusing the bomb
    // if the timers are set. The above check makes sure that no other
    // bot will be defusing the bomb.
    edict_t *pent = NULL;
    bool c4Found = false;
    while ((pent = UTIL_FindEntityByClassname(pent, "grenade")) != NULL) {
        if (UTIL_GetGrenadeType(pent) == 4) {     // It is a C4
            c4Found = true;
            break;
        }
    }

    if (!c4Found) {
        rprint_normal("Defuse()", "No C4 planted yet");
        return false;
    }

    rprint_normal("Defuse()", "C4 is planted!");

    // A c4 has been found, oh dear.
    // Remember, pent=c4 now!

    // Calculate the distance between our position to the c4
    Vector vC4 = pent->v.origin;
    float distance = func_distance(pEdict->v.origin, vC4);

    // can see C4
    bool canSeeC4 = canSeeVector(vC4);

    if (!canSeeC4) {
        rprint_trace("Defuse()", "Cannot see planted C4 - bailing");
        return false;
    }
    
    // it can be seen, so it has been discovered
    if (!Game.isPlantedC4Discovered()) {
        this->rprint_trace("Defuse()", "C4 is discovered, remembering its coordinates");
        Game.vPlantedC4 = vC4;
    }

    // We can do 2 things now
    // - If we are not close, we check if we can walk to it, and if so we face to the c4
    // - If we are close, we face it and (begin) defuse the bomb.
    int distanceForC4ToBeInReach = 70;
    if (distance < distanceForC4ToBeInReach) {
        vHead = vC4;
        vBody = vC4;

        setTimeToMoveToNode(3); // we are going to do non-path-follow stuff, so keep timer updated
        int angle_to_c4 = FUNC_InFieldOfView(pEdict, (vC4 - pEdict->v.origin));

        // if defusion timer has not been set (ie, the bot is not yet going to defuse the bomb)
        if (f_defuse < gpGlobals->time && angle_to_c4 < 35) {
            this->rprint("Defuse()", "I'll start defusing the bomb");
            // when we are 'about to' defuse, we simply set the timers
            f_defuse = gpGlobals->time + 90; // hold as long as you can
            f_allow_keypress = gpGlobals->time + 1.5;        // And stop any key pressing the first second
            // ABOUT TO DEFUSE BOMB
        }

        // Defusion timer is set and c4 is within vision
        if (f_defuse > gpGlobals->time && angle_to_c4 < 35) {
            this->rprint("Defuse()", "I'm defusing the bomb");
            setMoveSpeed(0.0);
            f_c4_time = gpGlobals->time + 6;
            UTIL_BotPressKey(this, IN_DUCK);

            if (func_distance(pEdict->v.origin, vC4) > 50
                && f_allow_keypress + 0.5 > gpGlobals->time) {
                setMoveSpeed(f_max_speed / 2);
            }
        }

        if (f_allow_keypress < gpGlobals->time && f_defuse > gpGlobals->time) {
            UTIL_BotPressKey(this, IN_USE);
        }

    } else {
        rprint_trace("Defuse()", "I can see C4, but it is out of reach.");
        int iGoalNode = NodeMachine.getClosestNode(vC4, distanceForC4ToBeInReach, NULL);
        if (iGoalNode < 0) {
            rprint_normal("Defuse()", "No node close, so just look at it/body face at it and move towards it.");
            vHead = vC4;
            vBody = vC4;
        }

        if (iGoalNode > -1) {
            // we are not heading for this goal yet
            if (iGoalNode > -1 && getGoalNode() != iGoalNode) {
                rprint_normal("Defuse()", "I don't have a goal towards the C4, overriding it now to C4 destination!");
                forgetPath();
                forgetGoal();
                setGoalNode(iGoalNode);
            } else {
                rprint_normal("Defuse()", "I already have a goal towards the C4!");
            }
        } else {
            rprint_normal("Defuse()", "C4 is somewhere without a close node.");
        }
        setMoveSpeed(f_max_speed);
    }                      // distance < ...

    // we can see the bomb, and we act upon it
    return true;
}

int cBot::keyPressed(int key) const {
    return pEdict->v.button & key;
}

// BOT: Act
void cBot::Act() {
    // chat
    if (fChatTime < gpGlobals->time) {
        if (chChatSentence[0] != '\0') {
            UTIL_SayTextBot(chChatSentence, this);
            memset(chChatSentence, 0, sizeof(chChatSentence));
        }
    }

    // camp
    if (f_camp_time > gpGlobals->time) {
        // When camping we duck and we don't move
        // todo, camping can be done standing too, but this does not look 'cool' atm.
        UTIL_BotPressKey(this, IN_DUCK);

        setMoveSpeed(0.0);       // do not move
        PickBestWeapon();         // pick weapon, do not stare with knife

        // when dropped C4 and CT we look at C4
        if (isCounterTerrorist() && Game.vDroppedC4 != Vector(9999, 9999, 9999)) {
            // look at dropped C4
            if (EntityIsVisible(pEdict, Game.vDroppedC4))
                vHead = Game.vDroppedC4;
            else {
                if (iGoalNode > -1)
                    vHead = vBody = NodeMachine.node_vector(iGoalNode);
                else {
                    // cannot find a node to the dropped C4, so where do we look at?
                    // todo : find a node to look at, i.e. something dangerous
                }
            }
        } else {
            // Look at iGoalNode
            if (iGoalNode > -1)
                vHead = vBody = NodeMachine.node_vector(iGoalNode);
            else {
                // look where to look at?
                // todo : find a node to look at, i.e. something dangerous
            }
        }
    }

    // C4 timer is set, this means:
    // T -> Is planting bomb
    // CT-> Is defusing bomb
    if (shouldActWithC4()) {
        // make sure we override this, or else we learn that we get stuck or something
        // which is not the case.
        setTimeToMoveToNode(2);

        // terrorist
        if (isTerrorist()) {
            // When still having the C4
            setMoveSpeed(0.0f);
//            f_strafe_speed = 0.0;

            // When no C4 selected yet, select it
            if (!isHoldingWeapon(CS_WEAPON_C4)) {
                UTIL_SelectItem(pEdict, "weapon_c4");
            } else {
                UTIL_BotPressKey(this, IN_ATTACK);  // plant it!
            }

            // When we no longer have the C4 , we stop doing this stupid shit
            if (!hasBomb() || Game.bBombPlanted) {
                rprint_trace("Act()", "I was planting the C4, and it got planted (I no longer have the C4), so find a nearby node to camp/guard the C4");
                f_c4_time = gpGlobals->time;
                setGoalNode(NodeMachine.getClosestNode(pEdict->v.origin, 200, pEdict));
                iPathFlags = PATH_CAMP;
                forgetPath();
            }
        } else {
            // counter-terrorist
            Defuse();              // old routine from RB AI V1.0 defusing, should get here and more cleaned up
        }
    }

    if (f_strafe_time < gpGlobals->time) {
        f_strafe_speed = 0;
    }

    // walk only when NOT holding duck (is same as walking, combination makes bot super slow)
    if (f_walk_time > gpGlobals->time && !(pEdict->v.button & IN_DUCK)) {
        // From "KickBot":  return (float) (((int)flMaxSpeed)/2 + ((int)flMaxSpeed)/50);
        //OLD: f_move_speed = f_max_speed / 2.0; // this is not correct

        pEdict->v.button &= (~IN_RUN);    // release IN_RUN
        rprint("Act", "Walk time > gpGlobals->time");
        setMoveSpeed((float) (((int) f_max_speed) / 2 + ((int) f_max_speed) / 50));
    }

    // When we are at max speed, press IN_RUN to get a running animation
    if (f_move_speed == f_max_speed) {
        UTIL_BotPressKey(this, IN_RUN);
    }

    if (!keyPressed(IN_MOVELEFT) || keyPressed(IN_MOVERIGHT)) {
        if (f_strafe_speed > 0.0f) {
            UTIL_BotPressKey(this, IN_MOVERIGHT);
        }
        else if (f_strafe_speed < 0.0f) {
            UTIL_BotPressKey(this, IN_MOVELEFT);
        }
    }

    // When we should go back, we go back
    if (f_goback_time > gpGlobals->time) {
        setMoveSpeed(-f_max_speed);
    }

    // When holding duck, we hold duck
    if (f_hold_duck > gpGlobals->time)
        UTIL_BotPressKey(this, IN_DUCK);

    // When we wait, we have no move speed
    // notice: 'wait' is not 'stuck' nor 'camping'. Wait should only be used to have a bot
    // 'do nothing' for a short period of time.
    if (f_wait_time > gpGlobals->time) {
        rprint("Act", "f_wait_time > gpGlobals->time");
        setMoveSpeed(0.0);
    }

    // Button usage, change vBody to a 'trigger multiple' because we have to touch these
    if (pButtonEdict) {
        if (strcmp(STRING(pButtonEdict->v.classname), "trigger_multiple") == 0) {
            if (func_distance(pEdict->v.origin, VecBModelOrigin(pButtonEdict)) < 60) {
                vBody = VecBModelOrigin(pButtonEdict);
            }
        }
    }

    // -------------------------------------------
    // MOVE TO : vBody
    // calculate the angle we MOVE to. (VecMoveAngles)
    // -------------------------------------------
    Vector vTarget = vBody - pEdict->v.origin;
    vecMoveAngles = UTIL_VecToAngles(vTarget);

    // Paulo-La-Frite - START bot aiming bug fix
    if (vecMoveAngles.x > 180)
        vecMoveAngles.x -= 360;

    vecMoveAngles.x = -vecMoveAngles.x;
    vecMoveAngles.z = 0;
    UTIL_FixAngles(&vecMoveAngles);

    // when filled in, we look to this (overrides)
    if (vEar != Vector(9999, 9999, 9999))
        vHead = vEar;

    // button overrides hearing
    if (pButtonEdict)
        vHead = VecBModelOrigin(pButtonEdict);

    // -------------------------------------------
    // FACE AT: vHead
    // calculate the angle we face at.
    //
    // -------------------------------------------
    vTarget = (vHead - pEdict->v.origin);
    pEdict->v.v_angle = UTIL_VecToAngles(vTarget);
    if (pEdict->v.v_angle.y > 180)
        pEdict->v.v_angle.y -= 360;

    // Paulo-La-Frite - START bot aiming bug fix
    if (pEdict->v.v_angle.x > 180)
        pEdict->v.v_angle.x -= 360;

    Vector v_shouldbe = pEdict->v.angles;

    // Vector how it should be, however, we don't allow such a fast turn!
    v_shouldbe.x = pEdict->v.v_angle.x / 3;
    v_shouldbe.y = pEdict->v.v_angle.y;
    v_shouldbe.z = 0;

    // set the body angles to point the gun correctly
    pEdict->v.angles.x = ReturnTurnedAngle(ipTurnSpeed, pEdict->v.angles.x, v_shouldbe.x);
    pEdict->v.angles.y = ReturnTurnedAngle(ipTurnSpeed, pEdict->v.angles.y, v_shouldbe.y);
    pEdict->v.angles.z = 0;

    // adjust the view angle pitch to aim correctly (MUST be after body v.angles stuff)
    pEdict->v.v_angle.x = -pEdict->v.v_angle.x;

    // Paulo-La-Frite - END
    pEdict->v.ideal_yaw = pEdict->v.v_angle.y;
    pEdict->v.idealpitch = pEdict->v.v_angle.x;

    botFixIdealYaw(pEdict);
    botFixIdealPitch(pEdict);
}

bool cBot::shouldActWithC4() const {
    return f_c4_time > gpGlobals->time;
}

// BOT: On ladder?
bool cBot::isOnLadder() {
    return FUNC_IsOnLadder(pEdict);
}

// BOT: Check around body and avoid obstacles
void cBot::CheckAround() {
    rprint_trace("CheckAround", "Start");
    // Do not act when on ladder
    if (isOnLadder())
        return;

    // The principle is to fire 2 tracelines, both forward; one left
    // and one right. When one of the 2 gets hit, we know we are 'about'
    // to get hit. Therefor we use strafing to keep distance to the coming wall
    // when left and right is both hit we have a problem as this should not happen.

    // Note: we use TRACEHULL instead of TRACELINE, because TRACEHULL detects
    // the famous 'undetectable' func_walls.
    TraceResult tr;
    Vector v_source, v_left, v_right, v_forward, v_forwardleft, v_forwardright;

//    v_source = pEdict->v.origin + Vector(0, 0, -CROUCHED_HEIGHT + (MAX_JUMPHEIGHT + 1));
    v_source = pEdict->v.origin + Vector(0, 0, ORIGIN_HEIGHT);

    // Go forward first
    float distance = 90;
    v_forward = v_source + gpGlobals->v_forward * distance;

    // now really go left/right
    v_right = v_source + gpGlobals->v_right * distance;
    v_left = v_source + gpGlobals->v_right * -distance;

    // now really go left/right
    v_forwardright = v_right + gpGlobals->v_forward * distance;
    v_forwardleft = v_left + gpGlobals->v_forward * -distance;

    // TRACELINE: forward
    UTIL_TraceHull(v_source, v_forward, dont_ignore_monsters, point_hull,  pEdict->v.pContainingEntity, &tr);
    bool bHitForward = tr.flFraction < 1.0;

    // TRACELINE: Left
    UTIL_TraceHull(v_source, v_left, dont_ignore_monsters, point_hull, pEdict->v.pContainingEntity, &tr);
    bool bHitLeft = tr.flFraction < 1.0;

    // TRACELINE: Right
    UTIL_TraceHull(v_source, v_right, dont_ignore_monsters, point_hull, pEdict->v.pContainingEntity, &tr);
    bool bHitRight = tr.flFraction < 1.0;

    // TRACELINE: Forward left
    UTIL_TraceHull(v_source, v_forwardleft, dont_ignore_monsters, point_hull, pEdict->v.pContainingEntity, &tr);
    bool bHitForwardLeft = tr.flFraction < 1.0;

    // TRACELINE: Forward right
    UTIL_TraceHull(v_source, v_forwardright, dont_ignore_monsters, point_hull, pEdict->v.pContainingEntity, &tr);
    bool bHitForwardRight = tr.flFraction < 1.0;


    char msg[255];
    sprintf(msg, "HIT results: forward: %d, left: %d, right: %d, forward left: %d, forward right: %d", bHitForward, bHitLeft, bHitRight, bHitForwardLeft, bHitForwardRight);
    rprint_trace("CheckAround", msg);

    // Set 'act' properties

    // we are surrounded, so move backwards
    if (bHitForward) {
        rprint_trace("CheckAround", "Something in front of me blocks, so move back.");
//        f_move_speed = -(f_max_speed);
    } else {
        rprint_trace("CheckAround", "Nothing in front of me");
    }

    if (!bHitForwardLeft && bHitForwardRight) {
        strafeLeft(0.5);
        rprint_trace("CheckAround", "Can strafe left (forward left)");
    } else if (bHitForwardLeft && !bHitForwardRight) {
        strafeRight(0.5);
        rprint_trace("CheckAround", "Can strafe right (forward right)");
    }

    if (bHitLeft && bHitRight) {
        rprint_trace("CheckAround", "Can't strafe left or right");
    } else if (!bHitLeft && bHitRight) {
        strafeLeft(0.5);
        rprint_trace("CheckAround", "Can strafe left");
    } else if (bHitLeft && !bHitRight) {
        strafeRight(0.5);
        rprint_trace("CheckAround", "Can strafe right");
    }

    // -------------------------------------------------------------
    // When checking around a bot also handles breakable stuff.
    // -------------------------------------------------------------
    char item_name[40];
    edict_t *pent = NULL;
    while ((pent = UTIL_FindEntityInSphere(pent, pEdict->v.origin, 60)) != NULL) {
        strcpy(item_name, STRING(pent->v.classname));

        // See if it matches our object name
        if (strcmp("func_breakable", item_name) == 0) {

            // Found a func_breakable
            Vector vBreakableOrigin = VecBModelOrigin(pent);

            // Shoot
            if ((pent->v.flags & FL_WORLDBRUSH) == 0)      // can it be broken?
            {

                // Thx for CF by fixing breakable coding
                if (pent->v.solid == SOLID_BSP && pent->v.takedamage == DAMAGE_YES && pent->v.impulse == 0 &&
                    pent->v.health < 150) // has it NOT been broken yet?
                {

                    // trace to vector to be sure we dont get blocked by anything else
                    if (VectorIsVisibleWithEdict(pEdict, vBreakableOrigin, "func_breakable")) {
                        setHeadAiming(vBreakableOrigin);
                        FireWeapon();
                    }
                    return;
                }
            }
        }                         // CAN BE BROKEN
    }                            // FUNC_BREAKABLE
}

// BOT: Should be taking cover?
bool cBot::TakeCover() {

    // Its time based.
    if (f_cover_time < gpGlobals->time)
        return false;

    // And if all went fine, we can return true.
    return true;
}

/**
 * Set the node to follow next as the next one (ie, increase index)
 */
void cBot::nextPathIndex() {
    this->pathIndex++;
}

/**
 * Set the node to follow next as the previous one (ie, decrease index). Calls forgetPath when index is getting < 0
 */
void cBot::prevPathIndex() {
    rprint("prevPathNodeIndex");
    this->pathIndex--;
    if (this->pathIndex < 0) {
        forgetPath();
    }
}

// Returns true if bot has a path to follow
bool cBot::isWalkingPath() {
    return this->pathIndex > -1;
}

// Returns true if bot has goal node
bool cBot::hasGoal() {
    return this->iGoalNode > -1;
}

// Returns true if bot has goal node index (ie referring to Goals[])
bool cBot::hasGoalIndex() {
    return this->goalIndex > -1;
}

/**
 * Returns goal data , if goal data exists
 * @return
 */
tGoal *cBot::getGoalData() {
    if (!hasGoalIndex()) return NULL;
    tGoal *ptr = NodeMachine.getGoal(this->goalIndex);
    if (ptr == NULL) return NULL;

    // only goals with a node are valid
    if (ptr->iNode > -1) return ptr;
    // else not

    return NULL;
}

// Returns true if bot has an enemy edict
bool cBot::hasEnemy() {
    return this->pEnemyEdict != NULL;
}

/**
 * Returns true when given edict == our enemy edict
 * @param pEdict
 * @return
 */
bool cBot::hasEnemy(edict_t * pEdict) {
    return this->pEnemyEdict == pEdict;
}

// Returns true if bot has a path to follow
bool cBot::shouldBeWandering() {
    if (this->fWanderTime > gpGlobals->time) {
        char msg[255];
        memset(msg, 0, sizeof(msg));
        sprintf(msg, "Wander time is %f , globals time is %f, so should still wander", this->fWanderTime, gpGlobals->time);
        rprint(msg);
        return true;
    }
    return false;
}

void cBot::setMoveSpeed(float value) {
//    char msg[255];
//    sprintf(msg, "setting to value %f / maxSpeed %f - sv_maxspeed = %f", value, this->f_max_speed, CVAR_GET_FLOAT("sv_maxspeed"));
//    rprint_trace("setMoveSpeed", msg);
    this->f_move_speed = value;
}

void cBot::setStrafeSpeed(float value, float time) {
    char msg[255];
    sprintf(msg, "%f for %f seconds.", value, time);
    rprint_trace("setStrafeSpeed", msg);
//    if (f_strafe_time > gpGlobals->time) {
//
//    } else {
    f_strafe_speed = value;
    f_strafe_time = gpGlobals->time + time;
//    }
}

void cBot::strafeLeft(float time) {
    setStrafeSpeed(-f_max_speed, time);
}

void cBot::strafeRight(float time) {
    setStrafeSpeed(f_max_speed, time);
}

void cBot::startWandering(float time) {
    this->fWanderTime = gpGlobals->time + time;
    setMoveSpeed(f_max_speed);
    char msg[255];
    memset(msg, 0, sizeof(msg));
    sprintf(msg, "Start wandering for %f seconds", time);
    rprint("startWandering", msg);
}

void cBot::stopMoving() {
    this->setMoveSpeed(0.0);
}

void cBot::forgetGoal() {
    rprint_trace("forgetGoal");
    this->iGoalNode = -1;
    this->goalIndex = -1;
}

int cBot::getPathIndex() {
    return this->pathIndex;
}

int cBot::getPreviousPathIndex() {
    return this->pathIndex - 1;
}

void cBot::forgetPath() {
    rprint("forgetPath");
    this->pathIndex = -1;
    NodeMachine.path_clear(this->iBotIndex);
}

void cBot::forgetEnemy() {
    this->pEnemyEdict = NULL;
}

edict_t * cBot::getEnemyEdict() {
    return this->pEnemyEdict;
}

int cBot::getGoalNode() {
    return this->iGoalNode;
}

void cBot::setGoalNode(int nodeIndex, int iGoalIndex) {
    if (nodeIndex < 0) {
        rprint("setGoalNode()", "WARN: Setting a goal lower than 0, assuming this is not intentional. If you need to forget a goal, use forgetGoal()");
    }
    this->iGoalNode = nodeIndex;
    this->goalIndex = iGoalIndex;

    tGoal *goalPtr = getGoalData();
    char msg[255];
    memset(msg, 0, sizeof(msg));

    if (goalPtr != NULL) {
        sprintf(msg, "Setting iGoalNode to [%d] and goalIndex [%d] - GOAL: type [%s], checked [%d]",
                nodeIndex,
                goalIndex,
                goalPtr->name,
                goalPtr->iChecked
        );
    } else {
        sprintf(msg, "Setting iGoalNode to [%d] and goalIndex [%d] - could not retrieve goal data.", nodeIndex, goalIndex);
    }
    rprint("setGoalNode()", msg);
}

void cBot::setGoalNode(int nodeIndex) {
    this->setGoalNode(nodeIndex, -1);
}

void cBot::setGoalNode(tGoal *goal) {
    if (goal != NULL && goal->iNode > -1) {
        rprint("setGoalNode with goal pointer\n");
        this->setGoalNode(goal->iNode, goal->index);
    }
}

/**
 * Always printed when debug mode is on
 * @param Function
 * @param msg
 */
void cBot::rprint(const char *Function, const char *msg) {
    REALBOT_PRINT(this, Function, msg);
}

/**
 * Only printed when debug mode is on and verbosity is trace
 * @param Function
 * @param msg
 */
void cBot::rprint_trace(const char *Function, const char *msg) {
    if (Game.messageVerbosity > 1) {
        REALBOT_PRINT(this, Function, msg);
    }
}

/**
 * Only printed when debug mode is on and verbosity is normal
 * @param Function
 * @param msg
 */
void cBot::rprint_normal(const char *Function, const char *msg) {
    if (Game.messageVerbosity > 1) {
        REALBOT_PRINT(this, Function, msg);
    }
}

void cBot::rprint(const char *msg) {
    rprint("rprint()", msg);
}

void cBot::rprint_normal(const char *msg) {
    rprint_normal("rprint()", msg);
}

void cBot::rprint_trace(const char *msg) {
    rprint_trace("rprint()", msg);
}

bool cBot::hasBomb() {
    return isOwningWeapon(CS_WEAPON_C4);
}

bool cBot::isCounterTerrorist() {
    return iTeam == 2;
}

bool cBot::isTerrorist() {
    return iTeam == 1;
}

bool cBot::hasPrimaryWeaponEquiped() {
    return iPrimaryWeapon > -1;
}

bool cBot::hasSecondaryWeaponEquiped() {
    return iSecondaryWeapon > -1;
}

bool cBot::hasSecondaryWeapon(int weaponId) {
    return isOwningWeapon(weaponId);
}

void cBot::performBuyWeapon(const char *menuItem, const char *subMenuItem) {
    // To be sure the console will only change when we MAY change.
    // The values will only be changed when console_nr is 0
    if (Game.getRoundStartedTime() + 4 < gpGlobals->time)
        return;                   // Not valid to buy

    if (this->console_nr == 0) {
        // set up first command and argument
        strcpy(this->arg1, "buy");
        strcpy(this->arg2, menuItem);

        if (subMenuItem != NULL) strcpy(this->arg3, subMenuItem);

        this->console_nr = 1;     // start console command sequence
    }
}

void cBot::performBuyActions(int weaponIdToBuy) {
    if (weaponIdToBuy < 0) {
        return;
    }
    // Buy...

    // TODO
    // FRASHMAN 30.08.04 haven't changed the cs 1.5 buycode, maybe there are also errors

    // CS 1.5 only
    if (counterstrike == 0) {
        switch (weaponIdToBuy) {
            case CS_WEAPON_AK47:
                performBuyWeapon("4", "1");
                break;
            case CS_WEAPON_DEAGLE:
                performBuyWeapon("1", "3");
                break;
            case CS_WEAPON_P228:
                performBuyWeapon("1", "4");
                break;
            case CS_WEAPON_SG552:
                performBuyWeapon("4", "2");
                break;
            case CS_WEAPON_SG550:
                performBuyWeapon("4", "8");
                break;
            case CS_WEAPON_SCOUT:
                performBuyWeapon("4", "5");
                break;
            case CS_WEAPON_AWP:
                performBuyWeapon("4", "6");
                break;
            case CS_WEAPON_MP5NAVY:
                performBuyWeapon("3", "1");
                break;
            case CS_WEAPON_UMP45:
                performBuyWeapon("3", "5");
                break;
            case CS_WEAPON_ELITE:
                performBuyWeapon("1", "5");
                break;              // T only
            case CS_WEAPON_MAC10:
                performBuyWeapon("3", "4");
                break;              // T only
            case CS_WEAPON_AUG:
                performBuyWeapon("4", "4");
                break;              // CT Only
            case CS_WEAPON_FIVESEVEN:
                performBuyWeapon("1", "6");
                break;              // CT only
            case CS_WEAPON_M4A1:
                performBuyWeapon("4", "3");
                break;              // CT Only
            case CS_WEAPON_TMP:
                performBuyWeapon("3", "2");
                break;              // CT only
            case CS_WEAPON_HEGRENADE:
                performBuyWeapon("8", "4");
                break;
            case CS_WEAPON_XM1014:
                performBuyWeapon("2", "2");
                break;
            case CS_WEAPON_SMOKEGRENADE:
                performBuyWeapon("8", "5");
                break;
            case CS_WEAPON_USP:
                performBuyWeapon("1", "1");
                break;
            case CS_WEAPON_GLOCK18:
                performBuyWeapon("1", "2");
                break;
            case CS_WEAPON_M249:
                performBuyWeapon("5", "1");
                break;
            case CS_WEAPON_M3:
                performBuyWeapon("2", "1");
                break;

            case CS_WEAPON_G3SG1:
                performBuyWeapon("4", "7");
                break;
            case CS_WEAPON_FLASHBANG:
                performBuyWeapon("8", "3");
                break;
            case CS_WEAPON_P90:
                performBuyWeapon("3", "3");
                break;

                // Armor
            case CS_WEAPON_ARMOR_LIGHT:
                performBuyWeapon("8", "1");
                break;
            case CS_WEAPON_ARMOR_HEAVY:
                performBuyWeapon("8", "2");
                break;

            case CS_DEFUSEKIT:
                performBuyWeapon("8", "6");
                break;
        }
    }

    // CS 1.6 only
    if (counterstrike == 1) { // FRASHMAN 30/08/04: redone switch block, it was full of errors
        switch (weaponIdToBuy) {
            //Pistols
            case CS_WEAPON_GLOCK18:
                performBuyWeapon("1", "1");
                break;
            case CS_WEAPON_USP:
                performBuyWeapon("1", "2");
                break;
            case CS_WEAPON_P228:
                performBuyWeapon("1", "3");
                break;
            case CS_WEAPON_DEAGLE:
                performBuyWeapon("1", "4");
                break;
            case CS_WEAPON_ELITE:
                performBuyWeapon("1", "5");
                break;
                //ShotGUNS
            case CS_WEAPON_M3:
                performBuyWeapon("2", "1");
                break;
            case CS_WEAPON_XM1014:
                performBuyWeapon("2", "2");
                break;
                //SMG
            case CS_WEAPON_MAC10:
                performBuyWeapon("3", "1");
                break;
            case CS_WEAPON_TMP:
                performBuyWeapon("3", "1");
                break;
            case CS_WEAPON_MP5NAVY:
                performBuyWeapon("3", "2");
                break;
            case CS_WEAPON_UMP45:
                performBuyWeapon("3", "3");
                break;
            case CS_WEAPON_P90:
                performBuyWeapon("3", "4");
                break;
                //rifles
            case CS_WEAPON_GALIL:
                performBuyWeapon("4", "1");
                break;
            case CS_WEAPON_FAMAS:
                performBuyWeapon("4", "1");
                break;
            case CS_WEAPON_AK47:
                performBuyWeapon("4", "2");
                break;
            case CS_WEAPON_M4A1:
                performBuyWeapon("4", "3");
                break;
            case CS_WEAPON_SG552:
                performBuyWeapon("4", "4");
                break;
            case CS_WEAPON_AUG:
                performBuyWeapon("4", "4");
                break;
            case CS_WEAPON_SG550:
                performBuyWeapon("4", "5");
                break;
            case CS_WEAPON_G3SG1:
                performBuyWeapon("4", "6");
                break;
                //machinegun
            case CS_WEAPON_M249:
                performBuyWeapon("5", "1");
                break;
                // equipment
            case CS_WEAPON_ARMOR_LIGHT:
                performBuyWeapon("8", "1");
                break;
            case CS_WEAPON_ARMOR_HEAVY:
                performBuyWeapon("8", "2");
                break;
            case CS_WEAPON_FLASHBANG:
                performBuyWeapon("8", "3");
                break;
            case CS_WEAPON_HEGRENADE:
                performBuyWeapon("8", "4");
                break;
            case CS_WEAPON_SMOKEGRENADE:
                performBuyWeapon("8", "5");
                break;
            case CS_WEAPON_SHIELD:
                performBuyWeapon("8", "8");
                break;

            case CS_DEFUSEKIT:
                performBuyWeapon("8", "6");
                break;
        }

        // This differs per team
        // FRASHMAN 30/08/04: all into one ifthen block
        if (iTeam == 2)  // counter
        {
            switch (weaponIdToBuy) {
                case CS_WEAPON_SCOUT:
                    performBuyWeapon("4", "2");
                    break;
                case CS_WEAPON_AWP:
                    performBuyWeapon("4", "6");
                    break;
                    //whats about nightvision? BuyWeapon (pBot, "8", "7")
            }
        } else                 // terror
        {
            switch (weaponIdToBuy) {
                case CS_WEAPON_SCOUT:
                    performBuyWeapon("4", "3");
                    break;
                case CS_WEAPON_AWP:
                    performBuyWeapon("4", "5");
                    break;
                    //whats about nightvision? BuyWeapon (pBot, "8", "6")
            }
        }
    }                         // end of cs 1.6 part
}                            // We actually gonna buy this weapon

// BOT: Memory()
// In this function the bot will receive data; this can be any kind of data.
// For hearing, the bot will check for sounds it should pay attention to and
// store this into its 'hearing vector'. The hearing vector will be used only
// when walking and not when fighting an enemy. Do note that this hearing vector
// is only filled when it is important enough, so all the decisions are made here.
void cBot::Memory() {

    // Skip method when it is too soon.
    if (fMemoryTime > gpGlobals->time) {
        return;
    }

    // Hear players: (loop through all players, determine if they are running and if
    // we can hear them (estimated distance)).
    if (pEnemyEdict == NULL) {
        Vector vHear = Vector(9999, 9999, 9999);
        edict_t *pHearPlayer = NULL;

        //f_walk_time = gpGlobals->time + 1;

        for (int i = 1; i <= gpGlobals->maxClients; i++) {
            edict_t *pPlayer = INDEXENT(i);

            // skip invalid players and skip self (i.e. this bot)
            if ((pPlayer) && (!pPlayer->free) && (pPlayer != pEdict)) {
                // skip this player if not alive (i.e. dead or dying)
                if (!IsAlive(pPlayer))
                    continue;

                // check if we can 'see it on radar' (skip teammates)
                if (UTIL_GetTeam(pPlayer) == UTIL_GetTeam(pEdict))
                    continue;

                // check if its running
                if (FUNC_PlayerRuns(FUNC_PlayerSpeed(pPlayer))) {
                    // check distance
                    float fDistance =
                            (pPlayer->v.origin - pEdict->v.origin).Length();

                    // estimated distance we can hear somebody
                    if (fDistance < BOT_HEARDISTANCE) {
                        // check if this 'hearing' vector is closer then our previous one
                        if (vHear != Vector(9999, 9999, 9999)) {
                            if (func_distance
                                        (pEdict->v.origin,
                                         pPlayer->v.origin) <
                                func_distance(pEdict->v.origin, vHear)) {
                                // this one is closer, thus more important
                                vHear = pPlayer->v.origin;
                                pHearPlayer = pPlayer;
                            }
                        } else {
                            vHear = pPlayer->v.origin;
                            pHearPlayer = pPlayer;
                        }
                    }
                }


                if ((pPlayer->v.button & IN_ATTACK)
                    && (FUNC_EdictHoldsWeapon(pEdict) != CS_WEAPON_HEGRENADE
                        && FUNC_EdictHoldsWeapon(pEdict) != CS_WEAPON_FLASHBANG
                        && FUNC_EdictHoldsWeapon(pEdict) !=
                           CS_WEAPON_SMOKEGRENADE)) {
                    // check distance
                    float fDistance =
                            (pPlayer->v.origin - pEdict->v.origin).Length();

                    // estimated distance we can hear somebody
                    if (fDistance < BOT_HEARFIREDISTANCE) {
                        // check if this 'hearing' vector is closer then our previous one
                        if (vHear != Vector(9999, 9999, 9999)) {
                            if (func_distance
                                        (pEdict->v.origin,
                                         pPlayer->v.origin) <
                                func_distance(pEdict->v.origin, vHear)) {
                                // this one is closer, thus more important
                                vHear = pPlayer->v.origin;
                                pHearPlayer = pPlayer;
                            }
                        } else {
                            vHear = pPlayer->v.origin;
                            pHearPlayer = pPlayer;
                        }
                    }
                }
                // zooming of a sniper rifle
                if (pPlayer->v.button & IN_ATTACK2) {
                    // check distance
                    float fDistance =
                            (pPlayer->v.origin - pEdict->v.origin).Length();

                    // estimated distance we can hear somebody
                    if (fDistance < BOT_HEARDISTANCE) {
                        // check if this 'hearing' vector is closer then our previous one
                        if (vHear != Vector(9999, 9999, 9999)) {
                            if (func_distance
                                        (pEdict->v.origin,
                                         pPlayer->v.origin) <
                                func_distance(pEdict->v.origin, vHear)) {
                                // this one is closer, thus more important
                                vHear = pPlayer->v.origin;
                                pHearPlayer = pPlayer;
                            }
                        } else {
                            vHear = pPlayer->v.origin;
                            pHearPlayer = pPlayer;
                        }
                    }
                }

            }
        }

        // Fill in hearing vectory if any:
        if (pHearPlayer != NULL) {
            if (RANDOM_LONG(0, 100) < (ipFearRate + 10)) {

                // determine fuzzyness by distance:
                int iFuzz =
                        (int) (func_distance(pEdict->v.origin, vHear) /
                               BOT_HEARDISTANCE) * 250;

                // skill depended
                iFuzz /= (bot_skill + 1);

                // create 'estimated hear vector'
                vHear =
                        vHear + Vector(RANDOM_LONG(-iFuzz, iFuzz),
                                       RANDOM_LONG(-iFuzz, iFuzz),
                                       RANDOM_LONG(-iFuzz, iFuzz));

                TraceResult tr;

                UTIL_TraceHull(pEdict->v.origin, vHear, dont_ignore_monsters,
                               point_hull, pEdict, &tr);

                int iNodeHearPlayer =
                        NodeMachine.getClosestNode(vHear, NODE_ZONE * 2, pHearPlayer);

                // if nothing hit:
                if (tr.flFraction >= 1.0) {
                    // we can look at this spot
                    vEar = vHear;
                }
                // we go to the destination

                float fTime = 5 + (ipFearRate / 7);

                if (RANDOM_LONG(0, 100) < ipFearRate
                    && f_walk_time + 5 < gpGlobals->time) // last 5 seconds did not walk
                    f_walk_time = gpGlobals->time + fTime;

                if (RANDOM_LONG(0, 100) < ipCampRate
                    && f_camp_time + 30 < gpGlobals->time // last 30 seconds did not camp
                    ) {
                    f_camp_time = gpGlobals->time + fTime;
                }

            } else {
                fMemoryTime = gpGlobals->time + 5;
            }

            /*


               int iNodeHearPlayer = NodeMachine.getCloseNode (vHear, NODE_ZONE*2, pHearPlayer);
               int iNodeFrom = NodeMachine.getCloseNode (pEdict->v.origin, NODE_ZONE*2, pEdict);
               int iHearToNode = NodeMachine.node_look_at_hear(iNodeHearPlayer, iNodeFrom, pEdict);

               // look at hearto node
               if (iHearToNode > -1)
               {
               vHead = NodeMachine.node_vector(iHearToNode);
               SERVER_PRINT("found smart look at node\n");
               }

               // only check for new goal when the current goal is way of distance and such
               if (ipCampRate > 30 && f_camp_time + 5 < gpGlobals->time)
               f_camp_time = gpGlobals->time + 2.5;
             */

            if (f_update_weapon_time + 2 < gpGlobals->time) {
                PickBestWeapon();
            }
        } else {
            vEar = Vector(9999, 9999, 9999);
//
//            // check for any 'beeps' of the bomb!
//            if (isCounterTerrorist() && Game.bBombPlanted) {
//                // find the bomb vector
//                edict_t *pent = NULL;
//                Vector vC4 = Vector(9999, 9999, 9999);
//                while ((pent = UTIL_FindEntityByClassname(pent, "grenade")) != NULL) {
//                    if (UTIL_GetGrenadeType(pent) == 4)      // It is a C4
//                    {
//                        vC4 = pent->v.origin; // store origin
//                        break;        // done our part now
//                    }
//                }                   // --- find the c4
//
//                if (vC4 != Vector(9999, 9999, 9999)) {
//
//                    if (func_distance(vC4, NodeMachine.node_vector(iGoalNode)) > 100 &&
//                        func_distance(pEdict->v.origin, vC4) < 1024) {
//                        // set new goal node
//                        setGoalNode(NodeMachine.getCloseNode(vC4, NODE_ZONE, NULL));
//                        forgetPath();
//                    }
//                }
//            }
        }

    } else {
        vEar = Vector(9999, 9999, 9999);
    }
}


// BOT: Do i carry weapon # now?
bool cBot::CarryWeapon(int iType) {
    if (current_weapon.iId == iType)
        return true;
    return false;
}

// BOT: Do i carry weapon TYPE # now?
int cBot::CarryWeaponType() {
    int kind = PRIMARY;
    int weapon_id = current_weapon.iId;

    // Check 1. Is it a knife?
    if (weapon_id == CS_WEAPON_KNIFE)
        kind = KNIFE;

    // Check 2, is it a 'tool'?
    if (weapon_id == CS_WEAPON_FLASHBANG || weapon_id == CS_WEAPON_HEGRENADE
        || weapon_id == CS_WEAPON_SMOKEGRENADE)
        kind = GRENADE;

    // Check 3, is it a secondary gun?
    if (weapon_id == CS_WEAPON_P228 || weapon_id == CS_WEAPON_ELITE
        || weapon_id == CS_WEAPON_UMP45 || weapon_id == CS_WEAPON_USP
        || weapon_id == CS_WEAPON_GLOCK18 || weapon_id == CS_WEAPON_DEAGLE
        || weapon_id == CS_WEAPON_FIVESEVEN)
        kind = SECONDARY;

    // Check 4, is it a sniper gun?
    if (weapon_id == CS_WEAPON_SCOUT || weapon_id == CS_WEAPON_SG550
        || weapon_id == CS_WEAPON_AWP || weapon_id == CS_WEAPON_G3SG1)
        kind = SNIPER;

    if (hasShield()) {
        kind = SHIELD;
    }
    //if (weapon_id < 1)
    //    kind = NONE;
    return kind;
}

// BOT: Think about objectives
//
// This function only takes action when the bot is close a goal. The function
// NodeMachine.path_think() handles WHERE the bot goes. Not WHAT to do at a goal.
void cBot::ThinkAboutGoals() {
    //REALBOT_PRINT(this, "thinkAboutGoals()", "start");
    // Depending on bot team we handle goals differently:
    // TERRORISTS
    if (isTerrorist()) {
        // Plant the bomb when the HUD says we can -- BERKED
        if (bHUD_C4_plantable)
            f_c4_time = gpGlobals->time + 1;       // plant bomb

        // A dropped C4 is not a 'goal' (ie. it won't let you win the game
        // when you pick up the bomb. Therefor the 'pickup the dropped bomb
        // code is in cNodeMachine::path_walk().
    } else if (isCounterTerrorist()) {
        // COUNTER-TERRORISTS
        if (vip) {
            // VIP
        } else {
            if (Game.bBombPlanted) {
                if (isCounterTerrorist()) {
                    // defuse (or set timers for it)
                    Defuse();
                }
            } else {
                if (Game.bHostageRescueMap) {
                    TryToGetHostageTargetToFollowMe(this);
                    checkIfHostagesAreRescued();
                    checkOfHostagesStillFollowMe();
                }
            }
        }
    }
    // in Act() we find the 'acting' code when timers above are set.
}

void cBot::rememberWhichHostageToRescue(edict_t *pHostage) {
    this->pBotHostage = pHostage;
}

edict_t * cBot::getHostageToRescue() {
    return pBotHostage;
}

edict_t * cBot::findHostageToRescue() {
    edict_t *pent = NULL;

    // Search for all hostages in the game
    while ((pent = UTIL_FindEntityByClassname(pent, "hostage_entity")) != NULL) {
        if (!isHostageRescueable(this, pent)) continue;
        if (!canSeeEntity(pent)) continue;
        // skip too far hostages, leave it up to the goal picking to get closer
        if (getDistanceTo(pent->v.origin) > (NODE_ZONE * 2.5)) continue;

        char msg[255];
        sprintf(msg, "Found hostage to rescue at %f,%f,%f", pent->v.origin.x, pent->v.origin.y, pent->v.origin.z);
        this->rprint_trace("findHostageToRescue()", msg);
        return pent;
    }

    return NULL;
}

bool cBot::isDefusing() {
    return f_defuse > gpGlobals->time;
}

bool cBot::hasTimeToMoveToNode() {
    return fMoveToNodeTime > -1 && getMoveToNodeTimeRemaining() > 0;
}
/**
This function will set the iCloseNode variable, which is the node most closest to
the bot. Returns the closest node it found.
**/
int cBot::determineCurrentNode() {
    iCloseNode = determineCurrentNode(NODE_ZONE);
    return iCloseNode;
}

/**
This function will set the iCloseNode variable, which is the node most closest to
the bot. Returns the closest node it found.
**/
int cBot::determineCurrentNodeWithTwoAttempts() {
    iCloseNode = determineCurrentNode();
    if (iCloseNode < 0) {
        iCloseNode = determineCurrentNode(NODE_ZONE * 2);
    }
    return iCloseNode;
}

/**
Find node close to bot, given range. Does not cache result.
**/
int cBot::determineCurrentNode(float range) {
    return NodeMachine.getClosestNode(pEdict->v.origin, range, pEdict);
}

/**
 * This returns the current node (iCloseNode) set. Instead of using determineCurrentNode, which is expensive,
 * call this to return the cached value. It will however call determineCurrentNode when node is < 0, usually it means
 * the state has been set.
 * @return
 */
int cBot::getCurrentNode() {
    if (iCloseNode < 0) {
        determineCurrentNode();
    }
    return iCloseNode;
}

/**
 * Aka, the node we are heading for.
 */
int cBot::getCurrentPathNodeToHeadFor() {
    return NodeMachine.getNodeIndexFromBotForPath(iBotIndex, pathIndex);
}

/**
 * Aka, the node we were coming from. In case the index is < 0 (ie, there is no previous node yet), this will
 * return -1;
 */
int cBot::getPreviousPathNodeToHeadFor() {
    return NodeMachine.getNodeIndexFromBotForPath(iBotIndex, getPreviousPathIndex());
}

bool cBot::isHeadingForGoalNode() {
    return getCurrentPathNodeToHeadFor() == getGoalNode();
}

/**
 * Aka, the next node after we have arrived at the current path node.
 */
int cBot::getNextPathNode() {
    return NodeMachine.getNodeIndexFromBotForPath(iBotIndex, pathIndex + 1);
}

// Is this bot dead?
bool cBot::isDead() {
    return (pEdict->v.health < 1) || (pEdict->v.deadflag != DEAD_NO);
}

// BOT: Think
void cBot::Think() {
    if (mod_id != CSTRIKE_DLL) return; // do not support non-counter-strike mods

    // BOT: If a bot did not join a team yet, then do it
    if (!hasJoinedTeam) {
        rprint("Need to join team, doing that now");
        JoinTeam();
        return;
    }

    // Set closest node
    determineCurrentNode();

    // BOT: If a bot is dead, re-initialize
    if (isDead()) {
        if (!bInitialize) return; // do nothing when no need to initialize
        rprint("Dead, need to re-initialize");

        // AUTOSKILL
        cBot *botPointerOfKiller = UTIL_GetBotPointer(killer_edict);

        // not killed by a fellow bot, presumably a human player
        if (botPointerOfKiller == NULL) {
            if (autoskill) {
                bot_skill--;
                if (bot_skill < 0)
                    bot_skill = 0;
            }

            if (Game.iKillsBroadcasting != BROADCAST_KILLS_NONE
                && killer_edict != NULL) {
                // This is a human, we will tell this human he has been killed
                // by a bot.
                int r = RANDOM_LONG(150, 255);
                int g = RANDOM_LONG(30, 155);
                int b = RANDOM_LONG(30, 155);
                char msg[128];
                if (Game.iDeathsBroadcasting == BROADCAST_DEATHS_FULL)
                    sprintf(msg,
                            "You have killed a RealBot!\n\nName:%s\nSkill:%d\n",
                            name, bot_skill);
                else
                    sprintf(msg, "You have killed a RealBot named %s!",
                            name);

                HUD_DrawString(r, g, b, msg, killer_edict);
            }
        }

        if (iCloseNode > -1 && !end_round) {
            iDiedNode = iCloseNode;
            NodeMachine.danger(iCloseNode, UTIL_GetTeam(pEdict));
        }

        if (console_nr == 0) {
            rprint("NewRound - because console_nr ?!");
            NewRound();
            bInitialize = false;
        }

        BotConsole(this);

        // dead messages
        if (console_nr == 0) {
            rprint("console_nr == 0"); //whatever this means
            // do some chatting
            if (RANDOM_LONG(0, 200) < ipChatRate) {
                if (fChatTime + 0.5 < gpGlobals->time)
                    if (chChatSentence[0] == '\0')   // we did not want to say anything
                    {
                        // we should say something now?
                        int iMax = -1;

                        for (int tc = 0; tc < 50; tc++) {
                            if (ChatEngine.ReplyBlock[99].sentence[tc][0] != '\0') iMax++;
                        }

                        int the_c = RANDOM_LONG(0, iMax);

                        if (the_c > -1 && iMax > -1) {
                            char chSentence[80];
                            memset(chSentence, 0, sizeof(chSentence));
                            sprintf(chSentence, "%s ",
                                    ChatEngine.ReplyBlock[99].sentence[the_c]);
                            //strcpy(chSentence, ChatEngine.ReplyBlock[99].sentence[the_c]);
                            PrepareChat(chSentence);
                        }
                    }
            } else {
                // we missed the chatrate chance
                if (fChatTime < gpGlobals->time)    // time
                    if (chChatSentence[0] == '\0')   // we did not want to say anything
                        if (RANDOM_LONG(0, 100) < ipChatRate) // rate
                            fChatTime = gpGlobals->time +
                                        RANDOM_FLOAT(0.0, ((Game.iProducedSentences + 1) / 2));      // wait

            }

            return;
        }
    } // isDead();

    // set this for the next time the bot dies so it will initialize stuff
    if (!bInitialize) {
        bInitialize = true;
    }

    if (end_round) {
        rprint("End round");
        MDLL_ClientKill(pEdict);
        pEdict->v.frags += 1;
        return;
    }

    // BOT: Played enough rounds
    if (played_rounds > play_rounds && internet_play) {
        rprint("Played enough rounds");
        bIsUsed = FALSE;          // no longer used
        char cmd[80];
        sprintf(cmd, "kick \"%s\"\n", name);
        SERVER_COMMAND(cmd);      // kick the bot using (kick "name")
        return;
    }

    // Move speed... moved_distance.
    if (distanceMovedTimer <= gpGlobals->time) {
        // see how far bot has moved since the previous position...
        Vector v_diff = prevOrigin - pEdict->v.origin;
        // make distanceMoved an average of this moment and the previous one.
        float movedTwoTimes = distanceMoved + v_diff.Length();

        // prevent division by zero
        if (movedTwoTimes > 0.0f) {
            distanceMoved = movedTwoTimes / 2;
        } else {
            distanceMoved = 0;
        }

        // save current position as previous
        prevOrigin = pEdict->v.origin;
        distanceMovedTimer = gpGlobals->time + 0.1;
    }

    // NEW ROUND
    if (Game.NewRound()) {
        rprint_trace("Think", "Game.NewRound");
    }

    // --------------------------------
    // MEMORY STEP
    // --------------------------------
    Memory();

    // --------------------------------
    // IMPORTANT THINKING GOING ON HERE
    // --------------------------------
    int healthChange = prev_health - bot_health;

    // handle damage taken
    if (prev_health > bot_health
        && healthChange > RANDOM_LONG(CSTRIKE_MIN_DAMAGE, CSTRIKE_MAX_DAMAGE)
        && hasEnemy()) {

        // need backup!
        if (FUNC_DoRadio(this)) {
            UTIL_BotRadioMessage(this, 3, "3", "");
        }

        BOT_DecideTakeCover(this);
    }

    prev_health = bot_health;

    // Do your console stuff
    BotConsole(this);

    // BOT: Blinded
    if (isBlindedByFlashbang()) {
        // Dude we are messed up.

        // 01/07/04 - Stefan - Pointed out on the forums by Josh Borke... (do not shoot when dontshoot is on)
        // shoot randomly
        if (!Game.bDoNotShoot) {
            if ((RANDOM_LONG(0, 100) < ipFearRate) && RANDOM_LONG(0, 100)) {
                UTIL_BotPressKey(this, IN_ATTACK);
            }
        }

        rprint_trace("Think()", "Blinded");
        return;
    }


    // NEW: When round time is over and still busy playing, kill bots
    float roundTimeInSeconds = CVAR_GET_FLOAT("mp_roundtime") * 60;
    float freezeTimeCVAR = CVAR_GET_FLOAT("mp_freezetime");
    if (Game.getRoundStartedTime() + 10.0 + roundTimeInSeconds + freezeTimeCVAR < gpGlobals->time) {
        end_round = true;
        // round is ended
    }

    // FREEZETIME:
    if (Game.getRoundStartedTime() > gpGlobals->time && freezeTime < gpGlobals->time) {
        freezeTime = gpGlobals->time + RANDOM_FLOAT(0.1, 2.0);
    }

    // 1 SECOND START OF ROUND
    if (Game.getRoundStartedTime() + 1 > gpGlobals->time &&
        Game.getRoundStartedTime() < gpGlobals->time) {
        // TODO: Issue radio command?
        this->rprint_trace("cBot::Think()", "First second of round");
    }

    // SITUATION: In freezetime
    if (isFreezeTime()) {
        stopMoving();
        lastSeenEnemyVector = Vector(0, 0, 0);
        setTimeToMoveToNode(2);
        vHead = vBody = pEdict->v.origin;

        // find any spawnpoint to look at:
        edict_t *pent = NULL;

        if (isCounterTerrorist()) {
            while ((pent = UTIL_FindEntityByClassname(pent, "info_player_start")) != NULL) {
                if (func_distance(pent->v.origin, pEdict->v.origin) < 200 &&
                    func_distance(pent->v.origin, pEdict->v.origin) > 50) {
                    break;
                }
            }
        } else {
            while ((pent = UTIL_FindEntityByClassname(pent, "info_player_deathmatch")) != NULL) {
                if (func_distance(pent->v.origin, pEdict->v.origin) < 200 &&
                    func_distance(pent->v.origin, pEdict->v.origin) > 50) {
                    break;
                }
            }
        }

        // when pent is filled, look at it
        if (pent != NULL) {
            vBody = vHead = pent->v.origin;
        }

        rprint_trace("Think()", "isFreezeTime");
        return;
    }

    // **---**---**---**---**---**---**
    // MAIN STATE: We have no enemy...
    // **---**---**---**---**---**---**
    if (!hasEnemy()) {

        if (!Game.bDoNotShoot) {
            InteractWithPlayers();
        }

        bool bMayFromGame = true;

        if (Game.fWalkWithKnife > 0)
            if (Game.getRoundStartedTime() + Game.fWalkWithKnife < gpGlobals->time)
                bMayFromGame = false;

        if (Game.fWalkWithKnife == 0)
            bMayFromGame = false;

        if (hasShield()) {
            if (!hasShieldDrawn() && f_allow_keypress < gpGlobals->time) {
                UTIL_BotPressKey(this, IN_ATTACK2);      // draw shield
                f_allow_keypress = gpGlobals->time + 0.7;
            }
        }

        if (CarryWeapon(CS_WEAPON_KNIFE) == false
            && f_camp_time < gpGlobals->time
            && freezeTime < gpGlobals->time
            && f_c4_time < gpGlobals->time
            && f_update_weapon_time < gpGlobals->time && bWalkKnife
            && bMayFromGame) {
            UTIL_SelectItem(pEdict, UTIL_GiveWeaponName(-1));   // -1 is knife
            f_update_weapon_time = gpGlobals->time + 0.7;
        }

        // When holding a grenade (and not switching to another weapon)
        if (CarryWeaponType() == GRENADE
            && f_update_weapon_time < gpGlobals->time) {
            if (iPrimaryWeapon > -1)
                UTIL_SelectItem(pEdict,
                                UTIL_GiveWeaponName(iPrimaryWeapon));

            else                // pick secondary
                UTIL_SelectItem(pEdict,
                                UTIL_GiveWeaponName(iSecondaryWeapon));
            f_update_weapon_time = gpGlobals->time + 0.7;
        }

        // Think about objectives
        ThinkAboutGoals();
    } else {
        // **---**---**---**---**---**---**
        // MAIN STATE: We have an enemy!
        // **---**---**---**---**---**---**

        // Keep interacting with players:
        InteractWithPlayers();

        // And combat enemies
        Combat();
    }

    // WALK()
    NodeMachine.path_think(this, distanceMoved);

    // SITUATION: Passed Freezetime

} // THINK()

bool cBot::isFreezeTime() const {
    return freezeTime > gpGlobals->time;
}

/**
Return true if one of the pointers is not NULL
**/
bool cBot::isEscortingHostages() {
    bool result = getAmountOfHostagesBeingRescued() > 0;
    if (result) {
        rprint("I am escorting hostages!");
    }
    return result;
}

void cBot::checkOfHostagesStillFollowMe() {
    if (fCheckHostageStatusTimer > gpGlobals->time) return;
    fCheckHostageStatusTimer = gpGlobals->time + 5;

////    this->rprint("checkOfHostagesStillFollowMe - START");
//    if (hostage1) {
//        if (!isHostageRescued(this, hostage1) && FUNC_EdictIsAlive(hostage1) && !canSeeEntity(hostage1) && getDistanceTo(hostage1->v.origin) > NODE_ZONE*2.5) {
//            rprint_trace("checkOfHostagesStillFollowMe", "lost track of hostage1");
//            forgetHostage(hostage1);
//        }
//    }
//    if (hostage2) {
//        if (!isHostageRescued(this, hostage2) && FUNC_EdictIsAlive(hostage2) && !canSeeEntity(hostage2) && getDistanceTo(hostage2->v.origin) > NODE_ZONE*2.5) {
//            rprint_trace("checkOfHostagesStillFollowMe", "lost track of hostage2");
//            forgetHostage(hostage2);
//        }
//    }
//    if (hostage3) {
//        if (!isHostageRescued(this, hostage3) && FUNC_EdictIsAlive(hostage3) && !canSeeEntity(hostage3) && getDistanceTo(hostage3->v.origin) > NODE_ZONE*2.5) {
//            rprint_trace("checkOfHostagesStillFollowMe", "lost track of hostage3");
//            forgetHostage(hostage3);
//        }
//    }
//
//    if (hostage4) {
//        if (!isHostageRescued(this, hostage4) && FUNC_EdictIsAlive(hostage4) && !canSeeEntity(hostage4) && getDistanceTo(hostage4->v.origin) > NODE_ZONE*2.5) {
//            rprint_trace("checkOfHostagesStillFollowMe", "lost track of hostage4");
//            forgetHostage(hostage4);
//        }
//    }
//    rprint("checkOfHostagesStillFollowMe - END");
}

void cBot::clearHostages() {
    rprint_trace("clearHostages");
    hostage1 = NULL;
    hostage2 = NULL;
    hostage3 = NULL;
    hostage4 = NULL;
    pBotHostage = NULL;
}

// BOT: CheckGear, part of UpdateStatus()
void cBot::CheckGear() {

    // PRIMARY
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_mp5navy"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_mp5navy");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_ak47"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_ak47");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_m3"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_m3");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_aug"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_aug");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_sg552"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_sg552");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_m249"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_m249");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_xm1014"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_xm1014");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_p90"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_p90");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_tmp"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_tmp");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_m4a1"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_m4a1");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_awp"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_awp");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_sg550"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_sg550");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_scout"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_scout");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_mac10"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_mac10");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_g3sg1"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_g3sg1");

    // Counter-Strike 1.6 weapon FAMAS/GALIL
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_famas"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_famas");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_galil"))) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_galil");

    // SECONDARY
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_ump45"))) iSecondaryWeapon = UTIL_GiveWeaponId("weapon_ump45");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_elite"))) iSecondaryWeapon = UTIL_GiveWeaponId("weapon_elite");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_fiveseven"))) iSecondaryWeapon = UTIL_GiveWeaponId("weapon_fiveseven");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_p228"))) iSecondaryWeapon = UTIL_GiveWeaponId("weapon_p228");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_deagle"))) iSecondaryWeapon = UTIL_GiveWeaponId("weapon_deagle");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_usp"))) iSecondaryWeapon = UTIL_GiveWeaponId("weapon_usp");
    if (isOwningWeapon(UTIL_GiveWeaponId("weapon_glock18"))) iSecondaryWeapon = UTIL_GiveWeaponId("weapon_glock18");

    // Handle shields as primary weapon
    if (hasShield()) iPrimaryWeapon = UTIL_GiveWeaponId("weapon_shield");
}

// BOT: Update personal status
void cBot::UpdateStatus() {
    // name filled in yet?
    if (name[0] == 0)
        strcpy(name, STRING(pEdict->v.netname));

    // Set thirdpartybot flag
    pEdict->v.flags |= FL_THIRDPARTYBOT;

    // Reset stuff
    pEdict->v.button = 0;
    setMoveSpeed(f_max_speed); // by default run

    // When its not time to strafe, don't.
    if (f_strafe_time < gpGlobals->time) {
        if (f_strafe_speed != 0.0f) {
            rprint_trace("UpdateStatus", "Strafe speed set to 0!");
            f_strafe_speed = 0.0f;
        }
    }

    // Update team state when started
    if (hasJoinedTeam) {
        iTeam = UTIL_GetTeam(pEdict) + 1; // 1 - TERRORIST, 2 - COUNTER-TERRORIST
    }

    // Check if we became VIP
    vip = UTIL_IsVip(pEdict);

    // Check gear
    CheckGear();

    // Set max speed and such when CS 1.6
    if (counterstrike == 1) {
        f_max_speed = pEdict->v.maxspeed;
//        char msg[255];
//        sprintf(msg, "f_max_speed set to %f", f_max_speed);
//        rprint_trace("UpdateStatus", msg);
        bot_health = (int) pEdict->v.health;
        bot_armor = (int) pEdict->v.armorvalue;
    }
}

// ---------------------------------- BOT CLASS FUNCTIONS
// ---------------------------------- BOT CLASS FUNCTIONS
// ---------------------------------- BOT CLASS FUNCTIONS

////////////////////////////////////////////////////////////////////////////////
/// Radio Action - Response
////////////////////////////////////////////////////////////////////////////////
bool BotRadioAction() {
    char name[64];
    bool unstood = false;
    edict_t *plr = NULL;
    int team = -1;
    int i;
    int radios = 0;              // Hold amount of replies here, so we don't flood :)
    strcpy(name, radio_messenger);

    // First find the team messager name
    for (i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t *pPlayer = INDEXENT(i);   // Get pEdict
        char netname[64];
        if (pPlayer)              // If player exists
        {
            strcpy(netname, STRING(pPlayer->v.netname));   // Copy netname
            if (strcmp(netname, name) == 0)        // If
            {
                plr = pPlayer;
                team = UTIL_GetTeam(pPlayer);
            }
        }
    }

    // Check players and check if radio message applies to them
    for (i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t *pPlayer = INDEXENT(i);
        char netname[64];
        if (pPlayer) {

            strcpy(netname, STRING(pPlayer->v.netname));

            if ((strcmp(netname, name) != 0) &&    // When not the same name
                (team == UTIL_GetTeam(pPlayer)) && // .. the same team...
                (pPlayer->v.deadflag == DEAD_NO) &&        // .. not dead ..
                ((UTIL_GetBotPointer(pPlayer) != NULL)))   // and a RealBot
            {
                // here are all bots
                cBot *BotPointer = UTIL_GetBotPointer(pPlayer);

                if (BotPointer == NULL)
                    continue;        // somehow this fucked up, bail out

                bool want_to_answer = false;        // want to answer radio call
                bool report_back = false;   // for reporting in
                float distance = func_distance(plr->v.origin,
                                               BotPointer->pEdict->v.origin);        // distance between the 2

                // Same team, randomly, do we even listen to the radio?
                // the more further away, the more chance it will not listen
                bool bWantToListen = false;

                // Reply on distance check
                if (RANDOM_LONG(0, 8192) > distance)
                    bWantToListen = true;

                // Hearrate (personality setting)
                if (RANDOM_LONG(0, 100) < BotPointer->ipHearRate &&
                    bWantToListen)
                    want_to_answer = true;

                bool can_do_negative = true;        // On some radio commands we can't say negative, thats stupid

                // If we want to listen to the radio... then handle it!
                if (bWantToListen) {

                    // Report in team!
                    if (strstr(message, "#Report_in_team") != NULL) {
                        // gives human knowledge who is on his team
                    }

                    // Regroup team!
                    if (strstr(message, "#Regroup_team") != NULL) {
                        // regroup now!
                        unstood = true;

                        // get to the leader position
                        BotPointer->rprint("Setting goal from regroup team");
                        BotPointer->setGoalNode(NodeMachine.getClosestNode(plr->v.origin, NODE_ZONE * 2, plr));
                        BotPointer->forgetPath();
                    }

                    // Hold this position
                    if (strstr(message, "#Hold_this_position") != NULL ||
                        strstr(message, "#Get_in_position_and_wait") != NULL) {
                        // do nothing
                    }
                    // Follow me!!
                    if (strstr(message, "#Follow_me") != NULL) {}

                    // You take the point!
                    // 23/06/04 - Stefan - Here the leader should break up his position?
                    // ie, the leader will be assigned to the bot this human/bot is facing?
                    if (strstr(message, "#You_take_the_point") != NULL) {
                        can_do_negative = false;
                    }
                    // Enemy Sotted!
                    if (strstr(message, "#Enemy_spotted") != NULL) {
                        can_do_negative = false;

                        // Find player who issues this message and go to it
                        int iBackupNode =
                                NodeMachine.getClosestNode(plr->v.origin, NODE_ZONE, plr);

                        // Help this player
                        if (iBackupNode > -1) {

                            unstood = true;

                            BotPointer->rprint("Setting goal for backup");
                            BotPointer->setGoalNode(iBackupNode);
                            BotPointer->forgetPath();
                            BotPointer->f_camp_time = gpGlobals->time - 1;
                            BotPointer->f_walk_time = gpGlobals->time;
                        }
                    }
                    // Enemy Down!
                    if (strstr(message, "#Enemy_down") != NULL) {
                        BotPointer->rprint_trace("BotRadioAction", "Understood Enemy down - no logic");

                        unstood = true;
                        can_do_negative = false;
                    }
                    // Stick together team!
                    if (strstr(message, "#Stick_together_team") != NULL) {
                        BotPointer->rprint_trace("BotRadioAction", "Understood Stick together team - no logic");
                        unstood = true;
                        // TODO: Find someone to follow. (to stick with)
                    }
                    // cover me|| strstr (message, "#Cover_me") != NULL

                    // Need backup / taking fire...
                    if (strstr(message, "#Need_backup") != NULL || strstr(message, "#Taking_fire") != NULL) {
                        BotPointer->rprint_trace("BotRadioAction", "Understood Need backup or Taking fire");

                        unstood = true;

                        // get source of backup
                        int iBackupNode = NodeMachine.getClosestNode(plr->v.origin, NODE_ZONE, plr);

                        if (iBackupNode > -1) {
                            BotPointer->rprint_trace("BotRadioAction", "Found node nearby player who requested backup/reported taking fire.");
                            BotPointer->setGoalNode(iBackupNode);
                            BotPointer->forgetPath();
                            BotPointer->f_camp_time = gpGlobals->time - 1;
                            BotPointer->f_walk_time = gpGlobals->time;
                        }
                    }

                    // Taking fire!
                    if (strstr(message, "#Taking_fire") != NULL) {
                        // todo todo todo backup our friend
                        // unstood = true;
                    }
                    // Team fall back!
                    if (strstr(message, "#Team_fall_back") != NULL) {

                    }
                    // Go GO Go, stop camping, stop following, get the heck out of there!
                    if (strstr(message, "#Go_go_go") != NULL) {
                        BotPointer->rprint_trace("BotRadioAction", "Understood Go Go Go");
                        unstood = true;
                        BotPointer->f_camp_time = gpGlobals->time - 30;
                        BotPointer->f_walk_time = gpGlobals->time;
                        BotPointer->f_cover_time = gpGlobals->time - 10;
                        BotPointer->f_hold_duck = gpGlobals->time - 10;
                        BotPointer->f_jump_time = gpGlobals->time - 10;
                        BotPointer->forgetPath();
                        BotPointer->forgetGoal();
                    }

                    if ((FUNC_DoRadio(BotPointer)) && (unstood)) {
                        int maxAllowedRadios = gpGlobals->maxClients / 4;
                        if (BotPointer->console_nr == 0 && radios < maxAllowedRadios) {

                            if (!report_back) {
                                UTIL_BotRadioMessage(BotPointer, 3, "1", "");   // Roger that!
                            } else {
                                UTIL_BotRadioMessage(BotPointer, 3, "6", "");   // Reporting in!
                            }

                            BotPointer->f_console_timer = gpGlobals->time + RANDOM_FLOAT(0.8, 2.0);
                            radios++;
                        }
                    }
                }                   // they even listen to the radio command?
                else {
                    /*
                                // filter out the commands where we cannot reply with negative
                                // You take the point!
                                if (strstr (message, "#You_take_the_point") != NULL)
                                    can_do_negative = false;

                                // Enemy Sotted!
                                if (strstr (message, "#Enemy_spotted") != NULL)
                                    can_do_negative = false;

                                // Enemy Down!
                                if (strstr (message, "#Enemy_down") != NULL)
                                    can_do_negative = false;

                                if ((FUNC_DoRadio(BotPointer))
                                    && (unstood) && (can_do_negative))

                                {
                                    if (BotPointer->console_nr == 0
                                        && radios < (gpGlobals->maxClients / 4))

                                    {
                                        if (report_back == false)

                                        {
                                            UTIL_BotRadioMessage (BotPointer, 3, "8", "");	// Negative!
                                            BotPointer->f_console_timer =	gpGlobals->time + RANDOM_FLOAT (0.8, 2.0);
                                            radios++;
                                        }
                                    }
                                }
                                */
                }
            }                      // End check!
        }                         // If (Player)
    }                            // FOR Clients
    return true;
}

// Is entity visible? (from Entity view)
bool EntityIsVisible(edict_t *pEntity, Vector dest) {

    //DebugOut("bot: EntityIsVisible()\n");
    TraceResult tr;

    // trace a line from bot's eyes to destination...
    UTIL_TraceLine(pEntity->v.origin + pEntity->v.view_ofs, dest,
                   dont_ignore_monsters, pEntity->v.pContainingEntity, &tr);

    // check if line of sight to object is not blocked (i.e. visible)
    if (tr.flFraction >= 1.0)
        return true;

    else
        return false;
}

// Can see Edict?
bool cBot::canSeeEntity(edict_t *pEntity) {
    if (pEntity == NULL) return false;

    TraceResult tr;
    Vector start = pEdict->v.origin + pEdict->v.view_ofs;
    Vector vDest = pEntity->v.origin;

    // trace a line from bot's eyes to destination...
    UTIL_TraceLine(start, vDest, ignore_monsters, pEdict->v.pContainingEntity, &tr);

    // it hit anything
    if (tr.flFraction < 1.0) {
        // if it hit the entity we wanted to see, then its ok!
        if (tr.pHit == pEntity) return true;
        return false;
    }

    return true;
}

/**
 * Returns distance from this bot to a given nodeIndex. If the given NodeIndex is invalid, the distance returned is 0.
 * @param nodeIndex
 * @return
 */
float cBot::getDistanceTo(int nodeIndex) {
    tNode *nodePtr = NodeMachine.getNode(nodeIndex);
    if (nodePtr != NULL) {
        return getDistanceTo(nodePtr->origin);
    }
    rprint("getDistanceTo(int nodeIndex)", "The given nodeIndex was invalid, returning 9999 distance");
    return 9999;
}

/**
 * Returns distance from this bot position (pEdict->v.origin) to given Vector.
 * @param vDest
 * @return
 */
float cBot::getDistanceTo(Vector vDest) {
    return func_distance(pEdict->v.origin, vDest);
}

bool cBot::isUsingHostage(edict_t *pHostage) {
    if (pHostage == NULL) return false;

    // checks if the current pEdict is already 'in use'
    // note: time check only when we have an hostage pointer assigned
    if (hostage1 == pHostage) {
        rprint("isUsingHostage", "hostage1");
        return true;
    }

    if (hostage2 == pHostage) {
        rprint("isUsingHostage", "hostage2");
        return true;
    }

    if (hostage3 == pHostage) {
        rprint("isUsingHostage", "hostage3");
        return true;
    }

    if (hostage4 == pHostage) {
        rprint("isUsingHostage", "hostage4");
        return true;
    }

    return false;
}

void cBot::forgetHostage(edict_t *pHostage) {
    // these are the hostages we are rescueing (ie, they are following this bot)
    if (hostage1 == pHostage)  {
        rprint("forgetHostage", "hostage1");
        hostage1 = NULL;
    }
    if (hostage2 == pHostage)  {
        rprint("forgetHostage", "hostage2");
        hostage2 = NULL;
    }
    if (hostage3 == pHostage)  {
        rprint("forgetHostage", "hostage3");
        hostage3 = NULL;
    }
    if (hostage4 == pHostage)  {
        rprint("forgetHostage", "hostage4");
        hostage4 = NULL;
    }

    // this is the hostage we have taken interest in
    if (pBotHostage == pHostage)  {
        rprint("forgetHostage", "pBotHostage");
        pBotHostage = NULL;
    }
}

int cBot::getAmountOfHostagesBeingRescued() {
    int count = 0;

    if (hostage1 != NULL) count++;
    if (hostage2 != NULL) count++;
    if (hostage3 != NULL) count++;
    if (hostage4 != NULL) count++;

    return count;
}

// Will return true when the vector is visible.
// TODO: Make this function more flexible, ie able to hit an entity that it searches
// and return true on that as well.  (mix it with the above function)
bool cBot::canSeeVector(Vector vDest) {
    TraceResult tr;
    Vector start = pEdict->v.origin + pEdict->v.view_ofs;

    // trace a line from bot's eyes to destination...
    UTIL_TraceLine(start, vDest, ignore_monsters, pEdict->v.pContainingEntity, &tr);

    if (tr.flFraction < 1.0)
        return false;

    return true;
}

// The coming 2 shield functions where originaly created by Whistler;
// i got them from the JoeBot source though. But... in the end, thank you
// Whistler!
bool cBot::hasShield() {
    // Adapted from Wei Mingzhi's YAPB
    return (strncmp(STRING(pEdict->v.viewmodel), "models/shield/v_shield_", 23) == 0);
}

bool cBot::hasShieldDrawn() {
    // Adapted from Wei Mingzhi's YAPB
    if (!hasShield())
        return false;

    return (pEdict->v.weaponanim == 6 || pEdict->v.weaponanim == 7);
}

/*
 BotThink() 
 This function is the very general/main/simplified thinking function of the bot.
 Do NOT add/remove/change code here! If you want to give the bot information to
 work with. Put it in UpdateStatus(). When the bot has to think about it, do it
 int Think() and everything else (when all is set, how to 'do' it) in Act().
 */
void BotThink(cBot *pBot) {
    // STEP 1: Update status
    pBot->UpdateStatus();

    // STEP 2: Think
    pBot->Think();

    // STEP 3: Act
    pBot->Act();

    // PASS THROUGH ENGINE

//    float frameInterval = m_lastCommandTime - gpGlobals->time;
    float msecval = (gpGlobals->time - pBot->fLastRunPlayerMoveTime) * 1000.0f;
    pBot->fLastRunPlayerMoveTime = gpGlobals->time;

    double upMove = 0.0;
    char msg[255];
    sprintf(msg, "moveSpeed %f, strafeSpeed %f, msecVal %f", pBot->f_move_speed, pBot->f_strafe_speed, msecval);
    pBot->rprint_trace("BotThink/pfnRunPlayerMove", msg);
    g_engfuncs.pfnRunPlayerMove(pBot->pEdict,
                                pBot->vecMoveAngles,
                                pBot->f_move_speed,
                                pBot->f_strafe_speed,
                                upMove,
                                pBot->pEdict->v.button,
                                0,
                                msecval);

    float fUpdateInterval = 1.0f / 60.0f; // update at 60 fps
    pBot->fUpdateTime = gpGlobals->time + fUpdateInterval;
}

// 17/07/04
// log important variables of the bot (it will be called from dll.cpp once per active bot)
// they are logged into reallog.txt file

void cBot::Dump(void) {
    char buffer[181];
    int iCurrentNode =
            NodeMachine.getClosestNode(pEdict->v.origin, (NODE_ZONE * 2), pEdict);

    _snprintf(buffer, 180,
              "%s (#%d %s): timers, now= %.0f, c4_time=%.0f, camp_time=%.0f, wait_time=%.0f, cover_time=%.0f, wander=%.0f, MoveToNodeTime=%.0f\n",
              name, iBotIndex, (iTeam == 1) ? "T" : "CT", gpGlobals->time,
              f_c4_time, f_camp_time, f_wait_time, f_cover_time, fWanderTime,
              fMoveToNodeTime);
    rblog(buffer);
    _snprintf(buffer, 180, "  GoalNode=%d, CurrentNode=%d, iPathFlags=",
              iGoalNode, iCurrentNode);
    switch (iPathFlags) {
        case PATH_NONE:
            strncat(buffer, "PATH_NONE ", 180);
            break;
        case PATH_DANGER:
            strncat(buffer, "PATH_DANGER ", 180);
            break;
        case PATH_CONTACT:
            strncat(buffer, "PATH_CONTACT ", 180);
            break;
        case PATH_CAMP:
            strncat(buffer, "PATH_CAMP ", 180);
            break;
        default:
            strncat(buffer, "???", 180);
    }
    strncat(buffer, "\n", 180);
    rblog(buffer);
    if (iGoalNode >= 0)
        NodeMachine.dump_path(iBotIndex, pathIndex);
}

/**
 * Will begin walk the path by setting pathNodeIndex to 0, which is a valid nr so that
 * isWalkingPath returns true.
 */
void cBot::beginWalkingPath() {
    this->pathIndex = 0;
}

bool cBot::hasHostageToRescue() {
    return pBotHostage != NULL;
}

bool cBot::canSeeHostageToRescue() {
    return canSeeEntity(pBotHostage);
}

void cBot::clearHostageToRescueTarget() {
    rprint_trace("clearHostageToRescueTarget", "clearing pBotHostage pointer");
    this->pBotHostage = NULL;
}

// Finds a free hostage pointer and assigns it.
void cBot::rememberHostageIsFollowingMe(edict_t *pHostage) {
    if (pHostage == NULL) {
        rprint_trace("rememberHostageIsFollowingMe", "ERROR assigning NULL pHostage pointer!?");
    }
    if (hostage1 == NULL) {
        rprint_trace("rememberHostageIsFollowingMe", "hostage1 slot is free.");
        hostage1 = pHostage;
    } else if (hostage2 == NULL) {
        rprint_trace("rememberHostageIsFollowingMe", "hostage2 slot is free.");
        hostage2 = pHostage;
    } else if (hostage3 == NULL) {
        rprint_trace("rememberHostageIsFollowingMe", "hostage3 slot is free.");
        hostage3 = pHostage;
    } else if (hostage4 == NULL) {
        rprint_trace("rememberHostageIsFollowingMe", "hostage4 slot is free.");
        hostage4 = pHostage;
    }
}

void cBot::checkIfHostagesAreRescued() {
    if (isHostageRescued(this, hostage1))  forgetHostage(hostage1);
    if (isHostageRescued(this, hostage2))  forgetHostage(hostage2);
    if (isHostageRescued(this, hostage3))  forgetHostage(hostage3);
    if (isHostageRescued(this, hostage4))  forgetHostage(hostage4);
}

bool cBot::isOnSameTeamAs(cBot *pBot) {
    if (pBot == NULL) return false;
    return pBot->iTeam == this->iTeam;
}

bool cBot::wantsToBuyStuff() {
    return buy_secondary == true ||
           buy_primary == true ||
           buy_ammo_primary == true ||
           buy_ammo_secondary == true ||
           buy_armor == true ||
           buy_defusekit == true ||
           buy_grenade == true ||
           buy_flashbang > 0;
}

bool cBot::isUsingConsole() {
    return console_nr > 0;
}

bool cBot::shouldBeAbleToMove() {
    return  !isDead() &&
            !isFreezeTime() &&
            !shouldCamp() &&
            !shouldWait() &&
            !shouldActWithC4();
//            !isDucking() &&
//            !isJumping();
}

edict_t *cBot::getEntityBetweenMeAndCurrentPathNodeToHeadFor() {
    TraceResult tr;
    Vector vOrigin = pEdict->v.origin;

    tNode *node = NodeMachine.getNode(getCurrentPathNodeToHeadFor());

    //Using TraceHull to detect de_aztec bridge and other entities.
    //DONT_IGNORE_MONSTERS, we reached it only when there are no other bots standing in our way!
    //UTIL_TraceHull(vOrigin, vNode, dont_ignore_monsters, point_hull, pBot->pEdict, &tr);
    //UTIL_TraceHull(vOrigin, vNode, dont_ignore_monsters, human_hull, pBot->pEdict, &tr);
    UTIL_TraceHull(vOrigin, node->origin, dont_ignore_monsters, head_hull, pEdict, &tr);

    // if nothing hit (else a wall is in between and we don't care about that):
    if (tr.flFraction < 1.0) {
        if (tr.pHit) {
            return tr.pHit;
        }
    }

    return NULL;
}

/**
 * Get distance to the next node we're heading for
 * @return
 */
float cBot::getDistanceToNextNode() {
    tNode *node = NodeMachine.getNode(getCurrentPathNodeToHeadFor());
    if (node) {
        return getDistanceTo(node->origin);
    }
    return MAP_MAX_SIZE;
}

void cBot::setBodyToNode(int nodeIndex) {
    tNode *node = NodeMachine.getNode(nodeIndex);
    if (node) {
        vBody = node->origin;
    }
}

void cBot::lookAtNode(int nodeIndex) {
    tNode *node = NodeMachine.getNode(nodeIndex);
    if (node) {
        vHead = node->origin;
    }
}

/**
 * Sets timer to allow movement to node, when timer expires we will think about severing the connection
 * we used.
 * @param timeInSeconds
 */
void cBot::setTimeToMoveToNode(float timeInSeconds) {
    char msg[255];
    float endTime = gpGlobals->time + timeInSeconds;
    sprintf(msg, "Set to %f so results into end time of %f", timeInSeconds, endTime);
    rprint_trace("setTimeToMoveToNode", msg);

    this->nodeTimeIncreasedAmount = 0;
    this->fMoveToNodeTime = endTime;
}

/**
 * Whatever was set, increase the time given in function param. This expands the time a bit.
 * @param timeInSeconds
 */
void cBot::increaseTimeToMoveToNode(float timeInSeconds) {
    if (nodeTimeIncreasedAmount < 2) {
        nodeTimeIncreasedAmount++;
        this->fMoveToNodeTime += timeInSeconds;
        float timeToMoveToNodeRemaining = getMoveToNodeTimeRemaining();
        char msg[255];
        memset(msg, 0, sizeof(msg));
        sprintf(msg, "increaseTimeToMoveToNode with %f for the %d time, making time to move to node remaining %f.", timeInSeconds, nodeTimeIncreasedAmount, timeToMoveToNodeRemaining);
        rprint_trace("increaseTimeToMoveToNode", msg);
    } else {
        rprint_trace("increaseTimeToMoveToNode", "Refused to increase time");
    }
}

float cBot::getMoveToNodeTimeRemaining() {
    return fMoveToNodeTime - gpGlobals->time;
}

bool cBot::shouldCamp() {
    return f_camp_time > gpGlobals->time;
}

bool cBot::shouldWait() {
    return f_wait_time > gpGlobals->time;
}

void cBot::setTimeToWait(float timeInSeconds) {
    this->f_wait_time = gpGlobals->time + timeInSeconds;
}

bool cBot::shouldBeAbleToInteractWithButton() {
    return fButtonTime < gpGlobals->time;
}

bool cBot::hasButtonToInteractWith() {
    return pButtonEdict != NULL;
}

bool cBot::hasCurrentNode() {
    return iCloseNode > -1;
}

/**
 * Shorthand method for creating path with flags PATH_NONE.
 * @param destinationNode
 * @return
 */
bool cBot::createPath(int destinationNode) {
    return createPath(destinationNode, PATH_NONE);
}

/**
 * Attempts to create a path from current node to destination. Returns true on success, false on failure.
 * @param destinationNode
 * @param flags
 * @return
 */
bool cBot::createPath(int destinationNode, int flags) {
    if (destinationNode < 0 || destinationNode >= MAX_NODES) {
        rprint("createPath()", "Unable to create path because destination node provided is < 0 or > MAX_NODES");
        return false;
    }

    int currentNode = getCurrentNode();
    if (currentNode < 0) {
        rprint("createPath()", "Unable to create path to destination because I am not close to a start node");
        return false;
    }

    forgetPath();

    char msg[255];
    memset(msg, 0, sizeof(msg));
    sprintf(msg, "Creating path from currentNode [%d] to destination node [%d]", currentNode, destinationNode);
    rprint("createPath()", msg);

    return NodeMachine.createPath(currentNode, destinationNode, iBotIndex, this, flags);
}

void cBot::doDuck() {
    UTIL_BotPressKey(this, IN_DUCK);
    this->f_hold_duck = gpGlobals->time + 0.5;

    this->increaseTimeToMoveToNode(0.5);
}

bool cBot::isDucking() {
    bool b = keyPressed(IN_DUCK) || this->f_hold_duck > gpGlobals->time;
    if (b) {
        rprint_trace("isDucking", "Yes I am ducking");
    }
    return b;
}

bool cBot::isWalking() {
    bool b = !keyPressed(IN_RUN) || this->f_walk_time > gpGlobals->time;
    if (b) {
        rprint_trace("isWalking", "Yes I am walking");
    }
    return b;
}

void cBot::doJump(Vector &vector) {
    rprint_trace("doJump", "With vector");
    // stay focussed with body and head to this vector
    this->vHead = vector;
    this->vBody = vector;

    doJump();
}

void cBot::doJump() {
    rprint_trace("doJump", "no vector");
    UTIL_BotPressKey(this, IN_JUMP);
    this->f_jump_time = gpGlobals->time + 0.5;

    // duck like this, because doDuck increases node time *again*, so no
    UTIL_BotPressKey(this, IN_DUCK); // DUCK jump by default
    this->f_hold_duck = gpGlobals->time + 0.5;

    this->increaseTimeToMoveToNode(0.75);
}

bool cBot::isJumping() {
    bool b = keyPressed(IN_JUMP) || f_jump_time > gpGlobals->time;
    if (b) {
        rprint_trace("isJumping", "Yes I am jumping");
    }
    return b;
}

// $Log: bot.cpp,v $
// Revision 1.21  2004/09/07 18:23:02  eric
// - bumped version to 3061
// - adding Frashman code to buy the weapon as selected by Josh's code
// - Realbot is really the result of multiple people :-)
//
// Revision 1.20  2004/09/07 15:44:34  eric
// - bumped build nr to 3060
// - minor changes in add2 (to add nodes for Bsp2Rbn utilities)
// - if compiled with USE_EVY_ADD, then the add2() function is used when adding
//   nodes based on human players instead of add()
// - else, it now compiles mostly without warnings :-)
//
// Revision 1.19  2004/08/07 18:42:56  eric
// - bumped version to 3058
// - added a cNodeMachine::add2 which should do the same job as ::add
//   but it seems to work better. ::add2 is used by Bsp2Rbn only for now.
// - added the display of node flags (water, ladder, jump) next to the
//   node position in 'debug nodes draw'
// - suppress the debugging information which displayed the hit entity
//   while establishing neighbourhood
//
// Revision 1.18  2004/07/30 15:02:29  eric
// - jumped to version 3057
// - improved readibility (wapen_tabel -> weapons_table) :-P
// - all Josh Borke modifications to the buying stuff:
//     * using a switch() instead of several if
//     * better buying code for shield and primary weapons
//     * new command 'debug pistols 0/1'
//
// Revision 1.16  2004/07/17 21:32:01  eric
// - bumped version to 3055
// - handling of es_ and as_ maps with new goals
// - added two debug commands:
//    realbot debug goals
//    realbot debug bots
// - added two nodes commands (for dedicated servers mainly)
//    realbot nodes connect n1 n2
//    realbot nodes disconnect n1 n2
// - slight modification in goal scoring (only reduced score when two bots of
//   the SAME team select the same goal)
//
// Revision 1.15  2004/07/03 15:58:54  eric
// nova test comment for erics account
//
// Revision 1.14  2004/07/02 16:43:35  stefan
// - upped to build 3051
// - changed log() into rblog()
// - removed BOT.CFG code that interpets old RB V1.0 commands
// - neater respons of the RealBot console
// - more help from RealBot console (ie, type realbot server broadcast ... with no arguments it will tell you what you can do with this, etc)
// - removed message "bot personality loaded from file"
// - in overal; some cleaning done, no extra features added
//
// Revision 1.13  2004/07/01 18:09:46  stefan
// - fixed skill 10 bots not causing memory bugger on re-adding (respawning)
// - added extra check for respawning bots so auto-add function cannot crash
// - fixed 2 nitpicks pointed out on the forums
//
// Revision 1.12  2004/06/25 07:39:00  stefan
// - upped to build 3050
// - fixed reaction time (instant reaction time) bug
// - added evy's goals, but they are not used yet
// - fixed some radio responses here and there for swat behaviour.
// - swat leader automaticly assigned again when one dies
// - HINT: you can see any changes made by me, by looking at DD/MM/YY - Stefan (ie, 22/06/04 - Stefan, will let you find all changes i made that day)
//
// Revision 1.11  2004/06/23 08:24:14  stefan
// - upped to build 3049
// - added swat behaviour (team leader assignment, radio response change and leaders command team-mates) - THIS IS EXPERIMENTAL AND DOES NOT ALWAYS WORK AS I WANT IT TO.
// - changed some conditions in nodemachine
// - sorry evy, still not added your new goals ;) will do next time, i promise
//
// Revision 1.10  2004/06/20 10:24:13  stefan
// - fixed another steep/stair thingy
// - changed a bit of the aiming code
//
// Revision 1.9  2004/06/19 21:06:14  stefan
// - changed distance check in nodemachine
// - fixed some 'steep' bug in nodemachine
//
// Revision 1.8  2004/06/17 21:23:23  stefan
// - fixes several connection problems with nodes. Going down from steep + crates (de_dust) PLUS going up/down from very steep slopes on as_oilrig.. 0wnage and thx to PMB and Evy
// - fixed chat bug in CS 1.6, its still CS 1.5 & CS 1.6 compatible though
//
// Revision 1.7  2004/06/13 20:08:21  stefan
// - 'bad score for goals' added
// - bmp dump info (Thanks Evy)
// - added 'realbot server players', so you can keep a server full at NR players at all times
// - slightly adjusted goal selection code
// - wander code disabled
// - lots of debug info introduced, do not use this source for REAL USAGE!
//
// Revision 1.6  2004/06/01 15:30:57  stefan
// *** empty log message ***
//
// Revision 1.5  2004/05/29 19:05:47  stefan
// - upped to BUILD 3044
// - removed several debug messages on screen
// - changed default 'chatrate (max sentences)' to 3
// - removed copyright notice, which is not valid due GPL license
//
// i know, nothing special :)
//
// Revision 1.4  2004/05/07 13:33:49  stefan
// added more comments, more neat code now
//
// Revision 1.3  2004/04/18 18:32:36  stefan
// - Restructured code a bit
//
// Revision 1.2  2004/04/18 17:39:19  stefan
// - Upped to build 2043
// - REALBOT_PRINT() works properly now
// - Log() works properly now
// - Clearing in dll.cpp of reallog.txt at dll init
// - Logging works now, add REALBOT_PRINT() at every point you want to log something.
//
