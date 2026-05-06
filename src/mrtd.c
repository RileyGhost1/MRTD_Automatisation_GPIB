
#include <math.h>
#include "core.h"
#include "measure.h"


/*  Pour préserver la lecture écriture de la carte SD nous allons stocker les fichiers
    temporaires dans une partie dédié de la ram.
*/
#define MRTD_PNG_PATH "/dev/shm/mrtd_result.png"
#define MRTD_DAT_PATH "/dev/shm/mesure.dat"
#define MRTD_GP_PATH  "/dev/shm/plot.gp"
/*
PARSE
*/

void mrtd_init_sequence(AppData *app, GpibData *context, MrtdProfile *profile) {
    GAsyncQueue *q = app->gpib_queue;

    if (profile->target_count == 0) {
        LOG_MSG("[MRTD] Profil vide");
        return;
    }

    // 1. Roue
    if (context->target_index != profile->targets[0].wheel_position) {
        gchar *cmd = g_strdup_printf("SA%d", profile->targets[0].wheel_position);
        g_async_queue_push(q, cmd);
    }

    // 2. Température
    if (profile->targets[0].dt_setpoint_c >= 0.0f) {
        gchar *cmd = g_strdup_printf("ST%.2f", profile->targets[0].dt_setpoint_c);
        g_async_queue_push(q, cmd);
    }
}


/*
SAVE
*/
void mrtd_cmd_save(AppData *app, GpibData *context, MrtdProfile *profile, MrtdMeasure measures[MAX_TARGETS][MAX_SAMPLES][2])
{
    // 1. Déduire la position courante depuis le tableau
    int current_target = -1;
    int current_sample = -1;

    LOG_MSG("[MRTD] profile: targets=%d, samples=%d",
        profile->target_count,
        profile->samples_per_frequency);

    LOG_MSG("[MRTD] Before store: POS valid=%d NEG valid=%d",
    measures[current_target][current_sample][POLARITY_POS].is_valid,
    measures[current_target][current_sample][POLARITY_NEG].is_valid);
    

    for (int t = 0; t < profile->target_count; t++) {
        for (int s = 0; s < profile->samples_per_frequency; s++) {
            if (!measures[t][s][POLARITY_POS].is_valid ||
                !measures[t][s][POLARITY_NEG].is_valid) {
                current_target = t;
                current_sample = s;
                break;
            }
        }

        if (current_target >= 0) {
            break;
        }
    }

    LOG_MSG("[MRTD] Actual target is %d, actual sample is %d", current_target, current_sample);

    // Test complet — rien à faire
    if (current_target < 0) {
        LOG_MSG("[MRTD] Tableau complet, aucune case disponible");
        return;
    }

    // 2. Vérifier qu'on est sur la bonne cible
    if (context->target_index != profile->targets[current_target].wheel_position) {
        LOG_MSG("[MRTD] Mauvaise cible, entrée ignorée (attendu %d, actuel %d)",
                profile->targets[current_target].wheel_position,
                context->target_index);
        g_idle_add(hmi_log_append_idle, strdup("[MRTD] Mauvaise cible, entrée ignorée."));
        return;
    }

    LOG_MSG("[MRTD] Before store: POS valid=%d NEG valid=%d",
            measures[current_target][current_sample][POLARITY_POS].is_valid,
            measures[current_target][current_sample][POLARITY_NEG].is_valid);

    int polarity = !measures[current_target][current_sample][POLARITY_POS].is_valid
                ? POLARITY_POS
                : POLARITY_NEG;

    measures[current_target][current_sample][polarity].temperature_delta = context->actual_dt;
    measures[current_target][current_sample][polarity].is_valid = true;

    LOG_MSG("[MRTD] After store: POS valid=%d NEG valid=%d",
            measures[current_target][current_sample][POLARITY_POS].is_valid,
            measures[current_target][current_sample][POLARITY_NEG].is_valid);
            
    LOG_MSG("[MRTD] Stored T[%d][%d][%s] = %.2f",
            current_target, current_sample,
            polarity == POLARITY_POS ? "POS" : "NEG",
            context->actual_dt);

    // 5. Sample complet ?
    if (!measures[current_target][current_sample][POLARITY_POS].is_valid ||
        !measures[current_target][current_sample][POLARITY_NEG].is_valid)
        return;

    // 6. Target complet ?
    if (current_sample + 1 < profile->samples_per_frequency)
        return;

    // 7. Autres targets ?
    if (current_target + 1 < profile->target_count) {
        gchar *cmd = g_strdup_printf("SA%d", profile->targets[current_target + 1].wheel_position);
        g_async_queue_push(app->gpib_queue, cmd);
        LOG_MSG("[MRTD] Rotation vers cible %d", profile->targets[current_target + 1].wheel_position);
        return;
    }

    // 8. Test complet
    LOG_MSG("[MRTD] Test complet");
    resume_mrtd(profile, measures);
    g_idle_add(hmi_log_append_idle, strdup("[MRTD] Test complet. Prêt pour export."));
}

