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
#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>
#include <string_view>


#include "bot.h"
#include "bot_client.h"
#include "bot_func.h"
#include "ChatEngine.h"

#include "engine.h"
#include "game.h"

extern enginefuncs_t g_engfuncs; //Redundant? [APG]RoboCop[CL]

extern cBot bots[32];
extern cGame Game;
extern cChatEngine ChatEngine;

extern int mod_id;
extern bool isFakeClientCommand;
extern char g_argv[1024];
extern int fake_arg_count;

// Ditlew's Radio
extern char radio_messenger[30];
extern bool radio_message;
extern char* message;
bool radio_message_start = false;
bool radio_message_from = false;
bool show_beginmessage = true;
// End Ditlew's Radio

void (*botMsgFunction)(void*, int) = nullptr;

void (*botMsgEndFunction)(void*, int) = nullptr;

int botMsgIndex;

void pfnChangeLevel(const char* s1, const char* s2) {
    // kick any bot off of the server after time/frag limit...
    for (cBot& bot : bots)
    {
        if (bot.bIsUsed)  // is this slot used?
        {
            char cmd[40];

            snprintf(cmd, sizeof(cmd), "kick \"%s\"\n", bot.name);

            bot.respawn_state = RESPAWN_NEED_TO_RESPAWN;

            SERVER_COMMAND(cmd);   // kick the bot using (kick "name")
        }
    }

    if (Game.bEngineDebug)
        rblog("ENGINE: pfnChangeLevel()\n");

    RETURN_META(MRES_IGNORED);
}

edict_t* pfnFindEntityByString(edict_t* pEdictStartSearchAfter,
    const char* pszField, const char* pszValue) {

    // Counter-Strike - New Round Started
    if (std::strcmp(pszValue, "info_map_parameters") == 0) {
        rblog("pfnFindEntityByString: Game new round\n");

        // New round started.
        Game.SetNewRound(true);
        Game.resetRoundTime();
        Game.DetermineMapGoal();
    }

    if (Game.bEngineDebug)
        rblog("ENGINE: pfnFindEntityByString()\n");


    RETURN_META_VALUE(MRES_IGNORED, NULL);
}

void pfnRemoveEntity(edict_t* e) {

#if DO_DEBUG == 2
    {
        fp = std::fopen("!rbdebug.txt", "a");
        std::fprintf(fp, R"(pfnRemoveEntity: %x)", e);
        if (e->v.model != 0)
            std::fprintf(fp, " model=%s\n", STRING(e->v.model));
        std::fclose(fp);
    }
#endif

    if (Game.bEngineDebug) {
        char msg[256];
        snprintf(msg, sizeof(msg), "ENGINE: pfnRemoveEntity() - model -> '%s'\n",
            STRING(e->v.model));
        rblog(msg);
    }

    RETURN_META(MRES_IGNORED);
}

void pfnClientCommand(edict_t* pEdict, const char* szFmt, ...) {
    // new?
    if (pEdict->v.flags & (FL_FAKECLIENT | FL_THIRDPARTYBOT))
        RETURN_META(MRES_SUPERCEDE);

    if (Game.bEngineDebug)
        rblog("ENGINE: pfnClientCommand()\n");

    RETURN_META(MRES_IGNORED);
}

