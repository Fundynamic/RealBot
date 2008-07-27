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
#include "NodeMachine.h"
#include "IniParser.h"
#include "bot_func.h"

extern cNodeMachine NodeMachine;
extern cGame Game;
extern cBot bots[32];

// GAME: Init
void
cGame::Init() {

   // Default Bot Skill
   iDefaultBotSkill = -1;       // random
   iOverrideBotSkill = GAME_YES;

   // Randomized skill boundries
   iRandomMinSkill = 0;
   iRandomMaxSkill = 10;

   // Internet simulation
   iMinPlayRounds = 5;
   iMaxPlayRounds = 25;

   // New round stuff
   bNewRound = false;

   // Broadcasting
   iVersionBroadcasting = BROADCAST_ROUND;
   iKillsBroadcasting = BROADCAST_KILLS_FULL;
   iDeathsBroadcasting = BROADCAST_DEATHS_FULL;
   bSpeechBroadcasting = false; // @ default we do NOT broadcast this shit every round to the listen server

   // Installed into correct directory
   bInstalledCorrectly = true;

   // Round time
   fRoundTime = 0.0;

   // Dropped C4 location
   vDroppedC4 = Vector(9999, 9999, 9999);

   // May we walk with knife (when bots want to), default = yes (3600 seconds)
   fWalkWithKnife = 3600;

   // Bomb planted
   bBombPlanted = false;
   bBombDiscovered = false;

   // Chat related
   iMaxSentences = 1;           // max sentences produced by chatengine per second (1=default)
   iProducedSentences = 0;      // currently produced sentences

   // DEBUG RELATED
   bDoNotShoot = false;         // ... guess
   bDebug = false;              // ... prints debug messages
   bEngineDebug = false;        // ... prints engine debug messages (for figuring out engine interceptions)
   bPistols = false;            // pistols only mode

   // Speech sentences (from POD and a *few* own made)
   strcpy(cSpeechSentences[0], "hello user,communication is acquired");
   strcpy(cSpeechSentences[1], "your presence is acknowledged");
   strcpy(cSpeechSentences[2], "high man, your in command now");
   strcpy(cSpeechSentences[3], "blast your hostile for good");
   strcpy(cSpeechSentences[4], "high man, kill some idiot here");
   strcpy(cSpeechSentences[5], "is there a doctor in the area");
   strcpy(cSpeechSentences[6], "warning, experimental materials detected");
   strcpy(cSpeechSentences[7], "high amigo, shoot some but");
   strcpy(cSpeechSentences[8], "attention, hours of work software, detected");
   strcpy(cSpeechSentences[9], "time for some bad ass explosion");
   strcpy(cSpeechSentences[10],"bad ass son of a breach device activated");
   strcpy(cSpeechSentences[11],"high, do not question this great service");
   strcpy(cSpeechSentences[12],"engine is operative, hello and goodbye");
   strcpy(cSpeechSentences[13],"high amigo, your administration has been great last day");
   strcpy(cSpeechSentences[14],"attention, expect experimental armed hostile presence");
   strcpy(cSpeechSentences[15],"warning,medical attention required");

}                               // Init()

// Returns random sentence for speech
char *cGame::RandomSentence() {
   return cSpeechSentences[RANDOM_LONG(0, 15)];
}

// GAME: Set round time
void cGame::SetRoundTime(float fTime) {
   fRoundTime = fTime;
}

// GAME: Get round time
// NOTE: This is the time when a round has just started!
float cGame::RoundTime() {
   return fRoundTime;
}

// GAME: Set new round flag
void cGame::SetNewRound(bool bState) {
   bNewRound = bState;
}

// GAME: Get new round status
bool cGame::NewRound() {
   return bNewRound;
}

