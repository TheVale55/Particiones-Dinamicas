#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#define SHM_KEY 0x1234       // Clave para memoria compartida
#define SEM_KEY 0x5678       // Clave para semáforo
#define MAX_LINEAS 1024      // Límite arbitrario para evitar uso excesivo

// Estructura de cada línea de memoria
typedef struct {
    int ocupado;     // 0 = libre, 1 = ocupado
    pid_t pid;       // PID del proceso que ocupa la línea (si aplica)
} LineaMemoria;

void crear_bitacora() {
    FILE *fp = fopen("bitacora.txt", "a");
    if (fp == NULL) {
        perror("No se pudo crear la bitácora");
        exit(EXIT_FAILURE);
    }
    time_t now = time(NULL);
    fprintf(fp, "==== Inicio de simulación (%s) ====\n", ctime(&now));
    fclose(fp);
}

int main() {
    int cantidad_lineas;
    printf("Ingrese la cantidad de líneas de memoria a simular: ");
    scanf("%d", &cantidad_lineas);

    if (cantidad_lineas <= 0 || cantidad_lineas > MAX_LINEAS) {
        printf("Cantidad inválida.\n");
        exit(EXIT_FAILURE);
    }

    // Crear memoria compartida
    int shmid = shmget(SHM_KEY, sizeof(LineaMemoria) * cantidad_lineas, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Error creando memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Conectar a la memoria compartida para inicializar
    LineaMemoria *memoria = (LineaMemoria *)shmat(shmid, NULL, 0);
    if (memoria == (void *)-1) {
        perror("Error conectando a la memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Inicializar la memoria como vacía
    for (int i = 0; i < cantidad_lineas; i++) {
        memoria[i].ocupado = 0;
        memoria[i].pid = -1;
    }

    // Desconectar memoria compartida
    shmdt(memoria);

    // Crear semáforo para exclusión mutua en el uso de la memoria
    int semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("Error creando semáforo");
        exit(EXIT_FAILURE);
    }

    // Inicializar semáforo a 1 (libre)
    if (semctl(semid, 0, SETVAL, 1) == -1) {
        perror("Error inicializando semáforo");
        exit(EXIT_FAILURE);
    }

    // Crear archivo de bitácora
    crear_bitacora();

    printf("Inicialización completa. Recursos creados:\n");
    printf(" - Memoria compartida con %d líneas\n", cantidad_lineas);
    printf(" - Semáforo de control de memoria\n");
    printf(" - Bitácora inicializada\n");

    return 0;
}

