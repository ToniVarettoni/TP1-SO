#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include "include/structs.h"
#include "include/shared_memory.h"

#define CELL_WIDTH 7
#define SIDE_BORDER_WIDTH 2
#define SIDE_BORDER "  "
#define CAT "=^.^="
#define MAX_LINE_LENGTH 8192

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

const char* PLAYER_BG[9] = {
    "\033[48;2;253;90;70m",
    "\033[48;2;255;122;0m",
    "\033[48;2;255;197;103m",
    "\033[48;2;145;234;104m",
    "\033[48;2;0;153;94m",
    "\033[48;2;5;140;215m",
    "\033[48;2;75;135;255m",
    "\033[48;2;180;145;255m",
    "\033[48;2;251;125;168m"
};

const char* PLAYER_HEAD[9] = {
    "\033[48;2;255;120;100m",
    "\033[48;2;255;152;40m",
    "\033[48;2;255;217;143m",
    "\033[48;2;170;255;130m",
    "\033[48;2;30;183;120m",
    "\033[48;2;35;170;235m",
    "\033[48;2;95;165;255m",
    "\033[48;2;200;170;255m",
    "\033[48;2;255;145;188m"
};

#define TEXT_DARK "\033[38;2;97;61;45m"

#define GRID_LIGHT_COLOR "\033[48;2;208;208;208m"
#define GRID_DARK_COLOR  "\033[48;2;160;160;160m"

#define BORDER_COLOR "\033[48;2;97;61;45m"
#define BACKGROUND_COLOR "\033[48;2;248;239;212m"
#define MID_BROWN_COLOR "\033[48;2;185;162;130m"

#define RESET "\033[0m"
#define BOLD "\033[1m"

void printHeader(int boardWidth, int totalPad);
void printBoardLine(GameState* gameState, int i, int pad);
void printScoreboard(GameState* gameState, int boardWidth, int totalPad, int termWidth);
void printRanking(GameState * gameState, int boardWidth, int totalPad, int termWidth);

int main(int argc, char *argv[]){
    int w = atoi(argv[1]);
    int h = atoi(argv[2]);
    int gameState_fd;
    int syncState_fd;

    GameState * gameState = (GameState *)shm_open_and_map("/game_state", sizeof(GameState) + sizeof(int) * h * w, O_RDONLY, RO, &gameState_fd);
    GameSync * syncState = (GameSync *)shm_open_and_map("/game_sync", sizeof(GameSync), O_RDWR, RW, &syncState_fd);

    struct winsize terminalSize;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminalSize);
    int termWidth = terminalSize.ws_col;

    int boardWidth = w * CELL_WIDTH;
    int totalWidth = boardWidth + 2 * SIDE_BORDER_WIDTH;
    int totalPad = (termWidth - totalWidth) / 2;
    if (totalPad < 0) totalPad = 0;
    
    while (!gameState->isOver)
    {
        sem_wait(&syncState->readyToPrint);
        
        printf("\033[H\033[J");

        printHeader(boardWidth, totalPad);
        for (int i = 0; i < h; i++) {
            printBoardLine(gameState, i, totalPad);
        }
        printScoreboard(gameState, boardWidth, totalPad, termWidth);

        sem_post(&syncState->printDone);
    }
    sleep(1);
    printRanking(gameState, boardWidth, totalPad, termWidth);

    shm_cleanup(gameState_fd, gameState, sizeof(GameState) + sizeof(int) * h * w);
    shm_cleanup(syncState_fd, syncState, sizeof(GameSync));

    return 0;
}

void printPad(int pad) {
    for (int s = 0; s < pad; s++) {
        printf(" ");
    }
}

int visibleLength(const char *s) {
    int len = 0;
    int i = 0;
    while (s[i]) {
        if (s[i] == '\033') {
            while (s[i] && s[i] != 'm')
                i++;
            if (s[i]) i++;
        } else {
            len++;
            i++;
        }
    }
    return len;
}

