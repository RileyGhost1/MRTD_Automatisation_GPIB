#include "core.h"
#include "gpib.h"

void *gpib_manual_mode(void *arg)
{
    AppData *app = (AppData *)arg;

    while (1)
    {
        /* --- Attente activation mode manuel --- */
        pthread_mutex_lock(&app->mutex);

        while (app->current_mode != MANUAL_MODE && !app->shutdown_requested)
            pthread_cond_wait(&app->cond, &app->mutex);

        if (app->shutdown_requested)
        {
            pthread_mutex_unlock(&app->mutex);
            break;
        }

        pthread_mutex_unlock(&app->mutex);

        /* --- Travail du thread --- */
        printf("Thread manuel actif — polling GPIB...\n");

        /* --- Vérifier si on doit s'arrêter ou changer de mode --- */
        pthread_mutex_lock(&app->mutex);
        if (app->current_mode != MANUAL_MODE || app->shutdown_requested)
        {
            pthread_mutex_unlock(&app->mutex);
            continue;   /* retourne en haut du while(1) → se rendort */
        }
        pthread_mutex_unlock(&app->mutex);

        sleep(1);   /* rythme de polling pour le test */
    }

    return NULL;
}