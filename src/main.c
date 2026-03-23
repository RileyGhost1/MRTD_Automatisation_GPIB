#include <stdlib.h>
#include <stdio.h>
#include "ui.h"
#include "gpib.h"

// plus tard: #include "gpib_ctrl.h"

int main(int argc, char **argv)
{

        if (gpib_init(1) < 0) {
            fprintf(stderr, "Failed to initialize GPIB\n");
            return EXIT_FAILURE;
        } else {
            printf("connection établie");
        }
    
        while(1){printf("connection établie");}
    

        if (hmi_init(&argc, &argv) < 0) {
            fprintf(stderr, "Failed to run UI\n");
            return EXIT_FAILURE;
        }
    
    #ifdef PLATFORM_LINUX
        gpib_close();
    #endif
        return EXIT_SUCCESS;

    return 0;
}

