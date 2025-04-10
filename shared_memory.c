#include "shared_memory.h"
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

void * shm_create_and_map(const char* name, size_t size, int prot, int * fd){
    *fd = shm_open(name, O_RDWR | O_CREAT, 0666);
    if (*fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(*fd, size) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void *p = mmap(NULL, size, prot, MAP_SHARED, *fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return p;
}

void * shm_open_and_map(const char* name, size_t size, int flags, int prot, int * fd){
    *fd = shm_open(name, flags, 0666);
    if (*fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    void *p = mmap(NULL, size, prot, MAP_SHARED, *fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return p;
}

void shm_cleanup(int fd, void * addr, size_t size){
    munmap(addr, size);
    close(fd);
}