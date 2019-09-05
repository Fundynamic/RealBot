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
int PriceWeapon(int weapon_id) {
    return weapons_table[weapons_table[weapon_id].iIdIndex].price;
}

// Returns the ID in the list, checking on WEAPON_ID (of CS)
int ListIdWeapon(int weapon_id) {
    return weapons_table[weapon_id].iIdIndex;
}

// The bot will be buying this weapon
void BuyWeapon(cBot *pBot, const char *arg1, const char *arg2) {
    // To be sure the console will only change when we MAY change.
    // The values will only be changed when console_nr is 0
    if (Game.RoundTime() + 4 < gpGlobals->time)
        return;                   // Not valid to buy

    if (pBot->console_nr == 0) {
        // set up first command and argument
        strcpy(pBot->arg1, "buy");
        strcpy(pBot->arg2, arg1);

        // add argument
        if (arg2 != NULL)
            strcpy(pBot->arg3, arg2);

        pBot->console_nr = 1;     // start console command sequence
    }
}

// Determines if the weapon that will be bought, is valid to be bought
// by specific team, cs version, etc. Returns TRUE if valid.
bool GoodWeaponForTeam(int weapon, int team) {
    // Mod CS:
    if (mod_id == CSTRIKE_DLL) {

        // When not playing Counter-Strike 1.6, these weapons are automaticly wrong for all teams.
        if (counterstrike == 0)
            if (weapon == CS_WEAPON_GALIL || weapon == CS_WEAPON_FAMAS
                || weapon == CS_WEAPON_SHIELD)
                return false;

        // Check the real thing
        if (team == 2)            // Counter-Terrorist
        {
            // 30/07/04 by Josh
            // Use a switch instead of multiple if
            switch (weapon) {
                case CS_WEAPON_SG552:
                    return false;
                    break;
                case CS_WEAPON_AK47:
                    return false;
                    break;
                case CS_WEAPON_ELITE:
                    return false;
                    break;
                case CS_WEAPON_MAC10:
                    return false;
                    break;
                case CS_WEAPON_GALIL:
                    return false;
                    break;
                    // 30.8.04 added by frashman
                case CS_WEAPON_G3SG1:
                    return false;
                    break;
            }
        } else {
            switch (weapon) {
                case CS_WEAPON_AUG:
                    return false;
                    break;
                case CS_WEAPON_FIVESEVEN:
                    return false;
                    break;
                case CS_WEAPON_M4A1:
                    return false;
                    break;
                case CS_WEAPON_TMP:
                    return false;
                    break;
                case CS_WEAPON_FAMAS:
                    return false;
                    break;
                case CS_WEAPON_SHIELD:
                    return false;
                    break;
                    //30.8.04 added by Frashman
                case CS_WEAPON_SG550:
                    return false;
                    break;
                case CS_DEFUSEKIT:
                    return false;
                    break;
            }
        }
    }
    // yes bot, you may buy this weapon.
    return true;
}

/*
 BotBuyStuff()
 
 In this function the bot will choose what weapon to buy from the table.
 */
