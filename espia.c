#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>

#define MEM_SIZE 100 // Simulación de 100 líneas de memoria
#define SHM_KEY 1234
#define SEM_KEY 5678

// Estructura para cada línea de memoria
typedef struct {
    int ocupada;        
    pid_t pid;         
} LineaMemoria;

// Simula llenado aleatorio de la memoria
void simular_memoria(LineaMemoria *memoria) {
    srand(time(NULL));  // Semilla para aleatoriedad
    for (int i = 0; i < MEM_SIZE; i++) {
        memoria[i].ocupada = rand() % 2;  // 0 o 1
        memoria[i].pid = memoria[i].ocupada ? (rand() % 5000 + 1000) : 0;
    }
}

void imprimir_estado_memoria(LineaMemoria *memoria) {
    printf("===== ESTADO DE MEMORIA =====\n");
    for (int i = 0; i < MEM_SIZE; i++) {
        printf("Línea %3d: %s", i, memoria[i].ocupada ? "OCUPADA" : "LIBRE  ");
        if (memoria[i].ocupada)
            printf(" | PID: %d", memoria[i].pid);
        printf("\n");
    }
    printf("=============================\n");
}

int main() {
    /*int shmid;
    LineaMemoria *memoria;

    // Obtener la memoria compartida
    shmid = shmget(SHM_KEY, sizeof(LineaMemoria) * MEM_SIZE, 0666);
    if (shmid == -1) {
        perror("Error al obtener memoria compartida");
        exit(1);
    }

    memoria = (LineaMemoria *) shmat(shmid, NULL, 0);
    if (memoria == (void *) -1) {
        perror("Error al conectar memoria compartida");
        exit(1);
    }*/

    LineaMemoria memoria[MEM_SIZE];

    // Llenar con valores aleatorios para simular ocupación de memoria
    simular_memoria(memoria);

    // Mostrar estado de la memoria
    imprimir_estado_memoria(memoria);

    /*/ Desconectar memoria
    shmdt(memoria);*/

    return 0;
}
