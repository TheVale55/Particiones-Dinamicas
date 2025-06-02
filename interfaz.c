#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

GtkWidget *entry_lineas;
GtkWidget *radio_first, *radio_best, *radio_worst;
GtkWidget *textview_espia;
GtkWidget *frame_productor;

// Función para ejecutar comandos de sistema
void ejecutar_comando(const char *comando) {
    system(comando);
}

// Callback del botón Inicializar
void on_inicializar_clicked(GtkButton *button, gpointer user_data) {
    const char *lineas = gtk_entry_get_text(GTK_ENTRY(entry_lineas));
    int n = atoi(lineas);

    if (n <= 0 || n > 1024) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                   "Cantidad inválida de líneas");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    char comando[128];
    snprintf(comando, sizeof(comando), "echo %d | ./inicializador", n);
    ejecutar_comando(comando);

    gtk_widget_set_sensitive(frame_productor, TRUE);
}

// Callback del botón Ejecutar Productor
void on_productor_clicked(GtkButton *button, gpointer user_data) {
    const char *algoritmo = "1";
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_best))) algoritmo = "2";
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_worst))) algoritmo = "3";

    char comando[128];
    snprintf(comando, sizeof(comando), "(echo %s | ./productor &) > /dev/null 2>&1", algoritmo);
    ejecutar_comando(comando);
}

// Callback del botón Espía
void on_espia_clicked(GtkButton *button, gpointer user_data) {
    FILE *fp = popen("./espia", "r");
    if (!fp) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview_espia));
    gtk_text_buffer_set_text(buffer, "", -1);

    char linea[256];
    while (fgets(linea, sizeof(linea), fp)) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_insert(buffer, &end, linea, -1);
    }
    pclose(fp);
}

// Callback del botón Finalizar
void on_finalizar_clicked(GtkButton *button, gpointer user_data) {
    ejecutar_comando("./finalizador");
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Simulador de Particiones Dinámicas");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 500);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Sección Inicializador
    GtkWidget *frame_init = gtk_frame_new("1. Inicializador");
    gtk_box_pack_start(GTK_BOX(vbox), frame_init, FALSE, FALSE, 0);
    GtkWidget *box_init = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_add(GTK_CONTAINER(frame_init), box_init);
    entry_lineas = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_lineas), "Líneas de memoria");
    GtkWidget *btn_init = gtk_button_new_with_label("Inicializar");
    g_signal_connect(btn_init, "clicked", G_CALLBACK(on_inicializar_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(box_init), entry_lineas, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box_init), btn_init, FALSE, FALSE, 5);

    // Sección Productor
    frame_productor = gtk_frame_new("2. Productor");
    gtk_box_pack_start(GTK_BOX(vbox), frame_productor, FALSE, FALSE, 0);
    GtkWidget *box_prod = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_add(GTK_CONTAINER(frame_productor), box_prod);
    radio_first = gtk_radio_button_new_with_label(NULL, "First-Fit");
    radio_best = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_first), "Best-Fit");
    radio_worst = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_first), "Worst-Fit");
    GtkWidget *btn_prod = gtk_button_new_with_label("Ejecutar Productor");
    g_signal_connect(btn_prod, "clicked", G_CALLBACK(on_productor_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(box_prod), radio_first, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box_prod), radio_best, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box_prod), radio_worst, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box_prod), btn_prod, FALSE, FALSE, 5);
    gtk_widget_set_sensitive(frame_productor, FALSE);

    // Sección Espía
    GtkWidget *frame_espia = gtk_frame_new("3. Espía");
    gtk_box_pack_start(GTK_BOX(vbox), frame_espia, TRUE, TRUE, 0);
    GtkWidget *box_espia = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(frame_espia), box_espia);
    GtkWidget *btn_espia = gtk_button_new_with_label("Mostrar Estado");
    g_signal_connect(btn_espia, "clicked", G_CALLBACK(on_espia_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(box_espia), btn_espia, FALSE, FALSE, 5);
    textview_espia = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview_espia), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), textview_espia);
    gtk_widget_set_size_request(scroll, -1, 200);
    gtk_box_pack_start(GTK_BOX(box_espia), scroll, TRUE, TRUE, 5);

    // Sección Finalizador
    GtkWidget *frame_fin = gtk_frame_new("4. Finalizador");
    gtk_box_pack_start(GTK_BOX(vbox), frame_fin, FALSE, FALSE, 0);
    GtkWidget *box_fin = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_add(GTK_CONTAINER(frame_fin), box_fin);
    GtkWidget *btn_fin = gtk_button_new_with_label("Finalizar Simulación");
    g_signal_connect(btn_fin, "clicked", G_CALLBACK(on_finalizar_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(box_fin), btn_fin, FALSE, FALSE, 5);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
