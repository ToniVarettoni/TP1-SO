#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include "include/structs.h"
#include "include/shared_memory.h"

unsigned char getDir(int x, int y, int i, int j);

int main(int argc, char * argv[]){

    int w = atoi(argv[1]);
    int h = atoi(argv[2]);
    int gameState_fd;
    int syncState_fd;

    GameState * gameState = (GameState *)shm_open_and_map("/game_state", sizeof(GameState) + sizeof(int) * h * w, O_RDONLY, RO, &gameState_fd);
    GameSync * syncState = (GameSync *)shm_open_and_map("/game_sync", sizeof(GameSync), O_RDWR, RW, &syncState_fd);

    pid_t myPid = getpid();
    int id = 0;

    for (size_t i = 0; i < gameState->playerAmount; i++)
    {
        if (myPid == gameState->players[i].pid)
        {
            id = i;
            break;
        }
    }
    
    int last_sum_valid_invalid = -1;
    int current_sum_valid_invalid;
    
    while (!gameState->isOver)
    {
        sem_wait(&syncState->masterSem);
        sem_post(&syncState->masterSem);

        
        do{
            sem_wait(&syncState->currReadingSem);
            if (syncState->currReading++ == 0)
            {
                sem_wait(&syncState->stateSem);
            }
            sem_post(&syncState->currReadingSem);
            
            if (gameState->players[id].cantMove)
            {
                sem_wait(&syncState->currReadingSem);
                if (syncState->currReading-- == 1)
                {
                    sem_post(&syncState->stateSem);
                }
                sem_post(&syncState->currReadingSem);
                break;
            }

            current_sum_valid_invalid = gameState->players[id].validMoves + gameState->players[id].invalidMoves;
                    
            sem_wait(&syncState->currReadingSem);
            if (syncState->currReading-- == 1)
            {
                sem_post(&syncState->stateSem);
            }
            sem_post(&syncState->currReadingSem); 
        }while (current_sum_valid_invalid == last_sum_valid_invalid);
        last_sum_valid_invalid = current_sum_valid_invalid;

        int max = 0;
        unsigned char move = 0;

        int x = gameState->players[id].x;
        int y = gameState->players[id].y;
        for (int i = x - 1; i <= x + 1; i++)
        {
            for (int j = y - 1; j <= y + 1; j++)
            {
                if (i >= 0 && j >= 0 && i < w && j < h){
                    if (max < gameState->map[i + w*j])
                    {
                        max = gameState->map[i + w*j];
                        move = getDir(x, y, i, j);
                    }
                }
            }
        }
        write(1, &move , sizeof(move)); 
    }
    shm_cleanup(gameState_fd, gameState, sizeof(GameState) + sizeof(int) * h * w);
    shm_cleanup(syncState_fd, syncState, sizeof(GameSync));
    
    return 0;
}

unsigned char getDir(int x, int y, int i, int j){

    if (i == x - 1){
        if (j == y - 1)
            return 7;
        else if(j == y)
            return 6;        
        return 5;
    }else if(i == x){
        if (j == y - 1)
             return 0;
        return 4;
    }else if (j == y - 1){
        return 1;

    }else if (j == y)
        return 2;

    return 3;
}
