#include <gpib/ib.h>


static int gpib_dev = -1;

/*
Public function to initialize the GPIB device
*/
int gpib_ctrl_init(int Device_Addr)
{
    gpib_dev = ibdev(0, Device_Addr, 0, T3s, 1, 0); // board index, Device Primary address, secondary address, timeout, EOI mode, EOS mode
    return gpib_dev;
}
/*======================================================================================================*/

/*
Public function to close the GPIB device
*/
int gpib_ctrl_close()
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
        fprintf(stderr, "[GPIB][%s] iberr=%d ", context, iberr);
        if (iberr == ENOL)  fprintf(stderr, "(No Listener)\n");
        if (iberr == EABO)  fprintf(stderr, "(Operation aborted)\n");
        if (iberr == ENEB)  fprintf(stderr, "(No GPIB board)\n");
        if (iberr == ETMO)  fprintf(stderr, "(Timeout)\n");
        ibclr(gpib_dev);
        return -1;
    }
    return 0;
}
/*======================================================================================================*/