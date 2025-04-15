gcc maxPlayer.c include/shared_memory.c include/process_manager.c include/game_utils.c  -o MAX -lrt -Wall -O0 -lm
gcc view.c include/shared_memory.c include/process_manager.c include/game_utils.c  -o view -lrt -Wall -O0 -lm
gcc master.c include/shared_memory.c include/process_manager.c include/game_utils.c  -o master -lrt -Wall -O0 -lm
docker run -v "${PWD}:/root" --privileged -ti agodio/itba-so-multi-platform:3.0