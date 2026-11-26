#ifndef QSO_EARTH_H
#define QSO_EARTH_H

static void earth_vertex(double lat, double lon)
{
    vec3 v = geographic(lat, lon);

    glTexCoord2d(
        (lon+180.0)/360.0,
        (lat+90.0)/180.0
    );

    glNormal3d(v.x, v.y, v.z);
    glVertex3d(v.x, v.y, v.z);
}

static void earth_mesh(void)
{
    const int slices = 256;
    const int stacks = 128;

    for (int j = 0; j < stacks; j++) {

        double lat1 = -90.0+180.0*j/stacks;
        double lat2 = -90.0+180.0*(j+1)/stacks;

        glBegin(GL_QUAD_STRIP);

        for (int i = 0; i <= slices; i++) {

            double lon = -180.0+360.0*i/slices;

            earth_vertex(lat1, lon);
            earth_vertex(lat2, lon);
        }

        glEnd();
    }
}

static void crosshair(void)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 0.0f);
    glLineWidth(1.0f);

    glBegin(GL_LINES);

    glVertex2f(-0.025f, 0.0f);
    glVertex2f( 0.025f, 0.0f);

    glVertex2f(0.0f, -0.025f);
    glVertex2f(0.0f,  0.025f);

    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
}

#endif