void BotBuyStuff(cBot *pBot) {
    /**
     * Stefan 02/09/2018
     *
     * This function is called multiple frames. And it basically checks boolean flags (buy_*) method, if true
     * then a weapon to buy is choosen. Once choosen it is bought. The flag which was 'true' now became 'false'.
     *
     * Then the next frame, the boolean flag before will become 'false' and thus another if block is executed.
     *
     * Effectively doing:
     * - buy primary weapon first
     * - then secondary
     * - ammo for primary
     * - ammo for secondary
     * - armor
     * - defuse kit
     * - etc.
     *
     * the order of if statements is leading
     *
     * In other words, it needs refactoring...
     *
     */
    int money = pBot->bot_money; // Money
    int team = pBot->iTeam;      // Team

    int buy_weapon = -1;

    pBot->rprint("BotBuyStuff()");
    // Buy a primary weapon, think of the best choice
    if (pBot->buy_primary) {
        pBot->rprint("BotBuyStuff()", "buy_primary");
        // Buy primary

        // Personality related:
        // Check if we can buy our favorite weapon
        if (!pBot->ownsFavoritePrimaryWeapon()) {
            if (GoodWeaponForTeam(pBot->ipFavoPriWeapon, pBot->iTeam)) { // can we buy it for this team?
                if (pBot->canAfford(PriceWeapon(pBot->ipFavoPriWeapon))) { // can we afford it?
                    // Buy favorite weapon!
                    buy_weapon = pBot->ipFavoPriWeapon;
                } else {
                    // bot personality: if we want to save money for our favorite weapon, then set other values to false
                    if (RANDOM_LONG(0, 100) < pBot->ipSaveForWeapon) {    // 31.08.04 Frashman forgotten brace
                        pBot->buy_primary = false;
                        pBot->buy_secondary = false;       // don't buy a secondary
                        return;    // get out of function, don't buy anything
                    }
                }
            }
        } else {
            // already have my favorite weapon
            pBot->buy_primary = false; // do not buy a primary weapon
            return;
        }

        // not decided what to buy yet
        if (buy_weapon < 0) {

            // Find weapon we can buy in the list of weapons
            for (int i = 0; i < MAX_WEAPONS; i++) {

                // 31.08.04 Frashman Filter Out all except PRIMARY and SHIELD
                // SHIELD is used as primary weapon

                if ((UTIL_GiveWeaponType(weapons_table[i].iId) != PRIMARY)
                    && (UTIL_GiveWeaponType(weapons_table[i].iId) != SHIELD))
                    continue;

                if (GoodWeaponForTeam(weapons_table[i].iId, team) == false)
                    continue;

                if (weapons_table[i].price <= money) {
                    if (pBot->iPrimaryWeapon > -1)   // has a primary weapon
                        if (weapons_table[ListIdWeapon(pBot->iPrimaryWeapon)].
                                priority >= weapons_table[i].priority)
                            continue;

                    if (buy_weapon == -1)
                        buy_weapon = weapons_table[i].iId;

                    else {
                        if (RANDOM_LONG(0, 100) < weapons_table[i].priority)
                            buy_weapon = weapons_table[i].iId; // randomly buy a different weapon
                    }
                    // 30.8.04 Frashman fixed from 150 to 100
                    if (RANDOM_LONG(0, 100) < weapons_table[i].priority)
                        break;
                }
            }
        }

        if (buy_weapon != -1) {
            pBot->buy_primary = false;

            // depending on amount of money we have left buy *also* secondary weapon
            int iMoneyLeft = money - PriceWeapon(buy_weapon);

            // TODO: this should be dependant on something else... not only money
            // 01.09.04 Frashman if buyed a Shield, try to buy a good Pistol
            if (iMoneyLeft >= 600)
                if ((RANDOM_LONG(0, 100) < 15)
                    || (pBot->iPrimaryWeapon == CS_WEAPON_SHIELD))
                    pBot->buy_secondary = true;
        }

    } else if (pBot->buy_secondary) {
        pBot->rprint("BotBuyStuff()", "buy_secondary");
        // Buy secondary

        // Personality related:
        // Check if we can buy our favorite weapon
        if (!pBot->ownsFavoriteSecondaryWeapon()) {
            if (GoodWeaponForTeam(pBot->ipFavoSecWeapon, pBot->iTeam)) {
                if (pBot->canAfford(pBot->ipFavoSecWeapon)) {
                    // Buy favorite weapon
                    buy_weapon = pBot->ipFavoPriWeapon;
                } else {
                    // We do here something to 'save' for our favorite weapon
                    if (RANDOM_LONG(0, 100) < pBot->ipSaveForWeapon) {    // 31.08.04 Frashman forgotten brace
                        pBot->buy_secondary = false;       // don't buy a secondary
                        return;    // get out of function, don't buy anything
                    }
                }
            }
        } else {
            return;             // get out of function, go buy a secondary weapon or something?
        }

        // no weapon choosen to buy yet
        if (buy_weapon < 0) {
            // Buy secondary
            // Find weapon we can buy in the list of weapons
            for (int i = 0; i < MAX_WEAPONS; i++) {

                // When enough money and the priority is high enough..
                // Filter out Secondary and Grenades
                if (UTIL_GiveWeaponType(weapons_table[i].iId) != SECONDARY)
                    continue;

                if (GoodWeaponForTeam(weapons_table[i].iId, team) == false)
                    continue;

                if (weapons_table[i].price <= money) {
                    if (pBot->iSecondaryWeapon > -1) {
                        int index =
                                weapons_table[pBot->iSecondaryWeapon].iIdIndex;
                        // 31.08.04 Frashman > corrected to >= ,
                        // else the bot will buy another weapon with the same priority
                        if (weapons_table[index].priority >=
                            weapons_table[i].priority)
                            continue;
                    }

                    if (buy_weapon == -1)
                        buy_weapon = weapons_table[i].iId;
                    else {
                        if (RANDOM_LONG(0, 100) < weapons_table[i].priority)
                            buy_weapon = weapons_table[i].iId;
                    }
                    if (RANDOM_LONG(0, 100) < weapons_table[i].priority)
                        break;
                }
            }
        }

        if (buy_weapon != -1) {
            pBot->buy_secondary = false;
        }
    } else if (pBot->buy_ammo_primary == true) {
        pBot->rprint("BotBuyStuff()", "buy_ammo_primary");
        // Buy primary ammo
        BuyWeapon(pBot, "6", NULL);
        pBot->buy_ammo_primary = false;
        return;

    } else if (pBot->buy_ammo_secondary == true) {
        pBot->rprint("BotBuyStuff()", "buy_ammo_secondary");
        // Buy secondary ammo
        BuyWeapon(pBot, "7", NULL);
        pBot->buy_ammo_secondary = false;
        return;
    } else if (pBot->buy_defusekit) {
        pBot->rprint("BotBuyStuff()", "buy_defusekit");
        if (money >= 200) {
            buy_weapon = CS_DEFUSEKIT;
            pBot->buy_defusekit = false;
        }
    } else if (pBot->buy_armor) {
        pBot->rprint("BotBuyStuff()", "buy_armor");
        if (money < 1000 && money >= 650) {
            // Buy light armor
            buy_weapon = CS_WEAPON_ARMOR_LIGHT;
        } else if (money >= 1000) {
            // Buy heavy armor
            buy_weapon = CS_WEAPON_ARMOR_HEAVY;
        }
        pBot->buy_armor = false;
    } else if (pBot->buy_grenade) {
        pBot->rprint("BotBuyStuff()", "buy_grenade");
        // Buy grenade
        if (money >= weapons_table[ListIdWeapon(CS_WEAPON_HEGRENADE)].price)
            buy_weapon = CS_WEAPON_HEGRENADE;

        pBot->buy_grenade = false;
    } else if (pBot->buy_flashbang > 0) {
        pBot->rprint("BotBuyStuff()", "buy_flashbang");
        // Buy flashbang
        if (money >= weapons_table[ListIdWeapon(CS_WEAPON_FLASHBANG)].price) {

            buy_weapon = CS_WEAPON_FLASHBANG;
            pBot->buy_flashbang--;
        } else
            pBot->buy_flashbang = 0;       // do not buy

    } else if (pBot->buy_smokegrenade)   //31.08.04 Frashman added Smoke Grenade support
    {
        pBot->rprint("BotBuyStuff()", "buy_smokegrenade");
        // Buy SmokeGrenade
        if (money >=
            weapons_table[ListIdWeapon(CS_WEAPON_SMOKEGRENADE)].price)
            buy_weapon = CS_WEAPON_SMOKEGRENADE;

        pBot->buy_smokegrenade = false;
    }

    // Perform the actual buy commands to acquire weapon
    pBot->performBuyActions(buy_weapon);
}

