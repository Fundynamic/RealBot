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

#include <string.h>
#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_client.h"
#include "bot_func.h"
#include "ChatEngine.h"

//#include "engine.h"
#include "game.h"

extern enginefuncs_t g_engfuncs;

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
extern char *message;
bool radio_message_start = false;
bool radio_message_from = false;
bool show_beginmessage = true;
// End Ditlew's Radio

void (*botMsgFunction)(void *, int) = NULL;

void (*botMsgEndFunction)(void *, int) = NULL;

int botMsgIndex;

void pfnChangeLevel(const char *s1, const char *s2) {
    // kick any bot off of the server after time/frag limit...
    for (int index = 0; index < 32; index++) {
        if (bots[index].bIsUsed)  // is this slot used?
        {
            char cmd[40];

            sprintf(cmd, "kick \"%s\"\n", bots[index].name);

            bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;

            SERVER_COMMAND(cmd);   // kick the bot using (kick "name")
        }
    }

    if (Game.bEngineDebug)
        rblog("ENGINE: pfnChangeLevel()\n");

    RETURN_META(MRES_IGNORED);
}

edict_t *pfnFindEntityByString(edict_t *pEdictStartSearchAfter,
                               const char *pszField, const char *pszValue) {

    // Counter-Strike - New Round Started
    if (strcmp(pszValue, "info_map_parameters") == 0) {
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

void pfnRemoveEntity(edict_t *e) {

#if DO_DEBUG == 2
    {
       fp = fopen("!rbdebug.txt", "a");
       fprintf(fp, "pfnRemoveEntity: %x\n", e);
       if (e->v.model != 0)
          fprintf(fp, " model=%s\n", STRING(e->v.model));
       fclose(fp);
    }
#endif

    if (Game.bEngineDebug) {
        char msg[256];
        sprintf(msg, "ENGINE: pfnRemoveEntity() - model -> '%s'\n",
                STRING(e->v.model));
        rblog(msg);
    }

    RETURN_META(MRES_IGNORED);
}

void pfnClientCommand(edict_t *pEdict, const char *szFmt, ...) {
    // new?
    if (pEdict->v.flags & (FL_FAKECLIENT | FL_THIRDPARTYBOT))
        RETURN_META(MRES_SUPERCEDE);

    if (Game.bEngineDebug)
        rblog("ENGINE: pfnClientCommand()\n");

    RETURN_META(MRES_IGNORED);
}

void
pfnMessageBegin(int msg_dest, int msg_type, const float *pOrigin, edict_t *edict) {

    if (Game.bEngineDebug) {
        char dmsg[256];
        sprintf(dmsg, "ENGINE: pfnMessageBegin(), dest=%d, msg_type=%d\n", msg_dest, msg_type);
        rblog(dmsg);
    }

    if (gpGlobals->deathmatch) {
        int index = -1;

        // Fix this up for CS 1.6 weaponlists
        // 01/07/04 - Stefan - Thanks to Whistler for pointing this out!
        if (msg_type == GET_USER_MSG_ID(PLID, "WeaponList", NULL))
            botMsgFunction = BotClient_CS_WeaponList;

        if (edict) {
            index = UTIL_GetBotIndex(edict);

            // is this message for a bot?
            if (index != -1) {
                botMsgFunction = NULL;      // no msg function until known otherwise
                botMsgEndFunction = NULL;   // no msg end function until known otherwise
                botMsgIndex = index;        // index of bot receiving message

                if (mod_id == VALVE_DLL) {
                    if (msg_type == GET_USER_MSG_ID(PLID, "WeaponList", NULL))
                        botMsgFunction = BotClient_Valve_WeaponList;
                    else if (msg_type ==
                             GET_USER_MSG_ID(PLID, "CurWeapon", NULL))
                        botMsgFunction = BotClient_Valve_CurrentWeapon;
                    else if (msg_type == GET_USER_MSG_ID(PLID, "AmmoX", NULL))
                        botMsgFunction = BotClient_Valve_AmmoX;
                    else if (msg_type ==
                             GET_USER_MSG_ID(PLID, "AmmoPickup", NULL))
                        botMsgFunction = BotClient_Valve_AmmoPickup;
                    else if (msg_type ==
                             GET_USER_MSG_ID(PLID, "WeapPickup", NULL))
                        botMsgFunction = BotClient_Valve_WeaponPickup;
                    else if (msg_type ==
                             GET_USER_MSG_ID(PLID, "ItemPickup", NULL))
                        botMsgFunction = BotClient_Valve_ItemPickup;
                    else if (msg_type == GET_USER_MSG_ID(PLID, "Health", NULL))
                        botMsgFunction = BotClient_Valve_Health;
                    else if (msg_type == GET_USER_MSG_ID(PLID, "Battery", NULL))
                        botMsgFunction = BotClient_Valve_Battery;
                    else if (msg_type ==
                             GET_USER_MSG_ID(PLID, "DeathMsg", NULL))
                        botMsgFunction = BotClient_Valve_Damage;
                    else if (msg_type ==
                             GET_USER_MSG_ID(PLID, "ScreenFade", NULL))
                        botMsgFunction = BotClient_Valve_ScreenFade;
                } else if (mod_id == CSTRIKE_DLL) {
                    if (msg_type == GET_USER_MSG_ID(PLID, "VGUIMenu", NULL))
                        botMsgFunction = BotClient_CS_VGUI;
                    else if (msg_type ==
                             GET_USER_MSG_ID(PLID, "ShowMenu", NULL))
                        botMsgFunction = BotClient_CS_ShowMenu;
                    else if (msg_type ==
                             GET_USER_MSG_ID(PLID, "WeaponList", NULL)) {
                        botMsgFunction = BotClient_CS_WeaponList;
                        //DebugOut("BUGBUG: WEAPONLIST FUNCTION CALLED\n");
                    } else if (msg_type ==
                               GET_USER_MSG_ID(PLID, "CurWeapon", NULL))
                        botMsgFunction = BotClient_CS_CurrentWeapon;
                    else if (msg_type == GET_USER_MSG_ID(PLID, "AmmoX", NULL))
                        botMsgFunction = BotClient_CS_AmmoX;
                    else if (msg_type ==
                             GET_USER_MSG_ID(PLID, "WeapPickup", NULL))
                        botMsgFunction = BotClient_CS_WeaponPickup;
                    else if (msg_type ==
                             GET_USER_MSG_ID(PLID, "AmmoPickup", NULL))
                        botMsgFunction = BotClient_CS_AmmoPickup;
                    else if (msg_type ==
                             GET_USER_MSG_ID(PLID, "ItemPickup", NULL))
                        botMsgFunction = BotClient_CS_ItemPickup;
                    else if (msg_type == GET_USER_MSG_ID(PLID, "Health", NULL))
                        botMsgFunction = BotClient_CS_Health;
                    else if (msg_type == GET_USER_MSG_ID(PLID, "Battery", NULL))
                        botMsgFunction = BotClient_CS_Battery;
                    else if (msg_type == GET_USER_MSG_ID(PLID, "Damage", NULL))
                        botMsgFunction = BotClient_CS_Damage;
                    else if (msg_type ==
                             GET_USER_MSG_ID(PLID, "StatusIcon", NULL)) {
                        BotClient_CS_StatusIcon(0, -1);       // clear state -- redo this -- BERKED
                        botMsgFunction = BotClient_CS_StatusIcon;
                    }
                    if (msg_type == GET_USER_MSG_ID(PLID, "SayText", NULL)) {
                        botMsgFunction = BotClient_CS_SayText;
                    } else if (msg_type == GET_USER_MSG_ID(PLID, "Money", NULL))
                        botMsgFunction = BotClient_CS_Money;
                    else if (msg_type ==
                             GET_USER_MSG_ID(PLID, "ScreenFade", NULL))
                        botMsgFunction = BotClient_CS_ScreenFade;
                }
            }
        } else if (msg_dest == MSG_ALL) {
            botMsgFunction = NULL; // no msg function until known otherwise
            botMsgIndex = -1;      // index of bot receiving message (none)

            if (mod_id == VALVE_DLL) {
                if (msg_type == GET_USER_MSG_ID(PLID, "DeathMsg", NULL))
                    botMsgFunction = BotClient_Valve_DeathMsg;
            } else if (mod_id == CSTRIKE_DLL) {
                if (msg_type == GET_USER_MSG_ID(PLID, "DeathMsg", NULL))
                    botMsgFunction = BotClient_CS_DeathMsg;
                else if (msg_type == GET_USER_MSG_ID(PLID, "SayText", NULL))
                    botMsgFunction = BotClient_CS_SayText;
            }
        }
            // STEFAN
        else if (msg_dest == MSG_SPEC) {
            botMsgFunction = NULL; // no msg function until known otherwise
            botMsgIndex = -1;      // index of bot receiving message (none)

            if (mod_id == CSTRIKE_DLL) {
                if (msg_type == GET_USER_MSG_ID(PLID, "HLTV", NULL)) {
                    botMsgFunction = BotClient_CS_HLTV;
                }
                else if (msg_type == GET_USER_MSG_ID(PLID, "SayText", NULL))
                    botMsgFunction = BotClient_CS_SayText;
            }

        }
        // STEFAN END
    }

    RETURN_META(MRES_IGNORED);
}

void pfnMessageEnd(void) {
    if (gpGlobals->deathmatch) {
        if (botMsgEndFunction)
            (*botMsgEndFunction)(NULL, botMsgIndex);      // NULL indicated msg end

        // clear out the bot message function pointers...
        botMsgFunction = NULL;
        botMsgEndFunction = NULL;
    }

    if (Game.bEngineDebug)
        rblog("ENGINE: pfnMessageEnd()\n");

    RETURN_META(MRES_IGNORED);
}

void pfnWriteByte(int iValue) {
    if (gpGlobals->deathmatch) {
        // if this message is for a bot, call the client message function...
        if (botMsgFunction)
            (*botMsgFunction)((void *) &iValue, botMsgIndex);
    }

    if (Game.bEngineDebug) {
        char msg[256];
        sprintf(msg, "ENGINE: pfnWriteByte() - '%d'\n", iValue);
        rblog(msg);
    }

    RETURN_META(MRES_IGNORED);
}

void pfnWriteChar(int iValue) {
    if (gpGlobals->deathmatch) {
        // if this message is for a bot, call the client message function...
        if (botMsgFunction)
            (*botMsgFunction)((void *) &iValue, botMsgIndex);
    }

    if (Game.bEngineDebug) {
        char msg[256];
        sprintf(msg, "ENGINE: pfnWriteChar() - '%d'\n", iValue);
        rblog(msg);
    }
    RETURN_META(MRES_IGNORED);
}

void pfnWriteShort(int iValue) {
    if (gpGlobals->deathmatch) {
        // if this message is for a bot, call the client message function...
        if (botMsgFunction)
            (*botMsgFunction)((void *) &iValue, botMsgIndex);
    }

    if (Game.bEngineDebug) {
        char msg[256];
        sprintf(msg, "ENGINE: pfnWriteShort() - '%d'\n", iValue);
        rblog(msg);
    }

    RETURN_META(MRES_IGNORED);
}

void pfnWriteLong(int iValue) {
    if (gpGlobals->deathmatch) {
        // if this message is for a bot, call the client message function...
        if (botMsgFunction)
            (*botMsgFunction)((void *) &iValue, botMsgIndex);
    }

    if (Game.bEngineDebug) {
        char msg[256];
        sprintf(msg, "ENGINE: pfnWriteLong() - '%d'\n", iValue);
        rblog(msg);
    }


    RETURN_META(MRES_IGNORED);
}

void pfnWriteAngle(float flValue) {
    if (gpGlobals->deathmatch) {
        // if this message is for a bot, call the client message function...
        if (botMsgFunction)
            (*botMsgFunction)((void *) &flValue, botMsgIndex);
    }

    if (Game.bEngineDebug) {
        char msg[256];
        sprintf(msg, "ENGINE: pfnWriteAngle() - '%f'\n", flValue);
        rblog(msg);
    }


    RETURN_META(MRES_IGNORED);
}

void pfnWriteCoord(float flValue) {
    if (gpGlobals->deathmatch) {
        // if this message is for a bot, call the client message function...
        if (botMsgFunction)
            (*botMsgFunction)((void *) &flValue, botMsgIndex);
    }

    if (Game.bEngineDebug) {
        char msg[256];
        sprintf(msg, "ENGINE: pfnWriteCoord() - '%f'\n", flValue);
        rblog(msg);
    }


    RETURN_META(MRES_IGNORED);
}

void pfnWriteString(const char *sz) {
    if (Game.bEngineDebug) {
        char msg[256];
        sprintf(msg, "ENGINE: pfnWriteByte() - '%s'\n", sz);
        rblog(msg);
    }

    if (gpGlobals->deathmatch) {
        // Ditlew's Radio
        if ((strstr(sz, "(RADIO):") != NULL) && !radio_message) {
            // We found an old radio message, we should convert the strings...
            radio_message = true;  // we found a radio message

            // Thank god Ditlew you already coded this...
            int length = strlen(sz) - strlen(strstr(sz, " (RADIO)"));
            strncpy(radio_messenger, sz, length);

            // Now search for any compatible radio command (old string).
            // if found then convert the message in the new way so the code
            // thinks its CS 1.1 and thus every version lower then CS 1.1 should work too...
            if ((strstr(sz, "Follow Me") != NULL)) {
                // convert string
                strcpy(message, "#Follow me");
            } else if ((strstr(sz, "You Take the Point") != NULL)) {
                // convert string
                strcpy(message, "#You_take_the_point");
            } else if ((strstr(sz, "Need backup") != NULL)) {
                // convert string
                strcpy(message, "#Need_backup");
            } else if ((strstr(sz, "Taking Fire.. Need Assistance!") != NULL)) {
                // convert string
                strcpy(message, "#Taking_fire");
            } else if ((strstr(sz, "Team, fall back!") != NULL)) {
                // convert string
                strcpy(message, "#Team_fall_back");
            } else if ((strstr(sz, "Go go go") != NULL)) {
                // convert string
                strcpy(message, "#Go_go_go");
            }

        }
        /*
           else
           {

           // normal text message
           if (strstr(sz, " : "))
           {

           // Get sender
           // Thank god Ditlew you already coded this...
           int length = strlen (sz) - strlen (strstr (sz, " : "));

           char messenger[30];
           char chMessage[80];
           for (int clear=0; clear < 80; clear ++)
           {
           if (clear < 30)
           messenger[clear]='\0';

           chMessage[clear]='\0';
           }

           // Find the sender (old fasioned way)
           int iM=0;
           bool SkippedFirst=false;
           for (int scan=1; scan < 80; scan++)
           {
           if (sz[scan] == ':')
           {
           // now cut one from the messenger
           //messenger[iM] = '\0';
           length=scan-1;
           scan++;
           break;
           }
           else if (sz[scan] != ' ' && SkippedFirst)
           {
           //messenger[iM] = sz[scan];
           iM++;
           }
           else if (sz[scan] == ' ')
           {
           if(SkippedFirst==false)
           SkippedFirst=true;
           else
           {
           // messenger[iM] = sz[scan];
           iM++;
           }
           }
           }

           strncpy (messenger, sz, length);
           SERVER_PRINT("MESSENGER:");
           SERVER_PRINT(messenger);



           // Everything else is just the sentence:
           int chM=0;
           for (scan; scan < 80; scan++)
           {
           chMessage[chM] = sz[scan];
           chM++;
           }


           ChatEngine.set_sentence(messenger, chMessage);

           }
           }
         */
        if (radio_message_start) {
            strcpy(radio_messenger, sz);   // the messenger of the radio
            radio_message_start = false;
            radio_message_from = true;
        } else if (radio_message_from) {
            strcpy(message, sz);   // copy message and handle at bot.cpp radio routine.
            radio_message = true;
            radio_message_from = false;
        } else if (strcmp(sz, "#Game_radio") == 0) {
            radio_message_start = true;
        }

        // End Ditlew's Radio

        // here it is not radio
        // here it is not radio



        // if this message is for a bot, call the client message function...
        if (botMsgFunction) {
            (*botMsgFunction)((void *) sz, botMsgIndex);
        }
    }

    RETURN_META(MRES_IGNORED);
}

void pfnWriteEntity(int iValue) {
    if (gpGlobals->deathmatch) {
        // if this message is for a bot, call the client message function...
        if (botMsgFunction)
            (*botMsgFunction)((void *) &iValue, botMsgIndex);
    }

    RETURN_META(MRES_IGNORED);
}

void pfnClientPrintf(edict_t *pEdict, PRINT_TYPE ptype, const char *szMsg) {
    // prevent bots sending these kind of messages
    if (pEdict->v.flags & (FL_FAKECLIENT | FL_THIRDPARTYBOT))
        RETURN_META(MRES_SUPERCEDE);
    RETURN_META(MRES_IGNORED);
}

const char *pfnCmd_Args(void) {
    if (isFakeClientCommand)
        RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[0]);
    RETURN_META_VALUE(MRES_IGNORED, NULL);
}

const char *pfnCmd_Argv(int argc) {
    if (isFakeClientCommand) {
        if (argc == 0)
            RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[64]);

        else if (argc == 1)
            RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[128]);

        else if (argc == 2)
            RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[192]);

        else
            RETURN_META_VALUE(MRES_SUPERCEDE, NULL);
    }
    RETURN_META_VALUE(MRES_IGNORED, NULL);
}

int pfnCmd_Argc(void) {
    if (isFakeClientCommand)
        RETURN_META_VALUE(MRES_SUPERCEDE, fake_arg_count);
    RETURN_META_VALUE(MRES_IGNORED, 0);
}

void pfnSetClientMaxspeed(const edict_t *pEdict, float fNewMaxspeed) {
    // Set client max speed (CS / All mods)

    // Check if edict_t is a bot, then set maxspeed
    cBot *pPlayerBot = NULL;
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

int pfnGetPlayerUserId(edict_t *e) {
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
GetEngineFunctions(enginefuncs_t *pengfuncsFromEngine,
                   int *interfaceVersion) {
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
    memcpy(pengfuncsFromEngine, &meta_engfuncs, sizeof(enginefuncs_t));
    return TRUE;
}