// GAME: Set min and max playing rounds
void cGame::SetPlayingRounds(int iMin, int iMax) {
   if (iMin > -1)
      iMinPlayRounds = iMin;
   if (iMax > -1)
      iMaxPlayRounds = iMax;

   // Boundries
   if (iMinPlayRounds < 1)
      iMinPlayRounds = 1;
   if (iMaxPlayRounds < 2)
      iMaxPlayRounds = 2;

   // Make sure MAX never is lower then MIN
   if (iMinPlayRounds > iMaxPlayRounds)
      iMinPlayRounds = iMaxPlayRounds - 1;

}                               // SetPlayingRounds()

// GAME: GET Max playing rounds
int cGame::GetMaxPlayRounds() {
   return iMaxPlayRounds;
}

// GAME: GET Min playing rounds
int cGame::GetMinPlayRounds() {
   return iMinPlayRounds;
}

// GAME: Load bot.cfg
void cGame::LoadCFG() {
   // loads bot.cfg?

}                               // LoadCFG()

// GAME: Load names file
// NOTE: This function is just a copy/paste stuff from Botmans template, nothing more, nothing less
// TODO: Rewrite this, can be done much cleaner.
void cGame::LoadNames() {
   FILE *bot_name_fp;
   int str_index;
   char name_buffer[80];
   int length, index;
   char filename[256];
   UTIL_BuildFileNameRB("rb_names.txt", filename);
   bot_name_fp = fopen(filename, "r");
   if (bot_name_fp != NULL) {
      while ((iAmountNames < MAX_BOT_NAMES) &&
             (fgets(name_buffer, 80, bot_name_fp) != NULL)) {
         length = strlen(name_buffer);
         if (name_buffer[length - 1] == '\n') {
            name_buffer[length - 1] = 0;        // remove '\n'
            length--;
         }
         str_index = 0;
         while (str_index < length) {
            if ((name_buffer[str_index] < ' ')
                  || (name_buffer[str_index] > '~')
                  || (name_buffer[str_index] == '"'))
               for (index = str_index; index < length; index++)
                  name_buffer[index] = name_buffer[index + 1];
            str_index++;
         }

         if (name_buffer[0] != 0) {
            strncpy(cBotNames[iAmountNames], name_buffer, BOT_NAME_LEN);
            iAmountNames++;
         }
      }
      fclose(bot_name_fp);
   }
}                               // LoadNames()

// Any names available?
bool cGame::NamesAvailable() {
   if (iAmountNames > 0)
      return true;

   return false;
}                               // NamesAvailable()


// Picks a random name
// rewritten on april 10th 2004
void cGame::SelectName(char *name) {
   int iNameIndex, iIndex;
   bool bUsed;
   edict_t *pPlayer;
   iNameIndex = 0;              // zero based (RANDOM_LONG (0, iAmountNames-1))

   bool iNameUsed[MAX_BOT_NAMES];
   for (int i = 0; i < MAX_BOT_NAMES; i++) {
      iNameUsed[i] = false;
   }

   // check make sure this name isn't used
   bUsed = true;
   while (bUsed) {
      iNameIndex = RANDOM_LONG(0, iAmountNames - 1);    // pick random one
      int iLimit = iNameIndex;  // remember this.

      // make sure it is not checked yet
      while (iNameUsed[iNameIndex]) {
         // check again
         if (iNameUsed[iNameIndex] == false)
            break;

         // add up
         iNameIndex++;

         // make sure that it does not check out of range
         if (iNameIndex == iAmountNames)
            iNameIndex = 0;     // go to 0 (will be set to 0 next 'name')

         // when we are back to where we came from, get the fuck outta here
         if (iNameIndex == iLimit) {
            strcpy(name, "RealBot");
            return;
         }
      }

      // so far we did not find evidence that this name has been used already
      bUsed = false;

      // check if this name is used
      for (iIndex = 1; iIndex <= gpGlobals->maxClients; iIndex++) {
         pPlayer = INDEXENT(iIndex);
         if (pPlayer && !pPlayer->free) {
            if (strcmp(cBotNames[iNameIndex], STRING(pPlayer->v.netname))
                  == 0) {
               // atten tion, this namehas been used.
               bUsed = true;
               break;
            }
         }
      }

      if (bUsed)
         iNameUsed[iNameIndex] = true;  // set on true

   }

   // copy name into the name_buffer
   strcpy(name, cBotNames[iNameIndex]);
}                               // SelectName()

