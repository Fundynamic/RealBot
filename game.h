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

#ifndef GAME_H
#define GAME_H


/**
  * GAME "handler" CLASS
  * COPYRIGHTED BY STEFAN HENDRIKS (C) 
  **/

// GAME MESSAGES
#define GAME_MSG_SUCCESS        0       // complete success
#define GAME_MSG_FAILURE        1       // complete failure

#define GAME_MSG_FAIL_SERVERFULL     2  // failure + reason
//#define GAME_MSG_WHATEVERYOUWANTTOPUTHERE

// SITUATIONS
#define GAME_YES                99
#define GAME_NO                 98

// BROADCASTING
#define BROADCAST_ROUND         0
#define BROADCAST_MAP           1
#define BROADCAST_KILLS_FULL   3        // killed, show full info (name+skill)
#define BROADCAST_KILLS_MIN    4        // killed, show min info (name)
#define BROADCAST_KILLS_NONE   5        // killed, show no information
#define BROADCAST_DEATHS_FULL   6       // died, show full info (name+skill)
#define BROADCAST_DEATHS_MIN    7       // died, show min info (name)
#define BROADCAST_DEATHS_NONE   8       // died, show no information

static const int MAX_BOTS = 32;
static const int MAX_NAME_LENGTH = 32;

// Debug messages for realbot
void REALBOT_PRINT(cBot * pBot, const char *Function, const char *msg);
void REALBOT_PRINT(const char *Function, const char *msg);

class cGame {
public:
   void Init();

   // ---------------------
   void LoadNames();
   void LoadCFG();
   void LoadBuyTable();

   // ---------------------
   void SelectName(char *name);
   bool NamesAvailable();
   void SetPlayingRounds(int iMin, int iMax);
   void SetNewRound(bool bState);
   void SetRoundTime(float fTime);
   void DetermineIfHostageRescueMap();

   // ---------------------
   char *RandomSentence();

   // ---------------------
   int GetMinPlayRounds();
   int GetMaxPlayRounds();

   bool NewRound();             // New round?
   float RoundTime();           // When did the round start? (time)

   int createBot(edict_t * pPlayer, const char *teamArg, const char *skillArg,
                 const char *modelArg, const char *nameArg);

   // ---------------------
   void UpdateGameStatus();     // Updates global game variables
   bool isC4Dropped();

   // ---------------------
   // public variables
   int iDefaultBotSkill;
   int iRandomMinSkill;
   int iRandomMaxSkill;
   int iOverrideBotSkill;       // Override "game botskill" with personality skill?
   float fWalkWithKnife;        // May bots walk with knife

   // Game related variables:
   Vector vDroppedC4;           // Dropped C4?
   bool bBombPlanted;           // Bomb planted?
   bool bHostageRescueMap;           // Hostage rescue map?
   bool bBombDiscovered;        // Is the bomb discovered?

   // Server vars
   int iVersionBroadcasting;    // 0 = every round , 1 = every new map
   int iKillsBroadcasting;      // 0 = full, 1 = min, 2 = none
   int iDeathsBroadcasting;     // 0 = full, 1 = min, 2 = none
   bool bInstalledCorrectly;    // false = RB is not in the correct directory
   bool bSpeechBroadcasting;    // true/false

   // DEBUG variables
   bool bDoNotShoot;            // Bots not shooting
   bool bDebug;                 // Print debug messages
   bool bEngineDebug;           // Print engine debug messages
   bool bPistols;               // 30/07/04 by Josh: bots will only use pistols

   // ChatEngine related
   int iMaxSentences;           // how many sentences may there be at max?
   int iProducedSentences;

private:
   // ---------------------
   // private variables
   char cSpeechSentences[16][80];
   int iAmountNames;
   char cBotNames[MAX_BOT_NAMES][BOT_NAME_LEN + 1];
   int iMinPlayRounds, iMaxPlayRounds;  // Min/Max playable rounds
   bool bNewRound;              // New round triggered?
   float fRoundTime;            // Round time
   float fUpdateGoalTimer;
};

#endif // GAME_H