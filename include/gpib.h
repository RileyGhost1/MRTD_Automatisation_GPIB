#ifndef GPIB_H
#define GPIB_H
#include <gtk/gtk.h>
#include <pthread.h>


void *gpib_poll_thread(void *arg);
int gpib_init(int master_addr, int dev_addr);
int gpib_close(void);
int gpib_temp_inc(void);
int gpib_temp_dec(void);
int gpib_next_target(void);
int gpib_read_all(void);
int gpib_write(const char *command);
int gpib_read(char *response);
int gpib_write_read(const char *command, char *response);
void cleanup_and_quit(void);
void *gpib_manual_mode(void *arg);

typedef enum {
    MASTER_OFFLINE_DEVICE_OFFLINE = 0,
    MASTER_ONLINE_DEVICE_OFFLINE,
    MASTER_ONLINE_DEVICE_ONLINE,
    DEVICE_ERROR    // réservé pour la phase mesure MRTD
} DeviceState;

typedef struct {
    double target_temp;
    double emitter_temp;
    int    target_index;
    int    last_error;
    DeviceState state;
    int    ud;
} ControllerState;

extern ControllerState g_controller;

#endif 