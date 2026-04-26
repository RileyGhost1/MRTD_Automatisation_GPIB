#ifndef MEASURE_H
#define MEASURE_H

#include "core.h"

#define MAX_TARGETS 12
#define MAX_SAMPLES 3

typedef enum {
    ORIENT_HORIZONTAL,
    ORIENT_VERTICAL
} Orientation;

typedef enum {
    POLARITY_POS = 0,
    POLARITY_NEG = 1
} Polarity;

//
typedef struct {
    char        target_id[8];           // "F1", "F2"...
    bool        enabled;
    int         wheel_position;         // Position roue 1-12
    float       spatial_frequency_cpm;  // cy/mrad
    Orientation orientations;
    int         orientation_count;
    float       dt_setpoint_c;          // -1.0f si null
    char        notes[128];
} MrtdTarget; //struct utiliser pour stocker les informations de chaque target presente dans le profil MRTD en json

//Permet de stocker les informations d'un profil MRTD chargé depuis un fichier JSON
typedef struct {
    char        name[64];
    int         samples_per_frequency;
    int         target_count; //stock le nombre total de target présente dans le profil, doit être <= MAX_TARGETS
    MrtdTarget  targets[MAX_TARGETS]; // utilise target_count pour naviger dans le tableau qui stock les différentes target du profil
} MrtdProfile;

// ce type est utiliser par le tableau qui stocks chaque mesure qui contient la valeurs dt pour chauque sample/cible et la validité de la case (si la mesure a été réalisé ou pas)
typedef struct {
    float   temperature_delta;   // ΔT mesuré en °C (valeur brute du corps noir)
    bool    is_valid;            // false = case vide (pas encore mesurée)
} MrtdMeasure; 


void mrtd_init_sequence(AppData *user_data, GpibData *context, MrtdProfile *profile);
void mrtd_cmd_save(AppData *app, GpibData *context, MrtdProfile *profile, MrtdMeasure measures[MAX_TARGETS][MAX_SAMPLES][2]);
void mrtd_cmd_table(MrtdMeasure measures[MAX_TARGETS][MAX_SAMPLES][2]);
void mrtd_cmd_undo_last_profile(AppData *app, GpibData *context, MrtdProfile *profile, MrtdMeasure measures[MAX_TARGETS][MAX_SAMPLES][2]);
int mrtd_load_profile(AppData *app, MrtdProfile *out);
static int profile_parse(const char *filepath, MrtdProfile *out);
static void profile_debug_print(const MrtdProfile *p);

#endif