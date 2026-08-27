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

#include <algorithm>
#include <cstring>
#include <unordered_set>
#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>
#include <entity_state.h>

#include "bot.h"
#include "bot_weapons.h"
#include "bot_func.h"
#include "game.h"

extern int mod_id;
extern int counterstrike;

extern cGame Game;

// Radio code
extern bool radio_message;

weapon_price_table weapons_table[32];

void LoadDefaultWeaponTable() {                               /* REMOVED */
}

// Returns price of a weapon, checking on WEAPON_ID (of CS)
int PriceWeapon(const int weapon_id) {
    return weapons_table[weapons_table[weapon_id].iIdIndex].price;
}

// Returns the ID in the list, checking on WEAPON_ID (of CS)
int ListIdWeapon(const int weapon_id) {
    return weapons_table[weapon_id].iIdIndex;
}

// The bot will be buying this weapon
void BotPrepareConsoleCommandsToBuyWeapon(cBot* pBot, const char* arg1, const char* arg2) {
    // To be sure the console will only change when we MAY change.
    // The values will only be changed when console_nr is 0
    if (Game.getRoundStartedTime() + 4 < gpGlobals->time)
        return;                   // Not valid to buy

    if (pBot->console_nr == 0) {
        // set up first command and argument
        snprintf(pBot->arg1, sizeof(pBot->arg1), "buy");
        snprintf(pBot->arg2, sizeof(pBot->arg2), "%s", arg1);

        // add argument
        if (arg2 != nullptr) {
            snprintf(pBot->arg3, sizeof(pBot->arg3), "%s", arg2);
        }
        else {
            pBot->arg3[0] = '\0';
        }

        pBot->console_nr = 1;     // start console command sequence
    }
}

/**
 * Determines if the weapon that will be bought, is valid to be bought
 * by specific team, cs version, etc. Returns TRUE if valid.
 * @param weapon
 * @param team
 * @return
 */
bool GoodWeaponForTeam(const int weapon, const int team) {
    // Mod CS:
    if (mod_id == CSTRIKE_DLL) {
        // When not playing Counter-Strike 1.6, these weapons are automatically wrong for all teams.
        if (counterstrike == 0) {
            if (weapon == CS_WEAPON_GALIL || weapon == CS_WEAPON_FAMAS || weapon == CS_WEAPON_SHIELD) {
                return false;
            }
        }

        // Define team-specific restricted weapons
        static const std::unordered_set<int> ct_restricted = {
            CS_WEAPON_SG552, CS_WEAPON_AK47, CS_WEAPON_DEAGLE, CS_WEAPON_MP5NAVY,
            CS_WEAPON_GALIL, CS_WEAPON_P90, CS_WEAPON_G3SG1
        };

        static const std::unordered_set<int> t_restricted = {
            CS_WEAPON_AUG, CS_WEAPON_DEAGLE, CS_WEAPON_M4A1, CS_WEAPON_MP5NAVY,
            CS_WEAPON_FAMAS, CS_WEAPON_P90, CS_WEAPON_SG550, CS_DEFUSEKIT
        };

        if (team == 2) { // Counter-Terrorist
            if (ct_restricted.count(weapon)) {
                return false;
            }
        }
        else { // Terrorist
            if (t_restricted.count(weapon)) {
                return false;
            }
        }
    }
    // yes bot, you may buy this weapon.
    return true;
}

/*
 BotDecideWhatToBuy()

 In this function the bot will choose what weapon to buy from the table.
 */
