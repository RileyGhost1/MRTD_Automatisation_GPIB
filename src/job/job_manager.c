#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <unistd.h> 
#include "gpib.h"
#include "core.h"

/*======================================================================================================*/

/*
Apres plusieur iterations d'architectures possibles, j'ai choisis de serparer le projet en utilisant plusieur threads,
la communication gpib se fera dans un thread et la vision dans un second thread.
J'ai separer la logique du thread du fichier gpib_ctrl.c pour que mon programme soit reutilisable facilement,
si l'on doit un jour passer d'un SR80 avec gpib vers un SR80 avec un bus uart (RS232 present sur les nouveaux modeles),
il suffira de remplacer les fonctions de controle gpib par des fonctions de controle uart,
sans toucher a la logique du thread de polling ni a l'interface graphique. 
*/
/*======================================================================================================*/




/*======================================================================================================*/
/*
Thread de polling qui gere le grab du mutex, l'attente de la condition avec pthread_cond_wait,
appele la fonction gpib_read_all() pour realiser la transition de donnee entre les deux structures local -> global.
*/

void *thread_gpib_polling(void *arg)
{
    int ret;        


    while (1)
    {
        ret = pthread_mutex_lock(&g_appdata.mutex);
        if (ret != 0) {
            fprintf(stderr, "[GPIB] Failed to lock mutex: %s\n", strerror(ret));
            usleep(100000);
            continue;  // ← repart au début du while(1)
        }

        while ((g_appdata.gpib_polling == FALSE)||(g_controller.state != MASTER_ONLINE_DEVICE_ONLINE) && !g_appdata.shutdown_requested) 
            pthread_cond_wait(&g_appdata.cond, &g_appdata.mutex); // <-- Le thread attend ici tant que la tache n'est pas appelée

        if (g_appdata.shutdown_requested) {
            g_appdata.gpib_polling = FALSE;
            pthread_mutex_unlock(&g_appdata.mutex);
            break;  // sortie propre
        }

        //gpib_read_all();

        pthread_mutex_unlock(&g_appdata.mutex);

        usleep(100000); /* 100ms — réactif sans surcharger le bus GPIB */
    }
    return NULL;
}