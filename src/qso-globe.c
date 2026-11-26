#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <poll.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include <jpeglib.h>
#include <X11/keysym.h>
#include "config.h"
#include "state.h"
#include "filesystem.h"
#include <X11/Xutil.h>

static globe_config settings;

#include "camera-config.h"
#include "earth.h"
#include <string.h>
#include "marker-render.h"
#include "path-render.h"
#include "qso-overlay-config.h"
#include "illumination-night.h"
#include "texture-preferences.h"
#include "qso-input.h"
#include "qso-queue.h"
#include <inttypes.h>

static double destination_range(double distance_km)
{
    double range =
        1.15 + 0.70 * sqrt(distance_km / 7000.0);

    return fmax(1.15, fmin(1.85, range));
}


#define WIDTH (settings.width)
#define HEIGHT (settings.height)

#define TEXTURE "/usr/share/xplanet/images/earth.jpg"

static double monotonic(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static GLuint load_texture(const char *filename)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    FILE *fp = fopen(filename, "rb");

    if (!fp) {
        perror(filename);
        exit(EXIT_FAILURE);
    }

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);

    cinfo.out_color_space = JCS_RGB;

    jpeg_start_decompress(&cinfo);

    unsigned int width  = cinfo.output_width;
    unsigned int height = cinfo.output_height;

    size_t stride = (size_t)width * 3;

    unsigned char *pixels = malloc(stride * height);

    if (!pixels) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    while (cinfo.output_scanline < height) {
        JSAMPROW row =
            pixels + (height - 1 -
            cinfo.output_scanline) * stride;

        jpeg_read_scanlines(&cinfo, &row, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);

    GLuint texture;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_S, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                 width, height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, pixels);

    glGenerateMipmap(GL_TEXTURE_2D);

    GLenum texture_error = glGetError();

    if (texture_error != GL_NO_ERROR) {
        fprintf(stderr, "Texture upload error: 0x%x\n",
                (unsigned int)texture_error);
        exit(EXIT_FAILURE);
    }

    free(pixels);

    printf("Texture: %u x %u\n", width, height);

    return texture;
}

