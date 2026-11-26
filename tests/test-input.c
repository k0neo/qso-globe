#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <poll.h>

#include "qso-input.h"
#include "geodesy.h"

int main(void)
{
    int fd = qso_input_open(QSO_PORT);

    if (fd < 0) {
        perror("qso_input_open");
        return EXIT_FAILURE;
    }

    printf("Listening on 127.0.0.1:%d\n", QSO_PORT);
    fflush(stdout);

    struct pollfd pfd = {
        .fd = fd,
        .events = POLLIN
    };

    for (;;) {

        int ready = poll(&pfd, 1, 1000);

        if (ready < 0) {
            perror("poll");
            break;
        }

        if (ready == 0)
            continue;

        qso_event qso;

        int status = qso_input_receive(fd, &qso);

        if (status == 0)
            continue;

        if (status == -2) {
            puts("Rejected malformed QSO message");
            continue;
        }

        if (status < 0) {
            perror("qso_input_receive");
            break;
        }

        geodesic g = geodesy_calculate(
            37.9792, -92.0417,
            qso.lat, qso.lon);

        printf("\nQSO ID:    %" PRIu64 "\n", qso.id);
        printf("Callsign:  %s\n", qso.callsign);
        printf("Locator:   %s\n", qso.grid);
        printf("Latitude:  %.6f\n", qso.lat);
        printf("Longitude: %.6f\n", qso.lon);
        printf("Distance:  %.2f km / %.2f mi\n",
               g.distance_km, g.distance_miles);
        printf("Bearing:   %.2f degrees true\n",
               g.short_bearing);

        fflush(stdout);
    }

    qso_input_close(fd);

    return EXIT_FAILURE;
}
