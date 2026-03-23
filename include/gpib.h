#ifndef GPIB_H
#define GPIB_H
#include <gtk/gtk.h>

typedef struct {
    // GPIB
    int   gpib_dev;

    // Données SR-80 lues par gpib_read_all()
    float target_temp;    // RT
    float emitter_temp;   // RE
    int   current_target; // RA

    // GTK
    GtkWidget *label_status;
    GtkWidget *statusbar;
    GtkWidget *label_tgt_temp; 
    GtkWidget *label_target;
    guint      ctx_id;
} AppContext;

int gpib_init(int PrimaryAddress);
int gpib_close(void);
float gpib_temp_inc(void);
float gpib_temp_dec(void);
int   gpib_next_target(void);
int gpib_read_all(AppContext *ctx);



#endif 