int BotBuyPrimaryWeapon(cBot *pBot) {
    const int money = pBot->bot_money;
    const int team = pBot->iTeam;
    int buy_weapon = -1;

    // Personality related:
    // Check if we can buy our favorite weapon
    if (pBot->hasFavoritePrimaryWeaponPreference()) {
        if (!pBot->ownsFavoritePrimaryWeapon()) {
            if (GoodWeaponForTeam(pBot->ipFavoPriWeapon, pBot->iTeam)) { // can we buy it for this team?
                if (pBot->canAfford(PriceWeapon(pBot->ipFavoPriWeapon))) { // can we afford it?
                    return pBot->ipFavoPriWeapon;
                }
                // bot personality: if we want to save money for our favorite weapon, then set other values to false
                if (RANDOM_LONG(0, 100) < pBot->ipSaveForWeapon) {
	                pBot->rprint("Decided to save extra money");
	                pBot->buy_secondary = false; // don't buy a secondary
	                return -2; // Special value to indicate "stop buying"
                }
            }
        }
        else {
            // already have my favorite weapon
            return -2; // Stop buying
        }
    }

    // Find weapon we can buy in the list of weapons
    for (const weapon_price_table& i : weapons_table) {
        if (UTIL_GiveWeaponType(i.iId) != PRIMARY && UTIL_GiveWeaponType(i.iId) != SHIELD)
            continue;

        if (!GoodWeaponForTeam(i.iId, team))
            continue;

        if (i.price <= money) {
            if (pBot->iPrimaryWeapon > -1) {
                if (weapons_table[ListIdWeapon(pBot->iPrimaryWeapon)].priority >= i.priority)
                    continue;
            }

            if (buy_weapon == -1) {
                buy_weapon = i.iId;
            }
            else {
                if (RANDOM_LONG(0, 100) < i.priority) {
                    buy_weapon = i.iId;
                }
            }
        }
    }

    return buy_weapon;
}

int BotBuySecondaryWeapon(cBot *pBot) {
    const int money = pBot->bot_money;
    const int team = pBot->iTeam;
    int buy_weapon = -1;

    if (pBot->hasFavoriteSecondaryWeaponPreference()) {
        if (!pBot->ownsFavoriteSecondaryWeapon()) {
            if (GoodWeaponForTeam(pBot->ipFavoSecWeapon, pBot->iTeam)) {
                if (pBot->canAfford(pBot->ipFavoSecWeapon)) {
                    return pBot->ipFavoSecWeapon;
                }
                if (RANDOM_LONG(0, 100) < pBot->ipSaveForWeapon) {
	                return -2; // Stop buying
                }
            }
        }
        else {
            return -2; // Stop buying
        }
    }

    for (const weapon_price_table& i : weapons_table) {
        if (UTIL_GiveWeaponType(i.iId) != SECONDARY)
            continue;

        if (!GoodWeaponForTeam(i.iId, team))
            continue;

        if (i.price <= money) {
            if (pBot->iSecondaryWeapon > -1) {
                const int index = weapons_table[pBot->iSecondaryWeapon].iIdIndex;
                if (weapons_table[index].priority >= i.priority)
                    continue;
            }

            if (buy_weapon == -1) {
                buy_weapon = i.iId;
            }
            else {
                if (RANDOM_LONG(0, 100) < i.priority) {
                    buy_weapon = i.iId;
                }
            }
            if (RANDOM_LONG(0, 100) < i.priority)
                break;
        }
    }
    return buy_weapon;
}

int BotBuyEquipment(cBot *pBot) {
    const int money = pBot->bot_money;

    if (pBot->buy_defusekit) {
        pBot->buy_defusekit = false;
        if (money >= 200) return CS_DEFUSEKIT;
    }

    if (pBot->buy_armor) {
        pBot->buy_armor = false;
        if (money >= 1000) return CS_WEAPON_ARMOR_HEAVY;
        if (money >= 650) return CS_WEAPON_ARMOR_LIGHT;
    }

    if (pBot->buy_grenade) {
        pBot->buy_grenade = false;
        if (money >= weapons_table[ListIdWeapon(CS_WEAPON_HEGRENADE)].price) return CS_WEAPON_HEGRENADE;
    }

    if (pBot->buy_flashbang > 0) {
        if (money >= weapons_table[ListIdWeapon(CS_WEAPON_FLASHBANG)].price) {
            pBot->buy_flashbang--;
            return CS_WEAPON_FLASHBANG;
        }
        pBot->buy_flashbang = 0;
    }

    if (pBot->buy_smokegrenade) {
        pBot->buy_smokegrenade = false;
        if (money >= weapons_table[ListIdWeapon(CS_WEAPON_SMOKEGRENADE)].price) return CS_WEAPON_SMOKEGRENADE;
    }

    return -1;
}


/*
 BotDecideWhatToBuy()

 In this function the bot will choose what weapon to buy from the table.
 */
