#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdio.h>
#include <semaphore.h>
#include "include/structs.h"
#include <time.h>
#include <unistd.h>
#include "include/shared_memory.h"


int main(int argc, char * argv[]){

    srand(time(NULL));

    int h = atoi(argv[1]);
    int w = atoi(argv[2]);
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
    
    while (!gameState->isOver)
    {
        sem_wait(&syncState->masterSem);


        sem_wait(&syncState->currReadingSem);
        syncState->currReading++;
        sem_post(&syncState->currReadingSem);
        if (syncState->currReading == 1)
        {
            sem_wait(&syncState->stateSem);
        }
        
        
        if (gameState->players[id].cantMove)
        {
            sem_wait(&syncState->currReadingSem);
            syncState->currReading--;
            sem_post(&syncState->currReadingSem);
            if (syncState->currReading == 0)
            {
                sem_post(&syncState->stateSem);
            }
            sem_post(&syncState->masterSem);
            break;
        }


        unsigned char move = (unsigned char) rand()%8;
        write(1, &move , sizeof(move));

        sem_wait(&syncState->currReadingSem);
        syncState->currReading--;
        sem_post(&syncState->currReadingSem);
        if (syncState->currReading == 0)
        {
            sem_post(&syncState->stateSem);
        }
        
        sem_post(&syncState->masterSem);
        usleep(500);
    }

    shm_cleanup(gameState_fd, gameState, sizeof(GameState) + sizeof(int) * h * w);
    shm_cleanup(syncState_fd, syncState, sizeof(GameSync));

    return 0;
}