void pfnMessageBegin(const int msg_dest, const int msg_type, const float* pOrigin, edict_t* edict) {

    if (Game.bEngineDebug) {
        char dmsg[256];
        snprintf(dmsg, sizeof(dmsg), "ENGINE: pfnMessageBegin(), dest=%d, msg_type=%d\n", msg_dest, msg_type);
        rblog(dmsg);
    }

    if (gpGlobals->deathmatch) {
        // Fix this up for CS 1.6 weaponlists
        // 01/07/04 - Stefan - Thanks to Whistler for pointing this out!
        if (msg_type == GET_USER_MSG_ID(PLID, "WeaponList", nullptr))
            botMsgFunction = BotClient_CS_WeaponList;

        if (edict) {
            const int index = UTIL_GetBotIndex(edict);

            // is this message for a bot?
            if (index != -1) {
                botMsgFunction = nullptr;      // no msg function until known otherwise
                botMsgEndFunction = nullptr;   // no msg end function until known otherwise
                botMsgIndex = index;        // index of bot receiving message

                if (mod_id == VALVE_DLL) {
                    if (msg_type == GET_USER_MSG_ID(PLID, "WeaponList", nullptr))
                        botMsgFunction = BotClient_Valve_WeaponList;
                    else if (msg_type ==
                        GET_USER_MSG_ID(PLID, "CurWeapon", nullptr))
                        botMsgFunction = BotClient_Valve_CurrentWeapon;
                    else if (msg_type == GET_USER_MSG_ID(PLID, "AmmoX", nullptr))
                        botMsgFunction = BotClient_Valve_AmmoX;
                    else if (msg_type ==
                        GET_USER_MSG_ID(PLID, "AmmoPickup", nullptr))
                        botMsgFunction = BotClient_Valve_AmmoPickup;
                    else if (msg_type ==
                        GET_USER_MSG_ID(PLID, "WeapPickup", nullptr))
                        botMsgFunction = BotClient_Valve_WeaponPickup;
                    else if (msg_type ==
                        GET_USER_MSG_ID(PLID, "ItemPickup", nullptr))
                        botMsgFunction = BotClient_Valve_ItemPickup;
                    else if (msg_type == GET_USER_MSG_ID(PLID, "Health", nullptr))
                        botMsgFunction = BotClient_Valve_Health;
                    else if (msg_type == GET_USER_MSG_ID(PLID, "Battery", nullptr))
                        botMsgFunction = BotClient_Valve_Battery;
                    else if (msg_type ==
                        GET_USER_MSG_ID(PLID, "DeathMsg", nullptr))
                        botMsgFunction = BotClient_Valve_Damage;
                    else if (msg_type ==
                        GET_USER_MSG_ID(PLID, "ScreenFade", nullptr))
                        botMsgFunction = BotClient_Valve_ScreenFade;
                }
                else if (mod_id == CSTRIKE_DLL) {
                    if (msg_type == GET_USER_MSG_ID(PLID, "VGUIMenu", nullptr))
                        botMsgFunction = BotClient_CS_VGUI;
                    else if (msg_type ==
                        GET_USER_MSG_ID(PLID, "ShowMenu", nullptr))
                        botMsgFunction = BotClient_CS_ShowMenu;
                    else if (msg_type ==
                        GET_USER_MSG_ID(PLID, "WeaponList", nullptr)) {
                        botMsgFunction = BotClient_CS_WeaponList;
                        //DebugOut("BUGBUG: WEAPONLIST FUNCTION CALLED\n");
                    }
                    else if (msg_type ==
                        GET_USER_MSG_ID(PLID, "CurWeapon", nullptr))
                        botMsgFunction = BotClient_CS_CurrentWeapon;
                    else if (msg_type == GET_USER_MSG_ID(PLID, "AmmoX", nullptr))
                        botMsgFunction = BotClient_CS_AmmoX;
                    else if (msg_type ==
                        GET_USER_MSG_ID(PLID, "WeapPickup", nullptr))
                        botMsgFunction = BotClient_CS_WeaponPickup;
                    else if (msg_type ==
                        GET_USER_MSG_ID(PLID, "AmmoPickup", nullptr))
                        botMsgFunction = BotClient_CS_AmmoPickup;
                    else if (msg_type ==
                        GET_USER_MSG_ID(PLID, "ItemPickup", nullptr))
                        botMsgFunction = BotClient_CS_ItemPickup;
                    else if (msg_type == GET_USER_MSG_ID(PLID, "Health", nullptr))
                        botMsgFunction = BotClient_CS_Health;
                    else if (msg_type == GET_USER_MSG_ID(PLID, "Battery", nullptr))
                        botMsgFunction = BotClient_CS_Battery;
                    else if (msg_type == GET_USER_MSG_ID(PLID, "Damage", nullptr))
                        botMsgFunction = BotClient_CS_Damage;
                    else if (msg_type ==
                        GET_USER_MSG_ID(PLID, "StatusIcon", nullptr)) {
                        BotClient_CS_StatusIcon(nullptr, -1);       // clear state -- redo this -- BERKED
                        botMsgFunction = BotClient_CS_StatusIcon;
                    }
                    if (msg_type == GET_USER_MSG_ID(PLID, "SayText", nullptr)) {
                        botMsgFunction = BotClient_CS_SayText;
                    } else if (msg_type == GET_USER_MSG_ID(PLID, "Money", nullptr))
                        botMsgFunction = BotClient_CS_Money;
                    else if (msg_type ==
                        GET_USER_MSG_ID(PLID, "ScreenFade", nullptr))
                        botMsgFunction = BotClient_CS_ScreenFade;
                }
            }
        } else if (msg_dest == MSG_ALL) {
            botMsgFunction = nullptr; // no msg function until known otherwise
            botMsgIndex = -1;      // index of bot receiving message (none)

            if (mod_id == VALVE_DLL) {
                if (msg_type == GET_USER_MSG_ID(PLID, "DeathMsg", nullptr))
                    botMsgFunction = BotClient_Valve_DeathMsg;
            } else if (mod_id == CSTRIKE_DLL) {
                if (msg_type == GET_USER_MSG_ID(PLID, "DeathMsg", nullptr))
                    botMsgFunction = BotClient_CS_DeathMsg;
                else if (msg_type == GET_USER_MSG_ID(PLID, "SayText", nullptr))
                    botMsgFunction = BotClient_CS_SayText;
            }
        }
        // STEFAN
        else if (msg_dest == MSG_SPEC) {
            botMsgFunction = nullptr; // no msg function until known otherwise
            botMsgIndex = -1;      // index of bot receiving message (none)

            if (mod_id == CSTRIKE_DLL) {
                if (msg_type == GET_USER_MSG_ID(PLID, "HLTV", nullptr)) {
                    botMsgFunction = BotClient_CS_HLTV;
                }
                else if (msg_type == GET_USER_MSG_ID(PLID, "SayText", nullptr))
                    botMsgFunction = BotClient_CS_SayText;
            }

        }
        // STEFAN END
    }

    RETURN_META(MRES_IGNORED);
}

