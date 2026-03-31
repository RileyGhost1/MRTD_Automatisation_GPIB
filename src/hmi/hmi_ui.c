#include <gtk/gtk.h>
#include "core.h"
#include "gpib.h"      

#define master_addr 0
#define dev_addr    1

static GtkWidget *stack1;

static GtkWidget *txtView_menu;
static GtkWidget *txtView_manual_log;

static GtkWidget *label_differential_temp;
static GtkWidget *label_setpoint_ready;
static GtkWidget *label_mrtd_progress;
static GtkWidget *label_emitter_temp;
static GtkWidget *label_target_temp;
static GtkWidget *label_target_index;
static GtkWidget *label_dev_status;
static GtkWidget *btn_auto;
static GtkWidget *btn_manual;
static GtkWidget *btn_help;
static GtkWidget *btn_connect_dev;
static GtkWidget *btn_tgt_table;
static GtkWidget *btn_serial_log;
static GtkWidget *btn_increase_temp;
static GtkWidget *btn_decrease_temp;
static GtkWidget *btn_save_mrtd_mesure;
static GtkWidget *btn_show_table;
static GtkWidget *btn_show_graph;
static GtkWidget *btn_reset_data;
static GtkWidget *btn_undo_last_mesure;
static GtkWidget *btn_invert_d;


/* ------------------------------------------------------------------ */
/* Timer de mise à jour UI                                            */
/* ------------------------------------------------------------------ */

typedef enum {
    UI_LOCK,
    UI_UNLOCK
} UiLockState;

void ui_sensitive(UiLockState ui_state)
{
    gtk_widget_set_sensitive(btn_manual, ui_state);
    gtk_widget_set_sensitive(btn_auto, ui_state);
    gtk_widget_set_sensitive(btn_tgt_table, ui_state);
    gtk_widget_set_sensitive(btn_serial_log, ui_state);
   /* gtk_widget_set_sensitive(btn_increase_temp, ui_state);
    gtk_widget_set_sensitive(btn_decrease_temp, ui_state);
    gtk_widget_set_sensitive(btn_save_mrtd_mesure, ui_state);
    gtk_widget_set_sensitive(btn_show_table, ui_state);
    gtk_widget_set_sensitive(btn_show_graph, ui_state);
    gtk_widget_set_sensitive(btn_reset_data, ui_state);
    gtk_widget_set_sensitive(btn_undo_last_mesure, ui_state);
    gtk_widget_set_sensitive(btn_invert_d, ui_state);*/
}

gboolean ui_update_labels(gpointer user_data)
{
    (void)user_data;

    switch(g_controller.state)
    {
        case MASTER_ONLINE_DEVICE_ONLINE:
            gtk_label_set_text(GTK_LABEL(label_dev_status), "ONLINE");
            ui_sensitive(UI_UNLOCK);
            break;
        default:
            gtk_label_set_text(GTK_LABEL(label_dev_status), "OFFLINE");
            //ui_sensitive(UI_LOCK);
            ui_sensitive(UI_UNLOCK); // pour dev, on laisse les boutons actifs même si pas de device
            break;
    }

    return TRUE; 
}



void hmi_log_append(const char *text)
{
    if (!txtView_menu) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txtView_menu));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, text, -1); 
    gtk_text_buffer_insert(buffer, &end, "\n", -1);

    GtkTextMark *mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(txtView_menu), mark, 0.0, FALSE, 0.0, 1.0);
}

/* ------------------------------------------------------------------ */
/* Callbacks MENU principal (page0)                                   */
/* ------------------------------------------------------------------ */

void on_btn_auto_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    /* TODO: basculer stack1 vers la page1 en mode AUTO, init séquence auto */
    /* gtk_stack_set_visible_child_name(GTK_STACK(stack1), "page1"); */
}

void on_btn_manual_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;

    gtk_stack_set_visible_child_name(GTK_STACK(stack1), "page1");
}

void on_btn_tgt_table_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    /* TODO: ouvrir / afficher table des cibles (fenêtre ou autre stack) */
}

void on_btn_help_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    /* TODO: afficher fenêtre d'aide / message dialog */
}

/* ------------------------------------------------------------------ */
/* Callbacks colonne droite (GPIB / système)                          */
/* ------------------------------------------------------------------ */

void on_btn_connect_dev_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;

    gpib_init(0, 1);

    if(g_controller.state == MASTER_ONLINE_DEVICE_ONLINE)
    {
        hmi_log_append("GPIB connecté — board 0, adresse 1");
    }
    else if(g_controller.state == MASTER_ONLINE_DEVICE_OFFLINE)
    {
        hmi_log_append("ERREUR : GPIB-USB online, device not responding");
    }
    else
    {
        hmi_log_append("Erreur : GPIB_USB not found");
    }
}

void on_btn_serial_log_toggled(GtkToggleButton *button, gpointer user_data)
{
    (void)user_data;
    gboolean active = gtk_toggle_button_get_active(button);

    /* TODO: activer/désactiver log série, afficher info dans txtView_menu */
    if (active) {
        /* log ON */
    } else {
        /* log OFF */
    }
}

void on_btn_rst_raspi_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    
    printf("Redémarrage du système...");
    hmi_log_append("Redémarrage du système...");
    cleanup_and_quit();
    hmi_log_append("GPIB libéré proprement");
    gtk_main_quit();
    //g_spawn_command_line_async("sudo reboot", NULL);
}

