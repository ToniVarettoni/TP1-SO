#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdio.h>
#include <semaphore.h>

typedef struct {
    sem_t readyToPrint; // Se usa para indicarle a la vista que hay cambios por imprimir
    sem_t printDone; // Se usa para indicarle al master que la vista terminó de imprimir
    sem_t masterSem; // Mutex para evitar inanición del master al acceder al estado
    sem_t stateSem; // Mutex para el estado del juego
    sem_t currReadingSem; // Mutex para la siguiente variable
    unsigned int currReading; // Cantidad de jugadores leyendo el estado
    } gameSync;
    

typedef struct {
    char name[16]; // Nombre del jugador
    unsigned int score; // Puntaje
    unsigned int validMoves; // Cantidad de solicitudes de movimientos inválidas realizadas
    unsigned int invalidMoves; // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short x, y; // Coordenadas x e y en el tablero
    pid_t pid; // Identificador de proceso
    bool canMove; // Indica si el jugador tiene movimientos válidos disponibles
} playerState;

typedef struct {
    unsigned short width; // Ancho del tablero
    unsigned short height; // Alto del tablero
    unsigned int playerAmount; // Cantidad de jugadores
    playerState players[9]; // Lista de jugadores
    bool isOver; // Indica si el juego se ha terminado
    int map[]; // Punter o al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} GameState;
        