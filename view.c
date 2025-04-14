#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include "include/structs.h"
#include "include/shared_memory.h"

typedef struct {
    PlayerState player;
    int index;
} PlayerStateIdx;

#define CELL_WIDTH 7
#define SIDE_BORDER_WIDTH 2
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

const char* PLAYER_BODY[9] = {
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

void printHeader(char * title, int borderWidth, char * borderColor, int centerWidth, char * centerColor, int pad);
void printBoardLine(GameState* gameState, int i, int pad);
void printScoreboard(GameState* gameState, int boardWidth, int totalPad, int termWidth);
void printScoreTable(PlayerStateIdx players[], int size, int totalPad, int termWidth, char * title, bool large);
void printRanking(GameState * gameState, int boardWidth, int totalPad, int termWidth);

#define PRINT_WITH_BORDER(borderWidth, borderColor, ...) \
    do { \
        for (int i = 0; i < borderWidth; i++) printf("%s ", borderColor); \
        printf("%s", RESET); \
        __VA_ARGS__; \
        for (int i = 0; i < borderWidth; i++) printf("%s ", borderColor); \
        printf("%s", RESET); \
    } while (0)

#define PRINT_LINE(pad, ...) \
    do { \
        for (int s = 0; s < pad; s++) printf(" "); \
        __VA_ARGS__; \
        printf("\n"); \
    } while (0)

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
    
    printf("\033[H\033[J");
    while (!gameState->isOver)
    {
        printf("\033[H");
        sem_wait(&syncState->readyToPrint);

        printHeader("ChompCats", SIDE_BORDER_WIDTH, BORDER_COLOR, boardWidth, BACKGROUND_COLOR, totalPad);
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

void printMargin(int width, int pad){
    PRINT_LINE(pad, {PRINT_WITH_BORDER(0, "\0", {for (int i = 0; i < width; i++) printf("%s ", BORDER_COLOR);});});
}

void printHeader(char * title, int borderWidth, char * borderColor, int centerWidth, char * centerColor, int pad) {
    printMargin(centerWidth + 2 * borderWidth, pad);
    PRINT_LINE(pad, {PRINT_WITH_BORDER(borderWidth, borderColor, {for (int i = 0; i < centerWidth; i++) printf("%s ", centerColor);});});
    
    int titleLen = strlen(title);
    int titlePad = (centerWidth - titleLen) / 2;

    PRINT_LINE(pad, {
        PRINT_WITH_BORDER(borderWidth, borderColor, {
            for (int i = 0; i < centerWidth; i++) {
                if (i == titlePad) {
                    printf("%s%s%s%s%s", BOLD, TEXT_DARK, centerColor, title, RESET);
                    i += titleLen - 1;
                } else {
                    printf("%s ", centerColor);
                }
            }
        });
    });
    PRINT_LINE(pad, {PRINT_WITH_BORDER(borderWidth, borderColor, {for (int i = 0; i < centerWidth; i++) printf("%s ", centerColor);});});
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
                bgColor = PLAYER_BODY[0 - cellValue];
                sprintf(sideLine + strlen(sideLine), "%s       %s", bgColor, RESET);
                sprintf(midLine + strlen(midLine), "%s       %s", bgColor, RESET);
            }
        } else {
            int isLight = (i + j) % 2;
            sprintf(sideLine + strlen(sideLine), "%s       %s", isLight? GRID_LIGHT_COLOR : GRID_DARK_COLOR, RESET);
            sprintf(midLine + strlen(midLine), "%s   %d   %s", isLight? GRID_LIGHT_COLOR : GRID_DARK_COLOR, cellValue, RESET);
        }
    }

    PRINT_LINE(pad, {PRINT_WITH_BORDER(SIDE_BORDER_WIDTH, BORDER_COLOR, printf("%s", sideLine));});
    PRINT_LINE(pad, {PRINT_WITH_BORDER(SIDE_BORDER_WIDTH, BORDER_COLOR, printf("%s", midLine));});
    PRINT_LINE(pad, {PRINT_WITH_BORDER(SIDE_BORDER_WIDTH, BORDER_COLOR, printf("%s", sideLine));});
}

