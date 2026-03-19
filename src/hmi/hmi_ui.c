#include <gtk/gtk.h>

/*
 * Callback function for the increment temperature button
 */
static void
on_inc_temp_clicked(GtkWidget *widget, gpointer user_data)
{
    // TODO: plus tard -> appeler gpib_set_temp(current + step);
    g_print("Augmenter température\n");
}

static void
on_dec_temp_clicked(GtkWidget *widget, gpointer user_data)
{
    // TODO: plus tard -> appeler gpib_set_temp(current - step);
    g_print("Diminuer température\n");
}

static void
on_target_temp_clicked(GtkWidget *widget, gpointer user_data)
{
    // TODO: plus tard -> appeler gpib_next_target(target);
    g_print("Prochaine cible demandée...\n");
}

static void
activate (GtkApplication *app, gpointer user_data)
{
    GtkWidget *window;
    GtkWidget *box_temp;
    GtkWidget *box_target;
    GtkWidget *btn_inc;
    GtkWidget *btn_dec;
    GtkWidget *btn_target;

    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "MRTD Control");

#ifdef __arm__
    gtk_window_fullscreen(GTK_WINDOW(window));
#else
    gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
#endif

    /* Appliquer le style CSS */
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "../src/hmi/gtk.css");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);

    /* box_temp */
    box_temp = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(box_temp, GTK_ALIGN_START);
    gtk_widget_set_margin_start(box_temp, 40);
    gtk_widget_set_valign(box_temp, GTK_ALIGN_CENTER);

    /* box_target */
    box_target = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(box_target, GTK_ALIGN_END);
    gtk_widget_set_margin_start(box_target, 40);
    gtk_widget_set_valign(box_target, GTK_ALIGN_CENTER);

    /* bouton + */
    btn_inc = gtk_button_new_with_label(" + Temp ");
    gtk_widget_add_css_class(btn_inc, "big");
    g_signal_connect(btn_inc, "clicked",
                     G_CALLBACK(on_inc_temp_clicked), NULL);

    /* bouton - */
    btn_dec = gtk_button_new_with_label(" - Temp ");
    gtk_widget_add_css_class(btn_dec, "big");
    g_signal_connect(btn_dec, "clicked",
                     G_CALLBACK(on_dec_temp_clicked), NULL);
    
    /* bouton cible */
    btn_target = gtk_button_new_with_label(" Next Target ");
    gtk_widget_add_css_class(btn_target, "big");
    g_signal_connect(btn_target, "clicked",
                     G_CALLBACK(on_target_temp_clicked), NULL);

    gtk_box_append(GTK_BOX(box_temp), btn_inc);
    gtk_box_append(GTK_BOX(box_temp), btn_dec);
    gtk_box_append(GTK_BOX(box_target), btn_target);

    gtk_window_set_child(GTK_WINDOW(window), box_temp);
    gtk_window_set_child(GTK_WINDOW(window), box_target);
    gtk_window_present(GTK_WINDOW(window));
}

int ui_run(int argc, char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}