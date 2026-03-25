#include <gpib/ib.h>
#include <gtk/gtk.h>
#include "gpib.h"

int gpib_temp_inc(void)
{
    float set_temp;
    char cmd[64];

    set_temp = g_controller.target_temp + 1.0f;

    snprintf(cmd, sizeof(cmd), "ST%.2f", set_temp);

    if (gpib_write(cmd) < 0) {
        fprintf(stderr, "Failed to send temp inc command\n");
        return -1;
    }

    g_controller.target_temp = set_temp;
    printf("Temp. set to %.2f °C\n", g_controller.target_temp);

    return 0;
}


int gpib_temp_dec(void)
{
    //gpib_write_cmd(const char *cmd)
    g_print("Temp. -01.00C\n");
    return 0;
}

int gpib_next_target(void)
{
    int set_target = 0;
    char cmd[64];

    if(g_controller.target_index < 12){
        set_target = g_controller.target_index + 1; 
    } else {
        set_target = 1;
    }
    snprintf(cmd, sizeof(cmd), "SA%d", set_target);

    if (gpib_write(cmd) < 0) {
        fprintf(stderr, "Failed to send next target command\n");
        return -1;
    }

    g_controller.target_index = set_target;
    printf("Target set to %d\n", g_controller.target_index);

    if(set_target = 12);
        set_target = 0;

    return 0;
}

