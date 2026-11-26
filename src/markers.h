#ifndef QSO_MARKERS_H
#define QSO_MARKERS_H

#include <stddef.h>

#define MAX_MARKERS 1024
#define CALLSIGN_LEN 32
#define HISTORY_DEFAULT 1800.0

typedef enum {
    MARKER_HOME,
    MARKER_ACTIVE,
    MARKER_HISTORY
} marker_type;

typedef struct {
    char callsign[CALLSIGN_LEN];
    double lat;
    double lon;
    double timestamp;
    marker_type type;
} station_marker;

void markers_init(double retention);
void markers_home(const char *call,
                  double lat, double lon);
void markers_qso(const char *call,
                 double lat, double lon,
                 double now);
void markers_finish(void);
void markers_expire(double now);

size_t markers_count(void);
const station_marker *markers_get(size_t index);

#endif
