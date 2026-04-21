#ifndef GPIB_H
#define GPIB_H

#include <gtk/gtk.h>
#include <pthread.h>
#include "core.h" // Nécessaire pour les types AppData et GpibData

/* --- Initialisation et Fermeture --- */

/**
 * Initialise le device GPIB.
 * @param master_addr : Adresse de la carte (généralement 0)
 * @param dev_addr    : Adresse de l'instrument (SR80)
 * @param ud_out      : Pointeur pour récupérer le descripteur d'unité (UD)
 */
int gpib_init(int master_addr, int dev_addr);
void gpib_is_temp_ready(int ud, GpibData *out_data);

/**
 * Ferme la connexion GPIB proprement.
 * @param ud : Le descripteur d'unité à fermer
 */
void gpib_close(int ud);

/* --- Fonctions de Communication (Bas niveau) --- */

int gpib_write(const char *command, int ud);
int gpib_read(char *response, int ud);
int gpib_write_read(const char *command, char *response, int ud);

/* --- Fonctions de Service (Haut niveau) --- */

/**
 * Lit toutes les données du SR80 et remplit une structure de snapshot.
 * @param ud       : Le descripteur d'unité
 * @param out_data : Pointeur vers la structure de destination (locale au thread)
 */
int gpib_read_all(int ud, GpibData *out_data);

/* --- Contrôles spécifiques (Exemples) --- */

int gpib_temp_inc(int ud);
int gpib_temp_dec(int ud);
int gpib_next_target(int ud);

#endif