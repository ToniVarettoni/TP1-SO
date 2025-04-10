#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stddef.h>

#define RW (PROT_READ | PROT_WRITE)
#define RO PROT_READ

void * shm_create_and_map(const char* name, size_t size, int prot, int * fd);

void * shm_open_and_map(const char* name, size_t size, int flags, int prot, int * fd);

void shm_cleanup(int fd, void * addr, size_t size);

#endif