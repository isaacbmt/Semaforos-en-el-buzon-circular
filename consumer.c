#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <stdbool.h>

typedef enum { automatic, manual, no_specified } mode;

struct Stats {
    int consumers;
    int producers;
    int messages_counter;

    int start_write;
    int start_read;

    int deleted_consumers_by_key;

    double wait_time;
    double block_time;
    double user_time;
    double kernel_time;

    int buffer_size;
    int block_size;

    int finish;
};

int create_file_descriptor(char *name);
int* create_shared_memory(int file_descriptor, int length);
int run_process(mode chosen_mode, int *map, struct Stats* stats,
                sem_t* statsSem, sem_t* producerSem, sem_t* consumerSem);

int main(int argc, char** argv) {
    char bufferName[256];
    strcpy(bufferName, argv[1]);
    int time = 2, statsSize = sizeof (struct Stats);

    int file_descriptor = shm_open(bufferName, O_RDWR, 0777);
    if (file_descriptor == -1) {
        printf("Error al crear el descriptor del archivo uwu\n");
        return -1;
    }
    struct Stats *map = (struct Stats*) mmap(NULL, statsSize, PROT_READ|PROT_WRITE, MAP_SHARED, file_descriptor, 0);

}

int create_file_descriptor(char *name) {
    int file_descriptor = shm_open(name, O_RDWR, 0644);
    if(file_descriptor == -1) {
        fprintf(stderr, "Error creating file descriptor: %s\n", strerror(errno));
        exit(1);
    }
    return file_descriptor;
}

int* create_shared_memory(int file_descriptor, int length) {
    int *map = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, file_descriptor, 0);
    if (map == MAP_FAILED) {
        printf("Shared memory map failed\n");
        exit(1);
    }
    return map;
}