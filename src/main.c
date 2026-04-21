#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <gpib/ib.h>
#include "core.h"
#include "gpib.h"

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

int main(int argc, char **argv)
{
    pthread_t th_handler, th_service;

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
    if (pthread_create(&th_service, NULL, thread_service_gpib, &g_appdata) != 0) {
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
    pthread_join(th_service, NULL);

    // Libération finale des ressources matérielles
    if (g_appdata.device_status.ud != -1) {
        ibonl(g_appdata.device_status.ud, 0);
    }

    printf("Application terminée proprement.\n");
    return EXIT_SUCCESS;
}