/*
TABLE
*/

void mrtd_cmd_table(MrtdMeasure measures[MAX_TARGETS][MAX_SAMPLES][2])
{
    LOG_MSG("========== MRTD TABLE ==========");

    for (int t = 0; t < MAX_TARGETS; t++) {
        LOG_MSG("Target [%d]", t);

        for (int s = 0; s < MAX_SAMPLES; s++) {
            LOG_MSG("  Sample [%d] | POS: valid=%d dt=%.2f | NEG: valid=%d dt=%.2f",
                    s,
                    measures[t][s][POLARITY_POS].is_valid,
                    measures[t][s][POLARITY_POS].temperature_delta,
                    measures[t][s][POLARITY_NEG].is_valid,
                    measures[t][s][POLARITY_NEG].temperature_delta);
        }
    }

    LOG_MSG("================================");
}

/*
App for send commande to gpib thread
*/

void mrtd_cmd_undo_last_profile(AppData *app,
                                GpibData *context,
                                MrtdProfile *profile,
                                MrtdMeasure measures[MAX_TARGETS][MAX_SAMPLES][2])
{


    for (int t = profile->target_count - 1; t >= 0; t--) {
        for (int s = profile->samples_per_frequency - 1; s >= 0; s--) {
            for (int p = POLARITY_NEG; p >= POLARITY_POS; p--) {
                if (measures[t][s][p].is_valid) {
                    measures[t][s][p].is_valid = false;
                    measures[t][s][p].temperature_delta = 0.0f;

                    if (context->target_index != profile->targets[t].wheel_position) {
                        gchar *cmd = g_strdup_printf("SA%d", profile->targets[t].wheel_position);
                        g_async_queue_push(app->gpib_queue, cmd);
                        LOG_MSG("[MRTD] Rotation vers cible %d pour undo", profile->targets[t].wheel_position);
                    }

                    LOG_MSG("[MRTD] Undo: T[%d][%d][%s] supprimé",
                            t, s, p == POLARITY_POS ? "POS" : "NEG");
                    return;
                }
            }
        }
    }

    LOG_MSG("[MRTD] Undo impossible: aucune mesure valide");
}

void resume_mrtd(const MrtdProfile *profile,
                 MrtdMeasure measures[MAX_TARGETS][MAX_SAMPLES][2])
{
    if (!profile || profile->target_count <= 0 || profile->samples_per_frequency <= 0) {
        LOG_MSG("[MRTD] Profil invalide pour calcul");
        return;
    }

    float dt_axes[MAX_TARGETS];
    float tgt_axes[MAX_TARGETS];
    int valid_points = 0;

    for (int t = 0; t < profile->target_count; t++) {
        float samples_average = 0.0f;
        int valid_samples = 0;
        float target_size = profile->targets[t].spatial_frequency_cpm;

        for (int s = 0; s < profile->samples_per_frequency; s++) {
            if (measures[t][s][POLARITY_POS].is_valid &&
                measures[t][s][POLARITY_NEG].is_valid) {
                float pos_dt = measures[t][s][POLARITY_POS].temperature_delta;
                float neg_dt = measures[t][s][POLARITY_NEG].temperature_delta;
                samples_average += fabsf((pos_dt - neg_dt) / 2.0f);
                valid_samples++;
            } else {
                LOG_MSG("[MRTD] Mesure manquante Target %d Sample %d, ignorée", t, s);
            }
        }

        if (valid_samples == 0) {
            LOG_MSG("[MRTD] Aucun sample valide pour Target %d, point ignoré", t);
            continue;
        }

        samples_average /= (float)valid_samples;
        samples_average *= 1000.0f; // Convertir en mK
        dt_axes[valid_points]  = samples_average;
        tgt_axes[valid_points] = target_size;

        LOG_MSG("[MRTD] Point %d: freq=%.3f cy/mrad, dt=%.3f mK",
                valid_points, target_size, samples_average);
        valid_points++;
    }

    if (valid_points == 0) {
        LOG_MSG("[MRTD] Aucun point valide, graphe non généré");
        return;
    }

    for (int i = 0; i < valid_points; i++)
        LOG_MSG("%d. MRTD: x=%.3f cy/mrad, y=%.3f mK", i, tgt_axes[i], dt_axes[i]);

    if(draw_mrtd_graph(dt_axes, tgt_axes, valid_points) != 0) {
        LOG_MSG("[MRTD] Erreur lors de la génération du graphe");
    }

}


