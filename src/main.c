#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <gpib/ib.h>
#include <sys/stat.h>  // stat(), mkdir(), struct stat
#include <unistd.h>    // readlink()
#include <libgen.h>    // dirname()
#include <stdio.h>     // perror(), printf()
#include <string.h>    // strncpy(), snprintf()
#include "core.h"
#include "gpib.h"

#define PROFILES_DIR "../profiles"

/* --- Définition du contexte global --- */
// On initialise ici la mémoire, mais on passera son adresse aux threads[cite: 15].
AppData g_appdata = {
    .service_gpib       = IDLE,
    .shutdown_requested = FALSE,
    .device_online      = FALSE,
    .mutex              = PTHREAD_MUTEX_INITIALIZER,
    .cond               = PTHREAD_COND_INITIALIZER,
    .gpib_queue         = NULL,
    .device_status      = {
        .ud = -1,
        .actual_dt = 0.0,
        .target_temp = 0.0,
        .emitter_temp = 0.0,
        .target_index = 0
    }
};

void init_profiles_dir(char *profiles_path, size_t size) {
    char exe_path[256] = {0};
    char exe_dir[256]  = {0};

    // Échec readlink → exe_path reste vide → chemin incorrect
    if (readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1) == -1) {
        perror("[INIT] readlink /proc/self/exe");
        strncpy(profiles_path, "../profiles", size - 1);  // Fallback
        return;
    }

    strncpy(exe_dir, dirname(exe_path), sizeof(exe_dir) - 1);
    snprintf(profiles_path, size, "%s/../profiles", exe_dir);

    struct stat st = {0};
    if (stat(profiles_path, &st) == -1) {
        if (mkdir(profiles_path, 0755) == -1) {
            perror("[INIT] mkdir profiles");
            // Le programme continue — opendir() échouera proprement ensuite
        } else {
            printf("[INIT] Dossier profiles créé : %s\n", profiles_path);
        }
    } else {
        printf("[INIT] Dossier profiles trouvé : %s\n", profiles_path);
    }
}

int main(int argc, char **argv)
{
    pthread_t th_handler, th_service_GPIB, th_service_MRTD;

    init_profiles_dir(g_appdata.profiles_path, sizeof(g_appdata.profiles_path));
    g_appdata.gpib_queue = g_async_queue_new();

    printf("Démarrage de l'application GPIB...\n");

    /* 1. Lancement du Thread 1 : Handler (Watchdog) */
    // On passe l'adresse de g_appdata pour que le thread soit autonome
    if (pthread_create(&th_handler, NULL, thread_handler_watchdog, &g_appdata) != 0) {
        fprintf(stderr, "Erreur: Impossible de créer le thread Handler\n");
        return EXIT_FAILURE;
    }

    /* 2. Lancement du Thread 2 : Service GPIB */
    // Ce thread gérera la machine à états et le polling
    if (pthread_create(&th_service_GPIB, NULL, thread_service_gpib, &g_appdata) != 0) {
        fprintf(stderr, "Erreur: Impossible de créer le thread Service\n");
        // Tentative de fermeture propre si le premier thread est lancé
        g_appdata.shutdown_requested = true;
        pthread_join(th_handler, NULL);
        return EXIT_FAILURE;
    }

    /* 3. Lancement du Thread 3 : Service MRTD */
    if (pthread_create(&th_service_MRTD, NULL, thread_service_MRTD, &g_appdata) != 0) {
        fprintf(stderr, "Erreur: Impossible de créer le thread Service\n");
        // Tentative de fermeture propre si le premier thread est lancé
        g_appdata.shutdown_requested = true;
        pthread_join(th_handler, NULL);
        return EXIT_FAILURE;
    }

    /* 3. Lancement de l'Interface Graphique (BLOQUANT) */
    // GTK prend la main sur le thread principal. 
    // Le callback du bouton shutdown de l'HMI devra passer g_appdata.shutdown_requested à true
    if (hmi_init(&argc, &argv, &g_appdata) < 0) {
        fprintf(stderr, "Erreur fatale: Échec de l'interface graphique\n");
    }

    /* 4. Phase de fermeture (Cleanup) */
    // Une fois que hmi_init (gtk_main) se termine :
    printf("Arrêt des services en cours...\n");
    
    // On s'assure que le flag est levé pour les threads 
    pthread_mutex_lock(&g_appdata.mutex);
    g_appdata.shutdown_requested = true;
    pthread_cond_broadcast(&g_appdata.cond);
    pthread_mutex_unlock(&g_appdata.mutex);

    // Attente de la fin des threads pour un retour propre au système 
    pthread_join(th_handler, NULL);
    pthread_join(th_service_GPIB, NULL);
    pthread_join(th_service_MRTD, NULL);


    // Libération finale des ressources matérielles
    if (g_appdata.device_status.ud != -1) {
        ibonl(g_appdata.device_status.ud, 0);
    }

    printf("Application terminée proprement.\n");
    return EXIT_SUCCESS;
}