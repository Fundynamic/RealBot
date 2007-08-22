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
  *	NODE MACHINE
  * COPYRIGHTED BY STEFAN HENDRIKS (C) 2003-2004
  **/

#include <string.h>
#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>
#include <entity_state.h>

// malloc stuff?
#include "malloc.h"
#include "stdlib.h"
// ---

#include "bot.h"
#include "IniParser.h"
#include "bot_weapons.h"
#include "game.h"
#include "bot_func.h"

#include "NodeMachine.h"

tNodestar astar_list[MAX_NODES];
extern edict_t *pHostEdict;
extern cGame Game;
extern cBot bots[32];
extern int draw_nodepath;
//---------------------------------------------------------
//CODE: CHEESEMONSTER

int                             // BERKED
cNodeMachine::GetVisibilityFromTo(int iFrom, int iTo) {
   // prevent negative indexes on iVisChecked below -- BERKED
   if (iFrom < 0 || iFrom > MAX_NODES || iTo < 0 || iTo > MAX_NODES) {
      return VIS_INVALID;
   }
   // -- BY STEFAN --
   if (iVisChecked[iFrom] == 0)
      return VIS_UNKNOWN;       // we have no clue
   // -- END --

   // was int

   // work out the position
   long iPosition = (iFrom * MAX_NODES) + iTo;

   long iByte = (int) (iPosition / 8);
   unsigned int iBit = iPosition % 8;

   if (iByte < g_iMaxVisibilityByte) {
      // Get the Byte that this is in
      unsigned char *ToReturn = (cVisTable + iByte);
      // get the bit in the byte
      return ((*ToReturn & (1 << iBit)) > 0) ? VIS_VISIBLE : VIS_BLOCKED;       // BERKED
   }

   return VIS_BLOCKED;          // BERKED
}

void
cNodeMachine::SetVisibilityFromTo(int iFrom, int iTo, bool bVisible) {
   // prevent negative indexes on iVisChecked below, fixing SEGV -- BERKED
   if (iFrom < 0 || iFrom > MAX_NODES || iTo < 0 || iTo > MAX_NODES) {
      return;
   }
   // -- STEFAN --
   iVisChecked[iFrom] = 1;      // WE HAVE CHECKED THIS ONE
   // -- END --

   // was int
   long iPosition = (iFrom * MAX_NODES) + iTo;

   long iByte = (int) (iPosition / 8);
   unsigned int iBit = iPosition % 8;

   if (iByte < g_iMaxVisibilityByte) {
      unsigned char *ToChange = (cVisTable + iByte);

      if (bVisible)
         *ToChange |= (1 << iBit);
      else
         *ToChange &= ~(1 << iBit);
   }
}

void cNodeMachine::ClearVisibilityTable(void) {
   if (cVisTable)
      memset(cVisTable, 0, g_iMaxVisibilityByte);
}

void cNodeMachine::FreeVisibilityTable(void) {
   if (cVisTable)
      free(cVisTable);
}

//---------------------------------------------------------

// Initialization of node machine
void cNodeMachine::init() {
   iMaxUsedNodes = 0;

   for (int i = 0; i < MAX_NODES; i++) {
      // --- nodes
      Nodes[i].origin = Vector(9999, 9999, 9999);
      for (int n = 0; n < MAX_NEIGHBOURS; n++)
         Nodes[i].iNeighbour[n] = -1;

      // No bits set
      Nodes[i].iNodeBits = 0;

      // --- info nodes
      for (int d = 0; d < 2; d++) {
         InfoNodes[i].fDanger[d] = 0.0;

      }

   }

   // Init trouble
   for (int t = 0; t < MAX_TROUBLE; t++) {
      Troubles[t].iFrom = -1;
      Troubles[t].iTo = -1;
      Troubles[t].iTries = -1;
   }

   // Init goals
   for (int g = 0; g < MAX_GOALS; g++) {
      Goals[g].iNode = -1;
      Goals[g].pGoalEdict = NULL;
      Goals[g].iType = GOAL_NONE;
      Goals[g].iChecked = 0;
      Goals[g].iBadScore = 0;   // no bad score at init
   }

   // Init paths
   for (int p = 0; p < 32; p++)
      path_clear(p);

   /*
      // Init VisTable
      for (int iVx=0; iVx < MAX_NODES; iVx++)
      for (int iVy=0; iVy < MAX_NODES; iVy++)
      iVisTable[iVx][iVy] = NULL;
    */

   // Init VisTable
   for (int iVx = 0; iVx < MAX_NODES; iVx++)
      iVisChecked[iVx] = 0;     // not checked yet

   // CODE: From cheesemonster
   unsigned long iSize = g_iMaxVisibilityByte;

   //create a heap type thing...
   FreeVisibilityTable();       // 16/07/04 - free it first
   cVisTable = (unsigned char *) malloc(iSize);
   memset(cVisTable, 0, iSize);
   ClearVisibilityTable();
   // END:

   // Init Meredians
   for (int iMx = 0; iMx < MAX_MEREDIANS; iMx++)
      for (int iMy = 0; iMy < MAX_MEREDIANS; iMy++)
         for (int iNode = 0; iNode < NODES_MEREDIANS; iNode++)
            Meredians[iMx][iMy].iNodes[iNode] = -1;
}

int cNodeMachine::TroubleExists(int iFrom, int iTo) {
   int t;
   for (t = 0; t < MAX_TROUBLE; t++)
      if (Troubles[t].iFrom == iFrom && Troubles[t].iTo == iTo)
         return t;

   return -1;
}

bool cNodeMachine::AddTrouble(int iFrom, int iTo) {
   if (TroubleExists(iFrom, iTo) > -1)
      return false;             // could not create a new one

   int iNew = -1;
   int t;

   for (t = 0; t < MAX_TROUBLE; t++)
      if (Troubles[t].iFrom < 0 || Troubles[t].iTo < 0) {
         iNew = t;
         break;
      }

   if (iNew < 0)
      return false;

   Troubles[iNew].iFrom = iFrom;
   Troubles[iNew].iTo = iTo;
   Troubles[iNew].iTries = 1;

   return true;
}

bool cNodeMachine::TroubleIsTrouble(int iFrom, int iTo) {
   // Max amount of trouble we may have in one game is 4 times.
   int t = TroubleExists(iFrom, iTo);

   if (t < 0)
      return false;

   if (Troubles[t].iTries > 3)
      return true;

   return false;
}

void cNodeMachine::IncreaseTrouble(int iFrom, int iTo) {
   int t = TroubleExists(iFrom, iTo);
   if (t < 0)
      return;

   Troubles[t].iTries++;
}

bool cNodeMachine::RemoveTrouble(int iFrom, int iTo) {
   int t = TroubleExists(iFrom, iTo);

   if (t < 0)
      return false;

   Troubles[t].iFrom = -1;
   Troubles[t].iTo = -1;
   Troubles[t].iTries = -1;

   return true;
}

void cNodeMachine::path_clear(int iPathId) {
   // clear path
   for (int i = 0; i < MAX_NODES; i++)
      iPath[iPathId][i] = -1;

   // cleared
}

// Return
Vector cNodeMachine::node_vector(int iNode) {
   if (iNode > -1)
      return Nodes[iNode].origin;

   return Vector(9999, 9999, 9999);
}

// Input: Vector, Output X and Y Meredians
void cNodeMachine::VectorToMeredian(Vector vOrigin, int *iX, int *iY) {
   // Called for lookupt and for storing
   float iCoordX = vOrigin.x + 8192.0;  // map height (converts from - to +)
   float iCoordY = vOrigin.y + 8192.0;  // map width (converts from - to +)

   // Meredian:
   iCoordX = iCoordX / SIZE_MEREDIAN;
   iCoordY = iCoordY / SIZE_MEREDIAN;

   *iX = (int) iCoordX;
   *iY = (int) iCoordY;
}

void cNodeMachine::AddToMeredian(int iX, int iY, int iNode) {
   int index = -1;
   for (int i = 0; i < NODES_MEREDIANS; i++)
      if (Meredians[iX][iY].iNodes[i] < 0) {
         index = i;
         break;
      }

   if (index < 0)
      return;

   Meredians[iX][iY].iNodes[index] = iNode;
}

// Does the node float?
bool cNodeMachine::node_float(Vector vOrigin, edict_t * pEdict) {
   TraceResult tr;
   Vector tr_end = vOrigin - Vector(0, 0, (ORIGIN_HEIGHT * 1.2));

   //Using TraceHull to detect de_aztec bridge and other entities. (skill self)
   //UTIL_TraceHull(vOrigin, tr_end, ignore_monsters, point_hull, pEdict->v.pContainingEntity, &tr);
   if (pEdict)
      UTIL_TraceHull(vOrigin, tr_end, ignore_monsters, human_hull,
                     pEdict->v.pContainingEntity, &tr);
   else
      UTIL_TraceHull(vOrigin, tr_end, ignore_monsters, human_hull, NULL,
                     &tr);

   // if nothing hit: floating too high, return false
   if (tr.flFraction >= 1.0)
      return true;              // floating

   // *NOTE*: Actually this check should not be nescesary!
   if (tr.flFraction < 1.0)
      if (tr.pHit == pEdict)
         return true;

   // if inside wall: return false
   if (tr.fStartSolid == 1) {
      // todo: make sure the node does not start within this wall
      return false;             // not floating
   }

   return false;                // not floating
}

// Does the node stand on a crate? or a steep slope?
bool cNodeMachine::node_on_crate(Vector vOrigin, edict_t * pEdict) {
   TraceResult tr;
   Vector tr_end = vOrigin - Vector(0, 0, (ORIGIN_HEIGHT * 1.2));

   //Using TraceHull to detect de_aztec bridge and other entities. (skill self)
   if (pEdict)
      UTIL_TraceHull(vOrigin, tr_end, ignore_monsters, human_hull,
                     pEdict->v.pContainingEntity, &tr);
   else
      UTIL_TraceHull(vOrigin, tr_end, ignore_monsters, human_hull, NULL,
                     &tr);

   // if nothing hit: floating too high, return false
   if (tr.flFraction >= 1.0)
      return false;             // floating

   // *NOTE*: Actually this check should not be nescesary!
   if (tr.flFraction < 1.0) {
      //  if (tr.pHit == pEdict)
      //  return false;

      // thanks a million to PMB , so i know what the fucking difference
      // is between something straight (crate) and steep... although i have
      // no clue yet how to compute these values myself.
      if ( /*tr.vecPlaneNormal.z >= 0.7 && */ tr.vecPlaneNormal.z == 1.0) {
         return true;
      }
   }


   return false;                // not floating
}

// Find close node
int cNodeMachine::close(Vector vOrigin, float fDist, edict_t * pEdict) {
   // REDO: Need faster method to find a node
   // TOADD: For secure results all nodes should be checked to figure out the real
   //        'closest' node.

   int iNodeOutOfFOV = -1;

   // Use Meredians to search for nodes
   int iX, iY;
   VectorToMeredian(vOrigin, &iX, &iY);

   if (iX > -1 && iY > -1) {
      // Search in this meredian
      for (int i = 0; i < NODES_MEREDIANS; i++)
         if (Meredians[iX][iY].iNodes[i] > -1) {
            int iNode = Meredians[iX][iY].iNodes[i];
            if (func_distance(vOrigin, Nodes[iNode].origin) < fDist) {
               TraceResult tr;
               Vector vNode = Nodes[iNode].origin;

               //Using TraceHull to detect de_aztec bridge and other entities.
               //DONT_IGNORE_MONSTERS, we reached it only when there are no other bots standing in our way!
               //UTIL_TraceHull(vOrigin, vNode, dont_ignore_monsters, point_hull, pEdict, &tr);
               //UTIL_TraceHull(vOrigin, vNode, dont_ignore_monsters, human_hull, pEdict, &tr);
               if (pEdict)
                  UTIL_TraceHull(vOrigin, vNode, dont_ignore_monsters,
                                 head_hull, pEdict->v.pContainingEntity,
                                 &tr);
               else
                  UTIL_TraceHull(vOrigin, vNode, dont_ignore_monsters,
                                 head_hull, NULL, &tr);

               // if nothing hit:
               if (tr.flFraction >= 1.0) {
                  if (pEdict != NULL) {
                     if (FInViewCone(&vNode, pEdict)
                           && FVisible(vNode, pEdict))
                        return iNode;
                     else
                        iNodeOutOfFOV = iNode;

                  } else
                     return iNode;
               }
               //return iNode;
            }
         }

   }

   /*
    for (int i=0; i < MAX_NODES; i++)
    {
   	if (Nodes[i].origin != Vector(9999,9999,9999))
   	  if (func_distance(vOrigin, Nodes[i].origin) < fDist)
   		return i; // do not search further.
    }
    */

   return iNodeOutOfFOV;        // fail
}

// Adds a neighbour connection to a node ID
bool cNodeMachine::add_neighbour_node(int iNode, int iToNode) {
   if (iNode < 0)
      return false;

   int iNeighbourId = neighbour_node(Nodes[iNode]);
   if (iNeighbourId > -1) {
      Nodes[iNode].iNeighbour[iNeighbourId] = iToNode;
      return true;
   }

   return false;
}

// Removes a neighbour connection from a node ID
bool cNodeMachine::remove_neighbour_node(int iNode, int iRemoveNode) {
   if (iNode < 0)
      return false;

   // Does the node connection exist already?
   if (is_neighbour_node(Nodes[iNode], iRemoveNode)) {
      // Find the connection and remove it
      int i = 0;
      for (i = 0; i < MAX_NEIGHBOURS; i++)
         if (Nodes[iNode].iNeighbour[i] == iRemoveNode) {
            Nodes[iNode].iNeighbour[i] = -1;
            return true;        // success
         }
   }

   return false;
}

// Removes ALL neighbour connections on iNode
bool cNodeMachine::remove_neighbour_nodes(int iNode) {
   if (iNode < 0)
      return false;

   int i = 0;
   for (i = 0; i < MAX_NEIGHBOURS; i++)
      Nodes[iNode].iNeighbour[i] = -1;

   return true;
}

// returns the next free 'neighbour id' for that node
int cNodeMachine::neighbour_node(tNode Node) {
   for (int i = 0; i < MAX_NEIGHBOURS; i++)
      if (Node.iNeighbour[i] < 0)
         return i;

   return -1;
}

// Checks on tNode if iNode is already a neighbour
int cNodeMachine::is_neighbour_node(tNode Node, int iNode) {
   for (int i = 0; i < MAX_NEIGHBOURS; i++)
      if (Node.iNeighbour[i] == iNode)
         return true;

   return false;
}

// Return the NODE id from bot path on Index NR
int cNodeMachine::NodeFromPath(int iBot, int iIndex) {
   if ((iIndex > -1) && (iBot > -1))
      return iPath[iBot][iIndex];

   return -1;                   // nothing found
}

