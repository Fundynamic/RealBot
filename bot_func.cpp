/**
  * RealBot : Artificial Intelligence
  * Version : Work In Progress
  * Author  : Stefan Hendriks
  * Url     : http://realbot.bots-united.com
  **
  * DISCLAIMER
  *
  * History, Information & Credits: 
  * RealBot is based partially uppon the HPB-Bot Template #3 by Botman
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
//
extern cNodeMachine NodeMachine;

// For taking cover decision
#define TOTAL_SCORE 16300       // 16000 money + 100 health + 100 fear + 100 camp desire

bool
VectorIsVisibleWithEdict(edict_t * pEdict, Vector dest, char *checkname) {
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

int FUNC_InFieldOfView(edict_t * pEntity, Vector dest) {
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
WaypointDrawBeam(edict_t * pEntity, Vector start, Vector end, int width,
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
cBot *search_near_players(cBot * pBot) {
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
bool BOOL_search_near_players(cBot * pBot) {
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

bool BotShouldJump(cBot * pBot) {
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
Vector FUNC_CalculateAngles(cBot * pBot) {
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

bool BotShouldDuck(cBot * pBot) {

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

bool FUNC_DoRadio(cBot * pBot) {
   if (pBot->fDoRadio > gpGlobals->time)
      return false;

   int iRadio = pBot->ipCreateRadio;

   // when leader more chance to do radio to control team
   if (BOT_IsLeader(pBot))
      iRadio += 30;

   if (RANDOM_LONG(0, 100) < iRadio)
      return true;

   return false;
}

// DECIDE: Take cover or not
bool FUNC_ShouldTakeCover(cBot * pBot) {
   // Do not allow taking cover within 5 seconds again.
   if (pBot->f_cover_time + 3 > gpGlobals->time)
      return false;

   if (pBot->pBotEnemy == NULL)
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

bool FUNC_BotHasWeapon(cBot * pBot, int type) {
   if (pBot->bot_weapons & (1 << type))
      return true;
   else
      return false;
}

bool FUNC_BotHoldsWeapon(cBot * pBot, int type) {
   if (pBot->current_weapon.iId == type)
      return true;

   return false;
}

void C4Near(cBot * pBot) {
   if (pBot->pBotEnemy != NULL)
      return;

   if (pBot->f_c4_time > gpGlobals->time
         && pBot->pEdict->v.button & IN_USE) {
      pBot->f_node_timer = gpGlobals->time + 3;

      // DEFUSING BOMB
      // make all other bots not defuse this..
      for (int i = 1; i <= gpGlobals->maxClients; i++) {
         edict_t *pPlayer = INDEXENT(i);
         cBot *bot = UTIL_GetBotPointer(pPlayer);

         if (bot)
            if (bot->iTeam == 2)
               if (bot != pBot) {
                  // when its not the same bot  but he is close
                  if (func_distance
                        (bot->pEdict->v.origin, pBot->pEdict->v.origin) < 250
                     && bot->f_camp_time < gpGlobals->time) {}

               }

      }
   }
   // NEW - Rewritten defuse code:
   // What i do, i search for the c4 timer, store its origin and check if this bot is close
   // If so, the bot should be defusing the bomb if the timers are set. The above check
   // makes sure that no other bot will be defusing the bomb.

   edict_t *pent = NULL;

   Vector c4_origin = Vector(9999, 9999, 9999);

   while ((pent = UTIL_FindEntityByClassname(pent, "grenade")) != NULL) {
      if (UTIL_GetGrenadeType(pent) == 4)       // It is a C4
      {
         c4_origin = pent->v.origin;    // store origin
         break;                 // done our part now
      }
   }                            // --- find the c4

   // When we found it, the vector of c4_origin should be something else then (9999,9999,9999)
   if (c4_origin != Vector(9999, 9999, 9999)) {
      // A c4 has been found, oh dear.
      // Remember, pent=c4 now!

      // Calculate the distance between our position to the c4
      float distance = func_distance(pBot->pEdict->v.origin, c4_origin);

      // can we see it?
      // FIXME: See the bomb you cow!
      if (VectorIsVisibleWithEdict(pBot->pEdict, c4_origin, "none")) {
         // We can do 2 things now
         // - If we are not close, we check if we can walk to it, and if so we face to the c4
         // - If we are close, we face it and (begin) defuse the bomb.

         if (distance < 70) {
            pBot->vHead = c4_origin;
            pBot->vBody = c4_origin;

            pBot->f_node_timer = gpGlobals->time + 3;   // we are going to do non-path-follow stuff, so keep timer updated

            int angle_to_c4 = FUNC_InFieldOfView(pBot->pEdict,
                                                 (c4_origin -
                                                  pBot->pEdict->v.origin));

            if (pBot->f_defuse < gpGlobals->time && angle_to_c4 < 35) {
               // when we are 'about to' defuse, we simply set the timers
               pBot->f_defuse = gpGlobals->time + 90;   // hold as long as you can
               pBot->f_allow_keypress = gpGlobals->time + 1.5;  // And stop any key pressing the first second
               // ABOUT TO DEFUSE BOMB
            }

            if (pBot->f_defuse > gpGlobals->time && angle_to_c4 < 35) {
               pBot->f_move_speed = 0.0;
               pBot->f_c4_time = gpGlobals->time + 6;
               UTIL_BotPressKey(pBot, IN_DUCK);

               if (func_distance(pBot->pEdict->v.origin, c4_origin) > 50
                     && pBot->f_allow_keypress + 0.5 > gpGlobals->time)
                  pBot->f_move_speed = pBot->f_max_speed / 2;
            }

            if (pBot->f_allow_keypress < gpGlobals->time
                  && pBot->f_defuse > gpGlobals->time)
               UTIL_BotPressKey(pBot, IN_USE);
         } else {
            // Check if we can walk to it
            // TODO: work on this, it does not have to be nescesarily walkable.
            // TODO TODO TODO , get this working with nodes
            pBot->vHead = c4_origin;
            pBot->vBody = c4_origin;
         }
      }

   }

   return;                      // defuse

}

int FUNC_BotEstimateHearVector(cBot * pBot, Vector v_sound) {
   // here we normally figure out where to look at when we hear an enemy, RealBot AI PR 2 lagged a lot on this so we need another approach

   return -1;
}

/*----------------
REALBOT: Think about goals
----------------*/
void FUNC_BotThinkAboutGoal(cBot * pBot) {
   // Depends on team
   if (pBot->iTeam == 1) {}
   else {}

   /*----------------
   REALBOT: Execute stuff
   ----------------*/

}

