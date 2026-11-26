#include "locator.h"

#include <ctype.h>
#include <string.h>

int locator_to_coordinates(const char *grid,
                           double *lat,
                           double *lon)
{
    if (!grid || !lat || !lon)
        return 0;

    size_t n = strlen(grid);

    if (n != 4 && n != 6 && n != 8)
        return 0;

    char g[9];

    for (size_t i = 0; i < n; i++)
        g[i] = toupper((unsigned char)grid[i]);

    g[n] = '\0';

    if (g[0] < 'A' || g[0] > 'R' ||
        g[1] < 'A' || g[1] > 'R' ||
        g[2] < '0' || g[2] > '9' ||
        g[3] < '0' || g[3] > '9')
        return 0;

    double x = -180.0
             + 20.0 * (g[0] - 'A')
             +  2.0 * (g[2] - '0');

    double y = -90.0
             + 10.0 * (g[1] - 'A')
             +        (g[3] - '0');

    double dx = 2.0;
    double dy = 1.0;

    if (n >= 6) {

        if (g[4] < 'A' || g[4] > 'X' ||
            g[5] < 'A' || g[5] > 'X')
            return 0;

        dx /= 24.0;
        dy /= 24.0;

        x += dx * (g[4] - 'A');
        y += dy * (g[5] - 'A');
    }

    if (n == 8) {

        if (g[6] < '0' || g[6] > '9' ||
            g[7] < '0' || g[7] > '9')
            return 0;

        dx /= 10.0;
        dy /= 10.0;

        x += dx * (g[6] - '0');
        y += dy * (g[7] - '0');
    }

    *lon = x + dx / 2.0;
    *lat = y + dy / 2.0;

    return 1;
}