void printScoreTable(PlayerStateIdx players[], int size, int totalPad, int termWidth, char * title, bool large){
    int colNameWidth     = 12;
    int colNumWidth      = 10;
    int colScoreWidth    = 10;
    int colValidWidth    = 14;
    int colInvalidWidth  = 14;

    char line[MAX_LINE_LENGTH];
    line[0] = '\0';
    
    sprintf(line, "%s%s%s%-*s%-*s%-*s%-*s%-*s", BOLD, BACKGROUND_COLOR, TEXT_DARK, colNameWidth, "Name", colNumWidth, "Number", colScoreWidth, "Score", colValidWidth, "Valid Moves", colInvalidWidth, "Invalid Moves");

    int vis_len = visibleLength(line);
    int totalWidth = vis_len + 2;
    int pad = (termWidth - totalWidth) / 2;
    if (pad < 0) pad = 0;

    if(strcmp(title, "\0") != 0){
        printHeader(title, pad - totalPad, BORDER_COLOR, totalWidth, MID_BROWN_COLOR, totalPad);
    }

    PRINT_LINE(totalPad, {PRINT_WITH_BORDER(pad - totalPad, BORDER_COLOR, {PRINT_WITH_BORDER(1, BACKGROUND_COLOR, {printf("%s", line);});});});

    for (int i = 0; i < size; i++) {
        line[0] = '\0';
        PlayerState p = players[i].player;
        int playerNum = players[i].index + 1;

        sprintf(line, "%s%s%-*s%-*d%-*d%-*d%-*d", TEXT_DARK, PLAYER_HEAD[players[i].index], colNameWidth, p.name, colNumWidth, playerNum, colScoreWidth, p.score, colValidWidth, p.validMoves, colInvalidWidth, p.invalidMoves);
                
        if(large) PRINT_LINE(totalPad, {PRINT_WITH_BORDER(pad - totalPad, BORDER_COLOR, {PRINT_WITH_BORDER(1, BACKGROUND_COLOR, {for(int j = 0; j < vis_len; j++) printf("%s ", PLAYER_HEAD[players[i].index]);});});});
        PRINT_LINE(totalPad, {PRINT_WITH_BORDER(pad - totalPad, BORDER_COLOR, {PRINT_WITH_BORDER(1, BACKGROUND_COLOR, {printf("%s", line);});});});
        if(large) PRINT_LINE(totalPad, {PRINT_WITH_BORDER(pad - totalPad, BORDER_COLOR, {PRINT_WITH_BORDER(1, BACKGROUND_COLOR, {for(int j = 0; j < vis_len; j++) printf("%s ", PLAYER_HEAD[players[i].index]);});});});
    }
    PRINT_LINE(totalPad, {PRINT_WITH_BORDER(pad - totalPad, BORDER_COLOR, {for (int i = 0; i < totalWidth; i++) printf("%s ", BACKGROUND_COLOR);});});
}

void printScoreboard(GameState* gameState, int boardWidth, int totalPad, int termWidth) {
    printMargin(boardWidth + 2 * SIDE_BORDER_WIDTH, totalPad);
    printMargin(boardWidth + 2 * SIDE_BORDER_WIDTH, totalPad);

    PlayerStateIdx players[gameState->playerAmount];
    for(int i = 0; i < gameState->playerAmount; i++){
        players[i].player = gameState->players[i];
        players[i].index = i;
    }

    printScoreTable(players, gameState->playerAmount, totalPad, termWidth, "\0", false);

    printMargin(boardWidth + 2 * SIDE_BORDER_WIDTH, totalPad);
    printMargin(boardWidth + 2 * SIDE_BORDER_WIDTH, totalPad);
}

int compareScores(const void *a, const void *b) {
    PlayerState playerA = ((PlayerStateIdx *) a)->player;
    PlayerState playerB = ((PlayerStateIdx *) b)->player;
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
    printHeader("ChompCats", SIDE_BORDER_WIDTH, BORDER_COLOR, boardWidth, BACKGROUND_COLOR, totalPad);
    printMargin(boardWidth + 2 * SIDE_BORDER_WIDTH, totalPad);

    PlayerStateIdx playersInOrder[gameState->playerAmount];
    for(int i = 0; i < gameState->playerAmount; i++){
        playersInOrder[i].player = gameState->players[i];
        playersInOrder[i].index = i;
    }
    qsort(playersInOrder, gameState->playerAmount, sizeof(PlayerStateIdx), compareScores);

    printScoreTable(playersInOrder, 1, totalPad, termWidth, "Winner", true);

    printMargin(boardWidth + 2 * SIDE_BORDER_WIDTH, totalPad);
    
    printScoreTable(playersInOrder, gameState->playerAmount, totalPad, termWidth, "Ranking", true);

    printMargin(boardWidth + 2 * SIDE_BORDER_WIDTH, totalPad);
    printMargin(boardWidth + 2 * SIDE_BORDER_WIDTH, totalPad);
}



