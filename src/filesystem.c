#define _POSIX_C_SOURCE 200809L

#include "filesystem.h"

#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static int ensure_directory(const char *path)
{
    if (mkdir(path, 0700) == 0)
        return 0;

    if (errno != EEXIST)
        return -1;

    struct stat st;

    if (stat(path, &st) != 0)
        return -1;

    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }

    return 0;
}

int ensure_parent_directory(const char *path)
{
    if (!path || !*path) {
        errno = EINVAL;
        return -1;
    }

    char *copy = strdup(path);

    if (!copy)
        return -1;

    char *last = strrchr(copy, '/');

    if (!last || last == copy) {
        free(copy);
        return 0;
    }

    *last = '\0';

    for (char *p = copy+1; *p; p++) {

        if (*p != '/')
            continue;

        *p = '\0';

        if (ensure_directory(copy) != 0) {

            int saved = errno;

            free(copy);
            errno = saved;

            return -1;
        }

        *p = '/';
    }

    int result = ensure_directory(copy);
    int saved = errno;

    free(copy);
    errno = saved;

    return result;
}
