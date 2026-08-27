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

#include <array>
#include <string>
#include <vector>

  /**
	* GAME "handler" CLASS
	* COPYRIGHTED BY STEFAN HENDRIKS (C)
	**/

	// GAME MESSAGES
enum : std::uint8_t
{
	GAME_MSG_SUCCESS = 0,       // complete success
	GAME_MSG_FAILURE = 1,       // complete failure

	GAME_MSG_FAIL_SERVERFULL = 2,  // failure + reason
	//#define GAME_MSG_WHATEVERYOUWANTTOPUTHERE

	// SITUATIONS
	GAME_YES = 99,
	GAME_NO = 98
};

// BROADCASTING
enum : std::uint8_t
{
	BROADCAST_ROUND = 0,
	BROADCAST_MAP = 1,
	BROADCAST_KILLS_FULL = 3,       // killed, show full info (name+skill)
	BROADCAST_KILLS_MIN = 4,        // killed, show min info (name)
	BROADCAST_KILLS_NONE = 5,       // killed, show no information
	BROADCAST_DEATHS_FULL = 6,      // died, show full info (name+skill)
	BROADCAST_DEATHS_MIN = 7,       // died, show min info (name)
	BROADCAST_DEATHS_NONE = 8       // died, show no information
};

static constexpr int MAX_BOTS = 32;
static constexpr int MAX_NAME_LENGTH = 32;

// Debug messages for realbot
void REALBOT_PRINT(cBot* pBot, const char* Function, const char* msg);
void REALBOT_PRINT(const char* Function, const char* msg);

class cGame {
public:
	void Init();
	void InitNewRound();

	// ---------------------
	void LoadNames();
	static void LoadCFG();
	static void LoadBuyTable();

	// ---------------------
	std::string SelectName() const;
	bool NamesAvailable() const;
	void SetPlayingRounds(int iMin, int iMax);
	void SetNewRound(bool bState);
	void resetRoundTime();
	void SetRoundTime(float fTime);
	static void DetermineMapGoal();

	// ---------------------
	const char* RandomSentence() const;

	// ---------------------
	int GetMinPlayRounds() const;
	int GetMaxPlayRounds() const;

	bool NewRound() const;             // New round?
	float getRoundStartedTime() const;           // When did the round start? (time)
	float getRoundTimeElapsed() const;           // difference between now and round started time

	int createBot(edict_t* pPlayer, const char* teamArg, const char* skillArg,
		const char* modelArg, const char* nameArg) const;

	// ---------------------
	void UpdateGameStatus();     // Updates global game variables
	bool isC4Dropped() const;
	bool isPlantedC4Discovered() const;

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
	bool bHostageRescueMap;      // Hostage rescue map? (CS_...)
	bool bHostageRescueZoneFound; // Is any rescue zone found? (CS_...)
	bool bBombPlantMap;          // Bomb plant map? (DE_...)
	Vector vPlantedC4;           // Is the bomb discovered?

	// Server vars
	int iVersionBroadcasting;    // 0 = every round , 1 = every new map
	int iKillsBroadcasting;      // 0 = full, 1 = min, 2 = none
	int iDeathsBroadcasting;     // 0 = full, 1 = min, 2 = none
	bool bInstalledCorrectly;    // false = RB is not in the correct directory
	bool bSpeechBroadcasting;    // true/false

	// DEBUG variables
	bool bDoNotShoot;            // Bots not shooting
	int  bDebug;                 // Print debug messages (if > -1, it prints messages for bot index...)
	int  messageVerbosity;       // Print debug messages (verbosity)
	bool bEngineDebug;           // Print engine debug messages
	bool bPistols;               // 30/07/04 by Josh: bots will only use pistols

	// ChatEngine related
	int iMaxSentences;           // how many sentences may there be at max?
	int iProducedSentences;

private:
	// ---------------------
	// private variables
	std::array<std::string, 16> cSpeechSentences;
	int iAmountNames = 0;
	std::vector<std::string> cBotNames;
	int iMinPlayRounds = 0, iMaxPlayRounds = 0;  // Min/Max playable rounds
	bool bNewRound = false;              // New round triggered?
	float fRoundTime = 0.0f;            // Round time
	float fUpdateGoalTimer = 0.0f;
};

#endif // GAME_H