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

typedef struct {
    double target_temp;
    double emitter_temp;
    int    target_index;
    int    connected;
    int    last_error;
} ControllerState;

extern ControllerState g_controller;

#endif 