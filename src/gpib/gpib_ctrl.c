#include <stdlib.h>
#include <stdio.h>
#include <gpib/ib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#define GPIB_BUFFER_SIZE 256
static int gpib_dev = -1;
static int ud = -1;

/*
Public function to initialize the GPIB device
*/
int gpib_init(int master_addr, int dev_addr)
{
	const int sad = 0;
	const int send_eoi = 1;
	const int eos_mode = 0;
	const int timeout = T1s;

	printf("trying to open pad = %i on /dev/gpib%i ...\n", dev_addr, master_addr);
	ud = ibdev(master_addr, dev_addr, sad, timeout, send_eoi, eos_mode);
	if(ud < 0)
	{
		fprintf(stderr, "ibdev error\n");
        fprintf(stderr, "Failed to initialize GPIB device\n");
        return -1;
		// Ajouter fonction de déconnection et màj du status du device 
	}

    printf("Device connected successfully !\n");

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
    ibwrt(ud, command, strlen(command));
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
    ibrd(ud, response, GPIB_BUFFER_SIZE - 1);
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
    //usleep(100000); 
    return gpib_read(response);
}
/*======================================================================================================*/

int gpib_read_all()
{
    char response[GPIB_BUFFER_SIZE];

    if (gpib_write_read("RT", response) < 0) {
        fprintf(stderr, "Failed to read from GPIB device\n");
        return -1;
    } else {
        printf("Target temperature: %s\n", response);
    }

    if (gpib_write_read("RE", response) < 0) {
        fprintf(stderr, "Failed to read from GPIB device\n");
        return -1;
    } else {
        printf("Emitter temperature: %s\n", response);
    }

    if (gpib_write_read("RA", response) < 0) {
        fprintf(stderr, "Failed to read from GPIB device\n");
        return -1;
    } else {
        printf("Target position: %s/12\n", response);
    }

    // Ajouter lecture utile ici

    return 0;
}