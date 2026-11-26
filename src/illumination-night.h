#ifndef QSO_ILLUMINATION_H
#define QSO_ILLUMINATION_H

#include "solar.h"

static GLuint illumination_program;
static GLint illumination_sun;
static solar_state illumination_state;
static time_t illumination_updated;

static GLint illumination_mode_uniform;
static GLint illumination_contour_uniform;

static GLuint illumination_night_texture;

static int illumination_mode = 1;
static int illumination_contour = 0;


static const char *illumination_vertex_source =
    "#version 120\n"
    "varying vec3 surface_normal;\n"
    "varying vec2 texcoord;\n"
    "void main() {\n"
    "    surface_normal = gl_Normal;\n"
    "    texcoord = gl_MultiTexCoord0.st;\n"
    "    gl_Position = ftransform();\n"
    "}\n";

static const char *illumination_fragment_source =
    "#version 120\n"
    "uniform sampler2D earth_texture;\n"
    "uniform sampler2D night_texture;\n"
    "uniform vec3 sun_direction;\n"
    "uniform int display_mode;\n"
    "uniform int show_terminator;\n"
    "varying vec3 surface_normal;\n"
    "varying vec2 texcoord;\n"
    "void main() {\n"
    "    vec3 normal = normalize(surface_normal);\n"
    "    float cosine = clamp(\n"
    "        dot(normal, sun_direction),\n"
    "        -1.0, 1.0);\n"
    "    float elevation = degrees(asin(cosine));\n"
    "    vec3 earth = texture2D(earth_texture,\n"
    "                           texcoord).rgb;\n"
    "    vec3 night = texture2D(night_texture,\n"
    "                           texcoord).rgb;\n"
    "\n"
    "    float civil = smoothstep(-6.0, 0.0,\n"
    "                             elevation);\n"
    "    float nautical = smoothstep(-12.0, -6.0,\n"
    "                                elevation);\n"
    "    float astronomical = smoothstep(-18.0, -12.0,\n"
    "                                    elevation);\n"
    "\n"
    "    float light = 0.15\n"
    "                + 0.10 * astronomical\n"
    "                + 0.20 * nautical\n"
    "                + 0.55 * civil;\n"
    "\n"
    "    float darkness = 1.0 -\n"
    "        smoothstep(-12.0, 0.0, elevation);\n"
    "\n"
    "    vec3 result = earth * light;\n"
    "    result += night * darkness * 0.85;\n"
    "\n"
    "    if (display_mode == 0)\n"
    "        result = earth;\n"
    "\n"
    "    if (display_mode == 2) {\n"
    "        float grayline = 1.0 -\n"
    "            smoothstep(0.0, 6.0,\n"
    "                       abs(elevation));\n"
    "        result = mix(result,\n"
    "            vec3(0.15, 0.65, 0.85),\n"
    "            grayline * 0.30);\n"
    "    }\n"
    "\n"
    "    if (show_terminator != 0) {\n"
    "        float width = max(fwidth(elevation), 0.02);\n"
    "        float contour = 1.0 -\n"
    "            smoothstep(0.0, width * 1.5,\n"
    "                       abs(elevation));\n"
    "        result = mix(result,\n"
    "            vec3(0.85, 0.90, 1.0),\n"
    "            contour * 0.85);\n"
    "    }\n"
    "\n"
    "    gl_FragColor = vec4(\n"
    "        clamp(result, 0.0, 1.0), 1.0);\n"
    "}\n";

