#include <gtk/gtk.h>
#include <cjson/cJSON.h>
#include <dirent.h>
#include <libgen.h>
#include "core.h"
#include "gpib.h"     

#define master_addr 0
#define dev_addr    1
#define INCREMENT_HOLD_DELAY 200 /*Permets d'ajuster la réactivité de l'incrémentation des btn (de)increase */


static GtkWidget *stack1;

static GtkWidget *txtView_menu;
static GtkWidget *txtView_manual_log;
static GtkWidget *popover_profiles  = NULL;
static GtkWidget *listbox_profiles  = NULL;
static GtkWidget *popover_keyboard = NULL;

static GtkWidget *label_differential_temp;
static GtkWidget *label_setpoint_ready;
static GtkWidget *label_setpoint_temp;
static GtkWidget *label_emitter_temp;
static GtkWidget *label_target_temp;
static GtkWidget *label_target_index;
static GtkWidget *label_dev_status;
static GtkWidget *label_profile;
static GtkWidget *btn_auto;
static GtkWidget *btn_manual;
static GtkWidget *btn_import_profile;
static GtkWidget *btn_connect_dev;
static GtkWidget *btn_serial_log;
static GtkWidget *btn_increase_temp;
static GtkWidget *btn_decrease_temp;
static GtkWidget *btn_save_mrtd_mesure;
static GtkWidget *btn_export_profile;
static GtkWidget *btn_show_graph;
static GtkWidget *btn_reset_data;
static GtkWidget *btn_undo_last_mesure;
static GtkWidget *btn_invert_d;
static GtkWidget *btn_back_menu;
static GtkWidget *btn_select_profile;

/*Pour page d'exports résultats*/
static GtkWidget *btn_cancel;         
static GtkWidget *btn_eject_usb;        
static GtkWidget *btn_export_usb;
static GtkWidget *entry_asset;      
static GtkWidget *txtView_export;

ProgramMode mode = MENU;

static void on_popover_refresh_clicked(GtkButton *button, gpointer user_data);
static void on_profile_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data);
/* ------------------------------------------------------------------ */
/* !!! GTK N'EST PAS THREAD-SAFE !!! 
Nous ne pouvons pas appeler les fonction GTK depuis un autre thread, 
Nous ne pouvons pas non plus faire du polling dans le thread GTK, cela rendrait l'interface non réactive et pourrait causer des blocages.
Nous ne pouvons pas mettre a jour un label comme gtk_label_set_text() depuis le thread de polling, cela causerait des comportements indéterminés et potentiellement des crashs.

TODO: mettre en place un systeme de queu pour transmettre les actions a executer par le thread gpib. 
*/

#pragma region COMMON

void force_hmi_on_dsi(GtkWidget *window_hmi) {
    GdkDisplay *display = gdk_display_get_default();
    int num_monitors = gdk_display_get_n_monitors(display);
    
    int index_dsi = 1;  // Par défaut, d'après ton diagnostic, le DSI était l'index 1
    
    // 1. Chercher dynamiquement l'index de l'écran tactile
    for (int i = 0; i < num_monitors; i++) {
        GdkMonitor *monitor = gdk_display_get_monitor(display, i);
        const char *modele = gdk_monitor_get_model(monitor);
        
        if (modele != NULL && strstr(modele, "DSI") != NULL) {
            index_dsi = i;
            break; // On a trouvé le DSI, on sort de la boucle
        }
    }
    
    // 2. Forcer la fenêtre principale sur cet écran précis
    gtk_widget_realize(window_hmi); 
    gtk_window_fullscreen_on_monitor(GTK_WINDOW(window_hmi), 
                                     gdk_window_get_screen(gtk_widget_get_window(window_hmi)), 
                                     index_dsi);
                                     
    gtk_widget_show_all(window_hmi);
}