void printBorder(int borderWidth, char * borderColor){
    for (int i = 0; i < borderWidth; i++) printf("%s ", borderColor);
    printf("%s", RESET);
}

void printWithBorder(int borderWidth, char * borderColor, char * string){
    printBorder(borderWidth, borderColor);
    printf("%s", string);
    printBorder(borderWidth, borderColor);
    printf("%s", RESET);
}

void printColorLine(int borderWidth, char * borderColor, int centerWidth, char * centerColor, int pad){
    printPad(pad);
    printBorder(borderWidth, borderColor);
    for (int i = 0; i < centerWidth; i++) printf("%s ", centerColor);
    printBorder(borderWidth, borderColor);
    printf("%s\n", RESET);
}

void printHeader(int boardWidth, int totalPad) {
    printColorLine(SIDE_BORDER_WIDTH, BORDER_COLOR, boardWidth, BORDER_COLOR, totalPad);
    printColorLine(SIDE_BORDER_WIDTH, BORDER_COLOR, boardWidth, BACKGROUND_COLOR, totalPad);
    
    const char * title = "ChompCats";
    int titleLen = strlen(title);
    int titlePad = (boardWidth - titleLen) / 2;

    printPad(totalPad);
    printf("%s%s", BORDER_COLOR, SIDE_BORDER);
    for (int i = 0; i < boardWidth; i++) {
        if (i == titlePad) {
            printf("%s%s%s%s%s", BOLD, TEXT_DARK, BACKGROUND_COLOR, title, RESET);
            i += titleLen - 1;
        } else {
            printf("%s ", BACKGROUND_COLOR);
        }
    }
    printf("%s%s%s\n", BORDER_COLOR, SIDE_BORDER, RESET);

    printColorLine(SIDE_BORDER_WIDTH, BORDER_COLOR, boardWidth, BACKGROUND_COLOR, totalPad);
}

void printBoardLine(GameState* gameState, int i, int pad) {
    char sideLine[MAX_LINE_LENGTH];
    sideLine[0] = '\0';
    char midLine[MAX_LINE_LENGTH];
    midLine[0] = '\0';

    for (int j = 0; j < gameState->width; j++) {
        int cellValue = gameState->map[i * gameState->width + j];

        if (cellValue <= PLAYER1 && cellValue >= PLAYER9) {
            PlayerState p = gameState->players[0 - cellValue];
            const char * bgColor;
            if(p.x == j && p.y == i){
                bgColor = PLAYER_HEAD[0 - cellValue];
                sprintf(sideLine + strlen(sideLine), "%s       %s", bgColor, RESET);
                sprintf(midLine + strlen(midLine), "%s %s %s", bgColor, CAT, RESET);
            }else{
                bgColor = PLAYER_BG[0 - cellValue];
                sprintf(sideLine + strlen(sideLine), "%s       %s", bgColor, RESET);
                sprintf(midLine + strlen(midLine), "%s       %s", bgColor, RESET);
            }
        } else {
            int isLight = (i + j) % 2;
            sprintf(sideLine + strlen(sideLine), "%s       %s", isLight? GRID_LIGHT_COLOR : GRID_DARK_COLOR, RESET);
            sprintf(midLine + strlen(midLine), "%s   %d   %s", isLight? GRID_LIGHT_COLOR : GRID_DARK_COLOR, cellValue, RESET);
        }
    }

    printPad(pad); printf("%s%s%s%s%s%s\n", BORDER_COLOR, SIDE_BORDER, sideLine, BORDER_COLOR, SIDE_BORDER, RESET);
    printPad(pad); printf("%s%s%s%s%s%s\n", BORDER_COLOR, SIDE_BORDER, midLine, BORDER_COLOR, SIDE_BORDER, RESET);
    printPad(pad); printf("%s%s%s%s%s%s\n", BORDER_COLOR, SIDE_BORDER, sideLine, BORDER_COLOR, SIDE_BORDER, RESET);
}

