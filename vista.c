#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "shared_memory.h"

enum player {
    PLAYER9 = -8,
    PLAYER8,
    PLAYER7,
    PLAYER6,
    PLAYER5,
    PLAYER4,
    PLAYER3,
    PLAYER2,
    PLAYER1
};

int visible_length(const char *s);
void printPad(int pad);

int main(int argc, char *argv[]){
    int h = atoi(argv[1]);
    int w = atoi(argv[2]);
    int gameState_fd;
    int syncState_fd;

    GameState * gameState = (GameState *)shm_open_and_map("/game_state", sizeof(GameState) + sizeof(int) * h * w, O_RDONLY, RO, &gameState_fd);
    gameSync * syncState = (gameSync *)shm_open_and_map("/game_sync", sizeof(gameSync), O_RDWR, RW, &syncState_fd);

    struct winsize terminalSize;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminalSize);
    int term_width = terminalSize.ws_col;

    while (!gameState->isOver)
    {
        sem_wait(&syncState->readyToPrint);
        printf("\033[H\033[J");

        int pad = 0;

        for (int i = 0; i < h; i++) {
            char line[4096];
            line[0] = '\0';

            for (int j = 0; j < w; j++) {
                int cellValue = gameState->map[i * w + j];
                if (cellValue > 0) {
                    sprintf(line + strlen(line), "[ %d ] ", cellValue);
                } else if (cellValue == PLAYER1) { // azul
                    sprintf(line + strlen(line), "\033[48;2;60;60;250m\033[38;2;0;0;10m[ 1 ]\033[0m ");
                } else if (cellValue == PLAYER2) { // rojo
                    sprintf(line + strlen(line), "\033[48;2;150;0;0m\033[38;2;0;0;10m[ 2 ]\033[0m ");
                } else if (cellValue == PLAYER3) { // verde
                    sprintf(line + strlen(line), "\033[48;2;0;100;0m\033[38;2;0;0;10m[ 3 ]\033[0m ");
                } else if (cellValue == PLAYER4) { // magenta
                    sprintf(line + strlen(line), "\033[48;2;130;0;130m\033[38;2;0;0;10m[ 4 ]\033[0m ");
                } else if (cellValue == PLAYER5) { // naranja
                    sprintf(line + strlen(line), "\033[48;2;200;100;0m\033[38;2;0;0;10m[ 5 ]\033[0m ");
                } else if (cellValue == PLAYER6) { // gris
                    sprintf(line + strlen(line), "\033[48;2;150;150;150m\033[38;2;0;0;10m[ 6 ]\033[0m ");
                } else if (cellValue == PLAYER7) { // marron
                    sprintf(line + strlen(line), "\033[48;2;119;53;10m\033[38;2;0;0;10m[ 7 ]\033[0m ");
                } else if (cellValue == PLAYER8) { // rosado
                    sprintf(line + strlen(line), "\033[48;2;200;100;150m\033[38;2;0;0;10m[ 8 ]\033[0m ");
                } else if (cellValue == PLAYER9) { // cyan
                    sprintf(line + strlen(line), "\033[48;2;0;50;70m\033[38;2;0;0;10m[ 9 ]\033[0m ");
                }
            }

            if (i == 0) {
                int vis_len = visible_length(line);
                pad = (term_width - vis_len) / 2;
                if (pad < 0) pad = 0;
            }

            for (int s = 0; s < pad; s++) {
                printf(" ");
            }

            printf("%s\n", line);
        }
        
        printf("\n"); printPad(pad); printf("SCOREBOARD:\n\n");

        for (int i = 0; i < gameState->playerAmount; i++) {
            int score = gameState->players[i].score;
            int invalidMoves = gameState->players[i].invalidMoves;
            int validMoves = gameState->players[i].validMoves;
            printPad(pad);
            printf("Score: %d | ValidMoves: %d | InvalidMoves: %d\n", score, validMoves, invalidMoves);
        }

        sem_post(&syncState->printDone);
    }

    shm_cleanup(gameState_fd, gameState, sizeof(GameState) + sizeof(int) * h * w);
    shm_cleanup(syncState_fd, syncState, sizeof(gameSync));

    return 0;
}

void printPad(int pad) {
    for (int s = 0; s < pad; s++) {
        printf(" ");
    }
}

int visible_length(const char *s) {
    int len = 0;
    int i = 0;
    while (s[i]) {
        if (s[i] == '\033') {
            // Salta toda la secuencia de escape hasta encontrar 'm'
            while (s[i] && s[i] != 'm')
                i++;
            if (s[i]) i++; // salta la 'm'
        } else {
            len++;
            i++;
        }
    }
    return len;
}
    



