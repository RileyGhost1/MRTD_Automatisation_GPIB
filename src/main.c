#include "ui.h"
// plus tard: #include "gpib_ctrl.h"

int main(int argc, char **argv)
{
    if (gpib_init(1) < 0) {
        fprintf(stderr, "Failed to initialize GPIB\n");
        return EXIT_FAILURE;
    }

    if (ui_run(argc, argv) < 0) {
        fprintf(stderr, "Failed to run UI\n");
        gpib_close();
        return EXIT_FAILURE;
    }

    gpib_close();
    return EXIT_SUCCESS;
}