void printScoreTable(PlayerStateAux players[], int size, int boardWidth, int totalPad, int termWidth, char * title){
    int colNameWidth     = 12;
    int colNumWidth      = 10;
    int colScoreWidth    = 10;
    int colValidWidth    = 14;
    int colInvalidWidth  = 14;

    char line[MAX_LINE_LENGTH];
    line[0] = '\0';
    char clearLine[MAX_LINE_LENGTH];
    clearLine[0] = '\0';
    char aux[2 * MAX_LINE_LENGTH];

    sprintf(line, "%-*s%-*s%-*s%-*s%-*s", colNameWidth, "Name", colNumWidth, "Number", colScoreWidth, "Score", colValidWidth, "Valid Moves", colInvalidWidth, "Invalid Moves");

    int vis_len = strlen(line);
    int totalWidth = vis_len + 2;
    int pad = (termWidth - totalWidth) / 2;
    if (pad < 0) pad = 0;

    printColorLine(SIDE_BORDER_WIDTH, BORDER_COLOR, boardWidth, BORDER_COLOR, totalPad);
    printColorLine(pad - totalPad, BORDER_COLOR, totalWidth, MID_BROWN_COLOR, totalPad);

    int titleLen = strlen(title);
    int titlePad = (totalWidth - titleLen) / 2;

    for (int i = 0; i < totalWidth; i++) {
        if (i == titlePad) {
            sprintf(clearLine + strlen(clearLine), "%s%s%s%s%s", BOLD, TEXT_DARK, MID_BROWN_COLOR, title, RESET);
            i += titleLen - 1;
        } else {
            sprintf(clearLine + strlen(clearLine), "%s ", MID_BROWN_COLOR);
        }
    }
    printPad(totalPad); printWithBorder(pad - totalPad, BORDER_COLOR, clearLine); printf("\n");
    printColorLine(pad - totalPad, BORDER_COLOR, totalWidth, MID_BROWN_COLOR, totalPad);
    

    sprintf(aux, "%s%s%s %s %s", BOLD, BACKGROUND_COLOR, TEXT_DARK, line, RESET);
    printPad(totalPad); printWithBorder(pad - totalPad, BORDER_COLOR, aux); printf("\n");

    for (int i = 0; i < size; i++) {
        clearLine[0] = '\0';
        line[0] = '\0';
        PlayerState p = players[i].player;
        int playerNum = players[i].index + 1;

        sprintf(line, "%-*s%-*d%-*d%-*d%-*d", colNameWidth, p.name, colNumWidth, playerNum, colScoreWidth, p.score, colValidWidth, p.validMoves, colInvalidWidth, p.invalidMoves);
        sprintf(aux, "%s%s %s%s%s %s", BACKGROUND_COLOR, TEXT_DARK, PLAYER_HEAD[players[i].index], line, BACKGROUND_COLOR, RESET);
        sprintf(clearLine, "%s %s", BACKGROUND_COLOR, PLAYER_HEAD[players[i].index]);
        for(int j = 0; j < strlen(line); j++){
            sprintf(clearLine + strlen(clearLine), " ");
        }
        sprintf(clearLine + strlen(clearLine), "%s ", BACKGROUND_COLOR);
        
        printPad(totalPad); printWithBorder(pad - totalPad, BORDER_COLOR, clearLine); printf("\n");
        printPad(totalPad); printWithBorder(pad - totalPad, BORDER_COLOR, aux); printf("\n");
        printPad(totalPad); printWithBorder(pad - totalPad, BORDER_COLOR, clearLine); printf("\n");
    }
    printColorLine(pad - totalPad, BORDER_COLOR, totalWidth, BACKGROUND_COLOR, totalPad);
}

