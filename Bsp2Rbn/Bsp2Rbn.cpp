// Attempt to create a RBN file from the BSP file
//
// evyncke@students.hec.be, Summer 2004
//
// Based extensively on Botman's bsp_slicer
//
// BSP_tool - botman's Half-Life BSP utilities
//
// (http://planethalflife.com/botman/)
//
// bsp_slicer.cpp
//
// Copyright (C) 2001 - Jeffrey "botman" Broome
//
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
//
// See the GNU General Public License for more details at:
// http://www.gnu.org/copyleft/gpl.html
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <stdio.h>
#include <string.h>
#ifdef __linux__
#include <libgen.h>
#else
extern char * basename(char *) ;
extern char * dirname(char *) ;
#endif

// Includes from Realbot
#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>
#include <entity_state.h>
#include "../bot.h"
#include "../IniParser.h"
#include "../bot_weapons.h"
#include "../game.h"
#include "../bot_func.h"
#include "../NodeMachine.h"

// Below is tricky and dirty required because HL SDK redefine vec3_t as class Vector which brings a lot of trouble
#undef vec3_t
typedef vec_t vec3_t[3];

// Include from Botman
#include "cmdlib.h"
#include "world.h"
#include "bspfile.h"
#include "mathlib.h"
#include "trace.h"

#define MAX_NODES_DISTANCE	(3*NODE_ZONE-1) // Should reflect NodeMachine:add() parameters
#define MOVE_FROM_EDGE		30.0f	// Distance used when moving a vertex from a wall towards center
#define MAXEDGES 48  // from bsp5.h
#define MAX(a,b) (a > b ? a : b)
#define xDUMP(string,vector) printf("%s (%.0f, %.0f, %.0f)\n",string,vector[0],vector[1],vector[2]) ; fflush(stdout) ;
#define DUMP(string,vector)

extern char * Version ;

World world;
extern cNodeMachine NodeMachine ;
extern globalvars_t *gpGlobals;

// Local variables
dmodel_t *model;
edict_t DummyEntity ;
int NodesFound = 0 ;
int WalkableCount = 0 ;
int FaceCount = 0 ;
vec_t minx = -9999.0f, miny = -9999.0f, maxx = 9999.0f, maxy = 9999.0f ;

// Some well known move

vec3_t down_to_ground = {0, 0, -(NODE_ZONE+10) } ;
vec3_t really_down_to_ground = {0, 0, -144.0f } ;
vec3_t up_off_floor = {0, 0, 36.0f} ;
static vec3_t up_a_little = { 0,0,10.0f};
static vec3_t wall_forward = {16, 0, 0};
static vec3_t wall_back = {-16, 0, 0};
static vec3_t wall_left = {0, 16, 0};
static vec3_t wall_right = {0, -16, 0};

// Local prototypes

void GetVertexFromIndex(vec3_t x, const int VertexId) ;
void AddNode(const vec3_t origin) ;
bool CheckWaypoint(const vec3_t coord) ;
void OnLadder(void) ;
void OffLadder(void) ;

void OnLadder(void) 
{
	DummyEntity.v.movetype = MOVETYPE_FLY ;
}

void OffLadder(void) 
{
	DummyEntity.v.movetype = MOVETYPE_WALK ;
}

void AddNode(const vec3_t x)
{
    Vector origin ;

   // Check whether to skip this node
   if ((x[0] < minx) || (maxx < x[0]) ||
        (x[1] < miny) || (maxy < x[1]))
        return ;

    origin=Vector(x[0],x[1],x[2]) ; 
    DummyEntity.v.origin.x = origin.x ;
    DummyEntity.v.origin.y = origin.y ;
    DummyEntity.v.origin.z = origin.z ;
DUMP("Adding a node",x) ;
    NodeMachine.add(origin,0,&DummyEntity) ;
    NodesFound ++ ;
}