static GLuint illumination_compile(GLenum type,
                                   const char *source)
{
    GLuint shader = glCreateShader(type);

    if (!shader)
        return 0;

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint status;

    glGetShaderiv(shader, GL_COMPILE_STATUS,
                  &status);

    if (!status) {
        char log[4096];

        glGetShaderInfoLog(shader, sizeof(log),
                           NULL, log);

        fprintf(stderr,
                "Illumination shader error:\n%s\n",
                log);

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static int illumination_init(void)
{
    GLuint vertex = illumination_compile(
        GL_VERTEX_SHADER,
        illumination_vertex_source);

    if (!vertex)
        return 0;

    GLuint fragment = illumination_compile(
        GL_FRAGMENT_SHADER,
        illumination_fragment_source);

    if (!fragment) {
        glDeleteShader(vertex);
        return 0;
    }

    illumination_program = glCreateProgram();

    if (!illumination_program) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }

    glAttachShader(illumination_program, vertex);
    glAttachShader(illumination_program, fragment);

    glLinkProgram(illumination_program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint status;

    glGetProgramiv(illumination_program,
                   GL_LINK_STATUS, &status);

    if (!status) {
        char log[4096];

        glGetProgramInfoLog(
            illumination_program,
            sizeof(log), NULL, log);

        fprintf(stderr,
                "Illumination link error:\n%s\n",
                log);

        glDeleteProgram(illumination_program);
        illumination_program = 0;

        return 0;
    }

    illumination_sun = glGetUniformLocation(
        illumination_program,
        "sun_direction");

    GLint sampler = glGetUniformLocation(
        illumination_program,
        "earth_texture");

    if (illumination_sun < 0 || sampler < 0) {
        fprintf(stderr,
                "Illumination uniforms unavailable\n");

        glDeleteProgram(illumination_program);
        illumination_program = 0;

        return 0;
    }

    glUseProgram(illumination_program);
    GLint night_sampler = glGetUniformLocation(
        illumination_program,
        "night_texture");

    illumination_mode_uniform =
        glGetUniformLocation(
            illumination_program,
            "display_mode");

    illumination_contour_uniform =
        glGetUniformLocation(
            illumination_program,
            "show_terminator");

    if (night_sampler < 0 ||
        illumination_mode_uniform < 0 ||
        illumination_contour_uniform < 0) {

        fprintf(stderr,
                "Night illumination uniforms unavailable\n");

        glDeleteProgram(illumination_program);
        illumination_program = 0;

        return 0;
    }

    glUniform1i(sampler, 0);
    glUniform1i(night_sampler, 1);

    glUseProgram(0);

    puts("Solar illumination shader initialized");

    return 1;
}

static void illumination_begin(void)
{
    time_t now = time(NULL);

    if (illumination_updated == 0 ||
        difftime(now, illumination_updated) >= 60.0 ||
        now < illumination_updated) {

        if (solar_calculate(now,
                            &illumination_state)) {

            illumination_updated = now;

            printf("Subsolar point: "
                   "%.4f lat, %.4f lon\n",
                   illumination_state.subsolar_lat,
                   illumination_state.subsolar_lon);
        }
    }

    glUseProgram(illumination_program);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,
                  illumination_night_texture);

    glActiveTexture(GL_TEXTURE0);

    glUniform1i(illumination_mode_uniform,
                illumination_mode);

    glUniform1i(illumination_contour_uniform,
                illumination_contour);

    glUniform3f(

        illumination_sun,
        (GLfloat)illumination_state.x,
        (GLfloat)illumination_state.y,
        (GLfloat)illumination_state.z);
}

static void illumination_end(void)
{
    glUseProgram(0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0);
}

static void illumination_key(KeySym key)
{
    switch (key) {

    case XK_d:
    case XK_D:
        illumination_mode =
            illumination_mode == 0 ? 1 : 0;
        break;

    case XK_n:
    case XK_N:
        illumination_mode =
            illumination_mode == 1 ? 0 : 1;
        break;

    case XK_g:
    case XK_G:
        illumination_mode =
            illumination_mode == 2 ? 1 : 2;
        break;

    case XK_t:
    case XK_T:
        illumination_contour =
            !illumination_contour;

        printf("Terminator contour: %s\n",
               illumination_contour
               ? "enabled" : "disabled");
        return;

    default:
        return;
    }

    static const char *modes[] = {
        "Full daylight",
        "Natural day/night",
        "Enhanced gray-line"
    };

    printf("Illumination: %s\n",
           modes[illumination_mode]);
}



static void illumination_cleanup(void)
{
    if (illumination_program)
        glDeleteProgram(illumination_program);
}

#endif