void printScoreboard(GameState* gameState, int boardWidth, int totalPad, int termWidth) {
    printColorLine(SIDE_BORDER_WIDTH, BORDER_COLOR, boardWidth, BORDER_COLOR, totalPad);
    printColorLine(SIDE_BORDER_WIDTH, BORDER_COLOR, boardWidth, BORDER_COLOR, totalPad);

    int colNameWidth     = 12;
    int colNumWidth      = 10;
    int colScoreWidth    = 10;
    int colValidWidth    = 14;
    int colInvalidWidth  = 14;

    char line[MAX_LINE_LENGTH];
    line[0] = '\0';

    sprintf(line, "%-*s%-*s%-*s%-*s%-*s", colNameWidth, "Name", colNumWidth, "Number", colScoreWidth, "Score", colValidWidth, "Valid Moves", colInvalidWidth, "Invalid Moves");

    int vis_len = strlen(line);
    int totalWidth = vis_len + 2;
    int pad = (termWidth - totalWidth) / 2;
    if (pad < 0) pad = 0;

    printPad(totalPad);
    printBorder(pad - totalPad, BORDER_COLOR);
    printf("%s%s%s %s %s", BOLD, BACKGROUND_COLOR, TEXT_DARK, line, RESET);
    printBorder(pad - totalPad, BORDER_COLOR);
    printf("\n");

    for (int i = 0; i < gameState->playerAmount; i++) {
        line[0] = '\0';
        PlayerState *p = &gameState->players[i];
        int playerNum = i + 1;

        sprintf(line, "%-*s%-*d%-*d%-*d%-*d", colNameWidth, p->name, colNumWidth, playerNum, colScoreWidth, p->score, colValidWidth, p->validMoves, colInvalidWidth, p->invalidMoves);

        printPad(totalPad);
        printBorder(pad - totalPad, BORDER_COLOR);
        printf("%s%s %s%s%s %s", BACKGROUND_COLOR, TEXT_DARK, PLAYER_HEAD[i], line, BACKGROUND_COLOR, RESET);
        printBorder(pad - totalPad, BORDER_COLOR);
        printf("\n");
    }
    printColorLine(pad - totalPad, BORDER_COLOR, totalWidth, BACKGROUND_COLOR, totalPad);
    printColorLine(SIDE_BORDER_WIDTH, BORDER_COLOR, boardWidth, BORDER_COLOR, totalPad);
    printColorLine(SIDE_BORDER_WIDTH, BORDER_COLOR, boardWidth, BORDER_COLOR, totalPad);
}

int compareScores(const void *a, const void *b) {
    PlayerState playerA = ((PlayerStateAux *) a)->player;
    PlayerState playerB = ((PlayerStateAux *) b)->player;
    int ans = playerB.score - playerA.score;
    if(ans == 0){
        ans = playerB.validMoves - playerA.validMoves;
        if(ans == 0){
            ans = playerA.invalidMoves - playerB.invalidMoves;
        }
    }
    return ans;
}

void printRanking(GameState * gameState, int boardWidth, int totalPad, int termWidth){
    printf("\033[H\033[J");
    printHeader(boardWidth, totalPad);
    printColorLine(SIDE_BORDER_WIDTH, BORDER_COLOR, boardWidth, BORDER_COLOR, totalPad);

    
    
    PlayerStateAux playersInOrder[gameState->playerAmount];
    for(int i = 0; i < gameState->playerAmount; i++){
        playersInOrder[i].player = gameState->players[i];
        playersInOrder[i].index = i;
    }
    qsort(playersInOrder, gameState->playerAmount, sizeof(PlayerStateAux), compareScores);

    printScoreTable(playersInOrder, 1, boardWidth, totalPad, termWidth, "Winner");

    printColorLine(SIDE_BORDER_WIDTH, BORDER_COLOR, boardWidth, BORDER_COLOR, totalPad);
    
    printScoreTable(playersInOrder, gameState->playerAmount, boardWidth, totalPad, termWidth, "Ranking");

    printColorLine(SIDE_BORDER_WIDTH, BORDER_COLOR, boardWidth, BORDER_COLOR, totalPad);
    printColorLine(SIDE_BORDER_WIDTH, BORDER_COLOR, boardWidth, BORDER_COLOR, totalPad);
}