/*On trace la courbe MRTD depuis GNUPLOT en stockant d'abord les coordonnées de nos points dans un fichier dédié*/
int draw_mrtd_graph(const float *dt_axes, const float *tgt_axes, int count)
{
    if (!dt_axes || !tgt_axes || count <= 0) return -1;

        FILE *data = fopen(MRTD_DAT_PATH, "w");
    if (!data) {
        perror("fopen mesure.dat");
        return -1;
    }

    for (int i = 0; i < count; i++) {
        LOG_MSG("Point %d: x=%.3f cy/mrad, y=%.3f mK", i, tgt_axes[i], dt_axes[i]);
        fprintf(data, "%f %f\n", tgt_axes[i], dt_axes[i]); // Fréquence spatiale en x, delta de température en y
    }
    fclose(data);

    FILE *gp = fopen(MRTD_GP_PATH, "w");
    if (!gp) {
        perror("fopen plot.gp");
        return -1;
    }

    fprintf(gp, "set terminal pngcairo size 1200,700 enhanced font \"Arial,12\"\n");
    fprintf(gp, "set output \"%s\"\n", MRTD_PNG_PATH);
    fprintf(gp, "set grid\n");
    fprintf(gp, "set logscale y\n"); // Axe Y en échelle logarithmique selon le stanag 4349
    fprintf(gp, "set xlabel \"Spatial frequency (cycles/mrad)\"\n");
    fprintf(gp, "set ylabel \"Temperature delta (mK)\"\n");
    // Syntaxe: set offsets <left>, <right>, <top>, <bottom>
    // 'graph 0.1' signifie 10% de la largeur totale du graphique.
    fprintf(gp, "set offsets graph 0.01, graph 0.01, graph 0.0, graph 0.0\n");
    fprintf(gp, "plot \"%s\" using 1:2 with points pt 7 ps 1.5 lc rgb \"black\" title \"Measure\", "
                "\"%s\" using 1:2 with lines lw 2 lc rgb \"blue\" notitle, "
                "\"%s\" using 1:2:(sprintf(\"%%.2f, %%.1f\", $1, $2)) with labels offset 1,1 font \"Arial,10\" tc rgb \"red\" notitle\n", 
                MRTD_DAT_PATH, MRTD_DAT_PATH, MRTD_DAT_PATH);

    fprintf(gp, "set output\n");

    fclose(gp);

    int ret = system("gnuplot " MRTD_GP_PATH);
    if (ret != 0) {
        fprintf(stderr, "Erreur gnuplot, code=%d\n", ret);
        return -1;
    }

    if (update_hdmi_display(MRTD_PNG_PATH) != 0) {
        LOG_MSG("[MRTD] Erreur lors de la mise à jour de l'affichage HDMI");
    }

    return 0;
}

/*Permet d'afficher des images sur le moniteur hdmi*/
int update_hdmi_display(const char *image_path) {
    // 1. Tuer l'ancienne instance
    
    system("pkill feh"); 

    // 2. Construire la commande avec redirection des erreurs vers un fichier log
    char command[512];
    snprintf(command, sizeof(command), 
             "DISPLAY=:0 feh --borderless --geometry 1680x1050+0+0 --hide-pointer -Z %s > /tmp/feh_debug.log 2>&1 &", 
             image_path);
             
    // 3. Lancer l'affichage et récupérer le code de retour système
    int ret = system(command);
    
    // 4. Afficher dans la console de ton application C
    LOG_MSG("[MRTD DEBUG] Lancement de feh (retour système : %d)\n", ret);
    LOG_MSG("[MRTD DEBUG] Fichier ciblé : %s\n", image_path);
    return ret;
}

/*Permet de caché l'image affichée sur l'écran HDMI*/
void clear_hdmi_display(void) {
    // Tuer l'instance de feh
    int ret = system("pkill feh");
    
    // Un retour système différent de 0 signifie juste que feh n'était pas en cours d'exécution
    if (ret == 0) {
        LOG_MSG("[MRTD DEBUG] Affichage HDMI nettoyé (feh fermé).");
    } else {
        LOG_MSG("[MRTD DEBUG] Affichage HDMI déjà vide.");
    }
    
    // (Optionnel) Supprimer le fichier en RAM pour être 100% clean
    // system("rm -f /dev/shm/mrtd_result.png"); 
}