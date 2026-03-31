#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "core.h"
#include "gpib.h"

AppData g_appdata = {
    .current_mode       = IDLE_MENU,
    .shutdown_requested = 0,
    .mutex              = PTHREAD_MUTEX_INITIALIZER,
    .cond               = PTHREAD_COND_INITIALIZER,
};

int main(int argc, char **argv)
{
    int ret = 0;
    g_appdata.current_mode       = IDLE_MENU;
    g_appdata.shutdown_requested = FALSE;

    ret = pthread_create(&g_appdata.thread_manual,NULL,gpib_manual_mode,NULL);

    if(!ret){
        printf("Thread for manual mode has ben initied successfully.\n");
    } else {
        fprintf(stderr,"Tread for manual mode failed to initialize,\nrefere to <pthread.c> - pthread_create() return: %s",strerror(ret));
    }

    if (hmi_init(&argc, &argv) < 0) {
        fprintf(stderr, "Failed to run UI\n");
        //attention quite sans attendre les threads, pas de cleanup possible
        return EXIT_FAILURE;
    }


    pthread_join(g_appdata.thread_manual,NULL);
    //printf("Application terminée proprement\n");
    return EXIT_SUCCESS;

}