/*
 ConsoleThink()
 
 This function is important to bots. The bot will actually execute any console command
 given right here. This also activates the buy behaviour at the start of a round.
*/
void ConsoleThink(cBot *pBot) {
    // RealBot only supports Counter-Strike
    if (mod_id != CSTRIKE_DLL) return;

    // Bot thinks what it should do with the console
    bool time_to_buy = false;
    bool need_to_buy = false; // We dont need anything!

    // TODO: take buy time cvar?
    if (Game.RoundTime() + 15 > gpGlobals->time && pBot->console_nr == 0) {
        pBot->rprint("ConsoleThink()", "time to buy = true");
        time_to_buy = true;
    }

    if (time_to_buy == true) {
        // Do only 'check for buying' stuff if its time, else this is wasted time and cpu load :)
        need_to_buy = pBot->buy_secondary == true ||
                pBot->buy_primary == true ||
                pBot->buy_ammo_primary == true ||
                pBot->buy_ammo_secondary == true ||
                pBot->buy_armor == true ||
                pBot->buy_defusekit == true ||
                pBot->buy_grenade == true ||
                pBot->buy_flashbang > 0;

        // * so you are asking yourself why i do == true all the time huh?
        // * i just love the blue color in my MSVC :)
    }

    // Ok, if its time to buy, check if its NEEDED to buy...
    if (time_to_buy && need_to_buy && pBot->console_nr == 0) {
        BotBuyStuff(pBot);
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
            pBot->arg1[0] = 0;     // clear
            pBot->arg2[0] = 0;     // the
            pBot->arg3[0] = 0;     // variables
        }
        ConsoleThink(pBot);       // Here it will use the console, think about what to do with it
    }

    // Here the bot will excecute the console commands if the console counter has been set/changed
    if (pBot->console_nr != 0 && pBot->f_console_timer < gpGlobals->time) {
        // safety net
        if (pBot->console_nr < 0)
            pBot->console_nr = 0;  // Set it to 0

        // issue command (buy/radio)
        if (pBot->console_nr == 1)
            FakeClientCommand(pBot->pEdict, pBot->arg1, NULL, NULL);

        // do menuselect
        if (pBot->console_nr == 2)
            FakeClientCommand(pBot->pEdict, "menuselect", pBot->arg2, NULL);

        // do menuselect
        if (pBot->console_nr == 3) {
            // When the last parameter is not null, we will perform that action.
            if (pBot->arg3[0] != 0)
                FakeClientCommand(pBot->pEdict, "menuselect", pBot->arg3,
                                  NULL);

            // reset
            pBot->console_nr = -1;
            pBot->f_console_timer = gpGlobals->time + RANDOM_FLOAT(0.2, 0.5);
        }

        if (pBot->console_nr > 0)
            pBot->console_nr++;    // Increase command

        return;
    }

}                               // BotConsole()
