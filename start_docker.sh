gcc player.c shared_memory.c process_manager.c game_utils.c -o player -lrt -Wall -O0
gcc maxPlayer.c shared_memory.c process_manager.c game_utils.c  -o MAX -lrt -Wall -O0
gcc vista.c shared_memory.c process_manager.c game_utils.c  -o vista -lrt -Wall -O0
gcc master.c shared_memory.c process_manager.c game_utils.c  -o master -lrt -Wall -O0
docker run -v "${PWD}:/root" --privileged -ti agodio/itba-so-multi-platform:3.0

#valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes ./master
#getopt
#lsof: lista las openfiles de un cierto programa



for (size_t i = 0; i < playerAmount; i++){

            if (view[0] != '\0'){
                usleep(delay * MICRO_TO_MILI);
                sem_post(&syncState->readyToPrint);
                sem_wait(&syncState->printDone);
            }

            currentPlayer = (currentPlayer + i) % gameState->playerAmount;

            int fd = playerPipes[currentPlayer];

            if (fd == -1)
                continue;

            if (checkCantMove(gameState, currentPlayer)){ 
                gameState->players[currentPlayer].cantMove = true;
                playingPlayers--;
                close(fd);
                playerPipes[currentPlayer] = -1;
                continue;
            }

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(fd, &readfds);

            struct timeval timeout = {0, 10};

            int ready = select(fd + 1, &readfds, NULL, NULL, &timeout);

            if (ready < 0 ){
                perror("select");
                continue;
            }

            if (ready > 0 && FD_ISSET(fd, &readfds)){

                unsigned char move;
                int n = read(fd, &move, sizeof(move));

                if (n <= 0){
                    close(fd);
                    playerPipes[currentPlayer] = -1;
                    playingPlayers--;
                }else if(processMove(gameState, currentPlayer, move)){
                    updateMap(gameState, currentPlayer);
                }
                lastMoveTime = time(NULL);
            }
        }