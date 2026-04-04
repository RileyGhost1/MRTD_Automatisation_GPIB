#ifndef CORE_H
#define CORE_H
#include <pthread.h>


typedef enum
{
    IDLE_MENU = 0,
    MANUAL_MODE,
    AUTO_MODE
} ProgramMode;

typedef struct
{
    ProgramMode current_mode;
    int gpib_polling;
    int shutdown_requested;

    pthread_t thread_auto;
    pthread_t thread_manual;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} AppData;

extern AppData g_appdata;

int hmi_init(int *argc, char ***argv);
void *thread_gpib_polling(void *arg);

#endif
