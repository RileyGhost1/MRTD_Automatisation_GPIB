#include <gtk/gtk.h>
#include "ui.h"
#include "gpib.h"
#define master_addr 0
#define dev_addr 1

void on_inc_temp_clicked(GtkButton *btn, gpointer data) {
    gpib_temp_inc();
}

void on_dec_temp_clicked(GtkButton *btn, gpointer data) {
    gpib_temp_dec();
}

void on_next_target_clicked(GtkButton *btn, gpointer data) {
    gpib_next_target();
}

void on_read_device_clicked(GtkButton *btn, gpointer data) {
    gpib_read_all();

}

void on_connect_device_clicked(GtkButton *btn, gpointer data) {
    gpib_init(master_addr,dev_addr);

}

int hmi_init(int *argc, char ***argv) {
    GtkBuilder *builder;
    GtkWidget  *window;
    GtkWidget  *btn_inc, *btn_dec, *btn_next, *btn_read, *btn_connect, *label_status, *label_tgt_temp, *label_target;  

    gtk_init(argc, argv);

    builder  = gtk_builder_new_from_file(GLADE_PATH);
    window   = GTK_WIDGET(gtk_builder_get_object(builder, "hWindows"));
    btn_inc  = GTK_WIDGET(gtk_builder_get_object(builder, "ButtonIncrease"));
    btn_dec  = GTK_WIDGET(gtk_builder_get_object(builder, "ButtonDecrease"));
    btn_next = GTK_WIDGET(gtk_builder_get_object(builder, "ButtonNextTgt"));
    btn_read = GTK_WIDGET(gtk_builder_get_object(builder, "ButtonRead"));
    btn_connect = GTK_WIDGET(gtk_builder_get_object(builder, "ButtonConnect"));
    label_status   = GTK_WIDGET(gtk_builder_get_object(builder, "LabelDeviceStatus"));
    label_tgt_temp = GTK_WIDGET(gtk_builder_get_object(builder, "LabelTgtTemp"));
    label_target   = GTK_WIDGET(gtk_builder_get_object(builder, "LabelTarget"));


    g_signal_connect(window,   "destroy", G_CALLBACK(gtk_main_quit),          NULL);
    g_signal_connect(btn_inc,  "clicked", G_CALLBACK(on_inc_temp_clicked),    NULL);
    g_signal_connect(btn_dec,  "clicked", G_CALLBACK(on_dec_temp_clicked),    NULL);
    g_signal_connect(btn_next, "clicked", G_CALLBACK(on_next_target_clicked), NULL);
    g_signal_connect(btn_read, "clicked", G_CALLBACK(on_read_device_clicked), NULL);
    g_signal_connect(btn_connect, "clicked", G_CALLBACK(on_connect_device_clicked), NULL);

    gtk_widget_show_all(window);
    g_object_unref(builder);

    gtk_main();
    return 0;
}