void display_detect() {
    // 1. Obtenir le "Display" (le serveur X11 actif)
    GdkDisplay *display = gdk_display_get_default();
    
    if (display == NULL) {
        g_printerr("Erreur critique: Aucun serveur graphique (X11) détecté.\n");
        return;
    }

    // 2. Récupérer le nombre de moniteurs branchés
    int num_monitors = gdk_display_get_n_monitors(display);
    g_print("--- DIAGNOSTIC DES ECRANS ---\n");
    g_print("Nombre d'écrans détectés : %d\n", num_monitors);

    // 3. Boucler pour analyser chaque moniteur
    for (int i = 0; i < num_monitors; i++) {
        GdkMonitor *monitor = gdk_display_get_monitor(display, i);
        
        // Récupérer le nom matériel (très pratique pour les différencier)
        const char *modele = gdk_monitor_get_model(monitor);
        
        // Récupérer la taille et la position (Géométrie)
        GdkRectangle geom;
        gdk_monitor_get_geometry(monitor, &geom);

        g_print("\n[ Moniteur %d ]\n", i);
        g_print("  -> Modèle     : %s\n", modele ? modele : "Inconnu");
        g_print("  -> Résolution : %d x %d pixels\n", geom.width, geom.height);
        g_print("  -> Position   : X=%d, Y=%d\n", geom.x, geom.y);
    }
    g_print("-----------------------------\n");
}

typedef enum {
    UI_LOCK,
    UI_UNLOCK
} UiLockState;

void ui_sensitive(UiLockState ui_state)
{
    gtk_widget_set_sensitive(btn_manual, ui_state);
    gtk_widget_set_sensitive(btn_auto, ui_state);
    
    gtk_widget_set_sensitive(btn_serial_log, ui_state);
   /* gtk_widget_set_sensitive(btn_increase_temp, ui_state);
    gtk_widget_set_sensitive(btn_decrease_temp, ui_state);
    gtk_widget_set_sensitive(btn_save_mrtd_mesure, ui_state);
    gtk_widget_set_sensitive(btn_export_profile, ui_state);
    gtk_widget_set_sensitive(btn_show_graph, ui_state);
    gtk_widget_set_sensitive(btn_reset_data, ui_state);
    gtk_widget_set_sensitive(btn_undo_last_mesure, ui_state);
    gtk_widget_set_sensitive(btn_invert_d, ui_state);*/
}


/*
    Exécuté toute les 500ms.
*/
gboolean ui_update_labels(gpointer user_data)
{
    AppData *app = (AppData *)user_data;
    GpibData snap;
    char     buf[64];

    if (app == NULL) return TRUE;

    pthread_mutex_lock(&app->mutex);
    snap = app->device_status;
    pthread_mutex_unlock(&app->mutex);

    /* Delta T */
    snprintf(buf, sizeof(buf), "ΔT = %.3f °C", snap.actual_dt);
    gtk_label_set_text(GTK_LABEL(label_differential_temp), buf);

    /* Température émetteur */
    snprintf(buf, sizeof(buf), "Emitter : %.2f °C", snap.emitter_temp);
    gtk_label_set_text(GTK_LABEL(label_emitter_temp), buf);

    /* Température cible */
    snprintf(buf, sizeof(buf), "Target : %.2f °C", snap.target_temp);
    gtk_label_set_text(GTK_LABEL(label_target_temp), buf);

    /* Index roue */
    snprintf(buf, sizeof(buf), "Target index :%d/12", snap.target_index);
    gtk_label_set_text(GTK_LABEL(label_target_index), buf);

    /* Temp Ready */
    if(snap.temp_ready){
        snprintf(buf, sizeof(buf), "Temp READY");
        gtk_label_set_text(GTK_LABEL(label_setpoint_ready), buf);
    } else {
        snprintf(buf, sizeof(buf), "Temp NOT READY");
        gtk_label_set_text(GTK_LABEL(label_setpoint_ready), buf);
    }


    return TRUE;
}

gboolean set_status_online(gpointer data)
{
    (void)data;
    gtk_label_set_text(GTK_LABEL(label_dev_status), "ONLINE");
    return FALSE;
}

gboolean set_status_offline(gpointer data)
{
    (void)data;
    gtk_label_set_text(GTK_LABEL(label_dev_status), "OFFLINE");
    return FALSE;
}

#include <gtk/gtk.h>
#include <string.h>

// On suppose que MANUAL, MENU et EXPORT sont des macros définies plus haut
// par exemple : #define MANUAL "page_manual"

