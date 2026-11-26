#define _POSIX_C_SOURCE 200809L

#include "state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int state_load(globe_config *cfg,
               const char *path)
{
    globe_config temporary = *cfg;

    int result = config_load(&temporary, path);

    if (result != 1)
        return result;

    /*
     * Only operator preferences may be restored.
     */

    cfg->width = temporary.width;
    cfg->height = temporary.height;

    cfg->window_x = temporary.window_x;
    cfg->window_y = temporary.window_y;

    cfg->illumination_mode =
        temporary.illumination_mode;

    cfg->terminator = temporary.terminator;

    return 1;
}

int state_save(const globe_config *cfg,
               const char *path)
{
    static const char *modes[] = {
        "day",
        "natural",
        "grayline"
    };

    if (cfg->illumination_mode < 0 ||
        cfg->illumination_mode > 2 ||
        cfg->width < 200 ||
        cfg->width > 16384 ||
        cfg->height < 200 ||
        cfg->height > 16384 ||
        cfg->window_x < -32768 ||
        cfg->window_x > 32767 ||
        cfg->window_y < -32768 ||
        cfg->window_y > 32767 ||
        (cfg->terminator != 0 &&
         cfg->terminator != 1)) {

        errno = EINVAL;
        return -1;
    }

    char temporary[CONFIG_PATH_LEN+32];

    int n = snprintf(
        temporary, sizeof(temporary),
        "%s.tmp.XXXXXX", path);

    if (n < 0 ||
        (size_t)n >= sizeof(temporary)) {

        errno = ENAMETOOLONG;
        return -1;
    }

    int fd = mkstemp(temporary);

    if (fd < 0)
        return -1;

    FILE *fp = fdopen(fd, "w");

    if (!fp) {

        int saved = errno;

        close(fd);
        unlink(temporary);

        errno = saved;
        return -1;
    }

    int valid = fprintf(
        fp,
        "# QSO Globe operator preferences\n"
        "# Automatically maintained.\n\n"
        "[window]\n"
        "width = %d\n"
        "height = %d\n"
        "x = %d\n"
        "y = %d\n\n"
        "[illumination]\n"
        "mode = %s\n"
        "terminator = %s\n",
        cfg->width,
        cfg->height,
        cfg->window_x,
        cfg->window_y,
        modes[cfg->illumination_mode],
        cfg->terminator ? "true" : "false") >= 0;

    if (valid && fflush(fp) != 0)
        valid = 0;

    if (valid && fsync(fd) != 0)
        valid = 0;

    if (fclose(fp) != 0)
        valid = 0;

    if (!valid) {

        int saved = errno ? errno : EIO;

        unlink(temporary);

        errno = saved;
        return -1;
    }

    /*
     * The previous state file remains intact
     * until the replacement is complete.
     */

    if (rename(temporary, path) != 0) {

        int saved = errno;

        unlink(temporary);

        errno = saved;
        return -1;
    }

    return 1;
}
