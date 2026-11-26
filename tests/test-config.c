#define _POSIX_C_SOURCE 200809L

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void write_config(const char *path,
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

    config_defaults(&cfg);

    assert(strcmp(cfg.callsign, "NOCALL") == 0);
    assert(cfg.latitude == 0.0);
    assert(cfg.longitude == 0.0);
    assert(cfg.width == 600);
    assert(cfg.height == 600);
    assert(cfg.home_range == 3.2);
    assert(cfg.transit == 4.0);
    assert(cfg.hold == 5.0);
    assert(cfg.retention == 1800.0);
    assert(cfg.port == 45832);
    assert(cfg.illumination_mode == 1);

    puts("Default configuration: PASS");

    char path[] = "/tmp/qso-config-XXXXXX";

    int fd = mkstemp(path);

    assert(fd >= 0);
    assert(close(fd) == 0);

    write_config(path,
        "# Test configuration\n"
        "[station]\n"
        "callsign = k0neo/p\n"
        "latitude = 40.0\n"
        "longitude = -90.0\n"
        "[window]\n"
        "width = 1024\n"
        "height = 768\n"
        "x = -200\n"
        "[animation]\n"
        "transit_seconds = 2.5\n"
        "hold_seconds = 0\n"
        "marker_retention = 30\n"
        "[illumination]\n"
        "mode = grayline\n"
        "terminator = true\n"
        "[network]\n"
        "port = 45833\n"
        "[textures]\n"
        "day = /tmp/day.jpg\n"
        "night = /tmp/night.jpg\n");

    assert(config_load(&cfg, path) == 1);

    assert(strcmp(cfg.callsign, "K0NEO/P") == 0);
    assert(cfg.latitude == 40.0);
    assert(cfg.longitude == -90.0);
    assert(cfg.width == 1024);
    assert(cfg.height == 768);
    assert(cfg.window_x == -200);
    assert(cfg.transit == 2.5);
    assert(cfg.hold == 0.0);
    assert(cfg.retention == 30.0);
    assert(cfg.illumination_mode == 2);
    assert(cfg.terminator == 1);
    assert(cfg.port == 45833);

    assert(strcmp(cfg.day_texture,
                  "/tmp/day.jpg") == 0);

    assert(strcmp(cfg.night_texture,
                  "/tmp/night.jpg") == 0);

    puts("Configuration overrides: PASS");

    globe_config previous = cfg;

    const char *invalid[] = {

        "[station]\nlatitude=91\n",
        "[station]\nlongitude=-181\n",
        "[station]\ncallsign=BAD CALL\n",
        "[window]\nwidth=0\n",
        "[network]\nport=65536\n",
        "[animation]\ntransit_seconds=0\n",
        "[animation]\nmarker_retention=-1\n",
        "[animation]\nhold_seconds=nan\n",
        "[illumination]\nmode=invalid\n",
        "[illumination]\nterminator=maybe\n",
        "[network]\nport=1234junk\n",
        "[unknown]\nsetting=value\n",
        "[station\n",
        "[station]\ncallsign\n",
        "[window]\nwidth=800\nheight=-1\n"
    };

    size_t count =
        sizeof(invalid)/sizeof(invalid[0]);

    for (size_t i = 0; i < count; i++) {

        write_config(path, invalid[i]);

        assert(config_load(&cfg, path) == -1);

        assert(memcmp(&cfg, &previous,
                      sizeof(cfg)) == 0);
    }

    printf("Invalid configuration rejection: "
           "PASS (%zu cases)\n", count);

    puts("Transactional rollback: PASS");

    assert(unlink(path) == 0);

    assert(config_load(&cfg, path) == 0);

    puts("Missing configuration handling: PASS");

    puts("\nAll configuration tests passed.");

    return EXIT_SUCCESS;
}
