#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>

#define RED "\033[1;31m"
#define BLACK "\033[1;30m"
#define GREEN "\033[1;32m"
#define BLUE "\033[1;34m"
#define RESET "\033[0m"

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
int  openSemaphores (sem_t const* statsSem, sem_t const* producerSem, sem_t const* consumerSem, char* statsSemaphoreName,
                     char* producerSemaphoreName, char* consumerSemaphoreName);
int endSemaphores(sem_t* statsSem, sem_t* producerSem, sem_t* consumerSem,
                  char* statsSemaphoreName, char* producerSemaphoreName, char* consumerSemaphoreName);
void finishingMessage(struct Stats* stats);
int run_process( struct Stats* stats, sem_t* statsSem, sem_t* producerSem,
        sem_t* consumerSem, char* bufferName, char* structName, int stats_fd, int mem_fd,
                char* statsSemaphoreName, char* producerSemaphoreName, char* consumerSemaphoreName);

int main(__attribute__((unused)) int argc, char** argv) {
    int stats_size = sizeof (struct Stats);
    char bufferName[256];
    char structName[256];
    char statsSemaphoreName[256];
    char producerSemaphoreName[256];
    char consumerSemaphoreName[256];
    sem_t* statsSem = NULL;
    sem_t* producerSem = NULL;
    sem_t* consumerSem = NULL;
    strcpy(bufferName, argv[1]);
    strcpy(structName, argv[1]);
    strcat(structName, "_struct");
    strcpy(statsSemaphoreName, bufferName);
    strcat(statsSemaphoreName, "_SS");
    strcpy(producerSemaphoreName, bufferName);
    strcat(producerSemaphoreName, "_PS");
    strcpy(consumerSemaphoreName, bufferName);
    strcat(consumerSemaphoreName, "_CS");

    //Initialize the semaphores
    int semaphoresInitResult = openSemaphores(statsSem, producerSem, consumerSem,
                                              statsSemaphoreName, producerSemaphoreName, consumerSemaphoreName);
    if(semaphoresInitResult == 1){
        printf("Unable to open the semaphores\n");
        return 1;
    }

    // Create shared struct
    int stats_fd = create_file_descriptor(structName);
    struct Stats *stats = (struct Stats*) create_shared_memory(stats_fd, stats_size);

    // Create shared buffer
    int mem_fd = create_file_descriptor(bufferName);
    sem_wait(statsSem);
    int length = stats->buffer_size * stats->block_size;
    sem_post(statsSem);
    create_shared_memory(mem_fd, length);

    // Enable the global finish value
    sem_wait(statsSem);
    stats->finish = 1;
    sem_post(statsSem);




    return run_process(stats, statsSem, producerSem, consumerSem, bufferName, structName, stats_fd, mem_fd,
                       statsSemaphoreName, producerSemaphoreName, consumerSemaphoreName);
}


int create_file_descriptor(char *name) {
    int file_descriptor = shm_open(name, O_RDWR, 0644);
    if(file_descriptor == -1) {
        fprintf(stderr, "Error creating file descriptor: %s\n", strerror(errno));
        shm_unlink(name);
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

int  openSemaphores (sem_t const* statsSem, sem_t const* producerSem, sem_t const* consumerSem, char* statsSemaphoreName,
                     char* producerSemaphoreName, char* consumerSemaphoreName) {
    statsSem = sem_open(statsSemaphoreName, 0);
    producerSem = sem_open(producerSemaphoreName, 0);
    consumerSem  = sem_open(consumerSemaphoreName, 0);
    if (statsSem == SEM_FAILED) {
        perror("Opening the stats semaphore failed\n");
        return 1;
    }
    else if (producerSem == SEM_FAILED) {
        perror("Opening the producer semaphore failed\n");
        return 1;
    }
    else if(consumerSem == SEM_FAILED) {
        perror("Opening the consumer semaphore failed\n");
        return 1;
    }
    else {
        return 0;
    }
}

int endSemaphores(sem_t* statsSem, sem_t* producerSem, sem_t* consumerSem,
                  char* statsSemaphoreName, char* producerSemaphoreName, char* consumerSemaphoreName) {

    if (sem_close(statsSem) < 0 || sem_unlink(statsSemaphoreName) < 0){
        printf("Failed to close or unlink the stats semaphore\n");
        return 1;
    }
    else if (sem_close(producerSem) < 0 || sem_unlink(producerSemaphoreName) < 0){
        printf("Failed to close or unlink the producer semaphore\n");
        return 1;
    }
    else if (sem_close(consumerSem) < 0 || sem_unlink(consumerSemaphoreName) < 0){
        printf("Failed to close or unlink the consumer semaphore\n");
        return 1;
    }
    else{
        return 0;
    }

}

void finishingMessage(struct Stats* stats, int current_cons, int current_prod) {
    printf(RED);
    printf("* * * * * Se ha cerrado la memoria compartida * * * * *\n");
    printf(BLUE);
    printf("Los datos finales son: \n");
    printf(GREEN);
    printf("Mensajes totales: ");
    printf(RESET);
    printf("%d\n", stats->messages_counter);
    printf(GREEN);
    printf("Consumidores totales: ");
    printf(RESET);
    printf("%d\n", current_cons);
    printf(GREEN);
    printf("Productores totales: ");
    printf(RESET);
    printf("%d\n", current_prod);
    printf(GREEN);
    printf("Consumidores eliminados por llave: ");
    printf(RESET);
    printf("%d\n", stats->deleted_consumers_by_key);
    printf(GREEN);
    printf("Tiempo esperando total: ");
    printf(RESET);
    printf("%lf\n", stats->wait_time);
    printf(GREEN);
    printf("Tiempo bloqueado total: ");
    printf(RESET);
    printf("%lf\n", stats->block_time);
    printf(GREEN);
    printf("Tiempo de usuario total: ");
    printf(RESET);
    printf("%lf\n", stats->user_time);
    printf(GREEN);
    printf("Tiempo de kernel total: ");
    printf(RESET);
    printf("%lf\n", stats->kernel_time);
    printf(RED);
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    printf(RESET);
}

int run_process(struct Stats* stats, sem_t* statsSem, sem_t* producerSem, sem_t* consumerSem, char* bufferName,
        char* structName, int stats_fd, int mem_fd, char* statsSemaphoreName,
        char* producerSemaphoreName, char* consumerSemaphoreName) {
    int current_cons = stats->consumers, current_prod = stats->producers;
    while(1){
        sem_wait(statsSem);
        if (stats->consumers == 0 && stats->producers) {
            sem_post(producerSem);
        }
        if(stats->consumers == 0 && stats->producers == 0) {
            sem_post(statsSem);
            break;
        }
        else{
            sem_post(statsSem);
        }
    }

    finishingMessage(stats);
    int semaphoreResult = endSemaphores(statsSem, producerSem, consumerSem, statsSemaphoreName, producerSemaphoreName,
                                        consumerSemaphoreName);
    if (semaphoreResult != 0) {
        printf("Unable to close the semaphores\n");
        return 1;
    }
    else {
        close(stats_fd);
        close(mem_fd);
        shm_unlink(bufferName);
        shm_unlink(structName);
        return 0;
    }

}