void hmi_log_append(const char *text)
{
    if ((!txtView_menu) || (!txtView_manual_log) || (!txtView_export)){
        LOG_MSG("Erreur: TextView non initialisée, impossible d'afficher le log.");
        return;
    }

    // PLUS BESOIN DU BUILDER ICI ! On utilise la variable globale stack1
    if (!stack1) {
        LOG_MSG("Erreur: stack1 global est NULL.");
        return;
    }

    // Récupérer le nom de la page (ex: "page_menu", "page_manual")
    const gchar *page = gtk_stack_get_visible_child_name(GTK_STACK(stack1));
    GtkWidget *target_view = NULL;

    if (!page) {
        LOG_MSG("Erreur: Aucune page visible dans le Stack.");
        return;
    }

    // On compare avec les noms exacts définis dans ton fichier .glade
    if (g_strcmp0(page, "MANUAL") == 0) {
        target_view = txtView_manual_log;
    } else if (g_strcmp0(page, "MENU") == 0) {
        target_view = txtView_menu;
    } else if (g_strcmp0(page, "EXPORT") == 0) {
        target_view = txtView_export;
    } else {
        LOG_MSG("Nom de page non géré pour les logs: %s", page);
        return;
    }

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(target_view));
    GtkTextIter end;

    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, text, -1);
    gtk_text_buffer_insert(buffer, &end, "\n", -1);

    GtkTextMark *mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(target_view), mark, 0.0, FALSE, 0.0, 1.0);
}


// Cette fonction sera exécutée par le thread GTK (UI) mais appelée depuis n'importe quel thread via g_idle_add() pour garantir la sécurité des threads.
gboolean hmi_log_append_idle(gpointer data) 
{
    char *text = (char *)data;


    if (text != NULL) {
        hmi_log_append(text); // Appelle ta fonction d'origine
        free(text);           // Libère la chaîne allouée par le thread service
    }

    return FALSE; // Indique à GTK de ne pas réexécuter cette fonction en boucle
}
#pragma endregion

#pragma region MENU
/* ------------------------------------------------------------------ */
/* Callbacks MENU principal (page0)                                   */
/* ------------------------------------------------------------------ */

void on_btn_auto_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;

    /* TODO: basculer stack1 vers la page1 en mode AUTO, init séquence auto */
    /* gtk_stack_set_visible_child_name(GTK_STACK(stack1), "page1"); */
}

gboolean change_window(gpointer data) { /* MENU, MANUAL */
    char *page = (char *)data;

    gtk_stack_set_visible_child_name(GTK_STACK(stack1),page);
    free(page);

    return FALSE;
}

void on_btn_manual_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    

    AppData *app = (AppData *)user_data;

    if (app == NULL) return;

    hmi_log_append("Mode manuel demandé...");
    mrtd_cmd_queu(app, PARSE);
    LOG_MSG("Parsing demandé");
    
}


static gboolean on_combo_touch(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    (void)data;
    (void)event;
    gtk_combo_box_popup(GTK_COMBO_BOX(widget));
    return FALSE;
}

void on_btn_select_profile_clicked(GtkComboBox *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    gtk_popover_popup(GTK_POPOVER(popover_profiles));
}

static void create_profile_popover(GtkWidget *anchor, AppData *app) {
    popover_profiles = gtk_popover_new(anchor);

    GtkWidget *vbox        = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *hbox        = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *btn_refresh = gtk_button_new_with_label("Refresh");
    GtkWidget *btn_import  = gtk_button_new_with_label("Import");
    GtkWidget *btn_export  = gtk_button_new_with_label("Export");
    GtkWidget *sep         = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    GtkWidget *scroll      = gtk_scrolled_window_new(NULL, NULL);

    listbox_profiles = gtk_list_box_new();

    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 400);
    gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(scroll),  400);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    gtk_box_pack_start(GTK_BOX(hbox), btn_refresh, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), btn_import,  TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), btn_export,  TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(scroll), listbox_profiles);
    gtk_box_pack_start(GTK_BOX(vbox), hbox,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), sep,    FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE,  TRUE,  0);
    gtk_container_add(GTK_CONTAINER(popover_profiles), vbox);
    gtk_widget_show_all(vbox);

    g_signal_connect(btn_refresh,      "clicked",       G_CALLBACK(on_popover_refresh_clicked), app);
    g_signal_connect(listbox_profiles, "row-activated", G_CALLBACK(on_profile_row_activated),   app);
}