// GAME: Load BUYTABLE.INI file
void cGame::LoadBuyTable() {
   INI_PARSE_BUYTABLE();        // run from IniParser.cpp
}                               // LoadBuyTable()

// GAME: Update global vars (called by StartFrame)
void cGame::UpdateGameStatus() {

   // Used variables
   edict_t *pEnt;
   pEnt = NULL;

   // ------------------
   // Update: Dropped C4
   // ------------------
   // Its not dropped unless stated otherwise.
   vDroppedC4 = Vector(9999, 9999, 9999);

   // Find the dropped bomb
   while ((pEnt = UTIL_FindEntityByClassname(pEnt, "weaponbox")) != NULL) {
      // when DROPPED C4
      if ((FStrEq(STRING(pEnt->v.model), "models/w_backpack.mdl"))) {
         vDroppedC4 = pEnt->v.origin;   // this is the origin.
         break;
      }
   }

   // ------------------
   // Update: Is the bomb planted?
   // ------------------
   // Same as dropped c4, its NOT, unless stated otherwise.
   pEnt = NULL;
   Vector vVec = Vector(9999, 9999, 9999);
   bool bPlanted = bBombPlanted;        // is it planted?

   while ((pEnt = UTIL_FindEntityByClassname(pEnt, "grenade")) != NULL) {
      if (UTIL_GetGrenadeType(pEnt) == 4) {
         // Found planted bomb:
         bPlanted = true;
         float fDist = 200;
         int iBot = -1, k = 1;
         vVec = pEnt->v.origin;

         // FIND:
         // Find the player near this bomb
         for (; k <= gpGlobals->maxClients; k++) {
            edict_t *pPlayer = INDEXENT(k);
            // skip invalid players
            if ((pPlayer) && (!pPlayer->free)) {
               // skip this player if not alive (i.e. dead or dying)
               if (!IsAlive(pPlayer))
                  continue;

               if (!(pPlayer->v.flags & FL_THIRDPARTYBOT))
                  continue;

               // skip real players (or NOT realbot's)
               if (UTIL_GetBotPointer(pPlayer) == NULL)
                  continue;

               // Check the distance
               if (func_distance(vVec, pPlayer->v.origin) < fDist) {
                  iBot = k;     // remember this ID
                  fDist = func_distance(vVec, pPlayer->v.origin);       // update distance
               }
            }
         } // End of search

         // all counter-terrorists should know this, and they should head for the bomb
         if (bPlanted && bPlanted != bBombPlanted) {
            int i;
            for (i = 1; i <= gpGlobals->maxClients; i++) {
               edict_t *pPlayer = INDEXENT(i);
               cBot *bot = UTIL_GetBotPointer(pPlayer);

               if (bot)         // valid bot
               {
                  if (UTIL_GetTeam(bot->pEdict) == 1)   // Counter-Terrorists
                  {
                     bot->bot_pathid = -1;
                     bot->iGoalNode = NodeMachine.node_goal(GOAL_BOMBSPOT);
                  }             // ct
               }                // bot
            }                   // through all clients

            // It is not yet discovered
            bBombDiscovered = false;

            // Now update bBombPlanted
            bBombPlanted = bPlanted;
         }                      // planted, and not planted before
      }
   }

   // When bPlanted = false, we set bBombPlanted to false
   if (bPlanted == false) {
	   bBombPlanted = false;
   }
}	// UpdateGameStatus()