bool CheckWaypoint(const vec3_t coord)
{
   vec3_t start, end, new_coord;
   botman_trace_t tr;

   // check if this coordinate is outside the bounds of the map...
   if ((coord[0] <= model->mins[0]) || (coord[0] >= model->maxs[0]) ||
       (coord[1] <= model->mins[1]) || (coord[1] >= model->maxs[1]) ||
       (coord[2] <= model->mins[2]) || (coord[2] >= model->maxs[2]))
   {
      return FALSE;
   }

   // Check whether to skip this node
   if ((coord[0] < minx) || (maxx < coord[0]) ||
        (coord[1] < miny) || (maxy < coord[1]))
        return FALSE;

   // Check whether coord is too close of a wall by looking
   // the contents in human_hull (where the walls are thicker)
   if (BotmanPointContentsInHull(head_hull,coord) == CONTENTS_SOLID) {
	return FALSE ;
   }

   // Check that we aren't too close to any walls (forward-backward)...
   VectorAdd(coord, wall_forward, start);
   VectorAdd(coord, wall_back, end);
   BotmanTraceLine(start, end, &tr);

   if (tr.fraction < 1.0)  // did we hit a wall?
        return FALSE ;

   // Check that we aren't too close to any walls (left-right)...
   VectorAdd(coord, wall_left, start);
   VectorAdd(coord, wall_right, end);
   BotmanTraceLine(start, end, &tr);

   if (tr.fraction < 1.0)  // did we hit a wall?
        return FALSE ;

   // no need to check for floor
   AddNode(coord) ;
   return TRUE ;
}

void GetVertexFromIndex(vec3_t x, const int VertexId)
{
    int edge ;

    // get the coordinates of the vertex of this edge...
     edge = dsurfedges[VertexId];
     if (edge < 0) { // Counter-clock wise
        edge = -edge;
        dedge_t* e = &dedges[edge];
        x[0] = dvertexes[e->v[1]].point[0];
        x[1] = dvertexes[e->v[1]].point[1];
        x[2] = dvertexes[e->v[1]].point[2];
     } else { // Clock wise
        dedge_t* e = &dedges[edge];
        x[0] = dvertexes[e->v[0]].point[0];
        x[1] = dvertexes[e->v[0]].point[1];
        x[2] = dvertexes[e->v[0]].point[2];
     }
}

