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

// Chatting Engine
#include <string.h>
#include <ctype.h>
// Some tests by EVYNCKE
#include <string>
#include <algorithm>
#include <cctype>

#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>
#include <entity_state.h>

#include "bot.h"
#include "IniParser.h"
#include "bot_weapons.h"
#include "game.h"
#include "bot_func.h"

#include "ChatEngine.h"

extern edict_t *pHostEdict;
extern cGame Game;
extern cBot bots[32];

// initialize all
void
cChatEngine::init() {
   // clear all blocks
   for (int iB = 0; iB < MAX_BLOCKS; iB++) {
      for (int iBs = 0; iBs < 50; iBs++)
         ReplyBlock[iB].sentence[iBs][0] = '\0';

      for (int iBw = 0; iBw < 10; iBw++)
         ReplyBlock[iB].word[iBw][0] = '\0';

      ReplyBlock[iB].bUsed = false;
   }

   iLastBlock = -1;
   iLastSentence = -1;

   // init sentence
   memset(sentence, 0, sizeof(sentence));
   memset(sender, 0, sizeof(sender));
}

// load
void cChatEngine::load() {
   init();

   // load blocks using INI parser
   INI_PARSE_CHATFILE();
}


// think
void cChatEngine::think() {
   if (fThinkTimer + 1.0 < gpGlobals->time) {
      // decrease
      if (Game.iProducedSentences > 1) {
         Game.iProducedSentences--;
      }

      if (sender[0] != '\0') {

         //      SERVER_PRINT("ChatEngine: handling sentence, sent by '");
         //      SERVER_PRINT(sender);
         //      SERVER_PRINT("' who said:\n");
         //      SERVER_PRINT(sentence);
         //      SERVER_PRINT(" --\n");

         // Run some checks on the 'sender'. We need the edict from that
         edict_t *pSender = NULL;
         int i;
         for (i = 1; i <= gpGlobals->maxClients; i++) {
            edict_t *pPlayer = INDEXENT(i);

            if (pPlayer && (!pPlayer->free)) {
               char name[30], name2[30];
               // clear
               memset(name, 0, sizeof(name));
               memset(name2, 0, sizeof(name2));

               // copy
               strcpy(name, STRING(pPlayer->v.netname));
               strcpy(name2, sender);

               if (strcmp(name, name2) == 0) {
                  pSender = pPlayer;
                  break;
               }
            }
         }

         // Scan the message so we know in what block we should be to reply:
         char word[20];
         memset(word, 0, sizeof(word));
         int c = 0;
         //for (c=0; c < 20; c++)
         //      word[c] = '\0';

         c = 0;
         int wc = 0;

         int length = strlen(sentence);

         // When length is not valid, get out.
         if (length == 0 || length >= 127) {

            memset(sentence, 0, sizeof(sentence));
            memset(sender, 0, sizeof(sender));

            // reset timer
            fThinkTimer = gpGlobals->time;

            return;
         }
         // Define word block scores:
         int WordBlockScore[MAX_BLOCKS];

         // Init, none of the block has a score yet (set to -1)
         for (int wbs = 0; wbs < MAX_BLOCKS; wbs++)
            WordBlockScore[wbs] = -1;

         // chSentence
         char chSentence[128];

         // clear first
         memset(chSentence, 0, sizeof(chSentence));

         // copy
         sprintf(chSentence, "%s", sentence);

         // C
         while (c < length) {

            // protection matters:
            if (c > length)
               break;

            if (c < 0)
               break;

            // End of protection matters

            // Step: Check character to identify the end of a word.
            if (sentence[c] == ' ' || sentence[c] == '\n' ||
                  sentence[c] == '.' || sentence[c] == '?' ||
                  sentence[c] == '!' || c == length) {
               // Now find the word and add up scors on the proper score blocks.

               if (c == length)
                  word[wc] = sentence[c];

               // not a good word (too small)
               if (strlen(word) <= 0) {
                  //SERVER_PRINT("This is not a good word!\n");
               } else {
                  for (int iB = 0; iB < MAX_BLOCKS; iB++) {
                     if (ReplyBlock[iB].bUsed) {
                        for (int iBw = 0; iBw < 10; iBw++) {
                           // skip any word in the reply block that is not valid
                           if (ReplyBlock[iB].word[iBw][0] == '\0')
                              continue; // not filled in

                           if (strlen(ReplyBlock[iB].word[iBw]) <= 0)
                              continue; // not long enough (a space?)

                           // 03/07/04
                           // add score to matching word (evy: ignoring case)
                           if (strcmpi(ReplyBlock[iB].word[iBw], word) == 0)
                              WordBlockScore[iB]++;
                        }       // all words in this block
                     }          // any used block
                  }             // for all blocks
               }                // good word

               // clear out entire word.
               //for (int cw=0; cw < 20; cw++)
               //      word[cw] = '\0';
               memset(word, 0, sizeof(word));

               wc = 0;          // reset WC position (start writing 'word[WC]' at 0 again)
               c++;             // next position in sentence
               continue;        // go to top again.
            }
            // when we end up here, we are still reading a 'non finishing word' character.
            // we will fill that in word[wc]. Then add up wc and c, until we find a character
            // that marks the end of a word again.

            // fill in the word:
            word[wc] = sentence[c];

            // add up.
            c++;
            wc++;
         }                      // end of loop

         // now loop through all blocks and find the one with the most score:
         int iMaxScore = -1;
         int iTheBlock = -1;

         // for all blocks
         for (int rB = 0; rB < MAX_BLOCKS; rB++) {
            // Any block that has the highest score
            if (WordBlockScore[rB] > iMaxScore) {
               iMaxScore = WordBlockScore[rB];
               iTheBlock = rB;
            }
         }

         // When we have found pSender edict AND we have a block to reply from
         // we continue here.
         if (pSender && iTheBlock > -1) {
            int iMax = -1;

            // now choose a sentence to reply with
            for (int iS = 0; iS < 50; iS++) {
               // Find max sentences of this reply block
               if (ReplyBlock[iTheBlock].sentence[iS][0] != '\0')
                  iMax++;
            }

            // loop through all bots:
            for (int i = 1; i <= gpGlobals->maxClients; i++) {
               edict_t *pPlayer = INDEXENT(i);

               // skip invalid players and skip self (i.e. this bot)
               if ((pPlayer) && (!pPlayer->free) && pSender != pPlayer) {

                  // only reply to the living when alive, and otherwise
                  bool bSenderAlive = false;
                  bool bPlayerAlive = false;

                  bSenderAlive = IsAlive(pSender);      // CRASH : it sometimes crashes here
                  bPlayerAlive = IsAlive(pPlayer);

                  if (bSenderAlive != bPlayerAlive)
                     continue;

                  cBot *pBotPointer = UTIL_GetBotPointer(pPlayer);

                  if (pBotPointer != NULL)
                     if (RANDOM_LONG(0, 100) <
                           (pBotPointer->ipChatRate + 25)) {
                        // When we have at least 1 sentence...
                        if (iMax > -1) {
                           // choose randomly a reply
                           int the_c = RANDOM_LONG(0, iMax);

                           if (iTheBlock == iLastBlock &&
                                 the_c == iLastSentence) {
                              // when this is the same, avoid it. Try to change again
                              if (iMax > 0)
                                 the_c++;
                              else
                                 continue;      // do not reply double

                              if (the_c > iMax)
                                 the_c = 0;
                           }
                           // the_c is choosen, it is the sentence we reply with.
                           // do a check if its valid:
                           if (ReplyBlock[iTheBlock].
                                 sentence[the_c][0] != '\0') {

                              // chSentence is eventually what the bot will say.
                              char chSentence[128];
                              char temp[80];

                              memset(chSentence, 0, sizeof(chSentence));
                              memset(temp, 0, sizeof(temp));

                              // get character position
                              char *name_pos =
                                 strstr(ReplyBlock[iTheBlock].
                                        sentence[the_c], "%n");

                              // when name_pos var is found, fill it in.
                              if (name_pos != NULL) {
                                 // when name is in this one:
                                 int name_offset =
                                    name_pos -
                                    ReplyBlock[iTheBlock].sentence[the_c];
                                 name_offset--;

                                 // copy every character till name_offset
                                 int nC;
                                 for (nC = 0; nC < name_offset; nC++) {
                                    //chSentence[nC] = ReplyBlock[iTheBlock].sentence[the_c][nC];
                                    temp[nC] =
                                       ReplyBlock[iTheBlock].
                                       sentence[the_c][nC];
                                 }

                                 temp[nC] = ' ';

                                 // copy senders name to chSentence
                                 strcat(temp, sender);

                                 // From here us 'tc' to keep track of chSentence and use
                                 // nC to keep reading from ReplyBlock
                                 int tc = nC;

                                 // Skip %n part in ReplyBlock
                                 nC = name_offset + 3;

                                 // we just copied a name to chSentence
                                 // set our cursor after the name now (name length + 1)
                                 tc = strlen(temp);

                                 // now finish the sentence
                                 // get entire length of ReplyBlock and go until we reach the end
                                 int length =
                                    strlen(ReplyBlock[iTheBlock].
                                           sentence[the_c]);


                                 // for every nC , read character from ReplyBlock
                                 for (; nC <= length; nC++) {
                                    // ... and copy it into chSentence
                                    temp[tc] =
                                       ReplyBlock[iTheBlock].
                                       sentence[the_c][nC];
                                    //char tmsg[80];
                                    //sprintf(tmsg,"Copying char %c , tc = %d, nC = %d\n", temp[tc], tc, nC);
                                    //SERVER_PRINT(tmsg);

                                    tc++;       // add up tc.
                                 }

                                 // terminate
                                 temp[tc] = '\n';

                                 sprintf(chSentence, "%s \n", temp);
                              }
                              // when no name pos is found, we just copy the string and say that (works ok)
                              else
                                 sprintf(chSentence, "%s \n",
                                         ReplyBlock[iTheBlock].
                                         sentence[the_c]);

                              // reply:
                              pBotPointer->PrepareChat(chSentence);

                              //UTIL_SayTextBot(chSentence, pBotPointer);

                              // update
                              iLastSentence = the_c;
                              iLastBlock = iTheBlock;
                           }
                        }
                     }
               }

            }

         }
         // clear sentence and such
         memset(sentence, 0, sizeof(sentence));
         memset(sender, 0, sizeof(sender));



      }
      fThinkTimer = gpGlobals->time + RANDOM_FLOAT(0.0, 0.5);
   }
}

