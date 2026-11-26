#ifndef QSO_OVERLAY_H
#define QSO_OVERLAY_H

static void overlay_text(double x, double y,
                         const char *text)
{
    glRasterPos2d(x, y);
    glListBase(marker_font_base);

    glCallLists((GLsizei)strlen(text),
                GL_UNSIGNED_BYTE, text);
}

static void qso_overlay(void)
{
    const station_marker *active = NULL;

    for (size_t i = 0; i < markers_count(); i++) {

        const station_marker *m = markers_get(i);

        if (m->type == MARKER_ACTIVE) {
            active = m;
            break;
        }
    }

    if (!active)
        return;

    geodesic g = geodesy_calculate(
        HOME_LAT, HOME_LON,
        active->lat, active->lon);

    GLint viewport[4];

    glGetIntegerv(GL_VIEWPORT, viewport);

    double width = viewport[2];
    double height = viewport[3];

    if (width < 320 || height < 120)
        return;

    char title[96];
    char distance[96];
    char bearing[96];

    snprintf(title, sizeof(title),
             "%s -> %s", settings.callsign, active->callsign);

    snprintf(distance, sizeof(distance),
             "Distance: %.0f km / %.0f mi",
             g.distance_km, g.distance_miles);

    if (isfinite(g.short_bearing))
        snprintf(bearing, sizeof(bearing),
                 "Short path: %.1f deg true",
                 g.short_bearing);
    else
        snprintf(bearing, sizeof(bearing),
                 "Short path: undefined");

    glPushAttrib(GL_ENABLE_BIT |
                 GL_CURRENT_BIT |
                 GL_COLOR_BUFFER_BIT |
                 GL_LIST_BIT);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA,
                GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0, width, 0, height, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor4f(0.0f, 0.0f, 0.0f, 0.72f);

    glBegin(GL_QUADS);

    glVertex2d(8, height-8);
    glVertex2d(310, height-8);
    glVertex2d(310, height-82);
    glVertex2d(8, height-82);

    glEnd();

    glColor3f(1.0f, 1.0f, 0.0f);
    overlay_text(16, height-26, title);

    glColor3f(1.0f, 1.0f, 1.0f);
    overlay_text(16, height-48, distance);
    overlay_text(16, height-68, bearing);

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);

    glPopAttrib();
}

#endif
