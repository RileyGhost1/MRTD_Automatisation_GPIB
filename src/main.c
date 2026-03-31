#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "core.h"
#include "gpib.h"



int main(int argc, char **argv)
{
    AppData app;

    app.current_mode      = IDLE_MENU;
    app.shutdown_requested = FALSE;

    if (pthread_mutex_init(&app.mutex, NULL) != 0) {
        fprintf(stderr, "Echec init mutex\n");
        return EXIT_FAILURE;
    }

    if (pthread_cond_init(&app.cond, NULL) != 0) {
        fprintf(stderr, "Echec init cond\n");
        pthread_mutex_destroy(&app.mutex);
        return EXIT_FAILURE;
    }

    // Lancer le thread de polling AVANT gtk_main
    if (pthread_create(&app.thread_manual, NULL, gpib_manual_mode, &app) != 0) {
        fprintf(stderr, "Failed to start GPIB polling thread\n");
        pthread_cond_destroy(&app.cond);
        pthread_mutex_destroy(&app.mutex);
        return EXIT_FAILURE;
    }

    if (hmi_init(&argc, &argv) < 0) {
        fprintf(stderr, "Failed to run UI\n");
        //attention quite sans attendre les threads, pas de cleanup possible
        return EXIT_FAILURE;
    }

    pthread_mutex_lock(&app.mutex);
    app.shutdown_requested = TRUE;
    pthread_cond_signal(&app.cond);
    pthread_mutex_unlock(&app.mutex);

    pthread_join(app.thread_manual, NULL);
    pthread_cond_destroy(&app.cond);
    pthread_mutex_destroy(&app.mutex);
    
    return EXIT_SUCCESS;
}