void BotDecideWhatToBuy(cBot *pBot) {
    const int money = pBot->bot_money;
    int buy_weapon;

    if (pBot->buy_primary) {
        buy_weapon = BotBuyPrimaryWeapon(pBot);
        pBot->buy_primary = false;

        if (buy_weapon == -2) return; // Stop buying

        if (buy_weapon != -1) {
            const int iMoneyLeft = money - PriceWeapon(buy_weapon);
            if (iMoneyLeft >= 600 && (RANDOM_LONG(0, 100) < 15 || pBot->iPrimaryWeapon == CS_WEAPON_SHIELD)) {
                pBot->buy_secondary = true;
            }
        }
    }
    else if (pBot->buy_secondary) {
        buy_weapon = BotBuySecondaryWeapon(pBot);
        pBot->buy_secondary = false;
        if (buy_weapon == -2) return;
    }
    else if (pBot->buy_ammo_primary) {
        BotPrepareConsoleCommandsToBuyWeapon(pBot, "6", nullptr);
        pBot->buy_ammo_primary = false;
        return;
    }
    else if (pBot->buy_ammo_secondary) {
        BotPrepareConsoleCommandsToBuyWeapon(pBot, "7", nullptr);
        pBot->buy_ammo_secondary = false;
        return;
    }
    else {
        buy_weapon = BotBuyEquipment(pBot);
    }

    // Perform the actual buy commands to acquire weapon
    if (buy_weapon != -1) {
        pBot->performBuyActions(buy_weapon);
    }
}

/*
 ConsoleThink()
 
 This function is important to bots. The bot will actually execute any console command
 given right here. This also activates the buy behaviour at the start of a round.
*/
void ConsoleThink(cBot *pBot) {
    // RealBot only supports Counter-Strike
    if (mod_id != CSTRIKE_DLL) return;
    if (pBot->isUsingConsole()) return; // busy executing console commands, so do not decide anything else

    // buy time is in minutes, we need
    // gpGlobals->time is in seconds, so we need to translate the minutes into seconds
    const float buyTime = CVAR_GET_FLOAT("mp_buytime") * 60;
    if (Game.getRoundStartedTime() + buyTime > gpGlobals->time &&
        pBot->wantsToBuyStuff()) {
        BotDecideWhatToBuy(pBot);
    }
    // Buying code in CS
}

////////////////////////////////////////////////////////////////////////////////
/// Console Handling by Bots
////////////////////////////////////////////////////////////////////////////////
void BotConsole(cBot *pBot) {
    // Nothing to execute and alive, think about any console action to be taken.
    if (pBot->console_nr == 0 && pBot->pEdict->v.health > 0) {
        if (pBot->f_console_timer <= gpGlobals->time) {
            pBot->arg1[0] = '\0';
            pBot->arg2[0] = '\0';
            pBot->arg3[0] = '\0';
        }
        ConsoleThink(pBot);       // Here it will use the console, think about what to do with it
        return; // Early exit - no need to check console_nr again [APG]RoboCop[CL]
    }

    // Here the bot will excecute the console commands if the console counter has been set/changed
    if (pBot->console_nr != 0 && pBot->f_console_timer < gpGlobals->time) {
        // safety net
        pBot->console_nr = std::max(pBot->console_nr, 0); // Set it to 0

        // issue command (buy/radio)
        if (pBot->console_nr == 1)
            FakeClientCommand(pBot->pEdict, pBot->arg1, nullptr, nullptr);

        // do menuselect
        if (pBot->console_nr == 2)
            FakeClientCommand(pBot->pEdict, "menuselect", pBot->arg2, nullptr);

        // do menuselect
        if (pBot->console_nr == 3) {
            // When the last parameter is not null, we will perform that action.
            if (pBot->arg3[0] != 0)
                FakeClientCommand(pBot->pEdict, "menuselect", pBot->arg3,
                nullptr);

            // reset
            pBot->console_nr = -1;
            pBot->f_console_timer = gpGlobals->time + RANDOM_FLOAT(0.2f, 0.5f);
        }

        if (pBot->console_nr > 0)
            pBot->console_nr++;    // Increase command

        //return;
    }

}                               // BotConsole()
