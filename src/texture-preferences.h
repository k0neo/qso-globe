#ifndef QSO_TEXTURE_CONFIG_H
#define QSO_TEXTURE_CONFIG_H

#include <unistd.h>

static const char *texture_path(
    const char *environment,
    const char *preference,
    const char *filename,
    const char *fallback,
    char *buffer,
    size_t capacity)
{
    const char *configured = getenv(environment);

    if (configured && *configured) {
        if (access(configured, R_OK) == 0)
            return configured;

        fprintf(stderr,
                "Texture unavailable: %s\n",
                configured);
    }

    /*
     * Configuration paths may be absolute or
     * relative to the application's texture directory.
     */

    if (preference && *preference) {

        if (preference[0] == '/') {

            if (access(preference, R_OK) == 0)
                return preference;

        } else {

            const char *home = getenv("HOME");

            if (home && *home) {

                int n = snprintf(
                    buffer, capacity,
                    "%s/.local/share/qso-globe/textures/%s",
                    home, preference);

                if (n > 0 &&
                    (size_t)n < capacity &&
                    access(buffer, R_OK) == 0)
                    return buffer;
            }
        }

        fprintf(stderr,
                "Configured texture unavailable: %s\n",
                preference);
    }


    const char *home = getenv("HOME");

    if (home && *home) {

        int n = snprintf(
            buffer, capacity,
            "%s/.local/share/qso-globe/textures/%s",
            home, filename);

        if (n > 0 &&
            (size_t)n < capacity &&
            access(buffer, R_OK) == 0)
            return buffer;
    }

    fprintf(stderr,
            "Using fallback texture: %s\n",
            fallback);

    return fallback;
}

#endif
