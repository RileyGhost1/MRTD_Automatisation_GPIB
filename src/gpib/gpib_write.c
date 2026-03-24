#include <gpib/ib.h>
#include <gtk/gtk.h>

static float current_temp   = 20.0f;
static int   current_target = 1;

int gpib_write_cmd(int dev, const char *cmd)
{


}
float gpib_temp_inc(void)
{
    //gpib_write_cmd(const char *cmd)
    g_print("Temp. +01.00C\n");
    current_temp += 1.0f;
    return current_temp;
}

float gpib_temp_dec(void)
{
    //gpib_write_cmd(const char *cmd)
    g_print("Temp. -01.00C\n");
    current_temp -= 1.0f;
    return current_temp;
}

int gpib_next_target(void)
{
    g_print("Target Wheel moving...\n");
    current_target++;
    return current_target;
}

