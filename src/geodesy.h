#ifndef QSO_GEODESY_H
#define QSO_GEODESY_H

#define EARTH_RADIUS_KM 6371.0088
#define KM_PER_MILE 1.609344

typedef struct {
    double distance_km;
    double distance_miles;
    double short_bearing;
    double long_distance_km;
    double long_bearing;
} geodesic;

geodesic geodesy_calculate(double lat1, double lon1,
                          double lat2, double lon2);

#endif
