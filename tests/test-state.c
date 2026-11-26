#define _POSIX_C_SOURCE 200809L

#include "state.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static void write_file(const char *path,
                       const char *contents)
{
    FILE *fp = fopen(path, "w");

    assert(fp != NULL);
    assert(fputs(contents, fp) >= 0);
    assert(fclose(fp) == 0);
}

int main(void)
{
    globe_config cfg;
    globe_config restored;

    config_defaults(&cfg);

    char directory[] = "/tmp/qso-state-XXXXXX";

    assert(mkdtemp(directory) != NULL);

    char path[CONFIG_PATH_LEN];

    int n = snprintf(
        path, sizeof(path),
        "%s/qso-globe.state", directory);

    assert(n > 0);
    assert((size_t)n < sizeof(path));

    assert(state_load(&cfg, path) == 0);

    puts("Missing state handling: PASS");

    cfg.width = 1024;
    cfg.height = 768;
    cfg.window_x = -200;
    cfg.window_y = 140;
    cfg.illumination_mode = 2;
    cfg.terminator = 1;

    assert(state_save(&cfg, path) == 1);

    puts("Atomic state creation: PASS");

    config_defaults(&restored);

    assert(state_load(&restored, path) == 1);

    assert(restored.width == 1024);
    assert(restored.height == 768);
    assert(restored.window_x == -200);
    assert(restored.window_y == 140);
    assert(restored.illumination_mode == 2);
    assert(restored.terminator == 1);

    puts("Preference restoration: PASS");

    struct stat st;

    assert(stat(path, &st) == 0);
    assert((st.st_mode & 0777) == 0600);

    puts("State file permissions: PASS");

    /*
     * State files cannot override station
     * or network configuration.
     */

    write_file(path,
        "[station]\n"
        "callsign=TEST1\n"
        "latitude=10.0\n"
        "longitude=20.0\n"
        "[network]\n"
        "port=45833\n"
        "[window]\n"
        "width=900\n");

    assert(state_load(&restored, path) == 1);

    assert(strcmp(restored.callsign,
                  "NOCALL") == 0);

    assert(restored.latitude == 0.0);
    assert(restored.longitude == 0.0);
    assert(restored.port == 45832);
    assert(restored.width == 900);

    puts("Station configuration isolation: PASS");

    globe_config previous = restored;

    write_file(path,
        "[window]\n"
        "width=800\n"
        "height=-1\n");

    assert(state_load(&restored, path) == -1);

    assert(memcmp(&restored, &previous,
                  sizeof(restored)) == 0);

    puts("Invalid state rollback: PASS");

    cfg.width = 1280;
    cfg.height = 960;

    assert(state_save(&cfg, path) == 1);
    assert(state_load(&restored, path) == 1);

    assert(restored.width == 1280);
    assert(restored.height == 960);

    puts("Atomic state replacement: PASS");

    cfg.width = 0;

    assert(state_save(&cfg, path) == -1);

    config_defaults(&restored);

    assert(state_load(&restored, path) == 1);
    assert(restored.width == 1280);

    puts("Invalid save protection: PASS");

    assert(unlink(path) == 0);
    assert(rmdir(directory) == 0);

    puts("\nAll state tests passed.");

    return EXIT_SUCCESS;
}
