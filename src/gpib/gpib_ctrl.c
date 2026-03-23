#include <gpib/ib.h>
#include <stdio.h>

static int gpib_dev = -1;

/*
Public function to initialize the GPIB device
*/
int gpib_init(int Device_Addr)
{
    gpib_dev = ibdev(0, Device_Addr, 0, T3s, 1, 0); // board index, Device Primary address, secondary address, timeout, EOI mode, EOS mode
    return gpib_dev;
}
/*======================================================================================================*/

/*
Public function to close the GPIB device
*/
int gpib_close()
{
    if (gpib_dev >= 0) {
        ibonl(gpib_dev, 0); // Take the device offline
        gpib_dev = -1;
    }
    return 0;
}
/*======================================================================================================*/

/*
Private function to check GPIB status before each action
*/
int gpib_check_status(const char *context) {
    
    if (ibsta & ERR) {
        switch (iberr) {
            case EABO:  fprintf(stderr, "Timeout GPIB\n"); break;
            case ENOL:  fprintf(stderr, "Pas de listener sur le bus\n"); break;
            case ENEB:  fprintf(stderr, "Interface GPIB absente\n"); break;
            case EARG:  fprintf(stderr, "Argument invalide\n"); break;
            default:    fprintf(stderr, "Erreur GPIB code: %d\n", iberr); break;
        }
    }
    return 0;
}
/*======================================================================================================*/