static void AnalyzeAllEdges(dface_t * face) 
{
   int edge_index, edge_index2;
   vec3_t Vertex, PreviousVertex, Median, Side, ToMiddle, Point, MiddlePoint = {0.0,0.0,0.0}  ;
   vec3_t MovedVertices[MAXEDGES] ;
   vec_t scale, MaxLength, Length ;

   if (face->numedges <= 0) return ;

   MaxLength = -1.0f ;
   GetVertexFromIndex(PreviousVertex,face->firstedge+face->numedges-1) ;
   
   // Check whether to skip this face due to clipping
   if ((PreviousVertex[0] < minx) || (maxx < PreviousVertex[0]) || 
	(PreviousVertex[1] < miny) || (maxy < PreviousVertex[1]))
	return ;

   // for each face, loop though all of the edges, getting the vertexes...
   // and compute an approximation for the middle of the face
   // as well as the longest side (an rough approximation to the important of the face
   for (edge_index = 0; edge_index < face->numedges; edge_index++) {
	GetVertexFromIndex(Vertex,face->firstedge + edge_index) ;
	MiddlePoint[0] += Vertex[0] ;
	MiddlePoint[1] += Vertex[1] ;
	MiddlePoint[2] += Vertex[2] ;
	VectorSubtract(Vertex,PreviousVertex,Side) ;
	Length = VectorLength(Side) ;
	if (Length > MaxLength) MaxLength = Length ;
 	VectorCopy(Vertex, PreviousVertex) ;
   }

   // Check the importance of the polygone (using maximum length as a hint)
   if (MaxLength < 50.0f)
	return ;

   MiddlePoint[0] /= face->numedges ;
   MiddlePoint[1] /= face->numedges ;
   MiddlePoint[2] /= face->numedges ;
DUMP(">>>MiddlePoint",MiddlePoint) ;

   // Use the middle point as first waypoint for this polygone 
   MiddlePoint[2] += 36.0f ;
   CheckWaypoint(MiddlePoint) ;
   MiddlePoint[2] -= 36.0f ;

   // for each face, loop through all of the edges, getting the vertexes...
   for (edge_index = 0; edge_index < face->numedges; edge_index++) {
	GetVertexFromIndex(Vertex,face->firstedge + edge_index) ;
DUMP("Vertex",Vertex) ;
	VectorCopy(Vertex,MovedVertices[edge_index]) ;

	// Try to add this vertex as a waypoint (but first check whether it is too close to a wall)
	Vertex[2] += 36.0f ; // Raise it from floor
	if (!CheckWaypoint(Vertex)) {
		Vertex[2] -= 36.0f ; // Put it back on the ground
		// Then, move the vertex towards the center (trying to avoid walls) and add as a node
		_VectorSubtract(MiddlePoint,Vertex,ToMiddle) ;
		if (VectorLength(ToMiddle) > MOVE_FROM_EDGE) {    // If we are far from the center
		       scale = VectorNormalize(ToMiddle) ; // Make its length = 1.0
		       // Increase it enough to be far from walls that could be at the edge
		       _VectorScale(ToMiddle,MOVE_FROM_EDGE,Point) ; 
		       _VectorAdd(Vertex,Point,ToMiddle) ;
			VectorCopy(ToMiddle,MovedVertices[edge_index]) ;
		       ToMiddle[2] += 36.0f ; // add half-size of a player
DUMP("   Moved vertex",ToMiddle) ;
		       CheckWaypoint(ToMiddle) ; // Hopefully, it is now far from walls...
		} else
			VectorCopy(Vertex,MovedVertices[edge_index]) ; // Save it for later
	} else {
		Vertex[2] -= 36.0f ; // Put it back on the ground
		VectorCopy(Vertex,MovedVertices[edge_index]) ; // Save it for later
	}

	// Try to put a node on all lines (called Medians) between vertexes
	for (edge_index2 = 0 ;edge_index2 < edge_index; edge_index2++) {
		int PointCount, i ;
//printf("  Trying to compute Medians between vertices %d and %d...\n",edge_index,edge_index2) ; fflush(stdout) ;
		_VectorSubtract(MovedVertices[edge_index2],MovedVertices[edge_index],Median) ;
#ifdef NEWVERSION
		Length = VectorLength(Median) ;
		if (Length < MAX_NODES_DISTANCE) // Is Median too small to make a node on it ?
			continue ;
		// Check how many nodes must be made in order for NodeMachine:add() to work
		PointCount = Length / MAX_NODES_DISTANCE ;
		VectorNormalize(Median) ; // make Median length to 1
		for (i=1; i < PointCount; i++) {
			_VectorScale(Median,Length*(float) i/(float) PointCount, Point) ;
			_VectorAdd(MovedVertices[edge_index],Point,Point) ;
DUMP("  Median",Point) ;
			Point[2] += 36.0f ; // add half-size of a player
			// Then, add the vertex as a node
			CheckWaypoint(Point) ;
		}
#else
              if (VectorLength(Median) < 3*NODE_ZONE-1.0f) // Is Median too small to make a node on it ?
                        continue ;
               Point[0] = (MovedVertices[edge_index2][0] + MovedVertices[edge_index][0]) / 2.0 ;
               Point[1] = (MovedVertices[edge_index2][1] + MovedVertices[edge_index][1]) / 2.0 ;
               Point[2] = (MovedVertices[edge_index2][2] + MovedVertices[edge_index][2]) / 2.0 ;
//DUMP("  Median",Point) ;
               Point[2] += 36.0f ; // add half-size of a player
               // Then, add the vertex as a node
               CheckWaypoint(Point) ;
#endif
	}
   }
}

