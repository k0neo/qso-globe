#ifndef QSO_SOLAR_H
#define QSO_SOLAR_H

#include <time.h>

#define SOLAR_AU_KM 149597870.7

typedef struct {
    double julian_date;

    double right_ascension_deg;
    double declination_deg;

    double gmst_deg;
    double gast_deg;

    double subsolar_lat;
    double subsolar_lon;

    double distance_au;

    double x;
    double y;
    double z;
} solar_state;

typedef enum {
    SOLAR_DAYLIGHT,
    SOLAR_CIVIL_TWILIGHT,
    SOLAR_NAUTICAL_TWILIGHT,
    SOLAR_ASTRONOMICAL_TWILIGHT,
    SOLAR_NIGHT
} solar_zone;

int solar_calculate(time_t utc, solar_state *sun);

double solar_elevation(const solar_state *sun,
                       double lat, double lon);

solar_zone solar_classify(double elevation);

const char *solar_zone_name(solar_zone zone);

#endif
