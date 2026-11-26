#ifndef QSO_MARKER_RENDER_H
#define QSO_MARKER_RENDER_H

#include "markers.h"

static GLuint marker_font_base;
static XFontStruct *marker_font;

static int marker_render_init(Display *display)
{
    marker_font = XLoadQueryFont(display, "9x15");

    if (!marker_font)
        marker_font = XLoadQueryFont(display, "fixed");

    if (!marker_font)
        return 0;

    marker_font_base = glGenLists(256);

    if (!marker_font_base) {
        XFreeFont(display, marker_font);
        marker_font = NULL;
        return 0;
    }

    glXUseXFont(marker_font->fid,
                0, 256, marker_font_base);

    return 1;
}

static void marker_render_cleanup(Display *display)
{
    if (marker_font_base)
        glDeleteLists(marker_font_base, 256);

    if (marker_font)
        XFreeFont(display, marker_font);
}

static void marker_color(marker_type type)
{
    switch (type) {

    case MARKER_HOME:
        glColor3f(0.2f, 1.0f, 0.2f);
        break;

    case MARKER_ACTIVE:
        glColor3f(1.0f, 1.0f, 0.0f);
        break;

    case MARKER_HISTORY:
        glColor3f(1.0f, 1.0f, 1.0f);
        break;
    }
}

static void marker_render(camera view)
{
    GLdouble model[16];
    GLdouble projection[16];
    GLint viewport[4];

    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    glPushAttrib(GL_ENABLE_BIT |
                 GL_CURRENT_BIT |
                 GL_POINT_BIT |
                 GL_LIST_BIT);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0, viewport[2],
            0, viewport[3], -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    for (size_t i = 0; i < markers_count(); i++) {

        const station_marker *m = markers_get(i);

        vec3 v = geographic(m->lat, m->lon);

        double dot =
            v.x*view.direction.x +
            v.y*view.direction.y +
            v.z*view.direction.z;

        /*
         * Perspective visibility test:
         * the station must lie above the
         * observer's geometric horizon.
         */

        if (dot <= 1.0/view.range)
            continue;

        GLdouble x, y, z;

        if (!gluProject(v.x, v.y, v.z,
                        model, projection, viewport,
                        &x, &y, &z))
            continue;

        if (x < 0 || x >= viewport[2] ||
            y < 0 || y >= viewport[3] ||
            z < 0 || z > 1)
            continue;

        double radius =
            m->type == MARKER_HISTORY ? 3.0 : 5.0;

        marker_color(m->type);

        glBegin(GL_TRIANGLE_FAN);

        glVertex2d(x, y);

        for (int j = 0; j <= 24; j++) {

            double angle = 2.0*PI*j/24.0;

            glVertex2d(x + radius*cos(angle),
                       y + radius*sin(angle));
        }

        glEnd();

        glRasterPos2d(x+radius+4, y+4);

        glListBase(marker_font_base);

        glCallLists((GLsizei)strlen(m->callsign),
                    GL_UNSIGNED_BYTE,
                    m->callsign);
    }

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);

    glPopAttrib();
}

#endif