void AnalyzeAllFaces(dmodel_t * model)
{
   dface_t *face;
   dplane_t *plane;
   int face_index;

   // loop through all the faces of the BSP file...
   for (face_index = 0; face_index < model->numfaces; face_index++)
   {
      FaceCount++ ;
      // Give progress feedback to user...
      if ((face_index % 100) == 99) {
      	printf(".") ; fflush(stdout) ;
      }
      face = &dfaces[model->firstface + face_index];
      plane = &dplanes[face->planenum];
      if ((plane->normal[2] < 0.707106f)   // Based on PMB navmesh tutorial, 
	|| ((plane->normal[2] == 1) && (face->side == 1)))
		// a simple check to see whether plane is flat or walkable < 45 degree
	continue ; // Discard this face
//printf(">>>>>> Processing face %d:",face_index) ;
//printf("  in plane %d (side=%d)",face->planenum,face->side) ;
//printf(", plane.normal=(%.2f,%.2f,%.2f) and plane.dist=%.1f\n",plane->normal[0],plane->normal[1],plane->normal[2],plane->dist) ; fflush(stdout) ;
      AnalyzeAllEdges(face) ;
      WalkableCount ++ ;
   }
} 

void AddPointEntities(char * name)
{
   int ent_index, len, model_index;
   char *value;
   vec3_t origin, start, end ;
   botman_trace_t tr;
   dmodel_t * model ;
   int Count, NodesAdded ;
            
   len = strlen(name);
   ent_index = -1;
   Count = NodesAdded = 0 ;
      
   while ((ent_index = FindEntityByWildcard(ent_index, name, len)) != -1)
   {
	value = ValueForKey(&entities[ent_index], "origin");
	if (value[0]) {
		sscanf(value, "%f %f %f", &origin[0], &origin[1], &origin[2]);
	} else {
		value = ValueForKey(&entities[ent_index], "model");
		if (value[0]) {
			sscanf(value, "*%d", &model_index);

			// get the min/max coordinates of this brush model...
			model = &dmodels[model_index];

			// calculate the "origin" of the brush model...
			origin[0] = (model->mins[0] + model->maxs[0]) / 2.0f;
			origin[1] = (model->mins[1] + model->maxs[1]) / 2.0f;
			origin[2] = (model->mins[2] + model->maxs[2]) / 2.0f;
		        // Check whether to skip this entity
		        if ((origin[0] < minx) || (maxx < origin[0]) ||
			    (origin[1] < miny) || (maxy < origin[1]))
			    continue ;
		} else {
			continue ;
		}
	}
 	Count ++ ;
         
	VectorAdd(origin, up_a_little, start);

	// trace downward to find the ground from here...
	VectorAdd(start, really_down_to_ground, end);
	BotmanTraceLine(start, end, &tr);
	if (tr.fraction < 1.0)  // did we hit ground?
	{
		// raise up off of floor (or ledge) to center of player model...
		VectorAdd(tr.endpos, up_off_floor, end);
		CheckWaypoint(end);
		NodesAdded ++ ;
	}
   }     
   if (Count) {
	printf("%d %s entities processed, %d nodes added.\n",Count,name,NodesAdded) ;
	fflush(stdout) ;
   } else {
	printf("Found no %s entity.\n",name) ;
	fflush(stdout) ;
   }
}

void AddBlockEntities(char * name)
{
   int ent_index, len, model_index;
   char *value;
   dmodel_t * model ;
   int Count ;
            
   len = strlen(name);
   ent_index = -1;
   Count = 0 ;
      
   while ((ent_index = FindEntityByWildcard(ent_index, name, len)) != -1)
   {
	value = ValueForKey(&entities[ent_index], "model");
	if (value[0]) {
		sscanf(value, "*%d", &model_index);
		model = &dmodels[model_index];

		// We now need to analyze all faces of this model...
		AnalyzeAllFaces(model) ;
		Count ++ ;
	}
   }     
   if (Count) {
	printf("%d %s entities processed.\n",Count,name) ;
	fflush(stdout) ;
   } else {
	printf("Found no %s entity.\n",name) ;
	fflush(stdout) ;
   }
}

