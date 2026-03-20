#ifdef _WIN32
    #include <ni488.h>
#else
    #include <gpib/ib.h>
#endif

#include "gpib.h"   

static GtkWidget *label_temp;
static GtkWidget *label_target;

/* ── Callbacks ────────────────────────────────────── */

static void
on_inc_temp_clicked(GtkWidget *widget, gpointer user_data)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "Temp: %.1f °C", gpib_temp_inc());
    gtk_label_set_text(GTK_LABEL(label_temp), buf);
}

static void
on_dec_temp_clicked(GtkWidget *widget, gpointer user_data)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "Temp: %.1f °C", gpib_temp_dec());
    gtk_label_set_text(GTK_LABEL(label_temp), buf);
}

static void
on_target_temp_clicked(GtkWidget *widget, gpointer user_data)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "Target: %d", gpib_next_target());
    gtk_label_set_text(GTK_LABEL(label_target), buf);
}

/* ── Helper : applique une classe CSS à un widget ── */

static void
add_css_class(GtkWidget *widget, const char *class_name)
{
    GtkStyleContext *ctx = gtk_widget_get_style_context(widget);
    gtk_style_context_add_class(ctx, class_name);
}

/* ── Activation ───────────────────────────────────── */

static void
activate(GtkApplication *app, gpointer user_data)
{
    GtkWidget *window;
    GtkWidget *main_box;
    GtkWidget *box_temp;
    GtkWidget *box_target;
    GtkWidget *btn_inc;
    GtkWidget *btn_dec;
    GtkWidget *btn_target;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "MRTD Control");

#ifdef __arm__
    gtk_window_fullscreen(GTK_WINDOW(window));
#else
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
#endif

    /* CSS */
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "../src/hmi/gtk.css", NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);

    /* main_box horizontale */
    main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 40);
    gtk_widget_set_halign(main_box, GTK_ALIGN_FILL);
    gtk_widget_set_valign(main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(main_box, TRUE);
    gtk_widget_set_vexpand(main_box, TRUE);

    /* box_temp */
    box_temp = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(box_temp, GTK_ALIGN_START);
    gtk_widget_set_margin_start(box_temp, 40);
    gtk_widget_set_valign(box_temp, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(box_temp, TRUE);
    gtk_widget_set_vexpand(box_temp, TRUE);

    /* box_target */
    box_target = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(box_target, GTK_ALIGN_END);
    gtk_widget_set_margin_end(box_target, 40);
    gtk_widget_set_valign(box_target, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(box_target, TRUE);
    gtk_widget_set_vexpand(box_target, TRUE);

    /* Labels */
    label_temp   = gtk_label_new("Temp: --.- °C");
    label_target = gtk_label_new("Target: --");

    /* Bouton + */
    btn_inc = gtk_button_new_with_label(" + Temp ");
    add_css_class(btn_inc, "big");
    gtk_widget_set_hexpand(btn_inc, TRUE);
    gtk_widget_set_vexpand(btn_inc, TRUE);
    g_signal_connect(btn_inc, "clicked", G_CALLBACK(on_inc_temp_clicked), NULL);

    /* Bouton - */
    btn_dec = gtk_button_new_with_label(" - Temp ");
    add_css_class(btn_dec, "big");
    g_signal_connect(btn_dec, "clicked", G_CALLBACK(on_dec_temp_clicked), NULL);

    /* Bouton cible */
    btn_target = gtk_button_new_with_label(" Next Target ");
    add_css_class(btn_target, "big");
    g_signal_connect(btn_target, "clicked", G_CALLBACK(on_target_temp_clicked), NULL);

    /* Arbre widgets */
    gtk_box_pack_start(GTK_BOX(box_temp),   label_temp,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_temp),   btn_inc,      TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(box_temp),   btn_dec,      TRUE,  TRUE,  0);

    gtk_box_pack_start(GTK_BOX(box_target), label_target, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_target), btn_target,   TRUE,  TRUE,  0);

    gtk_box_pack_start(GTK_BOX(main_box),   box_temp,     TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(main_box),   box_target,   TRUE,  TRUE,  0);

    gtk_container_add(GTK_CONTAINER(window), main_box);
    gtk_widget_show_all(window);
}

/* ── Point d'entrée UI ────────────────────────────── */

int ui_run(int argc, char **argv)
{
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
