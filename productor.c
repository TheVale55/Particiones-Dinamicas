#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

typedef struct {
    int ocupado;
    pid_t pid;
} LineaMemoria;

// Estructura para información compartida sobre los algoritmos
typedef struct {
    int algoritmo; // 1=First-Fit, 2=Best-Fit, 3=Worst-Fit
} Configuracion;

// Operaciones sobre semáforos
void wait_sem(int semid) {
    struct sembuf op = {0, -1, 0};
    semop(semid, &op, 1);
}

void signal_sem(int semid) {
    struct sembuf op = {0, 1, 0};
    semop(semid, &op, 1);
}

// Escribir en la bitácora
void escribir_bitacora(const char *mensaje) {
    FILE *fp = fopen("bitacora.txt", "a");
    if (fp) {
        time_t now = time(NULL);
        fprintf(fp, "[%ld] %s\n", (long)now, mensaje);
        fclose(fp);
    }
}

// Función que ejecutará cada proceso hijo
void proceso_hijo(LineaMemoria *memoria, int semid, int tamano, int tiempo, int algoritmo, int total_lineas) {
    char msg[256];
    pid_t mi_pid = getpid();
    
    sprintf(msg, "PROCESO %d: Iniciando (necesita %d líneas)", mi_pid, tamano);
    escribir_bitacora(msg);
    
    wait_sem(semid); // Entrar a región crítica
    
    int posicion = -1;
    int hueco_actual = 0;
    int mejor_hueco = -1;
    int peor_hueco = -1;
    int pos_mejor = -1;
    int pos_peor = -1;
    
    // Buscar espacio según el algoritmo seleccionado
    for (int i = 0; i < total_lineas; i++) {
        if (!memoria[i].ocupado) {
            hueco_actual++;
            if (hueco_actual >= tamano) {
                // First-Fit: tomar el primero que encuentre
                if (algoritmo == 1) {
                    posicion = i - tamano + 1;
                    break;
                }
                // Actualizar Best-Fit y Worst-Fit
                if (algoritmo == 2 && (mejor_hueco == -1 || hueco_actual < mejor_hueco)) {
                    mejor_hueco = hueco_actual;
                    pos_mejor = i - tamano + 1;
                }
                if (algoritmo == 3 && (peor_hueco == -1 || hueco_actual > peor_hueco)) {
                    peor_hueco = hueco_actual;
                    pos_peor = i - tamano + 1;
                }
            }
        } else {
            hueco_actual = 0;
        }
    }
    
    // Para Best-Fit y Worst-Fit
    if (algoritmo == 2 && pos_mejor != -1) posicion = pos_mejor;
    if (algoritmo == 3 && pos_peor != -1) posicion = pos_peor;
    
    if (posicion != -1) {
        // Asignar memoria
        for (int i = posicion; i < posicion + tamano; i++) {
            memoria[i].ocupado = 1;
            memoria[i].pid = mi_pid;
        }
        
        sprintf(msg, "PROCESO %d: Asignadas líneas %d-%d", mi_pid, posicion, posicion + tamano - 1);
        escribir_bitacora(msg);
        
        signal_sem(semid); // Salir de región crítica
        
        // Simular tiempo de ejecución
        sleep(tiempo);
        
        wait_sem(semid); // Entrar a región crítica para liberar
        
        // Liberar memoria
        for (int i = posicion; i < posicion + tamano; i++) {
            if (memoria[i].pid == mi_pid) {
                memoria[i].ocupado = 0;
                memoria[i].pid = -1;
            }
        }
        
        sprintf(msg, "PROCESO %d: Liberadas líneas %d-%d", mi_pid, posicion, posicion + tamano - 1);
        escribir_bitacora(msg);
    } else {
        sprintf(msg, "PROCESO %d: No encontró espacio (%d líneas necesarias)", mi_pid, tamano);
        escribir_bitacora(msg);
    }
    
    signal_sem(semid); // Salir de región crítica
    exit(0);
}

int main() {
    int shmid, semid, conf_shmid;
    LineaMemoria *memoria;
    Configuracion *config;
    int total_lineas;
    
    // Obtener memoria compartida
    shmid = shmget(SHM_KEY, 0, 0666);
    if (shmid == -1) {
        perror("Error obteniendo memoria compartida");
        exit(1);
    }
    
    memoria = (LineaMemoria *)shmat(shmid, NULL, 0);
    if (memoria == (void *)-1) {
        perror("Error conectando a memoria compartida");
        exit(1);
    }
    
    // Obtener semáforo
    semid = semget(SEM_KEY, 0, 0666);
    if (semid == -1) {
        perror("Error obteniendo semáforo");
        exit(1);
    }
    
    // Obtener número total de líneas
    struct shmid_ds shm_info;
    shmctl(shmid, IPC_STAT, &shm_info);
    total_lineas = shm_info.shm_segsz / sizeof(LineaMemoria);
    
    // Seleccionar algoritmo
    printf("Seleccione algoritmo de asignación:\n");
    printf("1. First-Fit\n");
    printf("2. Best-Fit\n");
    printf("3. Worst-Fit\n");
    printf("Opción: ");
    
    int algoritmo;
    scanf("%d", &algoritmo);
    if (algoritmo < 1 || algoritmo > 3) {
        printf("Opción inválida\n");
        exit(1);
    }
    
    char algo_str[20];
    switch(algoritmo) {
        case 1: strcpy(algo_str, "First-Fit"); break;
        case 2: strcpy(algo_str, "Best-Fit"); break;
        case 3: strcpy(algo_str, "Worst-Fit"); break;
    }
    
    char msg[256];
    sprintf(msg, "PRODUCTOR: Iniciando con algoritmo %s", algo_str);
    escribir_bitacora(msg);
    
    // Generar procesos continuamente
    srand(time(NULL));
    while (1) {
        int tamano = rand() % 10 + 1; // 1-10 líneas
        int tiempo = rand() % 41 + 20; // 20-60 segundos
        
        pid_t pid = fork();
        if (pid == 0) { // Proceso hijo
            proceso_hijo(memoria, semid, tamano, tiempo, algoritmo, total_lineas);
        } else if (pid > 0) { // Proceso padre
            sprintf(msg, "PRODUCTOR: Creado proceso %d (tamaño=%d, tiempo=%ds)", pid, tamano, tiempo);
            escribir_bitacora(msg);
        } else {
            perror("Error al crear proceso");
        }
        
        // Esperar un tiempo aleatorio antes de crear el siguiente proceso
        sleep(rand() % 15 + 5); // 31 + 30 30-60 segundos
    }
    
    shmdt(memoria);
    return 0;
}