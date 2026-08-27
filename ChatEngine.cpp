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

// Chatting Engine
#include <cstring>
#include <cctype>
// Some tests by EVYNCKE
#include <string>
#include <string_view>
#include <algorithm>
#include <vector>
#include <sstream>


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

extern edict_t* pHostEdict;
extern cGame Game;
extern cBot bots[32];

namespace {
    edict_t* findPlayerEdictByName(const char* playerName) {
        if (!playerName || *playerName == '\0') {
            return nullptr;
        }

        for (int i = 1; i <= gpGlobals->maxClients; i++) {
            edict_t* pPlayer = INDEXENT(i);
            if (pPlayer && !pPlayer->free) {
                if (std::strcmp(STRING(pPlayer->v.netname), playerName) == 0) {
                    return pPlayer;
                }
            }
        }
        return nullptr;
    }
}

// initialize all
void cChatEngine::init() {
    // clear all blocks
    for (tReplyBlock& iB : ReplyBlock)
    {
        for (char(&iBs)[128] : iB.sentence)
            iBs[0] = '\0';

        for (char(&iBw)[25] : iB.word)
            iBw[0] = '\0';

        iB.bUsed = false;
    }

    iLastBlock = -1;
    iLastSentence = -1;

    // init sentence
    std::memset(sentence, 0, sizeof(sentence));
    std::memset(sender, 0, sizeof(sender));
}

// load
void cChatEngine::initAndload() {
    init();

    // load blocks using INI parser
    INI_PARSE_CHATFILE();
}

// think
void cChatEngine::think() {
    if (fThinkTimer + 1.0f > gpGlobals->time) return; // not time yet to think

    // decrease over time to avoid flooding
    if (Game.iProducedSentences > 1) {
        Game.iProducedSentences--;
    }

    // if no sender is set, do nothing
    if (sender[0] == '\0') return;

    edict_t* pSender = findPlayerEdictByName(sender);

    // Scan the message so we know in what block we should be to reply:
    std::string_view sentence_sv(sentence);

    if (sentence_sv.empty() || sentence_sv.length() >= MAX_SENTENCE_LENGTH - 1) {
        // clear out sentence and sender
        std::memset(sentence, 0, sizeof(sentence));
        std::memset(sender, 0, sizeof(sender));

        // reset timer
        fThinkTimer = gpGlobals->time;

        return;
    }

    std::vector WordBlockScore(MAX_BLOCKS, 0);

    std::stringstream ss(sentence);
    std::string word_str;
    while (ss >> word_str) {
        for (int iB = 0; iB < MAX_BLOCKS; iB++) {
            if (ReplyBlock[iB].bUsed) {
                for (const char (&iBw)[25] : ReplyBlock[iB].word) {
                    if (iBw[0] != '\0' && word_str == iBw) {
                        WordBlockScore[iB]++;
                    }
                }
            }
        }
    }

    // now loop through all blocks and find the one with the most score:
    int iMaxScore = 0;
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
        for (const char(&iS)[128] : ReplyBlock[iTheBlock].sentence)
        {
            // Find max sentences of this reply block
            if (iS[0] != '\0')
                iMax++;
        }

        // loop through all bots:
        for (int i = 1; i <= gpGlobals->maxClients; i++) {
            edict_t* pPlayer = INDEXENT(i);

            // skip invalid players and skip self (i.e. this bot)
            if (pPlayer && !pPlayer->free && pSender != pPlayer)
            {
                if (!IsAlive(pSender) || !IsAlive(pPlayer))
                    continue;

                cBot* pBotPointer = UTIL_GetBotPointer(pPlayer);

                if (pBotPointer != nullptr)
                    if (constexpr int CHAT_RATE_THRESHOLD = 25; RANDOM_LONG(0, 100)
                        < pBotPointer->ipChatRate + CHAT_RATE_THRESHOLD) {
                        // When we have at least 1 sentence...
                        if (iMax > -1) {
                            // choose randomly a reply
                            int the_c = RANDOM_LONG(0, iMax);

                            if (iTheBlock == iLastBlock &&
                                the_c == iLastSentence) {
                                // when this is the same, avoid it. Try to change again
                                if (iMax > 0)
                                    the_c = (the_c + 1) % (iMax + 1);
                                else
                                    continue;      // do not reply double
                            }
                            // the_c is choosen, it is the sentence we reply with.
                            // do a check if its valid:
                            if (ReplyBlock[iTheBlock].
                                sentence[the_c][0] != '\0') {

                                // chSentence is eventually what the bot will say.
                                char chSentence[128];
                                std::string_view reply_template(ReplyBlock[iTheBlock].sentence[the_c]);
                                size_t name_pos = reply_template.find("%n");

                                if (name_pos != std::string_view::npos) {
                                    std::string final_sentence = std::string(reply_template.substr(0, name_pos));
                                    final_sentence += sender;
                                    final_sentence += reply_template.substr(name_pos + 2);
                                    snprintf(chSentence, sizeof(chSentence), "%s \n", final_sentence.c_str());
                                }
                                // when no name pos is found, we just copy the string and say that (works ok)
                                else {
                                    snprintf(chSentence, sizeof(chSentence), "%s \n",
                                        ReplyBlock[iTheBlock].sentence[the_c]);
                                }

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
    std::memset(sentence, 0, sizeof(sentence));
    std::memset(sender, 0, sizeof(sender));


    fThinkTimer = gpGlobals->time + RANDOM_FLOAT(0.0f, 0.5f);
}

void cChatEngine::handle_sentence() {
    // Define a constant for the chat rate threshold
    constexpr int CHAT_RATE_THRESHOLD = 25;

    // Check if there is a sentence to process
    if (sentence[0] == '\0') {
        return;
    }

    // Loop through all clients
    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t* pPlayer = INDEXENT(i);

        // Skip invalid players
        if (pPlayer && !pPlayer->free) {
            cBot* pBotPointer = UTIL_GetBotPointer(pPlayer);

            // Check if bot pointer is valid and decide to prepare chat
            if (pBotPointer != nullptr && RANDOM_LONG(0, 100) < pBotPointer->ipChatRate + CHAT_RATE_THRESHOLD) {
                pBotPointer->PrepareChat(sentence);
            }
        }
    }

    // Clear sentence and sender buffers
    std::memset(sentence, 0, sizeof(sentence));
    std::memset(sender, 0, sizeof(sender));

    // Set the think timer
    fThinkTimer = gpGlobals->time + RANDOM_FLOAT(0.0f, 0.5f);
}


void cChatEngine::set_sentence(char csender[MAX_NAME_LENGTH], char csentence[MAX_SENTENCE_LENGTH]) {  
    if (sender[0] == ' ' || sender[0] == '\0') {  
        strncpy(sender, csender, sizeof(sender) - 1);  
        sender[sizeof(sender) - 1] = '\0';  

#ifdef _WIN32  
        _strupr(csentence);  
#else  
        std::transform(csentence, csentence + std::strlen(csentence), csentence, ::toupper);  
#endif  
        strncpy(sentence, csentence, sizeof(sentence) - 1);  
        sentence[sizeof(sentence) - 1] = '\0';  
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
