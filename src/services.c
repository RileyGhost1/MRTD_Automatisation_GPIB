#include <stdio.h>
#include <unistd.h>
#include <gpib/ib.h>
#include "core.h"
#include "gpib.h"

#define POLLING_DELAY_US 500000 // 500ms de délai entre les polls pour éviter de saturer le CPU et le bus GPIB
#define WATCHDOG_DELAY_US 1000000 // 1s de délai pour le watchdog, ajustable selon les besoins

/*====================================================================================================*/
/* Ces fonctions on pour but d'aerer le code, permettent de simplifier la lecture et la logique de la machine d'etat du thread de service */



// Lis et exécute les commandes disponibles dans la queue, return si queue vide.
void gpib_cmd_queu(int ud, AppData *app)
{
    GAsyncQueue *q = app->gpib_queue;
    gchar       *cmd;

        while ((cmd = g_async_queue_try_pop(q)) != NULL) {
            gpib_write(cmd, ud);
            g_message("Commande envoyée : %s", cmd);
            g_free(cmd);
        }
    //g_message("Aucune commande dans la queue.");
}



// Change l'état de connexion et le mode de service en une fois (securise)
void app_set_connection_status(AppData *app, bool online, ServiceGpib state) {
    pthread_mutex_lock(&app->mutex);
    app->device_online = online;
    app->service_gpib = state;
    pthread_mutex_unlock(&app->mutex);
}

// Envoie un nouvel ordre depuis GTK ou le Watchdog (securise)
void app_set_service_gpib(AppData *app, ServiceGpib service) {
    pthread_mutex_lock(&app->mutex);
    app->service_gpib = service;
    pthread_cond_signal(&app->cond); // On réveille le service ici !
    pthread_mutex_unlock(&app->mutex);
}

// Réalise la tansmission du snapshot -> global (securise) pour que les autres threads puissent y accéder et que le mutex soit libere le plus tôt possible.
void global_data_transfer(AppData *app, GpibData *local)
{
    pthread_mutex_lock(&app->mutex);
    app->device_status = *local;   // Copie par valeur — sûr car que des scalaires
    pthread_mutex_unlock(&app->mutex);
}

// Récupère l'état pour le Watchdog ou l'UI (securise)
bool app_is_device_online(AppData *app) {
    bool status;
    pthread_mutex_lock(&app->mutex);
    status = app->device_online;
    pthread_mutex_unlock(&app->mutex);
    return status;
}

// Récupère l'état pour le Watchdog ou l'UI (securise)
bool app_check_shutdown_requested(AppData *app) {
    bool status;
    pthread_mutex_lock(&app->mutex);
    status = app->shutdown_requested;
    pthread_mutex_unlock(&app->mutex);
    return status;
}



/*====================================================================================================*/

