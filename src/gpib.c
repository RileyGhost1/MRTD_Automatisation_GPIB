#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gpib/ib.h>
#include "gpib.h"
#include "core.h"

#define GPIB_BUFFER_SIZE 256
#define SR80_TEMP_SET_BIT (1 << 3)
#define POLLING_LOG 0                   /* 1 = actif, 0 = inactif, modifiable sans risque */

/* --- Fonctions de communication Bas Niveau --- */

int gpib_write(const char *command, int ud) {
    if (ud < 0) return -1;

    #if POLLING_LOG
        printf("Sending: %s\n", command);
    #endif

    ibwrt(ud, command, strlen(command));
    if (ThreadIbsta() & ERR) {
        fprintf(stderr, "ibwrt error\n");
        return -1;
    }
    return 0;
}

int gpib_read(char *response, int ud) {
    memset(response, 0, GPIB_BUFFER_SIZE);
    ibrd(ud, response, GPIB_BUFFER_SIZE - 1);
    if (ThreadIbsta() & ERR) {
        fprintf(stderr, "ibrd error\n");
        return -1;
    } 
    return 0;   
}

int gpib_write_read(const char *command, char *response, int ud) {
    if (gpib_write(command, ud) < 0) return -1;
    return gpib_read(response, ud);
}

void gpib_is_temp_ready(int ud,GpibData *out_data)
{
    char    raw    = 0;
    uint8_t status;

    ibrsp(ud, &raw);
    status = (uint8_t)raw;

    out_data->temp_ready = (status & SR80_TEMP_SET_BIT) != 0;

    #if POLLING_LOG
        printf("parsed polling byte: %x\n",status);
        printf("Temp Ready bit parsed: %d\n",out_data->temp_ready);
    #endif
}                                       

/* --- Initialisation --- */

int gpib_init(int master_addr, int dev_addr) {
    const int sad      = 0;
    const int send_eoi = 1;
    const int eos_mode = 0;
    const int timeout  = T3s;
    char response[GPIB_BUFFER_SIZE] = "";

    printf("Trying to open pad=%i on /dev/gpib%i...\n", dev_addr, master_addr);

    int ud = ibdev(master_addr, dev_addr, sad, timeout, send_eoi, eos_mode);
    if (ud < 0) {
        fprintf(stderr, "ibdev failed\n");
        return -1;
    }

    if (gpib_write_read("RV", response, ud) < 0) {
        fprintf(stderr, "Device not responding\n");
        ibonl(ud, 0);
        return -1;
    }

    if (strstr(response, "SR80") == NULL) {
        fprintf(stderr, "Unexpected device: %s\n", response);
        ibonl(ud, 0);
        return -1;
    }

    printf("SR80 trouvé: %s\n", response);

    if (gpib_write("CM2", ud) < 0) {
        fprintf(stderr, "Échec commande CM2\n");
        ibonl(ud, 0);
        return -1;
    }

    if (gpib_write("ST00.00", ud) < 0) { 
        fprintf(stderr, "Échec commande ST00.00\n");
        ibonl(ud, 0);
        return -1;
    }

    return ud;
}

/* --- Logique de Polling --- */

// Note : out_data est ici une structure locale au thread (snapshot)
int gpib_read_all(int ud, GpibData *out_data) {
    char response[GPIB_BUFFER_SIZE];

    if (gpib_write_read("RD", response, ud) < 0) return -1;
    out_data->actual_dt = atof(response);

    if (gpib_write_read("RT", response, ud) < 0) return -1;
    out_data->target_temp = atof(response);

    if (gpib_write_read("RE", response, ud) < 0) return -1;
    out_data->emitter_temp = atof(response);

    if (gpib_write_read("RA", response, ud) < 0) return -1;
    out_data->target_index = atoi(response);

    return 0;
}