void on_btn_shutdown_raspi_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    
    printf("Extinction du système...");
    hmi_log_append("Extinction du système...");
    cleanup_and_quit();
    hmi_log_append("GPIB libéré proprement");
    gtk_main_quit();
    //n'est pas exécuté g_spawn_command_line_async("sudo shutdown now", NULL);
}

/* ------------------------------------------------------------------ */
/* Callbacks page MRTD / MANUAL (page1)                               */
/* ------------------------------------------------------------------ */

void on_btn_back_menu_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    /* Retour au menu principal (page0) */
    gtk_stack_set_visible_child_name(GTK_STACK(stack1), "page0");
}

void on_btn_increase_pressed(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    /* TODO: commencer incrémentation température (appui long ?) */
    /* ex: gpib_temp_inc_start(); */
}

void on_btn_increase_released(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    /* TODO: arrêter incrémentation température */
    /* ex: gpib_temp_inc_stop(); */
}

void on_btn_decrease_temp_pressed(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    /* TODO: commencer décrémentation température */
}

void on_btn_decrease_temp_released(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    /* TODO: arrêter décrémentation température */
}

void on_btn_save_mrtd_mesure_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    /* TODO: sauver point MRTD courant, mettre à jour label_mrtd_progress */
}

void on_btn_show_table_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    /* TODO: afficher table MRTD */
}

void on_btn_show_graph_clicked(GtkButton *button, gpointer user_data){
    (void)button;
    (void)user_data;
}

void on_btn_reset_data_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    /* TODO: reset data MRTD + refresh labels/graph */
}

void on_btn_undo_last_mesure_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    /* TODO: annuler dernier point MRTD */
}

void on_btn_invert_d_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    /* TODO: inverser signe de d (deltaT ?) et mettre à jour affichage */
}

/* ------------------------------------------------------------------ */
/* Initialisation HMI                                                 */
/* ------------------------------------------------------------------ */

int hmi_init(int *argc, char ***argv)
{
    GtkBuilder *builder;
    GtkWidget  *window;

    gtk_init(argc, argv);

    builder = gtk_builder_new_from_file(GLADE_PATH);

    /* Fenêtre principale */
    window = GTK_WIDGET(gtk_builder_get_object(builder, "hWindows"));

    /* Stack principal */
    stack1 = GTK_WIDGET(gtk_builder_get_object(builder, "stack1"));

    /* Widgets texte/log */
    txtView_menu = GTK_WIDGET(gtk_builder_get_object(builder, "txtView_menu"));

    /* Labels status/temp/MRTD */
    label_differential_temp = GTK_WIDGET(gtk_builder_get_object(builder, "label_differential_temp"));
    label_setpoint_ready    = GTK_WIDGET(gtk_builder_get_object(builder, "label_setpoint_ready"));
    label_mrtd_progress     = GTK_WIDGET(gtk_builder_get_object(builder, "label_mrtd_progress"));
    label_emitter_temp      = GTK_WIDGET(gtk_builder_get_object(builder, "label_emitter_temp"));
    label_target_temp       = GTK_WIDGET(gtk_builder_get_object(builder, "label_target_temp"));
    label_target_index      = GTK_WIDGET(gtk_builder_get_object(builder, "label_target_index"));
    label_dev_status        = GTK_WIDGET(gtk_builder_get_object(builder, "label_dev_status"));

    /* Boutons */
    btn_connect_dev      = GTK_WIDGET(gtk_builder_get_object(builder, "btn_connect_dev"));
    btn_manual           = GTK_WIDGET(gtk_builder_get_object(builder, "btn_manual"));
    btn_auto             = GTK_WIDGET(gtk_builder_get_object(builder, "btn_auto"));
    btn_help             = GTK_WIDGET(gtk_builder_get_object(builder, "btn_help"));
    btn_tgt_table        = GTK_WIDGET(gtk_builder_get_object(builder, "btn_tgt_table"));
    btn_serial_log       = GTK_WIDGET(gtk_builder_get_object(builder, "btn_serial_log"));
    btn_increase_temp    = GTK_WIDGET(gtk_builder_get_object(builder, "btn_increase_temp"));
    btn_decrease_temp    = GTK_WIDGET(gtk_builder_get_object(builder, "btn_decrease_temp"));
    btn_save_mrtd_mesure = GTK_WIDGET(gtk_builder_get_object(builder, "btn_save_mrtd_mesure"));
    btn_show_table       = GTK_WIDGET(gtk_builder_get_object(builder, "btn_show_table"));
    btn_show_graph       = GTK_WIDGET(gtk_builder_get_object(builder, "btn_show_graph"));
    btn_reset_data       = GTK_WIDGET(gtk_builder_get_object(builder, "btn_reset_data"));
    btn_undo_last_mesure = GTK_WIDGET(gtk_builder_get_object(builder, "btn_undo_last_mesure"));
    btn_invert_d         = GTK_WIDGET(gtk_builder_get_object(builder, "btn_invert_d"));

    /* Signaux automatiques (basés sur handler="..." dans le .glade) */
    gtk_builder_connect_signals(builder, NULL);

    /* Signaux génériques */
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
    g_object_unref(builder);  /* ← après tous les gtk_builder_get_object */

    /* Timer de mise à jour des labels */
    g_timeout_add(500, ui_update_labels, NULL);

    gtk_main();
    return 0;
}