static void resize(int width, int height)
{
    if (height < 1)
        height = 1;

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(45.0,
                   (double)width / height,
                   0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char **argv)
{

    for (int i = 1; i < argc; i++) {

        if (!strcmp(argv[i], "--version")) {
            puts("QSO Globe 1.0.0");
            return EXIT_SUCCESS;
        }

        if (!strcmp(argv[i], "--help")) {

            puts(
                "QSO Globe 1.0.0\n"
                "\n"
                "Usage: qso-globe [options]\n"
                "\n"
                "Options:\n"
                "  --help         Display this help\n"
                "  --version      Display version\n"
                "  --show-config  Display configuration\n"
                "  --test-expiry  Use 30-second retention\n"
                "\n"
                "Keyboard controls:\n"
                "  D       Full daylight toggle\n"
                "  N       Natural illumination toggle\n"
                "  G       Gray-line highlighting toggle\n"
                "  T       Terminator contour toggle\n"
                "  Space   London demonstration\n"
                "  Escape  Exit\n");

            return EXIT_SUCCESS;
        }
    }



    config_defaults(&settings);

    char config_path[CONFIG_PATH_LEN];

    const char *xdg = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");

    int n = -1;

    if (xdg && *xdg && xdg[0] == '/')
        n = snprintf(
            config_path, sizeof(config_path),
            "%s/qso-globe/qso-globe.conf", xdg);

    else if (home && *home)
        n = snprintf(
            config_path, sizeof(config_path),
            "%s/.config/qso-globe/qso-globe.conf",
            home);

    if (n < 0 ||
        (size_t)n >= sizeof(config_path)) {

        fprintf(stderr,
                "Cannot determine configuration path\n");

        return EXIT_FAILURE;
    }

    int loaded = config_load(&settings, config_path);

    if (loaded < 0)
        return EXIT_FAILURE;

    if (loaded == 0)
        puts("Configuration absent; using defaults");

    else
        printf("Configuration loaded: %s\n",
               config_path);

    int test_expiry = 0;
    int show_config = 0;

    for (int i = 1; i < argc; i++) {

        if (!strcmp(argv[i], "--test-expiry"))
            test_expiry = 1;

        else if (!strcmp(argv[i], "--show-config"))
            show_config = 1;

        else {

            fprintf(stderr,
                    "Unknown option: %s\n", argv[i]);

            return EXIT_FAILURE;
        }
    }

    if (test_expiry)
        settings.retention = 30.0;


    /*
     * Derive the state path from the
     * configuration directory.
     */

    char state_path[CONFIG_PATH_LEN];

    const char *separator =
        strrchr(config_path, '/');

    if (!separator) {

        fprintf(stderr,
                "Invalid configuration path\n");

        return EXIT_FAILURE;
    }

    int state_length = snprintf(
        state_path, sizeof(state_path),
        "%.*s/qso-globe.state",
        (int)(separator-config_path),
        config_path);

    if (state_length < 0 ||
        (size_t)state_length >= sizeof(state_path)) {

        fprintf(stderr,
                "State path exceeds limit\n");

        return EXIT_FAILURE;
    }

    int restored = state_load(
        &settings, state_path);

    if (restored > 0)
        printf("Operator preferences restored: %s\n",
               state_path);

    else if (restored < 0)
        fprintf(stderr,
                "Ignoring invalid operator state\n");


    config_print(&settings);

    if (show_config)
        return EXIT_SUCCESS;

    if (!test_expiry &&
        ensure_parent_directory(state_path) != 0) {

        perror("Cannot create configuration directory");

        return EXIT_FAILURE;
    }

    if (!strcmp(settings.callsign, "NOCALL")) {

        fprintf(stderr,
                "Station not configured. "
                "Edit %s\n", config_path);
    }



    illumination_mode = settings.illumination_mode;
    illumination_contour = settings.terminator;


    Display *display = XOpenDisplay(NULL);

    if (!display) {
        fprintf(stderr, "Cannot open X display\n");
        return EXIT_FAILURE;
    }

    int attributes[] = {
        GLX_RGBA,
        GLX_DOUBLEBUFFER,
        GLX_DEPTH_SIZE, 24,
        None
    };

    XVisualInfo *visual =
        glXChooseVisual(display,
                        DefaultScreen(display),
                        attributes);

    if (!visual) {
        fprintf(stderr, "No suitable GLX visual\n");
        XCloseDisplay(display);
        return EXIT_FAILURE;
    }

    XSetWindowAttributes swa;

    swa.colormap =
        XCreateColormap(display,
                        RootWindow(display, visual->screen),
                        visual->visual,
                        AllocNone);

    swa.event_mask =
        ExposureMask |
        StructureNotifyMask |
        KeyPressMask;

    Window window =
        XCreateWindow(display,
                      RootWindow(display, visual->screen),
                      settings.window_x, settings.window_y,
                      WIDTH, HEIGHT,
                      0, visual->depth,
                      InputOutput,
                      visual->visual,
                      CWColormap | CWEventMask,
                      &swa);

    XStoreName(display, window,
               "QSO Globe");

    Atom wm_delete =
        XInternAtom(display,
                    "WM_DELETE_WINDOW", False);

    XSetWMProtocols(display, window, &wm_delete, 1);

    GLXContext context =
        glXCreateContext(display, visual, NULL, True);

    if (!context) {
        fprintf(stderr, "Cannot create GLX context\n");
        XDestroyWindow(display, window);
        XFreeColormap(display, swa.colormap);
        XFree(visual);
        XCloseDisplay(display);
        return EXIT_FAILURE;
    }


    /*
     * Request client-window positioning
     * relative to the root window.
     */

    XSizeHints hints = {0};

    hints.flags =
        USPosition | USSize | PWinGravity;

    hints.x = settings.window_x;
    hints.y = settings.window_y;

    hints.width = settings.width;
    hints.height = settings.height;

    hints.win_gravity = StaticGravity;

    XSetWMNormalHints(
        display, window, &hints);


    XMapWindow(display, window);

    if (!glXMakeCurrent(display, window, context)) {
        fprintf(stderr, "Cannot activate GLX context\n");
        glXDestroyContext(display, context);
        XDestroyWindow(display, window);
        XFreeColormap(display, swa.colormap);
        XFree(visual);
        XCloseDisplay(display);
        return EXIT_FAILURE;
    }

    printf("Vendor:   %s\n", glGetString(GL_VENDOR));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL:   %s\n", glGetString(GL_VERSION));
    printf("Direct:   %s\n",
           glXIsDirect(display, context) ? "Yes" : "No");



    char day_path[4096];
    char night_path[4096];

    const char *day_file = texture_path(
        "QSO_DAY_TEXTURE",
        settings.day_texture,
        "earth-8192.jpg",
        TEXTURE,
        day_path,
        sizeof(day_path));

    const char *night_file = texture_path(
        "QSO_NIGHT_TEXTURE",
        settings.night_texture,
        "night-8192.jpg",
        "/usr/share/xplanet/images/night.jpg",
        night_path,
        sizeof(night_path));

    printf("Day texture:   %s\n", day_file);
    printf("Night texture: %s\n", night_file);

    GLuint texture = load_texture(day_file);


    puts("Loading nighttime texture");

    illumination_night_texture = load_texture(night_file);

    glBindTexture(GL_TEXTURE_2D, texture);


    if (!illumination_init()) {
        fprintf(stderr,
                "Solar illumination initialization failed\n");
        return EXIT_FAILURE;
    }

    double retention = settings.retention;

    markers_init(retention);

    markers_home(settings.callsign, HOME_LAT, HOME_LON);

    printf("Marker retention: %.0f seconds\n",
           retention);

    if (!marker_render_init(display)) {
        fprintf(stderr, "Cannot initialize marker font\n");
        return EXIT_FAILURE;
    }

    GLUquadric *sphere = gluNewQuadric();

    if (!sphere) {
        fprintf(stderr, "Cannot create sphere\n");
        return EXIT_FAILURE;
    }

    gluQuadricTexture(sphere, GL_TRUE);
    gluQuadricNormals(sphere, GLU_SMOOTH);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glClearColor(0.0, 0.0, 0.0, 1.0);

    resize(WIDTH, HEIGHT);

    /*
     * Compile the sphere geometry once.
     */

    GLuint mesh = glGenLists(1);

    glNewList(mesh, GL_COMPILE);
    earth_mesh();
    glEndList();

        int running = 1;
    int dirty = 1;
    int was_moving = 0;
    int exit_status = EXIT_SUCCESS;

    unsigned long frames = 0;
    double report = 0.0;

    double animation_start = -1.0;


    double target_lat = DX_LAT;
    double target_lon = DX_LON;
    double target_range = DX_RANGE;

    int qso_fd = qso_input_open(settings.port);

    if (qso_fd < 0) {
        perror("Cannot open QSO receiver");
        return EXIT_FAILURE;
    }

    printf("Listening for QSOs on 127.0.0.1:%d\n",
           settings.port);


        while (running) {

        /*
         * Process Xlib's internal event queue before
         * considering a blocking poll().
         */

        while (XPending(display)) {

            XEvent event;

            XNextEvent(display, &event);

            if (event.type == ClientMessage &&
                (Atom)event.xclient.data.l[0] == wm_delete) {
                running = 0;
            }

            if (event.type == Expose ||
                event.type == MapNotify) {
                dirty = 1;
            }

            if (event.type == KeyPress) {

                KeySym key = XLookupKeysym(
                    &event.xkey, 0);

                illumination_key(key);
                dirty = 1;

                if (key == XK_Escape)
                    running = 0;

                if (key == XK_space &&
                    animation_start < 0.0) {

                    target_lat = DX_LAT;
                    target_lon = DX_LON;
                    target_range = DX_RANGE;

                    animation_start = monotonic();

                    markers_qso("LONDON-TEST",
                                DX_LAT, DX_LON,
                                animation_start);

                    puts("QSO animation started");
                }
            }

            if (event.type == ConfigureNotify) {

                resize(event.xconfigure.width,
                       event.xconfigure.height);

                dirty = 1;
            }
        }

        if (!running)
            break;

        double now = monotonic();

        /*
         * Complete the previous animation before
         * accepting another queued destination.
         */

        if (animation_start >= 0.0 &&
            now-animation_start >=
                2.0*TRANSIT+HOLD) {

            markers_finish();
            animation_start = -1.0;
            dirty = 1;

            puts("QSO animation complete");
        }

        /*
         * Drain available UDP notifications.
         * Network processing remains bounded.
         */

        for (int i = 0; i < 64; i++) {

            qso_event incoming;

            int status =
                qso_input_receive(qso_fd, &incoming);

            if (status == 0)
                break;

            if (status == -2) {
                puts("Rejected malformed QSO");
                continue;
            }

            if (status < 0) {
                perror("QSO receive");
                break;
            }

            int queued = queue_push(incoming);

            if (queued > 0)
                printf("Queued QSO %" PRIu64
                       ": %s (%s)\n",
                       incoming.id,
                       incoming.callsign,
                       incoming.grid);

            else if (queued == 0)
                printf("Duplicate QSO %" PRIu64 "\n",
                       incoming.id);

            else
                puts("QSO queue full");
        }

        now = monotonic();

        /*
         * Start the next queued QSO when idle.
         */

        if (animation_start < 0.0) {

            qso_event next;

            if (queue_pop(&next)) {

                target_lat = next.lat;
                target_lon = next.lon;

                geodesic g = geodesy_calculate(
                    HOME_LAT, HOME_LON,
                    target_lat, target_lon);

                target_range =
                    destination_range(g.distance_km);

                animation_start = now;

                markers_qso(next.callsign,
                            target_lat,
                            target_lon,
                            now);

                dirty = 1;

                printf("Animating %s: %.2f km, "
                       "%.2f deg, range %.3f\n",
                       next.callsign,
                       g.distance_km,
                       g.short_bearing,
                       target_range);
            }
        }

        /*
         * Detect expired history markers.
         */

        size_t previous_count = markers_count();

        markers_expire(now);

        if (markers_count() != previous_count)
            dirty = 1;

        /*
         * Determine whether solar illumination
         * requires recalculation.
         */

        time_t utc = time(NULL);

        if (illumination_updated == 0 ||
            difftime(utc, illumination_updated) >= 60.0 ||
            utc < illumination_updated) {

            dirty = 1;
        }

        /*
         * Determine the current camera phase.
         */

        double elapsed =
            animation_start < 0.0
            ? -1.0
            : now-animation_start;

        int moving =
            elapsed >= 0.0 &&
            (elapsed < TRANSIT ||
             (elapsed >= TRANSIT+HOLD &&
              elapsed < 2.0*TRANSIT+HOLD));

        /*
         * Report FPS only during camera movement.
         */

        if (moving && !was_moving) {
            frames = 0;
            report = now;
            puts("Camera motion started");
        }

        if (!moving && was_moving) {

            double duration = now-report;

            if (duration > 0.0)
                printf("Animation rendering: %.2f FPS\n",
                       frames/duration);

            frames = 0;
            dirty = 1;
        }

        was_moving = moving;

        if (moving)
            dirty = 1;

        /*
         * Render only when the scene changes.
         */

        if (dirty) {

            camera view = camera_at(
                elapsed,
                target_lat,
                target_lon,
                target_range);

            glClear(GL_COLOR_BUFFER_BIT |
                    GL_DEPTH_BUFFER_BIT);

            glLoadIdentity();

            camera_apply(view);

            glBindTexture(GL_TEXTURE_2D, texture);

            illumination_begin();

            glCallList(mesh);

            illumination_end();

            path_render();
            marker_render(view);
            qso_overlay();

            glXSwapBuffers(display, window);

            if (moving)
                frames++;

            dirty = 0;

            fflush(stdout);
        }

        /*
         * During motion, retain the verified
         * synchronized OpenGL rendering loop.
         */

        if (moving)
            continue;

        /*
         * Calculate the next required wakeup.
         *
         * Use a one-second maximum sleep as a
         * safeguard against wall-clock adjustments.
         */

        now = monotonic();
        utc = time(NULL);

        double deadline = now+1.0;

        /*
         * Solar ephemeris refresh deadline.
         */

        double solar_remaining =
            60.0-difftime(
                utc, illumination_updated);

        if (solar_remaining < 0.0)
            solar_remaining = 0.0;

        deadline = fmin(
            deadline,
            now+solar_remaining);

        /*
         * Resume movement at the end of the
         * destination hold.
         */

        if (animation_start >= 0.0) {

            deadline = fmin(
                deadline,
                animation_start+TRANSIT+HOLD);
        }

        /*
         * Find the earliest historical marker
         * expiration deadline.
         */

        for (size_t i = 0;
             i < markers_count(); i++) {

            const station_marker *m =
                markers_get(i);

            if (m->type == MARKER_HISTORY) {

                deadline = fmin(
                    deadline,
                    m->timestamp+retention);
            }
        }

        double remaining =
            fmax(0.0, deadline-monotonic());

        int timeout =
            (int)ceil(remaining*1000.0);

        /*
         * Xlib may already have buffered events
         * even when its socket is not readable.
         */

        if (XPending(display))
            continue;

        struct pollfd descriptors[2] = {
            {
                .fd = ConnectionNumber(display),
                .events = POLLIN
            },
            {
                .fd = qso_fd,
                .events = POLLIN
            }
        };

        int ready = poll(
            descriptors, 2, timeout);

        if (ready < 0) {

            if (errno == EINTR)
                continue;

            perror("Event-loop poll");

            exit_status = EXIT_FAILURE;
            break;
        }

        if ((descriptors[0].revents |
             descriptors[1].revents) &
             (POLLERR | POLLHUP | POLLNVAL)) {

            fprintf(stderr,
                    "Event-loop descriptor failure\n");

            exit_status = EXIT_FAILURE;
            break;
        }
    }

    qso_input_close(qso_fd);

    /*
     * Save operator preferences only
     * after a successful normal session.
     */

    if (exit_status == EXIT_SUCCESS &&
        !test_expiry) {

        XWindowAttributes attributes;

        if (XGetWindowAttributes(
                display, window, &attributes)) {

            if (attributes.width >= 200 &&
                attributes.width <= 16384 &&
                attributes.height >= 200 &&
                attributes.height <= 16384) {

                settings.width = attributes.width;
                settings.height = attributes.height;
            }

            Window child;

            int root_x;
            int root_y;

            if (XTranslateCoordinates(
                    display,
                    window,
                    attributes.root,
                    0, 0,
                    &root_x, &root_y,
                    &child)) {

                if (root_x >= -32768 &&
                    root_x <= 32767 &&
                    root_y >= -32768 &&
                    root_y <= 32767) {

                    settings.window_x = root_x;
                    settings.window_y = root_y;
                }
            }
        }

        settings.illumination_mode =
            illumination_mode;

        settings.terminator =
            illumination_contour;

        if (state_save(
                &settings, state_path) == 1) {

            printf("Operator preferences saved: %s\n",
                   state_path);

        } else {

            perror("Cannot save operator preferences");
        }
    }




    marker_render_cleanup(display);


illumination_cleanup();

    glDeleteTextures(1, &illumination_night_texture);

    glDeleteLists(mesh, 1);
    glDeleteTextures(1, &texture);

    gluDeleteQuadric(sphere);

    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, context);

    XDestroyWindow(display, window);
    XFreeColormap(display, swa.colormap);
    XFree(visual);
    XCloseDisplay(display);

    return exit_status;
}
