#ifndef GPIB_H
#define GPIB_H
#include <gtk/gtk.h>

int gpib_init(int PrimaryAddress);
int gpib_close(void);
float gpib_temp_inc(void);
float gpib_temp_dec(void);
int   gpib_next_target(void);
int gpib_read_all(void);



#endif 