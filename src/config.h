#ifndef QSO_CONFIG_H
#define QSO_CONFIG_H

#include <stddef.h>

#define CONFIG_PATH_LEN 4096
#define CONFIG_CALL_LEN 32

typedef struct {

    char callsign[CONFIG_CALL_LEN];

    double latitude;
    double longitude;

    int width;
    int height;
    int window_x;
    int window_y;

    double home_range;
    double transit;
    double hold;
    double retention;

    int illumination_mode;
    int terminator;

    int port;

    char day_texture[CONFIG_PATH_LEN];
    char night_texture[CONFIG_PATH_LEN];

} globe_config;

void config_defaults(globe_config *cfg);

int config_load(globe_config *cfg,
                const char *path);

void config_print(const globe_config *cfg);

#endif
