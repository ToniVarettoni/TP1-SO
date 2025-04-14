#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "structs.h"
#include <unistd.h>

#define MAX_LENGTH_NUM 10
#define MAX_LENGTH_PATH 100

void initView(GameState * gameState, char * view, int * viewPid);

int initPlayer(GameState * gameState, int i);


#endif