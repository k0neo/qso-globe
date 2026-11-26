#include "markers.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>

static station_marker stations[MAX_MARKERS];
static size_t count;
static double retention_seconds;

void markers_init(double retention)
{
    count = 0;

    retention_seconds =
        retention > 0.0 ? retention : HISTORY_DEFAULT;
}

static void assign_marker(station_marker *m,
                          const char *call,
                          double lat, double lon,
                          double now,
                          marker_type type)
{
    snprintf(m->callsign, sizeof(m->callsign),
             "%s", call);

    m->lat = lat;
    m->lon = lon;
    m->timestamp = now;
    m->type = type;
}

void markers_home(const char *call,
                  double lat, double lon)
{
    for (size_t i = 0; i < count; i++) {
        if (stations[i].type == MARKER_HOME) {
            assign_marker(&stations[i], call,
                          lat, lon, 0.0, MARKER_HOME);
            return;
        }
    }

    if (count < MAX_MARKERS)
        assign_marker(&stations[count++], call,
                      lat, lon, 0.0, MARKER_HOME);
}

void markers_finish(void)
{
    for (size_t i = 0; i < count; i++) {
        if (stations[i].type == MARKER_ACTIVE)
            stations[i].type = MARKER_HISTORY;
    }
}

void markers_expire(double now)
{
    size_t i = 0;

    while (i < count) {

        station_marker *m = &stations[i];

        if (m->type == MARKER_HISTORY &&
            now - m->timestamp >= retention_seconds) {

            printf("Expired marker: %s\n",
                   m->callsign);

            stations[i] = stations[--count];

        } else {
            i++;
        }
    }
}

void markers_qso(const char *call,
                 double lat, double lon,
                 double now)
{
    if (!call || !*call ||
        strlen(call) >= CALLSIGN_LEN ||
        !isfinite(lat) || !isfinite(lon) ||
        lat < -90.0 || lat > 90.0 ||
        lon < -180.0 || lon > 180.0)
        return;

    markers_finish();
    markers_expire(now);

    for (size_t i = 0; i < count; i++) {

        if (stations[i].type != MARKER_HOME &&
            strcasecmp(stations[i].callsign,
                       call) == 0) {

            assign_marker(&stations[i], call,
                          lat, lon, now,
                          MARKER_ACTIVE);

            printf("Refreshed marker: %s\n", call);
            return;
        }
    }

    size_t slot = count;

    if (count == MAX_MARKERS) {

        double oldest = INFINITY;
        slot = MAX_MARKERS;

        for (size_t i = 0; i < count; i++) {
            if (stations[i].type == MARKER_HISTORY &&
                stations[i].timestamp < oldest) {

                oldest = stations[i].timestamp;
                slot = i;
            }
        }

        if (slot == MAX_MARKERS)
            return;

    } else {
        count++;
    }

    assign_marker(&stations[slot], call,
                  lat, lon, now, MARKER_ACTIVE);

    printf("Added marker: %s\n", call);
}

size_t markers_count(void)
{
    return count;
}

const station_marker *markers_get(size_t index)
{
    return index < count ? &stations[index] : NULL;
}
