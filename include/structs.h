#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdio.h>
#include <semaphore.h>

#define MAX_LENGTH_NAME 16
#define MAX_PLAYERS 9

typedef struct {
    sem_t readyToPrint;
    sem_t printDone;
    sem_t masterSem;
    sem_t stateSem;
    sem_t currReadingSem;
    unsigned int currReading;
} GameSync;  

typedef struct {
    char name[MAX_LENGTH_NAME];
    unsigned int score;
    unsigned int invalidMoves;
    unsigned int validMoves;
    unsigned short x, y;
    pid_t pid;
    bool cantMove;
} PlayerState;

typedef struct {
    unsigned short width;
    unsigned short height;
    unsigned int playerAmount;
    PlayerState players[MAX_PLAYERS];
    bool isOver;
    int map[];
} GameState;

#endif