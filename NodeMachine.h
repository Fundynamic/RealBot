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

/**
  * NODE MACHINE
  * COPYRIGHTED BY STEFAN HENDRIKS (C) 
  **/

#ifndef NODEMACHINE_H

// player sizes for path_connection_walkable
#define MAX_JUMPHEIGHT 60       // confirmed // 45 without crouching
#define MAX_FALLHEIGHT 130      // not confirmed (200 is to high, adjusted)
#define MAX_STAIRHEIGHT 18      // confirmed
#define HEAD_HEIGHT 72          // confirmed
#define ORIGIN_HEIGHT 36        // confirmed (?)
#define CROUCHED_HEIGHT 37      // confirmed
#define PLAYER_WIDTH 32         // confirmed (?)

// File version
// Version 1.0
#define FILE_NODE_VER1   1
#define FILE_EXP_VER1    1

// Version 2.0
#define FILE_NODE_VER2   2
#define FILE_EXP_VER2    2

// Node bits (for navigational performance)
#define BIT_LADDER	(1 << 0)
#define BIT_WATER	(1 << 1)
#define BIT_JUMP	(1 << 2)
#define BIT_DUCK	(1 << 3)

// Path flags
#define PATH_DANGER     39      // Picked a random number here
#define PATH_CONTACT    37      // w0h00
#define PATH_NONE       32      // - rushing
#define PATH_CAMP       31      // camp path

// Visibility flags
#define VIS_INVALID		96      // BERKED
#define VIS_UNKNOWN     97
#define VIS_VISIBLE     98
#define VIS_BLOCKED     99

// Goal types & info
#define MAX_GOALS		75

#define GOAL_SPAWNCT	1
#define GOAL_SPAWNT		2
#define GOAL_BOMBSPOT	3
#define GOAL_BOMB		4       // updates all the time
#define GOAL_HOSTAGE	5       // updates all the time
#define GOAL_RESCUEZONE 6       // rescue zone
#define GOAL_CONTACT	7       // zones where teams often have contact
#define GOAL_IMPORTANT	8
#define GOAL_VIP		9       // as_ maps VIP starting point
#define GOAL_VIPSAFETY	10      // as_ maps VIP safety zone
#define GOAL_ESCAPEZONE	11      // es_ maps escape zone
#define GOAL_WEAPON		12      // pre-dropped weapons like in awp_map
#define GOAL_NONE		99

// Node costs
#define NODE_DANGER		 8192   // Value
#define NODE_DANGER_STEP 0.5    // Step to take to get dangerous
#define NODE_DANGER_DIST 512.0  // Distance

// Node contact costs
#define NODE_CONTACT      8192
#define NODE_CONTACT_STEP 0.2
#define NODE_CONTACT_DIST 128

// Node boundries
#define MAX_NODES		4096
#define MAX_NEIGHBOURS	16
#define NODE_ZONE		45
#define MAX_PATH_NODES	MAX_NODES

// Max troubled node connections we remember
#define MAX_TROUBLE     100

// Meridian stuff
#define SIZE_MEREDIAN	256
#define MAX_MEREDIANS	16384 / SIZE_MEREDIAN   // Size of HL map devided by
#define NODES_MEREDIANS	120     // EVY: higher number, number of nodes per meredian
//#define NODES_MEREDIANS       80      // (size meredian / zone (~6) times 2 (surface) , rounded to 80

// Pathfinder
#define OPEN			1   // open
#define CLOSED		    2   // closed, but may open

const unsigned long g_iMaxVisibilityByte = (MAX_NODES * MAX_NODES) / 8;

// doors (doors.cpp) HLSDK
#define SF_DOOR_ROTATE_Y			0
#define	SF_DOOR_START_OPEN			1
#define SF_DOOR_ROTATE_BACKWARDS	2
#define SF_DOOR_PASSABLE			8
#define SF_DOOR_ONEWAY				16
#define	SF_DOOR_NO_AUTO_RETURN		32
#define SF_DOOR_ROTATE_Z			64
#define SF_DOOR_ROTATE_X			128
#define SF_DOOR_USE_ONLY			256     // door must be opened by player's use button.
#define SF_DOOR_NOMONSTERS			512     // Monster can't open
#define SF_DOOR_SILENT				0x80000000


// Player information on map
typedef struct {
   Vector vPrevPos;             // Previous Position
   int iNode;                   // Previous Node
}
tPlayer;

// Astar Node informaiton
typedef struct {
   int state;                   // OPEN/CLOSED
   double cost;                 // Cost
   double danger;
   int parent;                  // Who opened this node?
}
tNodestar;

// Additional Node Information
typedef struct {
   float fDanger[2];            // Danger information (0.0 - no danger, 1.0 dangerous)
   float fContact[2];           // How many times have contact with enemy (0.0 none, 1.0 , a lot)
}
tInfoNode;

typedef struct {
   int iNodes[NODES_MEREDIANS];
}
tMeredian;

// Trouble connections
typedef struct {
   int iFrom;                   // From NODE
   int iTo;                     // To NODE
   int iTries;                  // How many times we had trouble with this connection
}
tTrouble;

// Node
typedef struct {
   Vector origin;               // Node origin
   int iNeighbour[MAX_NEIGHBOURS];      // Neighbour reachable nodes
   int iNodeBits;
}
tNode;

