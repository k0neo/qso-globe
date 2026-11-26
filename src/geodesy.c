#include "geodesy.h"
#include <math.h>

#define PI 3.14159265358979323846
#define RAD (PI / 180.0)
#define DEG (180.0 / PI)

geodesic geodesy_calculate(double lat1, double lon1,
                          double lat2, double lon2)
{
    double p1 = lat1 * RAD;
    double p2 = lat2 * RAD;
    double dl = (lon2 - lon1) * RAD;

    double y = sin(dl) * cos(p2);

    double x = cos(p1) * sin(p2)
             - sin(p1) * cos(p2) * cos(dl);

    double dot = sin(p1) * sin(p2)
               + cos(p1) * cos(p2) * cos(dl);

    double angle = atan2(hypot(x, y), dot);

    double bearing = hypot(x, y) < 1e-12
                   ? NAN
                   : fmod(atan2(y, x)*DEG + 360.0,
                          360.0);

    double distance = EARTH_RADIUS_KM * angle;

    return (geodesic) {
        .distance_km = distance,
        .distance_miles = distance / KM_PER_MILE,
        .short_bearing = bearing,
        .long_distance_km =
            2.0*PI*EARTH_RADIUS_KM - distance,
        .long_bearing = fmod(bearing+180.0, 360.0)
    };
}