//
void cChatEngine::set_sentence(char csender[30], char csentence[128]) {
   if (sender[0] == ' ' || sender[0] == '\0') {
      //      SERVER_PRINT("Sender & sentence set.\nSender=");
      //      SERVER_PRINT(csender);
      //      SERVER_PRINT("\nSentence=");
      //      SERVER_PRINT(csentence);
      //      SERVER_PRINT("--\n");

      strcpy(sender, csender);
#ifdef _WIN32

      _strupr(csentence);
      // #elseif did not compile in MSVC - stefan (26/04/04)
#else
      //for linux by ok:
      //further changed back by evyncke as these are normal string not CString
      //Hence, hardcoding the strupr inline...
      char *pString;
      pString = csentence;
      while (*pString) {
         *pString = toupper(*pString);
         pString++;
      }
      //              transform (csentence.begin(), csentence.end(), csentence.begin(), toupper);
#endif

      strcpy(sentence, csentence);
   }
}


// $Log: ChatEngine.cpp,v $
// Revision 1.11  2004/09/07 15:44:34  eric
// - bumped build nr to 3060
// - minor changes in add2 (to add nodes for Bsp2Rbn utilities)
// - if compiled with USE_EVY_ADD, then the add2() function is used when adding
//   nodes based on human players instead of add()
// - else, it now compiles mostly without warnings :-)
//
// Revision 1.10  2004/07/05 19:15:54  eric
// - bumped to build 3052
// - modified the build_nr system to allow for easier build increment
// - a build.cpp file has been added and need to be incremented on each commit
// - some more flexibility for ChatEngine: ignore case and try to cope
//   with accute letters of French and Spanish and .. languages
//
