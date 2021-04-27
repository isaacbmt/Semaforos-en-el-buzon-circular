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

#define RED "\033[1;31m"
#define BLACK "\033[1;30m"
#define GREEN "\033[1;32m"
#define BLUE "\033[1;34m"
#define RESET "\033[0m"

typedef enum { automatic, manual, no_specified } mode;

struct Times {
    int messages_counter;
    double wait_time;
    double block_time;
    double user_time;
    double kernel_time;
};

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
void copy_chars_in_index(char *dest, const char* source, const int *index, int length);
void write_in_buffer(char *source, int *map, struct Stats* stats);
int openSemaphores (const sem_t* statsSem, const sem_t* producerSem, const sem_t* consumerSem, char* statsSemaphoreName,
                    char* producerSemaphoreName, char* consumerSemaphoreName);
int run_process(mode chosen_mode, int *map, struct Stats* stats,
        sem_t* statsSem, sem_t* producerSem, sem_t* consumerSem);


int main(__attribute__((unused)) int argc, char** argv) {
    int stats_size = sizeof (struct Stats);
    int length;
    int time_ra = 2;
    char bufferName[256];
    char structName[256];
    char statsSemaphoreName[256];
    char producerSemaphoreName[256];
    char consumerSemaphoreName[256];
    sem_t *statsSem = NULL;
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
    srand(time(0));
    mode chosen_mode = strcmp(argv[2], "auto") == 0? automatic : strcmp(argv[2], "manual") == 0? manual : no_specified;

    if (chosen_mode == no_specified) {
        printf("Error: No se especifico el modo del productor");
        return 1;
    }

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
    sem_wait(statsSem);
    length = stats->buffer_size * stats->block_size;
    sem_post(statsSem);
    int mem_fd = create_file_descriptor(bufferName);
    int *mem_map = create_shared_memory(mem_fd, length);


    return run_process(chosen_mode, mem_map, stats, statsSem, producerSem, consumerSem);
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

int openSemaphores (char* statsSemaphoreName,
                    char* producerSemaphoreName, char* consumerSemaphoreName) {

    producerSem = sem_open(producerSemaphoreName, 0);

    if (producerSem == SEM_FAILED) {
        perror("Opening the producer semaphore failed\n");
        return 1;
    }

    consumerSem  = sem_open(consumerSemaphoreName, 0);

    if(consumerSem == SEM_FAILED) {
        perror("Opening the consumer semaphore failed\n");
        return 1;
    }

    statsSem = sem_open(statsSemaphoreName, 0);

    if (statsSem == SEM_FAILED) {
        perror("Opening the stats semaphore failed\n");
        return 1;
    }

    return 0;
}

void write_in_buffer(char *source, int *map, struct Stats* stats) {
    char message[stats->buffer_size];
    time_t raw_time;
    time(&raw_time);
    int magic_number = rand() % 7;

    stats->messages_counter++;
    times.messages_counter++;
    struct tm *info = localtime(&raw_time);

    if (stats->finish == 1 && stats->consumers != 0) {
        sprintf(buffer_block, "/STOP");
        strcpy(source, "/STOP");
    } else {
        sprintf(buffer_block, "{Número mágico: %d, Productor id: %d, Mensaje: %s, Fecha: %s}",
                magic_number, getpid(), source, asctime(info));
    }
    copy_chars_in_index((char *) map, message, map + stats->start_write * stats->buffer_size, stats->buffer_size);

    printf(BLACK);
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    printf(RED);
    printf("         Un mensaje ingreso un mensaje en el buffer        \n");
    printf(GREEN);
    printf("Mensaje: ");
    printf(RESET);
    printf("%s\n", source);
    printf(GREEN);
    printf("Numero de bloque: ");
    printf(RESET);
    printf("%d\n", stats->start_write);
    printf(GREEN);
    printf("Cantidad de consumidores: ");
    printf(RESET);
    printf("%d\n", stats->consumers);
    printf(GREEN);
    printf("Numero de productores: ");
    printf(RESET);
    printf("%d\n", stats->producers);
    printf(RED);
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n\n");
    printf(RESET);
}

void copy_chars_in_index(char *dest, const char* source, const int *index, int length) {
    int k = 0;
    for (uintptr_t i = (uintptr_t) index; k <= length-1; ++i) {
        dest[i] = source[k];
        k++;
    }
}

int run_process(mode chosen_mode, int *map, struct Stats* stats, sem_t* statsSem, sem_t* producerSem,
        sem_t* consumerSem) {
    int olo = false;
    char message[stats->buffer_size];
    while (true) {
        if (olo == true) {
            break;
        }
        switch (chosen_mode) {
            case automatic:
                sprintf(message, "Hola soy un mensaje autogenerado y mi numero favorito es el %d", rand()%30);
                break;
            case manual:
                printf("Escribe un mensaje: ");
                fgets(message, stats->buffer_size + 1, stdin);
                message[strcspn(message, "\n")] = 0;
                break;
            default:
                break;
        }

        // Escribe el mensaje en el buffer
        write_in_buffer(message, map, stats);
        olo = true;
    }
    return 0;
}
