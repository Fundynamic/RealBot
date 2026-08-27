// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
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


#include <cstring>
#include <cmath>
#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>
#include <entity_state.h>

#include "bot.h"
#include "bot_weapons.h"
#include "bot_func.h"
#include "game.h"
#include "NodeMachine.h"

extern int mod_id;
extern edict_t* pHostEdict;

// Obstacle Avoidance
constexpr float TURN_ANGLE = 75.0f; // Degrees to turn when avoiding obstacles
constexpr float MOVE_DISTANCE = 24.0f; // Distance to move forward
constexpr float SCAN_RADIUS = 60.0f; // Radius to scan to prevent blocking with players

// Bot dimensions and movement capabilities
constexpr float BODY_SIDE_OFFSET = 16.0f; // Offset from center to check for body clearance
constexpr float FORWARD_CHECK_DISTANCE = 24.0f; // How far forward to check for obstacles
constexpr float STAND_VIEW_HEIGHT_OFFSET = -36.0f; // Eye level offset from origin when standing
constexpr float DUCK_VIEW_HEIGHT_OFFSET = -36.0f; // Eye level offset from origin when ducking
constexpr float DUCK_HEIGHT = 36.0f; // Height of the bot when ducking
constexpr float HEAD_CLEARANCE_CHECK_HEIGHT = 108.0f; // Height from origin to check for head clearance
constexpr float JUMP_CLEARANCE_CHECK_DROP = -81.0f; // Distance to trace down for jump clearance
constexpr float DUCK_CLEARANCE_CHECK_RISE = 72.0f; // Distance to trace up for duck clearance
constexpr float FEET_OFFSET = -35.0f; // Offset from origin to near the bot's feet

/**
 * Given an angle, makes sure it wraps around properly
 * @param angle
 * @return
 */
float fixAngle(float angle) {
	// Use std::fmod for better performance with extreme values - [APG]RoboCop[CL]
    angle = std::fmod(angle, 360.0f);
    if (angle > 180.0f) angle -= 360.0f;
    else if (angle < -180.0f) angle += 360.0f;
    return angle;
}

void botFixIdealPitch(edict_t* pEdict) {
    pEdict->v.idealpitch = fixAngle(pEdict->v.idealpitch);
}

void botFixIdealYaw(edict_t* pEdict) {
    pEdict->v.ideal_yaw = fixAngle(pEdict->v.ideal_yaw);
}

bool traceLine(const Vector& v_source, const Vector& v_dest, const edict_t* pEdict, TraceResult& tr) {
    if (!pEdict || !pEdict->v.pContainingEntity) {
        return false;
    }
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
    return tr.flFraction >= 1.0f;
}

// Helper function to perform a series of traces (center, left, right)
bool traceArea(const edict_t* pEdict, const Vector& base_source, const Vector& forward, const Vector& right, const bool check_hit) {
    TraceResult tr;

    // Center trace
    if (traceLine(base_source, base_source + forward, pEdict, tr) == check_hit) return false;

    // Left trace
    Vector left_source = base_source - right * BODY_SIDE_OFFSET;
    if (traceLine(left_source, left_source + forward, pEdict, tr) == check_hit) return false;

    // Right trace
    Vector right_source = base_source + right * BODY_SIDE_OFFSET;
    if (traceLine(right_source, right_source + forward, pEdict, tr) == check_hit) return false;

    return true;
}

// Helper function to set up vectors for movement checks
void setupMovementVectors(const edict_t* pEdict, Vector& forward, Vector& right) {
    Vector angle = pEdict->v.v_angle;
    angle.x = 0;
    angle.z = 0;
    UTIL_MakeVectors(angle);
    forward = gpGlobals->v_forward;
    right = gpGlobals->v_right;
}

bool BotCanJumpUp(cBot* pBot) {
    // What I do here is trace 3 lines straight out, one unit higher than
    // the highest normal jumping distance.  I trace once at the center of
    // the body, once at the right side, and once at the left side.  If all
    // three of these TraceLines don't hit an obstruction then I know the
    // area to jump to is clear.  I then need to trace from head level,
    // above where the bot will jump to, downward to see if there is anything
    // blocking the jump.  There could be a narrow opening that the body
    // will not fit into.  These horizontal and vertical TraceLines seem
    // to catch most of the problems with falsely trying to jump on something
    // that the bot can not get onto.
    if (!pBot || !pBot->pEdict) {
        return false;
    }

    const edict_t* pEdict = pBot->pEdict;
    Vector v_forward, v_right;
    setupMovementVectors(pEdict, v_forward, v_right);

    // Horizontal check at jump height
    Vector v_source_horizontal = pEdict->v.origin + Vector(0, 0, STAND_VIEW_HEIGHT_OFFSET + static_cast<float>(MAX_JUMPHEIGHT));
    if (!traceArea(pEdict, v_source_horizontal, v_forward * FORWARD_CHECK_DISTANCE, v_right, true)) {
        return false;
    }

    // Vertical check for head clearance
    Vector v_source_vertical = pEdict->v.origin + v_forward * FORWARD_CHECK_DISTANCE;
    v_source_vertical.z += HEAD_CLEARANCE_CHECK_HEIGHT;
    if (!traceArea(pEdict, v_source_vertical, Vector(0, 0, JUMP_CLEARANCE_CHECK_DROP), v_right, true)) {
        return false;
    }

    return true;
}