static void on_popover_refresh_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    AppData *app = (AppData *)user_data;
    if (app == NULL) return;

    gtk_container_foreach(GTK_CONTAINER(listbox_profiles),
                          (GtkCallback)gtk_widget_destroy, NULL);

    DIR *dir = opendir(app->profiles_path);
    if (dir == NULL) { perror("[REFRESH] opendir"); return; }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        size_t len_name = strlen(entry->d_name);
        if (len_name < 5) continue; 
        if (!strstr(entry->d_name, ".json")) continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s",
                 app->profiles_path, entry->d_name);

        FILE *f = fopen(filepath, "r");
        if (!f) continue;

        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        rewind(f);

        char *buf = malloc(len + 1);
        fread(buf, 1, len, f);
        buf[len] = '\0';
        fclose(f);

        cJSON *root = cJSON_Parse(buf);
        free(buf);
        if (!root) continue;

        cJSON *name = cJSON_GetObjectItem(root, "name");
        if (cJSON_IsString(name)) {
            GtkWidget *lbl = gtk_label_new(name->valuestring);
            gtk_widget_set_halign(lbl,        GTK_ALIGN_START);
            gtk_widget_set_margin_top(lbl,    10);
            gtk_widget_set_margin_bottom(lbl, 10);
            gtk_widget_set_margin_start(lbl,  14);
            gtk_widget_set_margin_end(lbl,    14);

            GtkWidget *row = gtk_list_box_row_new();
            g_object_set_data_full(G_OBJECT(row), "filename",
                                   g_strdup(entry->d_name), g_free);
            gtk_container_add(GTK_CONTAINER(row), lbl);
            gtk_container_add(GTK_CONTAINER(listbox_profiles), row);
        }
        cJSON_Delete(root);
    }
    closedir(dir);
    gtk_widget_show_all(listbox_profiles);
}


static void on_profile_row_activated(GtkListBox *box, GtkListBoxRow *row,
                                     gpointer user_data)
{
    (void)box;
    char full_path[512];
    AppData *app = (AppData *)user_data;
    if (app == NULL || row == NULL) return;

    const char *filename = (const char *)g_object_get_data(G_OBJECT(row), "filename");
    if (!filename) return;

    g_free(app->selected_profile_path);

    snprintf(full_path, sizeof(full_path), "%s/%s", app->profiles_path, filename);
    app->selected_profile_path = strdup(full_path);

    printf("[PROFILE] Sélectionné : %s\n", app->selected_profile_path);

    char tmp[256];
    strncpy(tmp, app->selected_profile_path, sizeof(tmp) - 1);
    gtk_label_set_text(GTK_LABEL(label_profile), basename(tmp));

    gtk_popover_popdown(GTK_POPOVER(popover_profiles));
}

void on_btn_import_profile_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    
    /* TODO: afficher fenêtre d'aide / message dialog */
}

void on_btn_connect_dev_clicked(GtkButton *button, gpointer user_data)
{
    (void)button; // Pour éviter le warning "unused parameter"

    // RÉCUPÉRATION DU POINTEUR :
    AppData *app = (AppData *)user_data;

    if (app == NULL) return; // Sécurité

    app_set_service_gpib(app, CONNECT);
    hmi_log_append("Ordre de connexion envoyé...");
    
}

void on_btn_serial_log_toggled(GtkToggleButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
}

void on_btn_rst_raspi_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    
    
    printf("Redémarrage du système...");
    hmi_log_append("Redémarrage du système...");
    //cleanup_and_quit();
    hmi_log_append("GPIB libéré proprement");
    gtk_main_quit();
    //g_spawn_command_line_async("sudo reboot", NULL);
}

void on_btn_shutdown_raspi_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    
    
    printf("Extinction du système...");
    hmi_log_append("Extinction du système...");
    hmi_log_append("GPIB libéré proprement");
    gtk_main_quit();
    //n'est pas exécuté g_spawn_command_line_async("sudo shutdown now", NULL);
}
#pragma endregion

#pragma region MANUAL
/* ------------------------------------------------------------------ */
/* Callbacks page MRTD / MANUAL (page1)                               */
/* ------------------------------------------------------------------ */