// Most of below code comes from botman BSP_tools
void AddLadders(void)
{
   int ent_index, model_index;
   char *value;
   vec3_t ladder_origin;
   dmodel_t *model;
   dface_t *face;
   dplane_t *plane;
   int face_index, edges, edge_index;
   vec3_t v[4];  // hold up to four vertexes for each face
   int vertex_index;
   int front_face_index;
   float face_width, max_face_width;
   vec3_t v_top[2], v_bottom[2], v_temp;
   vec3_t v_forward, v_backward, v_outward;
   vec3_t ladder_points[3];  // origin, "top" and "bottom" of ladder
   vec3_t v_start, v_end;
   botman_trace_t tr;
   int loop, num_segments;
   bool outward_found;
   float dist;
   int LadderCount, NodesAdded ;

   // loop through all the entities looking for "func_ladder"...
   ent_index = -1;
   OnLadder() ; // Specify that the DummyEntity is on a ladder
   LadderCount = NodesAdded = 0 ;
   while ((ent_index = FindEntityByClassname(ent_index, "func_ladder")) != -1)
   {
      value = ValueForKey(&entities[ent_index], "model");
      if (value[0])
      {
         sscanf(value, "*%d", &model_index);

         // get the "origin" of the ladder...
         model = &dmodels[model_index];

         // calculate the "origin" of the brush model...

         ladder_origin[0] = (model->mins[0] + model->maxs[0]) / 2.0f;
         ladder_origin[1] = (model->mins[1] + model->maxs[1]) / 2.0f;
         ladder_origin[2] = (model->mins[2] + model->maxs[2]) / 2.0f;
	 // Check whether to skip this entity
	 if ((ladder_origin[0] < minx) || (maxx < ladder_origin[0]) ||
	    (ladder_origin[1] < miny) || (maxy < ladder_origin[1]))
	    continue ;

         // find the "front" and "back" of the ladder.  do this by looking
         // for a faces whose vertex Z values are not all the same (i.e. it's
         // not the top face or the bottom face).  then look for the face
         // that is the widest.  this should be either the front or the back.

         max_face_width = 0.0;

         // loop though each face for this model...
         for (face_index = 0; face_index < model->numfaces; face_index++)
         {
            face = &dfaces[model->firstface + face_index];

            plane = &dplanes[face->planenum];

            for (vertex_index = 0; vertex_index < 4; vertex_index++)
            {
               v[vertex_index][0] = 0;
               v[vertex_index][1] = 0;
               v[vertex_index][2] = 0;
            }

            vertex_index = 0;

            for (edges = 0; edges < face->numedges; edges++)
            {
               // get the coordinates of the vertex of this edge...
               edge_index = dsurfedges[face->firstedge + edges];

               if (edge_index < 0)
               {
                  edge_index = -edge_index;
                  dedge_t* e = &dedges[edge_index];
                  v[vertex_index][0] = dvertexes[e->v[1]].point[0];
                  v[vertex_index][1] = dvertexes[e->v[1]].point[1];
                  v[vertex_index][2] = dvertexes[e->v[1]].point[2];
               }
               else
               {
                  dedge_t* e = &dedges[edge_index];
                  v[vertex_index][0] = dvertexes[e->v[0]].point[0];
                  v[vertex_index][1] = dvertexes[e->v[0]].point[1];
                  v[vertex_index][2] = dvertexes[e->v[0]].point[2];
               }

               if (vertex_index < 3)
                  vertex_index++;
            }

            // now look through the 4 vertexs to see if the Z coordinates
            // are the "same".

            if ((abs(long(v[0][2] - v[1][2])) < 8) && (abs(long(v[1][2] - v[2][2])) < 8) &&
                (abs(long(v[2][2] - v[3][2])) < 8) && (abs(long(v[3][2] - v[0][2])) < 8))
               continue;  // continue with next face if this is top or bottom

            // now calculate the 2D "width" of this face (ignoring Z axis)

            // is this a "base" (i.e. are the Z coordinates the "same")
            if (abs(long(v[0][2] - v[1][2])) < 8)
            {
               VectorSubtract(v[0], v[1], v_temp);
               face_width = Vector2DLength(v_temp);

               if (face_width > max_face_width)  // widest so far?
               {
                  max_face_width = face_width;
                  front_face_index = face_index;  // save it for later

                  // compare the Z coordinates of the 2 "bases"
                  if (v[0][2] > v[2][2])  // v[0] higher than v[2]?
                  {
                     VectorCopy(v[0], v_top[0]);     // save the top for later
                     VectorCopy(v[1], v_top[1]);
                     VectorCopy(v[2], v_bottom[0]);  // save the bottom for later
                     VectorCopy(v[3], v_bottom[1]);
                  }
                  else
                  {
                     VectorCopy(v[2], v_top[0]);     // save the top for later
                     VectorCopy(v[3], v_top[1]);
                     VectorCopy(v[0], v_bottom[0]);  // save the bottom for later
                     VectorCopy(v[1], v_bottom[1]);
                  }
               }
            }

            if (abs(long(v[1][2] - v[2][2])) < 8)  // is this a "base"?
            {
               VectorSubtract(v[1], v[2], v_temp);
               face_width = Vector2DLength(v_temp);

               if (face_width > max_face_width)  // widest so far?
               {
                  max_face_width = face_width;
                  front_face_index = face_index;  // save it for later

                  // compare the Z coordinates of the 2 "bases"
                  if (v[1][2] > v[0][2])  // v[0] higher than v[2]?
                  {
                     VectorCopy(v[1], v_top[0]);     // save the top for later
                     VectorCopy(v[2], v_top[1]);
                     VectorCopy(v[0], v_bottom[0]);  // save the bottom for later
                     VectorCopy(v[3], v_bottom[1]);
                  }
                  else
                  {
                     VectorCopy(v[0], v_top[0]);     // save the top for later
                     VectorCopy(v[3], v_top[1]);
                     VectorCopy(v[1], v_bottom[0]);  // save the bottom for later
                     VectorCopy(v[2], v_bottom[1]);
                  }
               }
            }
         }

         // now we know that front_face_index is either the front or back
         // trace vectors out from the ladder to see if world collisions or
         // brush model collisions occur to find out which direction this
         // func_ladder is aproachable from.

         face = &dfaces[model->firstface + front_face_index];

         plane = &dplanes[face->planenum];

         // create points in the middle of the ladder lengthwise...

         VectorCopy(ladder_origin, ladder_points[0]);  // middle of the ladder

         VectorAdd(v_top[0], v_top[1], v_temp);
         VectorScale(v_temp, 0.5, ladder_points[1]);  // middle of the top
         VectorAdd(v_bottom[0], v_bottom[1], v_temp);
         VectorScale(v_temp, 0.5, ladder_points[2]);  // middle of the bottom

         // adjust top middle and bottom middle by 10 units (towards the center)
         VectorSubtract(ladder_points[2], ladder_points[1], v_temp);
         VectorNormalize(v_temp);
         VectorScale(v_temp, 10.0, v_temp);
         VectorAdd(ladder_points[1], v_temp, ladder_points[1]);
         VectorInverse(v_temp);
         VectorAdd(ladder_points[2], v_temp, ladder_points[2]);

         // loop though the ladder points tracing a line outward in both the
         // forward and backward directions to see if we hit the world.

         VectorScale(plane->normal, 20.0, v_forward);
         VectorScale(plane->normal, 20.0, v_backward);
         VectorInverse(v_backward);

         outward_found = FALSE;

         loop = 0;
         while ((loop < 3) && (!outward_found))
         {
            VectorCopy(ladder_points[loop], v_start);
            VectorAdd(ladder_points[loop], v_forward, v_end);

            BotmanTraceLine(v_start, v_end, &tr);

            if (tr.fraction < 1.0)  // we hit something going forward
            {
               VectorCopy(v_backward, v_outward);  // outward is v_backward
               VectorNormalize(v_outward);  // make unit vector
               outward_found = TRUE;
            }
            else
            {
               VectorCopy(ladder_points[loop], v_start);
               VectorAdd(ladder_points[loop], v_backward, v_end);

               BotmanTraceLine(v_start, v_end, &tr);

               if (tr.fraction < 1.0)  // we hit something going backward
               {
                  VectorCopy(v_forward, v_outward);  // outward is forward
                  VectorNormalize(v_outward);  // make unit vector
                  outward_found = TRUE;
               }
            }

            loop++;
         }

         // if outward_found is still FALSE here then need to check for
         // collisions with other world brushes (other than func_ladder)...

         // ADD THIS CODE LATER - FIXME!!!


         if (outward_found)  // do we now know which way is outward?
         {
	    LadderCount ++ ;
            // create a waypoint at the top of the ladder
            // (remember that ladder_points[1] is already down 10 units...

            VectorSubtract(ladder_points[1], ladder_points[2], v_temp);
            VectorNormalize(v_temp);
            // a little bit lower than half of the player height...
            VectorScale(v_temp, 32.0 + 10.0, v_temp);
            VectorAdd(ladder_points[1], v_temp, v_end);  // top of ladder

            VectorScale(v_outward, 20.0, v_temp);
            VectorAdd(v_end, v_temp, v_end);  // move outward 20 units

            AddNode(v_end);  // add top waypoint
	    NodesAdded ++ ;

            // create a waypoint at the bottom of the ladder
            // (remember that ladder_points[2] is already up 10 units...
            VectorSubtract(ladder_points[1], ladder_points[2], v_temp);
            VectorNormalize(v_temp);
            // a little bit higher than half of the player height...
            VectorScale(v_temp, 44.0 - 10.0, v_temp);
            VectorAdd(ladder_points[2], v_temp, v_start);

            VectorScale(v_outward, 20.0, v_temp);
            VectorAdd(v_start, v_temp, v_start);  // move outward 20 units

            AddNode(v_start);  // add bottom waypoint
	    NodesAdded ++ ;

            // now determine how many waypoints to place in between
            // the start and end points on the ladder...

            VectorSubtract(v_end, v_start, v_temp);
            dist = VectorLength(v_temp);

            num_segments = (dist + 100.0) / NODE_ZONE;  // place every ?? units

            dist = VectorLength(v_temp) / num_segments;  // distance apart

            VectorSubtract(v_end, v_start, v_temp);
            VectorNormalize(v_temp);
            VectorScale(v_temp, dist, v_temp);  // delta vector for steps

            for (loop = 1; loop < num_segments; loop++)
            {
               VectorAdd(v_start, v_temp, v_start);
               AddNode(v_start);  // add middle waypoints
	       NodesAdded ++ ;
            }
         }
         else
         {
            printf("Ladder found, but can't tell which way is outward!\n");
         }
      }
   }
   OffLadder() ;
   if (LadderCount) {
	printf("%d ladders processed and %d nodes added.\n",LadderCount, NodesAdded) ; fflush(stdout) ;
   } else {
	printf("No ladder found.\n") ;
 	fflush(stdout) ;
   }
}

