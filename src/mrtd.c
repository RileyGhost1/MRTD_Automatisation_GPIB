
#include "core.h"
#include "measure.h"

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