void on_btn_back_menu_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    
    /* Retour au menu principal (page0) */

        // RÉCUPÉRATION DU POINTEUR :
    AppData *app = (AppData *)user_data;

    if (app == NULL) return; // Sécurité

    app_set_service_gpib(app, IDLE);
    gtk_stack_set_visible_child_name(GTK_STACK(stack1), "MENU");
    mode = MENU;
}
                
void on_btn_show_table_clicked(GtkButton *button, gpointer user_data){
    
    (void)button;
    (void)user_data;
    
    AppData *app = (AppData *)user_data;

    if (app == NULL) return;

    hmi_log_append("sending mrtd table show cmd");
    mrtd_cmd_queu(app, TABLE);
    LOG_MSG("sending MRTD table show cmd to MRTD thread");
}

/*
* est appelée quand btn_released, envoie la commande pour que setpoint sois màj sur le sr80
* la commande est transmise au fichier service.c au thread service_gpib pour via une GasyncQueu
*
*/
static void apply_increment(gpointer user_data)
{
    AppData     *app = (AppData *)user_data;
    BtnTemp     *b   = &app->btn_hold;
    GAsyncQueue *q   = app->gpib_queue;

    // Allouer la chaîne sur le heap directement
    gchar *cmd = g_strdup_printf("ST%.2f", b->temp_set_point);

    g_async_queue_push(q, cmd);  // push du char*
}

/* ── Timer hold générique (+ ou -) ── */
static gboolean on_btn_temp_hold(gpointer user_data)
{
    AppData  *app = (AppData *)user_data;
    BtnTemp  *b   = &app->btn_hold;
    char      buf[64];

    b->hold_duration += INCREMENT_HOLD_DELAY;

    float step;
    if      (b->hold_duration < 1000) step = 0.01f;
    else if (b->hold_duration < 3000) step = 0.1f;
    else                              step = 0.5f;

    b->temp_set_point += step * b->direction;  // ← +1.0f ou -1.0f
    
    printf("[BTN] released after %dms → setpoint %.3f°C\n", b->hold_duration, b->temp_set_point);

    snprintf(buf, sizeof(buf), "Setpoint ΔT = %.3f °C", b->temp_set_point);
    gtk_label_set_text(GTK_LABEL(label_setpoint_temp), buf);

    return G_SOURCE_CONTINUE;
}

/* ── Logique commune pressed ── */
static void btn_temp_pressed(AppData *app, float direction)
{
    BtnTemp *b = &app->btn_hold;

    b->hold_duration  = 0;
    b->direction      = direction;

    b->temp_set_point += 0.01f * direction;  // incrément initial
    b->hold_timer_id   = g_timeout_add(500, on_btn_temp_hold, app);

    char buf[64];
    snprintf(buf, sizeof(buf), "Setpoint ΔT = %.3f °C", b->temp_set_point);
    gtk_label_set_text(GTK_LABEL(label_setpoint_temp), buf);
}

/* ── Logique commune released ── */
static void btn_temp_released(AppData *app)
{
    BtnTemp *b   = &app->btn_hold;
    char     buf[64];

    if (b->hold_timer_id != 0) {
        g_source_remove(b->hold_timer_id);
        b->hold_timer_id = 0;
    }

    snprintf(buf, sizeof(buf), "Setpoint ΔT = %.3f °C", b->temp_set_point);
    gtk_label_set_text(GTK_LABEL(label_setpoint_temp), buf);

    printf("[BTN] released after %dms → setpoint %.3f°C\n",
           b->hold_duration, b->temp_set_point);
    b->hold_duration = 0;

    apply_increment(app);
}

/* ── Callbacks GTK (wrappers minimalistes) ── */
gboolean on_btn_increase_pressed(GtkWidget *button, gpointer user_data)
{
    (void)button;
    btn_temp_pressed((AppData *)user_data, +1.0f);
    return FALSE;
}

gboolean on_btn_increase_released(GtkWidget *button, gpointer user_data)
{
    (void)button;
    btn_temp_released((AppData *)user_data);
    return FALSE;
}

gboolean on_btn_decrease_temp_pressed(GtkWidget *button, gpointer user_data)
{
    (void)button;
    btn_temp_pressed((AppData *)user_data, -1.0f);
    return FALSE;
}

