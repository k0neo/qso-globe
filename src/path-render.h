#ifndef QSO_PATH_RENDER_H
#define QSO_PATH_RENDER_H

#include "geodesy.h"

#define ARC_SEGMENTS 256
#define ARC_RADIUS 1.0015

static void path_render(void)
{
    vec3 home = geographic(HOME_LAT, HOME_LON);

    glPushAttrib(GL_ENABLE_BIT |
                 GL_CURRENT_BIT |
                 GL_LINE_BIT |
                 GL_DEPTH_BUFFER_BIT);

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    for (size_t i = 0; i < markers_count(); i++) {

        const station_marker *m = markers_get(i);

        if (m->type == MARKER_HOME)
            continue;

        vec3 dx = geographic(m->lat, m->lon);

        if (m->type == MARKER_ACTIVE) {
            glColor3f(1.0f, 0.9f, 0.0f);
            glLineWidth(2.0f);
        } else {
            glColor3f(0.65f, 0.65f, 0.65f);
            glLineWidth(1.0f);
        }

        glBegin(GL_LINE_STRIP);

        for (int j = 0; j <= ARC_SEGMENTS; j++) {

            double t = (double)j / ARC_SEGMENTS;

            vec3 p = slerp(home, dx, t);

            glVertex3d(p.x * ARC_RADIUS,
                       p.y * ARC_RADIUS,
                       p.z * ARC_RADIUS);
        }

        glEnd();
    }

    glPopAttrib();
}

#endif
