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

#ifndef BOT_FUNC_H
#define BOT_FUNC_H

//prototypes of bot functions...
void BotThink(cBot * pBot);

void BotFixIdealPitch(edict_t * pEdict);
void BotFixIdealYaw(edict_t * pEdict);
bool BotCanJumpUp(cBot * pBot);
bool BotCanDuckUnder(cBot * pBot);

bool EntityIsVisible(edict_t * pEntity, Vector dest);

// bot_func.cpp
bool VectorIsVisible(Vector start, Vector dest, char *checkname);
float func_distance(Vector v1, Vector v2);
void WaypointDrawBeam(edict_t * pEntity, Vector start, Vector end,
                      int width, int noise, int red, int green, int blue,
                      int brightness, int speed);
cBot *search_near_players(cBot * pBot);
bool BOOL_search_near_players(cBot * pBot);
bool BotShouldJump(cBot * pBot);
bool BotShouldDuck(cBot * pBot);
void HostageNear(cBot * pBot);
void FUNC_BotUpdateHostages(cBot * pBot);
Vector FUNC_CalculateAngles(cBot * pBot);

// New funcs
bool FUNC_DoRadio(cBot * pBot);

bool FUNC_ShouldTakeCover(cBot * pBot);
bool FUNC_TakeCover(cBot * pBot);

int FUNC_EdictHoldsWeapon(edict_t * pEdict);

int FUNC_FindFarWaypoint(cBot * pBot, Vector avoid, bool safest);
int FUNC_FindCover(int from, int to);
int FUNC_PlayerSpeed(edict_t * pPlayer);

bool FUNC_PlayerRuns(int speed);
void FUNC_HearingTodo(cBot * pBot);
void FUNC_ClearEnemyPointer(edict_t * c_pointer);

bool FUNC_IsOnLadder(edict_t * pEntity);
void FUNC_FindBreakable(cBot * pBot);
void FUNC_CheckForBombPlanted();

bool FUNC_UsedHostage(cBot * pBot, edict_t * pEdict);   // returns true when already used
void FUNC_UseHostage(cBot * pBot, edict_t * pEdict);    // run this when a bot uses a hostage
int FUNC_GiveHostage(cBot * pBot);      // gives any hostage we still have to go for
int FUNC_AmountHostages(cBot * pBot);
bool FUNC_FreeHostage(cBot * pBot, edict_t * pEdict);   // is this hostage not used by any other bot?
void FUNC_RemoveHostage(cBot * pBot, edict_t * pEdict); // remove hostage from memory

int FUNC_BotEstimateHearVector(cBot * pBot, Vector v_sound);
bool FUNC_HostagesMoving(cBot * pBot);

bool FUNC_BotHoldsZoomWeapon(cBot * pBot);

int FUNC_InFieldOfView(edict_t * pEntity, Vector dest);

bool VectorIsVisibleWithEdict(edict_t * pEdict, Vector dest,
                              char *checkname);

bool BOT_DecideTakeCover(cBot * pBot);

// bot_buycode.cpp
void BotConsole(cBot * pBot);

void rblog(char *txt);

// bot.cpp

// util.cpp
int UTIL_GiveWeaponId(const char *name);
int UTIL_GiveWeaponType(int weapon_id);
int UTIL_GetGrenadeType(edict_t * pEntity);

bool UTIL_IsVip(edict_t * pEntity);

char *UTIL_GiveWeaponName(int id);

void UTIL_SpeechSynth(edict_t * pEdict, char *szMessage);
void UTIL_BotRadioMessage(cBot * pBot, int radio, char *arg1, char *arg2);
void UTIL_BotPressKey(cBot * pBot, int type);
// bot_navigate.cpp

// ..
void CenterMessage(char *buffer);

#endif                          // BOT_FUNC_H
