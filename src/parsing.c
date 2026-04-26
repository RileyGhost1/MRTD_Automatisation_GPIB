#include <stdio.h> 
#include <cjson/cJSON.h>
#include "measure.h"
#include "core.h"
/* ------------------------------------------------------------------------------------------
This file is providing mrtd_load_profile(AppData *app) API to MRTD service for json profile parsing
   ------------------------------------------------------------------------------------------ */

/*
PUBLIC FUNCTION - Used by Mrtd service thread to load the selected profile localy to thread
*/
int mrtd_load_profile(AppData *app, MrtdProfile *out) {
    char path[256] = {0};

    pthread_mutex_lock(&app->mutex);
    strncpy(path, app->selected_profile_path, sizeof(path) - 1);
    pthread_mutex_unlock(&app->mutex);

    if (profile_parse(path, out) < 0) {
        g_log("MRTD", G_LOG_LEVEL_WARNING, "Erreur parsing: '%s' (len=%zu)", path, strlen(path));
        g_idle_add(hmi_log_append_idle, strdup("[MRTD] Erreur parsing profil."));
        return -1;
    }

    profile_debug_print(out);
    g_idle_add(hmi_log_append_idle, strdup("[MRTD] Profil chargé avec succès."));
    LOG_MSG("[MRTD] Profil chargé: %s",out->name);
    return 0;
}

/*
PRIVATE FUNCTION - Open the filepath of selected profile from UI and parse the .json -> c
*/
int profile_parse(const char *filepath, MrtdProfile *out)
{
    memset(out, 0, sizeof(MrtdProfile));

    FILE *f = fopen(filepath, "r");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    char *buf = malloc(len + 1);
    if (!buf) {
        fclose(f);
        return -1;
    }

    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        const char *err = cJSON_GetErrorPtr();
        if (err) fprintf(stderr, "[PROFILE] Erreur JSON avant: %.20s\n", err);
        free(buf);
        return -1;
    }
    free(buf);

    cJSON *name = cJSON_GetObjectItem(root, "name");
    if (!cJSON_IsString(name)) {
        cJSON_Delete(root);
        return -1;
    }
    strncpy(out->name, name->valuestring, sizeof(out->name) - 1);

    cJSON *targets = cJSON_GetObjectItem(root, "targets");
    if (!cJSON_IsArray(targets)) {
        cJSON_Delete(root);
        return -1;
    }

    cJSON *samples = cJSON_GetObjectItem(root, "samples_per_frequency");
    if (!cJSON_IsNumber(samples)) {
        cJSON_Delete(root);
        return -1;
    }

    out->samples_per_frequency = samples->valueint;

    if (out->samples_per_frequency <= 0 || out->samples_per_frequency > MAX_SAMPLES) {
        cJSON_Delete(root);
        return -1;
    }

    out->target_count = 0;
    cJSON *t;
    cJSON_ArrayForEach(t, targets) {
        if (out->target_count >= MAX_TARGETS) break;

        MrtdTarget *mt = &out->targets[out->target_count];

        cJSON *id = cJSON_GetObjectItem(t, "target_id");
        cJSON *wp = cJSON_GetObjectItem(t, "wheel_position");
        cJSON *sf = cJSON_GetObjectItem(t, "spatial_frequency_cpm");
        cJSON *dt = cJSON_GetObjectItem(t, "dt_setpoint_c");

        if (!cJSON_IsString(id) || !cJSON_IsNumber(wp) || !cJSON_IsNumber(sf)) {
            cJSON_Delete(root);
            return -1;
        }

        strncpy(mt->target_id, id->valuestring, sizeof(mt->target_id) - 1);
        mt->wheel_position = wp->valueint;
        mt->spatial_frequency_cpm = (float)sf->valuedouble;
        mt->dt_setpoint_c = (!dt || cJSON_IsNull(dt)) ? -1.0f : (float)dt->valuedouble;

        out->target_count++;
    }
    
    cJSON_Delete(root);
    return (out->target_count > 0) ? 0 : -1;
}

/*
PRIVATE FUNCTION - Simply display in cmd the result of parsing
*/
void profile_debug_print(const MrtdProfile *p) {
    printf("[PROFILE] name=%s targets=%d samples_per_frequency=%d", p->name, p->target_count, p->samples_per_frequency);
    for (int i = 0; i < p->target_count; i++) {
        const MrtdTarget *t = &p->targets[i];
        printf("  [%s] pos=%d freq=%.2f dt=%.1f enabled=%d\n",
            t->target_id,
            t->wheel_position,
            t->spatial_frequency_cpm,
            t->dt_setpoint_c,
            t->enabled);
    }

}
