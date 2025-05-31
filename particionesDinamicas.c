#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

#define MEMORY_SIZE 100  // Tamaño predeterminado de la memoria

typedef struct {
    int size;
    int partitions[100];
    char process_ids[100][20];
} shared_memory;

// Variables globales
shared_memory *mem;
int shmid, semid;
FILE *log_file;

// Función para crear/obtener semáforo
int get_semaphore() {
    key_t key = ftok("/tmp", 'S');
    int sem_id = semget(key, 1, 0666 | IPC_CREAT);
    
    if (sem_id == -1) {
        perror("semget failed");
        exit(1);
    }
    
    // Inicializar semáforo solo si lo acabamos de crear
    if (semctl(sem_id, 0, GETVAL) == 0) {
        union semun {
            int val;
            struct semid_ds *buf;
            unsigned short *array;
        } arg;
        
        arg.val = 1;
        if (semctl(sem_id, 0, SETVAL, arg) == -1) {
            perror("semctl SETVAL failed");
            exit(1);
        }
    }
    
    return sem_id;
}

// Función para crear/obtener memoria compartida
int get_shared_memory() {
    key_t key = ftok("/tmp", 'M');
    int shm_id = shmget(key, sizeof(shared_memory), 0666 | IPC_CREAT);
    
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }
    
    // Adjuntar memoria
    shared_memory *shm = shmat(shm_id, NULL, 0);
    if (shm == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }
    
    // Inicializar memoria si es nueva
    if (shm->size == 0) {
        shm->size = MEMORY_SIZE;
        for (int i = 0; i < MEMORY_SIZE; i++) {
            shm->partitions[i] = 0;
            shm->process_ids[i][0] = '\0';
        }
        printf("Memoria inicializada con tamaño %d\n", MEMORY_SIZE);
    }
    
    return shm_id;
}

void sem_op(int semid, int sem_num, int op) {
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = op;
    sb.sem_flg = 0;
    semop(semid, &sb, 1);
}

void log_action(const char *pid, const char *action, const char *type, int lines) {
    time_t now;
    time(&now);
    fprintf(log_file, "[%s] PID: %s, Action: %s, Type: %s, Lines: %d\n", 
            ctime(&now), pid, action, type, lines);
    fflush(log_file);
}

void *process_thread(void *arg) {
    char pid[20];
    sprintf(pid, "%ld", (long)pthread_self());
    
    int size = (rand() % 10) + 1;
    int duration = (rand() % 41) + 20;
    
    sem_op(semid, 0, -1);
    log_action(pid, "Attempting to allocate memory", "allocation", size);
    
    // Algoritmo First-Fit
    int start = -1;
    for (int i = 0; i < mem->size; i++) {
        if (mem->partitions[i] == 0) {
            int j, free_space = 0;
            for (j = i; j < mem->size && mem->partitions[j] == 0 && free_space < size; j++) {
                free_space++;
            }
            if (free_space >= size) {
                start = i;
                break;
            }
        }
    }
    
    if (start != -1) {
        for (int i = start; i < start + size; i++) {
            mem->partitions[i] = 1;
            strcpy(mem->process_ids[i], pid);
        }
        
        log_action(pid, "Memory allocated successfully", "allocation", size);
        sem_op(semid, 0, 1);
        
        sleep(duration);
        
        sem_op(semid, 0, -1);
        for (int i = 0; i < mem->size; i++) {
            if (strcmp(mem->process_ids[i], pid) == 0) {
                mem->partitions[i] = 0;
                mem->process_ids[i][0] = '\0';
            }
        }
        
        log_action(pid, "Memory freed", "deallocation", size);
        sem_op(semid, 0, 1);
    } else {
        log_action(pid, "Failed to allocate memory - no space available", "allocation", size);
        sem_op(semid, 0, 1);
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <algoritmo: 1=First-Fit, 2=Best-Fit, 3=Worst-Fit>\n", argv[0]);
        return 1;
    }
    
    srand(time(NULL));
    
    // Configurar archivo de log
    log_file = fopen("memory_log.txt", "a");
    if (!log_file) {
        perror("Error al abrir archivo de bitácora");
        return 1;
    }
    
    // Crear/obtener recursos IPC
    shmid = get_shared_memory();
    semid = get_semaphore();
    
    // Adjuntar memoria compartida
    mem = shmat(shmid, NULL, 0);
    if (mem == (void *)-1) {
        perror("shmat failed");
        return 1;
    }
    
    printf("Productor iniciado. Memoria disponible: %d particiones\n", mem->size);
    
    // Bucle principal
    while (1) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, process_thread, NULL) != 0) {
            perror("Error al crear hilo");
            continue;
        }
        pthread_detach(thread);
        
        int wait_time = (rand() % 31) + 30;
        sleep(wait_time);
    }
    
    // Limpieza (en teoría nunca se ejecuta)
    shmdt(mem);
    fclose(log_file);
    return 0;
}