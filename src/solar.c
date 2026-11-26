#include "solar.h"

#include <math.h>
#include <stddef.h>

#define PI 3.14159265358979323846
#define RAD (PI / 180.0)
#define DEG (180.0 / PI)

#define JD_UNIX 2440587.5
#define JD_J2000 2451545.0

static double wrap360(double value)
{
    value = fmod(value, 360.0);

    return value < 0.0
         ? value + 360.0
         : value;
}

static double wrap180(double value)
{
    return wrap360(value + 180.0) - 180.0;
}

int solar_calculate(time_t utc, solar_state *sun)
{
    if (!sun)
        return 0;

    /*
     * Approximate UTC as UT1 for Earth rotation.
     */

    double jd = JD_UNIX + (double)utc / 86400.0;

    double d = jd - JD_J2000;
    double t = d / 36525.0;

    /*
     * Solar mean anomaly and mean longitude.
     */

    double g = wrap360(
        357.529 + 0.98560028*d);

    double q = wrap360(
        280.459 + 0.98564736*d);

    /*
     * Apparent ecliptic longitude.
     */

    double lambda = wrap360(
        q
        + 1.915*sin(g*RAD)
        + 0.020*sin(2.0*g*RAD));

    /*
     * Mean obliquity of the ecliptic.
     */

    double epsilon = 23.439 - 0.00000036*d;

    /*
     * Equatorial coordinates.
     */

    double ra = wrap360(
        atan2(cos(epsilon*RAD)*sin(lambda*RAD),
              cos(lambda*RAD))*DEG);

    double dec = asin(
        sin(epsilon*RAD)*sin(lambda*RAD))*DEG;

    /*
     * Earth-Sun distance in astronomical units.
     */

    double distance =
          1.00014
        - 0.01671*cos(g*RAD)
        - 0.00014*cos(2.0*g*RAD);

    /*
     * Greenwich mean sidereal time.
     */

    double gmst = wrap360(
          280.46061837
        + 360.98564736629*d
        + 0.000387933*t*t
        - t*t*t/38710000.0);

    /*
     * Approximate equation of the equinoxes.
     * Principal nutation terms.
     */

    double omega = wrap360(
        125.04 - 0.052954*d);

    double delta_psi =
          -0.00478*sin(omega*RAD)
          -0.00037*sin(2.0*q*RAD);

    double gast = wrap360(
        gmst + delta_psi*cos(epsilon*RAD));

    /*
     * Subsolar geographic coordinates.
     * East longitude is positive.
     */

    double lat = dec;
    double lon = wrap180(ra - gast);

    /*
     * Earth-fixed unit vector toward the Sun.
     */

    double p = lat*RAD;
    double l = lon*RAD;

    *sun = (solar_state) {
        .julian_date = jd,

        .right_ascension_deg = ra,
        .declination_deg = dec,

        .gmst_deg = gmst,
        .gast_deg = gast,

        .subsolar_lat = lat,
        .subsolar_lon = lon,

        .distance_au = distance,

        .x = cos(p)*cos(l),
        .y = cos(p)*sin(l),
        .z = sin(p)
    };

    return 1;
}

double solar_elevation(const solar_state *sun,
                       double lat, double lon)
{
    if (!sun ||
        !isfinite(lat) ||
        !isfinite(lon) ||
        lat < -90.0 ||
        lat > 90.0)
        return NAN;

    double p = lat*RAD;
    double l = lon*RAD;

    double x = cos(p)*cos(l);
    double y = cos(p)*sin(l);
    double z = sin(p);

    double dot =
          x*sun->x
        + y*sun->y
        + z*sun->z;

    dot = fmax(-1.0, fmin(1.0, dot));

    return asin(dot)*DEG;
}

solar_zone solar_classify(double elevation)
{
    if (elevation >= 0.0)
        return SOLAR_DAYLIGHT;

    if (elevation >= -6.0)
        return SOLAR_CIVIL_TWILIGHT;

    if (elevation >= -12.0)
        return SOLAR_NAUTICAL_TWILIGHT;

    if (elevation >= -18.0)
        return SOLAR_ASTRONOMICAL_TWILIGHT;

    return SOLAR_NIGHT;
}

const char *solar_zone_name(solar_zone zone)
{
    switch (zone) {

    case SOLAR_DAYLIGHT:
        return "Daylight";

    case SOLAR_CIVIL_TWILIGHT:
        return "Civil twilight";

    case SOLAR_NAUTICAL_TWILIGHT:
        return "Nautical twilight";

    case SOLAR_ASTRONOMICAL_TWILIGHT:
        return "Astronomical twilight";

    case SOLAR_NIGHT:
        return "Night";
    }

    return "Unknown";
}
