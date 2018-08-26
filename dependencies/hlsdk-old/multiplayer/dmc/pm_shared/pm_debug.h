//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PM_DEBUG_H
#define PM_DEBUG_H
#ifdef _WIN32
#ifndef __MINGW32__
#pragma once
#endif /* not __MINGW32__ */
#endif

void PM_ViewEntity( void );
void PM_DrawBBox(vec3_t mins, vec3_t maxs, vec3_t origin, int pcolor, float life);
void PM_ParticleLine(vec3_t start, vec3_t end, int pcolor, float life, float vert);
void PM_ShowClipBox( void );

#endif // PMOVEDBG_H
