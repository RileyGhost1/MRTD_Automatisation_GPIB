#include "core.h"
#include "gpib.h"

void *gpib_manual_mode(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&g_appdata.mutex);

        while (g_appdata.current_mode != MANUAL_MODE && !g_appdata.shutdown_requested) 
            pthread_cond_wait(&g_appdata.cond, &g_appdata.mutex); // <-- Le thread ici attend tant que mode != MANUAL_MODE

        if (g_appdata.shutdown_requested) {
            pthread_mutex_unlock(&g_appdata.mutex);
            break;  // sortie propre
        }

        pthread_mutex_unlock(&g_appdata.mutex);  // ← libère AVANT la tâche, sinon retour au IDLE_MENU impossible

        // Polling GPIB, contrôle corps noir manuel
        // (pas besoin du mutex ici si tu lis seulement des périphériques)

        // Re-vérifie le mode si besoin
        int ret = pthread_mutex_lock(&g_appdata.mutex);
        if (ret != 0) {
            fprintf(stderr, "[GPIB] Failed to lock mutex: %s\n", strerror(ret));
            usleep(100000);
            continue;  // ← repart au début du while(1) sans lire les variables
        }

        int still_manual = (g_appdata.current_mode == MANUAL_MODE);
        int device_ok    = (g_controller.state == MASTER_ONLINE_DEVICE_ONLINE);
        pthread_mutex_unlock(&g_appdata.mutex);

        if (still_manual && device_ok)
        {
            if (gpib_read_all() < 0) {
                fprintf(stderr, "Error reading GPIB in manual mode\n");
                pthread_mutex_lock(&g_appdata.mutex);
                g_controller.state = MASTER_OFFLINE_DEVICE_OFFLINE;
                pthread_mutex_unlock(&g_appdata.mutex);
            }
        }

        usleep(100000); /* 100ms — réactif sans surcharger le bus GPIB */
    }
    return NULL;
}