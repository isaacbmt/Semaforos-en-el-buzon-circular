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
#define RESET "\033[0m"

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
void copy_chars_in_index(char *dest, const char* source, const int *index, int length);
int read_in_buffer(int *map, struct Stats* stats, int processID);
int openSemaphores (const sem_t* statsSem, const sem_t* producerSem, const sem_t* consumerSem, char* statsSemaphoreName,
                    char* producerSemaphoreName, char* consumerSemaphoreName);
int run_process(mode chosen_mode, int *map, struct Stats* stats,
                sem_t* statsSem, sem_t* producerSem, sem_t* consumerSem);


int main(__attribute__((unused)) int argc, char** argv) {
    int stats_size = sizeof (struct Stats);
    int length;
    int time_ra = 2;
    int processID = getpid();
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
        printf("Error: No se especifico el modo del consumidor");
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
    stats->consumers += 1;
    sem_post(statsSem);
    int mem_fd = create_file_descriptor(bufferName);
    int *mem_map = create_shared_memory(mem_fd, length);


    return run_process(chosen_mode, mem_map, stats, statsSem, producerSem, consumerSem, processID);
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

int openSemaphores (const sem_t* statsSem, const sem_t* producerSem, const sem_t* consumerSem, char* statsSemaphoreName,
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
    else{
        return 0;
    }
}

int read_in_buffer(int *map, struct Stats* stats, int processID) {

    char message[stats->buffer_size];

    copy_chars_in_index(message, (char *) map, map + stats->start_read * stats->buffer_size, stats->buffer_size);

    printf(RED);
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    printf(RED);
    printf("              Se leyo un mensaje en el buffer              \n");
    printf(GREEN);
    printf("Mensaje: ");
    printf(RESET);
    printf("%s\n", message);
    printf(GREEN);
    printf("Numero de bloque: ");
    printf(RESET);
    printf("%d\n", stats->start_read);
    printf(GREEN);
    printf("Cantidad de consumidores: ");
    printf(RESET);
    printf("%d\n", stats->consumers);
    printf(GREEN);
    printf("Numero de productores: ");
    printf(RESET);
    printf("%d\n", stats->producers);
    printf(BLACK);
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    printf(RESET);

    char* ptr1;
    int MAGICNUMBER = (int) strtol(&message[16], &ptr1, 10);

    if (strcmp(message, "/STOP") == 0) {
        return 1;
    }
    else if (processID % 6 == MAGICNUMBER) {
        return 2;
    }
    else{
        return 0;
    }
}

void copy_chars_in_index(char *dest, const char* source, const int *index, int length) {
    int k = 0;
    for (uintptr_t i = (uintptr_t) index; k <= length-1; ++i) {
        dest[k] = source[i];
        k++;
    }
}

int run_process(mode chosen_mode, int *map, struct Stats* stats, sem_t* statsSem, sem_t* producerSem,
        sem_t* consumerSem, int processID) {

    char message[10];
    int exitCode;

    while (true) {

        sem_wait(consumerSem);
        sem_wait(statsSem);

        switch (chosen_mode) {
            case automatic:
                printf("Automatico");
                break;
            case manual:
                printf("Presione enter para leer un mensaje: ");
                fgets(message, 10, stdin);
                break;
            default:
                break;
        }

        // Read message in the buffer
        exitCode = read_in_buffer(map, stats, processID);

        if(exitCode == 1 || exitCode == 2){
            stats->consumers -= 1;
            printFinalMessage(exitCode, processID, stats);
            sem_post(statsSem);
            break;
        }

        sem_post(producerSem);
        // Aquí iría el SLEEPPPPPPPPPPPPPPPPP
        sem_post(statsSem);

    }
    return 0;
}

void printFinalMessage(int exitCode, int processID, struct Stats* stats) {

    printf("Se escriben todos los datos finales\n");
}