// Compute the horizontal distance between A and B (ignoring z coordinate)
static float horizontal_distance(Vector a, Vector b) {
   return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

#define STEP	20              //Incremental move

// Return the floor below V
// TO BE IMPROVED use pEntityCOntaining
static Vector FloorBelow(Vector V) {
   static TraceResult tr;       // Keep it available even outside of the call
   Vector ReallyDown, UpALittle;
   int HullNumber, HullHeight;

   // First use this hull
   HullNumber = human_hull;
   HullHeight = 36;

   // Bump V a little higher (to allow for a steep climb)
   UpALittle = V + Vector(0, 0, HullHeight);
   ReallyDown = V + Vector(0, 0, -500);
   UTIL_TraceHull(UpALittle, ReallyDown, ignore_monsters, HullNumber, NULL,
                  &tr);
   //printf("      Floor %.0f -> %.0f, TraceHull fraction = %.2f, vecEndPos.z=%.0f %s %s\n",
   //UpALittle.z,ReallyDown.z,tr.flFraction,tr.vecEndPos.z,
   //(tr.fAllSolid) ? "AllSolid" : "",
   //(tr.fStartSolid) ? "StartSolid" : "") ;
   if (tr.fStartSolid) {        // Perhaps we where too high and hit the ceiling
      UpALittle = V + Vector(0, 0, 0);
      ReallyDown = V + Vector(0, 0, -500);
      UTIL_TraceHull(UpALittle, ReallyDown, ignore_monsters, HullNumber,
                     NULL, &tr);
      //printf("      Floor without raising %.0f -> %.0f, TraceHull fraction = %.2f, vecEndPos.z=%.0f %s %s\n",
      //UpALittle.z,ReallyDown.z,tr.flFraction,tr.vecEndPos.z,
      //(tr.fAllSolid) ? "AllSolid" : "",
      //(tr.fStartSolid) ? "StartSolid" : "") ;
      if (tr.fStartSolid) {     // Fall back to a point_hull
         HullNumber = point_hull;
         HullHeight = 0;
         UpALittle = V + Vector(0, 0, STEP);
         ReallyDown = V + Vector(0, 0, -500);
         UTIL_TraceHull(UpALittle, ReallyDown, ignore_monsters, HullNumber,
                        NULL, &tr);
         //printf("      Floor with point hull %.0f -> %.0f, TraceHull fraction = %.2f, vecEndPos.z=%.0f %s %s\n",
         //UpALittle.z,ReallyDown.z,tr.flFraction,tr.vecEndPos.z,
         //(tr.fAllSolid) ? "AllSolid" : "",
         //(tr.fStartSolid) ? "StartSolid" : "") ;
         if (tr.fStartSolid) {
            tr.vecEndPos = V;
            tr.vecEndPos.z = -4000;
         }
      }
   }
   tr.vecEndPos.z -= HullHeight;        //Look at feet position (depends on used hull)
   return tr.vecEndPos;
}

// Check whether we can reach End from Start being a normal player
// It uses (or should use) different heurestics:
// - plain walking on a floor with upward jumps or downward falls
// - downward falls either from solid ground or from a point in the air
//   to a point in water or a point in the air
// - ducking and walking (assuming flat ground)
// - swimming
int cNodeMachine::Reachable(const int iStart, const int iEnd) {
   Vector IncMove, Check, Floor, Start, End;
   float Dist, Height, PreviousHeight;
   TraceResult tr;

   Start = Nodes[iStart].origin;
   End = Nodes[iEnd].origin;
#ifdef DEBUG_REACHABLE

   printf("Reachable %d(%.0f,%.0f,%.0f)%s", iStart, Start.x, Start.y,
          Start.z,
          (Nodes[iStart].iNodeBits & BIT_LADDER) ? "OnLadder" : "");
   printf(" >> %d(%.0f,%.0f,%.0f)%s\n", iEnd, End.x, End.y, End.z,
          (Nodes[iEnd].iNodeBits & BIT_LADDER) ? "OnLadder" : "");
#endif
   // Just in case
   if (Start.x == 9999 || End.x == 9999)
      return false;

   // Quick & dirty check whether we can go through...
   // This is simply to quickly decide whether the move is impossibe
   UTIL_TraceHull(Start, End, ignore_monsters, point_hull, NULL, &tr);
#ifdef DEBUG_REACHABLE

   printf("TraceHull --> tr.flFraction = %.2f\n", tr.flFraction);
#endif

   if (tr.flFraction < 1.0)
      return false;

   // If either start/end is on ladder, assume we can fly without falling
   // but still check whether a human hull can go through
   if ((Nodes[iStart].iNodeBits & BIT_LADDER)
         || (Nodes[iEnd].iNodeBits & BIT_LADDER)) {
      UTIL_TraceHull(Start, End, ignore_monsters, human_hull, NULL, &tr);
      return (tr.flFraction >= 1.0);
   }
   // Heurestic for falling
   // TODO

   // Heurestic for walking/jumping/falling on the ground
   // Now check whether we have ground while going from Start to End...

   // Initialize IncMove to a STEP units long vector from Start to End
   IncMove = End - Start;
   IncMove = IncMove.Normalize();
   IncMove = IncMove * STEP;

   // Move Check towards End by small increment and check Z variation
   // Assuming that the move is a pure line TO BE IMPROVED !!

   Check = Start;
   Dist = (End - Check).Length();
   Floor = FloorBelow(Check);
   PreviousHeight = Floor.z;
   while (Dist > STEP) {
      Floor = FloorBelow(Check);
      Height = Floor.z;
#ifdef DEBUG_REACHABLE

      printf
      ("   in loop Check=(%.0f,%.0f,%.0f), Dist = %.0f, Height = %.0f\n",
       Check.x, Check.y, Check.z, Dist, Height);
#endif

      if (Height > PreviousHeight) {    // Going upwards
         if (Height - PreviousHeight > MAX_JUMPHEIGHT) {
            //printf("   too high for upward jump\n") ;
            return false;
         }
      } else {                  // Going downwards
         if (PreviousHeight - Height > MAX_FALLHEIGHT)
            if (UTIL_PointContents(Floor + Vector(0, 0, 5))
                  != CONTENTS_WATER)
               //{printf("   too high for a downward fall not in water\n") ;
               return false;    // Falling from too high not in water
         //}
         //else printf("     ouf we are in water\n") ;
      }
      // TO BE IMPROVED should check whether the surface is walkable & is not LAVA
      PreviousHeight = Height;
      Check = Check + IncMove;
      Dist = (End - Check).Length();
   }
   Height = End.z - 36.0f;
#ifdef DEBUG_REACHABLE

   printf("   after loop End=(%.0f,%.0f,%.0f), Height = %.0f\n",
          End.x, End.y, End.z, Height);
#endif

   if (Height > PreviousHeight) {       // Going upwards
      if (Height - PreviousHeight > MAX_JUMPHEIGHT) {
         //printf("   too high for upward jump\n") ;
         return false;
      }
   } else {                     // Going downwards
      if (PreviousHeight - Height > MAX_FALLHEIGHT)
         if (UTIL_PointContents(End) != CONTENTS_WATER) {
            //printf("   too high for a downward fall not in water\n") ;
            return false;       // Falling from too high not in water
         }
      //else printf("     ouf we are in water\n") ;
   }
   // TODO TO BE IMPROVED should check whether the surface is walkable & is not LAVA

   // Heurestic for ducking
   // TODO

   // Heurestic for swimming
   // TODO

   // Success !
   return true;
}

// Adding a node: another way...
int cNodeMachine::add2(Vector vOrigin, int iType, edict_t * pEntity) {
   int i;
   int iX, iY;

#ifdef DEBUG_ADD

   printf("add2(%.0f,%.0f,%.0f)\n", vOrigin.x, vOrigin.y, vOrigin.z);
   fflush(stdout);
#endif

   // Do not add a node when there is already one close
   if (close(vOrigin, NODE_ZONE, pEntity) > -1)
      return -1;

   for (i = 0; i < MAX_NODES; i++)
      if (Nodes[i].origin == Vector(9999, 9999, 9999))
         break;

   // failed too find a free slot to add a new node
   if (i >= MAX_NODES)
      return -1;

   Nodes[i].origin = vOrigin;
   if (i > iMaxUsedNodes)
      iMaxUsedNodes = i;

#ifdef DEBUG_ADD

   printf("  will use node id %d\n", i);
   fflush(stdout);
#endif
   // Set different flags about the node
   Nodes[i].iNodeBits = iType;  // EVY's extension
   if (pEntity) {
      // ladder
      if (FUNC_IsOnLadder(pEntity))
         Nodes[i].iNodeBits |= BIT_LADDER;

      // water at origin (=waist) or at the feet (-30)
      if (UTIL_PointContents(pEntity->v.origin) == CONTENTS_WATER ||
            UTIL_PointContents(pEntity->v.origin - Vector(0, 0, 30)) ==
            CONTENTS_WATER)
         Nodes[i].iNodeBits |= BIT_WATER;

      // record jumping
      if (pEntity->v.button & IN_JUMP)
         Nodes[i].iNodeBits |= BIT_JUMP;
   } else {
      // water at origin (=waist) or at the feet (-30)
      if (UTIL_PointContents(vOrigin) == CONTENTS_WATER ||
            UTIL_PointContents(vOrigin - Vector(0, 0, 30)) == CONTENTS_WATER)
         Nodes[i].iNodeBits |= BIT_WATER;
   }

   // add to meredians database
   VectorToMeredian(Nodes[i].origin, &iX, &iY);

   if (iX > -1 && iY > -1)
      AddToMeredian(iX, iY, i);

   // Connect this node to other nodes (and vice versa)
   // TODO should use the Meredian structure to only check for nodes in adjacents meredians... (faster algo)
   int MyNeighbourCount, j;
   for (j = 0, MyNeighbourCount = 0;
         j < iMaxUsedNodes && MyNeighbourCount < MAX_NEIGHBOURS; j++) {

      if (j == i)
         continue;              // Exclude self
      if (Nodes[j].origin == Vector(9999, 9999, 9999))
         continue;              // Skip non existing nodes

      // When walking the human player can't pass a certain speed and distance
      // however, when a human is falling, the distance will be bigger.
      if (horizontal_distance(Nodes[i].origin, Nodes[j].origin) >
            3 * NODE_ZONE)
         continue;

      // j is a potential candidate for a neighbourood
      // Let's do further tests

      if (Reachable(i, j)) {    // Can reach j from i ?
         add_neighbour_node(i, j);      // Add j as a neighbour of mine
         MyNeighbourCount++;
      }
      if (Reachable(j, i))      // Can reach i from j ?
         add_neighbour_node(j, i);      // Add i as a neighbour of j

   }

   return i;
}

// Adding a node
int cNodeMachine::add
   (Vector vOrigin, int iType, edict_t * pEntity) {
   // Do not add a node when there is already one close
   if (close(vOrigin, NODE_ZONE, pEntity) > -1)
      return -1;

   int index = -1;
   int i = 0;                   // <-- ADDED BY PMB ELSE LINUX COMPILER ISN'T HAPPY
   for (i = 0; i < MAX_NODES; i++)
      if (Nodes[i].origin == Vector(9999, 9999, 9999)) {
         index = i;
         break;
      }
   // failed
   if (index < 0)
      return -1;

   Nodes[index].origin = vOrigin;

   // SET BITS:
   bool bIsInWater = false;
   bool bIndexFloats = false, bIndexOnCrate = false;
   bool bDucks = false;

   // ladder
   if (pEntity) {
      // Does this thing float?
      bIndexFloats = node_float(Nodes[index].origin, pEntity);
      bIndexOnCrate = node_on_crate(Nodes[index].origin, pEntity);

      if (FUNC_IsOnLadder(pEntity))
         Nodes[index].iNodeBits |= BIT_LADDER;

      // water
      if (UTIL_PointContents(pEntity->v.origin) == CONTENTS_WATER) {
         Nodes[index].iNodeBits |= BIT_WATER;
         bIsInWater = true;
      }
      // record jumping
      // FIXED: removed ; , thanks Whistler
      if (pEntity->v.button & IN_JUMP)
         Nodes[index].iNodeBits |= BIT_JUMP;

      if (FUNC_IsOnLadder(pEntity))
         bIndexFloats = false;

      if (pEntity->v.button & IN_DUCK)
         bDucks = true;

   }                            // do only check pEntity when its not NULL
   else {
      // Does this thing float?
      bIndexFloats = node_float(Nodes[index].origin, NULL);
      bIndexOnCrate = node_on_crate(Nodes[index].origin, NULL);
   }

   // add to meredians database
   int iX, iY;
   VectorToMeredian(Nodes[index].origin, &iX, &iY);

   if (iX > -1 && iY > -1)
      AddToMeredian(iX, iY, index);

   // done.
   TraceResult tr;              // for traceresults
   int nId = 0;                 // for neighbours




   // Connect this node to other nodes (and vice versa)
   for (int j = 0; j < MAX_NODES; j++) {
      // Exclude rules

      // self
      if (j == index)
         continue;

      // invalid
      if (Nodes[j].origin == Vector(9999, 9999, 9999))
         continue;

      // normalized vector
      Vector vNormalizedOrigin = Nodes[j].origin;
      Vector vNormalizedIndex = Nodes[i].origin;

      vNormalizedOrigin.z = 0;
      vNormalizedIndex.z = 0;

      // When walking the human player can't pass a certain speed and distance
      // however, when a human is falling, the distance will be bigger.
      if (func_distance(vNormalizedOrigin, vNormalizedIndex) >
            (NODE_ZONE * 3)) {
         continue;
      }
      // ----------------------
      // The node is close enough, is valid, and not the same as the created one.
      // ----------------------

      // Traceline from J to I
      bool bNeighbourFloats = node_float(Nodes[j].origin, pEntity);
      bool bNeighOnCrate = node_on_crate(Nodes[j].origin, pEntity);
      bool bNeighbourWater = false;

      // when pEntity is on ladder, it is NOT floating!
      if (FUNC_IsOnLadder(pEntity))
         bNeighbourFloats = false;

      if (Nodes[j].iNodeBits & BIT_LADDER)
         bNeighbourFloats = false;

      if (Nodes[j].iNodeBits & BIT_WATER)
         bNeighbourWater = true;

      // Check if Index is LOWER then J, if so, the height should be jumpable!
      // When height does not differ to much we can add this connection.
      //
      //
      // 14/06/04 - fix: Some slopes are not counted in here..., de_dust jump from crates connects now
      if ((bIndexFloats /* && bNeighbourFloats */ )
            && (Nodes[j].origin.z > Nodes[index].origin.z)
            && (Nodes[j].origin.z - Nodes[index].origin.z > MAX_FALLHEIGHT)
            && (bNeighbourWater == false && bIsInWater == false)) {

         //char msg[80];
         //sprintf(msg, "NODES FLOATING: INDEX.Z = %f LOWER THEN J.Z = %f\n", Nodes[index].origin.z, Nodes[j].origin.z);
         //   UTIL_ClientPrintAll( HUD_PRINTNOTIFY, msg);
         //SERVER_PRINT(msg);
         continue;
      }
      // skip nodes which are to high
      if ((Nodes[index].origin.z - Nodes[j].origin.z) > MAX_FALLHEIGHT) {
         //                SERVER_PRINT("to high\n");
         continue;
      }
      // Index is not floating, we should check if we can jump it
      // 14/06/04 - temporary fix: We distinguish 2 kind of checks
      // when the nodes are on crates or not.

      if (bIndexOnCrate || bNeighOnCrate) {
         if (bIndexFloats == false &&   // we stand on something
               Nodes[j].origin.z > Nodes[index].origin.z &&       // we MUST jump (not fall)
               bNeighbourWater == false && fabs(Nodes[j].origin.z - Nodes[index].origin.z) > MAX_JUMPHEIGHT)      // and cannot jump to it
         {
            //                        SERVER_PRINT("Cannot jump to it");
            continue;           // next neighbour
         }
      } else {
         // both are on steep, do a bit simplified check
         if (bIndexFloats == false &&   // we stand on something
               Nodes[j].origin.z > Nodes[index].origin.z &&       // we MUST jump (not fall)
               bNeighbourWater == false && fabs(Nodes[j].origin.z - Nodes[index].origin.z) > MAX_FALLHEIGHT)      // and cannot jump to it
         {
            //                        SERVER_PRINT("Insanity jump not possible either!\n");
            continue;           // next neighbour
         }
      }

      int hull_type = head_hull;

      if (FUNC_IsOnLadder(pEntity))
         hull_type = point_hull;

      // falling
      // 14/06/05 - this code does not cause bad connections! - Stefan
      // 20/06/05 - oops, it does... because it blocks when we are NOT falling...
      if ((Nodes[j].origin.z - Nodes[index].origin.z) > MAX_FALLHEIGHT) {
         hull_type = human_hull;        // when we fall, be sure we can freely fall.
      }
      // when we go steep, we have a point hull!!
      if ((bIndexOnCrate && bNeighOnCrate == false)
            || (bIndexOnCrate == false && bNeighOnCrate)) {
         // 20/06/05 - head_hull -> point_hull, somehow 'down stairs' does not get connected well
         // probably because head_hull is the size of a ducking player. So lets do a point_hull
         // then...

         // it seems that when going down the human_hull is used,
         hull_type = point_hull;
      }


      UTIL_TraceHull(Nodes[index].origin, Nodes[j].origin, ignore_monsters,
                     hull_type, pEntity->v.pContainingEntity, &tr);

      // if nothing hit:
      if (tr.flFraction >= 1.0) {

         // Add this to the neighbouring list
         Nodes[index].iNeighbour[nId] = j;
         // Do a reverse check immidiatly if possible
         bool bCanConnect = true;

         // ---- the same checks as above, but reversed ----

         // Check if J is LOWER then Index, if so, the height should be jumpable!
         // When height does not differ to much we can add this connection.
         if ((bIndexFloats && bNeighbourFloats)
               && (bNeighbourWater == false && bIsInWater == false)
               && Nodes[index].origin.z >= Nodes[j].origin.z) {
            char msg[80];
            sprintf(msg, "J.Z = %f, INDEX.Z = %f\n", Nodes[j].origin.z,
                    Nodes[index].origin.z);
            //              UTIL_ClientPrintAll( HUD_PRINTNOTIFY, msg);
            bCanConnect = false;        // cannot connect
         }
         // Index is not floating, we should check if we can jump it
         if (bNeighbourFloats == false &&       // we stand on something
               Nodes[index].origin.z > Nodes[j].origin.z &&       // we MUST jump (not fall)
               bIsInWater == false && fabs(Nodes[index].origin.z - Nodes[j].origin.z) > MAX_JUMPHEIGHT)   // and cannot jump to it
            bCanConnect = false;        // cannot connect

         // All water stuff can connect to each other
         if (bNeighbourWater && bIsInWater)
            bCanConnect = true;

         if (bCanConnect) {
            int jNeigh = neighbour_node(Nodes[j]);
            if (jNeigh > -1)
               Nodes[j].iNeighbour[jNeigh] = index;     // reversed also possible
         }

         nId++;
         if (nId > MAX_NEIGHBOURS)
            break;              // found enough neighbours

      } else {
         //              UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NodeMachine: Hit something!!!!\n");
         //                if (tr.pHit != NULL)
         //                        UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(tr.pHit->v.classname));
      }

   }

   if (index > iMaxUsedNodes)
      iMaxUsedNodes = index;

   //UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NodeMachine: Succesfully added node\n");
   return index;
}

// Players initialization
void cNodeMachine::init_players() {
   for (int i = 0; i < 32; i++) {
      Players[i].vPrevPos = Vector(9999, 9999, 9999);
      Players[i].iNode = -1;
   }
}

// Init on roundstart
void cNodeMachine::init_round() {
   //UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NodeMachine: Roundstart\n");
   for (int index = 1; index <= gpGlobals->maxClients; index++) {
      edict_t *pPlayer = INDEXENT(index);

      // skip invalid players
      if ((pPlayer) && (!pPlayer->free))
         Players[(index - 1)].vPrevPos = pPlayer->v.origin;
   }

   // Init paths
   for (int p = 0; p < 32; p++)
      path_clear(p);

   // decrease bad score for bad goal nodes
   for (int g = 0; g < MAX_GOALS; g++)
      if (Goals[g].iBadScore > 0)
         Goals[g].iBadScore--;
}

// Set up origin for players etc.
void cNodeMachine::players_plot() {
   for (int index = 1; index <= gpGlobals->maxClients; index++) {
      edict_t *pPlayer = INDEXENT(index);

      // skip invalid players
      if ((pPlayer) && (!pPlayer->free))
         if (IsAlive(pPlayer)) {
            int iPlayerIndex = index - 1;
            // update this player information
            // FIX: Increased distance.. with 75%
            if (func_distance
                  (pPlayer->v.origin,
                   Players[iPlayerIndex].vPrevPos) > (NODE_ZONE * 1.75)) {
               //UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NodeMachine: Players plot\n");
               Players[iPlayerIndex].vPrevPos = pPlayer->v.origin;
#ifdef USE_EVY_ADD

               add2(pPlayer->v.origin, 0, pPlayer);     // Add a node
#else

               add2(pPlayer->v.origin, 0, pPlayer);     // Add a node
#endif

            }
         }

   }
}

// Draw connections of the node we are standing on
void cNodeMachine::connections(edict_t * pEntity) {
   int i = close(pEntity->v.origin, NODE_ZONE, pEntity);
   if (i > -1) {
      for (int j = 0; j < MAX_NEIGHBOURS; j++) {
         if (Nodes[i].iNeighbour[j] > -1) {
            Vector start = Nodes[i].origin;
            Vector end = Nodes[Nodes[i].iNeighbour[j]].origin;
            WaypointDrawBeam(pEntity, start, end, 2, 0, 255, 255, 0, 255,
                             1);
         }
      }

   }
}

// Draw
void cNodeMachine::draw(edict_t * pEntity) {
   //DebugOut("waypoint: waypoint_draw()\n");
   int i = 0, max_drawn = 0;

   for (i = 0; i < MAX_NODES; i++) {
      if (Nodes[i].origin != Vector(9999, 9999, 9999)) {
         Vector start = Nodes[i].origin - Vector(0, 0, 36);
         Vector end = Nodes[i].origin;

         bool good = VectorIsVisibleWithEdict(pEntity, end, "none");

         int angle_to_waypoint =
            FUNC_InFieldOfView(pEntity, (end - pEntity->v.origin));

         if (good && angle_to_waypoint < 65) {

            int r, g, b, l;
            r = g = b = l = 250;
            l = 250;
            //l = 250; // Normally light is 250

            if (Nodes[i].iNodeBits & BIT_LADDER)
               b = g = 0;

            if (Nodes[i].iNodeBits & BIT_WATER)
               r = g = 0;

            if (Nodes[i].iNodeBits & BIT_DUCK)
               r = b = 0;

            if (Nodes[i].iNeighbour[0] < 0)
               r = 0;

            if (max_drawn < 39) {
               WaypointDrawBeam(pEntity, start, end, 4, 0, r, g, b, l, 1);
               max_drawn++;
            }
         }
      }                         // for
   }
   int iNodeClose = close(pEntity->v.origin, NODE_ZONE, pEntity);

   char msg[50];
   char Flags[10];

   Flags[0] = 0;
   if (Nodes[iNodeClose].iNodeBits & BIT_LADDER)
      strcat(Flags, "L");
   if (Nodes[iNodeClose].iNodeBits & BIT_WATER)
      strcat(Flags, "W");
   if (Nodes[iNodeClose].iNodeBits & BIT_JUMP)
      strcat(Flags, "J");
   if (Nodes[iNodeClose].iNodeBits & BIT_DUCK)
      strcat(Flags, "D");
   sprintf(msg, "Node %d(%.0f,%.0f,%.0f)%s\nMe: (%.0f,%.0f,%.0f)\n",
           iNodeClose, (iNodeClose < 0) ? -1 : Nodes[iNodeClose].origin.x,
           (iNodeClose < 0) ? -1 : Nodes[iNodeClose].origin.y,
           (iNodeClose < 0) ? -1 : Nodes[iNodeClose].origin.z, Flags,
           pEntity->v.origin.x, pEntity->v.origin.y, pEntity->v.origin.z);
   CenterMessage(msg);
}

// Save Experience
void cNodeMachine::experience_save() {
   char dirname[256];
   char filename[256];
   int i;

   // Set Directory name
   strcpy(dirname, "data/cstrike/exp/");
   strcat(dirname, STRING(gpGlobals->mapname));
   strcat(dirname, ".rbx");     // nodes file

   // writes whole path into "filename", Linux compatible
   UTIL_BuildFileNameRB(dirname, filename);

   FILE *rbl;
   // Only save if lock type is < 1
   rbl = fopen(filename, "wb");

   if (rbl != NULL) {
      int iVersion = FILE_EXP_VER2;
      fwrite(&iVersion, sizeof(int), 1, rbl);

      for (i = 0; i < MAX_NODES; i++) {
         fwrite(&InfoNodes[i].fDanger[0], sizeof(Vector), 1, rbl);
         fwrite(&InfoNodes[i].fDanger[1], sizeof(Vector), 1, rbl);

         fwrite(&InfoNodes[i].fContact[0], sizeof(Vector), 1, rbl);
         fwrite(&InfoNodes[i].fContact[1], sizeof(Vector), 1, rbl);
      }

      if (iMaxUsedNodes > MAX_NODES)
         iMaxUsedNodes = MAX_NODES;

      // Here write down the MAX amounts of nodes used from vis table!
      unsigned long iSize = (iMaxUsedNodes * MAX_NODES) / 8;

      fwrite(&iMaxUsedNodes, sizeof(int), 1, rbl);

      // Write down 512 bytes chunks, and when a remaining piece is left over
      // also write that down. Only when 'size' is 0 we quit the loop
      unsigned long iPos = 0;
      unsigned long iChunk = 512;

      // When we exceed the size we make sure that the chunk is not to big
      if (iSize < iChunk)
         iChunk = 512 - iSize;

      unsigned long iRemaining = iSize - iChunk;        // unsigned caused SEGV while below -- BERKED

      // While we still have bytes remaining to write to disk.
      while (iRemaining > 0) {
         fwrite(&cVisTable[iPos], iChunk, 1, rbl);

         iChunk = 512;          // keep the size 512

         // When we exceed the size we make sure that the chunk is not to big
         if (iRemaining < iChunk)
            iChunk = 512 - iRemaining;

         iRemaining -= iChunk;
         iPos += iChunk;        // increase position to read from cVisTable

         if (iRemaining < 1)
            break;              // escape
      }

      // write down the checked vis
      fwrite(iVisChecked, sizeof(iVisChecked), 1, rbl);
      fclose(rbl);
   } else
      fprintf(stderr, "Cannot write experience file %s\n", filename);
}

// Load Danger
void cNodeMachine::experience_load() {
   char dirname[256];
   char filename[256];
   int i;

   // Set Directory name
   strcpy(dirname, "data/cstrike/exp/");
   strcat(dirname, STRING(gpGlobals->mapname));
   strcat(dirname, ".rbx");     // nodes file

   // writes whole path into "filename", Linux compatible
   UTIL_BuildFileNameRB(dirname, filename);

   FILE *rbl;
   rbl = fopen(filename, "rb");

   if (rbl != NULL) {
      int iVersion = FILE_EXP_VER1;
      fread(&iVersion, sizeof(int), 1, rbl);

      if (iVersion == FILE_EXP_VER1) {
         for (i = 0; i < MAX_NODES; i++) {
            fread(&InfoNodes[i].fDanger[0], sizeof(Vector), 1, rbl);
            fread(&InfoNodes[i].fDanger[1], sizeof(Vector), 1, rbl);

            fread(&InfoNodes[i].fContact[0], sizeof(Vector), 1, rbl);
            fread(&InfoNodes[i].fContact[1], sizeof(Vector), 1, rbl);
         }

         fread(&iMaxUsedNodes, sizeof(int), 1, rbl);

         // make sure we never exceed the limit
         if (iMaxUsedNodes > MAX_NODES)
            iMaxUsedNodes = MAX_NODES;

         unsigned long iSize = (iMaxUsedNodes * MAX_NODES) / 8;

         // Read table from what we know
         fread(cVisTable, iSize, 1, rbl);
         fread(iVisChecked, sizeof(iVisChecked), 1, rbl);
      } else if (iVersion == FILE_EXP_VER2) {
         for (i = 0; i < MAX_NODES; i++) {
            fread(&InfoNodes[i].fDanger[0], sizeof(Vector), 1, rbl);
            fread(&InfoNodes[i].fDanger[1], sizeof(Vector), 1, rbl);

            fread(&InfoNodes[i].fContact[0], sizeof(Vector), 1, rbl);
            fread(&InfoNodes[i].fContact[1], sizeof(Vector), 1, rbl);
         }

         fread(&iMaxUsedNodes, sizeof(int), 1, rbl);

         // make sure we never exceed the limit
         if (iMaxUsedNodes > MAX_NODES)
            iMaxUsedNodes = MAX_NODES;

         unsigned long iSize = (iMaxUsedNodes * MAX_NODES) / 8;

         // Now read the cVisTable from what we know
         ClearVisibilityTable();        // clear first

         unsigned long iPos = 0;
         unsigned long iChunk = 512;
         unsigned long iRemaining = iSize;

         while (iRemaining > 0) {
            // Emergency, when iPos is getting to big, we get the heck out of here
            if (iPos > iSize)
               break;

            // Read table from what we know
            fread(&cVisTable[iPos], iChunk, 1, rbl);

            // When we exceed the size we make sure that the chunk is not to big
            if (iRemaining < iChunk)
               iChunk = 512 - iRemaining;

            iRemaining -= iChunk;
            iPos += iChunk;     // increase position to read from cVisTable

            if (iRemaining < 1)
               break;           // escape
         }

         fread(iVisChecked, sizeof(iVisChecked), 1, rbl);
      }

      fclose(rbl);
   }
}

// Save
void cNodeMachine::save() {
   char dirname[256];
   char filename[256];
   int i, n;

   // Set Directory name
   strcpy(dirname, "data/cstrike/maps/");
   strcat(dirname, STRING(gpGlobals->mapname));
   strcat(dirname, ".rbn");     // nodes file

   // writes whole path into "filename", Linux compatible
   UTIL_BuildFileNameRB(dirname, filename);

   FILE *rbl;
   // Only save if lock type is < 1
   rbl = fopen(filename, "wb");

   if (rbl != NULL) {
      // Write down version number
      int iVersion = FILE_NODE_VER1;
      fwrite(&iVersion, sizeof(int), 1, rbl);
      for (i = 0; i < MAX_NODES; i++) {
         fwrite(&Nodes[i].origin, sizeof(Vector), 1, rbl);
         for (n = 0; n < MAX_NEIGHBOURS; n++)
            fwrite(&Nodes[i].iNeighbour[n], sizeof(int), 1, rbl);

         // save bit flags
         fwrite(&Nodes[i].iNodeBits, sizeof(int), 1, rbl);
      }
      fclose(rbl);
   } else
      fprintf(stderr, "Cannot write experience file %s\n", filename);
}

void cNodeMachine::save_important() {
   char dirname[256];
   char filename[256];

   // Set Directory name
   strcpy(dirname, "data/cstrike/ini/");
   strcat(dirname, STRING(gpGlobals->mapname));
   strcat(dirname, ".ini");     // nodes file

   // writes whole path into "filename", Linux compatible
   UTIL_BuildFileNameRB(dirname, filename);

   // Only save if lock type is < 1
   FILE *rbl = fopen(filename, "w+t");

   if (rbl) {
      fprintf(rbl,
              "; RealBot : Important Area Definition file\n; Do not hand-edit, this is _not_ an editable ini file!\n;\n\n");

      // save important areas:
      for (int iGn = 0; iGn < MAX_GOALS; iGn++) {
         if (Goals[iGn].iType == GOAL_IMPORTANT) {
            // save this area
            fprintf(rbl, "[AREA]\n");
            Vector iGoalVector = node_vector(Goals[iGn].iNode);
            fprintf(rbl, "X=%f\n", iGoalVector.x);
            fprintf(rbl, "Y=%f\n", iGoalVector.y);
            fprintf(rbl, "Z=%f\n\n", iGoalVector.z);

         }
      }

      fprintf(rbl, "; Eof");
      fclose(rbl);
   }
}

// Load
void cNodeMachine::load() {
   char dirname[256];
   char filename[256];
   int i, n;

   // Set Directory name
   strcpy(dirname, "data/cstrike/maps/");

   strcat(dirname, STRING(gpGlobals->mapname));
   strcat(dirname, ".rbn");     // nodes file

   // writes whole path into "filename", Linux compatible
   UTIL_BuildFileNameRB(dirname, filename);

   FILE *rbl;
   rbl = fopen(filename, "rb");

   if (rbl != NULL) {
      int iVersion = FILE_NODE_VER1;
      fread(&iVersion, sizeof(int), 1, rbl);

      // Version 1.0
      if (iVersion == FILE_NODE_VER1) {
         for (i = 0; i < MAX_NODES; i++) {
            fread(&Nodes[i].origin, sizeof(Vector), 1, rbl);
            for (n = 0; n < MAX_NEIGHBOURS; n++) {
               fread(&Nodes[i].iNeighbour[n], sizeof(int), 1, rbl);
            }

            // save bit flags
            fread(&Nodes[i].iNodeBits, sizeof(int), 1, rbl);

            if (Nodes[i].origin != Vector(9999, 9999, 9999))
               iMaxUsedNodes = i;
         }
      }
      // Add nodes to meredians
      for (i = 0; i < MAX_NODES; i++)
         if (Nodes[i].origin != Vector(9999, 9999, 9999)) {
            int iX, iY;
            VectorToMeredian(Nodes[i].origin, &iX, &iY);
            if (iX > -1 && iY > -1)
               AddToMeredian(iX, iY, i);
         }

      fclose(rbl);

      // 04/07/04
      // Check for full nodes table

      if (Nodes[MAX_NODES - 1].origin != Vector(9999, 9999, 9999))
         rblog("!!! Nodes table is full\n");

      char msg[80];
      sprintf(msg, "After NodeMachine::load iMaxUsedNodes=%d\n",
              iMaxUsedNodes);
      rblog(msg);
      SERVER_PRINT("Going to load IAD file : ");
      INI_PARSE_IAD();
   } else {
      //    char msg[128];
      //    sprintf(msg, "Notice: Could not open %s.\n");
      //    SERVER_PRINT(msg);
   }
}

void cNodeMachine::ClearImportantGoals() {
   for (int iGn = 0; iGn < MAX_GOALS; iGn++) {
      if (Goals[iGn].iType == GOAL_IMPORTANT && Goals[iGn].iNode > -1) {
         Goals[iGn].iType = -1;
         Goals[iGn].iNode = -1;
         Goals[iGn].pGoalEdict = NULL;
      }
   }
}

// Draw path 0 (user)
void cNodeMachine::path_draw(edict_t * pEntity) {
   //DebugOut("waypoint: waypoint_draw()\n");
   int i = 0, max_drawn = 0;

   for (i = 0; i < MAX_NODES; i++) {
      int iNode = iPath[draw_nodepath][i];
      int iNextNode = iPath[draw_nodepath][(i + 1)];

      if (iNode > -1 && iNextNode > -1) {
         Vector start = Nodes[iNode].origin;
         Vector end = Nodes[iNextNode].origin;

         bool good = VectorIsVisibleWithEdict(pEntity, end, "none");
         int angle_to_waypoint =
            FUNC_InFieldOfView(pEntity, (end - pEntity->v.origin));

         if (max_drawn < 39 && good && angle_to_waypoint < 50) {
            WaypointDrawBeam(pEntity, start, end, 10, 0, 255, 0, 255, 255,
                             5);
            max_drawn++;
         }
      }                         // for
   }
   int iNodeClose = close(pEntity->v.origin, 35, pEntity);

   char msg[30];
   sprintf(msg, "Node %d\n", iNodeClose);
   CenterMessage(msg);
}

// Spread contact areas
void cNodeMachine::contact(int iNode, int iTeam) {
   if (iNode < 0 || iNode >= MAX_NODES)
      return;

   if (iTeam < 0 || iTeam > 1)
      return;

   // Nodes near have more danger, increase with:
   // ((DISTANCE / NODE_CONTACT_DIST) * NODE_CONTACT_STEP)
   // example:
   // 200 / 512 = 0.39 * 0.2 = 0.07

   // Spot itself is + NODE_CONTACT_STEP;
   InfoNodes[iNode].fContact[iTeam] += NODE_CONTACT_STEP;

   // Go through all valid nodes, except iNode, and increase danger if needed.
   for (int i = 0; i < MAX_NODES; i++) {
      if (Nodes[i].origin != Vector(9999, 9999, 9999) && i != iNode) {
         float fDist = func_distance(Nodes[i].origin, Nodes[iNode].origin);
         if (fDist < NODE_CONTACT_DIST) {
            //Using TraceHull to detect de_aztec bridge and other entities.
            TraceResult tr;

            //UTIL_TraceHull(Nodes[iNode].origin, Nodes[i].origin, ignore_monsters, human_hull, NULL, &tr);
            UTIL_TraceHull(Nodes[iNode].origin, Nodes[i].origin,
                           ignore_monsters, point_hull, NULL, &tr);

            // within distance and 'reachable'
            if (tr.flFraction >= 1.0)
               InfoNodes[i].fContact[iTeam] +=
                  (fDist / NODE_CONTACT_DIST) * NODE_CONTACT_STEP;
         }
      }
   }
}

// Spread danger around
void cNodeMachine::danger(int iNode, int iTeam) {
   if (iNode < 0 || iNode >= MAX_NODES)
      return;

   if (iTeam < 0 || iTeam > 1)
      return;

   // Nodes near have more danger, increase with:
   // ((DISTANCE / NODE_DANGER_DIST) * NODE_DANGER_STEP)
   // example:
   // 200 / 512 = 0.39 * 0.2 = 0.07

   // The further away, the less danger increased. At actual
   // danger spot the danger increases with 10% (0.10)

   // Spot itself is + NODE_DANGER_STEP;
   InfoNodes[iNode].fDanger[iTeam] += NODE_DANGER_STEP;

   // Go through all valid nodes, except iNode, and increase danger if needed.
   for (int i = 0; i < MAX_NODES; i++) {
      if (Nodes[i].origin != Vector(9999, 9999, 9999) && i != iNode) {
         float fDist = func_distance(Nodes[i].origin, Nodes[iNode].origin);
         if (fDist < NODE_DANGER_DIST) {
            //Using TraceHull to detect de_aztec bridge and other entities.
            TraceResult tr;
            UTIL_TraceHull(Nodes[iNode].origin, Nodes[i].origin,
                           ignore_monsters, point_hull, NULL, &tr);

            // within distance and reachable
            if (tr.flFraction >= 1.0) {
               InfoNodes[i].fDanger[iTeam] +=
                  (fDist / NODE_DANGER_DIST) * NODE_DANGER_STEP;

               // TODO TODO TODO works?
               //if (InfoNodes[i].fDanger[iTeam] > 1.2)
                //  InfoNodes[i].fDanger[iTeam] = 1.2;
            }
         }
      }
   }
}

// Adds a new goal to the array
void cNodeMachine::goal_add(edict_t * pEdict, int iType, Vector vVec) {
   //
   // 14/06/04
   // Be carefull with adding SERVER_PRINT messages here
   // if so, only use them with maps that have NO RBN FILE! When loading
   // and RBN file with a corresponding INI file this will crash HL.EXE
   //

   if (pEdict != NULL) {
      if (goal_exists(pEdict)) {
         return;                // do not add goal that is already in our list
      }
   }

   int index = -1;
   int g = 0;                   // <-- ADDED BY PMB ELSE LINUX COMPILER COMPLAINS (ISO COMPLIANCE)
   for (g = 0; g < MAX_GOALS; g++)
      if (Goals[g].iType == GOAL_NONE) {
         index = g;
         break;
      }

   if (index < 0)
      return;                   // no more arrays left

   int iDist = NODE_ZONE * 2;

   //if (iType == GOAL_BOMBSPOT)
   //      iDist = 75;

   //if (iType == GOAL_IMPORTANT)
   //      iDist = 75;
   int nNode = close(vVec, iDist, pEdict);

   if (nNode < 0) {
      // 11/06/04 - stefan - when no goal exists, add it.
      if (iType != GOAL_HOSTAGE) {
         nNode = add
                    (vVec, iType, pEdict);
      }

      if (nNode < 0)
         return;                // no node near
   }

   Goals[g].iNode = nNode;
   Goals[g].pGoalEdict = pEdict;
   Goals[g].iType = iType;

}

// Does the goal already exist in your goals array?
bool cNodeMachine::goal_exists(edict_t * pEdict) {
   for (int g = 0; g < MAX_GOALS; g++)
      if (Goals[g].pGoalEdict == pEdict)
         return true;

   // Does not exist
   return false;
}

void cNodeMachine::goal_reset() {
   for (int g = 0; g < MAX_GOALS; g++) {
      if (Goals[g].iChecked > 0)
         Goals[g].iChecked = 0;
   }
}

// returns goal type from node, -1 for unknown
int cNodeMachine::goal_from_node(int iNode) {
   for (int g = 0; g < MAX_GOALS; g++)
      if (Goals[g].iNode == iNode)
         return Goals[g].iType;

   return -1;
}

// HOSTAGE: Return (random) goal node close to hostage
int cNodeMachine::goal_hostage(cBot * pBot) {

   int iGoals[10];
   for (int i = 0; i < 10; i++)
      iGoals[i] = -1;

   int iIndex = 0;

   edict_t *pent = NULL;
   while ((pent =
              UTIL_FindEntityByClassname(pent, "hostage_entity")) != NULL) {

      if (pent->v.effects & EF_NODRAW)
         continue;              // already rescued

      // Not used by any other bot?
      if (FUNC_UsedHostage(pBot, pent))
         continue;

      // not moving?
      if (FUNC_PlayerSpeed(pent) > 0)
         continue;              // already in use (else it would not move)

      // not using it already self?
      if ((FUNC_FreeHostage(pBot, pent)) == false
            || (FUNC_UsedHostage(pBot, pent) == true))
         continue;

      int iClose = close(pent->v.origin, 100, pent);
      if (iClose > -1) {
         iGoals[iIndex] = iClose;
         iIndex++;
      }
   }

   // Now return value
   if (iIndex > 0) {
      int iReturn = iGoals[RANDOM_LONG(0, (iIndex - 1))];
      return iReturn;
   }

   return -1;
}

// Goal attacher
void cNodeMachine::goals() {
   // Called on round-start, will update goals,
   // because Nodes get expanded all the time the bot should eventually learn
   // how to reach other goals.

   edict_t *pent = NULL;

   // TODO: when a bot knows the goal , the path, but fails, we should make it
   // GOAL_UNREACHABLE to prevent stupid actions in the future

   // GOAL #1 - Counter Terrorist Spawn points.
   while ((pent =
              UTIL_FindEntityByClassname(pent, "info_player_start")) != NULL) {
      if (goal_exists(pent) == false)
         goal_add(pent, GOAL_SPAWNCT, pent->v.origin);
   }

   // GOAL #2 - Terrorist Spawn points.
   while ((pent =
              UTIL_FindEntityByClassname(pent,
                                         "info_player_deathmatch")) != NULL) {
      if (goal_exists(pent) == false)
         goal_add(pent, GOAL_SPAWNT, pent->v.origin);
   }

   // GOAL #3 - Hostage rescue zone
   while ((pent =
              UTIL_FindEntityByClassname(pent,
                                         "func_hostage_rescue")) != NULL) {
      if (goal_exists(pent) == false)
         goal_add(pent, GOAL_RESCUEZONE, VecBModelOrigin(pent));
   }

   // EVY: rescue zone can also be an entitity of info_hostage_rescue
   while ((pent =
              UTIL_FindEntityByClassname(pent,
                                         "info_hostage_rescue")) != NULL) {
      if (goal_exists(pent) == false)
         goal_add(pent, GOAL_RESCUEZONE, VecBModelOrigin(pent));
   }

   // GOAL #4 - Bombspot zone
   // Bomb spot
   while ((pent =
              UTIL_FindEntityByClassname(pent, "func_bomb_target")) != NULL) {
      if (goal_exists(pent) == false)
         goal_add(pent, GOAL_BOMBSPOT, VecBModelOrigin(pent));
   }

   while ((pent =
              UTIL_FindEntityByClassname(pent, "info_bomb_target")) != NULL) {
      if (goal_exists(pent) == false)
         goal_add(pent, GOAL_BOMBSPOT, VecBModelOrigin(pent));
   }

   // GOAL #5 - Hostages (this is the 'starting' position):
   while ((pent =
              UTIL_FindEntityByClassname(pent, "hostage_entity")) != NULL) {
      if (pent->v.effects & EF_NODRAW)
         continue;              // already rescued

      // not moving?
      if (FUNC_PlayerSpeed(pent) > 0)
         continue;              // already in use (else it would not move)

      // Add hostage to goals list
      goal_add(pent, GOAL_HOSTAGE, pent->v.origin);
   }

   // GOAL  #6 - VIP (this is the 'starting' position) (EVY)
   while ((pent =
              UTIL_FindEntityByClassname(pent, "info_vip_start")) != NULL) {
      if (goal_exists(pent) == false)
         goal_add(pent, GOAL_VIP, VecBModelOrigin(pent));
   }

   // GOAL  #7 - VIP safety (this is the 'rescue' position) (EVY)
   while ((pent =
              UTIL_FindEntityByClassname(pent,
                                         "func_vip_safetyzone")) != NULL) {
      if (goal_exists(pent) == false)
         goal_add(pent, GOAL_VIPSAFETY, VecBModelOrigin(pent));
   }

   // GOAL  #8 - Escape zone for es_ (EVY)
   while ((pent =
              UTIL_FindEntityByClassname(pent, "func_escapezone")) != NULL) {
      if (goal_exists(pent) == false)
         goal_add(pent, GOAL_ESCAPEZONE, VecBModelOrigin(pent));
   }

   // 05/07/04
   // GOAL  #9 - Free weapons on the ground EVY
   while ((pent =
              UTIL_FindEntityByClassname(pent, "armoury_entity")) != NULL) {
      if (goal_exists(pent) == false)
         goal_add(pent, GOAL_WEAPON, VecBModelOrigin(pent));
   }


   // Add important goals: (from ini file)
}

// Find a goal, and return the node close to it
int cNodeMachine::node_goal(int iType) {
   if (iType == GOAL_NONE)
      return -1;

   int goals_list[MAX_GOALS];
   for (int c = 0; c < MAX_GOALS; c++)
      goals_list[c] = -1;

   int iId = 0;
   for (int g = 0; g < MAX_GOALS; g++)
      if (Goals[g].iType == iType && Goals[g].iNode > -1) {
         goals_list[iId] = Goals[g].iNode;
         iId++;
      }

   if (iId == 0)
      return -1;                // nothing found :(

   iId--;
   // we have an amount of goals (iId has them)
   int the_goal = RANDOM_LONG(0, iId);

   return goals_list[the_goal];
}

// Contact scaler (on round start)
void cNodeMachine::scale_contact() {
   // On round start: Search for highest and make that 1.0 in scale
   // rescale all other values to stay correct.
   int iTeam = 0;
   while (iTeam < 2) {
      float fHighest = 0.0;
      int i = 0;                // <-- ADDED BY PMB ELSE LINUX COMPILER ISNT HAPPY
      for (i = 0; i < MAX_NODES; i++)
         if (InfoNodes[i].fContact[iTeam] > fHighest)
            fHighest = InfoNodes[i].fContact[iTeam];

      if (fHighest < 1.0) {
         iTeam++;
         continue;              // no need to rescale
      }
      // Check how much we passed the limit
      float fLimit = 1.0 / fHighest;

      // Now rescale all
      for (i = 0; i < MAX_NODES; i++)
         if (InfoNodes[i].fContact[iTeam] > 0.0)
            InfoNodes[i].fContact[iTeam] *= fLimit;     // rescale

      iTeam++;
      if (iTeam > 1)
         break;
   }
}

// Danger scaler (on round start)
void cNodeMachine::scale_danger() {
   // On round start: Search for highest danger and make that 1.0 in scale
   // rescale all other danger values to stay correct.
   int iTeam = 0;
   while (iTeam < 2) {
      float fHighest = 0.0;
      int i = 0;                // ADDED BY PMB FOR COMPILING UNDER LINUX
      for (i = 0; i < MAX_NODES; i++)
         if (InfoNodes[i].fDanger[iTeam] > fHighest)
            fHighest = InfoNodes[i].fDanger[iTeam];

      if (fHighest < 0.8) {
         iTeam++;
         continue;              // no need to rescale
      }
      // Check how much we passed the limit
      float fLimit = 0.7 / fHighest;

      // Now rescale all
      for (i = 0; i < MAX_NODES; i++)
         if (InfoNodes[i].fDanger[iTeam] > 0.0)
            InfoNodes[i].fDanger[iTeam] *= fLimit;      // rescale

      iTeam++;
      if (iTeam > 1)
         break;
   }
}

// Pathfinder
void cNodeMachine::path(int iFrom, int iTo, int iPathId, cBot * pBot,
                        int iFlags) {
   // Will create a path from iFrom to iTo, and store it into index number iPath

   if (iFrom < 0 || iTo < 0 || iPathId < 0)
      return;                   // do not create a path

   if (iFrom > iMaxUsedNodes || iTo > iMaxUsedNodes)
      return;

   if (Nodes[iFrom].origin == Vector(9999, 9999, 9999))
      return;

   if (Nodes[iTo].origin == Vector(9999, 9999, 9999))
      return;

   int path_index = 0, i;

   // clear path
   for (i = 0; i < MAX_NODES; i++)
      if (iPath[iPathId][i] < 0) {
         break;
      } else
         iPath[iPathId][i] = -1;

   // INIT: Start

   // First, all waypoints are closed.
   for (i = 0; i < MAX_NODES; i++) {
      astar_list[i].cost = 0;
      astar_list[i].parent = -1;
      astar_list[i].state = CLOSED;
   }

   // Our start waypoint is open
   astar_list[iFrom].state = OPEN;
   astar_list[iFrom].parent = iFrom;
   astar_list[iFrom].cost = func_distance(Nodes[iFrom].origin, Nodes[iTo].origin);

   bool valid = true;           // is it still valid to loop through the lists for pathfinding?
   bool succes = false;         // any succes finding the goal?

   // INIT: End
   // PATHFINDER: Start
   while (valid) {
      bool found_one_to_open = false;

      // go through all open waypoints
      for (i = 0; i < MAX_NODES; i++)
         if (Nodes[i].origin != Vector(9999, 9999, 9999))
            if (astar_list[i].state == OPEN) {
               if (i == iTo) {
                  // OPENED: This Node is our destination
                  succes = true;
                  valid = false;        // not valid to check anymore
                  break;        // Get out of here
               }

               // open this waypoint
               int the_wpt = -1;

               double lowest_cost = 99999999;
               double cost = 99999999;

               // Now loop from here through all closed waypoints
               int j = 0;       // <-- ADDED BY PMB FOR ISO COMPLIANCE (ELSE LINUX GOES MAD)
               
			   for (j = 0; j < MAX_NEIGHBOURS; j++) {
                  if (Nodes[i].iNeighbour[j] > -1) {
                     int nNode = Nodes[i].iNeighbour[j];
                     if (astar_list[nNode].state == CLOSED) {
                        cost =
                           astar_list[i].cost + 
                           (double) func_distance(Nodes[nNode].origin,
                                                  Nodes[iTo].origin);

                        int iTeam = UTIL_GetTeam(pBot->pEdict);

						float dangerCost = InfoNodes[i].fDanger[iTeam] * cost;
						float contactCost = InfoNodes[i].fContact[iTeam] * cost;

                        cost += dangerCost;
                        cost += contactCost;

                        if (cost < lowest_cost) {
                           the_wpt = nNode;
                           lowest_cost = cost;
                        }
                     }
                  }             // connected
               }                // for j

               // this waypoint has the lowest cost to get to the next best waypoint
               if (the_wpt > -1) {
                  j = the_wpt;
                  astar_list[j].state = OPEN;
                  astar_list[j].cost = lowest_cost;
                  astar_list[j].parent = i;
                  found_one_to_open = true;
               } else {
			   
			   }

               //}
            }
      // when no closed waypoints found
      if (found_one_to_open == false)
         valid = false;

   }
   // PATHFINDER: End

   // RESULT: Success
   if (succes) {
      // Build path (from goal to start, read out parent waypoint to backtrace)
      int temp_path[MAX_PATH_NODES];

      // INIT: Start
      for (i = 0; i < MAX_PATH_NODES; i++)
         temp_path[i] = -1;

      // The path has been built yet?
      bool built = false;

      // The variables needed to backtrace
      // wpta = waypoint we use to backtrace (starting at goal)
      // p = index for temp_path (the path will be GOAL-START, reversed later)
      int wpta = iTo, p = 0;

      // INIT: End

      // START: When path is not built yet
      while (built == false) {
         temp_path[p] = wpta;   // Copy the waypoint into temp_path[index]

         // IF: At current (start) waypoint
         if (wpta == iFrom) {
            // Check if we did not already had this waypoint before
            built = true;       // We finished building this path.
         } else {
            // Whenever wpta is containing bad information...
            if (wpta < 0 || wpta > MAX_NODES)
               break;           // ...get out aswell
         }

         // waypoint we use to backtrace will be set to parent waypoint.
         wpta = astar_list[wpta].parent;

         // Increase index for temp_path
         p++;

         // Whenever we reach the limit, get out.
         if (p >= MAX_PATH_NODES)
            break;

      }

      // When using RB_MESSAGES, we see this:
      /*
         char msg[80];
         sprintf(msg, "PATH A*:I have %d NODES in path.\n", p);
         BotDebug(msg);
         UTIL_ClientPrintAll(HUD_PRINTNOTIFY, msg);
       */
      // End Of notification

      // INIT: Start
      path_index = 0;           // done above, but done again to be sure
      // INIT: End

      // Now set the path up correctly
      //DEBUG: Bad bug by Greg found
      //DEBUG: Probably found bug due wrong call, DO NOT START AT MAX_PATH_NODES.
      for (i = (MAX_NODES - 1); i > -1; i--) {
         if (temp_path[i] < 0)
            continue;

         iPath[iPathId][path_index] = temp_path[i];
         path_index++;
      }

      if (pBot != NULL) {
         pBot->bot_pathid = 0;
      }

   } else {
      return;                   // no succes
   }

   iPath[iPathId][path_index] = iTo;    // terminate path
   path_index++;
   iPath[iPathId][path_index] = -1;     // terminate path

   // set timer
   pBot->fMoveToNodeTime = gpGlobals->time + 3;

   return;                      // path found
}

// Find a node which has almost no danger!
int cNodeMachine::node_camp(Vector vOrigin, int iTeam) {
   // Use Meredians to search for nodes
   int iX, iY;
   VectorToMeredian(vOrigin, &iX, &iY);
   float fDanger = 2.0;
   float fDistance = 9999;
   int iVisibility = 9999;
   int iBestNode = -1;

   // Theory:
   // Find a node, close, and less danger...
   // and with less visibility

   if (iX > -1 && iY > -1) {
      // Search in this meredian
      for (int i = 0; i < NODES_MEREDIANS; i++)
         if (Meredians[iX][iY].iNodes[i] > -1) {
            int iNode = Meredians[iX][iY].iNodes[i];

            if (Nodes[iNode].iNodeBits & BIT_WATER)
               continue;        // next node, do not camp under water!

            if (InfoNodes[iNode].fDanger[iTeam] < fDanger)
               if (func_distance(vOrigin, Nodes[iNode].origin) < fDistance) {
                  // Calculate visibility
                  int iVis = -1;

                  for (int iVisNr = 0; iVisNr < iNode; iVisNr++)
                     if (GetVisibilityFromTo(iNode, iVisNr) == VIS_VISIBLE)     // BERKED
                        iVis++;

                  if (iVis < 0)
                     iVis = 99999;

                  if (iVis < iVisibility) {
                     iBestNode = iNode;
                     fDistance =
                        func_distance(vOrigin, Nodes[iNode].origin);
                     fDanger = InfoNodes[iNode].fDanger[iTeam];
                     iVisibility = iVis;
                  }
               }
         }
   }

   return iBestNode;

}

// Check if iFrom is visible from other nodes (and opposite)
void cNodeMachine::vis_calculate(int iFrom) {
   // Check around your area to see what is visible
   float fClosest = 1024;
   for (int i = 0; i < MAX_NODES; i++)
      if ((i != iFrom) && (Nodes[i].origin != Vector(9999, 9999, 9999))) {
         float fDistance =
            func_distance(Nodes[i].origin, Nodes[iFrom].origin);
         if (fDistance < fClosest) {
            TraceResult tr;
            if (GetVisibilityFromTo(iFrom, i) == VIS_UNKNOWN)   // BERKED
            {
               UTIL_TraceHull(Nodes[iFrom].origin, Nodes[i].origin,
                              ignore_monsters, point_hull, NULL, &tr);

               if (tr.flFraction < 1.0) {
                  SetVisibilityFromTo(iFrom, i, false);
                  SetVisibilityFromTo(i, iFrom, false);
               } else {
                  SetVisibilityFromTo(iFrom, i, true);
                  SetVisibilityFromTo(i, iFrom, true);
               }
            }
         }
      }
}

// Find a node to look at when camping
int cNodeMachine::node_look_camp(Vector vOrigin, int iTeam,
                                 edict_t * pEdict) {
   float fDanger = -0.1;
   float fDistance = 0;
   int iBestNode = -2;

   // Theory:
   // Find a node, far, and a lot danger...
   int iFrom = close(vOrigin, 75, pEdict);
   // Search in this meredian
   for (int i = 0; i < MAX_NODES; i++) {
      int iNode = i;
      if (InfoNodes[iNode].fDanger[iTeam] > fDanger)
         if (func_distance(vOrigin, Nodes[iNode].origin) > fDistance) {
            // all nodes within this range may be tracelined
            bool bVisible = true;
            TraceResult tr;
            if (GetVisibilityFromTo(iFrom, iNode) == VIS_UNKNOWN)       // BERKED
            {
               UTIL_TraceLine(Nodes[iFrom].origin, Nodes[iNode].origin,
                              ignore_monsters, ignore_glass, pEdict, &tr);
               //UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NODE, MEREDIAN: VISIBILITY NOT KNOWN \n");
               if (tr.flFraction < 1.0) {
                  bVisible = false;
                  // Set to false
                  SetVisibilityFromTo(iFrom, iNode, false);
                  SetVisibilityFromTo(iNode, iFrom, false);
               } else {
                  SetVisibilityFromTo(iFrom, iNode, true);
                  SetVisibilityFromTo(iNode, iFrom, true);
               }

            } else {
               if (GetVisibilityFromTo(iFrom, iNode) == VIS_BLOCKED)    // BERKED
               {
                  bVisible = false;
                  //UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NODE, MEREDIAN: VISIBILITY KNOWN / NOT VISIBLE\n");
               }
            }

            // Visible
            if (bVisible) {
               //UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NODE, MEREDIAN: FOUND LOOK@ NODE \n");
               iBestNode = iNode;
               fDistance = func_distance(vOrigin, Nodes[iNode].origin);
               fDanger = InfoNodes[iNode].fDanger[iTeam];
            }
         }
   }
   /*
      if (iBestNode < 0)
      UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NODE: Could not find a node to look at, camping\n");
      else
      {
      UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NODE: SUCCES, to look at a node, camping\n");
      }
    */
   return iBestNode;
}

// Walk the path Neo
void cNodeMachine::path_walk(cBot * pBot, float moved_distance) {
   int BotIndex = pBot->iIndex;

   // Check if path is valid
   if (iPath[BotIndex][0] < 0) {
      pBot->f_move_speed = 0.0; // do not move
      return;                   // back out
   }
   // If we should wait, we do nothing here
   if (pBot->f_wait_time > gpGlobals->time)
      return;

   pBot->f_move_speed = pBot->f_max_speed;

   // 25/06/04 - Stefan - Updated
   // When our leader walks slow, we walk slow
   /*
   if ((pBot->pSwatLeader != NULL) && (IsAlive(pBot->pSwatLeader))) {
      // Only immitate the player movements when he is MOVING (ie, do not duck
      // when the leader is sitting in a corner taking position)
      if (FUNC_PlayerSpeed(pBot->pSwatLeader) > 20)     // upped, a slightest movement caused improper behaviour
      {
         if (pBot->pSwatLeader->v.button & IN_DUCK)
            pBot->f_hold_duck = gpGlobals->time + 0.5;

         pBot->f_move_speed = FUNC_PlayerSpeed(pBot->pSwatLeader);
      }
   }*/
   // Walk the path
   int iCurrentNode = iPath[BotIndex][pBot->bot_pathid];        // Node we are heading for
   int iNextNode = iPath[BotIndex][pBot->bot_pathid + 1];       // EVY TESTING
   //  int iNextNode = iPath[BotIndex][pBot->bot_pathid + 3];

   // when pButtonEdict is filled in, we check if we are close!
   if (pBot->pButtonEdict) {
      Vector vButtonVector = VecBModelOrigin(pBot->pButtonEdict);

      float fDistance = 90;
      bool bTrigger = false;

      if (strcmp
            (STRING(pBot->pButtonEdict->v.classname),
             "trigger_multiple") == 0) {
         fDistance = 32;
         bTrigger = true;
      }

      if (func_distance(pBot->pEdict->v.origin, vButtonVector) < fDistance) {
         TraceResult trb;
         // TRACELINE ON PURPOSE!
         if (bTrigger)
            UTIL_TraceLine(pBot->pEdict->v.origin, vButtonVector,
                           dont_ignore_monsters, dont_ignore_glass,
                           pBot->pEdict, &trb);
         else
            UTIL_TraceLine(pBot->pEdict->v.origin, vButtonVector,
                           ignore_monsters, dont_ignore_glass,
                           pBot->pEdict, &trb);

         bool isGood = false;

         // if nothing hit:
         if (trb.flFraction >= 1.0)
            isGood = true;
         else {
            // we hit this button we check for
            if (trb.pHit == pBot->pButtonEdict)
               isGood = true;
         }

         if (isGood || bTrigger) {
            pBot->vHead = vButtonVector;
            pBot->vBody = pBot->vHead;

            // kill edict in memory
            pBot->pButtonEdict = NULL;

            // press use
            if (bTrigger == false)
               UTIL_BotPressKey(pBot, IN_USE);

            SERVER_PRINT
            ("This button i was looking for, is close, i can see it, i use it!!!\n");

            // wait a little
            pBot->f_wait_time = gpGlobals->time + 0.5;
            pBot->fButtonTime = gpGlobals->time + 5.0;
            pBot->f_node_timer = gpGlobals->time + 3;
            pBot->bot_pathid = -1;
            return;
         } else
            SERVER_PRINT("TRACELINE FUCKED UP!\n");
         return;
      }
   }
   // end of path
   if (iCurrentNode < 0) {
      // When f_cover_time is > gpGlobals, we are taking cover
      // so we 'wait'
      if (pBot->f_cover_time > gpGlobals->time) {
         if (pBot->pBotEnemy != NULL && pBot->v_enemy != Vector(0, 0, 0))
            pBot->vHead = pBot->v_enemy;

         pBot->f_wait_time = pBot->f_cover_time - RANDOM_FLOAT(0.0, 3.0);
         pBot->f_cover_time = gpGlobals->time;

         if (RANDOM_LONG(0, 100) < 75)
            pBot->f_hold_duck = gpGlobals->time + RANDOM_FLOAT(1.0, 4.0);
      } else {
         if (pBot->iPathFlags == PATH_CAMP) {
            // Camp
            pBot->f_camp_time = gpGlobals->time + RANDOM_FLOAT(10, 30);

            if (RANDOM_LONG(0, 100) < pBot->ipFearRate)
               pBot->iPathFlags = PATH_DANGER;
            else
               pBot->iPathFlags = PATH_NONE;    // do not go to contact areas

            // RADIO: I am in position!
            if (FUNC_DoRadio(pBot)) {
               if (pBot->pSwatLeader != NULL)
                  UTIL_BotRadioMessage(pBot, 3, "5", "");
               else if (BOT_IsLeader(pBot))
                  UTIL_BotRadioMessage(pBot, 1, "3", "");       // "HOLD THIS POSITION"
            }



         } else {
            // Set on camp mode
            if (RANDOM_LONG(0, 100) < pBot->ipCampRate
                  && pBot->f_camp_time + ((100 - pBot->ipCampRate) / 2) <
                  gpGlobals->time && FUNC_AmountHostages(pBot) < 1)
               pBot->iPathFlags = PATH_CAMP;
         }

         if (Game.bBombPlanted && Game.bBombDiscovered == false) {
            int iGoalType = goal_from_node(pBot->iGoalNode);

            if (iGoalType == GOAL_BOMBSPOT && pBot->iTeam == 2) {
               if (pBot->Defuse() == false) {
                  // sector clear!
                  if (FUNC_DoRadio(pBot))
                     UTIL_BotRadioMessage(pBot, 3, "4", "");

                  // find the corresponding goal
                  /*
                     for (int g=0; g < MAX_GOALS; g++)
                     {
                     if (Goals[g].iType == GOAL_BOMBSPOT && Goals[g].iNode == pBot->iGoalNode)
                     {
                     Goals[g].iChecked++;
                     }
                     }                                                                            
                   */
               }
            } else if (iGoalType == GOAL_IMPORTANT) {
               // find the corresponding goal
               /*
                  for (int g=0; g < MAX_GOALS; g++)
                  {
                  if (Goals[g].iType == GOAL_IMPORTANT && Goals[g].iNode == pBot->iGoalNode)
                  {
                  Goals[g].iChecked++;
                  }
                  }
                */
            }

         }
         // reached the end
         pBot->iGoalNode = -1;
         pBot->bot_pathid = -1;
      }

      // get out.
      return;

   }
   // Near Node
   bool bNearNode = false;
   bool bReachNode = false;

   Vector vEdict, vNode;
   vEdict = pBot->pEdict->v.origin;
   vNode = Nodes[iCurrentNode].origin;

#ifdef EVY_IS_WRONG
   // Make same height
   vEdict.z = 0;
   vNode.z = 0;
#endif

   TraceResult tr;
   Vector vOrigin = pBot->pEdict->v.origin;
   edict_t *pEntityHit = NULL;


   //Using TraceHull to detect de_aztec bridge and other entities.
   //DONT_IGNORE_MONSTERS, we reached it only when there are no other bots standing in our way!
   //UTIL_TraceHull(vOrigin, vNode, dont_ignore_monsters, point_hull, pBot->pEdict, &tr);
   //UTIL_TraceHull(vOrigin, vNode, dont_ignore_monsters, human_hull, pBot->pEdict, &tr);
   UTIL_TraceHull(vOrigin, vNode, dont_ignore_monsters, head_hull,
                  pBot->pEdict, &tr);

   // if nothing hit:
   if (tr.flFraction >= 1.0)
      bReachNode = true;
   else {
      // set entity info we hit
      if (tr.pHit)
         pEntityHit = tr.pHit;
   }

   if (pBot->f_strafe_time < gpGlobals->time)
      pBot->vBody = Nodes[iCurrentNode].origin;

   // Overwrite vBody when we are very close to the bomb.
   // FIXED: Terrorists only
   if (pBot->iTeam == 1)
      if (Game.vDroppedC4 != Vector(9999, 9999, 9999))
         if (VectorIsVisibleWithEdict
               (pBot->pEdict, Game.vDroppedC4, "weaponbox"))
            if (func_distance(pBot->pEdict->v.origin, Game.vDroppedC4) <
                  200)
               pBot->vBody = Game.vDroppedC4;

   if (FUNC_IsOnLadder(pBot->pEdict)) {
      // Set touch radius
      pBot->f_strafe_speed = 0.0;       // we may not strafe
      pBot->f_move_speed = (pBot->f_max_speed / 2);     // move speed
      //pBot->pEdict->v.button |= IN_DUCK;                    // duck

      // Look at the waypoint we are heading to.
      pBot->vHead = Nodes[iCurrentNode].origin;
      pBot->vBody = Nodes[iCurrentNode].origin;

      // Press forward key to move on ladder
      UTIL_BotPressKey(pBot, IN_FORWARD);

      if (BotShouldDuck(pBot)) {
         UTIL_BotPressKey(pBot, IN_DUCK);
         pBot->f_hold_duck = gpGlobals->time + 0.2;
      }

      if (func_distance(vEdict, vNode) < 25)
         bNearNode = true;
   } else {
      if (Nodes[iCurrentNode].iNodeBits & BIT_LADDER) {
         // Going to a ladder waypoint
         if (func_distance(vEdict, vNode) < 25)
            bNearNode = true;
      } else {
         if (func_distance(vEdict, vNode) < 40)
            bNearNode = true;
      }

      // If we should duck, duck.
      if (BotShouldDuck(pBot)) {
         UTIL_BotPressKey(pBot, IN_DUCK);
         pBot->f_hold_duck = gpGlobals->time + 0.2;
      }
   }

   if (bNearNode) {
      pBot->bot_pathid++;
      // Calculate vis table from here
      vis_calculate(iCurrentNode);

      // calculate how long we should take to get to next node
      // nodes are mostly 90 units apart from each other. We give the bot 2 seconds
      // to get to the next node.
      pBot->fMoveToNodeTime = gpGlobals->time + 3.0;
   }
   // TODO TODO TODO Water Navigation

   // NO ENEMY, CHECK AROUND AREA
   if (pBot->pBotEnemy == NULL) {
      if (iNextNode > -1)
         pBot->vHead = Nodes[iNextNode].origin;
      else
         pBot->vHead = Nodes[iCurrentNode].origin;
   }
   // Jump over possible gaps, this is when a node is floating..
   /*
      if (node_float(Nodes[iCurrentNode].origin))
      {
      // check if we are going to 'fall'.
      TraceResult tr;
      Vector v_jump, v_source, v_dest;
      edict_t *pEdict = pBot->pEdict;

      // convert current view angle to vectors for TraceLine math...

      v_jump = pEdict->v.v_angle;
      v_jump.x = 0;  // reset pitch to 0 (level horizontally)
      v_jump.z = 0;  // reset roll to 0 (straight up and down)

      UTIL_MakeVectors( v_jump );

      // use center of the body first...

      // check if the bot is going to 'fall'
      v_source = pEdict->v.origin + Vector(0,0,32);
      v_dest = v_source + gpGlobals->v_forward * 45;

      v_dest = v_dest - Vector(0, 0, 90);

      // trace a line forward at duck height...
      UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
      pEdict->v.pContainingEntity, &tr);

      //WaypointDrawBeam(listenserver_edict, v_source, v_dest, 15, 0, 255, 255, 255, 255, 5);

      // if trace DID NOT hit something, return FALSE
      if (tr.flFraction >= 1.0 && pBot->f_jump_time < gpGlobals->time)
      {
      // we are going to fall
      // JUMP baby
      UTIL_BotPressKey(pBot, IN_JUMP);
      pBot->f_jump_time = gpGlobals->time + 0.3;
      UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "NodeMachine: Connected jump i think.\n");
      }

      }
    */

   // When longer not stuck for some time we clear stuff
   if (pBot->fNotStuckTime + 2 < gpGlobals->time) {
      pBot->iDuckTries = 0;
      pBot->iJumpTries = 0;
   }
   // When we have to many duck/jump tries, we reset path stuff
   if (pBot->iDuckTries > 7 || pBot->iJumpTries > 7) {
      pBot->bot_pathid = -1;    // reset path
      return;
   }

   bool bShouldMove = false;
   if (pBot->f_freeze_time + 3 < gpGlobals->time
         && pBot->f_stuck_time < gpGlobals->time
         && pBot->f_camp_time < gpGlobals->time
         && pBot->f_wait_time < gpGlobals->time
         && pBot->f_c4_time < gpGlobals->time
         && pBot->f_jump_time + 1 < gpGlobals->time)
      bShouldMove = true;       // according to our timers we should move

   // When blocked by an entity, we should figure out why:
   if (pEntityHit && pBot->pButtonEdict == NULL && bShouldMove &&
         pBot->fButtonTime < gpGlobals->time) {
      // hit by a door?
      bool bDoor = false;

      // normal door (can be used as an elevator)
      if (strcmp(STRING(pEntityHit->v.classname), "func_door") == 0)
         bDoor = true;
      // I am not 100% sure about func_wall, but include it anyway
      if (strcmp(STRING(pEntityHit->v.classname), "func_wall") == 0)
         bDoor = true;
      // rotating door
      if (strcmp(STRING(pEntityHit->v.classname), "func_door_rotating") ==
            0)
         bDoor = true;

      if (bDoor) {
         // check if we have to 'use' it
         if (FBitSet(pEntityHit->v.spawnflags, SF_DOOR_USE_ONLY)
               &&
               !(FBitSet(pEntityHit->v.spawnflags, SF_DOOR_NO_AUTO_RETURN))) {
            // use only, press use and wait
            pBot->vHead = VecBModelOrigin(pEntityHit);
            pBot->vBody = pBot->vHead;
            UTIL_BotPressKey(pBot, IN_USE);
            pBot->f_wait_time = gpGlobals->time + 0.5;
            pBot->fButtonTime = gpGlobals->time + 5;
            pBot->pButtonEdict = NULL;

            // TODO: when this door is opened by a trigger_multiple (on touch)
            // then we do not have to wait and press USE at all.

            //SERVER_PRINT("We have to use this door in order to get it opened!\n");
            return;
         }
         // this thing has a name (needs button to activate)
         if (STRING(pEntityHit->v.targetname)) {
            // find this entity
            edict_t *pButtonEdict = NULL;
            edict_t *pent = NULL;
            TraceResult trb;


            // search for all buttons
            while ((pent =
                       UTIL_FindEntityByClassname(pent,
                                                  "func_button")) != NULL) {
               // skip anything that could be 'self' (unlikely)
               if (pent == pEntityHit)
                  continue;

               /*
                  SERVER_PRINT("FUNC_DOOR: target-> '");
                  SERVER_PRINT(STRING(pent->v.target));
                  SERVER_PRINT("' (Targetname='");
                  SERVER_PRINT(STRING(pEntityHit->v.targetname));
                  SERVER_PRINT("'\n");
                */

               // get vectr
               Vector vPentVector = VecBModelOrigin(pent);

               // found button entity
               if (strcmp
                     (STRING(pent->v.target),
                      STRING(pEntityHit->v.targetname)) == 0) {
                  UTIL_TraceLine(pBot->pEdict->v.origin, vPentVector,
                                 ignore_monsters, dont_ignore_glass,
                                 pBot->pEdict, &trb);


                  bool isGood = false;

                  // if nothing hit:
                  if (trb.flFraction >= 1.0)
                     isGood = true;
                  else {
                     // we hit this button we check for
                     if (trb.pHit == pent)
                        isGood = true;
                  }

                  if (isGood) {
                     // Button found to head for!
                     pButtonEdict = pent;
                     break;
                  } else {
                     // we failed here
                     // it is probably a button 'on the other side of the wall'
                     // as most doors have 2 buttons to access it (ie prodigy)
                     //SERVER_PRINT("TRACELINE FAILS\n");
                  }
               }
            }                   // while

            if (pButtonEdict == NULL) {
               // TOUCH buttons (are not func_button!)
               pent = NULL;
               // search for all buttons
               while ((pent =
                          UTIL_FindEntityByClassname(pent,
                                                     "trigger_multiple")) !=
                      NULL) {
                  // skip anything that could be 'self' (unlikely)
                  if (pent == pEntityHit)
                     continue;

                  /*
                     SERVER_PRINT("TRIGGER_MULTIPLE: target-> '");
                     SERVER_PRINT(STRING(pent->v.target));
                     SERVER_PRINT("' (Targetname='");
                     SERVER_PRINT(STRING(pEntityHit->v.targetname));
                     SERVER_PRINT("'\n");
                   */

                  // get vectr
                  Vector vPentVector = VecBModelOrigin(pent);

                  // found button entity
                  if (strcmp
                        (STRING(pent->v.target),
                         STRING(pEntityHit->v.targetname)) == 0) {
                     //SERVER_PRINT("Found a match, going to check if we can reach it!\n");

                     UTIL_TraceHull(pBot->pEdict->v.origin, vPentVector,
                                    dont_ignore_monsters, point_hull,
                                    pBot->pEdict, &trb);

                     bool isGood = false;

                     // if nothing hit:
                     if (trb.flFraction >= 1.0)
                        isGood = true;
                     else {
                        // we hit this button we check for
                        if (trb.pHit == pent)
                           isGood = true;
                     }

                     if (isGood) {
                        // Button found to head for!
                        pButtonEdict = pent;
                        break;
                     } else {
                        // we failed here
                        // it is probably a button 'on the other side of the wall'
                        // as most doors have 2 buttons to access it (ie prodigy)

                        // hits by worldspawn here
                        if (strcmp
                              (STRING(trb.pHit->v.classname),
                               "worldspawn") == 0) {
                           // DE_PRODIGY FIX:
                           // Somehow the button is not detectable. Find a node, that is close to it.
                           // then retry the traceline. It should NOT hit a thing now.
                           // On success, it is still our button
                           int iClose =
                              close(vPentVector, NODE_ZONE, pent);

                           if (iClose > -1) {
                              // retry the tracehull
                              UTIL_TraceHull(pBot->pEdict->v.origin,
                                             node_vector(iClose),
                                             dont_ignore_monsters,
                                             point_hull,
                                             pBot->pEdict, &trb);

                              // if nothing hit:
                              if (trb.flFraction >= 1.0) {
                                 pButtonEdict = pent;
                                 //                      SERVER_PRINT("I don't like it, but i have detected it anyways!");
                                 break;
                              }
                           }
                        }

                     }
                  }
               }                // while
            }
            // We have found a button to go to
            if (pButtonEdict) {
               // Get its vector
               Vector vButtonVector = VecBModelOrigin(pButtonEdict);

               // Search a node close to it
               int iButtonNode =
                  close(vButtonVector, NODE_ZONE, pButtonEdict);

               // When node found, create path to it
               if (iButtonNode > -1) {
                  // Get current node
                  int iCurrentNode =
                     close(pBot->pEdict->v.origin, NODE_ZONE,
                           pBot->pEdict);

                  // when valid...
                  if (iCurrentNode > -1) {
                     pBot->bot_pathid = -1;
                     path(iCurrentNode, iButtonNode, pBot->iIndex, pBot,
                          PATH_NONE);
                     pBot->pButtonEdict = pButtonEdict;
                     return;
                  }

               }                // button node
            }                   // pButtonEdict found
         }
      }
   }


   // When we run out of time, we move back to previous to try again
   if (pBot->fMoveToNodeTime < gpGlobals->time) {
      // We have trouble with this one somehow? Check for nearby players before
      // we judge its done by worldspawn.
      bool bPlayersNear = BOOL_search_near_players(pBot);

      // when we recorded a jump, this might help
      if (Nodes[iCurrentNode].iNodeBits & BIT_JUMP)
         UTIL_BotPressKey(pBot, IN_JUMP);

      // Not near players, and we should move!!
      if (bPlayersNear == false && bShouldMove && pBot->bot_pathid > 0) {
         // Add this connection to the 'troubled ones'
         int iFrom = iPath[pBot->iIndex][pBot->bot_pathid - 1];
         int iTo = iCurrentNode;

         if (TroubleExists(iFrom, iTo) < 0) {
            rblog("cNodeMachine: Added a trouble connection\n");
            AddTrouble(iFrom, iTo);
         } else {
            // Troubled connection already known, make it more trouble!
            IncreaseTrouble(iFrom, iTo);
            rblog
            ("cNodeMachine: Increasing trouble on specific connection\n");

            if (TroubleIsTrouble(iFrom, iTo)) {
               // remove connection.
               rblog
               ("cNodeMachine: Connection caused to much trouble. Removed connection\n");
               remove_neighbour_node(iFrom, iTo);
               RemoveTrouble(iFrom, iTo);
            }
         }
      }
      pBot->bot_pathid = -1;
      pBot->fMoveToNodeTime += 3.0;
      pBot->iGoalNode = -1;
      return;
   }
   // When not moving (and we should move):
   // - learn from mistakes
   // - unstuck
   // - go back in path...
   if (moved_distance < 0.5 && bShouldMove) {
      cBot *pBotStuck = search_near_players(pBot);
      bool bPlayersNear = BOOL_search_near_players(pBot);
      pBot->fNotStuckTime = gpGlobals->time;

      // JUMP & DUCK
      if (BotShouldJump(pBot)
            || (Nodes[iCurrentNode].iNodeBits & BIT_JUMP)) {
         //UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "NodeMachine: I should jump.\n");
         UTIL_BotPressKey(pBot, IN_JUMP);
         // stay focussed with body and head to this vector
         pBot->vHead = Nodes[iCurrentNode].origin;
         pBot->vBody = Nodes[iCurrentNode].origin;
         pBot->iJumpTries++;
         pBot->fMoveToNodeTime += gpGlobals->time + 1;
      } else if (BotShouldDuck(pBot)) {
         //UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "NodeMachine: I should duck.\n");
         UTIL_BotPressKey(pBot, IN_DUCK);
         pBot->f_hold_duck = gpGlobals->time + 0.2;
         pBot->iDuckTries++;
         pBot->fMoveToNodeTime += gpGlobals->time + 1;
      } else {
         // check if the connection we want is going up
         // - when going up
         // - when we should move
         // - when no players are close (could be blocked by them, do not learn stupid things)
         if (Nodes[iCurrentNode].origin.z > pBot->pEdict->v.origin.z &&
               bShouldMove && pBotStuck == NULL) {
            // check if the next node is floating (skip self)
            if (node_float(Nodes[iCurrentNode].origin, pBot->pEdict)) {
               // it floats, cannot reach
               // Our previous node in the list sent us here, so disable that connection
               int iCurrentPathId = pBot->bot_pathid - 1;
               int iPrevNode = -1;
               if (iCurrentPathId > -1)
                  iPrevNode = iPath[BotIndex][iCurrentPathId];

               // Find the neighbour connection, and remove it
               for (int n = 0; n < MAX_NEIGHBOURS; n++)
                  if (Nodes[iPrevNode].iNeighbour[n] == iCurrentNode) {
                     Nodes[iPrevNode].iNeighbour[n] = -1;       // disable this connection
                     //UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "NodeMachine: Fixed connection ------- #1.\n");
                     // be sure we reset our path stuff
                     pBot->bot_pathid = -1;
                     break;     // get out
                  }
            } else {
               // When this node is not floating, it could be something we should jump to.
               // somehow we cannot reach that node, so we fix it here.
               int iCurrentPathId = pBot->bot_pathid - 1;
               int iPrevNode = -1;
               if (iCurrentPathId > -1)
                  iPrevNode = iPath[BotIndex][iCurrentPathId];

               // Find the neighbour connection, and remove it
               for (int n = 0; n < MAX_NEIGHBOURS; n++)
                  if (Nodes[iPrevNode].iNeighbour[n] == iCurrentNode) {
                     Nodes[iPrevNode].iNeighbour[n] = -1;       // disable this connection
                     //UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "NodeMachine: Fixed connection ------ #2.\n");
                     // be sure we reset our path stuff
                     break;     // get out
                  }
               //                    pBot->iGoalNode = -1;
               //pBot->bot_pathid = -1;
               pBot->fWanderTime = gpGlobals->time + 0.1;
            }

         }
         // Any other connections to 'learn'?
         // Check if we are stuck with other players
         if (bPlayersNear && bShouldMove) {
            // Depending on how the other bot looks, act
            // pBotStuck -> faces pBot , do same
            // pBotStuck -> cannot see pBot, do opposite
            // Check if pBotStuck can see pBot (pBot can see pBotStuck!)
            int angle_to_player = 40;

            if (pBotStuck != NULL)
               angle_to_player =
                  FUNC_InFieldOfView(pBot->pEdict,
                                     (pBotStuck->pEdict->v.origin -
                                      pBot->pEdict->v.origin));

            bool bReverse = false;
            if (angle_to_player > 45)
               bReverse = true;

            // Method: both bots do exactly the same?
            if (RANDOM_LONG(0, 100) < 50) {
               pBot->f_strafe_speed = pBot->f_max_speed;
               if (pBotStuck != NULL)
                  if (bReverse)
                     pBotStuck->f_strafe_speed = pBotStuck->f_max_speed;
                  else
                     pBotStuck->f_strafe_speed = -pBotStuck->f_max_speed;
            } else {
               pBot->f_strafe_speed = -pBot->f_max_speed;
               if (pBotStuck != NULL)
                  if (bReverse)
                     pBotStuck->f_strafe_speed = pBotStuck->f_max_speed;
                  else
                     pBotStuck->f_strafe_speed = -pBotStuck->f_max_speed;
            }

            pBot->f_strafe_time = gpGlobals->time + 1.6;
            pBot->f_goback_time = RANDOM_FLOAT(0.5, 1.5);

            if (pBotStuck != NULL) {
               pBotStuck->f_strafe_time = gpGlobals->time + 0.8;
               pBot->f_stuck_time = gpGlobals->time + 0.2;
               pBotStuck->f_stuck_time = gpGlobals->time + 0.2;

               if (bReverse) {
                  pBotStuck->f_goback_time = gpGlobals->time;
                  pBotStuck->f_move_speed = pBotStuck->f_max_speed;
               } else
                  pBotStuck->f_goback_time =
                     gpGlobals->time + RANDOM_FLOAT(0.5, 1.5);

            }

            if (RANDOM_LONG(0, 100) < 50) {
               UTIL_BotPressKey(pBot, IN_JUMP);

               if (pBotStuck != NULL)
                  UTIL_BotPressKey(pBotStuck, IN_DUCK);

            } else {
               UTIL_BotPressKey(pBot, IN_DUCK);

               if (pBotStuck != NULL)
                  UTIL_BotPressKey(pBotStuck, IN_JUMP);
            }
         } else if (bShouldMove) {
            // we are stuck by 'world', we go back one step
            pBot->bot_pathid--;
            pBot->f_stuck_time = gpGlobals->time + 0.2;

            // When its not reachable disable goal
            //  if (bReachNode == false)
            //     pBot->iGoalNode = -1;

            if (RANDOM_LONG(0, 100) < 50) {
               //pBot->iGoalNode = -1;
               pBot->bot_pathid = -1;
            }
         }
      }                         // Cannot jump or duck
   }                            // Moved distance < 2.0
   else {
      // experimental:

      /*
         // JUMP & DUCK
         if (BotShouldJump (pBot))
         {
         //UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "NodeMachine: I should jump.\n");
         UTIL_BotPressKey (pBot, IN_JUMP);
         // stay focussed with body and head to this vector
         pBot->vHead = Nodes[iCurrentNode].origin;
         pBot->vBody = Nodes[iCurrentNode].origin;
         pBot->iJumpTries++;
         pBot->fMoveToNodeTime += gpGlobals->time + 1;
         }
         else */ if (BotShouldDuck(pBot))
      {
         //UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "NodeMachine: I should duck.\n");
         UTIL_BotPressKey(pBot, IN_DUCK);
         pBot->f_hold_duck = gpGlobals->time + 0.2;
         pBot->iDuckTries++;
         pBot->fMoveToNodeTime += gpGlobals->time + 1;
      }

   }
}

// Think about path creation here
void cNodeMachine::path_think(cBot * pBot, float moved_distance) {

   if (pBot->pBotHostage != NULL && pBot->CanSeeEntity(pBot->pBotHostage))
      return;                   // bot has hostage, can see hostage

   if (pBot->f_c4_time > gpGlobals->time) {
      //SERVER_PRINT("BOT: Not allowed to 'path_think', f_c4_time set.\n");
      return;
   }

/*
   if (pBot->fWanderTime > gpGlobals->time) {
      wander_think(pBot, moved_distance);       // don't be busy with paths and such, just do stupid
      pBot->bot_pathid = -1;
      //pBot->iGoalNode = -1;
      return;
   } */

   // When camping we do not think about paths, we think where to look at
   if (pBot->f_camp_time > gpGlobals->time) {
      if (pBot->iGoalNode == -1) {
         pBot->iGoalNode =
            node_look_camp(pBot->pEdict->v.origin,
                           UTIL_GetTeam(pBot->pEdict), pBot->pEdict);
      }

      return;
   }

   int BotIndex = pBot->iIndex;

   // When no path, create one.
   if (pBot->bot_pathid < 0) {
      pBot->f_move_speed = 0.0;

      // Find a new goal
      if (pBot->iGoalNode < 0 && pBot->pBotEnemy == NULL) {
         // 20/06/04 - stefan
		  /*
         if (pBot->pSwatLeader != NULL) {
            // The swat leader can be human or a bot. When it is a bot, we just get his goal
            // and the other bots move to the same goal. When it is human, we follow the
            // human player.

            cBot *BotPointer = UTIL_GetBotPointer(pBot->pSwatLeader);

            // it is a bot
            if (BotPointer) {
               pBot->iGoalNode = BotPointer->iGoalNode;
               pBot->bot_pathid = -1;

               if (pBot->iGoalNode < 0) {
                  pBot->fWanderTime = gpGlobals->time + 1.0;
               }
               // next frame we calculate path
               return;
            } 
		else {
               if (func_distance
                     (pBot->pSwatLeader->v.origin,
                      pBot->pEdict->v.origin) > NODE_ZONE * 4) {
                  pBot->iGoalNode =
                     close(pBot->pSwatLeader->v.origin, NODE_ZONE * 3,
                           pBot->pSwatLeader);

                  if (pBot->iGoalNode < 0) {
                     pBot->fWanderTime = gpGlobals->time + 1.0;
                  }
               } else {
                  // TODO: take in tactical positions?
                  if (FUNC_PlayerSpeed(pBot->pSwatLeader) < 40)
                     pBot->f_wait_time = gpGlobals->time + 2.0;
                  else
                     pBot->f_wait_time = gpGlobals->time + 0.5;
               }


               return;
            }
         } */
         // Depending on team we have a goal
         int iCurrentNode =
            close(pBot->pEdict->v.origin, 75, pBot->pEdict);

         if (iCurrentNode < 0) {
            iCurrentNode = add(pBot->pEdict->v.origin, 0, pBot->pEdict);
		 }

         if (iCurrentNode < 0) {
            pBot->fWanderTime = gpGlobals->time + 0.1;
            pBot->iGoalNode = -1;
            return;
         }

         bool bHasBomb = FUNC_BotHasWeapon(pBot, CS_WEAPON_C4);

         // Loop through all goals.
         int iFinalGoalNode = -1;
         long iFinalGoalScore = -1;
         int iFinalGoalID = -1;
         int iFinalGoalBadScore = 100;
         int iFinalGoalVisited = 9999;

         int iGn = 0;
         for (iGn = 0; iGn < MAX_GOALS; iGn++) {
            // Okay: A goal can gain a specific score, the higher the score the more we want to head for that goal
            // score depends on:
            // - favoriteness (the more bots head for it, the less score)
            // - danger (the more danger, the less score)
            // - objective (objective scores higher)
            // NOTE:
            // - distance (everything closer then xyz distance is bad)

            // Make sure this goal is valid
            if (Goals[iGn].iNode < 0)
               continue;

            // warning, do not use => or else we will be stuck to ONE goal for ALL bots (ouch)
            if (Goals[iGn].iBadScore > iFinalGoalBadScore)
               continue;

            if (Goals[iGn].iChecked > iFinalGoalVisited)
               continue;

            // step 1: check if any other bot already has this goal
            long iScore = 3200;
            for (int iBots = 0; iBots < 32; iBots++) {
               // decrease score when its famous
               // 17/07/04
               // should only be applicable when the other bot is in the same team...
               if ((bots[iBots].iGoalNode == Goals[iGn].iNode)
                     && (bots[iBots].iTeam == pBot->iTeam))
                  iScore -= 100;
            }

            // step 2: danger
            iScore -=
               (1000 -
                (int) InfoNodes[Goals[iGn].iNode].
                fDanger[UTIL_GetTeam(pBot->pEdict)] * 1000);

            // step 3:objective stuff & distance
            float fDistanceToGoal = func_distance(pBot->pEdict->v.origin,
                                                  node_vector(Goals[iGn].
                                                              iNode));


            if ((Goals[iGn].iType == GOAL_SPAWNCT
                  || Goals[iGn].iType == GOAL_SPAWNT)
                  && Game.bBombPlanted == false) {
               iScore += 1000;
               iScore *= (int) fDistanceToGoal / 8124.0;

               if (pBot->iTeam == 1 && Goals[iGn].iType == GOAL_SPAWNCT)
                  if (fDistanceToGoal > 500 && fDistanceToGoal < 750)
                     iScore += 4096;

               if (pBot->iTeam == 2 && Goals[iGn].iType == GOAL_SPAWNT)
                  if (fDistanceToGoal > 500 && fDistanceToGoal < 750)
                     iScore += 4096;
            } else if (Goals[iGn].iType == GOAL_BOMBSPOT) {

               // i carry bomb, or the bomb is planted and i am ct
               if ((pBot->iTeam == 1 && bHasBomb)
                     || (pBot->iTeam == 2 && Game.bBombPlanted)) {
                  iScore += 1000;
                  // the closer to a bombspot, the better, but not too close!
                  if (fDistanceToGoal > 500)
                     iScore =
                        iScore +
                        (iScore * (1 - (int) fDistanceToGoal / 2048.0));

                  // go to this bombspot!
                  if (pBot->iTeam == 1 && fDistanceToGoal < 750)
                     iScore += 10000;
               } else {
                  // When terror, a bombspot is okay
                  if (pBot->iTeam == 1)
                     iScore += 1500;
                  else
                     iScore += 550;

                  iScore *= (int) fDistanceToGoal / 8124.0;

               }
            } else if (Goals[iGn].iType == GOAL_HOSTAGE) {
               // when i am ct, this is very important
               if (pBot->iTeam == 2) {
                  if (fDistanceToGoal > 500)
                     iScore =
                        iScore +
                        (iScore * (1 - (int) fDistanceToGoal / 8124.0));

                  // go to this one
                  if (fDistanceToGoal < 500)
                     iScore += 5000;
               } else
                  iScore *= (int) fDistanceToGoal / 8124.0;
            } else if (Goals[iGn].iType == GOAL_IMPORTANT) {
               //iScore += RANDOM_LONG(0, 1024);

               // "important goals" should not always be chosen, somehow
               // i am not really confident how this works. This seems
               // to overrule everything or something. Yikes.
               // using ugly RANDOM_LONG to bypass most checks, but still
               // keep them in the game

               if (RANDOM_LONG(0, 100) < 25) {


                  if (fDistanceToGoal > 500)
                     iScore =
                        iScore +
                        (iScore * (1 - (int) fDistanceToGoal / 8124.0));
                  else
                     iScore *= (int) fDistanceToGoal / 8124.0;

               }

            }
            // 17/07/04
            // Basic attempts to handle other kind of goals...
            else if (Goals[iGn].iType == GOAL_VIPSAFETY)        // basic goals added for as_ maps
            {
               // Maximum importance when acting as CT should check whether we are the VIP!
               if (pBot->iTeam == 2)
                  iScore += 10000;
               else             // We are T
                  iScore *= (int) fDistanceToGoal / 8124.0;
            } else if (Goals[iGn].iType == GOAL_ESCAPEZONE)     // basic goals added for es_  maps
            {
               // Maximum importance when acting as T
               if (pBot->iTeam == 1)
                  iScore += 10000;
            }
            /*
                                      else if (Goals[iGn].iType == GOAL_WEAPON) // finding weapons only when they are close 
                                      {
            				  if (fDistanceToGoal > 500) // should also depends on whether bot has already a weapon
            					  iScore = iScore + (iScore*(1-(int)round(fDistanceToGoal / 8124.0)));
            				  else
            					  iScore += 5000;
            			  }
            */

            // checked , used for bombspots, when the checked value is filled, we will make the
            // score drasticly lower!
            /*
               if (Goals[iGn].iChecked > 0)
               {
               iScore /= Goals[iGn].iChecked;

               if (Goals[iGn].iChecked > 10)
               Goals[iGn].iChecked = RANDOM_LONG(0,10);
               }
             */


            // finally update values, and get to goal
            if (iScore > iFinalGoalScore) {
               iFinalGoalScore = iScore;
               iFinalGoalNode = Goals[iGn].iNode;
               iFinalGoalID = iGn;
               iFinalGoalBadScore = Goals[iGn].iBadScore;
               iFinalGoalVisited = Goals[iGn].iChecked;
            }
         }

         int iGoalNode = -1;

         iGoalNode = iFinalGoalNode;



         // Randomly we choose random goals! yeah.
         /*
            if (RANDOM_LONG (0, 100) < pBot->ipRandom)
            {
            iGoalNode = RANDOM_LONG (0, iMaxUsedNodes);
            REALBOT_PRINT (pBot, "cNodeMachine::path_think()", "Goalnode is random.");
            }
          */


         // When we are terrorist
         if (pBot->iTeam == 1)
            if (Game.vDroppedC4 != Vector(9999, 9999, 9999)) {
               if (RANDOM_LONG(0, 100) < pBot->ipDroppedBomb) {
                  // Dropped c4
                  iGoalNode = close(Game.vDroppedC4, 75, NULL);
               }
            }

         if (pBot->pButtonEdict)
            iGoalNode =
               close(VecBModelOrigin(pBot->pButtonEdict), NODE_ZONE,
                     pBot->pButtonEdict);

         if (RANDOM_LONG(0, 100) < pBot->ipFearRate)
            pBot->iPathFlags = PATH_DANGER;

         if (iGoalNode > -1) {
            //        UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "NodeMachine: Found goal, going to create path\n");
         } else
            pBot->fWanderTime = gpGlobals->time + 1;    // no goal found

         // When camp flag is set, so we should camp at our 'destination' then we should determine
         // where to camp there.
         /*
            if (pBot->iPathFlags == PATH_CAMP && iGoalNode > -1)
            {
            int iSearchNode = node_dangerous (UTIL_GetTeam (pBot->pEdict), Nodes[iGoalNode].origin, 300);

            if (iSearchNode > -1)
            iGoalNode = node_camp (Nodes[iSearchNode].origin,  UTIL_GetTeam (pBot->pEdict));
            else
            iGoalNode = node_camp (Nodes[pBot->iDiedNode].origin, UTIL_GetTeam (pBot->pEdict));          
            }
          */

         pBot->iGoalNode = iGoalNode;

         // REALBOT_PRINT(pBot, "cNodeMachine::path_think", "I have choosen to go to goalnode:");

         /*
            if (iFinalGoalNode == iGoalNode)
            sprintf(msg, "Node %d, Node Type = %d, iScore=%d, Distance=%f\n", iGoalNode, Goals[iFinalGoalID].iType, iFinalGoalScore, func_distance(pBot->pEdict->v.origin, Nodes[iGoalNode].origin));    
            else
            sprintf(msg, "Node %d, Distance=%f\n", iGoalNode, func_distance(pBot->pEdict->v.origin, Nodes[iGoalNode].origin));

            //log(msg);
            //SERVER_PRINT(msg);
          */

         // create path
         path(iCurrentNode, pBot->iGoalNode, BotIndex, pBot,
              pBot->iPathFlags);

         // If we still did not find a path, we set wander time
         // for 1 second we wait before a new attempt to find a goal and create a path.
         if (pBot->bot_pathid < 0) {
            pBot->fWanderTime = gpGlobals->time + 0.1;

            if (iFinalGoalBadScore > 8) {
               pBot->fWanderTime =
                  gpGlobals->time +
                  ((10 - (iFinalGoalBadScore + 1)) * 15);

               // TODO: Perhaps a bot should notify the players that it does not know
               // how and where to move...? (using chatting?)
            }


            pBot->iGoalNode = -1;

            // Note, we give this goal a BAD SCORE
            if (iFinalGoalNode == iGoalNode) {
               // fix: 14/06/04 - do not get over a score of 10, or we might screw up INT max ;)
               if (Goals[iFinalGoalID].iBadScore < 10)
                  Goals[iFinalGoalID].iBadScore++;      // increase badly!


               Goals[iFinalGoalID].iChecked++;

               // bombspots do not get high scores
               if (Goals[iGn].iType == GOAL_BOMBSPOT)
                  if (Goals[iGn].iChecked > 7)
                     Goals[iGn].iChecked = RANDOM_LONG(0, 6);


               if (Goals[iGn].iChecked > 10)
                  Goals[iGn].iChecked = RANDOM_LONG(0, 10);
            }
         } else {
            // 21/06/04
            // success on creating path, order any teammmates on our team to move to this goal
           // if (BOT_IsLeader(pBot))
             //  ORDER_BotsOfLeader(pBot->pEdict, pBot->iGoalNode);
         }
      }                         // WE HAVE NO GOAL AND NO PATH
      else {
         // situation:
         // there is a goal
         // there is no path

         int iCurrentNode =
            close(pBot->pEdict->v.origin, 50, pBot->pEdict);

         // there is no current node, thats odd, but we should take care of it.
         if (iCurrentNode < 0) {
            pBot->iGoalNode = -1;
            pBot->fWanderTime = gpGlobals->time + 1;
            return;
         }
         // we are already pretty close to our goal , or we are in fact ON our goal. So we
         // should not be here.
         if ((func_distance
               (pBot->pEdict->v.origin, Nodes[pBot->iGoalNode].origin) < 50)
               || iCurrentNode == pBot->iGoalNode) {
            pBot->iGoalNode = -1;
            pBot->bot_pathid = -1;
            path_clear(pBot->iIndex);
            return;
         }
         // create path
         path(iCurrentNode, pBot->iGoalNode, BotIndex, pBot,
              pBot->iPathFlags);

         if (pBot->bot_pathid < 0) {
            pBot->iGoalNode = -1;       // reset goal
         }
      }
   } else
      path_walk(pBot, moved_distance);  // walk the path
}

/*
// Think about path creation here
void cNodeMachine::path_think(cBot *pBot, float moved_distance)
{
    if (pBot->pBotHostage != NULL && pBot->CanSeeEntity(pBot->pBotHostage))
        return; // bot has hostage, can see hostage
 
    if (pBot->f_c4_time > gpGlobals->time)
    {
        //SERVER_PRINT("BOT: Not allowed to 'path_think', f_c4_time set.\n");
        return;
    }
 
	if (pBot->fWanderTime > gpGlobals->time)
	{
		wander_think(pBot, moved_distance); // don't be busy with paths and such, just do stupid
		pBot->bot_pathid = -1;		
        //pBot->iGoalNode = -1;
		return;
	}
 
    // When camping we do not think about paths, we think where to look at
    if (pBot->f_camp_time > gpGlobals->time)
    {
        if (pBot->iGoalNode == -1 )
        {
            pBot->iGoalNode = node_look_camp(pBot->pEdict->v.origin, UTIL_GetTeam(pBot->pEdict), pBot->pEdict);
        }
 
        return;
    }
 
 
	int BotIndex = pBot->iIndex;		
	
    // When no path, create one.
	if (pBot->bot_pathid < 0)
	{		
        pBot->f_move_speed = 0.0;        
		// Find a new goal
		if (pBot->iGoalNode < 0 && 
            pBot->pBotEnemy == NULL)
		{            
			//UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "NodeMachine: No path & no goal, going to find goal\n");
			// Depending on team we have a goal
            int iCurrentNode = close(pBot->pEdict->v.origin, 75, pBot->pEdict);
 
            if (iCurrentNode < 0)
			{
				pBot->fWanderTime = gpGlobals->time + 1;
                pBot->iGoalNode=-1;
				return;
			}
 
			int iGoalNode = -1;
 
			if (pBot->iTeam == 1)
				iGoalNode = node_goal(GOAL_SPAWNCT);
			else
				iGoalNode = node_goal(GOAL_SPAWNT);
 
            // When basic goals are not available, do not look further
			if (iGoalNode < 0)
			{
				pBot->fWanderTime = gpGlobals->time + 2;
				return;
			}
 
			// Check if we are close
			if (func_distance(Nodes[iGoalNode].origin, pBot->pEdict->v.origin) < 1500)
			{
                // switch goal nodes
                if (pBot->iTeam == 1)
                    iGoalNode = node_goal(GOAL_SPAWNT);
                else
                    iGoalNode = node_goal(GOAL_SPAWNCT);
			}
 
            // Now we have had the most basic goal selection (spawn points)
            // ============================================================
           
            // Randomly we choose random goals! yeah.
            if (RANDOM_LONG(0,100) < pBot->ipRandom)
            {
                iGoalNode = RANDOM_LONG(0,iMaxUsedNodes);
                REALBOT_PRINT(pBot, "cNodeMachine::path_think()", "Goalnode is random.");
            }
 
            // Depending on team first
            if (pBot->iTeam == 1)
            {
 
                // T
                // Check bombsite if it exists
                int iBombNode = node_goal(GOAL_BOMBSPOT);
                int iGoForIt = pBot->ipBombspot;
                bool bHasBomb = FUNC_BotHasWeapon(pBot, CS_WEAPON_C4);
 
                // We have the bomb or not
                if (bHasBomb)
                {
                    iGoForIt += 50; // 50% more chance
                    REALBOT_PRINT(pBot, "cNodeMachine::path_think()", "I have C4.");
 
                    if (RANDOM_LONG(0,100) < iGoForIt)
                    {
                        iGoalNode = iBombNode;                        
                        pBot->iPathFlags = PATH_NONE;
                        REALBOT_PRINT(pBot, "cNodeMachine::path_think()", "Goalnode set to bombspot.");
                    }
                }
                else if (Game.vDroppedC4 != Vector(9999,9999,9999))
                {        
                    if (RANDOM_LONG(0,100) < pBot->ipDroppedBomb)
                    {
                        // Dropped c4
                        iGoalNode = close(Game.vDroppedC4, 75, NULL);
                        REALBOT_PRINT(pBot, "cNodeMachine::path_think()", "Goalnode set to node near dropped bomb.");
                    }
                }
                else
                {
                    if (RANDOM_LONG(0,100) < iGoForIt &&
                        func_distance(Nodes[iBombNode].origin, pBot->pEdict->v.origin) > 400)
                    {
                        iGoalNode = iBombNode;
                        REALBOT_PRINT(pBot, "cNodeMachine::path_think()", "Goalnode set to bombspot (2).");
                    }
 
                }
                
            }
            else if (pBot->iTeam == 2)
            {
                // CT / make vip seperated code
                if (pBot->vip)
                {
                       REALBOT_PRINT(pBot, "cNodeMachine::path_think()", "I am VIP.");
                }
                else
                {
                
                    int iHostageNode = goal_hostage(pBot);
 
                    if (pBot->pBotHostage != NULL)
                    {
                        int iReplace = close(pBot->pBotHostage->v.origin, 75, pBot->pBotHostage);
                        if (iReplace > -1)
                        {
                            iHostageNode = iReplace;
                            REALBOT_PRINT(pBot, "cNodeMachine::path_think()", "Override hostage node by 'goal hostage'.");
                        }
                    }   
 
                    if (iHostageNode > -1)
                    {                        
                        if (RANDOM_LONG(0,100) < pBot->ipHostage)
                        {
                            REALBOT_PRINT(pBot, "cNodeMachine::path_think()", "Goalnode set to hostage.");
                            iGoalNode = iHostageNode;
                        }
                    }
                    else
                    {
                        // Check bombsite if it exists
                        int iBombNode = node_goal(GOAL_BOMBSPOT);
                        int iGoForIt = pBot->ipBombspot;
                        int iResNode = node_goal(GOAL_RESCUEZONE);
                        
                        // Is the bomb dropped?
                        if (Game.vDroppedC4 != Vector(9999,9999,9999))
                        {
                            if (RANDOM_LONG(0,100) < pBot->ipDroppedBomb)
                            {
                                // Dropped c4
                                iGoalNode = close(Game.vDroppedC4, 75, NULL);
                                REALBOT_PRINT(pBot, "cNodeMachine::path_think()", "Goalnode set to dropped bomb.");
 
                            }
                        }
 
                        // Does a bombsite exist? if so, a bomb can be planted
                        if (iBombNode > -1)
                        {
                            if (RANDOM_LONG(0,100) < pBot->ipBombspot)
                            {
                                iGoalNode = iBombNode;
                                REALBOT_PRINT(pBot, "cNodeMachine::path_think()", "Goalnode set to bombsite.");
                            }
                        }
 
                        // When a rescue zone is found, go for it
                        // when we have atleast one hostage. We go for the rescue
                        // zone because if there where still unrescued hostages
                        // the iHostageNode should be > -1.
                        if (iResNode > -1)
                            if (FUNC_AmountHostages(pBot) > 0)
                            {
                                iGoalNode = iResNode;
                                REALBOT_PRINT(pBot, "cNodeMachine::path_think()", "Goalnode set to rescue zone.");
                            }                       
                        
                        // When bomb planted, go for bombnode for sure
                        if (Game.bBombPlanted && iBombNode > -1)
                        {
                            iGoalNode = iBombNode;
                            REALBOT_PRINT(pBot, "cNodeMachine::path_think()", "Goalnode set to bombsite (bomb is planted).");
                            pBot->iPathFlags = PATH_NONE;
                        }                       
                        
                        
                    }
                }
            }
 
            
 
            if (RANDOM_LONG(0,100) < pBot->ipFearRate)            
                pBot->iPathFlags = PATH_DANGER;
 
			if (iGoalNode > -1)
            {
		    	//	UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "NodeMachine: Found goal, going to create path\n");
            }
			else			
				pBot->fWanderTime = gpGlobals->time + 2;
 
			
            // When camp flag is set, so we should camp at our 'destination' then we should determine
            // where to camp there.
            if (pBot->iPathFlags == PATH_CAMP && iGoalNode > -1)
            {            
                int iSearchNode = node_dangerous(UTIL_GetTeam(pBot->pEdict), Nodes[iGoalNode].origin, 736);
                
                if (iSearchNode > -1)
                    iGoalNode = node_camp(Nodes[iSearchNode].origin, UTIL_GetTeam(pBot->pEdict));
                else
                    iGoalNode = node_camp(Nodes[pBot->iDiedNode].origin, UTIL_GetTeam(pBot->pEdict));
            }
 
			pBot->iGoalNode = iGoalNode;			
            
            path(iCurrentNode, pBot->iGoalNode, BotIndex, pBot, pBot->iPathFlags);
 
            
            // If we still did not find a path, we set wander time            
            if (pBot->bot_pathid < 0)
            {
                pBot->fWanderTime = gpGlobals->time + 1;
                pBot->iGoalNode=-1;
                //UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NODEMACHINE: wander time activated\n");
            }
            
		}
		else
		{
			// create path to goal
			//UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "NodeMachine: No path, already have goal, going to create path\n");
			
			int iCurrentNode = close(pBot->pEdict->v.origin, 50, pBot->pEdict);
			if (iCurrentNode < 0)
			{
				pBot->iGoalNode = -1;
				pBot->fWanderTime = gpGlobals->time + 1;
				return;
			}
 
            if ((func_distance(pBot->pEdict->v.origin, Nodes[pBot->iGoalNode].origin) < 50) || 
                iCurrentNode == pBot->iGoalNode)
            {
                pBot->iGoalNode = -1;
                pBot->bot_pathid = -1;
                path_clear(pBot->iIndex);
                return;
            }
			
			path(iCurrentNode, pBot->iGoalNode, BotIndex, pBot, pBot->iPathFlags);
			
            if (pBot->bot_pathid < 0)
            {
				pBot->iGoalNode = -1; // reset goal
            }
 
			// kill goal
			//pBot->iGoalNode = -1;
			//pBot->fWanderTime = gpGlobals->time + 10;
			
 
		}
		
	}
	else
		path_walk(pBot, moved_distance); // walk the path
 
} 
*/
// Wander around code
void cNodeMachine::wander_think(cBot * pBot, float moved_distance) {
   // set wait time (stand still, looks like bot is thinking hehe)
   pBot->f_wait_time = pBot->fWanderTime;

   pBot->f_move_speed = 0.0;


   //SERVER_PRINT("WANDER_THINK: executing\n");
   /*
      if (pBot->iGoalNode < 0)
      {
      pBot->f_move_speed = 0.0;    // do nothing yet

      // Find a node we can move to.
      // Use Meredians to search for nodes
      int iX, iY;
      Vector vOrigin = pBot->pEdict->v.origin;
      VectorToMeredian (vOrigin, &iX, &iY);

      int iList[100];
      for (int iNr = 0; iNr < 100; iNr++)
      iList[iNr] = -1;

      int iIndex = 0;
      float fDist = 90;

      if (iX > -1 && iY > -1)
      {
      // Search in this meredian
      for (int i = 0; i < NODES_MEREDIANS; i++)
      if (Meredians[iX][iY].iNodes[i] > -1)
      {
      int iNode = Meredians[iX][iY].iNodes[i];
      if (fabs (vOrigin.z - Nodes[iNode].origin.z) < 45)   // around same height
      if (func_distance (vOrigin, Nodes[iNode].origin) > fDist)    // not to close
      if (func_distance (vOrigin, Nodes[iNode].origin) < 512)      // not to far
      {
      TraceResult tr;
      Vector vNode = Nodes[iNode].origin;

      //Using TraceHull to detect de_aztec bridge and other entities.
      //DONT_IGNORE_MONSTERS, we reached it only when there are no other bots standing in our way!
      //UTIL_TraceHull(vOrigin, vNode, dont_ignore_monsters, point_hull, pBot->pEdict, &tr);
      UTIL_TraceHull (vOrigin, vNode, dont_ignore_monsters,
      human_hull, pBot->pEdict, &tr);

      // if nothing hit: 
      if (tr.flFraction >= 1.0)
      {
      iList[iIndex] = iNode;
      iIndex++;
      fDist =
      func_distance (vOrigin, Nodes[iNode].origin);
      }

      if (iIndex > 99)
      break;

      //return iNode;
      }
      }
      }

      iIndex--;
      pBot->iGoalNode = iList[RANDOM_LONG (0, iIndex)];
      // SET TO MOVE SOMEWHERE!

      }
      else
      {
      // look at this node
      pBot->f_move_speed = pBot->f_max_speed;

      pBot->vHead = Nodes[pBot->iGoalNode].origin;
      pBot->vBody = Nodes[pBot->iGoalNode].origin;

      if (func_distance
      (pBot->pEdict->v.origin, Nodes[pBot->iGoalNode].origin) < 50)
      {
      //    UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NODEMACHINE: CLOSE AT WANDER GOAL\n");
      pBot->iGoalNode = -1;
      }
      }
    */

   // TODO TODO TODO , waypointless navigation
}

// Find cover
int cNodeMachine::node_cover(int iFrom, int iTo, edict_t * pEdict) {
   if (iFrom == iTo)
      return iFrom;             // cover at current position

   if (iFrom < 0)
      return -1;

   if (iTo < 0)
      return -1;

   // From here the difficult part starts.

   // We don't want to scan the entire node table, > 4000 nodes
   // Using meredians, we should take care that some nodes are
   // not in the same meredian. So we should scan meredian x, and x+1
   // but also x - 1. Same goes for y, and y+1, y-1. That means we have
   // 9 meredians. 9*80 = ~720 nodes at max. Instead of 4000. This is
   // already a speedup increasement.

   // Another problem, firing 720 tracelines is way to much. Visibilty
   // table calculation for 4000 nodes, (4000x4000 nodes table is way to much)
   // is no option.

   // Note: this function is only called once for finding some node to take cover from
   // Most bad situation would be that 31 bots call this function in 1 frame.

   // TEMP sollution:
   float fClosest = 512;
   int iClosest = -1;
   for (int i = 0; i < MAX_NODES; i++)
      if ((i != iTo) && (Nodes[i].origin != Vector(9999, 9999, 9999))) {
         float fDistance =
            func_distance(Nodes[i].origin, Nodes[iFrom].origin);
         if (fDistance < fClosest) {
            // all nodes within this range may be tracelined
            bool bVisible = true;
            TraceResult tr;

            if (GetVisibilityFromTo(iFrom, i) == VIS_UNKNOWN)   // BERKED
            {
               UTIL_TraceLine(Nodes[iFrom].origin, Nodes[i].origin,
                              ignore_monsters, ignore_glass, pEdict, &tr);

               if (tr.flFraction < 1.0)
                  bVisible = false;
            } else {
               if (GetVisibilityFromTo(iFrom, i) == VIS_BLOCKED)        // BERKED
                  bVisible = false;
            }

            // Hit something
            if (bVisible == false) {
               // Update VisTable
               SetVisibilityFromTo(iFrom, i, false);
               SetVisibilityFromTo(i, iFrom, false);
               fClosest = fDistance;
               iClosest = i;
            } else {
               SetVisibilityFromTo(iFrom, i, true);
               SetVisibilityFromTo(i, iFrom, true);
            }
         }
      }
   // We have found something?
   return iClosest;

}

int cNodeMachine::node_look_at_hear(int iFrom, int iOrigin,
                                    edict_t * pEdict) {
   if (iFrom == iOrigin)
      return iFrom;             // impossible

   if (iFrom < 0) {
      SERVER_PRINT("iFrom is lower then 0\n");
      return -1;
   }

   if (iOrigin < 0) {
      SERVER_PRINT("iOrigin is lower then 0\n");
      return -1;
   }
   // From here the difficult part starts...
   // .. see bs at above function ..

   // what we do:
   // find a CLOSE node that has almost NO DANGER _and_
   // that can SEE iFROM.

   // TEMP sollution:
   int iClosest = -1;

   // we search for a node that can see the sound node (iFrom) and the origin node (iOrigin)
   for (int i = 0; i < MAX_NODES; i++)
      if ((i != iOrigin && i != iFrom)
            && (Nodes[i].origin != Vector(9999, 9999, 9999))) {
         if (func_distance(Nodes[i].origin, Nodes[iOrigin].origin) >
               BOT_HEARDISTANCE)
            continue;

         // all nodes within this range may be tracelined
         bool bVisible = true;  // visible from I to iFrom (= sound origin)
         bool bVisibleTo = true;        // visible from I to iTo (= where we are located)

         TraceResult tr;

         if (GetVisibilityFromTo(iFrom, i) == VIS_UNKNOWN)      // BERKED
         {
            UTIL_TraceLine(Nodes[iFrom].origin, Nodes[i].origin,
                           ignore_monsters, ignore_glass, pEdict, &tr);

            if (tr.flFraction < 1.0) {
               SetVisibilityFromTo(iFrom, i, false);
               SetVisibilityFromTo(i, iFrom, false);
            }
         }

         if (GetVisibilityFromTo(iOrigin, i) == VIS_UNKNOWN)    // BERKED
         {
            UTIL_TraceLine(Nodes[iOrigin].origin, Nodes[i].origin,
                           ignore_monsters, ignore_glass, pEdict, &tr);

            if (tr.flFraction < 1.0) {
               SetVisibilityFromTo(iOrigin, i, false);
               SetVisibilityFromTo(i, iOrigin, false);
            }
         }
         // only when really blocked, count it as false.
         if (GetVisibilityFromTo(iFrom, i) == VIS_BLOCKED)
            bVisible = false;

         if (GetVisibilityFromTo(iOrigin, i) == VIS_BLOCKED)
            bVisibleTo = false;

         // Hit something
         if (bVisible && bVisibleTo) {
            SERVER_PRINT
            ("Found one node that could be a possible 'look to'\n");
            iClosest = i;
         }
      }
   // We have found something?
   return iClosest;

}

// Returns randomly a dangerous node
// fMaxDistance is the maximum distance the node may be from vOrigin,
int cNodeMachine::node_dangerous(int iTeam, Vector vOrigin,
                                 float fMaxDistance) {
   int iTotal = 0;
   int iList[250];

   if (fMaxDistance < 0)
      fMaxDistance = 9999;

   if (vOrigin == Vector(9999, 9999, 9999))
      return -1;

   // Clear list
   for (int j = 0; j < 250; j++)
      iList[j] = -1;

   // Limit is 0.6 or higher!
   for (int i = 0; i < MAX_NODES; i++)
      if (Nodes[i].origin != Vector(9999, 9999, 9999))
         if (InfoNodes[i].fDanger[iTeam] >= 0.6)
            if (func_distance(vOrigin, Nodes[i].origin) < fMaxDistance) {
               iList[iTotal] = i;
               iTotal++;
               if (iTotal > 249) {
                  break;
               }
            }


   iTotal--;

   if (iTotal > 0)
      return iList[RANDOM_LONG(0, iTotal)];

   return -1;
}

void cNodeMachine::dump_goals(void) {
   int i;
   char buffer[100];
   Vector v;

   rblog("Dump of all goals\n");
   for (i = 0; (i < MAX_GOALS) && (Goals[i].iNode >= 0); i++) {
      v = Nodes[Goals[i].iNode].origin;
      sprintf(buffer,
              "Goal#%d is at node %d (%.0f, %.0f, %.0f), iChecked= %d, ",
              i + 1, Goals[i].iNode, v.x, v.y, v.z, Goals[i].iChecked);
      switch (Goals[i].iType) {
      case GOAL_SPAWNCT:
         strcat(buffer, "GOAL_SPAWNCT");
         break;
      case GOAL_SPAWNT:
         strcat(buffer, "GOAL_SPAWNT");
         break;
      case GOAL_BOMBSPOT:
         strcat(buffer, "GOAL_BOMBSPOT");
         break;
      case GOAL_BOMB:
         strcat(buffer, "GOAL_BOMB");
         break;
      case GOAL_HOSTAGE:
         strcat(buffer, "GOAL_HOSTAGE");
         break;
      case GOAL_RESCUEZONE:
         strcat(buffer, "GOAL_RESCUEZONE");
         break;
      case GOAL_CONTACT:
         strcat(buffer, "GOAL_CONTACT");
         break;
      case GOAL_IMPORTANT:
         strcat(buffer, "GOAL_IMPORTANT");
         break;
      case GOAL_VIP:
         strcat(buffer, "GOAL_VIP");
         break;
      case GOAL_VIPSAFETY:
         strcat(buffer, "GOAL_VIPSAFETY");
         break;
      case GOAL_ESCAPEZONE:
         strcat(buffer, "GOAL_ESCAPEZONE");
         break;
      case GOAL_WEAPON:
         strcat(buffer, "GOAL_WEAPON");
         break;
      case GOAL_NONE:
         strcat(buffer, "GOAL_NONE");
         break;
      default:
         strcat(buffer, "unknown type");
      }
      strcat(buffer, "\n");
      rblog(buffer);
   }

}

// EVY: another dump
void cNodeMachine::dump_path(int iBot, int CurrentPath) {
   char buffer[80];
   int i, j, CurrentNode;
   Vector v;

   if (CurrentPath >= 0)
      CurrentNode = iPath[iBot][CurrentPath];
   else
      CurrentNode = -1;
   rblog("  Path is: ");
   for (i = 0; (i < MAX_NODES) && (iPath[iBot][i] >= 0); i++) {
      if (i == CurrentPath)
         sprintf(buffer, "<%d> ", iPath[iBot][i]);
      else
         sprintf(buffer, "%d ", iPath[iBot][i]);
      rblog(buffer);
   }
   rblog("\n");
   if (CurrentNode < 0)
      return;
   rblog("  Current direct neighbours are:\n");
   for (i = 0; i < MAX_NEIGHBOURS; i++)
      if (Nodes[CurrentNode].iNeighbour[i] >= 0) {
         j = Nodes[CurrentNode].iNeighbour[i];
         v = Nodes[j].origin;
         sprintf(buffer, "      %d (%.0f, %.0f, %.0f)\n", j, v.x, v.y,
                 v.z);
         rblog(buffer);
      }
   rblog("\n");
}

// EVY a lot of things to draw: nodes, neighbours, goals, paths, ...
// Graphs from PMB & Botman

// width and height of the debug bitmap image
#define DEBUG_BMP_WIDTH 2048
#define DEBUG_BMP_HEIGHT 2048

static char *bmp_buffer;
static float maxx, maxy, minx, miny;
static float scale;

static void InitDebugBitmap(void) {
   // this function allocates memory and clears the debug bitmap buffer

   if (bmp_buffer)
      free(bmp_buffer);         // reliability check, free BMP buffer if already allocated

   bmp_buffer = NULL;
   bmp_buffer = (char *) malloc(DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);    // allocate memory
   if (bmp_buffer == NULL) {
      fprintf(stderr,
              "InitDebugBitmap(): unable to allocate %d kbytes for BMP buffer!\n",
              DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT / 1024);
      exit(1);
   }

   memset(bmp_buffer, 14, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);  // Set all to all white (and allow for darker palette)
}

// Draw a small cross
static void DrawPoint(const Vector v, unsigned char color) {
   int offset, x0, y0;

   if (bmp_buffer == NULL) {
      fprintf(stderr,
              "DrawLineInDebugBitmap(): function called with NULL BMP buffer!\n");
      return;                   // reliability check: cancel if bmp buffer unallocated
   }
   // translate the world coordinates in image pixel coordinates
   x0 = (int) ((v.x - minx) / scale);
   y0 = (int) ((v.y - miny) / scale);

   offset = y0 * DEBUG_BMP_WIDTH + x0;
   if ((offset < 0) || (offset >= DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT)) {
      fprintf(stderr,
              "DrawLineInDebugBitmap(): bad BMP buffer index %d (range 0 - %d)\n",
              offset, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);
      exit(1);
   }

   bmp_buffer[offset] = color;  // draw the point itself
   if (offset + 1 < DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT)
      bmp_buffer[offset + 1] = color;   // make a small star on the right
   if (offset - 1 >= 0)
      bmp_buffer[offset - 1] = color;   // make a small star on the left
   if (offset + DEBUG_BMP_WIDTH < DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT)
      bmp_buffer[offset + DEBUG_BMP_WIDTH] = color;     // make a small star below
   if (offset - DEBUG_BMP_WIDTH >= 0)
      bmp_buffer[offset - DEBUG_BMP_WIDTH] = color;     // make a small star above
}

// From PMB and Botman's code

static void
DrawLineInDebugBitmap(const Vector v_from, const Vector v_to,
                      unsigned char color) {
   // blind copy of botman's Bresenham(). This function prints a vector line into a bitmap dot
   // matrix. The dot matrix (bmp_buffer) is a global array. The size of the bitmap is always
   // assumed to be DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT pixels (currently 2000 * 2000 to fit with
   // the size of the universe, with an adaptative unit scale, up to 1 pixel = 10 vector units).

   int x0, y0, x1, y1;
   int dx, stepx, dy, stepy;
   int offset, fraction;

   if (bmp_buffer == NULL) {
      fprintf(stderr,
              "DrawLineInDebugBitmap(): function called with NULL BMP buffer!\n");
      return;                   // reliability check: cancel if bmp buffer unallocated
   }
   // translate the world coordinates in image pixel coordinates
   x0 = (int) ((v_from.x - minx) / scale);
   y0 = (int) ((v_from.y - miny) / scale);
   x1 = (int) ((v_to.x - minx) / scale);
   y1 = (int) ((v_to.y - miny) / scale);

   dx = (x1 - x0) * 2;
   dy = (y1 - y0) * 2;
   if (dx < 0) {
      dx = -dx;
      stepx = -1;
   } else
      stepx = 1;
   if (dy < 0) {
      dy = -dy;
      stepy = -1;
   } else
      stepy = 1;

   offset = y0 * DEBUG_BMP_WIDTH + x0;
   if ((offset < 0) || (offset >= DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT)) {
      fprintf(stderr,
              "DrawLineInDebugBitmap(): bad BMP buffer index %d (range 0 - %d)\n",
              offset, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);
      exit(1);
   }

   bmp_buffer[offset] = color;  // draw the first point of the line

   // is the line rather horizontal than vertical ? We need to know this to determine the step
   // advance in the Bresenham grid, either we draw y = f(x), or x = f(y).
   if (dx > dy) {
      // the line is rather horizontal, we can draw it safely for incremental values of x

      fraction = 2 * dy - dx;   // fraction of height in x0 pixel's 'square' where y0 should be

      // while we've not reached the end of the segment...
      while (x0 != x1) {
         // if y0 should rather be drawn on a different height than its previous height...
         if (fraction >= 0) {
            y0 += stepy;        // draw it one pixel aside, then (depending on line orientation)
            fraction -= 2 * dx; // and reset its fraction (Bresenham, not sure I get the math)
         }
         x0 += stepx;           // in either case, draw x0 one pixel aside its previous position
         fraction += 2 * dy;    // and update y0's fraction (not sure I get the math - but whatever)

         // compute the offset in the BMP buffer corresponding to this point
         offset = y0 * DEBUG_BMP_WIDTH + x0;
         if ((offset < 0)
               || (offset >= DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT)) {
            fprintf(stderr,
                    "DrawLineInDebugBitmap(): bad BMP buffer index %d (range 0 - %d)\n",
                    offset, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);
            exit(1);
         }

         bmp_buffer[offset] = color;    // set this point to have the specified color
      }
   } else {
      // else the line is rather vertical, we NEED to draw it for incremental values of y (if we
      // did it for incremental values of x instead, we would drop half the pixels).

      fraction = 2 * dx - dy;   // fraction of width in y0 pixel's 'square' where x0 should be

      // while we've not reached the end of the segment...
      while (y0 != y1) {
         // if x0 should rather be drawn on a different width than its previous width...
         if (fraction >= 0) {
            x0 += stepx;        // draw it one pixel aside, then (depending on line orientation)
            fraction -= 2 * dy; // and reset its fraction (Bresenham, not sure I get the math)
         }
         y0 += stepy;           // in either case, draw y0 one pixel aside its previous position
         fraction += 2 * dx;    // and update x0's fraction (not sure I get the math - but whatever)

         // compute the offset in the BMP buffer corresponding to this point
         offset = y0 * DEBUG_BMP_WIDTH + x0;
         if ((offset < 0)
               || (offset >= DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT)) {
            fprintf(stderr,
                    "DrawLineInDebugBitmap(): bad BMP buffer index %d (range 0 - %d)\n",
                    offset, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);
            exit(1);
         }
         bmp_buffer[offset] = color;    // set this point to have the specified color
      }
   }
   return;                      // finished, segment has been printed into the BMP dot matrix
}

// from PMB & Botman code

static void WriteDebugBitmap(const char *filename) {
   // this function writes the debug bitmap image buffer in a .BMP file to disk. The format is
   // 256 color and 2000 * 2000 pixels. The center of the world being roughly the center of the
   // bitmap. The bitmap is stored in the file specified by 'filename' (which can be a relative
   // path from the Half-Life base directory).
   FILE *fp;
   int data_start, file_size;
   unsigned long dummy;

   if (bmp_buffer == NULL) {
      fprintf(stderr,
              "WriteDebugBitmap(): function called with NULL BMP buffer!\n");
      return;                   // reliability check: cancel if bmp buffer unallocated
   }
   // open (or create) the .bmp file for writing in binary mode...
   fp = fopen(filename, "wb");
   if (fp == NULL) {
      fprintf(stderr, "WriteDebugBitmap(): unable to open BMP file!\n");
      if (bmp_buffer)
         free(bmp_buffer);      // cannot open file, free DXF buffer
      bmp_buffer = NULL;
      return;                   // cancel if error creating file
   }
   // write the BMP header
   fwrite("BM", 2, 1, fp);      // write the BMP header tag
   fseek(fp, sizeof(unsigned long), SEEK_CUR);  // skip the file size field (will write it last)
   fwrite("\0\0", sizeof(short), 1, fp);        // dump zeros in the first reserved field (unused)
   fwrite("\0\0", sizeof(short), 1, fp);        // dump zeros in the second reserved field (unused)
   fseek(fp, sizeof(unsigned long), SEEK_CUR);  // skip the data start field (will write it last)

   // write the info header
   dummy = 40;
   fwrite(&dummy, sizeof(unsigned long), 1, fp);        // write the info header size (does 40 bytes)
   dummy = DEBUG_BMP_WIDTH;
   fwrite(&dummy, sizeof(long), 1, fp); // write the image width (2000 px)
   dummy = DEBUG_BMP_HEIGHT;
   fwrite(&dummy, sizeof(long), 1, fp); // write the image height (2000 px)
   dummy = 1;
   fwrite(&dummy, sizeof(short), 1, fp);        // write the # of planes (1)
   dummy = 8;
   fwrite(&dummy, sizeof(short), 1, fp);        // write the bit count (8)
   dummy = 0;
   fwrite(&dummy, sizeof(unsigned long), 1, fp);        // write the compression id (no compression)
   dummy = DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT;
   fwrite(&dummy, sizeof(unsigned long), 1, fp);        // write the image size (2000 * 2000)
   dummy = 0;
   fwrite(&dummy, sizeof(long), 1, fp); // write the X pixels per meter (not specified)
   fwrite(&dummy, sizeof(long), 1, fp); // write the Y pixels per meter (not specified)
   dummy = 256;
   fwrite(&dummy, sizeof(unsigned long), 1, fp);        // write the # of colors used (all)
   fwrite(&dummy, sizeof(unsigned long), 1, fp);        // write the # of important colors (wtf ?)

   // write the color palette (B, G, R, reserved byte)
   fputc(0x00, fp);
   fputc(0x00, fp);
   fputc(0x00, fp);
   fputc(0x00, fp);             // 0=BLACK
   fputc(0xFF, fp);
   fputc(0xFF, fp);
   fputc(0xFF, fp);
   fputc(0x00, fp);             // 1=WHITE
   fputc(0x80, fp);
   fputc(0x80, fp);
   fputc(0x80, fp);
   fputc(0x00, fp);             // 2=GREY
   fputc(0xC0, fp);
   fputc(0xC0, fp);
   fputc(0xC0, fp);
   fputc(0x00, fp);             // 3=SILVER
   fputc(0x80, fp);
   fputc(0x00, fp);
   fputc(0x00, fp);
   fputc(0x00, fp);             // 4=DARK BLUE
   fputc(0xFF, fp);
   fputc(0x00, fp);
   fputc(0x00, fp);
   fputc(0x00, fp);             // 5=BLUE
   fputc(0x80, fp);
   fputc(0x80, fp);
   fputc(0x00, fp);
   fputc(0x00, fp);             // 6=DARK YELLOW
   fputc(0xFF, fp);
   fputc(0xFF, fp);
   fputc(0x00, fp);
   fputc(0x00, fp);             // 7=YELLOW ? LIGHT BLUE
   fputc(0x00, fp);
   fputc(0x80, fp);
   fputc(0x00, fp);
   fputc(0x00, fp);             // 8=DARK GREEN
   fputc(0x00, fp);
   fputc(0xFF, fp);
   fputc(0x00, fp);
   fputc(0x00, fp);             // 9=GREEN
   fputc(0x00, fp);
   fputc(0x00, fp);
   fputc(0x80, fp);
   fputc(0x00, fp);             // 10=DARK RED
   fputc(0x00, fp);
   fputc(0x00, fp);
   fputc(0xFF, fp);
   fputc(0x00, fp);             // 11=RED
   fputc(0x80, fp);
   fputc(0x00, fp);
   fputc(0x80, fp);
   fputc(0x00, fp);             // 12=DARK PURPLE
   fputc(0xFF, fp);
   fputc(0x00, fp);
   fputc(0xFF, fp);
   fputc(0x00, fp);             // 13=PURPLE
   fputc(0xFF, fp);
   fputc(0xFF, fp);
   fputc(0xFF, fp);
   fputc(0x00, fp);             // 14=WHITE
   fputc(0xEF, fp);
   fputc(0xEF, fp);
   fputc(0xEF, fp);
   fputc(0x00, fp);             // 15=WHITE-GREY
   fputc(0xDF, fp);
   fputc(0xDF, fp);
   fputc(0xDF, fp);
   fputc(0x00, fp);             // 16=GREY
   fputc(0xCF, fp);
   fputc(0xCF, fp);
   fputc(0xCF, fp);
   fputc(0x00, fp);             // 17=DARKGREY
   fputc(0xBF, fp);
   fputc(0xBF, fp);
   fputc(0xBF, fp);
   fputc(0x00, fp);             // 18=DARKGREY
   fputc(0xAF, fp);
   fputc(0xAF, fp);
   fputc(0xAF, fp);
   fputc(0x00, fp);             // 19=DARKGREY

   for (dummy = 20; dummy < 256; dummy++) {
      // fill out the rest of the palette with zeros
      fputc(0x00, fp);
      fputc(0x00, fp);
      fputc(0x00, fp);
      fputc(0x00, fp);
   }

   // write the actual image data
   data_start = ftell(fp);      // get the data start position (that's where we are now)
   fwrite(bmp_buffer, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT, 1, fp);       // write the image
   file_size = ftell(fp);       // get the file size now that the image is dumped

   // now that we've dumped our data, we know the file size and the data start position

   fseek(fp, 0, SEEK_SET);      // rewind
   fseek(fp, 2, SEEK_CUR);      // skip the BMP header tag "BM"
   fwrite(&file_size, sizeof(unsigned long), 1, fp);    // write the file size at its location
   fseek(fp, sizeof(short), SEEK_CUR);  // skip the first reserved field
   fseek(fp, sizeof(short), SEEK_CUR);  // skip the second reserved field
   fwrite(&data_start, sizeof(unsigned long), 1, fp);   // write the data start at its location

   fclose(fp);                  // finished, close the BMP file

   if (bmp_buffer)
      free(bmp_buffer);         // and free the BMP buffer
   bmp_buffer = NULL;

   return;                      // and return
}


// Find the border of all nodes to scale the bitmap

void cNodeMachine::FindMinMax(void) {
   int i;
   float scalex, scaley;

   minx = miny = 9999.0;
   maxx = maxy = -9999.0;
   for (i = 0;
         (i < MAX_NODES) && (Nodes[i].origin != Vector(9999, 9999, 9999));
         i++) {
      if (Nodes[i].origin.x > maxx)
         maxx = Nodes[i].origin.x;
      if (Nodes[i].origin.y > maxy)
         maxy = Nodes[i].origin.y;
      if (Nodes[i].origin.x < minx)
         minx = Nodes[i].origin.x;
      if (Nodes[i].origin.y < miny)
         miny = Nodes[i].origin.y;
   }
   // Avoid having lines/points just on the bitmap border, add some more spaces
   maxx += NODE_ZONE;
   minx -= NODE_ZONE;
   maxy += NODE_ZONE;
   miny -= NODE_ZONE;

   // first compute the X and Y divider scale, and take the greatest of both
   scalex = (1 + maxx - minx) / DEBUG_BMP_WIDTH;
   scaley = (1 + maxy - miny) / DEBUG_BMP_WIDTH;
   if (scalex > scaley)
      scale = scalex + scalex / 100;    // add a little offset (margin) for safety
   else
      scale = scaley + scaley / 100;    // add a little offset (margin) for safety
}

// 05/07/04
// Mark meridians as slighly darker in alternance
// Palette is defined such that increasing the palette index
// Makes a slightly darker dark

void cNodeMachine::MarkAxis(void) {
   int x, y, x0, y0;

   x0 = (int) ((0 - minx) / scale);
   y0 = (int) ((0 - miny) / scale);

   // Mark X axis by keeping X to 0 and varying Y
   if ((minx < 0) && (0 < maxx))
      for (y = 0; y < DEBUG_BMP_HEIGHT; y++)
         bmp_buffer[y * DEBUG_BMP_WIDTH + x0] += 2;

   // Mark Y axis by keeping Y to 0 and varying X
   if ((miny < 0) && (0 < maxy))
      for (x = 0; x < DEBUG_BMP_WIDTH; x++)
         bmp_buffer[y0 * DEBUG_BMP_WIDTH + x] += 2;
}

// 05/07/04
// Mark each meredians (default 256 units) of alternating color

void cNodeMachine::MarkMeredians(void) {
   int x, y;
   int Meredian;

   // Mark some meredians
   for (x = 0; x < DEBUG_BMP_WIDTH; x++) {
      Meredian =
         (int) ((float) x * scale + minx +
                8192.0) / (float) SIZE_MEREDIAN;
      if (Meredian & 0x01) {
         for (y = 0; y < DEBUG_BMP_HEIGHT; y++)
            bmp_buffer[y * DEBUG_BMP_WIDTH + x]++;
      }
   }

   // Mark some meredians
   for (y = 0; y < DEBUG_BMP_HEIGHT; y++) {
      Meredian =
         (int) ((float) y * scale + miny +
                8192.0) / (float) SIZE_MEREDIAN;
      if (Meredian & 0x01) {
         for (x = 0; x < DEBUG_BMP_HEIGHT; x++)
            bmp_buffer[y * DEBUG_BMP_WIDTH + x]++;
      }
   }
}

// Put a cross on all nodes in RBN + draw lines to all neighbours

void cNodeMachine::PlotNodes(int NeighbourColor, int NodeColor) {
   int i, j;

   // Draw all neighbours
   for (i = 0;
         (i < MAX_NODES) && (Nodes[i].origin != Vector(9999, 9999, 9999));
         i++)
      for (j = 0; (j < MAX_NEIGHBOURS) && (Nodes[i].iNeighbour[j] >= 0);
            j++)
         DrawLineInDebugBitmap(Nodes[i].origin,
                               Nodes[Nodes[i].iNeighbour[j]].origin,
                               NeighbourColor);
   // Draw all nodes
   for (i = 0;
         (i < MAX_NODES) && (Nodes[i].origin != Vector(9999, 9999, 9999));
         i++)
      DrawPoint(Nodes[i].origin, NodeColor);
}

// Put a small cross at all goal points

void cNodeMachine::PlotGoals(int color) {
   int i;
   Vector v;

   for (i = 0; (i < MAX_GOALS) && (Goals[i].iNode >= 0); i++) {
      v = Nodes[Goals[i].iNode].origin;
      DrawPoint(v, color);
   }
}

// Plot the computed paths for all life bots
void cNodeMachine::PlotPaths(int Tcolor, int CTcolor) {
   int i, iBot, From, To;

   for (iBot = 0; (iBot < 32); iBot++) {
      if (bots[iBot].bIsUsed) {
         From = iPath[iBot][0];
         if (From < 0)
            continue;           // This bot has not path

         for (i = 1; (i < MAX_NODES) && (iPath[iBot][i] >= 0); i++) {
            To = iPath[iBot][i];
            DrawLineInDebugBitmap(Nodes[From].origin, Nodes[To].origin,
                                  (bots[iBot].iTeam ==
                                   1) ? Tcolor : CTcolor);
            From = To;
         }
      }
   }
}

// Called from 'realbot debug nodes dumpbmp' command in dll.cpp
// Creates a .BMP file with all nodes (blue cross), goals (green cross), neighbour (black lines),
// goals (light blue lines for CT and red lines for T)
// 05/07/04
// Marking Axis, Meredians, other colors, other filenames (linked to map names)

void cNodeMachine::Draw(void) {
   static int Count = 0;        // Static to create filenames like cs_siege0000.bmp, cs_siege0001.bmp, ...
   char Filename[80];

   FindMinMax();
   InitDebugBitmap();
   MarkMeredians();
   MarkAxis();
   PlotNodes(0, 5);             // 0 = black, 5 = blue
   PlotPaths(11, 7);            // 11 = Red 7 = light blue ?
   PlotGoals(9);                // 9 = green
   sprintf(Filename, "%s%4.4d.bmp", STRING(gpGlobals->mapname), Count++);
   WriteDebugBitmap(Filename);
}

// $Log: NodeMachine.cpp,v $
// Revision 1.20  2004/09/07 15:44:34  eric
// - bumped build nr to 3060
// - minor changes in add2 (to add nodes for Bsp2Rbn utilities)
// - if compiled with USE_EVY_ADD, then the add2() function is used when adding
//   nodes based on human players instead of add()
// - else, it now compiles mostly without warnings :-)
//
// Revision 1.17  2004/07/17 21:32:01  eric
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
// Revision 1.16  2004/07/16 12:41:34  stefan
// - fixed memory leaks mentioned on the forum (thx Josh & Whistler)
//
// Revision 1.15  2004/07/08 09:04:57  stefan
// - fixed some shield detection bug... hopefully fixes improper buy behaviour
//
// Revision 1.14  2004/07/05 21:32:29  eric
// - bumped to build 3053
// - minor modifications in 'realbot debug nodes dumpbmp'...
// -    .BMP filename linked to the map name
// -    X and Y axis are now marked by darker line
// -    Meredians (256 units wide per default) are of alternating colors to get
//      idea of the map size and node position
// - added a new goal type GOAL_WEAPON for spawned weapons
//      BUT goal system does not use them yet...
//
// Revision 1.13  2004/07/02 16:43:34  stefan
// - upped to build 3051
// - changed log() into rblog()
// - removed BOT.CFG code that interpets old RB V1.0 commands
// - neater respons of the RealBot console
// - more help from RealBot console (ie, type realbot server broadcast ... with no arguments it will tell you what you can do with this, etc)
// - removed message "bot personality loaded from file"
// - in overal; some cleaning done, no extra features added
//
// Revision 1.12  2004/07/01 18:09:45  stefan
// - fixed skill 10 bots not causing memory bugger on re-adding (respawning)
// - added extra check for respawning bots so auto-add function cannot crash
// - fixed 2 nitpicks pointed out on the forums
//
// Revision 1.11  2004/06/25 07:39:00  stefan
// - upped to build 3050
// - fixed reaction time (instant reaction time) bug
// - added evy's goals, but they are not used yet
// - fixed some radio responses here and there for swat behaviour.
// - swat leader automaticly assigned again when one dies
// - HINT: you can see any changes made by me, by looking at DD/MM/YY - Stefan (ie, 22/06/04 - Stefan, will let you find all changes i made that day)
//
// Revision 1.10  2004/06/23 08:24:13  stefan
// - upped to build 3049
// - added swat behaviour (team leader assignment, radio response change and leaders command team-mates) - THIS IS EXPERIMENTAL AND DOES NOT ALWAYS WORK AS I WANT IT TO.
// - changed some conditions in nodemachine
// - sorry evy, still not added your new goals ;) will do next time, i promise
//
// Revision 1.9  2004/06/20 10:24:13  stefan
// - fixed another steep/stair thingy
// - changed a bit of the aiming code
//
// Revision 1.8  2004/06/19 21:06:14  stefan
// - changed distance check in nodemachine
// - fixed some 'steep' bug in nodemachine
//
// Revision 1.7  2004/06/18 12:20:05  stefan
// - fixed another bug in chatting, CS 1.5 won't crash now
// - added some limit to bots searching for goals
//
// Revision 1.6  2004/06/17 21:23:22  stefan
// - fixes several connection problems with nodes. Going down from steep + crates (de_dust) PLUS going up/down from very steep slopes on as_oilrig.. 0wnage and thx to PMB and Evy
// - fixed chat bug in CS 1.6, its still CS 1.5 & CS 1.6 compatible though
//
// Revision 1.5  2004/06/13 20:08:21  stefan
// - 'bad score for goals' added
// - bmp dump info (Thanks Evy)
// - added 'realbot server players', so you can keep a server full at NR players at all times
// - slightly adjusted goal selection code
// - wander code disabled
// - lots of debug info introduced, do not use this source for REAL USAGE!
//
// Revision 1.4  2004/05/29 19:05:46  stefan
// - upped to BUILD 3044
// - removed several debug messages on screen
// - changed default 'chatrate (max sentences)' to 3
// - removed copyright notice, which is not valid due GPL license
//
// i know, nothing special :)
//
// Revision 1.3  2004/05/12 13:24:44  stefan
// fixed ; in jump recording
//
// Revision 1.2  2004/04/18 17:39:19  stefan
// - Upped to build 2043
// - REALBOT_PRINT() works properly now
// - Log() works properly now
// - Clearing in dll.cpp of reallog.txt at dll init
// - Logging works now, add REALBOT_PRINT() at every point you want to log something.
//
