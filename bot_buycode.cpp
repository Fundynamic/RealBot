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

//30.8.04 redefined by frashman
#define CS_DEFUSEKIT		98      // old value was 99, same as SHIELD -> Bug??

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
void BuyWeapon(cBot * pBot, char *arg1, char *arg2) {
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
void BotBuyStuff(cBot * pBot) {
   int money = pBot->bot_money; // Money
   int team = pBot->iTeam;      // Team

   int buy_weapon = -1;

   // What to buy?
   if (pBot->buy_primary == true) {
      // Buy primary
      bool bBuyOther = true;

      // Personality related:
      // Check if we can buy our favorite weapon
      if (pBot->ipFavoPriWeapon > -1) {
         if (FUNC_BotHasWeapon(pBot, pBot->ipFavoPriWeapon) == false) {
            if (GoodWeaponForTeam(pBot->ipFavoPriWeapon, pBot->iTeam))
               if (PriceWeapon(pBot->ipFavoPriWeapon) <= money) {
                  // Buy favorite weapon
                  buy_weapon = pBot->ipFavoPriWeapon;
                  bBuyOther = false;
               } else {
                  // We do here something to 'save' for our favorite weapon
                  if (RANDOM_LONG(0, 100) < pBot->ipSaveForWeapon) {    // 31.08.04 Frashman forgotten brace
                     pBot->buy_primary = false;
                     bBuyOther = false;
                     pBot->buy_secondary = false;       // don't buy a secondary
                     return;    // get out of function, don't buy anything
                  }
               }
         } else {
            pBot->buy_primary = false;  // already have our favorite weapon
            bBuyOther = false;
            return;             // get out of function, go buy a secondary weapon or something?
         }
      }
      // Normal buy code
      if (bBuyOther) {
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
      }                         // Buy a primary weapon


      if (buy_weapon != -1) {
         pBot->buy_primary = false;

         // depending on amount of money we have left buy a secondary weapon
         int iMoneyLeft = money - PriceWeapon(buy_weapon);

         // TODO: this should be dependant on something else... not only money
         // 01.09.04 Frashman if buyed a Shield, try to buy a good Pistol
         if (iMoneyLeft >= 600)
            if ((RANDOM_LONG(0, 100) < 15)
                  || (pBot->iPrimaryWeapon == CS_WEAPON_SHIELD))
               pBot->buy_secondary = true;
      }

   } else if (pBot->buy_secondary == true) {
      // Buy secondary
      bool bBuyOther = true;

      // Personality related:
      // Check if we can buy our favorite weapon
      if (pBot->ipFavoSecWeapon > -1) {
         if (FUNC_BotHasWeapon(pBot, pBot->ipFavoSecWeapon) == false) {
            if (GoodWeaponForTeam(pBot->ipFavoSecWeapon, pBot->iTeam))
               if (PriceWeapon(pBot->ipFavoSecWeapon) < money) {
                  // Buy favorite weapon
                  buy_weapon = pBot->ipFavoPriWeapon;
                  bBuyOther = false;
               } else {
                  // We do here something to 'save' for our favorite weapon
                  if (RANDOM_LONG(0, 100) < pBot->ipSaveForWeapon) {    // 31.08.04 Frashman forgotten brace
                     bBuyOther = false;
                     pBot->buy_secondary = false;       // don't buy a secondary
                     return;    // get out of function, don't buy anything
                  }
               }
         } else {
            bBuyOther = false;
            return;             // get out of function, go buy a secondary weapon or something?
         }
      }
      // Normal buy code
      if (bBuyOther) {
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

      if (buy_weapon != -1)
         pBot->buy_secondary = false;
   } else if (pBot->buy_ammo_primary == true) {
      // Buy primary ammo
      BuyWeapon(pBot, "6", NULL);
      pBot->buy_ammo_primary = false;
      return;

   } else if (pBot->buy_ammo_secondary == true) {
      // Buy secondary ammo
      BuyWeapon(pBot, "7", NULL);
      pBot->buy_ammo_secondary = false;
      return;
   } else if (pBot->buy_defusekit) {
      if (money >= 200) {
         buy_weapon = CS_DEFUSEKIT;
         pBot->buy_defusekit = false;
      }
   } else if (pBot->buy_armor) {
      if (money < 1000 && money >= 650) {
         // Buy light armor
         buy_weapon = CS_WEAPON_ARMOR_LIGHT;
      } else if (money >= 1000) {
         // Buy heavy armor
         buy_weapon = CS_WEAPON_ARMOR_HEAVY;
      }
      pBot->buy_armor = false;
   } else if (pBot->buy_grenade) {
      // Buy grenade
      if (money >= weapons_table[ListIdWeapon(CS_WEAPON_HEGRENADE)].price)
         buy_weapon = CS_WEAPON_HEGRENADE;

      pBot->buy_grenade = false;
   } else if (pBot->buy_flashbang > 0) {
      // Buy flashbang
      if (money >= weapons_table[ListIdWeapon(CS_WEAPON_FLASHBANG)].price) {

         buy_weapon = CS_WEAPON_FLASHBANG;
         pBot->buy_flashbang--;
      } else
         pBot->buy_flashbang = 0;       // do not buy

   } else if (pBot->buy_smokegrenade)   //31.08.04 Frashman added Smoke Grenade support
   {
      // Buy SmokeGrenade
      if (money >=
            weapons_table[ListIdWeapon(CS_WEAPON_SMOKEGRENADE)].price)
         buy_weapon = CS_WEAPON_SMOKEGRENADE;

      pBot->buy_smokegrenade = false;
   }

   if (buy_weapon != -1) {
      // Buy...

      // TODO
      // FRASHMAN 30.08.04 haven't changed the cs 1.5 buycode, maybe there are also errors

      // CS 1.5 only
      if (counterstrike == 0) {
         switch (buy_weapon) {
         case CS_WEAPON_AK47:
            BuyWeapon(pBot, "4", "1");
            break;
         case CS_WEAPON_DEAGLE:
            BuyWeapon(pBot, "1", "3");
            break;
         case CS_WEAPON_P228:
            BuyWeapon(pBot, "1", "4");
            break;
         case CS_WEAPON_SG552:
            BuyWeapon(pBot, "4", "2");
            break;
         case CS_WEAPON_SG550:
            BuyWeapon(pBot, "4", "8");
            break;
         case CS_WEAPON_SCOUT:
            BuyWeapon(pBot, "4", "5");
            break;
         case CS_WEAPON_AWP:
            BuyWeapon(pBot, "4", "6");
            break;
         case CS_WEAPON_MP5NAVY:
            BuyWeapon(pBot, "3", "1");
            break;
         case CS_WEAPON_UMP45:
            BuyWeapon(pBot, "3", "5");
            break;
         case CS_WEAPON_ELITE:
            BuyWeapon(pBot, "1", "5");
            break;              // T only
         case CS_WEAPON_MAC10:
            BuyWeapon(pBot, "3", "4");
            break;              // T only
         case CS_WEAPON_AUG:
            BuyWeapon(pBot, "4", "4");
            break;              // CT Only
         case CS_WEAPON_FIVESEVEN:
            BuyWeapon(pBot, "1", "6");
            break;              // CT only
         case CS_WEAPON_M4A1:
            BuyWeapon(pBot, "4", "3");
            break;              // CT Only
         case CS_WEAPON_TMP:
            BuyWeapon(pBot, "3", "2");
            break;              // CT only
         case CS_WEAPON_HEGRENADE:
            BuyWeapon(pBot, "8", "4");
            break;
         case CS_WEAPON_XM1014:
            BuyWeapon(pBot, "2", "2");
            break;
         case CS_WEAPON_SMOKEGRENADE:
            BuyWeapon(pBot, "8", "5");
            break;
         case CS_WEAPON_USP:
            BuyWeapon(pBot, "1", "1");
            break;
         case CS_WEAPON_GLOCK18:
            BuyWeapon(pBot, "1", "2");
            break;
         case CS_WEAPON_M249:
            BuyWeapon(pBot, "5", "1");
            break;
         case CS_WEAPON_M3:
            BuyWeapon(pBot, "2", "1");
            break;

         case CS_WEAPON_G3SG1:
            BuyWeapon(pBot, "4", "7");
            break;
         case CS_WEAPON_FLASHBANG:
            BuyWeapon(pBot, "8", "3");
            break;
         case CS_WEAPON_P90:
            BuyWeapon(pBot, "3", "3");
            break;

            // Armor
         case CS_WEAPON_ARMOR_LIGHT:
            BuyWeapon(pBot, "8", "1");
            break;
         case CS_WEAPON_ARMOR_HEAVY:
            BuyWeapon(pBot, "8", "2");
            break;

            // Defuse kit
         case CS_DEFUSEKIT:
            BuyWeapon(pBot, "8", "6");
            break;
         }
      }
      // CS 1.6 only
      if (counterstrike == 1) { // FRASHMAN 30/08/04: redone switch block, it was full of errors
         switch (buy_weapon) {
            //Pistols
         case CS_WEAPON_GLOCK18:
            BuyWeapon(pBot, "1", "1");
            break;
         case CS_WEAPON_USP:
            BuyWeapon(pBot, "1", "2");
            break;
         case CS_WEAPON_P228:
            BuyWeapon(pBot, "1", "3");
            break;
         case CS_WEAPON_DEAGLE:
            BuyWeapon(pBot, "1", "4");
            break;
         case CS_WEAPON_ELITE:
            BuyWeapon(pBot, "1", "5");
            break;
            //ShotGUNS
         case CS_WEAPON_M3:
            BuyWeapon(pBot, "2", "1");
            break;
         case CS_WEAPON_XM1014:
            BuyWeapon(pBot, "2", "2");
            break;
            //SMG
         case CS_WEAPON_MAC10:
            BuyWeapon(pBot, "3", "1");
            break;
         case CS_WEAPON_TMP:
            BuyWeapon(pBot, "3", "1");
            break;
         case CS_WEAPON_MP5NAVY:
            BuyWeapon(pBot, "3", "2");
            break;
         case CS_WEAPON_UMP45:
            BuyWeapon(pBot, "3", "3");
            break;
         case CS_WEAPON_P90:
            BuyWeapon(pBot, "3", "4");
            break;
            //rifles
         case CS_WEAPON_GALIL:
            BuyWeapon(pBot, "4", "1");
            break;
         case CS_WEAPON_FAMAS:
            BuyWeapon(pBot, "4", "1");
            break;
         case CS_WEAPON_AK47:
            BuyWeapon(pBot, "4", "2");
            break;
         case CS_WEAPON_M4A1:
            BuyWeapon(pBot, "4", "3");
            break;
         case CS_WEAPON_SG552:
            BuyWeapon(pBot, "4", "4");
            break;
         case CS_WEAPON_AUG:
            BuyWeapon(pBot, "4", "4");
            break;
         case CS_WEAPON_SG550:
            BuyWeapon(pBot, "4", "5");
            break;
         case CS_WEAPON_G3SG1:
            BuyWeapon(pBot, "4", "6");
            break;
            //machinegun
         case CS_WEAPON_M249:
            BuyWeapon(pBot, "5", "1");
            break;
            // equipment
         case CS_WEAPON_ARMOR_LIGHT:
            BuyWeapon(pBot, "8", "1");
            break;
         case CS_WEAPON_ARMOR_HEAVY:
            BuyWeapon(pBot, "8", "2");
            break;
         case CS_WEAPON_FLASHBANG:
            BuyWeapon(pBot, "8", "3");
            break;
         case CS_WEAPON_HEGRENADE:
            BuyWeapon(pBot, "8", "4");
            break;
         case CS_WEAPON_SMOKEGRENADE:
            BuyWeapon(pBot, "8", "5");
            break;
         case CS_WEAPON_SHIELD:
            BuyWeapon(pBot, "8", "8");
            break;

         case CS_DEFUSEKIT:
            BuyWeapon(pBot, "8", "6");
            break;
         }

         // This differs per team
         // FRASHMAN 30/08/04: all into one ifthen block
         if (pBot->iTeam == 2)  // counter
         {
            switch (buy_weapon) {
            case CS_WEAPON_SCOUT:
               BuyWeapon(pBot, "4", "2");
               break;
            case CS_WEAPON_AWP:
               BuyWeapon(pBot, "4", "6");
               break;
               //whats about nightvision? BuyWeapon (pBot, "8", "7")
            }
         } else                 // terror
         {
            switch (buy_weapon) {
            case CS_WEAPON_SCOUT:
               BuyWeapon(pBot, "4", "3");
               break;
            case CS_WEAPON_AWP:
               BuyWeapon(pBot, "4", "5");
               break;
               //whats about nightvision? BuyWeapon (pBot, "8", "6")
            }
         }
      }                         // end of cs 1.6 part
   }                            // We actually gonna buy this weapon

}

/*
 ConsoleThink()
 
 This function is important to bots. The bot will actually execute any console command
 given right here. This also activates the buy behaviour at the start of a round.
 
 */

void ConsoleThink(cBot * pBot) {
   if (mod_id == CSTRIKE_DLL) {
      // Bot thinks what it should do with the console
      bool time_to_buy = false;
      bool need_to_buy = false; // We dont need anything!

      if (Game.RoundTime() + 15 > gpGlobals->time && pBot->console_nr == 0)
         time_to_buy = true;

      if (time_to_buy == true) {
         // Do only 'check for buying' stuff if its time, else this is wasted time and cpu load :)
         if (pBot->buy_secondary == true ||
               pBot->buy_primary == true ||
               pBot->buy_ammo_primary == true ||
               pBot->buy_ammo_secondary == true ||
               pBot->buy_armor == true ||
               pBot->buy_defusekit == true ||
               pBot->buy_grenade == true || pBot->buy_flashbang > 0)
            need_to_buy = true;

         // * so you are asking yourself why i do == true all the time huh?
         // * i just love the blue color in my MSVC :)
      }
      // Ok, if its time to buy, check if its NEEDED to buy...
      if (time_to_buy == true && need_to_buy == true
            && pBot->console_nr == 0) {
         //           char tmp[80];
         //           sprintf (tmp,"My current weapon is: %d\n",pBot->iPrimaryWeapon);
         //           rblog(tmp);
         BotBuyStuff(pBot);
      }

   }                            // Buying code in CS

}

////////////////////////////////////////////////////////////////////////////////
/// Console Handling by Bots
////////////////////////////////////////////////////////////////////////////////
void BotConsole(cBot * pBot) {
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