// Add bot -> ARG1(team), ARG2(skill), ARG3(model), ARG4(name)
int cGame::CreateBot(edict_t * pPlayer, const char *arg1, const char *arg2,
                     const char *arg3, const char *arg4) {
   edict_t *BotEnt;
   cBot *pBot;
   char c_skin[BOT_SKIN_LEN + 1];
   char c_name[BOT_NAME_LEN + 1];

   // clear
   memset(c_skin, 0, sizeof(c_skin));
   memset(c_name, 0, sizeof(c_name));

   int skill;
   int i, j, length;
   if ((arg4 != NULL) && (*arg4 != 0)) {
      strncpy(c_name, arg4, BOT_NAME_LEN - 1);
      c_name[BOT_NAME_LEN] = 0; // make sure c_name is null terminated
   } else {
      if (NamesAvailable())
         SelectName(c_name);
      else
         strcpy(c_name, "RealBot");
   }

   skill = -2;                  // -2, not valid

   if ((arg2 != NULL) && (*arg2 != 0))
      skill = atoi(arg2);       // set to given skill

   // when not valid (-2), it has default skill
   if ((skill < -1) || (skill > 10))
      skill = iDefaultBotSkill;

   // When skill is -1, random, we set it by boundries given
   if (skill == -1)
      skill = RANDOM_LONG(iRandomMinSkill, iRandomMaxSkill);

   // length of name
   length = strlen(c_name);

   // remove any illegal characters from name...
   for (i = 0; i < length; i++) {
      if ((c_name[i] <= ' ') || (c_name[i] > '~') || (c_name[i] == '"')) {
         for (j = i; j < length; j++)   // shuffle chars left (and null)
            c_name[j] = c_name[j + 1];
         length--;
      }
   }

   BotEnt = (*g_engfuncs.pfnCreateFakeClient) (c_name);
   if (FNullEnt(BotEnt)) {
      REALBOT_PRINT(NULL, "cGame::CreateBot",
                    "Cannot create bot, server is full");
      return GAME_MSG_FAIL_SERVERFULL;  // failed
   } else {
      char ptr[128];            // allocate space for message from ClientConnect
      char *infobuffer;
      int clientIndex;
      int index;
      index = 0;
      while ((bots[index].bIsUsed) && (index < 32))
         index++;
      if (index == 32) {
         return GAME_MSG_FAILURE;
      }
      // create the player entity by calling MOD's player function
      // (from LINK_ENTITY_TO_CLASS for player object)

      // FIX: Free data for bot, so we can fill in new
      if (BotEnt->pvPrivateData != NULL)
         FREE_PRIVATE(BotEnt);

      BotEnt->pvPrivateData = NULL;
      BotEnt->v.frags = 0;

      // END OF FIX: --- score resetted
      CALL_GAME_ENTITY(PLID, "player", VARS(BotEnt));
      infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer) (BotEnt);
      clientIndex = ENTINDEX(BotEnt);

      (*g_engfuncs.pfnSetClientKeyValue) (clientIndex, infobuffer, "model", "");
      (*g_engfuncs.pfnSetClientKeyValue) (clientIndex, infobuffer, "rate", "3500.000000");
      (*g_engfuncs.pfnSetClientKeyValue) (clientIndex, infobuffer, "cl_updaterate", "20");
      (*g_engfuncs.pfnSetClientKeyValue) (clientIndex, infobuffer, "cl_lw", "1");
      (*g_engfuncs.pfnSetClientKeyValue) (clientIndex, infobuffer, "cl_lc", "1");
      (*g_engfuncs.pfnSetClientKeyValue) (clientIndex, infobuffer, "tracker", "0");
      (*g_engfuncs.pfnSetClientKeyValue) (clientIndex, infobuffer, "cl_dlmax", "128");

	  if (RANDOM_LONG(0, 100) < 50) {
         (*g_engfuncs.pfnSetClientKeyValue) (clientIndex, infobuffer, "lefthand", "1");
	  } else {
         (*g_engfuncs.pfnSetClientKeyValue) (clientIndex, infobuffer, "lefthand", "0");
	  }

      (*g_engfuncs.pfnSetClientKeyValue) (clientIndex, infobuffer, "friends", "0");
      (*g_engfuncs.pfnSetClientKeyValue) (clientIndex, infobuffer, "dm", "0");
      (*g_engfuncs.pfnSetClientKeyValue) (clientIndex, infobuffer, "ah", "1");
      (*g_engfuncs.pfnSetClientKeyValue) (clientIndex, infobuffer, "_vgui_menus", "0");

      MDLL_ClientConnect(BotEnt, c_name, "127.0.0.1", ptr);

      // Pieter van Dijk - use instead of DispatchSpawn() - Hip Hip Hurray!
      MDLL_ClientPutInServer(BotEnt);
      BotEnt->v.flags |= FL_THIRDPARTYBOT;

      // initialize all the variables for this bot...

      // Retrieve Pointer
      pBot = &bots[index];

      // Set variables
      pBot->iIndex = index;
      pBot->bIsUsed = true;
      pBot->respawn_state = RESPAWN_IDLE;
      pBot->fCreateTime = gpGlobals->time;
      pBot->fKickTime = 0.0;
      pBot->name[0] = 0;        // name not set by server yet
      pBot->bot_money = 0;
      strcpy(pBot->skin, c_skin);
      pBot->pEdict = BotEnt;
      pBot->bStarted = false;   // hasn't joined game yet

      // CS Message IDLE..
      pBot->start_action = MSG_CS_IDLE;
      pBot->SpawnInit();
      pBot->bInitialize = false;        // don't need to initialize yet
      BotEnt->v.idealpitch = BotEnt->v.v_angle.x;
      BotEnt->v.ideal_yaw = BotEnt->v.v_angle.y;
      BotEnt->v.pitch_speed = BOT_PITCH_SPEED;
      BotEnt->v.yaw_speed = BOT_YAW_SPEED;
      pBot->bot_skill = skill;

      // Personality related
      pBot->ipHostage = 0;
      pBot->ipBombspot = 0;
      pBot->ipRandom = 0;
      pBot->ipTurnSpeed = 20;
      pBot->ipReplyToRadio = 0;
      pBot->ipCreateRadio = 0;
      pBot->ipHelpTeammate = 0;
      pBot->ipWalkWithKnife = 0;
      pBot->ipDroppedBomb = 0;
      pBot->ipCampRate = 0;
      pBot->ipChatRate = 0;
      pBot->ipFearRate = 0;
      pBot->ipHearRate = 0;

      pBot->played_rounds = 0;

      // Buy-personality related
      pBot->ipFavoPriWeapon = -1;
      pBot->ipFavoSecWeapon = -1;
      pBot->ipBuyFlashBang = 0;
      pBot->ipBuyGrenade = 0;
      pBot->ipBuySmokeGren = 0;
      pBot->ipBuyDefuseKit = 0;
      pBot->ipSaveForWeapon = 0;
      pBot->ipBuyArmour = 0;

      // here we set team
      if ((arg1 != NULL) && (arg1[0] != 0)) {
         pBot->iTeam = atoi(arg1);

         // and class
         if ((arg3 != NULL) && (arg3[0] != 0)) {
            pBot->bot_class = atoi(arg3);
         }
      }
      // Parsing name into bot identity
      INI_PARSE_BOTS(c_name, pBot);

      // return success
      return GAME_MSG_SUCCESS;
   }
}                               // CreateBot()