bool BotCanDuckUnder(cBot* pBot) {
    // What I do here is trace 3 lines straight out, one unit higher than
    // the ducking height.  I trace once at the center of the body, once
    // at the right side, and once at the left side.  If all three of these
    // TraceLines don't hit an obstruction then I know the area to duck to
    // is clear.  I then need to trace from the ground up, 72 units, to make
    // sure that there is something blocking the TraceLine.  Then we know
    // we can duck under it.
    if (!pBot || !pBot->pEdict) {
        return false;
    }

    const edict_t* pEdict = pBot->pEdict;
    Vector v_forward, v_right;
    setupMovementVectors(pEdict, v_forward, v_right);

    // Horizontal check at duck height
    Vector v_source_horizontal = pEdict->v.origin + Vector(0, 0, DUCK_VIEW_HEIGHT_OFFSET + DUCK_HEIGHT + 1);
    if (!traceArea(pEdict, v_source_horizontal, v_forward * FORWARD_CHECK_DISTANCE, v_right, true)) {
        return false;
    }

    // Vertical check for something to duck under
    Vector v_source_vertical = pEdict->v.origin + v_forward * FORWARD_CHECK_DISTANCE;
    v_source_vertical.z += FEET_OFFSET;
    if (!traceArea(pEdict, v_source_vertical, Vector(0, 0, DUCK_CLEARANCE_CHECK_RISE), v_right, false)) {
        return false;
    }

    return true;
}

bool isBotNearby(const cBot* pBot, const float radius) {
    if (!pBot || !pBot->pEdict) {
        return false; // Validate input
    }

    const float radiusSquared = radius * radius;

    for (int i = 0; i < gpGlobals->maxClients; ++i) {
        edict_t* pPlayer = INDEXENT(i + 1);

        if (pPlayer && !pPlayer->free && pPlayer != pBot->pEdict) {
            Vector diff = pPlayer->v.origin - pBot->pEdict->v.origin;
            float distanceSquared = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
            if (distanceSquared < radiusSquared) {
                return true;
            }
        }
    }

    return false;
}

void adjustBotAngle(cBot* pBot, const float angle) {
    if (!pBot || !pBot->pEdict) {
        return;
    }

    pBot->pEdict->v.v_angle.y += angle;
    UTIL_MakeVectors(pBot->pEdict->v.v_angle);
}

void avoidClustering(cBot* pBot) {
    if (!pBot) {
        return;
    }

    if (isBotNearby(pBot, SCAN_RADIUS)) {
        adjustBotAngle(pBot, TURN_ANGLE);
    }
}

bool isPathBlocked(const cBot* pBot, const Vector& v_dest) {
    if (!pBot || !pBot->pEdict) {
        return true; // Assume blocked if input is invalid
    }

    TraceResult tr;
    return !traceLine(pBot->pEdict->v.origin, v_dest, pBot->pEdict, tr);
}

void adjustPathIfBlocked(cBot* pBot) {
    if (!pBot) {
        return;
    }

    Vector v_dest = pBot->pEdict->v.origin + gpGlobals->v_forward * MOVE_DISTANCE;

    if (isPathBlocked(pBot, v_dest)) {
        adjustBotAngle(pBot, TURN_ANGLE);
    }
}

void BotNavigate(cBot* pBot) {
    if (!pBot || !pBot->pEdict) {
        return;
    }

    // Avoid clustering with other bots
    avoidClustering(pBot);

    // Check if the path is blocked and adjust angle if necessary
    Vector v_dest = pBot->pEdict->v.origin + gpGlobals->v_forward * MOVE_DISTANCE;
    if (isPathBlocked(pBot, v_dest)) {
        adjustBotAngle(pBot, TURN_ANGLE);
        v_dest = pBot->pEdict->v.origin + gpGlobals->v_forward * MOVE_DISTANCE; // Recalculate destination
    }

    // If the path is clear, move the bot
    if (!isPathBlocked(pBot, v_dest)) {
        pBot->pEdict->v.origin = v_dest;
    }
}