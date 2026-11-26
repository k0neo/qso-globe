#ifndef QSO_CAMERA_H
#define QSO_CAMERA_H

#define PI 3.14159265358979323846
#define RAD (PI / 180.0)

#define HOME_LAT (settings.latitude)
#define HOME_LON (settings.longitude)

#define DX_LAT     51.5074
#define DX_LON     -0.1278

#define HOME_RANGE (settings.home_range)
#define DX_RANGE    1.85

#define TRANSIT (settings.transit)
#define HOLD (settings.hold)

typedef struct {
    double x, y, z;
} vec3;

typedef struct {
    vec3 direction;
    double range;
} camera;

static vec3 geographic(double lat, double lon)
{
    lat *= RAD;
    lon *= RAD;

    return (vec3) {
        cos(lat) * cos(lon),
        cos(lat) * sin(lon),
        sin(lat)
    };
}

static vec3 normalize(vec3 v)
{
    double n = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);

    return (vec3) {
        v.x/n, v.y/n, v.z/n
    };
}

static vec3 slerp(vec3 a, vec3 b, double t)
{
    double dot = a.x*b.x + a.y*b.y + a.z*b.z;

    dot = fmax(-1.0, fmin(1.0, dot));

    if (dot > 0.999999)
        return normalize((vec3) {
            a.x + t*(b.x-a.x),
            a.y + t*(b.y-a.y),
            a.z + t*(b.z-a.z)
        });

    vec3 tangent;

    if (dot < -0.999999999999) {
        vec3 axis = fabs(a.z) < 0.9
                  ? (vec3){0,0,1}
                  : (vec3){1,0,0};

        double d = a.x*axis.x +
                   a.y*axis.y +
                   a.z*axis.z;

        tangent = normalize((vec3) {
            axis.x-d*a.x,
            axis.y-d*a.y,
            axis.z-d*a.z
        });
    } else {
        tangent = normalize((vec3) {
            b.x-dot*a.x,
            b.y-dot*a.y,
            b.z-dot*a.z
        });
    }

    double angle = acos(dot)*t;

    return normalize((vec3) {
        a.x*cos(angle)+tangent.x*sin(angle),
        a.y*cos(angle)+tangent.y*sin(angle),
        a.z*cos(angle)+tangent.z*sin(angle)
    });
}

static double ease(double t)
{
    t = fmax(0.0, fmin(1.0, t));

    return t*t*(3.0-2.0*t);
}

static camera camera_at(double elapsed,
                        double dx_lat,
                        double dx_lon,
                        double dx_range)
{
    vec3 home = geographic(HOME_LAT, HOME_LON);
    vec3 dx   = geographic(dx_lat, dx_lon);

    double t;

    if (elapsed < 0.0 ||
        elapsed >= 2.0*TRANSIT + HOLD)
        return (camera){home, HOME_RANGE};

    if (elapsed < TRANSIT)
        t = ease(elapsed/TRANSIT);

    else if (elapsed < TRANSIT+HOLD)
        t = 1.0;

    else
        t = 1.0-ease(
            (elapsed-TRANSIT-HOLD)/TRANSIT);

    return (camera) {
        slerp(home, dx, t),
        HOME_RANGE+(dx_range-HOME_RANGE)*t
    };
}

static void camera_apply(camera c)
{
    vec3 v = c.direction;

    vec3 up = fabs(v.z) > 0.999999
            ? (vec3){0,1,0}
            : (vec3){0,0,1};

    gluLookAt(
        c.range*v.x,
        c.range*v.y,
        c.range*v.z,
        0.0, 0.0, 0.0,
        up.x, up.y, up.z
    );
}

#endif
