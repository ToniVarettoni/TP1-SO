<<<<<<< HEAD
gcc player.c include/shared_memory.c include/process_manager.c include/game_utils.c -o player -lrt -Wall -O0 -lm
gcc maxPlayer.c include/shared_memory.c include/process_manager.c include/game_utils.c  -o MAX -lrt -Wall -O0 -lm
gcc vista.c include/shared_memory.c include/process_manager.c include/game_utils.c  -o vista -lrt -Wall -O0 -lm
gcc view.c include/shared_memory.c include/process_manager.c include/game_utils.c  -o view -lrt -Wall -O0 -lm
gcc master.c include/shared_memory.c include/process_manager.c include/game_utils.c  -o master -lrt -Wall -O0 -lm
=======
gcc player.c shared_memory.c -o player -lrt -Wall -O0
gcc maxPlayer.c shared_memory.c -o MAX -lrt -Wall -O0
gcc vista.c shared_memory.c -o vista -lrt -Wall -O0
gcc master.c shared_memory.c -o master -lrt -Wall -O0
>>>>>>> 0c45ae58daa5431665467dadff1dfc637e0cd6c5
docker run -v "${PWD}:/root" --privileged -ti agodio/itba-so-multi-platform:3.0

#valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes ./master
#getopt
#lsof: lista las openfiles de un cierto programa