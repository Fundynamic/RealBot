// globals.h

#ifndef GLOBALS_H
#define GLOBALS_H

#include <const.h>

#include "bot.h"
#include "NodeMachine.h"
#include "ChatEngine.h"
#include "game.h"

extern cGame Game;
extern cNodeMachine NodeMachine;
extern cChatEngine ChatEngine;
extern FILE* fpRblog;

extern float f_load_time;
extern float f_minplayers_think;
extern int mod_id;
extern int m_spriteTexture;
extern bool isFakeClientCommand;
extern int fake_arg_count;
extern float bot_check_time;
extern int min_bots;
extern int max_bots;
extern int min_players;
extern int num_bots;
extern int prev_num_bots;
extern bool g_GameRules;
extern edict_t* clients[32];
extern edict_t* pHostEdict;
extern float welcome_time;
extern bool welcome_sent;

extern FILE* bot_cfg_fp;
extern bool need_to_open_cfg;
extern float bot_cfg_pause_time;
extern float respawn_time;
extern bool spawn_time_reset;

extern int internet_max_interval;
extern int internet_min_interval;

extern int counterstrike;

extern bool end_round;

extern bool autoskill;
extern bool draw_nodes;
extern int draw_nodepath;
extern bool draw_connodes;
extern int kick_amount_bots;
extern int kick_bots_team;

extern bool internet_addbot;
extern float add_timer;
extern bool internet_play;

#endif // GLOBALS_H