void pfnMessageEnd() {
    if (gpGlobals->deathmatch) {
        if (botMsgEndFunction)
            (*botMsgEndFunction)(nullptr, botMsgIndex);      // NULL indicated msg end

        // clear out the bot message function pointers...
        botMsgFunction = nullptr;
        botMsgEndFunction = nullptr;
    }

    if (Game.bEngineDebug)
        rblog("ENGINE: pfnMessageEnd()\n");

    RETURN_META(MRES_IGNORED);
}

void pfnWriteByte(int iValue) {
    if (gpGlobals->deathmatch) {
        // if this message is for a bot, call the client message function...
        if (botMsgFunction)
            (*botMsgFunction)(static_cast<void*>(&iValue), botMsgIndex);
    }

    if (Game.bEngineDebug) {
        char msg[256];
        snprintf(msg, sizeof(msg), "ENGINE: pfnWriteByte() - '%d'\n", iValue);
        rblog(msg);
    }

    RETURN_META(MRES_IGNORED);
}

void pfnWriteChar(int iValue) {
    if (gpGlobals->deathmatch) {
        // if this message is for a bot, call the client message function...
        if (botMsgFunction)
            (*botMsgFunction)(static_cast<void*>(&iValue), botMsgIndex);
    }

    if (Game.bEngineDebug) {
        char msg[256];
        snprintf(msg, sizeof(msg), "ENGINE: pfnWriteChar() - '%d'\n", iValue);
        rblog(msg);
    }
    RETURN_META(MRES_IGNORED);
}

void pfnWriteShort(int iValue) {
    if (gpGlobals->deathmatch) {
        // if this message is for a bot, call the client message function...
        if (botMsgFunction)
            (*botMsgFunction)(static_cast<void*>(&iValue), botMsgIndex);
    }

    if (Game.bEngineDebug) {
        char msg[256];
        snprintf(msg, sizeof(msg), "ENGINE: pfnWriteShort() - '%d'\n", iValue);
        rblog(msg);
    }

    RETURN_META(MRES_IGNORED);
}

