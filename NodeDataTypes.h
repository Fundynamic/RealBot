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
	* NODE MACHINE data types
	* COPYRIGHTED BY STEFAN HENDRIKS (C)
	**/

#ifndef NODEDATATYPES_H
#define NODEDATATYPES_H

#include <array>
#include <cstdint>

// player sizes for path_connection_walkable
enum : std::uint8_t
{
	MAX_JUMPHEIGHT = 60, // confirmed // 45 without crouching
	MAX_FALLHEIGHT = 130, // not confirmed (200 is to high, adjusted)
	MAX_STAIRHEIGHT = 18, // confirmed
	HEAD_HEIGHT = 72, // confirmed
	ORIGIN_HEIGHT = 36, // confirmed (?)
	CROUCHED_HEIGHT = 37, // confirmed
	PLAYER_WIDTH = 32          // confirmed (?)
};

// File version
// Version 1.0
enum : std::uint8_t
{
	FILE_NODE_VER1 = 1,
	FILE_EXP_VER1 = 1
};

// Version 2.0
enum : std::uint8_t
{
	FILE_NODE_VER2 = 2,
	FILE_EXP_VER2 = 2
};

// Version 3.0 (sparse serialization - only used nodes/data are stored)
enum : std::uint8_t
{
	FILE_NODE_VER3 = 3,
	FILE_EXP_VER3 = 3
};

// Node bits (for navigational performance)
enum : std::uint8_t
{
	BIT_LADDER = (1 << 0),
	BIT_WATER = (1 << 1),
	BIT_JUMP = (1 << 2),
	BIT_DUCK = (1 << 3),
	BIT_DUCKJUMP = (1 << 4)
};

// Path flags
enum : std::uint8_t
{
	PATH_DANGER = 39, // Picked a random number here
	PATH_CONTACT = 37, // w0h00
	PATH_NONE = 32, // - rushing
	PATH_CAMP = 31      // camp path
};

// Visibility flags
enum : std::uint8_t
{
	VIS_INVALID = 96, // BERKED
	VIS_UNKNOWN = 97,
	VIS_VISIBLE = 98,
	VIS_BLOCKED = 99
};

// Goal types & info
constexpr int MAX_GOALS = 75;

// Node types / goal types
enum : std::uint8_t
{
	GOAL_SPAWNCT = 1,
	GOAL_SPAWNT = 2,
	GOAL_BOMBSPOT = 3,
	GOAL_BOMB = 4, // updates all the time
	GOAL_HOSTAGE = 5, // updates all the time
	GOAL_RESCUEZONE = 6, // rescue zone (for hostages)
	GOAL_CONTACT = 7, // zones where teams often have contact
	GOAL_IMPORTANT = 8, // important goals
	GOAL_VIP = 9, // as_ maps VIP starting point
	GOAL_VIPSAFETY = 10, // as_ maps VIP safety zone
	GOAL_ESCAPEZONE = 11, // es_ maps escape zone
	GOAL_WEAPON = 12, // pre-dropped weapons like in awp_map
	GOAL_NONE = 99
};

// Node costs
enum : std::uint16_t
{
	NODE_DANGER = 8192    // Value
};

constexpr float NODE_DANGER_STEP = 0.5f;    // Step to take to get dangerous;
constexpr float NODE_DANGER_DIST = 512.0f;  // Distance;

// Node contact costs
enum : std::uint16_t
{
	NODE_CONTACT = 8192
};

constexpr float NODE_CONTACT_STEP = 0.2f;

enum : std::uint8_t
{
	NODE_CONTACT_DIST = 128
};

// Node boundries
enum : std::uint8_t
{
	NODE_ZONE = 45 // Maybe increase it to 128 or 144 to reduce the amount of excess nodes [APG]RoboCop[CL]
};

constexpr int MAX_NODES = 4096;
constexpr int MAX_NEIGHBOURS = 16; // Maybe increase it to 128 or 144 to reduce the amount of excess nodes [APG]RoboCop[CL]

#define MAX_PATH_NODES    MAX_NODES

// Max troubled node connections we remember
constexpr int MAX_TROUBLE = 100;

// Meridian stuff
enum : std::uint16_t
{
	SIZE_MEREDIAN = 256,
	MAP_MAX_SIZE = 16384
};

#define MAX_MEREDIANS    (MAP_MAX_SIZE / SIZE_MEREDIAN)   // Size of HL map divided by SIZE of a meridian to evenly spread
constexpr int MAX_NODES_IN_MEREDIANS = 120;     // EVY: higher number, number of nodes per meredian;
//#define MAX_NODES_IN_MEREDIANS       80      // (size meredian / zone (~6) times 2 (surface) , rounded to 80

// A* defines OPEN/CLOSED lists
enum : std::uint8_t
{
	OPEN = 1, // open, can still re-evaluate
	CLOSED = 2, // closed, do nothing with it
	AVAILABLE = 3   // available, may open
};

constexpr unsigned long g_iMaxVisibilityByte = MAX_NODES * MAX_NODES / 8;

// doors (doors.cpp) HLSDK
enum : std::uint16_t
{
	SF_DOOR_ROTATE_Y = 0,
	SF_DOOR_START_OPEN = 1,
	SF_DOOR_ROTATE_BACKWARDS = 2,
	SF_DOOR_PASSABLE = 8,
	SF_DOOR_ONEWAY = 16,
	SF_DOOR_NO_AUTO_RETURN = 32,
	SF_DOOR_ROTATE_Z = 64,
	SF_DOOR_ROTATE_X = 128,
	SF_DOOR_USE_ONLY = 256, // door must be opened by player's use button.
	SF_DOOR_NOMONSTERS = 512     // Monster can't open
};

constexpr unsigned SF_DOOR_SILENT = 0x80000000;

// Player information on map
typedef struct {
	Vector vPrevPos;             // Previous Position
	int iNode;                   // Previous Node
}
tPlayer;

// Astar Node informaiton
typedef struct tNodestar {
	int state;                   // OPEN/CLOSED
	int parent;                  // Who opened this node?
	float cost;                 // Cost
	double danger;

	// Comparison operator for priority queue
	bool operator<(const tNodestar& other) const {
		return cost > other.cost; // Note: Use '>' for min-heap (lower cost has higher priority)
	}
} tNodestar;

// Additional Node Information
typedef struct {
	float fDanger[2];            // Danger information (0.0 - no danger, 1.0 dangerous). Indexed per team (T/CT)
	float fContact[2];           // How many times have contact with enemy (0.0 none, 1.0 , a lot)
}
tInfoNode;

typedef struct {
	int iNodes[MAX_NODES_IN_MEREDIANS];
}
tMeredian;

// Trouble connections
typedef struct {
	int iFrom;                   // From NODE
	int iTo;                     // To NODE
	int iTries;                  // How many times we had trouble with this connection
}
tTrouble;

// Node (stored in RBN file, do not change casually)
typedef struct {
	Vector origin;                   // Node origin
	std::array<int, MAX_NEIGHBOURS> iNeighbour;  // Reachable nodes for this node
	int iNodeBits;
	int index;
}
tNode;

// Goal Node information
typedef struct {
	edict_t* pGoalEdict;         // edict of goal
	int iNode;                   // index of node attached to it
	int iType;                   // type of goal
	int iChecked;                // many times checked/visited?
	int iBadScore;               // bad score for a node (when it seems to be unreachable?)
	int index;                   // the index in the Goals[] array
	char name[32];               // name of goal
}
tGoal;

#endif // NODEDATATYPES_H
