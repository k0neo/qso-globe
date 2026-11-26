#define _POSIX_C_SOURCE 200809L

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

void config_defaults(globe_config *cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    snprintf(cfg->callsign,
             sizeof(cfg->callsign),
             "NOCALL");

    cfg->latitude = 0.0;
    cfg->longitude = 0.0;

    cfg->width = 600;
    cfg->height = 600;
    cfg->window_x = 200;
    cfg->window_y = 200;

    cfg->home_range = 3.2;
    cfg->transit = 4.0;
    cfg->hold = 5.0;
    cfg->retention = 1800.0;

    cfg->illumination_mode = 1;
    cfg->terminator = 0;

    cfg->port = 45832;
}

static char *trim(char *s)
{
    while (isspace((unsigned char)*s))
        s++;

    char *end = s + strlen(s);

    while (end > s &&
           isspace((unsigned char)end[-1]))
        *--end = '\0';

    return s;
}

static int parse_double(const char *s,
                        double minimum,
                        double maximum,
                        double *result)
{
    char *end;

    errno = 0;
    double value = strtod(s, &end);

    if (errno || end == s || *end ||
        !isfinite(value) ||
        value < minimum ||
        value > maximum)
        return 0;

    *result = value;

    return 1;
}

static int parse_int(const char *s,
                     int minimum,
                     int maximum,
                     int *result)
{
    char *end;

    errno = 0;
    long value = strtol(s, &end, 10);

    if (errno || end == s || *end ||
        value < minimum ||
        value > maximum)
        return 0;

    *result = (int)value;

    return 1;
}

static int parse_bool(const char *s,
                      int *result)
{
    if (!strcmp(s, "true") ||
        !strcmp(s, "yes") ||
        !strcmp(s, "1")) {

        *result = 1;
        return 1;
    }

    if (!strcmp(s, "false") ||
        !strcmp(s, "no") ||
        !strcmp(s, "0")) {

        *result = 0;
        return 1;
    }

    return 0;
}

static int copy_value(char *destination,
                      size_t capacity,
                      const char *value)
{
    size_t length = strlen(value);

    if (length == 0 || length >= capacity)
        return 0;

    memcpy(destination, value, length+1);

    return 1;
}

static int parse_callsign(globe_config *cfg,
                          const char *value)
{
    size_t length = strlen(value);

    if (length == 0 ||
        length >= sizeof(cfg->callsign))
        return 0;

    for (size_t i = 0; i < length; i++) {

        unsigned char c =
            (unsigned char)value[i];

        if (!isalnum(c) && c != '/')
            return 0;
    }

    for (size_t i = 0; i < length; i++)
        cfg->callsign[i] =
            (char)toupper(
                (unsigned char)value[i]);

    cfg->callsign[length] = '\0';

    return 1;
}

static int set_value(globe_config *cfg,
                     const char *section,
                     const char *key,
                     const char *value)
{
    if (!strcmp(section, "station")) {

        if (!strcmp(key, "callsign"))
            return parse_callsign(cfg, value);

        if (!strcmp(key, "latitude"))
            return parse_double(
                value, -90.0, 90.0,
                &cfg->latitude);

        if (!strcmp(key, "longitude"))
            return parse_double(
                value, -180.0, 180.0,
                &cfg->longitude);
    }

    if (!strcmp(section, "window")) {

        if (!strcmp(key, "width"))
            return parse_int(
                value, 200, 16384,
                &cfg->width);

        if (!strcmp(key, "height"))
            return parse_int(
                value, 200, 16384,
                &cfg->height);

        if (!strcmp(key, "x"))
            return parse_int(
                value, -32768, 32767,
                &cfg->window_x);

        if (!strcmp(key, "y"))
            return parse_int(
                value, -32768, 32767,
                &cfg->window_y);
    }

    if (!strcmp(section, "animation")) {

        if (!strcmp(key, "home_range"))
            return parse_double(
                value, 1.85, 20.0,
                &cfg->home_range);

        if (!strcmp(key, "transit_seconds"))
            return parse_double(
                value, 0.25, 60.0,
                &cfg->transit);

        if (!strcmp(key, "hold_seconds"))
            return parse_double(
                value, 0.0, 300.0,
                &cfg->hold);

        if (!strcmp(key, "marker_retention"))
            return parse_double(
                value, 1.0, 604800.0,
                &cfg->retention);
    }

    if (!strcmp(section, "illumination")) {

        if (!strcmp(key, "mode")) {

            if (!strcmp(value, "day"))
                cfg->illumination_mode = 0;

            else if (!strcmp(value, "natural"))
                cfg->illumination_mode = 1;

            else if (!strcmp(value, "grayline"))
                cfg->illumination_mode = 2;

            else
                return 0;

            return 1;
        }

        if (!strcmp(key, "terminator"))
            return parse_bool(
                value, &cfg->terminator);
    }

    if (!strcmp(section, "network")) {

        if (!strcmp(key, "port"))
            return parse_int(
                value, 1024, 65535,
                &cfg->port);
    }

    if (!strcmp(section, "textures")) {

        if (!strcmp(key, "day"))
            return copy_value(
                cfg->day_texture,
                sizeof(cfg->day_texture),
                value);

        if (!strcmp(key, "night"))
            return copy_value(
                cfg->night_texture,
                sizeof(cfg->night_texture),
                value);
    }

    return 0;
}

