#include <stdlib.h>
#include <stdio.h>
#include "ui.h"
#include "gpib.h"

// plus tard: #include "gpib_ctrl.h"

int main(int argc, char **argv)
{
    AppContext ctx = {0};

    if (hmi_init(&argc, &argv, &ctx) < 0) {
        fprintf(stderr, "Failed to run UI\n");
        return EXIT_FAILURE;
    }
    

    gpib_close();
    return EXIT_SUCCESS;

}

