#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "solar.h"

static int failures;

static void check(const char *name, int result)
{
    printf("%-42s %s\n",
           name, result ? "PASS" : "FAIL");

    if (!result)
        failures++;
}

static void location(const solar_state *sun,
                     const char *name,
                     double lat, double lon)
{
    double elevation =
        solar_elevation(sun, lat, lon);

    printf("%-18s %8.3f deg  %s\n",
           name,
           elevation,
           solar_zone_name(
               solar_classify(elevation)));
}

static void seasonal_test(const char *name,
                          time_t utc,
                          double expected,
                          double tolerance)
{
    solar_state sun;

    solar_calculate(utc, &sun);

    printf("%-24s declination: %8.3f deg\n",
           name, sun.declination_deg);

    check(name,
          fabs(sun.declination_deg - expected)
          < tolerance);
}

int main(void)
{
    solar_state sun;

    time_t now = time(NULL);

    struct tm utc;

    if (!gmtime_r(&now, &utc))
        return EXIT_FAILURE;

    char timestamp[64];

    strftime(timestamp, sizeof(timestamp),
             "%Y-%m-%d %H:%M:%S UTC", &utc);

    if (!solar_calculate(now, &sun))
        return EXIT_FAILURE;

    puts("=== Current solar ephemeris ===");

    printf("UTC:           %s\n", timestamp);
    printf("Julian date:   %.8f\n", sun.julian_date);

    printf("Solar RA:      %.6f deg\n",
           sun.right_ascension_deg);

    printf("Declination:   %.6f deg\n",
           sun.declination_deg);

    printf("GMST:          %.6f deg\n",
           sun.gmst_deg);

    printf("GAST:          %.6f deg\n",
           sun.gast_deg);

    printf("Subsolar lat:  %.6f deg\n",
           sun.subsolar_lat);

    printf("Subsolar lon:  %.6f deg\n",
           sun.subsolar_lon);

    printf("Sun distance:  %.6f AU\n",
           sun.distance_au);

    printf("Solar vector:  %.8f %.8f %.8f\n",
           sun.x, sun.y, sun.z);

    puts("\n=== Geographic illumination ===");

    location(&sun, "K0NEO",
             37.9792, -92.0417);

    location(&sun, "Grenada",
             12.108, -61.679);

    location(&sun, "London",
             51.5074, -0.1278);

    location(&sun, "Subsolar point",
             sun.subsolar_lat,
             sun.subsolar_lon);

    location(&sun, "Antisolar point",
             -sun.subsolar_lat,
             sun.subsolar_lon + 180.0);

    puts("\n=== Mathematical regression tests ===");

    check("Solar vector has unit length",
          fabs(sqrt(sun.x*sun.x +
                    sun.y*sun.y +
                    sun.z*sun.z)-1.0) < 1e-12);

    check("Valid subsolar latitude",
          fabs(sun.subsolar_lat) <= 23.5);

    check("Valid subsolar longitude",
          sun.subsolar_lon >= -180.0 &&
          sun.subsolar_lon < 180.0);

    check("Subsolar elevation is +90 degrees",
          fabs(solar_elevation(
              &sun,
              sun.subsolar_lat,
              sun.subsolar_lon)-90.0) < 1e-5);

    check("Antisolar elevation is -90 degrees",
          fabs(solar_elevation(
              &sun,
              -sun.subsolar_lat,
              sun.subsolar_lon+180.0)+90.0)
              < 1e-5);

    check("Geometric terminator elevation is zero",
          fabs(solar_elevation(
              &sun, 0.0,
              sun.subsolar_lon+90.0))
              < 1e-8);

    check("Earth-Sun distance is plausible",
          sun.distance_au > 0.98 &&
          sun.distance_au < 1.02);

    check("Daylight classification",
          solar_classify(45.0) == SOLAR_DAYLIGHT);

    check("Civil twilight classification",
          solar_classify(-3.0) ==
              SOLAR_CIVIL_TWILIGHT);

    check("Nautical twilight classification",
          solar_classify(-9.0) ==
              SOLAR_NAUTICAL_TWILIGHT);

    check("Astronomical twilight classification",
          solar_classify(-15.0) ==
              SOLAR_ASTRONOMICAL_TWILIGHT);

    check("Night classification",
          solar_classify(-25.0) == SOLAR_NIGHT);

    puts("\n=== Seasonal regression tests ===");

    /* Approximate seasonal reference dates, 2026 UTC. */

    seasonal_test("March equinox",
                  (time_t)1774008000,
                  0.0, 0.5);

    seasonal_test("June solstice",
                  (time_t)1782043200,
                  23.44, 0.1);

    seasonal_test("September equinox",
                  (time_t)1790164800,
                  0.0, 0.5);

    seasonal_test("December solstice",
                  (time_t)1797854400,
                  -23.44, 0.1);

    printf("\n=== Result: %d failures ===\n",
           failures);

    return failures
         ? EXIT_FAILURE
         : EXIT_SUCCESS;
}