/*
    *Thread qui gere toutes interactions avec l'hardware via GPIB. Travaille de pair avec le thread de watchdog pour la gestion des erreurs et la logique de l'application. 
     - En IDLE, il attend une commande du watchdog pour se connecter ou faire du polling.
     - En CONNECT, il tente de se connecter au SR80 et repasse en IDLE.
     - En COMMUNICATION, il réalise le polling tant que l'appareil est en ligne, sinon repasse en IDLE.
     - En SHUTDOWN, il ferme proprement la connexion GPIB et termine le thread.
*/
void* thread_service_gpib(void* arg) {
    ServiceGpib service_gpib; 
    AppData *app = (AppData*)arg;
    GpibData local_snapshot; // Stockage local pour le polling "hors-mutex", Structure local de type GpibData

    printf("Thread Service GPIB démarré.\n");

    while (1) {

        pthread_mutex_lock(&app->mutex);

        while((app->service_gpib == IDLE) && (!app->shutdown_requested))
        {
            pthread_cond_wait(&app->cond,&app->mutex); 
        }

        service_gpib = app->service_gpib;   //libere le mutex avant le switch

        pthread_mutex_unlock(&app->mutex);
        /*
            * Machine à états du service GPIB: Travaille en parallele avec le thread de watchdog. Le watchdog est la pour simplifier la logique du programme.
            * la tentative de connection se lance et repasse en IDLE, le polling continue tant que l'appareil est en ligne && si un mode est selectionne.
            * Si une erreur survient, le service repasse systematiquement en IDLE et attend que le watchdog gere la situation.
        */
        switch(service_gpib){

            case CONNECT:

                if((local_snapshot.ud = gpib_init(0,1)) < 0){
                    g_idle_add(hmi_log_append_idle, strdup("ERROR: Connection au SR80 échouée."));                          
                    app_set_connection_status(app, false, IDLE);                                                                                
                } else {                                                                                               
                    g_idle_add(hmi_log_append_idle, strdup("INFO: Connection au SR80 réussie."));
                    app_set_connection_status(app, true, IDLE); 
                }                            
                break;
                                                                                                  
            case COMMUNICATION:
                /* Vérification si une commande est présente, si pas de commande dans la queu -> break */        
                gpib_cmd_queu(local_snapshot.ud, app);

                if(app_is_device_online(app) == false){                                                                 
                    g_idle_add(hmi_log_append_idle, strdup("ERROR: Appareil hors ligne.\nImpossible de lancer le polling.\nRetour en IDLE.")); 
                    app_set_connection_status(app, false, IDLE);
                    break;
                } else if (gpib_read_all(local_snapshot.ud, &local_snapshot) < 0) {                                              
                    g_idle_add(hmi_log_append_idle, strdup("ERROR: Lecture GPIB échouée.\nRetour en IDLE."));           
                    app_set_connection_status(app, false, IDLE); 
                } else {
                    //app_set_connection_status(app, true, COMMUNICATION); //Décommenter si polling constant même dans le menu désiré. 
                    //g_idle_add(hmi_log_append_idle, strdup("INFO: Lecture GPIB OK.")); //POUR test
                    gpib_is_temp_ready(local_snapshot.ud, &local_snapshot);
                    global_data_transfer(app, &local_snapshot);
                    usleep(POLLING_DELAY_US); // Délai de polling pour éviter de saturer le CPU et le bus GPIB
                }                                                                                  
                break;     

            case SHUTDOWN:

                ibonl(local_snapshot.ud,0);                                     
                return NULL;

            default:

            fprintf(stderr, "[ERROR-SERVICE_GPIB ] État inconnu: %d", service_gpib);
            abort();           //ne doit techniquement jamais arriver ici.

        }
    }
    return NULL;
}

void* thread_handler_watchdog(void* arg) {
    AppData *app = (AppData*)arg;
    bool last_known_status = false;  // Evite le spam — n'envoie que sur changement

    while (!app_check_shutdown_requested(app)) {
        bool online = app_is_device_online(app);

        if (online) {
            if (ThreadIbsta() & (ERR | TIMO)) {
                // Erreur matérielle détectée
                app_set_connection_status(app, false, IDLE);
                g_idle_add(hmi_log_append_idle, strdup("ALERTE: Erreur matérielle détectée.\n"));
                online = false;  // Force le changement d'état
            }
        }

        // N'envoie à GTK que si l'état a changé
        if (online != last_known_status) {
            if (online) {
                g_idle_add(set_status_online, NULL);
            } else {
                g_idle_add(set_status_offline, NULL);
            }
            last_known_status = online;
        }

        usleep(WATCHDOG_DELAY_US); // Délai de vérification pour éviter de saturer le CPU
    }

    app_set_connection_status(app, false, SHUTDOWN);
    return NULL;
}

/* TODO: parsing complet du fichier json selectionne via la GTKComboBoxText, Si le parsin est reussis le thread copie localement les donnees du SR-80 via
la structure gpibdata (global) en utilisant un mutex. 
*/
void* thread_service_MRTD(void* arg) {
    AppData *app = (AppData*)arg; // structure globale de l'application (sous mutex)
    GpibData local_snapshot; // Structure local de type GpibData

    if (app == NULL) return NULL;


    //while(1){}
    //mettre en place un switch case pour les commandes envoye depuis l'UI (ex: lancement de profil, reset data, etc)
    //g_async_queue_try_pop(app->MRTD_queue); // Lire les commandes de l'UI
    
    pthread_mutex_lock(&app->mutex);
    local_snapshot = app->device_status;  // Copie par valeur — sûr car que des scalaires (Si char * ou tableau, il faudrait faire une copie profonde)
    pthread_mutex_unlock(&app->mutex);
    
}