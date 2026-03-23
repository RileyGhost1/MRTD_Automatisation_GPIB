#include <gtk/gtk.h>
#include "ui.h"
#include "gpib.h"

void on_inc_temp_clicked(GtkButton *btn, gpointer data) {
    gpib_temp_inc();
}

void on_dec_temp_clicked(GtkButton *btn, gpointer data) {
    gpib_temp_dec();
}

void on_next_target_clicked(GtkButton *btn, gpointer data) {
    gpib_next_target();
}

int hmi_init(int *argc, char ***argv) {
    GtkBuilder *builder;
    GtkWidget  *window;
    GtkWidget  *btn_inc, *btn_dec, *btn_next;

    gtk_init(argc, argv);

    builder = gtk_builder_new_from_file(GLADE_PATH);

    window   = GTK_WIDGET(gtk_builder_get_object(builder, "hWindows"));
    btn_inc  = GTK_WIDGET(gtk_builder_get_object(builder, "ButtonIncrease"));
    btn_dec  = GTK_WIDGET(gtk_builder_get_object(builder, "ButtonDecrease"));
    btn_next = GTK_WIDGET(gtk_builder_get_object(builder, "ButtonNextTgt"));

    g_signal_connect(window,   "destroy", G_CALLBACK(gtk_main_quit),          NULL);
    g_signal_connect(btn_inc,  "clicked", G_CALLBACK(on_inc_temp_clicked),    NULL);
    g_signal_connect(btn_dec,  "clicked", G_CALLBACK(on_dec_temp_clicked),    NULL);
    g_signal_connect(btn_next, "clicked", G_CALLBACK(on_next_target_clicked), NULL);

    gtk_widget_show_all(window);
    g_object_unref(builder);

    gtk_main();
    return 0;
}