// Added Stefan
// 7 November 2001
int FUNC_PlayerSpeed(edict_t * pPlayer) {
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
int FUNC_EdictHoldsWeapon(edict_t * pEdict) {
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
void FUNC_HearingTodo(cBot * pBot) {
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
      pBot->iGoalNode = -1;
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
void FUNC_ClearEnemyPointer(edict_t * pPtr) {
   // Go through all bots and remove their enemy pointer that matches the given
   // pointer pPtr
   for (int i = 1; i <= gpGlobals->maxClients; i++) {
      edict_t *pPlayer = INDEXENT(i);

      // Skip invalid players.
      if ((pPlayer) && (!pPlayer->free)) {
         // skip this player if not alive (i.e. dead or dying)
         if (!IsAlive(pPlayer))
            continue;

         if (!(pPlayer->v.flags & FL_THIRDPARTYBOT))
            continue;

         // Only check bots
         cBot *botpointer = UTIL_GetBotPointer(pPlayer);

         if (botpointer)        // Is a bot
            if (botpointer->pBotEnemy == pPtr)  // Has same pointer
               botpointer->pBotEnemy = NULL;    // Clear its pointer
      }

   }
   // End of function
}

// Returns true/false if an entity is on a ladder
bool FUNC_IsOnLadder(edict_t * pEntity) {
   if (pEntity == NULL)
      return false;

   if (pEntity->v.movetype == MOVETYPE_FLY)
      return true;

   return false;
}

// Amount of hostages of a bot
int FUNC_AmountHostages(cBot * pBot) {
   int a = 0;

   if (pBot->hostage1 != NULL)
      a++;

   if (pBot->hostage2 != NULL)
      a++;

   if (pBot->hostage3 != NULL)
      a++;

   if (pBot->hostage4 != NULL)
      a++;

   return a;
}

int FUNC_GiveHostage(cBot * pBot) {
   // loops through all the hostages for the bot that has to be rescued.
   // Only give hostage that is not close to us.

   edict_t *pent = NULL;

   int iNode = -1;

   while ((pent =
              UTIL_FindEntityByClassname(pent, "hostage_entity")) != NULL) {
      if (pent->v.effects & EF_NODRAW)
         continue;              // rescued already

      // dead
      if (pent->v.health < 1)
         continue;

      // find a waypoint near
      if (FUNC_UsedHostage(pBot, pent) == true) {
         //BotDebug("Hostage is already in use by me!\n");
         continue;              // we seem to use this hostage already
      }

      if (FUNC_FreeHostage(pBot, pent) == false) {
         //tDebug("Hostage is already in use by someone else!\n");
         continue;              // somebody else is taking this hostage already.
      }

      iNode = NodeMachine.close(pent->v.origin, 75, pBot->pEdict);

      if (iNode > -1)
         return iNode;

   }

   return -1;

}

bool FUNC_UsedHostage(cBot * pBot, edict_t * pEdict) {

   // checks if the current pEdict is already 'in use'
   // note: time check only when we own the hostage!
   if (pBot->hostage1 == pEdict)
      return true;

   if (pBot->hostage2 == pEdict)
      return true;

   if (pBot->hostage3 == pEdict)
      return true;

   if (pBot->hostage4 == pEdict)
      return true;

   return false;
}

// REMOVE a hostage
void FUNC_RemoveHostage(cBot * pBot, edict_t * pEdict) {
   if (pBot->hostage1 == pEdict)
      pBot->hostage1 = NULL;

   if (pBot->hostage2 == pEdict)
      pBot->hostage2 = NULL;

   if (pBot->hostage3 == pEdict)
      pBot->hostage3 = NULL;

   if (pBot->hostage4 == pEdict)
      pBot->hostage4 = NULL;
}

// USE a hostage
void FUNC_UseHostage(cBot * pBot, edict_t * pEdict) {
   if (pBot->hostage1 == NULL)
      pBot->hostage1 = pEdict;
   else if (pBot->hostage2 == NULL)
      pBot->hostage2 = pEdict;
   else if (pBot->hostage3 == NULL)
      pBot->hostage3 = pEdict;
   else if (pBot->hostage4 == NULL)
      pBot->hostage4 = pEdict;

   // TODO TODO TODO
   // Add to memory about this event (used a hostage)
}

bool FUNC_FreeHostage(cBot * pBot, edict_t * pEdict) {
   for (int i = 1; i <= gpGlobals->maxClients; i++) {
      edict_t *pPlayer = INDEXENT(i);

      // Skip invalid players.
      if ((pPlayer) && (!pPlayer->free)) {

         // skip this player if not alive (i.e. dead or dying)
         if (!IsAlive(pPlayer))
            continue;

         if (!(pPlayer->v.flags & FL_THIRDPARTYBOT))
            continue;

      }
      // Only check bots and do not check self!
      cBot *botpointer = UTIL_GetBotPointer(pPlayer);
      if (botpointer && (botpointer != pBot))
         if (FUNC_UsedHostage(botpointer, pEdict))
            return false;       // uh oh!

   }

   return true;                 // ok!
}

void HostageNear(cBot * pBot) {
   if (pBot->pBotEnemy != NULL)
      return;                   // enemy, do not check

   edict_t *pent = NULL;

   Vector ent_vec = Vector(9999, 9999, 9999);
   if (pBot->pBotHostage == NULL) {

      // Search for all hostages in the game
      while ((pent =
                 UTIL_FindEntityByClassname(pent, "hostage_entity")) != NULL) {

         // 1. Already rescued?
         if (pent->v.effects & EF_NODRAW)
            continue;

         // 2. Already used?
         if (FUNC_UsedHostage(pBot, pent))
            continue;

         // 3. Already moving? (used 2)
         if (FUNC_PlayerSpeed(pent) > 2)
            continue;

         // 4. Is the hostage not used by anyone else? (used 3)
         if (FUNC_FreeHostage(pBot, pent) == false)
            continue;

         // 5. Already close?
         if (func_distance(pBot->pEdict->v.origin, pent->v.origin) < 125)       // in range?
            if (pBot->CanSeeEntity(pent))       // visible?
            {
               ent_vec = pent->v.origin;
               pBot->pBotHostage = pent;
               REALBOT_PRINT(pBot, "HostageNear()",
                             "I have found a new hostage target");
               break;
            }
      }
   }
   // Whenever we have a hostage to go after
   if (pBot->pBotHostage != NULL) {
      bool bIsStillFree = true;

      // 1. Already rescued?
      if (pBot->pBotHostage->v.effects & EF_NODRAW)
         bIsStillFree = false;

      // 2. Already used?
      if (FUNC_UsedHostage(pBot, pBot->pBotHostage))
         bIsStillFree = false;

      // 3. Already moving? (used 2)
      if (FUNC_PlayerSpeed(pBot->pBotHostage) > 2)
         bIsStillFree = false;

      // 4. Is the hostage not used by anyone else? (used 3)
      if (FUNC_FreeHostage(pBot, pBot->pBotHostage) == false)
         bIsStillFree = false;

      // Not free anymore... darn
      if (bIsStillFree == false) {
         pBot->pBotHostage = NULL;
         REALBOT_PRINT(pBot, "HostageNear", "pBotHostage is in use\n");
         return;
      }
      // Prevent bots getting to close here
      if (func_distance
            (pBot->pEdict->v.origin, pBot->pBotHostage->v.origin) < 80)
         pBot->f_move_speed = 0.0;      // do not get to close ;)

      // From here, we should get the hostage when still visible
      if (pBot->CanSeeEntity(pBot->pBotHostage)) {
         pBot->vBody = pBot->vHead =
                          pBot->pBotHostage->v.origin + Vector(0, 0, 36);
         pBot->f_move_speed = pBot->f_max_speed;
         int angle_to_hostage = FUNC_InFieldOfView(pBot->pEdict,
                                (pBot->vBody -
                                 pBot->pEdict->v.
                                 origin));

         if (func_distance
               (pBot->pEdict->v.origin, pBot->pBotHostage->v.origin) < 90) {
            pBot->f_move_speed = 0.0;
            REALBOT_PRINT(pBot, "HostageNear()",
                          "I can see hostage, we are close, awaiting timer and angle status.");

            if (angle_to_hostage <= 30
                  && (pBot->f_use_timer < gpGlobals->time)) {
               pBot->f_use_timer = gpGlobals->time + 0.7;       // give the bot time to face other hostages (if any)
               UTIL_BotPressKey(pBot, IN_USE);
               FUNC_UseHostage(pBot, pBot->pBotHostage);
               pBot->pBotHostage = NULL;
               REALBOT_PRINT(pBot, "HostageNear()",
                             "I pressed USE and assume i have used the hostage");
               pBot->f_wait_time = gpGlobals->time + 0.5;
            }
         }
         pBot->iGoalNode = -1;
      }
   }                            //
}                               // HostageNear()

bool FUNC_HostagesMoving(cBot * pBot) {
   // the question is, do the used hostages move?
   // and..  (if they just got used, they probabbly dont move, so check if they are close then )
   if (pBot->f_use_timer + 5 > gpGlobals->time) {
      // we just used a hostage, it could be that one of them is not moving, so this is ok!
      return true;
   } else {
      // ok it has been a while, do some checking
      if (pBot->hostage1 != NULL)
         if (FUNC_PlayerSpeed(pBot->hostage1) > 0)
            return true;
         else
            return false;

      if (pBot->hostage2 != NULL)
         if (FUNC_PlayerSpeed(pBot->hostage3) > 0)
            return true;
         else
            return false;

      if (pBot->hostage3 != NULL)
         if (FUNC_PlayerSpeed(pBot->hostage3) > 0)
            return true;
         else
            return false;

      if (pBot->hostage4 != NULL)
         if (FUNC_PlayerSpeed(pBot->hostage4) > 0)
            return true;
         else
            return false;

   }

   return false;
}

// updates hostage when rescued or not
void FUNC_BotUpdateHostages(cBot * pBot) {

   if (pBot->hostage1 != NULL)
      if (pBot->hostage1->v.effects & EF_NODRAW)
         pBot->hostage1 = NULL;

   if (pBot->hostage2 != NULL)
      if (pBot->hostage2->v.effects & EF_NODRAW)
         pBot->hostage2 = NULL;

   if (pBot->hostage3 != NULL)
      if (pBot->hostage3->v.effects & EF_NODRAW)
         pBot->hostage3 = NULL;

   if (pBot->hostage4 != NULL)
      if (pBot->hostage4->v.effects & EF_NODRAW)
         pBot->hostage4 = NULL;

}

bool FUNC_BotHoldsZoomWeapon(cBot * pBot) {
   // Check if the bot holds a weapon that can zoom, but is not a sniper gun.
   if (FUNC_BotHoldsWeapon(pBot, CS_WEAPON_AUG))
      return true;

   if (FUNC_BotHoldsWeapon(pBot, CS_WEAPON_SG552))
      return true;

   return false;
}

void FUNC_BotChecksFalling(cBot * pBot) {
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
bool BOT_DecideTakeCover(cBot * pBot) {
   /*
      	UTIL_ClientPrintAll( HUD_PRINTCENTER, "DECISION TO TAKE COVER\n" );
    
   	int iNodeEnemy = NodeMachine.close(pBot->pBotEnemy->v.origin, NODE_ZONE);
   	int iNodeHere  = NodeMachine.close(pBot->pEdict->v.origin, NODE_ZONE);
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


// 20/06/04 - Stefan
// check if there are any bots pointing to this one.
bool BOT_IsLeader(cBot * pBot) {
   int iBot;

   for (iBot = 0; iBot < 32; iBot++) {
      if (bots[iBot].bIsUsed) {
         // pointer to this one
         cBot *pThisBot = &bots[iBot];

         // not a bot (not valid)
         if (pThisBot == NULL)
            continue;

         // do not check self
         if (pThisBot != pBot) {
            if (pThisBot->pSwatLeader == pBot->pEdict)
               return true;     // we found a bot that links to this bot!
         }
      }
   }

   return false;
}

// return a pointer that could be a leader!
edict_t *EDICT_LEADER(int iIndex) {

   // go through all clients
   for (; iIndex <= gpGlobals->maxClients; iIndex++) {
      edict_t *pPlayer = INDEXENT(iIndex);

      // skip invalid players
      if ((pPlayer) && (!pPlayer->free)) {
         if (!IsAlive(pPlayer))
            continue;

         // randomly return this one...
         cBot *pBot = UTIL_GetBotPointer(pPlayer);
         bool bCanBeLeader = false;

         if (pBot) {
            // can be leader?
            if (pBot->pSwatLeader == NULL)
               bCanBeLeader = true;
         } else
            bCanBeLeader = true;

         // found leader
         if (bCanBeLeader)
            return pPlayer;
      }
   }

   return NULL;
}

// initialize the bots of leader edict
void ORDER_BotsOfLeader(edict_t * pEdict, int iGoalNode) {
   int iBot;
   for (iBot = 0; (iBot < 32); iBot++) {
      if (bots[iBot].bIsUsed) {
         // pointer to this one
         cBot *pBot = &bots[iBot];

         if (pBot)
            if (pBot->pSwatLeader == pEdict) {
               // when this bot has leader set to this edict, clear path settings and go to iGoalNode
               char msg[128];
               sprintf(msg, "'%s' ordered '%s' to go to %d\n",
                       STRING(pEdict->v.netname),
                       STRING(pBot->pEdict->v.netname), iGoalNode);
               rblog(msg);

               if (pBot->iGoalNode != iGoalNode) {
                  pBot->iGoalNode = iGoalNode;
                  pBot->bot_pathid = -1;        // reset path id, create new one
               }

               pBot->f_wait_time =
                  gpGlobals->time + RANDOM_FLOAT(1.0, 3.0);
            }
      }
   }
}

// logs into a file
void rblog(char *txt) {
   FILE *fplog;
   fplog = fopen("reallog.txt", "at");

   if (fplog) {
      fprintf(fplog, "%s", txt);        // print the text into the file
      fclose(fplog);
   }
}
