#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#include "structs.h"

void spawnPlayer(GameState * gameState, int i);

void updateMap(GameState * gameState, int index);

void setMap(GameState * gameState, unsigned int seed);

bool processMove(GameState * gameState, int currentPlayer, unsigned char move);

bool checkCantMove(GameState * gameState, int currentPlayer);

#endif