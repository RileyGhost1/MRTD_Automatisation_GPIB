#include <stdlib.h>
#include <stdio.h>
#include <gpib/ib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <unistd.h> 
#include "gpib.h"

#define GPIB_BUFFER_SIZE 256

ControllerState g_controller = {
    .target_temp   = 0.0,
    .emitter_temp  = 0.0,
    .target_index  = 0,
    .state         = MASTER_OFFLINE_DEVICE_OFFLINE,
    .last_error    = 0,
    .ud            = -1
};

/*
Public function to initialize the GPIB device
*/
int gpib_init(int master_addr, int dev_addr)
{
    const int sad      = 0;
    const int send_eoi = 1;
    const int eos_mode = 0;
    const int timeout  = T1s;
    char response[GPIB_BUFFER_SIZE];

    printf("Trying to open pad=%i on /dev/gpib%i...\n", dev_addr, master_addr);

    g_controller.ud = ibdev(master_addr, dev_addr, sad, timeout, send_eoi, eos_mode);
    if (g_controller.ud < 0)
    {
        fprintf(stderr, "ibdev failed\n");
        g_controller.state = MASTER_OFFLINE_DEVICE_OFFLINE;
        return -1;
    }

    g_controller.state = MASTER_ONLINE_DEVICE_OFFLINE;

    if (gpib_write_read("RV", response) < 0)
    {
        fprintf(stderr, "Device not responding (RV command failed)\n");
        ibonl(g_controller.ud, 0);
        g_controller.state = MASTER_ONLINE_DEVICE_OFFLINE;
        return -1;
    }

    /* Vérification que la réponse contient "SR80" */
    if (strstr(response, "SR80") == NULL)
    {
        fprintf(stderr, "Unexpected device response: %s\n expected response was SR80", response);
        ibonl(g_controller.ud, 0);
        g_controller.state = MASTER_ONLINE_DEVICE_OFFLINE;
        return -1;
    }

    printf("Corps noir SR80 confirmé — réponse: %s\n", response);
    g_controller.state = MASTER_ONLINE_DEVICE_ONLINE;
    return 0;
}
/*======================================================================================================*/

/*
Public function to close the GPIB device
*/
int gpib_close()
{

}
/*======================================================================================================*/

/*
Private function to check GPIB status before each action
*/
int gpib_check_status(const char *context) {


}
/*======================================================================================================*/

int gpib_write(const char *command)
{

	printf("sending string: %s\n", command);
    ibwrt(g_controller.ud, command, strlen(command));
	if((ThreadIbsta() & ERR))
	{
        fprintf(stderr, "ibwrt error\n");
		return -1;
	}
	return 0;
}
/*======================================================================================================*/

int gpib_read(char *response)
{
    memset(response, 0, GPIB_BUFFER_SIZE);
    ibrd(g_controller.ud, response, GPIB_BUFFER_SIZE - 1);
    if (ThreadIbsta() & ERR)
    {
        fprintf(stderr, "ibrd error\n");
        return -1;
    } 
    return 0;   
}
/*======================================================================================================*/

int gpib_write_read(const char *command, char *response)
{
    if (gpib_write(command) < 0) return -1;
    return gpib_read(response);
}
/*======================================================================================================*/

/* 
Read data from black body controller (asci), parse and store this data in ControllerState structure 
*/
int gpib_read_all()
{
    char response[GPIB_BUFFER_SIZE];

    if (gpib_write_read("RT", response) < 0) {
        fprintf(stderr, "Failed to read RT from GPIB device\n");
        return -1;
    }
    g_controller.target_temp = atof(response);
    printf("Target temperature: %f\n", g_controller.target_temp);

    if (gpib_write_read("RE", response) < 0) {
        fprintf(stderr, "Failed to read RE from GPIB device\n");
        return -1;
    } 
    g_controller.emitter_temp = atof(response);
    printf("Emitter temperature: %f\n", g_controller.emitter_temp);

    if (gpib_write_read("RA", response) < 0) {
        fprintf(stderr, "Failed to read RA from GPIB device\n");
        return -1;
    } 
    g_controller.target_index = atoi(response);
    printf("Target position: %d/12\n", g_controller.target_index);
    

    // Ajouter lecture utile ici
    
    return 0;
}

void *gpib_poll_thread(void *arg)
{
    (void)arg;

    while(1)
    {
        while (g_controller.state == MASTER_ONLINE_DEVICE_ONLINE)
        {
            if (gpib_read_all() < 0) {
                fprintf(stderr, "Error reading from GPIB device in polling thread\n");
                g_controller.state = MASTER_OFFLINE_DEVICE_OFFLINE;
            }
            sleep(1);
        }     
        sleep(1);
    }

    return NULL;
}

void cleanup_and_quit(void)
{
    // Libère le device GPIB si connecté
    if (g_controller.state == MASTER_ONLINE_DEVICE_ONLINE)
    {
        ibonl(g_controller.ud, 0);
        g_controller.state = MASTER_OFFLINE_DEVICE_OFFLINE;
    }

}