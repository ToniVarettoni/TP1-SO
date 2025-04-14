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
    unsigned short width; // Ancho del tablero
    unsigned short height; // Alto del tablero
    unsigned int playerAmount; // Cantidad de jugadores
    PlayerState players[MAX_PLAYERS]; // Lista de jugadores
    bool isOver; // Indica si el juego se ha terminado
    int map[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} GameState;

typedef struct {
    PlayerState player;
    int index;
} PlayerStateAux;
#endif