int main (int argc, char **argv)
{
   int n;
   char filename[256], * mapname, * directory;
   char * s ;

   printf("Bsp2Rbn V%s\nBased on software from PM Baty and Botman by eric@vyncke.org\n\n",Version) ;
   if ((argc != 2) && (argc != 6))
   {
      printf("Usage: %s bspfile [minx maxx miny maxy]\n",argv[0]);
      printf("  Will create .RBN and .RBX files in the current directory.\n") ;
      printf("  minx...maxy are optional and will create only nodes within this area.\n") ;
      return 1;
   }

   strcpy(filename,argv[1]) ;

   if (argc ==6) {
	minx = atoi(argv[2]) ;
	maxx = atoi(argv[3]) ;
	miny = atoi(argv[4]) ;
	maxy = atoi(argv[5]) ;
	printf("Map will be clipped between (%.0f,%0.f) and (%.0f, %.0f)\n",
		minx,miny,maxx,maxy) ;
   }

// Play with argument to find: map name, directory name, filename, ...
   directory=strdup(filename) ;
   directory = dirname(directory) ;
   mapname=strdup(filename) ;
   mapname = basename(mapname) ;
   s = strchr(mapname,'.') ;
   if ((s != NULL) && *s) *s = 0 ;

   world.LoadBSP(filename);

   model = &dmodels[0];  // model 0 is the BSP world

// Initialize the Realbot NodeMachine
   gpGlobals->pStringBase = (const char *) malloc(256) ;
   strcpy((char *) gpGlobals->pStringBase,mapname) ;
   gpGlobals->mapname = 0 ; // Offset in pStringBase

// Initialize a dummy player
   memset(&DummyEntity,0, sizeof(DummyEntity)) ;
   DummyEntity.v.pContainingEntity=NULL ;
   n= strlen(mapname)+1 ; 
   strcpy((char *) (gpGlobals->pStringBase+n),"player") ;
   DummyEntity.v.classname = n ;
   DummyEntity.v.movetype = MOVETYPE_WALK ;
   DummyEntity.v.health = 100 ;
   DummyEntity.v.deadflag = DEAD_NO ;
   DummyEntity.v.dmg_take = 0 ;
   DummyEntity.v.dmg_save = 0 ;
   DummyEntity.v.friction = 1.0f ;
   DummyEntity.v.gravity = 1.0f ;
   DummyEntity.v.button = 0 ;
   NodeMachine.init() ;

// Add important nodes using entities class names with wildcards
   AddLadders() ;
   AddBlockEntities("func_illusionary") ;
   AddBlockEntities("func_wall") ;
   AddPointEntities("info_player_") ;
   AddPointEntities("hostage_entity") ;
   AddPointEntities("info_hostage_rescue") ;
   AddPointEntities("func_hostage_rescue") ;
   AddPointEntities("func_bomb_target") ;
   AddPointEntities("info_bomb_target") ;
   AddPointEntities("func_escapezone") ;
   AddPointEntities("armoury_entity") ;
   AddPointEntities("info_vip_start") ;
   AddPointEntities("func_vip_safetyzone") ;

// Add a lot of nodes of 'worldspawn' model
   printf("Analysing all faces of worldspawn...") ; fflush(stdout) ;
   AnalyzeAllFaces(model) ;
   printf("\n%d faces analyzed; %d are walkable.\n",FaceCount,WalkableCount) ;
   printf("%d nodes were discovered by faces analysis (not all of them actually added).\n",NodesFound) ;
   fflush(stdout) ;

// Save the RBN file
   NodeMachine.Draw() ;
   printf("%s0000.bmp has been generated in the current directory.\n",mapname) ; fflush(stdout) ;
   NodeMachine.save() ;
   printf("%s.rbn has been generated in the current directory.\n",mapname) ; fflush(stdout) ;
   NodeMachine.experience_save() ;
   printf("%s.rbx has been generated in the current directory.\n",mapname) ; fflush(stdout) ;

// Success!
   return 0 ;
}

// $Log: Bsp2Rbn.cpp,v $
// Revision 1.6  2004/07/27 07:43:35  eric
// - bumped version to 0.9.6
// - slightly better 'GUI' to show progress
// - it took long, I lost hair ;-), but TraceHull is now fully implemented
//   without the HL engine, just by looking into BSP, w00t !
// - added a COPYING file for GPL
// - better wall avoidance when creating a node
// - handling of func_wall & func_illusionary (for de_aztec bridge)
//
// Revision 1.4  2004/07/19 15:22:40  eric
// - bumped version to 0.9.2
// - tried to fixed the core dump at the exit (no need to worry anyway it happens
//   after everything has been generated)
// - now handling all entities including VIP & T escape points
// - else, no change in the algorithms
//
// Revision 1.3  2004/07/18 18:52:29  eric
// - added version number, currently 0.9.2
// - added Windows version of basename & dirname functions
// - added two other utilities DrawNodes & DumpNodes
// - updated README file
// - fixed compilation warnings (thanks dstruct2k)
//
// Revision 1.2  2004/07/08 13:35:39  eric
// Fix usage help
//
// Revision 1.1  2004/07/08 08:13:06  eric
// - not sure whether this file was added
//