int config_load(globe_config *cfg,
                const char *path)
{
    FILE *fp = fopen(path, "r");

    if (!fp) {

        if (errno == ENOENT)
            return 0;

        perror(path);
        return -1;
    }

    globe_config temporary = *cfg;

    char *line = NULL;
    size_t capacity = 0;

    char section[64] = "";

    unsigned long line_number = 0;
    int valid = 1;

    while (getline(&line, &capacity, fp) >= 0) {

        line_number++;

        char *s = trim(line);

        if (!*s || *s == '#' || *s == ';')
            continue;

        if (*s == '[') {

            char *end = strrchr(s, ']');

            if (!end || end[1]) {
                valid = 0;
                break;
            }

            *end = '\0';

            if (!copy_value(
                    section,
                    sizeof(section),
                    trim(s+1))) {

                valid = 0;
                break;
            }

            continue;
        }

        char *equal = strchr(s, '=');

        if (!equal) {
            valid = 0;
            break;
        }

        *equal = '\0';

        char *key = trim(s);
        char *value = trim(equal+1);

        if (!set_value(
                &temporary,
                section, key, value)) {

            fprintf(stderr,
                    "%s:%lu: invalid setting "
                    "[%s] %s = %s\n",
                    path, line_number,
                    section, key, value);

            valid = 0;
            break;
        }
    }

    if (ferror(fp))
        valid = 0;

    free(line);
    fclose(fp);

    if (!valid) {

        fprintf(stderr,
                "%s:%lu: configuration rejected\n",
                path, line_number);

        return -1;
    }

    *cfg = temporary;

    return 1;
}

void config_print(const globe_config *cfg)
{
    static const char *modes[] = {
        "day", "natural", "grayline"
    };

    printf("Station:       %s\n",
           cfg->callsign);

    printf("Coordinates:   %.4f, %.4f\n",
           cfg->latitude, cfg->longitude);

    printf("Window:        %dx%d at %d,%d\n",
           cfg->width, cfg->height,
           cfg->window_x, cfg->window_y);

    printf("Home range:    %.2f\n",
           cfg->home_range);

    printf("Transit:       %.2f seconds\n",
           cfg->transit);

    printf("Hold:          %.2f seconds\n",
           cfg->hold);

    printf("Retention:     %.0f seconds\n",
           cfg->retention);

    printf("Illumination:  %s\n",
           modes[cfg->illumination_mode]);

    printf("Terminator:    %s\n",
           cfg->terminator ? "on" : "off");

    printf("UDP port:      %d\n",
           cfg->port);

    printf("Day texture:   %s\n",
           cfg->day_texture[0]
           ? cfg->day_texture : "automatic");

    printf("Night texture: %s\n",
           cfg->night_texture[0]
           ? cfg->night_texture : "automatic");
}