void pfnWriteLong(int iValue) {
    if (gpGlobals->deathmatch) {
        // if this message is for a bot, call the client message function...
        if (botMsgFunction)
            (*botMsgFunction)(static_cast<void*>(&iValue), botMsgIndex);
    }

    if (Game.bEngineDebug) {
        char msg[256];
        snprintf(msg, sizeof(msg), "ENGINE: pfnWriteLong() - '%d'\n", iValue);
        rblog(msg);
    }


    RETURN_META(MRES_IGNORED);
}

void pfnWriteAngle(float flValue) {
    if (gpGlobals->deathmatch) {
        // if this message is for a bot, call the client message function...
        if (botMsgFunction)
            (*botMsgFunction)(static_cast<void*>(&flValue), botMsgIndex);
    }

    if (Game.bEngineDebug) {
        char msg[256];
        snprintf(msg, sizeof(msg), "ENGINE: pfnWriteAngle() - '%f'\n", flValue);
        rblog(msg);
    }


    RETURN_META(MRES_IGNORED);
}

void pfnWriteCoord(float flValue) {
    if (gpGlobals->deathmatch) {
        // if this message is for a bot, call the client message function...
        if (botMsgFunction)
            (*botMsgFunction)(static_cast<void*>(&flValue), botMsgIndex);
    }

    if (Game.bEngineDebug) {
        char msg[256];
        snprintf(msg, sizeof(msg), "ENGINE: pfnWriteCoord() - '%f'\n", flValue);
        rblog(msg);
    }


    RETURN_META(MRES_IGNORED);
}

void pfnWriteString(const char *sz) {
    if (sz == nullptr)
    {
        if (Game.bEngineDebug)
            rblog("ENGINE: pfnWriteString() - sz is null\n");
        RETURN_META(MRES_IGNORED);
    }

    if (Game.bEngineDebug) {
        char msg[256];
        snprintf(msg, sizeof(msg), "ENGINE: pfnWriteByte() - '%s'\n", sz);
        rblog(msg);
    }

    if (gpGlobals->deathmatch) {
        // Ditlew's Radio
        std::string_view sz_sv(sz);
        if (sz_sv.find("(RADIO):") != std::string_view::npos && !radio_message) {
            radio_message = true;

            if (size_t radio_ptr_pos = sz_sv.find(" (RADIO)"); radio_ptr_pos != std::string_view::npos) {
                std::string_view messenger_sv = sz_sv.substr(0, radio_ptr_pos);
                strncpy(radio_messenger, messenger_sv.data(), messenger_sv.length());
                radio_messenger[messenger_sv.length()] = '\0';
            }

            if (sz_sv.find("Follow Me") != std::string_view::npos) {
                strcpy(message, "#Follow me");
            }
            else if (sz_sv.find("You Take the Point") != std::string_view::npos) {
                strcpy(message, "#You_take_the_point");
            }
            else if (sz_sv.find("Need backup") != std::string_view::npos) {
                strcpy(message, "#Need_backup");
            }
            else if (sz_sv.find("Enemy spotted") != std::string_view::npos) {
                strcpy(message, "#Enemy_spotted");
            }
            else if (sz_sv.find("Taking Fire.. Need Assistance!") != std::string_view::npos) {
                strcpy(message, "#Taking_fire");
            }
            else if (sz_sv.find("Team, fall back!") != std::string_view::npos) {
                strcpy(message, "#Team_fall_back");
            }
            else if (sz_sv.find("Go go go") != std::string_view::npos) {
                strcpy(message, "#Go_go_go");
            }
        }

        if (radio_message_start) {
            std::strcpy(radio_messenger, sz);
            radio_message_start = false;
            radio_message_from = true;
        }
        else if (radio_message_from) {
            std::strcpy(message, sz);
            radio_message = true;
            radio_message_from = false;
        }
        else if (sz_sv == "#Game_radio") {
            radio_message_start = true;
        }

        // End Ditlew's Radio

        // if this message is for a bot, call the client message function...
        if (botMsgFunction) {
            (*botMsgFunction)((void*)sz, botMsgIndex);
        }
    }

    RETURN_META(MRES_IGNORED);
}

void pfnWriteEntity(int iValue) {
    if (gpGlobals->deathmatch) {
        // if this message is for a bot, call the client message function...
        if (botMsgFunction)
            (*botMsgFunction)(static_cast<void*>(&iValue), botMsgIndex);
    }

    RETURN_META(MRES_IGNORED);
}

