#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <ctype.h>

#define BITACORA "bitacora.txt"

// Cambiar estos si usas shmget con clave directa
#define SHM_KEY ftok("/tmp", 'M')
#define SEM_KEY ftok("/tmp", 'S')

void escribir_bitacora_cierre() {
    FILE *fp = fopen(BITACORA, "a");
    if (!fp) {
        perror("Error abriendo la bitácora");
        return;
    }

    time_t now = time(NULL);
    fprintf(fp, "==== Finalización de simulación (%s) ====\n", ctime(&now));
    fclose(fp);
}

int es_productor(const char *cmdline) {
    return strstr(cmdline, "productor") != NULL;
}

void matar_productores() {
    DIR *proc = opendir("/proc");
    if (!proc) {
        perror("No se pudo abrir /proc");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(proc)) != NULL) {
        if (!isdigit(entry->d_name[0])) continue;

        char path[256];
        snprintf(path, sizeof(path), "/proc/%s/cmdline", entry->d_name);

        FILE *cmdf = fopen(path, "r");
        if (!cmdf) continue;

        char cmdline[256];
        fread(cmdline, 1, sizeof(cmdline), cmdf);
        fclose(cmdf);

        if (es_productor(cmdline)) {
            pid_t pid = atoi(entry->d_name);
            printf("Matando proceso productor PID %d\n", pid);
            kill(pid, SIGTERM);  // o SIGKILL si es necesario
        }
    }

    closedir(proc);
}

int main() {
    // Matar procesos productores
    matar_productores();

    // Eliminar memoria compartida
    int shmid = shmget(SHM_KEY, 0, 0666);
    if (shmid != -1) {
        if (shmctl(shmid, IPC_RMID, NULL) == 0) {
            printf("Memoria compartida eliminada correctamente.\n");
        } else {
            perror("Error al eliminar la memoria compartida");
        }
    } else {
        printf("No se encontró memoria compartida.\n");
    }

    // Eliminar semáforo
    int semid = semget(SEM_KEY, 1, 0666);
    if (semid != -1) {
        if (semctl(semid, 0, IPC_RMID) == 0) {
            printf("Semáforo eliminado correctamente.\n");
        } else {
            perror("Error al eliminar el semáforo");
        }
    } else {
        printf("No se encontró el semáforo.\n");
    }

    // Escribir cierre en bitácora
    escribir_bitacora_cierre();

    printf("Finalización completa.\n");
    return 0;
}
