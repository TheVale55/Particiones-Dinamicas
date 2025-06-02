#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

#define SHM_KEY 0x1234

typedef struct {
    int ocupada;
    pid_t pid;
} LineaMemoria;

int contiene(pid_t *lista, int count, pid_t pid) {
    for (int i = 0; i < count; i++) {
        if (lista[i] == pid) return 1;
    }
    return 0;
}

void imprimir_estado_memoria(LineaMemoria *memoria, int total_lineas) {
    printf("===== ESTADO DE LA MEMORIA =====\n");
    for (int i = 0; i < total_lineas; i++) {
        printf("Línea %3d: %s", i, memoria[i].ocupada ? "OCUPADA" : "LIBRE  ");
        if (memoria[i].ocupada)
            printf(" | PID: %d", memoria[i].pid);
        printf("\n");
    }
    printf("=================================\n\n");
}

void imprimir_estado_procesos(LineaMemoria *memoria, int total_lineas) {
    pid_t procesos[1024];
    int total = 0;

    // Recoger todos los PIDs únicos que están en la memoria ocupada
    for (int i = 0; i < total_lineas; i++) {
        if (memoria[i].ocupada && !contiene(procesos, total, memoria[i].pid)) {
            procesos[total++] = memoria[i].pid;
        }
    }

    pid_t ejecutando[1024], bloqueado[1024], finalizado[1024];
    int ej = 0, bl = 0, fn = 0;

    for (int i = 0; i < total; i++) {
        char ruta[256];
        snprintf(ruta, sizeof(ruta), "/proc/%d/status", procesos[i]);
        FILE *fp = fopen(ruta, "r");

        if (!fp) {
            finalizado[fn++] = procesos[i];
            continue;
        }

        char linea[256];
        char estado = '?';
        while (fgets(linea, sizeof(linea), fp)) {
            if (strncmp(linea, "State:", 6) == 0) {
                sscanf(linea, "State:\t%c", &estado);
                break;
            }
        }
        fclose(fp);

        switch (estado) {
            case 'R':
                ejecutando[ej++] = procesos[i];
                break;
            case 'S':
            case 'D':
                bloqueado[bl++] = procesos[i];
                break;
            default:
                finalizado[fn++] = procesos[i];
                break;
        }
    }

    printf("===== ESTADO DE LOS PROCESOS =====\n");

    printf("Procesos con acceso a memoria:\n");
    for (int i = 0; i < total; i++) {
        printf(" - PID %d\n", procesos[i]);
    }

    printf("\nProcesos ejecutando:\n");
    for (int i = 0; i < ej; i++) {
        printf(" - PID %d\n", ejecutando[i]);
    }

    printf("\nProcesos bloqueados:\n");
    for (int i = 0; i < bl; i++) {
        printf(" - PID %d\n", bloqueado[i]);
    }

    printf("\nProcesos finalizados o inexistentes:\n");
    for (int i = 0; i < fn; i++) {
        printf(" - PID %d\n", finalizado[i]);
    }

    printf("=====================================\n");
}

int main() {
    // Conectarse a la memoria compartida ya existente
    int shmid = shmget(SHM_KEY, 0, 0666);
    if (shmid == -1) {
        perror("Error obteniendo memoria compartida");
        exit(1);
    }

    // Obtener el tamaño real del segmento
    struct shmid_ds shm_info;
    if (shmctl(shmid, IPC_STAT, &shm_info) == -1) {
        perror("Error obteniendo información del segmento");
        exit(1);
    }

    int total_lineas = shm_info.shm_segsz / sizeof(LineaMemoria);

    LineaMemoria *memoria = (LineaMemoria *) shmat(shmid, NULL, 0);
    if (memoria == (void *) -1) {
        perror("Error al conectar memoria compartida");
        exit(1);
    }

    // Mostrar estado de memoria y procesos
    imprimir_estado_memoria(memoria, total_lineas);
    imprimir_estado_procesos(memoria, total_lineas);

    shmdt(memoria);
    return 0;
}


