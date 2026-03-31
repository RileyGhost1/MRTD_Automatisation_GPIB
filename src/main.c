#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "core.h"
#include "gpib.h"



int main(int argc, char **argv)
{
    AppData app;

    app.current_mode       = IDLE_MENU;
    app.shutdown_requested = FALSE;

    if (hmi_init(&argc, &argv) < 0) {
        fprintf(stderr, "Failed to run UI\n");
        //attention quite sans attendre les threads, pas de cleanup possible
        return EXIT_FAILURE;
    }

    //printf("Application terminée proprement\n");
    return EXIT_SUCCESS;
}

