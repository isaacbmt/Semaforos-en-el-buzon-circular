#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>

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

int init_semaphores(char* statsSemaphore, char* producerSemaphore, char* consumerSemaphore, int block_size);
int create_file_descriptor(char *name, int length);
int* create_shared_memory(int file_descriptor, int length);

int main(__attribute__((unused)) int argc, char** argv) {
    char bufferName[256];
    char structName[256];
    char statsSemaphoreName[256];
    char producerSemaphoreName[256];
    char consumerSemaphoreName[256];
    char* ptr1, *ptr2;
    // Set name to the buffer and shared struct
    strcpy(bufferName, argv[1]);
    strcpy(structName, argv[1]);
    strcat(structName, "_struct");
    // Set name to semaphores
    strcpy(statsSemaphoreName, bufferName);
    strcat(statsSemaphoreName, "_SS");
    strcpy(producerSemaphoreName, bufferName);
    strcat(producerSemaphoreName, "_PS");
    strcpy(consumerSemaphoreName, bufferName);
    strcat(consumerSemaphoreName, "_CS");
    // Set size of buffer and blocks
    int bufferSize = (int) strtol(argv[2], &ptr1, 10);
    int blockSize = (int) strtol(argv[3], &ptr2, 10);
    int stats_size = (int) sizeof(struct Stats);
    int length =  bufferSize * blockSize;

    // Create shared buffer
    int mem_fd = create_file_descriptor(bufferName, length);
    create_shared_memory(mem_fd, length);

    // Create shared struct
    int stats_fd = create_file_descriptor(structName, stats_size);
    struct Stats *stats = (struct Stats*) create_shared_memory(stats_fd, stats_size);

    int semaphoresInitResult = init_semaphores(statsSemaphoreName, producerSemaphoreName, consumerSemaphoreName, blockSize);

    if (semaphoresInitResult == 1){
        printf("Error while creating the semaphores\n");
        close(stats_fd);
        close(mem_fd);
        shm_unlink(bufferName);
        shm_unlink(structName);
        return 1;
    }
    stats->block_size = blockSize;
    stats->buffer_size = bufferSize;
    printf("Semaphores and Shared Buffer created successful\n");
    return 0;
}

int create_file_descriptor(char *name, int length) {
    int file_descriptor = shm_open(name, O_CREAT | O_RDWR, 0644);
    if(file_descriptor == -1) {
        fprintf(stderr, "Error creating file descriptor: %s\n", strerror(errno));
        shm_unlink(name);
        exit(1);
    }
    if (ftruncate(file_descriptor, length) == -1) {
        fprintf(stderr, "Error truncate \n");
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

int init_semaphores (char* statsSemaphore, char* producerSemaphore, char* consumerSemaphore, int block_size) {

    sem_t* statsSem = sem_open(statsSemaphore, O_CREAT, 0644, 1);
    sem_t* producerSem = sem_open(producerSemaphore, O_CREAT, 0644, block_size);
    sem_t* consumerSem = sem_open(consumerSemaphore, O_CREAT, 0644, 0);

    if (statsSem == SEM_FAILED){
        perror("Open the stats semaphore failed\n");
        return 1;
    }

    else if (producerSem == SEM_FAILED){
        perror("Open the producer semaphore failed\n");
        return 1;
    }

    else if (consumerSem == SEM_FAILED){
        perror("Open the consumer semaphore failed\n");
        return 1;
    }

    else{
        return 0;
    }
}
