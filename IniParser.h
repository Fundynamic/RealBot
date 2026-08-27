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

/**
  * INI PARSER
  * COPYRIGHTED BY STEFAN HENDRIKS (C) 
  **/

// Sections
#ifndef INIPARSER_H
#define INIPARSER_H

enum : std::int8_t
{
	INI_NONE = (-1),
	INI_SKILL = 0,		// Bot skill
	INI_WEAPON = 1,		// Bot weapon preference
	INI_GAME = 2,		// Bot general game behaviour
	INI_RADIO = 3,		// Bot radio behaviour
	INI_TEAM = 4,		// Bot team behaviour
	INI_PERSON = 5		// Bot person itself
};

enum : std::uint8_t
{
	INI_AREA = 10,
	INI_BLOCK = 11,
	INI_DEATHS = 12,
	INI_WELCOME = 13
};

// 'Weapon Sections' are the same as WEAPON ID in Counter-Strike.
// NOTE: For weapon_buy_table.iId!

// 'Words'
enum : std::int8_t
{
	WORD_NONE = (-1),
	WORD_WALK = 0,
	WORD_RUN = 1,
	WORD_SHOOT = 2,
	WORD_WAIT = 3,
	WORD_RADIO = 4
};

// BOTPERSONALITY.INI words
enum : std::uint8_t
{
	WORD_PRIWEAPON = 31,
	WORD_SECWEAPON = 32,
	WORD_SAVEFORWEAP = 33,
	WORD_GRENADE = 34,
	WORD_FLASHBANG = 35,
	WORD_SMOKEGREN = 36,
	WORD_DEFUSEKIT = 37,
	WORD_ARMOUR = 54,
	WORD_XOFFSET = 38,
	WORD_YOFFSET = 39,
	WORD_ZOFFSET = 40,
	WORD_BOTSKILL = 41,
	WORD_MAXREACTTIME = 42,
	WORD_MINREACTTIME = 43,
	WORD_TURNSPEED = 44,

	WORD_HOSTAGERATE = 45,
	WORD_BOMBSPOTRATE = 46,
	WORD_RANDOMRATE = 47,

	WORD_REPLYRADIO = 48,
	WORD_CREATERADIO = 49,

	WORD_HELPTEAM = 50,

	WORD_CAMPRATE = 51,
	WORD_CHATRATE = 52,
	WORD_WALKKNIFE = 53,

	WORD_FEARRATE = 55,
	WORD_HEARRATE = 56,

	WORD_DROPPEDBOMB = 57,

// AREA SHIT
	WORD_AREAX = 60,
	WORD_AREAY = 61,
	WORD_AREAZ = 62,

// CHAT
	WORD_SENTENCE = 67,
	WORD_WORD = 68,

// BUYTABLE.INI Words (arguments per weapon)
	WORD_PRIORITY = 5,
	WORD_PRICE = 6,
	WORD_MAXAMMO1 = 88,
	WORD_MAXAMMO2 = 89,
	WORD_ISLOT = 90,
	WORD_IPOSITION = 91,
	WORD_IFLAGS = 92,
	WORD_INDEX1 = 93,
	WORD_INDEX2 = 94
};


void INI_PARSE_BOTS(char cBotName[33], cBot * pBot);
void INI_PARSE_BUYTABLE();
void INI_PARSE_IAD();
void INI_PARSE_CHATFILE();

#endif // INIPARSER_H