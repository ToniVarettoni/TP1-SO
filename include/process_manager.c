#include "process_manager.h"
#include <string.h>


void initView(GameState * gameState, char * view, int * viewPid){

    int p = fork();
    
    if (p == -1){
        perror("fork failed");
        exit(EXIT_FAILURE);
    }else if (p == 0){
        char path[MAX_LENGTH_PATH];
        strcpy(path, "./"); 
        strcat(path, view);
        char height[MAX_LENGTH_NUM];
        char width[MAX_LENGTH_NUM];

        sprintf(height, "%d", gameState->height);
        sprintf(width, "%d", gameState->width);

        char *args[] = {path, width, height, NULL};
        execve(path, args, NULL);
        perror("execve view");
        exit(EXIT_FAILURE);
    } else *(viewPid) = p;
}

int initPlayer(GameState * gameState, int i){


    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int p = fork(); 
    
    if (p == -1){
        perror("fork failed");
        exit(EXIT_FAILURE);
    }else if (p == 0){
        gameState->players[i].pid = getpid();
        
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        char path[MAX_LENGTH_PATH];
        strcpy(path, "./"); 
        strcat(path, gameState->players[i].name);
        char height[MAX_LENGTH_NUM];
        char width[MAX_LENGTH_NUM];

        sprintf(height, "%d", gameState->height);
        sprintf(width, "%d", gameState->width);

        char *args[] = {path, width, height, NULL};        
        execve(path, args, NULL);
        perror("execve player");
        exit(EXIT_FAILURE);
    }   

    close(pipefd[1]);
    return pipefd[0];

}