void pfnClientPrintf(edict_t* pEdict, PRINT_TYPE ptype, const char* szMsg) {
    // prevent bots sending these kind of messages
    if (pEdict->v.flags & (FL_FAKECLIENT | FL_THIRDPARTYBOT))
        RETURN_META(MRES_SUPERCEDE);
    RETURN_META(MRES_IGNORED);
}

const char* pfnCmd_Args() {
    if (isFakeClientCommand)
        RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[0]);
    RETURN_META_VALUE(MRES_IGNORED, NULL);
}

const char* pfnCmd_Argv(const int argc) {
    if (isFakeClientCommand) {
        if (argc == 0)
            RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[64]);
        if (argc == 1)
            RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[128]);
        if (argc == 2)
            RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[192]);
        RETURN_META_VALUE(MRES_SUPERCEDE, NULL);
    }
    RETURN_META_VALUE(MRES_IGNORED, NULL);
}

int pfnCmd_Argc() {
    if (isFakeClientCommand)
        RETURN_META_VALUE(MRES_SUPERCEDE, fake_arg_count);
    RETURN_META_VALUE(MRES_IGNORED, 0);
}

void pfnSetClientMaxspeed(const edict_t* pEdict, const float fNewMaxspeed) {
    // Set client max speed (CS / All mods)

    // Check if edict_t is a bot, then set maxspeed
    cBot* pPlayerBot = nullptr;
    int index;

    for (index = 0; index < 32; index++) {
        if (bots[index].pEdict == pEdict) {
            break;
        }
    }

    if (index < 32)
        pPlayerBot = (&bots[index]);

    if (pPlayerBot)
        pPlayerBot->f_max_speed = fNewMaxspeed;

    RETURN_META(MRES_IGNORED);
}

int pfnGetPlayerUserId(edict_t* e) {
    if (gpGlobals->deathmatch) {
        //if (mod_id == GEARBOX_DLL)
        //{
        //   // is this edict a bot?
        //   if (UTIL_GetBotPointer( e ))
        //      return 0;  // don't return a valid index (so bot won't get kicked)
        //}
    }

    RETURN_META_VALUE(MRES_IGNORED, 0);
}

C_DLLEXPORT int
GetEngineFunctions(enginefuncs_t* pengfuncsFromEngine,
    int* interfaceVersion) {
    meta_engfuncs.pfnChangeLevel = pfnChangeLevel;
    meta_engfuncs.pfnFindEntityByString = pfnFindEntityByString;
    meta_engfuncs.pfnRemoveEntity = pfnRemoveEntity;
    meta_engfuncs.pfnClientCommand = pfnClientCommand;
    meta_engfuncs.pfnMessageBegin = pfnMessageBegin;
    meta_engfuncs.pfnMessageEnd = pfnMessageEnd;
    meta_engfuncs.pfnWriteByte = pfnWriteByte;
    meta_engfuncs.pfnWriteChar = pfnWriteChar;
    meta_engfuncs.pfnWriteShort = pfnWriteShort;
    meta_engfuncs.pfnWriteLong = pfnWriteLong;
    meta_engfuncs.pfnWriteAngle = pfnWriteAngle;
    meta_engfuncs.pfnWriteCoord = pfnWriteCoord;
    meta_engfuncs.pfnWriteString = pfnWriteString;
    meta_engfuncs.pfnWriteEntity = pfnWriteEntity;
    meta_engfuncs.pfnClientPrintf = pfnClientPrintf;
    meta_engfuncs.pfnCmd_Args = pfnCmd_Args;
    meta_engfuncs.pfnCmd_Argv = pfnCmd_Argv;
    meta_engfuncs.pfnCmd_Argc = pfnCmd_Argc;
    meta_engfuncs.pfnSetClientMaxspeed = pfnSetClientMaxspeed;
    meta_engfuncs.pfnGetPlayerUserId = pfnGetPlayerUserId;
    std::memcpy(pengfuncsFromEngine, &meta_engfuncs, sizeof(enginefuncs_t));
    return 1;
}