// Goal Node information
typedef struct {
   edict_t *pGoalEdict;         // edict of goal
   int iNode;                   // index of node attached to it
   int iType;                   // type of goal
   int iChecked;                // many times checked/visited?
   int iBadScore;               // bad score for a node (when it seems to be unreachable?)
}
tGoal;

// CLASS: NodeMachine
class cNodeMachine {
public:
   // -----------------
   int add(Vector vOrigin, int iType, edict_t * pEntity);
   int Reachable(const int iStart, const int iEnd);
   int add2(Vector vOrigin, int iType, edict_t * pEntity);
   int getCloseNode(Vector vOrigin, float fDist, edict_t * pEdict);    // returns a close node

   // -----------------
   bool add_neighbour_node(int iNode, int iToNode);
   bool remove_neighbour_node(int iNode, int iRemoveNode);
   bool remove_neighbour_nodes(int iNode);
   int neighbour_node(tNode Node);
   int is_neighbour_node(tNode Node, int iNode);

   // -----------------
   void init();                 // Init (info)nodes
   void save();                 // Save nodes on disk
   void load();                 // Load nodes on disk
   void save_important();

   // -----------------
   Vector node_vector(int iNode);

   // -----------------
   int TroubleExists(int iFrom, int iTo);
   bool AddTrouble(int iFrom, int iTo);
   bool TroubleIsTrouble(int iFrom, int iTo);
   void IncreaseTrouble(int iFrom, int iTo);
   bool RemoveTrouble(int iFrom, int iTo);

   // -----------------
   void goals();                // find new goals and attach them to the nodes
   int goal_from_node(int iNode);
   void goal_reset();
   void ClearImportantGoals();
   bool goal_exists(edict_t * pEdict);
   void goal_add(edict_t * pEdict, int iType, Vector vVec);
   int goal_hostage(cBot * pBot);
   int node_goal(int iType);    // return a node close to a iType goal (random)
   bool node_float(Vector vOrigin, edict_t * pEdict);
   bool node_on_crate(Vector vOrigin, edict_t * pEdict);

   int node_dangerous(int iTeam, Vector vOrigin, float fMaxDistance);
   int node_look_camp(Vector vOrigin, int iTeam, edict_t * pEdict);

   // -----------------
   void danger(int iNode, int iTeam);   // Make spot dangerous
   void scale_danger();

   // -----------------
   void contact(int iNode, int iTeam);  // Add contact area
   void scale_contact();

   // -----------------
   void experience_save();
   void experience_load();

   // -----------------
   int node_cover(int iFrom, int iTo, edict_t * pEdict);
   int node_look_at_hear(int iFrom, int iTo, edict_t * pEdict);
   int node_camp(Vector vOrigin, int iTeam);
   void vis_calculate(int iFrom);

   // -----------------
   void wander_think(cBot * pBot, float moved_distance);        // wander around

   // -----------------
   void path(int iFrom, int iTo, int iPath, cBot * pBot, int iFlags);   // know the path
   void path_draw(edict_t * pEntity);   // draw the path
   void path_walk(cBot * pBot, float moved_distance);   // walk the path
   void path_think(cBot * pBot, float moved_distance);  // think about paths
   void path_clear(int iPathId);
   int NodeFromPath(int iBot, int iIndex);

   // -----------------
   void VectorToMeredian(Vector vOrigin, int *iX, int *iY);     // Input: origin, output X and Y Meredians
   void AddToMeredian(int iX, int iY, int iNode);

   // -----------------
   void draw(edict_t * pEntity);        // Draw nodes
   void connections(edict_t * pEntity); // Draw neighbours

   // -----------------
   void players_plot();         // Players plot around!
   void init_players();         // Initialize players (dll load)
   void init_round();           // Initialize on round start

   // -------------------
   // From cheesemonster:
   int GetVisibilityFromTo(int iFrom, int iTo); // BERKED
   void ClearVisibilityTable(void);
   void SetVisibilityFromTo(int iFrom, int iTo, bool bVisible);
   void FreeVisibilityTable(void);

   // Some debugging by EVY
   void dump_goals(void);
   void dump_path(int iBot, int ThisNode);
   void Draw(void);

private:
   tNode Nodes[MAX_NODES];     // Nodes
   tInfoNode InfoNodes[MAX_NODES];      // Info for Nodes
   tPlayer Players[32];         // Players to keep track off
   tGoal Goals[MAX_GOALS];      // Goals in the game
   tMeredian Meredians[MAX_MEREDIANS][MAX_MEREDIANS];   // Meredian lookup search for Nodes
   int iPath[32][MAX_PATH_NODES];       // 32 bots, with max waypoints paths
   int iMaxUsedNodes;
   //byte        iVisTable[MAX_NODES][MAX_NODES];
   byte iVisChecked[MAX_NODES];
   unsigned char *cVisTable;
   tTrouble Troubles[MAX_TROUBLE];
   void FindMinMax(void);
   void MarkAxis(void);
   void MarkMeredians(void);
   void PlotNodes(int NeighbourColor, int NodeColor);
   void PlotPaths(int Tcolor, int CTcolor);
   void PlotGoals(int GoalColor);
};

#endif