gboolean on_btn_decrease_temp_released(GtkWidget *button, gpointer user_data)
{
    (void)button;
    btn_temp_released((AppData *)user_data);
    return FALSE;
}

void on_btn_save_mrtd_mesure_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    

    AppData *app = (AppData *)user_data;

    if (app == NULL) return;

    hmi_log_append("sending saving cmd");
    mrtd_cmd_queu(app, SAVE);
    LOG_MSG("sending saving cmd to MRTD thread");
}

void on_btn_export_profile_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;

    AppData *app = (AppData *)user_data;
    mode = EXPORT;
    if (app == NULL) return;
}

void on_btn_show_graph_clicked(GtkButton *button, gpointer user_data){
    (void)button;
    (void)user_data;

    AppData *app = (AppData *)user_data;

    if (app == NULL) return;

    change_window(g_strdup("EXPORT"));

    hmi_log_append("Affichage du graph MRTD demandé...");
    mrtd_cmd_queu(app, GRAPH);
    LOG_MSG("Display graph");
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
    
    AppData *app = (AppData *)user_data;

    if (app == NULL) return;

    hmi_log_append("sending mrtd table show cmd");
    mrtd_cmd_queu(app, UNDO);
    LOG_MSG("sending MRTD undo last measure cmd to MRTD thread");
}

/* ── Inversion du signe de ΔT ── */
static void invert_d(AppData *app)
{
    BtnTemp     *b   = &app->btn_hold;
    GAsyncQueue *q   = app->gpib_queue;
    char         buf[64];

    // Inverser le signe
    b->temp_set_point = -b->temp_set_point;

    // Nettoyer le -0.0f
    if (b->temp_set_point == 0.0f) b->temp_set_point = 0.0f;

    // Mettre à jour le label
    snprintf(buf, sizeof(buf), "Setpoint ΔT = %.3f °C", b->temp_set_point);
    gtk_label_set_text(GTK_LABEL(label_setpoint_temp), buf);

    // Envoyer la commande au thread GPIB via la queue
    gchar *cmd = g_strdup_printf("ST%.2f", b->temp_set_point);
    g_async_queue_push(q, cmd);

    g_debug("ΔT inversé → setpoint %.3f°C", b->temp_set_point);
}

void on_btn_invert_d_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    AppData *app = (AppData *)user_data;

    if (app == NULL) return;

    invert_d(app);
}
#pragma endregion

#pragma region EXPORT
/* PAGE EXPORT */

void on_btn_cancel_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;

    LOG_MSG("Export cancelled");
    change_window(g_strdup("MANUAL"));
    mode = MANUAL;
}

static void task_export_result_usb(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable) {
    AppData *app = (AppData *)task_data;
    int success;

    // Générer le PDF (prend du temps)
    success = thread_export(app); //exporting.c
    //copy_to_usb("/mnt/usb/rapport.pdf");
    
    // Renvoyer le résultat
    if (success == 0) {
        g_task_return_boolean(task, TRUE);
    } else {
        g_task_return_new_error(task, G_IO_ERROR, G_IO_ERROR_FAILED, "Erreur d'écriture USB");
    }
}

// La fonction appelée quand le thread a fini (S'exécute DANS LE THREAD GTK)
static void on_export_finished(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GError *error = NULL;
    gboolean success = g_task_propagate_boolean(G_TASK(res), &error);
    
    if (success) {
        hmi_log_append("Export PDF et copie USB réussis !");
        gtk_widget_set_sensitive(btn_eject_usb, TRUE);
        // Optionnel : cacher un spinner de chargement sur l'UI
    } else {
        hmi_log_append("Échec de l'export : vérifiez la clé USB.");
        gtk_widget_set_sensitive(btn_eject_usb, FALSE);
        g_error_free(error);
    }

    gtk_widget_set_sensitive(btn_export_usb, TRUE);
}

