#include <gpib/ib.h>
#include "gpib.h"

// Fonction générique lecture GPIB — envoie cmd, récupère réponse dans buf
static int gpib_query(int dev, const char *cmd, char *buf, size_t buf_size)
{
    ibwrt(dev, cmd, strlen(cmd));
    if (ibsta & ERR) {
        fprintf(stderr, "ibwrt error [%s]: iberr=%d\n", cmd, iberr);
        return -1;
    }

    ibrd(dev, buf, buf_size - 1);
    if (ibsta & ERR) {
        fprintf(stderr, "ibrd error [%s]: iberr=%d\n", cmd, iberr);
        return -1;
    }

    buf[ibcntl] = '\0';  // terminer la chaîne proprement
    return 0;
}

// Lecture de toutes les valeurs du contrôleur
int gpib_read_all(AppContext *ctx)
{
    char buf[64];

    // --- Température de la cible ---
    if (gpib_query(ctx->gpib_dev, "RT\r\n", buf, sizeof(buf)) == 0) {
        ctx->target_temp = atof(buf);
    }

    // --- Température de l'émetteur ---
    if (gpib_query(ctx->gpib_dev, "RE\r\n", buf, sizeof(buf)) == 0) {
        ctx->emitter_temp = atof(buf);
    }

    // --- Position cible (target wheel) ---
    if (gpib_query(ctx->gpib_dev, "RA\r\n", buf, sizeof(buf)) == 0) {
        ctx->current_target = atoi(buf);
    }

    // --- Ajouter ici d'autres lectures selon le manuel SR-80 ---

    return 0;
}