#ifndef CORE_H
#define CORE_H
#include <pthread.h> 
#include <stdbool.h>
#include <gtk/gtk.h>

/*  
 Pour préserver la lecture écriture de la carte SD nous allons stocker les fichiers
 temporaires dans une partie dédié de la ram.
*/
#define MRTD_PNG_PATH "/dev/shm/mrtd_result.png"
#define MRTD_DAT_PATH "/dev/shm/mesure.dat"
#define MRTD_GP_PATH  "/dev/shm/plot.gp"
#define MRTD_PDF_PATH "rapport_mrtd.pdf"

#define LOG_MSG(fmt, ...) \
    g_message("[%s | tid=%lu] " fmt, __func__, pthread_self(), ##__VA_ARGS__) //macro pour affichage de log

typedef enum
{
    MENU,
    MANUAL,
    AUTO,
    EXPORT
} ProgramMode;

typedef enum
{
    IDLE,
    CONNECT,
    COMMUNICATION,
    SHUTDOWN
} ServiceGpib;

typedef enum {
    PARSE,   // User a sélectionné profil + appuyé "Mode Manuel"
    SAVE,
    UNDO,
    TABLE,
    GRAPH,
    STOP
} MrtdCmd;

// Structure pour les données brutes du matériel
typedef struct {
    int             ud;             // Unit Descriptor GPIB
    bool            temp_ready;
    float           actual_dt;      // Delta entre target et emitter
    double          target_temp;    // Température cible
    double          emitter_temp;   // Température réelle
    int             target_index;   // Position de la roue
} GpibData;

typedef struct {
    guint  hold_timer_id;
    gint   hold_duration;
    float  temp_set_point;
    float  direction;
} BtnTemp;

typedef struct {
    float target;  // Ton tgt_axes (ex: la fréquence spatiale ou l'index)
    float delta_t; // Ton dt_axes (ex: la différence de température)
} MrtdPoint;

// Structure de contexte global pour l'application
typedef struct {
    char profiles_path[256];           //Est utilisé une fois à l'init pour stocker le path si dossier profiles présents
    char *selected_profile_path;   //le profil sélectionné par le user

    char *asset_name; 
    char *graph_png_path; 
    MrtdPoint mrtd_results[12]; // pointeurs vers un tableau, permet de définir la taille dynamiquement selon le profil chargé et de le partager à travers les différentes fonctions de l'application (ex: save, export, etc)
    int results_count; 


    ServiceGpib     service_gpib;      // Partagé entre thread service et watchdog
    bool            shutdown_requested;
    bool            device_online;
    pthread_mutex_t mutex;              // Protection des accès 
    pthread_cond_t  cond;               // Signal de synchronisation 
    GpibData        device_status;      // Données de mesure GLOBALES, a utiliser via MUTEX
    BtnTemp         btn_hold;           // Pour logique de bouton d'incrémentation 
    ProgramMode     mode;               // indique le mode actuel de l'hmi (Non protégé)
    GAsyncQueue     *gpib_queue;        // Queu de commande threadsafe ui.c -> service.c
    GAsyncQueue     *MRTD_queue;        // Queu de commande threadsafe service.c -> ui.c (ex: log, update labels, etc)
} AppData;


gboolean change_window(gpointer data);
void mrtd_cmd_queu(AppData *app, MrtdCmd cmd);
void app_set_service_gpib(AppData *app, ServiceGpib service);
gboolean hmi_log_append_idle(gpointer data);
gboolean set_status_online(gpointer data);
gboolean set_status_offline(gpointer data);
int hmi_init(int *argc, char ***argv, AppData *app);
void* thread_service_gpib(void* arg);
void* thread_handler_watchdog(void* arg);
void* thread_service_MRTD(void* arg);
int   thread_export(AppData *app);
#endif