/*Gestion de la copie du résultat sur clée usb, géré par un thread glib*/
void on_btn_export_usb_clicked(GtkButton *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    
    const char *texte = gtk_entry_get_text(GTK_ENTRY(entry_asset));

    if (texte == NULL || texte[0] == '\0') {
        g_print("[HMI] Erreur : L'identifiant de l'asset est vide.\n");
        hmi_log_append("Erreur : Veuillez saisir l'Asset avant d'exporter.");
        return;
    }
    // On libère l'ancien nom s'il y en avait un
    if (app->asset_name) free(app->asset_name); 
    app->asset_name = strdup(texte);

    hmi_log_append("Début de la génération PDF et de la copie USB...");
    // Optionnel : Afficher un spinner ou griser le bouton
    gtk_widget_set_sensitive(btn_export_usb, FALSE);
    gtk_widget_set_sensitive(btn_eject_usb, FALSE);

    // Lancer la tâche en arrière-plan
    GTask *task = g_task_new(NULL, NULL, on_export_finished, app);
    g_task_set_task_data(task, app, NULL);
    g_task_run_in_thread(task, task_export_result_usb);
    g_object_unref(task);
}
/* ----------------------------------------------------------------- */
void on_btn_eject_usb_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;

    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);

    sync();

    int ret = system("umount /media/usb");

    if (ret == 0) {
        LOG_MSG("[USB] Clé démontée avec succès.");
        hmi_log_append("La clé USB a été démontée.");
        hmi_log_append("Vous pouvez retirer la clé en toute sécurité.");
    } else {
        LOG_MSG("[USB] Erreur : Impossible de démonter. Un fichier est-il encore ouvert ?");
        hmi_log_append("Erreur : Impossible de démonter la clé USB.");
    }
    
}

gboolean on_entry_asset_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    g_print("[HMI] Toucher détecté sur Entry, ouverture du Popover.\n");
    
    // Afficher le Popover
    gtk_popover_popup(GTK_POPOVER(popover_keyboard));
    
    // STOPPER la propagation de l'événement pour éviter la fermeture immédiate !
    return TRUE; 
}

// Callback UNIFIÉ pour TOUTES tes touches (A-Z, 0-9)
gboolean on_key_pressed(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    // 1. Récupérer la lettre du bouton qui a été touché
    const gchar *lettre = gtk_button_get_label(GTK_BUTTON(widget));
    
    // Sécurité : au cas où un bouton n'aurait pas de label
    if (lettre == NULL) return TRUE; 

    // 2. Préparer l'insertion à la fin du texte existant
    gint position = -1; // -1 signifie "ajouter à la fin"
    
    // 3. Insérer la lettre dans GtkEntry (entry_asset)
    gtk_editable_insert_text(GTK_EDITABLE(entry_asset), lettre, -1, &position);
    
    // 4. Retourner TRUE pour empêcher GTK de faire des animations lourdes (focus, etc.)
    return TRUE; 
}

gboolean on_key_delete_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    gint length = gtk_entry_get_text_length(GTK_ENTRY(entry_asset));
    
    if (length > 0) {
        // Supprime le dernier caractère
        gtk_editable_delete_text(GTK_EDITABLE(entry_asset), length - 1, length);
    }
    return TRUE;
}

gboolean on_key_enter_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    // Ferme le popover
    gtk_popover_popdown(GTK_POPOVER(popover_keyboard));
    return TRUE;
}

#pragma endregion

/* ------------------------------------------------------------------ */
/* Initialisation HMI                                                 */
/* ------------------------------------------------------------------ */