// Debug message
void REALBOT_PRINT(cBot * pBot, char *Function, char *msg) {
   // Message format:
   // Function name - [BOT NAME, BOT TEAM]: Message
   char cMessage[256];
   char team[9];
   char name[32];

   memset(team, 0, sizeof(team));       // clear
   memset(name, 0, sizeof(name));       // clear

   strcpy(team, "TERROR");      // t
   strcpy(name, "FUNCTION");

   if (pBot) {
      memset(name, 0, sizeof(name));    // clear
      strcpy(name, pBot->name); // copy name

      if (pBot->iTeam == 2)
         strcpy(team, "COUNTER");
   } else {
      strcpy(team, "NONE");
   }

   sprintf(cMessage, "RBPRINT->[%s '%s']-[Team %s] : %s\n", name, Function,
           team, msg);

   // print in console only when on debug print
   if (Game.bDebug)
      SERVER_PRINT(cMessage);

   // print this realbot message also in the LOG file.
   rblog(cMessage);
}  // REALBOT_PRINT()

// $Log: game.cpp,v $
// Revision 1.18  2004/09/07 15:44:34  eric
// - bumped build nr to 3060
// - minor changes in add2 (to add nodes for Bsp2Rbn utilities)
// - if compiled with USE_EVY_ADD, then the add2() function is used when adding
//   nodes based on human players instead of add()
// - else, it now compiles mostly without warnings :-)
//
// Revision 1.17  2004/07/30 15:02:30  eric
// - jumped to version 3057
// - improved readibility (wapen_tabel -> weapons_table) :-P
// - all Josh Borke modifications to the buying stuff:
//     * using a switch() instead of several if
//     * better buying code for shield and primary weapons
//     * new command 'debug pistols 0/1'
//
// Revision 1.16  2004/07/02 16:43:35  stefan
// - upped to build 3051
// - changed log() into rblog()
// - removed BOT.CFG code that interpets old RB V1.0 commands
// - neater respons of the RealBot console
// - more help from RealBot console (ie, type realbot server broadcast ... with no arguments it will tell you what you can do with this, etc)
// - removed message "bot personality loaded from file"
// - in overal; some cleaning done, no extra features added
//
// Revision 1.15  2004/07/01 18:09:46  stefan
// - fixed skill 10 bots not causing memory bugger on re-adding (respawning)
// - added extra check for respawning bots so auto-add function cannot crash
// - fixed 2 nitpicks pointed out on the forums
//
// Revision 1.14  2004/06/23 08:24:14  stefan
// - upped to build 3049
// - added swat behaviour (team leader assignment, radio response change and leaders command team-mates) - THIS IS EXPERIMENTAL AND DOES NOT ALWAYS WORK AS I WANT IT TO.
// - changed some conditions in nodemachine
// - sorry evy, still not added your new goals ;) will do next time, i promise
//
// Revision 1.13  2004/06/18 12:20:07  stefan
// - fixed another bug in chatting, CS 1.5 won't crash now
// - added some limit to bots searching for goals
//
// Revision 1.12  2004/06/17 21:23:23  stefan
// - fixes several connection problems with nodes. Going down from steep + crates (de_dust) PLUS going up/down from very steep slopes on as_oilrig.. 0wnage and thx to PMB and Evy
// - fixed chat bug in CS 1.6, its still CS 1.5 & CS 1.6 compatible though
//
// Revision 1.11  2004/06/13 20:08:21  stefan
// - 'bad score for goals' added
// - bmp dump info (Thanks Evy)
// - added 'realbot server players', so you can keep a server full at NR players at all times
// - slightly adjusted goal selection code
// - wander code disabled
// - lots of debug info introduced, do not use this source for REAL USAGE!
//
// Revision 1.10  2004/05/29 19:05:47  stefan
// - upped to BUILD 3044
// - removed several debug messages on screen
// - changed default 'chatrate (max sentences)' to 3
// - removed copyright notice, which is not valid due GPL license
//
// i know, nothing special :)
//
// Revision 1.9  2004/04/26 16:36:48  stefan
// changed:
// #elseif into #else to keep MSVC satisfied
//
// added:
// comment lines in game.cpp so you know what vars are what.
//
// Revision 1.8  2004/04/18 19:33:57  stefan
// - Restructured code a bit
//
// Revision 1.7  2004/04/18 17:39:19  stefan
// - Upped to build 2043
// - REALBOT_PRINT() works properly now
// - Log() works properly now
// - Clearing in dll.cpp of reallog.txt at dll init
// - Logging works now, add REALBOT_PRINT() at every point you want to log something.
//