int hmi_init(int *argc, char ***argv, AppData *app)
{
    GtkWidget  *window;
    GtkBuilder *builder;

    gtk_init(argc, argv);

    //display_detect(); // Affiche les infos sur les écrans connectés (utile pour le debug et la configuration)
    
    GtkSettings *settings = gtk_settings_get_default();
    g_object_set(settings, "gtk-enable-animations", FALSE, NULL);

    builder = gtk_builder_new_from_file(GLADE_PATH);

    /* Fenêtre principale */
    window = GTK_WIDGET(gtk_builder_get_object(builder, "hWindows"));
    system("xinput map-to-output \"10-0038 generic ft5x06 (79)\" DSI-1");

    force_hmi_on_dsi(window);
    stack1 = GTK_WIDGET(gtk_builder_get_object(builder, "stack1"));
    txtView_menu = GTK_WIDGET(gtk_builder_get_object(builder, "txtView_menu"));
    txtView_manual_log = GTK_WIDGET(gtk_builder_get_object(builder, "txtView_manual_log"));
    txtView_export       = GTK_WIDGET(gtk_builder_get_object(builder, "txtView_export"));

    /* Labels status/temp/MRTD */
    label_differential_temp = GTK_WIDGET(gtk_builder_get_object(builder, "label_differential_temp"));
    label_setpoint_ready    = GTK_WIDGET(gtk_builder_get_object(builder, "label_setpoint_ready"));
    label_setpoint_temp     = GTK_WIDGET(gtk_builder_get_object(builder, "label_setpoint_temp"));
    label_emitter_temp      = GTK_WIDGET(gtk_builder_get_object(builder, "label_emitter_temp"));
    label_target_temp       = GTK_WIDGET(gtk_builder_get_object(builder, "label_target_temp"));
    label_target_index      = GTK_WIDGET(gtk_builder_get_object(builder, "label_target_index"));
    label_dev_status        = GTK_WIDGET(gtk_builder_get_object(builder, "label_dev_status"));
    label_profile           = GTK_WIDGET(gtk_builder_get_object(builder, "label_profile"));


    /* Boutons */
    btn_connect_dev      = GTK_WIDGET(gtk_builder_get_object(builder, "btn_connect_dev"));
    btn_manual           = GTK_WIDGET(gtk_builder_get_object(builder, "btn_manual"));
    btn_auto             = GTK_WIDGET(gtk_builder_get_object(builder, "btn_auto"));
    btn_serial_log       = GTK_WIDGET(gtk_builder_get_object(builder, "btn_serial_log"));
    btn_increase_temp    = GTK_WIDGET(gtk_builder_get_object(builder, "btn_increase_temp"));
    btn_decrease_temp    = GTK_WIDGET(gtk_builder_get_object(builder, "btn_decrease_temp"));
    btn_save_mrtd_mesure = GTK_WIDGET(gtk_builder_get_object(builder, "btn_save_mrtd_mesure"));
    btn_export_profile   = GTK_WIDGET(gtk_builder_get_object(builder, "btn_export_profile"));
    btn_import_profile   = GTK_WIDGET(gtk_builder_get_object(builder, "btn_import_profile"));
    btn_show_graph       = GTK_WIDGET(gtk_builder_get_object(builder, "btn_show_graph"));
    btn_reset_data       = GTK_WIDGET(gtk_builder_get_object(builder, "btn_reset_data"));
    btn_undo_last_mesure = GTK_WIDGET(gtk_builder_get_object(builder, "btn_undo_last_mesure"));
    btn_invert_d         = GTK_WIDGET(gtk_builder_get_object(builder, "btn_invert_d"));
    btn_back_menu        = GTK_WIDGET(gtk_builder_get_object(builder, "btn_back_menu"));

    /* page d'export des résultats */
    btn_cancel           = GTK_WIDGET(gtk_builder_get_object(builder, "btn_cancel"));
    btn_eject_usb        = GTK_WIDGET(gtk_builder_get_object(builder, "btn_eject_usb"));
    btn_export_usb       = GTK_WIDGET(gtk_builder_get_object(builder, "btn_export_usb"));       
    entry_asset          = GTK_WIDGET(gtk_builder_get_object(builder, "entry_asset"));
    GtkWidget *grid_keyboard = GTK_WIDGET(gtk_builder_get_object(builder, "grid_keyboard"));


    GtkWidget *btn_select = GTK_WIDGET(gtk_builder_get_object(builder, "btn_select_profile"));
    create_profile_popover(btn_select, app);

    popover_keyboard = gtk_popover_new(entry_asset);
    gtk_widget_set_size_request(popover_keyboard, 750, 400);

    /* 3. On insère ta magnifique grille Glade dans le Popover C */
    gtk_container_add(GTK_CONTAINER(popover_keyboard), grid_keyboard);

    /* 4. ON FORCE L'AFFICHAGE (La ligne qui manquait) */
    gtk_widget_show_all(grid_keyboard);

    /* Signaux automatiques (basés sur handler="..." dans le .glade) */
    gtk_builder_connect_signals(builder, app);


    g_object_unref(builder);  /* ← après tous les gtk_builder_get_object */

    /* Timer de mise à jour des labels */
    g_timeout_add(500, ui_update_labels, app);

    gtk_main();
    return 0;
}