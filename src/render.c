#include "app.h"
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <SDL3/SDL_opengl.h>
#endif
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __EMSCRIPTEN__
#define GL_API(ret, name, args) static ret(APIENTRY *q##name) args;
GL_API(GLuint, CreateShader, (GLenum))
GL_API(void, ShaderSource, (GLuint, GLsizei, const GLchar *const *, const GLint *))
GL_API(void, CompileShader, (GLuint))
GL_API(void, GetShaderiv, (GLuint, GLenum, GLint *))
GL_API(void, GetShaderInfoLog, (GLuint, GLsizei, GLsizei *, GLchar *))
GL_API(GLuint, CreateProgram, (void)) GL_API(void, AttachShader, (GLuint, GLuint))
    GL_API(void, LinkProgram, (GLuint)) GL_API(void, GetProgramiv, (GLuint, GLenum, GLint *))
        GL_API(void, UseProgram, (GLuint)) GL_API(GLint, GetUniformLocation,
                                                  (GLuint, const GLchar *))
            GL_API(void, UniformMatrix4fv, (GLint, GLsizei, GLboolean, const GLfloat *))
                GL_API(void, Uniform1i, (GLint, GLint)) GL_API(void, Uniform1f, (GLint, GLfloat))
                    GL_API(void, Uniform3f, (GLint, GLfloat, GLfloat, GLfloat))
                        GL_API(void, GenVertexArrays,
                               (GLsizei, GLuint *)) GL_API(void, BindVertexArray, (GLuint))
                            GL_API(void, GenBuffers, (GLsizei, GLuint *)) GL_API(void, BindBuffer,
                                                                                 (GLenum, GLuint))
                                GL_API(void, BufferData, (GLenum, GLsizeiptr, const void *, GLenum))
                                    GL_API(void, EnableVertexAttribArray, (GLuint))
                                        GL_API(void, VertexAttribPointer,
                                               (GLuint, GLint, GLenum, GLboolean, GLsizei,
                                                const void *)) GL_API(void, DeleteShader, (GLuint))
                                            GL_API(void, DeleteProgram, (GLuint))
                                                GL_API(void, DeleteBuffers,
                                                       (GLsizei, const GLuint *))
                                                    GL_API(void, DeleteVertexArrays,
                                                           (GLsizei, const GLuint *))
#define glCreateShader qCreateShader
#define glShaderSource qShaderSource
#define glCompileShader qCompileShader
#define glGetShaderiv qGetShaderiv
#define glGetShaderInfoLog qGetShaderInfoLog
#define glCreateProgram qCreateProgram
#define glAttachShader qAttachShader
#define glLinkProgram qLinkProgram
#define glGetProgramiv qGetProgramiv
#define glUseProgram qUseProgram
#define glGetUniformLocation qGetUniformLocation
#define glUniformMatrix4fv qUniformMatrix4fv
#define glUniform1i qUniform1i
#define glUniform1f qUniform1f
#define glUniform3f qUniform3f
#define glGenVertexArrays qGenVertexArrays
#define glBindVertexArray qBindVertexArray
#define glGenBuffers qGenBuffers
#define glBindBuffer qBindBuffer
#define glBufferData qBufferData
#define glEnableVertexAttribArray qEnableVertexAttribArray
#define glVertexAttribPointer qVertexAttribPointer
#define glDeleteShader qDeleteShader
#define glDeleteProgram qDeleteProgram
#define glDeleteBuffers qDeleteBuffers
#define glDeleteVertexArrays qDeleteVertexArrays
#endif

                                                        typedef struct {
    float p[3], color[4], uv[2];
} Vertex;
#define MAX_VERTS 350000
static Vertex verts[MAX_VERTS];
static Vertex *write_buffer = verts;
static int count;
static GLuint program, vbo, vao, texture;
static GLint umatrix, ufog, udaylight, ucamera, uui;
static unsigned char font[128][35];
static WcArt *art;
static WcPos origin;
static double castle_angle;
static WcVec castle_offset;
static float day = 1;
static WcColor sky;
static int local_mode;
typedef struct {
    int64_t cx, cz;
    uint32_t seed;
    int ready, used;
    Vertex mesh[32 * 32 * 6];
} TerrainChunk;
static TerrainChunk chunks[WC_CHUNKS];

static WcColor color(int m) {
    return art->palette[m & 15];
}
static void vertex(WcVec p, WcColor c, float shade, float u, float v, float alpha) {
    if (count >= MAX_VERTS)
        return;
    if (local_mode) {
        double x = p.x, z = p.z;
        p.x = cos(castle_angle) * x - sin(castle_angle) * z + castle_offset.x;
        p.z = sin(castle_angle) * x + cos(castle_angle) * z + castle_offset.z;
        p.y += castle_offset.y;
    }
    Vertex *o = &write_buffer[count++];
    o->p[0] = (float)p.x;
    o->p[1] = (float)p.y;
    o->p[2] = (float)p.z;
    o->color[0] = c.r * shade;
    o->color[1] = c.g * shade;
    o->color[2] = c.b * shade;
    o->color[3] = alpha;
    o->uv[0] = u;
    o->uv[1] = v;
}
static void quad(WcVec a, WcVec b, WcVec c, WcVec d, WcColor col, float shade, float repeat) {
    vertex(a, col, shade, 0, 0, 1);
    vertex(b, col, shade, repeat, 0, 1);
    vertex(c, col, shade, repeat, repeat, 1);
    vertex(a, col, shade, 0, 0, 1);
    vertex(c, col, shade, repeat, repeat, 1);
    vertex(d, col, shade, 0, repeat, 1);
}
static void cube(double x, double y, double z, double w, double h, double d, int m) {
    WcColor col = color(m);
    double x0 = x - w * .5, x1 = x + w * .5, y0 = y - h * .5, y1 = y + h * .5, z0 = z - d * .5,
           z1 = z + d * .5;
    WcVec p[8] = {{x0, y0, z0}, {x1, y0, z0}, {x1, y1, z0}, {x0, y1, z0},
                  {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}};
    quad(p[4], p[5], p[6], p[7], col, .8, 2);
    quad(p[1], p[0], p[3], p[2], col, .72, 2);
    quad(p[0], p[4], p[7], p[3], col, .65, 2);
    quad(p[5], p[1], p[2], p[6], col, .88, 2);
    quad(p[3], p[7], p[6], p[2], col, 1, 2);
    quad(p[0], p[1], p[5], p[4], col, .5, 2);
}
static void beam(WcVec a, WcVec b, double width, int m) {
    WcVec up = {0, width * .5, 0}, side = {width * .5, 0, width * .5};
    WcColor c = color(m);
    WcVec p[8] = {
        {a.x - side.x, a.y - up.y, a.z - side.z}, {a.x + side.x, a.y - up.y, a.z + side.z},
        {a.x + side.x, a.y + up.y, a.z + side.z}, {a.x - side.x, a.y + up.y, a.z - side.z},
        {b.x - side.x, b.y - up.y, b.z - side.z}, {b.x + side.x, b.y - up.y, b.z + side.z},
        {b.x + side.x, b.y + up.y, b.z + side.z}, {b.x - side.x, b.y + up.y, b.z - side.z}};
    quad(p[0], p[1], p[5], p[4], c, .8, 1);
    quad(p[1], p[2], p[6], p[5], c, .9, 1);
    quad(p[2], p[3], p[7], p[6], c, 1, 1);
    quad(p[3], p[0], p[4], p[7], c, .7, 1);
}
static void roof(double x, double y, double z, double w, double d) {
    WcColor c = color(5);
    quad((WcVec){x - w * .5, y, z - d * .5}, (WcVec){x, y + 1.4, z - d * .5},
         (WcVec){x, y + 1.4, z + d * .5}, (WcVec){x - w * .5, y, z + d * .5}, c, .85, 6);
    quad((WcVec){x, y + 1.4, z - d * .5}, (WcVec){x + w * .5, y, z - d * .5},
         (WcVec){x + w * .5, y, z + d * .5}, (WcVec){x, y + 1.4, z + d * .5}, c, 1, 6);
    for (int k = 0; k < 4; k++)
        beam((WcVec){x - w * .5 + (w * .5) * k / 3, y + 1.4 * k / 3, z - d * .5},
             (WcVec){x - w * .5 + (w * .5) * k / 3, y + 1.4 * k / 3, z + d * .5}, .08, 3);
}
static void plant(double x, double y, double z, int kind, double age) {
    double h = .2 + .65 * age;
    cube(x, y + h * .5, z, .08, h, .08, 4);
    for (int i = 0; i < 4; i++) {
        double a = i * 1.57;
        cube(x + cos(a) * .18, y + h * .55, z + sin(a) * .18, .35, .09, .27, kind == 0 ? 14 : 15);
    }
    if (kind == 0)
        cube(x, y + h, z, .24, .24, .24, 10);
}
static void cutout(WcVec p, double width, double height, int material) {
    WcColor c = color(material);
    for (int cross = 0; cross < 2; cross++) {
        WcVec a = {p.x - (cross ? 0 : width * .5), p.y, p.z - (cross ? width * .5 : 0)};
        WcVec b = {p.x + (cross ? 0 : width * .5), p.y, p.z + (cross ? width * .5 : 0)}, d = a,
              top = b;
        d.y += height;
        top.y += height;
        vertex(a, c, 1, 0, 1, -1);
        vertex(b, c, 1, 1, 1, -1);
        vertex(top, c, 1, 1, 0, -1);
        vertex(a, c, 1, 0, 1, -1);
        vertex(top, c, 1, 1, 0, -1);
        vertex(d, c, 1, 0, 0, -1);
    }
}
static void wall_x(double x, double y, double z, double d, int door) {
    cube(x, y + .48, z, .2, .96, d, 1);
    cube(x, y + 3.05, z, .22, .65, d, 2);
    int spans = (int)(d / 3);
    for (int i = 0; i <= spans; i++)
        cube(x, y + 1.8, z - d * .5 + i * d / spans, .3, 3.4, .22, 2);
    if (door) {
        cube(x, y + .48, z, .22, .98, 2.8, 2); /* portals removed by caller below */
    }
}
static void wall_z(double x, double y, double z, double w, int door) {
    if (door) {
        cube(x - w * .25 - .7, y + 1.7, z, w * .5 - 1.4, 3.4, .22, 2);
        cube(x + w * .25 + .7, y + 1.7, z, w * .5 - 1.4, 3.4, .22, 2);
        cube(x, y + 3.1, z, 2.8, .6, .22, 2);
    } else {
        cube(x, y + .48, z, w, .96, .22, 1);
        cube(x, y + 3.05, z, w, .65, .22, 2);
        for (int i = 0; i <= (int)(w / 3); i++)
            cube(x - w * .5 + i * 3, y + 1.8, z, .22, 3.4, .3, 2);
    }
}
static void castle(WcApp *a) {
    WcState *s = &a->game.s;
    local_mode = 1;
    castle_angle = s->castle_yaw;
    castle_offset = wc_relative(s->castle, origin);
    /* Tapered armored belly, substantial articulated six-legged undercarriage. */
    cube(0, -.6, 0, 11, 1.1, 15, 3);
    cube(0, -1.4, 0, 8, .6, 12, 3);
    for (int side = -1; side <= 1; side += 2)
        for (int leg = 0; leg < 3; leg++) {
            double z = (leg - 1) * 5.3;
            double swing = sin(s->time * 2.5 + leg * 2 + side) * fmin(1, fabs(s->speed) / 5);
            WcPos foot_world = wc_local_world(s, (WcVec){side * 7.3, 0, z + swing});
            double foot = wc_height(s->seed, foot_world) - s->castle.y + .25;
            WcVec hip = {side * 4.8, -1.1, z}, knee = {side * 6.8, -2, z + swing * .6},
                  toe = {side * 7.4, foot + .4, z + swing};
            beam(hip, knee, .65, 3);
            beam(knee, toe, .45, 3);
            beam((WcVec){hip.x, hip.y + .4, hip.z + .3}, (WcVec){knee.x, knee.y + .3, knee.z + .3},
                 .19, 5);
            cube(knee.x, knee.y, knee.z, .9, .9, .9, 5);
            cube(toe.x, foot, toe.z, 1.6, .45, 2, 3);
        }
    for (int i = 0; i < s->room_count; i++) {
        WcRoom r = s->rooms[i];
        WcVec dimensions = wc_room_dimensions(r.kind, r.rotation);
        WcVec center = wc_room_center(&r);
        double x = center.x, y = center.y, z = center.z, w = dimensions.x, d = dimensions.z;
        int upper = 0, left = 0, right = 0, front = 0, back = 0;
        for (int j = 0; j < s->room_count; j++) {
            WcRoom o = s->rooms[j];
            if (o.parent == i && o.tier > r.tier)
                upper = 1;
            if (o.tier == r.tier && o.gx == r.gx - 1 && o.gz == r.gz)
                left = 1;
            if (o.tier == r.tier && o.gx == r.gx + 1 && o.gz == r.gz)
                right = 1;
            if (o.tier == r.tier && o.gx == r.gx && o.gz == r.gz - 1)
                front = 1;
            if (o.tier == r.tier && o.gx == r.gx && o.gz == r.gz + 1)
                back = 1;
        }
        if (i == 0 || (r.parent >= 0 && s->rooms[r.parent].tier < r.tier)) {
            if (i == 0)
                cube(x, y - .15, z, w, .3, d, 7);
            else {
                cube(x - w * .25 - .55, y - .15, z, w * .5 - 1.1, .3, d, 7);
                cube(x + w * .25 + .55, y - .15, z, w * .5 - 1.1, .3, d, 7);
                cube(x, y - .15, z - d * .25 - 1.5, 2.2, .3, d * .5 - 3, 7);
                cube(x, y - .15, z + d * .25 + 1.5, 2.2, .3, d * .5 - 3, 7);
            }
        } else
            cube(x, y - .15, z, w, .3, d, 7);
        for (double xx = -w * .5 + .8; xx < w * .5; xx += 1.0)
            beam((WcVec){x + xx, y + .008, z - d * .5}, (WcVec){x + xx, y + .008, z + d * .5}, .018,
                 2);
        if (!left)
            wall_x(x - w * .5, y, z, d, 0);
        else {
            wall_x(x - w * .5, y, z - d * .25 - 1, d * .5 - 2, 0);
            wall_x(x - w * .5, y, z + d * .25 + 1, d * .5 - 2, 0);
            cube(x - w * .5, y + 3, z, .2, .8, 2, 2);
        }
        if (!right)
            wall_x(x + w * .5, y, z, d, 0);
        else {
            wall_x(x + w * .5, y, z - d * .25 - 1, d * .5 - 2, 0);
            wall_x(x + w * .5, y, z + d * .25 + 1, d * .5 - 2, 0);
            cube(x + w * .5, y + 3, z, .2, .8, 2, 2);
        }
        wall_z(x, y, z - d * .5, w, front);
        wall_z(x, y, z + d * .5, w, i == 0 || back);
        if (!upper) {
            cube(x, y + 3.7, z, w, .15, d, 7);
            roof(x, y + 3.85, z, w + .8, d + .8);
        }
        if (r.parent >= 0 && s->rooms[r.parent].tier < r.tier) {
            for (int step = 0; step < 24; step++) {
                double zz = z - 3 + (step + .5) * 6 / 24, yy = y - (step + .5) * 4 / 24;
                cube(x, yy - .09, zz, 2, .18, .28, 7);
            }
            beam((WcVec){x - 1.05, y + 1, z - 3}, (WcVec){x - 1.05, y - 3, z + 3}, .1, 5);
            beam((WcVec){x + 1.05, y + 1, z - 3}, (WcVec){x + 1.05, y - 3, z + 3}, .1, 5);
        }
        if (i > 0) {
            cube(x - 2, y + .65, z, 1.5, 1.3, .8, 2);
            cube(x - 2, y + .7, z + .42, .08, .12, .05, 10);
            cube(x + 2, y + .4, z + 1, 1.8, .8, 1.2, 2);
            cube(x + 2, y + .8, z + 1, 1.6, .12, 1, 6);
            beam((WcVec){x - w * .5, -.5, z - d * .5}, (WcVec){x, y - .4, z}, .3, 3);
        }
        for (int k = 0; k < 2; k++) {
            int ci = i * 2 + k;
            double px = i == 0 ? 4 : x + 2, pz = i == 0 ? -3 + k * 3 : z + 1 + k * 1.5;
            if (i > 0 && k == 0)
                continue;
            cube(px, y + .45, pz, 1.65, .9, 1.3, 7);
            cube(px, y + .92, pz, 1.45, .08, 1.1, 6);
            WcCrop crop = s->crops[ci];
            if (crop.kind >= 0)
                for (int v = 0; v < 3; v++)
                    plant(px - .45 + v * .45, y + .96, pz, crop.kind,
                          fmin(1, (s->time - crop.planted) / crop.duration));
        }
    }
    for (int i = 0; i < art->box_count; i++) {
        WcArtBox b = art->home[i];
        cube(b.p.x, b.p.y, b.p.z, b.size.x, b.size.y, b.size.z, b.material);
    }
    /* A readable physical wheel, map and gate shelter. */
    for (int i = 0; i < 12; i++) {
        double t = i * WC_PI / 6;
        cube(cos(t) * .6, 1.55 + sin(t) * .6, -5.8, .19, .19, .18, 5);
    }
    cube(0, 1.55, -5.8, .18, .18, .3, 3);
    beam((WcVec){-.5, 1.55, -5.8}, (WcVec){.5, 1.55, -5.8}, .09, 5);
    beam((WcVec){0, 1, -5.8}, (WcVec){0, 2.1, -5.8}, .09, 5);
    if (!s->gate) {
        cube(-.72, 1.35, 8, 1.4, 2.7, .2, 2);
        cube(.72, 1.35, 8, 1.4, 2.7, .2, 2);
        cube(-.2, 1.2, 8.14, .08, .3, .12, 5);
        cube(.2, 1.2, 8.14, .08, .3, .12, 5);
    } else {
        cube(-1.55, 1.35, 8.7, .2, 2.7, 1.4, 2);
        cube(1.55, 1.35, 8.7, .2, 2.7, 1.4, 2);
    }
    roof(0, 3, 8.8, 4.2, 3);
    cube(-1.8, 1.5, 9.4, .2, 3, .2, 2);
    cube(1.8, 1.5, 9.4, .2, 3, .2, 2);
    if (s->stairs.deployed) {
        WcVec points[3] = {
            {0, 0, 8}, s->stairs.turned ? s->stairs.bend : s->stairs.landing, s->stairs.landing};
        for (int f = 0; f < (s->stairs.turned ? 2 : 1); f++) {
            WcVec a0 = points[f], b = points[f + 1];
            double dx = b.x - a0.x, dz = b.z - a0.z, len = hypot(dx, dz);
            double sx = dz / len, sz = -dx / len;
            quad((WcVec){a0.x - sx, a0.y, a0.z - sz}, (WcVec){a0.x + sx, a0.y, a0.z + sz},
                 (WcVec){b.x + sx, b.y, b.z + sz}, (WcVec){b.x - sx, b.y, b.z - sz}, color(7), 1,
                 (float)len);
            for (int side = -1; side <= 1; side += 2) {
                WcVec q = {a0.x + sx * side, a0.y + 1, a0.z + sz * side},
                      r = {b.x + sx * side, b.y + 1, b.z + sz * side};
                beam(q, r, .1, 5);
                for (double t = 0; t <= 1; t += 1 / fmax(1, len / 1.5)) {
                    WcVec bot = {a0.x + dx * t + sx * side, a0.y + (b.y - a0.y) * t,
                                 a0.z + dz * t + sz * side};
                    WcVec top = bot;
                    top.y++;
                    beam(bot, top, .07, 3);
                }
            }
        }
        WcVec land = s->stairs.landing;
        cube(land.x, land.y - .06, land.z, 2.5, .12, 2, 7);
        cube(land.x + 1.4, land.y + .65, land.z, .12, 1.3, .12, 3);
        cube(land.x + 1.4, land.y + 1.35, land.z, .4, .4, .4, 5);
    }
    if (s->hearth) {
        cube(-3, .5, -5, 1.8, 1, 1.3, 1);
        cube(-3, .55, -4.32, .8, .4, .08, 10);
    }
    if (s->tempered) {
        cube(-4, 1.3, -1, 1, .3, .6, 3);
    }
    /* Braced lookout above the highest central roof. */
    double top = 4;
    for (int i = 0; i < s->room_count; i++)
        top = fmax(top, s->rooms[i].tier * 4 + 4);
    for (int x = -1; x <= 1; x += 2)
        for (int z = -1; z <= 1; z += 2)
            cube(x * .8, top + .6, z * .8, .12, 1.4, .12, 2);
    roof(0, top + 1.3, 0, 2.4, 2.4);
    local_mode = 0;
}
static void terrain(WcApp *a) {
    WcState *s = &a->game.s;
    WcPos center = s->indoor ? s->castle : s->player;
    /* Nine visible chunks, a bounded LRU of 32, no unbounded generation queue. */
    for (int dz = -1; dz <= 1; dz++)
        for (int dx = -1; dx <= 1; dx++) {
            WcPos base = {center.cx + dx, center.cz + dz, 0, 0, 0};
            TerrainChunk *chunk = NULL, *oldest = &chunks[0];
            for (int i = 0; i < WC_CHUNKS; i++) {
                TerrainChunk *candidate = &chunks[i];
                if (candidate->ready && candidate->seed == s->seed && candidate->cx == base.cx &&
                    candidate->cz == base.cz) {
                    chunk = candidate;
                    break;
                }
                if (!candidate->ready || candidate->used < oldest->used)
                    oldest = candidate;
            }
            if (!chunk) {
                chunk = oldest;
                WcPos previous_origin = origin;
                origin = base;
                int previous_count = count;
                count = 0;
                write_buffer = chunk->mesh;
                int step = 4;
                for (int z = 0; z < 128; z += step)
                    for (int x = 0; x < 128; x += step) {
                        WcPos p[4] = {base, base, base, base};
                        wc_pos_add(&p[0], x, 0, z);
                        wc_pos_add(&p[1], x + step, 0, z);
                        wc_pos_add(&p[2], x + step, 0, z + step);
                        wc_pos_add(&p[3], x, 0, z + step);
                        WcVec v[4];
                        for (int k = 0; k < 4; k++) {
                            p[k].y = wc_height(s->seed, p[k]);
                            v[k] = wc_relative(p[k], origin);
                        }
                        WcColor c = color(wc_biome(s->seed, p[0]) == 2 ? 1 : 0);
                        double n = (wc_hash((uint64_t)(x + z * 129) ^ wc_hash((uint64_t)base.cx) ^
                                            wc_hash((uint64_t)base.cz)) %
                                    100) /
                                   100.0;
                        c.r *= .9 + n * .15;
                        c.g *= .9 + n * .15;
                        c.b *= .9 + n * .15;
                        quad(v[0], v[1], v[2], v[3], c, 1, 4);
                    }
                chunk->cx = base.cx;
                chunk->cz = base.cz;
                chunk->seed = s->seed;
                chunk->ready = 1;
                origin = previous_origin;
                count = previous_count;
                write_buffer = verts;
            }
            chunk->used = a->frame_count + 1;
            WcVec offset = wc_relative(base, origin);
            for (int i = 0; i < 32 * 32 * 6 && count < MAX_VERTS; i++) {
                Vertex v = chunk->mesh[i];
                v.p[0] += (float)offset.x;
                v.p[2] += (float)offset.z;
                verts[count++] = v;
            }
        }
    /* Non-colliding horizon has a fixed 64 segments and does not allocate chunks. */
    WcVec eye = wc_relative(center, origin);
    for (int i = 0; i < 64; i++) {
        double t = i * WC_PI / 32, t2 = (i + 1) * WC_PI / 32;
        double h = 24 + sin(t * 3) * 12 + sin(t * 7) * 7;
        quad((WcVec){eye.x + sin(t) * 360, -15, eye.z + cos(t) * 360},
             (WcVec){eye.x + sin(t2) * 360, -15, eye.z + cos(t2) * 360},
             (WcVec){eye.x + sin(t2) * 360, h, eye.z + cos(t2) * 360},
             (WcVec){eye.x + sin(t) * 360, h, eye.z + cos(t) * 360}, (WcColor){.39, .48, .49}, 1,
             1);
    }
}
static void scenery(WcApp *a) {
    WcState *s = &a->game.s;
    for (int i = 0; i < a->scene.count; i++) {
        WcFeature f = a->scene.f[i];
        WcVec p = wc_relative(f.p, origin);
        double distance = hypot(p.x, p.z);
        if (distance > 280)
            continue;
        int depleted = 0;
        for (int m = 0; m < s->mutation_count; m++)
            if (s->mutations[m].id == f.id &&
                (s->mutations[m].ready > s->time || s->mutations[m].flags & 2))
                depleted = 1;
        if (f.type == FEATURE_TREE) {
            cube(p.x, p.y + 3, p.z, .8, 6, .8, 2);
            for (int k = 0; k < 3; k++)
                cube(p.x, p.y + 5 + k * 1.7, p.z, 5 - k * 1.2, 2.5, 5 - k * 1.2, 4);
        }
        if (f.type == FEATURE_TIMBER) {
            cube(p.x, p.y + .3, p.z, 1.6, .6, .7, 2);
            if (!depleted) {
                cube(p.x + .35, p.y + .7, p.z, 1, .4, .6, 7);
            }
        }
        if (f.type == FEATURE_STONE)
            cube(p.x, p.y + (depleted ? .15 : .5), p.z, 1.3, depleted ? .3 : 1, 1, 1);
        if (f.type == FEATURE_FIBER || f.type == FEATURE_SEED) {
            if (!depleted)
                for (int j = 0; j < 4; j++)
                    plant(p.x + sin(j * 2) * .4, p.y, p.z + cos(j * 2) * .4,
                          f.type == FEATURE_SEED ? 0 : 1, 1);
            if (!depleted)
                cutout(p, 2, 1.5, 4);
        }
        if (f.type == FEATURE_TRADER) {
            cube(p.x, p.y + 1, p.z, 1, 2, .7, 11);
            cube(p.x, p.y + 2.25, p.z, .7, .6, .65, 9);
            cube(p.x, p.y + 2.6, p.z, 1.3, .18, 1, 5);
            cube(p.x + 2, p.y + .7, p.z, 2, 1.4, 1.8, 2);
            roof(p.x, p.y + 3, p.z, 5, 3);
        }
        if (f.type == FEATURE_PROWLER && !depleted) {
            double phase = fmod(s->time + (f.id % 31) * .1, 3);
            cube(p.x, p.y + .65, p.z, 1.3, 1.3, .9, phase < 1.4 ? 13 : 11);
            cube(p.x, p.y + 1.35, p.z, .7, .6, .7, 3);
            cube(p.x - .22, p.y + 1.4, p.z + .37, .12, .12, .05, 10);
            cube(p.x + .22, p.y + 1.4, p.z + .37, .12, .12, .05, 10);
        }
        if (f.type == FEATURE_FOUNDRY || (f.type >= FEATURE_KEEP && f.type <= FEATURE_HAMLET)) {
            WcBox boxes[64];
            int n = wc_site_boxes(f.type, boxes, 64);
            for (int b = 0; b < n; b++) {
                WcBox v = boxes[b];
                cube(p.x + v.center.x, p.y + v.center.y, p.z + v.center.z, v.half.x * 2,
                     v.half.y * 2, v.half.z * 2, v.material);
            }
            if (f.type == FEATURE_HAMLET)
                for (int j = 0; j < 4; j++)
                    roof(p.x + (j % 2 ? 1 : -1) * 10, p.y + 4, p.z + (j / 2 ? 1 : -1) * 11, 9, 9);
            if (f.type == FEATURE_MINE)
                roof(p.x, p.y + 4, p.z, 18, 56);
            if (f.type == FEATURE_FOUNDRY) {
                cube(p.x - 5, p.y + .7, p.z + 5, 1.5, 1.4, .7, 2);
                cube(p.x - 5, p.y + 1.5, p.z + 5, 1.4, .1, 1, 8);
                cube(p.x, p.y + .035, p.z + 3, 2.6, .07, 2.2,
                     s->foundry_trap          ? 3
                     : fmod(s->time, 3) > 1.2 ? 13
                                              : 5);
                cube(p.x + 6, p.y + 1, p.z - 5, .3, 2, .3, 5);
                beam((WcVec){p.x + 6, p.y + 1.6, p.z - 5}, (WcVec){p.x + 5.5, p.y + 2, p.z - 5},
                     .15, 13);
                if (!s->foundry_brake) {
                    cube(p.x, p.y + 1.6, p.z - 8, 2.2, 3.2, 1.6, 3);
                    cube(p.x, p.y + 1.8, p.z - 7.15, .6, .7, .1, fmod(s->time, 3) > 1.8 ? 10 : 13);
                }
                if (!s->foundry_reserve)
                    cube(p.x, p.y + .5, p.z - 9, 2, 1, 1, 5);
                if (!(s->achievements & (1u << 8)))
                    cube(p.x - 5, p.y + .8, p.z + 2, 1.5, 1.6, 1, 5);
                for (int j = 0; j < 5; j++)
                    cube(p.x + 5.8, p.y + .02, p.z + 3 - j * 2, .5, .04, .8, 10);
            } else {
                cube(p.x, p.y + 1, p.z, 1, 2, .6, 2);
                cube(p.x, p.y + 2, p.z, 1.5, .8, .2, 8);
            }
        }
        if (f.type == FEATURE_CLUE) {
            cube(p.x, p.y + .6, p.z, .8, 1.2, .8, 1);
            cube(p.x, p.y + 1.3, p.z, 1, .2, .8, 5);
        }
    }
    for (int i = 0; i < WC_CACHES; i++)
        if (s->caches[i].active) {
            WcVec p = wc_relative(s->caches[i].pos, origin);
            cube(p.x, p.y + .4, p.z, 1, .8, .8, 12);
            cube(p.x, p.y + 1.3, p.z, .12, 1, .12, 10);
        }
    /* Sun, moon, stars and drifting flat-bottomed pixel clouds. */
    WcVec eye = wc_relative(s->indoor ? s->castle : s->player, origin);
    double phase = (9.0 / 24 + s->time / 720) * WC_PI * 2;
    cube(eye.x + cos(phase) * 180, eye.y + sin(phase) * 150, eye.z - 150, 10, 10, 2, 10);
    cube(eye.x - cos(phase) * 180, eye.y - sin(phase) * 150, eye.z + 150, 6, 6, 2, 8);
    for (int i = 0; i < 20; i++) {
        uint64_t h = wc_hash(i + 70);
        double x = (double)(h % 400) - 200, z = (double)((h >> 16) % 400) - 200;
        cube(eye.x + x + fmod(s->time * .4, 100), 55 + i % 4 * 8, eye.z + z, 18 + i % 3 * 6, 2.4, 8,
             8);
    }
    if (day < .4)
        for (int i = 0; i < 65; i++) {
            uint64_t h = wc_hash(i + 300);
            cube(eye.x + (double)(h % 400) - 200, eye.y + 70 + (double)((h >> 16) % 130),
                 eye.z + (double)((h >> 32) % 400) - 200, .4, .4, .4, 8);
        }
}
static void identity(float *m) {
    memset(m, 0, 64);
    m[0] = m[5] = m[10] = m[15] = 1;
}
static void multiply(float *out, const float *a, const float *b) {
    float r[16] = {0};
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++)
            for (int k = 0; k < 4; k++)
                r[col * 4 + row] += a[k * 4 + row] * b[col * 4 + k];
    memcpy(out, r, 64);
}
static void camera_matrix(WcApp *a, float *m, WcVec *eye_out) {
    WcState *s = &a->game.s;
    WcPos p = s->indoor ? wc_local_world(s, s->local) : s->player;
    WcVec eye = wc_relative(p, origin);
    eye.y += a->xr ? 0 : 1.62;
    *eye_out = eye;
    double yaw = a->xr ? a->xr_base_yaw : s->yaw, pitch = a->xr ? 0 : s->pitch;
    WcVec f = {sin(yaw) * cos(pitch), sin(pitch), -cos(yaw) * cos(pitch)},
          r = {cos(yaw), 0, sin(yaw)},
          up = {-sin(yaw) * sin(pitch), cos(pitch), cos(yaw) * sin(pitch)};
    float v[16];
    identity(v);
    v[0] = r.x;
    v[4] = r.y;
    v[8] = r.z;
    v[1] = up.x;
    v[5] = up.y;
    v[9] = up.z;
    v[2] = -f.x;
    v[6] = -f.y;
    v[10] = -f.z;
    v[12] = -(r.x * eye.x + r.z * eye.z);
    v[13] = -(up.x * eye.x + up.y * eye.y + up.z * eye.z);
    v[14] = f.x * eye.x + f.y * eye.y + f.z * eye.z;
    float projection[16] = {0};
    float t = 1 / tanf(75 * WC_PI / 360);
    projection[0] = t / (a->width / (float)a->height);
    projection[5] = t;
    projection[10] = -1.0004f;
    projection[11] = -1;
    projection[14] = -.120024f;
    if (a->xr) {
        float head[16];
        multiply(head, a->xr_view, v);
        multiply(m, a->xr_projection, head);
    } else
        multiply(m, projection, v);
}
static void flush(float *matrix, int ui, WcVec eye) {
    glUseProgram(program);
    glUniformMatrix4fv(umatrix, 1, GL_FALSE, matrix);
    glUniform1i(uui, ui);
    glUniform1f(udaylight, day);
    glUniform3f(ufog, sky.r, sky.g, sky.b);
    glUniform3f(ucamera, eye.x, eye.y, eye.z);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(count * sizeof(Vertex)), verts, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, count);
    count = 0;
}
static void rect(float x, float y, float w, float h, WcColor c, float alpha) {
    WcVec p[4] = {{x, y, 0}, {x + w, y, 0}, {x + w, y + h, 0}, {x, y + h, 0}};
    int indices[] = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < 6; i++)
        vertex(p[indices[i]], c, 1, .5, .5, alpha);
}
static WcColor cream = {.92, .89, .77}, gold = {.89, .67, .35}, muted = {.55, .65, .65},
               ink = {.055, .09, .11}, green = {.49, .70, .48};
static void text(float x, float y, float scale, WcColor c, const char *fmt, ...) {
    char line[300];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    float start = x;
    for (const unsigned char *p = (unsigned char *)line; *p; p++) {
        if (*p == '\n') {
            y += 9 * scale;
            x = start;
            continue;
        }
        int ch = toupper(*p);
        if (ch < 128)
            for (int row = 0; row < 7; row++)
                for (int col = 0; col < 5; col++)
                    if (font[ch][row * 5 + col])
                        rect(x + col * scale, y + row * scale, scale, scale, c, 1);
        x += 6 * scale;
    }
}
static void wrapped(float x, float y, float width, float scale, WcColor c, const char *line) {
    char part[200];
    int max = (int)(width / (6 * scale)), n = (int)strlen(line), at = 0;
    while (at < n) {
        int len = n - at;
        if (len > max) {
            len = max;
            for (int i = max; i > max / 2; i--)
                if (line[at + i] == ' ') {
                    len = i;
                    break;
                }
        }
        memcpy(part, line + at, len);
        part[len] = 0;
        text(x, y, scale, c, "%s", part);
        at += len;
        while (line[at] == ' ')
            at++;
        y += 10 * scale;
    }
}
static void button(WcApp *a, float x, float y, float w, const char *label, int action, int p, int q,
                   int r) {
    int i = a->button_count;
    if (i >= 64)
        return;
    WcButton *b = &a->buttons[a->button_count++];
    *b = (WcButton){x, y, w, 25, action, p, q, r, ""};
    snprintf(b->label, sizeof(b->label), "%s", label);
    int mx, my;
    float sx, sy;
    SDL_GetMouseState(&sx, &sy);
    mx = (int)(sx / a->width * 960);
    my = (int)(sy / a->height * 540);
    int hot =
        (mx >= x && mx <= x + w && my >= y && my <= y + 25) || (a->panel && a->menu_focus == i);
    rect(x, y, w, 25, hot ? (WcColor){.22, .29, .28} : (WcColor){.105, .16, .17}, 1);
    rect(x, y, 2, 25, hot ? gold : muted, 1);
    text(x + 10, y + 8, 1.1, hot ? cream : muted, "%s", label);
}
static void cost_line(WcApp *a, float x, float y, const int *cost) {
    char line[250] = "SHARED: ";
    for (int i = 0; i < ITEM_COUNT; i++)
        if (cost[i]) {
            char tmp[70];
            snprintf(tmp, sizeof(tmp), "%s %d/%d  ", a->game.defs.items[i].name,
                     wc_available(&a->game.s, i), cost[i]);
            strncat(line, tmp, sizeof(line) - strlen(line) - 1);
        }
    wrapped(x, y, 480, 1, muted, line);
}
static void panel(WcApp *a) {
    WcState *s = &a->game.s;
    float x = 214, y = 100, w = 532;
    rect(x - 7, y - 7, w + 14, 355, ink, .97);
    rect(x - 7, y - 7, w + 14, 2, gold, 1);
    const char *titles[] = {"",
                            "YOUR HOME, ON THE HORIZON",
                            "A MOMENT AT HOME",
                            "BACKPACK & EQUIPMENT",
                            "THE WORKSHOP",
                            "HOME STORAGE",
                            "THE GARDEN",
                            "A QUIET REST",
                            "MEADOW MERCHANT",
                            "KNOWLEDGE & TECHNIQUES",
                            "GROW THE HEARTHSTRIDER",
                            "REPLACE A CABINET",
                            "THE SILENT FOUNDRY",
                            "BEGIN A NEW JOURNEY?"};
    text(x + 14, y + 13, 1.8, cream, "%s", titles[a->panel]);
    text(x + 14, y + 38, 1, muted, "SIMULATION PAUSED  /  FIRST-PERSON VIEW RETAINED");
    float bx = x + 14, by = y + 68;
    char label[180];
    switch (a->panel) {
    case PANEL_TITLE:
        wrapped(bx, by, 460, 1.4, cream,
                "One wanderer. Six legs. A home that grows with every journey.");
        text(bx, by + 46, 1.1, muted, "PLANT A GARDEN. FOLLOW A RIDGE. BRING IT HOME.");
        button(a, bx, by + 85, 464, "Begin wandering", UI_RESUME, 0, 0, 0);
        button(a, bx, by + 118, 464, "Restore saved journey", UI_LOAD, 0, 0, 0);
        button(a, bx, by + 151, 464, "New journey / random seed", UI_NEW, 0, 0, 0);
        text(bx, by + 199, 1, gold, "WASD MOVE  /  MOUSE OR ARROWS LOOK  /  E INTERACT");
        break;
    case PANEL_PAUSE:
        button(a, bx, by, 464, "Return to the journey", UI_RESUME, 0, 0, 0);
        button(a, bx, by + 33, 224, "Save journey [F5]", UI_SAVE, 0, 0, 0);
        button(a, bx + 240, by + 33, 224, "Restore [F9]", UI_LOAD, 0, 0, 0);
        button(a, bx, by + 66, 464, "Techniques & knowledge [K]", UI_PANEL, PANEL_TECH, 0, 0);
        button(a, bx, by + 99, 464, "New journey...", UI_NEW, 0, 0, 0);
        button(a, bx, by + 132, 464, "Backpack & equipment [I]", UI_PANEL, PANEL_INVENTORY, 0, 0);
#ifndef __EMSCRIPTEN__
        button(a, bx, by + 165, 464, "Quit", UI_QUIT, 0, 0, 0);
#endif
        break;
    case PANEL_INVENTORY:
        text(bx, by, 1.1, gold, "BACKPACK %d/%d UNITS  /  %d COINS",
             wc_inventory_used(&s->backpack), s->backpack.capacity, s->coins);
        for (int i = 0; i < ITEM_COUNT; i++) {
            float xx = bx + (i / 8) * 240, yy = by + 23 + (i % 8) * 17;
            text(xx, yy, 1, cream, "%-16s %2d", a->game.defs.items[i].name, s->backpack.n[i]);
        }
        text(bx, by + 170, 1, muted, "GEAR: %s  /  HOE: REUSABLE",
             s->spear ? "TEMPERED SPEAR" : "WALKER'S STAFF");
        text(bx, by + 188, 1, muted, "DAMAGE %d  /  ARMOR %.0f%%  /  POWER %d",
             s->spear ? a->game.defs.spear.damage : a->game.defs.staff.damage,
             (s->tempered ? a->game.defs.spear.armor : a->game.defs.staff.armor) * 100,
             s->spear ? a->game.defs.spear.power : a->game.defs.staff.power);
        button(a, bx, by + 210, 464, "Eat trail stew (active benefits never stack)", UI_COMMAND,
               CMD_EAT, 0, 0);
        break;
    case PANEL_WORKSHOP:
        for (int i = 0; i < a->game.defs.recipe_count; i++) {
            WcRecipe *r = &a->game.defs.recipes[i];
            snprintf(label, sizeof(label), "%s", r->name);
            button(a, bx, by + i * 26, 222, label, UI_RECIPE, i, 0, 0);
            char supplies[160] = "";
            for (int j = 0; j < ITEM_COUNT; j++)
                if (r->cost[j]) {
                    char quantity[40];
                    snprintf(quantity, sizeof(quantity), "%.5s %d/%d  ", a->game.defs.items[j].name,
                             wc_available(s, j), r->cost[j]);
                    strncat(supplies, quantity, sizeof(supplies) - strlen(supplies) - 1);
                }
            wrapped(bx + 234, by + i * 26 + 7, 232, .9, muted, supplies);
        }
        snprintf(label, sizeof(label), "Hearth: planks %d/2, stone %d/2%s", wc_available(s, PLANK),
                 wc_available(s, STONE), s->hearth ? " / installed" : "");
        button(a, bx, by + 162, 464, label, UI_COMMAND, CMD_HEARTH, 0, 0);
        snprintf(label, sizeof(label), "Temper: alloy %d/2, planks %d/4, stone %d/4",
                 wc_available(s, ALLOY), wc_available(s, PLANK), wc_available(s, STONE));
        button(a, bx, by + 190, 464, label, UI_COMMAND, CMD_TEMPER, 0, 0);
        snprintf(label, sizeof(label), "Spear: alloy %d/2, planks %d/2, twine %d/1",
                 wc_available(s, ALLOY), wc_available(s, PLANK), wc_available(s, TWINE));
        button(a, bx, by + 218, 464, label, UI_COMMAND, CMD_SPEAR, 0, 0);
        text(bx, by - 14, .9, muted, "SHARED: BACKPACK > PANTRY > CABINETS. OUTPUT TO BACKPACK.");
        break;
    case PANEL_STORAGE: {
        WcInventory *inv = a->storage ? &s->rooms[a->storage - 1].cabinet : &s->pantry;
        text(bx, by, 1.1, gold, "%s %d/%d  /  BACKPACK %d/%d", a->storage ? "CABINET" : "PANTRY",
             wc_inventory_used(inv), inv->capacity, wc_inventory_used(&s->backpack),
             s->backpack.capacity);
        for (int i = 0; i < 8; i++) {
            int item = (a->selection / 8) * 8 + i;
            if (item >= ITEM_COUNT)
                break;
            snprintf(label, sizeof(label), "%s  BAG %d / STORE %d", a->game.defs.items[item].name,
                     s->backpack.n[item], inv->n[item]);
            text(bx, by + 28 + i * 23, 1, cream, "%s", label);
            button(a, bx + 294, by + 20 + i * 23, 80, "Put 1", UI_DEPOSIT, item, 1, 0);
            button(a, bx + 384, by + 20 + i * 23, 80, "Take 1", UI_WITHDRAW, item, 1, 0);
        }
        button(a, bx, by + 211, 222, "Items 1-8", UI_PANEL, PANEL_STORAGE, 0, 0);
        button(a, bx + 240, by + 211, 224, "Items 9-16", UI_PANEL, PANEL_STORAGE, 8, 0);
        break;
    }
    case PANEL_GARDEN: {
        WcCrop c = s->crops[a->garden];
        text(bx, by, 1.2, gold, "PLANTER %d  /  REUSABLE HOE", a->garden + 1);
        if (c.kind < 0) {
            text(bx, by + 26, 1.2, cream, "READY FOR SOMETHING GREEN");
            button(a, bx, by + 60, 464, "Plant sunroot (45s) / 1 sunroot seed", UI_PLANT, a->garden,
                   0, 0);
            button(a, bx, by + 94, 464, "Plant meadow mint (60s) / 1 mint seed", UI_PLANT,
                   a->garden, 1, 0);
        } else {
            double remain = fmax(0, c.duration - (s->time - c.planted));
            text(bx, by + 30, 1.4, cream, "%s  /  %s", c.kind ? "MEADOW MINT" : "SUNROOT",
                 remain > 0 ? "GROWING" : "READY TO HARVEST");
            text(bx, by + 53, 1.1, muted, "%.1f ACTIVE SECONDS REMAINING", remain);
            button(a, bx, by + 85, 464, "Harvest: 3 crops + 1 replacement seed", UI_HARVEST,
                   a->garden, 0, 0);
            button(a, bx, by + 119, 464, "Tend once: compost or learned Careful Tending", UI_TEND,
                   a->garden, 0, 0);
        }
        text(bx, by + 174, 1, muted, "SUN SEEDS %d/1  /  MINT SEEDS %d/1  /  COMPOST %d/1",
             wc_available(s, SUN_SEED), wc_available(s, MINT_SEED), wc_available(s, COMPOST));
        wrapped(bx, by + 198, 460, 1, muted,
                "Shared home supplies. Mature crops wait indefinitely. Menus and closed time do "
                "not advance growth.");
        break;
    }
    case PANEL_BED:
        wrapped(bx, by, 460, 1.3, cream,
                "The road can wait. Rest restores health and stamina instantly, without moving "
                "time forward.");
        button(a, bx, by + 66, 464, "Rest / 120 rested effort seconds", UI_COMMAND, CMD_REST, 0, 0);
        button(a, bx, by + 104, 464, "Save journey", UI_SAVE, 0, 0, 0);
        break;
    case PANEL_TRADE: {
        int item = a->trade_item;
        WcItem d = a->game.defs.items[item];
        text(bx, by, 1.4, gold, "%s  /  %d COINS", d.name, s->coins);
        text(bx, by + 26, 1.1, cream, "CARRIED %d  /  BUY %d COINS  /  SELL %d COINS",
             s->backpack.n[item], d.buy, d.sell);
        int stock = 20;
        for (int i = 0; i < s->mutation_count; i++)
            if (s->mutations[i].id == UINT64_C(0xf0000000) + item)
                stock = s->time >= s->mutations[i].ready && s->mutations[i].ready > 0
                            ? 20
                            : s->mutations[i].stock;
        text(bx, by + 46, 1, muted, "MERCHANT STOCK %d/1  /  REPLENISHES IN 120 ACTIVE S", stock);
        button(a, bx, by + 76, 222, "Previous item", UI_PANEL, PANEL_TRADE, -1, 0);
        button(a, bx + 240, by + 76, 224, "Next item", UI_PANEL, PANEL_TRADE, 1, 0);
        snprintf(label, sizeof(label), "Buy 1 / %d coins", d.buy);
        button(a, bx, by + 111, 222, label, UI_BUY, item, 1, 0);
        snprintf(label, sizeof(label), "Sell 1 / receive %d coins", d.sell);
        button(a, bx + 240, by + 111, 224, label, UI_SELL, item, 1, 0);
        wrapped(bx, by + 158, 460, 1.2, cream,
                "Gardening pays for peaceful expansion. Foundry alloy trade includes workshop "
                "knowledge. Bought supplies are protected on defeat.");
        break;
    }
    case PANEL_TECH:
        text(bx, by, 1.2, gold, "KNOWLEDGE %d XP  /  %d UNSPENT POINTS", s->xp, s->points);
        text(bx, by + 25, 1, muted, "ONE POINT PER 12 XP / THREE POINTS TOTAL");
        button(a, bx, by + 51, 464,
               s->techniques & 1 ? "Second Wind [V] - learned"
                                 : "Learn Second Wind / 15 rested effort -> 40 stamina",
               UI_COMMAND, CMD_LEARN, 1, 0);
        button(a, bx, by + 85, 464,
               s->techniques & 2 ? "Power Strike [C] - learned"
                                 : "Learn Power Strike / double damage, 25 stamina",
               UI_COMMAND, CMD_LEARN, 2, 0);
        button(a, bx, by + 119, 464,
               s->techniques & 4 ? "Careful Tending - learned"
                                 : "Learn Careful Tending / halve growth once",
               UI_COMMAND, CMD_LEARN, 4, 0);
        break;
    case PANEL_BUILD: {
        char why[120];
        int valid = wc_room_valid(s, a->build_parent, a->build_direction, a->build_kind,
                                  a->build_rotation, why, sizeof(why));
        text(bx, by, 1.2, gold, "%s  /  %s  /  ROTATION %d",
             a->build_kind == ROOM_HALL ? "HALL 12 X 16M" : "ANNEX 6 X 6M",
             a->build_direction == 0 ? "UP" : "SIDE", a->build_rotation * 90);
        text(bx, by + 25, 1, valid ? green : gold, "%s", why);
        cost_line(a, bx, by + 46, a->game.defs.room_cost);
        button(a, bx, by + 83, 222, "Next parent room", UI_BUILD_PARENT, 0, 0, 0);
        button(a, bx + 240, by + 83, 224, "Next socket", UI_BUILD_DIR, 0, 0, 0);
        button(a, bx, by + 116, 222, "Hall / annex", UI_BUILD_KIND, 0, 0, 0);
        button(a, bx + 240, by + 116, 224, "Rotate [R]", UI_ROTATE, 0, 0, 0);
        button(a, bx, by + 158, 464, "Confirm furnished room", UI_BUILD, 0, 0, 0);
        text(bx, by + 202, 1, muted, "PARENT %d  /  ROOMS %d/12  /  SANCTUARY EXTENDS",
             a->build_parent + 1, s->room_count);
        break;
    }
    case PANEL_FURNITURE:
        text(bx, by, 1.2, gold, "ROOM %d  /  %s CABINET", a->storage + 1,
             s->rooms[a->storage].furniture ? "COMPACT" : "STANDARD");
        text(bx, by + 28, 1, cream, "PRESERVES ALL CONTENTS; RETURNS THE OLD CABINET.");
        text(bx, by + 49, 1, muted, "PACKED FIRST. ELSE SHARED: PLANKS %d/2, STONE %d/1",
             wc_available(s, PLANK), wc_available(s, STONE));
        button(a, bx, by + 83, 464, "Replace with standard cabinet / 40 units", UI_REPLACE,
               FURN_CABINET, 0, 0);
        button(a, bx, by + 119, 464, "Replace with compact cabinet / 12 units", UI_REPLACE,
               FURN_COMPACT, 0, 0);
        break;
    case PANEL_NOTES:
        wrapped(bx, by, 460, 1.3, cream,
                "THE SILENT FOUNDRY\nA wedge can silence the pressure plate. Follow the gold "
                "maintenance marks along the east wall to the brake. The brake stops steam and the "
                "forge guardian; both routes unlock the same reserve.");
        wrapped(bx, by + 121, 460, 1.2, gold,
                "Route board: return home now to secure your haul, or continue with field "
                "resources at risk. The guardian exposes its core after the warning slam.");
        break;
    case PANEL_NEW:
        wrapped(bx, by, 460, 1.3, cream,
                "Start fresh with a new seed? Your existing save stays intact until you explicitly "
                "save the new journey.");
        button(a, bx, by + 73, 464, "Confirm new journey / random seed", UI_CONFIRM_NEW, 0, 0, 0);
        button(a, bx, by + 111, 464, "Keep this journey", UI_RESUME, 0, 0, 0);
        break;
    default:
        break;
    }
    if (a->panel != PANEL_TITLE && a->panel != PANEL_NEW)
        button(a, x + w - 100, y + 311, 84, "Close [Esc]", UI_RESUME, 0, 0, 0);
}
static const char *prompt(int kind) {
    const char *p[] = {"",
                       "E  OCCUPY HELM",
                       "E  OPEN / CLOSE OAK GATE",
                       "E  RING ENTRANCE BELL",
                       "E  REST OR SAVE",
                       "E  PANTRY / SHARED SUPPLIES",
                       "E  WORKSHOP & RECIPES",
                       "E  PLANTER / GARDEN",
                       "E  ROOM CABINET",
                       "E  HEARTH / COOK",
                       "E  GATHER",
                       "E  TRADE",
                       "E  READ FOUNDRY NOTES",
                       "E  INSTALL MECHANISM WEDGE",
                       "E  ENGAGE MAINTENANCE BRAKE",
                       "E  SALVAGE",
                       "E  CLAIM RESERVE / X ATTACK GUARDIAN",
                       "X  ATTACK / Q REPEL",
                       "E  RECORD DISCOVERY",
                       "E  RECOVER CACHE",
                       "B  CONSTRUCT"};
    return kind >= 0 && kind <= HIT_SOCKET ? p[kind] : "";
}
static void hud(WcApp *a) {
    WcState *s = &a->game.s;
    a->button_count = 0;
    float matrix[16];
    identity(matrix);
    matrix[0] = 2.0 / 960;
    matrix[5] = -2.0 / 540;
    matrix[12] = -1;
    matrix[13] = 1;
    rect(18, 18, 273, 54, ink, .87);
    rect(18, 18, 3, 54, gold, 1);
    text(32, 29, 1.5, cream, "WANDERING CASTLE");
    text(32, 51, 1, muted, "HEARTHSTRIDER  /  %s", s->indoor ? "SANCTUARY" : "FIELD JOURNAL");
    double hours = fmod(9 + s->time / 30, 24);
    text(694, 25, 1.15, cream, "%02d:%02d  /  SEED %u", (int)hours, (int)(hours * 60) % 60,
         s->seed);
    button(a, 789, 48, 153, a->help ? "Hide help [F1]" : "Show help [F1]", UI_HELP, 0, 0, 0);
    rect(18, 420, 273, 102, ink, .9);
    text(30, 432, 1.05, cream, "HEALTH %3.0f / 100", s->health);
    rect(30, 446, 241, 4, (WcColor){.22, .19, .17}, 1);
    rect(30, 446, 241 * s->health / 100, 4, (WcColor){.75, .40, .32}, 1);
    text(30, 460, 1.05, cream, "STAMINA %3.0f / 100", s->stamina);
    rect(30, 474, 241, 4, (WcColor){.17, .24, .21}, 1);
    rect(30, 474, 241 * s->stamina / 100, 4, green, 1);
    text(30, 488, 1, muted, "RESTED %.0fS / MEAL %.0fS", s->rested, s->meal);
    text(30, 505, 1, gold, "PACK %d/%d  /  %d COINS", wc_inventory_used(&s->backpack),
         s->backpack.capacity, s->coins);
    rect(688, 420, 254, 102, ink, .9);
    text(700, 432, 1.05, gold, "%s  %.1f M/S",
         s->helm         ? "AT THE HELM"
         : s->speed == 0 ? "PARKED"
                         : "WANDERING",
         fabs(s->speed));
    text(700, 451, 1, cream, "HEADING %03.0f  /  %s", fmod(s->castle_yaw * 180 / WC_PI + 360, 360),
         fabs(s->speed) > 5 ? "TURBO" : "ORDINARY");
    text(700, 469, 1, muted, "KNOWLEDGE %d  /  POINTS %d", s->xp, s->points);
    text(700, 487, 1, muted, "DAMAGE %d  /  ARMOR %.0f%%",
         s->spear ? a->game.defs.spear.damage : a->game.defs.staff.damage,
         (s->tempered ? a->game.defs.spear.armor : a->game.defs.staff.armor) * 100);
    int biome = wc_biome(s->seed, s->indoor ? s->castle : s->player);
    text(700, 505, 1, biome == 2 ? gold : muted, "%s / POWER %d %s",
         biome == 0   ? "MEADOW"
         : biome == 1 ? "WOODLAND"
                      : "HIGHLANDS",
         biome == 2   ? 8
         : biome == 1 ? 3
                      : 1,
         !s->indoor && biome > 0 && !s->spear ? "CAUTION" : "");
    if (a->help && !a->panel) {
        rect(694, 102, 248, 286, ink, .91);
        text(708, 117, 1.3, gold, "A HOME THAT WANDERS");
        const char *lines[] = {"WASD  WALK / PILOT",
                               "MOUSE OR ARROWS  LOOK",
                               "SPACE  JUMP",
                               "SHIFT  SPRINT / HELM TURBO",
                               "E  INTERACT / RELEASE HELM",
                               "I  BACKPACK   K  TECHNIQUES",
                               "B  ROOM PREVIEW   R  ROTATE",
                               "F  REPLACE CABINET",
                               "X / CLICK  OUTDOOR MELEE",
                               "Q  REPEL WITHOUT REWARDS",
                               "V  SECOND WIND  C  POWER",
                               "F5 SAVE  F9 RESTORE",
                               "ESC  PAUSE  F1 / TAB HELP"};
        for (int i = 0; i < 13; i++)
            text(708, 143 + i * 15, 1, cream, "%s", lines[i]);
        text(708, 353, 1, gold, "NO FUEL. NO HUNGER. NO RUSH.");
        text(708, 370, 1, muted, "HOME IS ALWAYS SAFE.");
    }
    if (!a->panel) {
        rect(476, 269, 8, 1, cream, .9);
        rect(479.5, 265.5, 1, 8, cream, .9);
        if (a->hit.kind) {
            const char *p = prompt(a->hit.kind);
            float ww = strlen(p) * 6 + 24;
            rect(480 - ww * .5, 327, ww, 23, ink, .9);
            text(492 - ww * .5, 335, 1, cream, "%s", p);
        }
        if (s->helm) {
            text(327, 386, 1.2, gold, "WASD STEER  /  SHIFT 5X  /  E RELEASE");
        }
    }
    if (a->panel)
        panel(a);
    rect(308, 462, 364, 60, ink, .92);
    wrapped(320, 473, 340, 1, muted, a->game.message);
    if (a->attack_flash) {
        rect(450, 310, 60, 2, gold, 1);
        a->attack_flash--;
    }
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    flush(matrix, 1, (WcVec){0});
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
static GLuint shader(GLenum kind, const char *source) {
    GLuint s = glCreateShader(kind);
    glShaderSource(s, 1, &source, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        fprintf(stderr, "Shader: %s\n", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}
static void construction_preview(WcApp *a, float *matrix, WcVec eye) {
    if (a->panel != PANEL_BUILD || a->build_parent < 0 || a->build_parent >= a->game.s.room_count)
        return;
    WcRoom room = a->game.s.rooms[a->build_parent];
    int dir = a->build_direction;
    room.tier += (dir == 0);
    room.gx += (dir == 2) - (dir == 1);
    room.gz += (dir == 4) - (dir == 3);
    WcVec center = wc_room_center(&room),
          size = wc_room_dimensions(a->build_kind, a->build_rotation);
    int valid =
        wc_room_valid(&a->game.s, a->build_parent, dir, a->build_kind, a->build_rotation, NULL, 0);
    local_mode = 1;
    WcVec corners[8];
    for (int i = 0; i < 8; i++)
        corners[i] =
            (WcVec){center.x + ((i & 1) ? 1 : -1) * size.x * .5, center.y + ((i & 2) ? 4 : 0),
                    center.z + ((i & 4) ? 1 : -1) * size.z * .5};
    for (int i = 0; i < 8; i++)
        for (int bit = 1; bit <= 4; bit *= 2)
            if (!(i & bit))
                beam(corners[i], corners[i | bit], .06, valid ? 15 : 13);
    local_mode = 0;
    glDisable(GL_DEPTH_TEST);
    flush(matrix, 0, eye);
    glEnable(GL_DEPTH_TEST);
}
static GLuint shader_file(WcApp *a, GLenum kind, const char *name, const char *version) {
    char path[600];
    snprintf(path, sizeof(path), "%s/shaders/%s", a->asset_dir, name);
    size_t length = 0;
    char *file = SDL_LoadFile(path, &length);
    if (!file)
        return 0;
    char *source = SDL_malloc(length + strlen(version) + 1);
    if (!source) {
        SDL_free(file);
        return 0;
    }
    strcpy(source, version);
    memcpy(source + strlen(version), file, length);
    source[strlen(version) + length] = 0;
    GLuint result = shader(kind, source);
    SDL_free(source);
    SDL_free(file);
    return result;
}
bool wc_render_init(WcApp *a) {
#ifndef __EMSCRIPTEN__
#undef GL_API
#define LOAD(name)                                                                                 \
    do {                                                                                           \
        void *p = (void *)SDL_GL_GetProcAddress("gl" #name);                                       \
        memcpy(&q##name, &p, sizeof(p));                                                           \
        if (!q##name)                                                                              \
            return false;                                                                          \
    } while (0)
    LOAD(CreateShader);
    LOAD(ShaderSource);
    LOAD(CompileShader);
    LOAD(GetShaderiv);
    LOAD(GetShaderInfoLog);
    LOAD(CreateProgram);
    LOAD(AttachShader);
    LOAD(LinkProgram);
    LOAD(GetProgramiv);
    LOAD(UseProgram);
    LOAD(GetUniformLocation);
    LOAD(UniformMatrix4fv);
    LOAD(Uniform1i);
    LOAD(Uniform1f);
    LOAD(Uniform3f);
    LOAD(GenVertexArrays);
    LOAD(BindVertexArray);
    LOAD(GenBuffers);
    LOAD(BindBuffer);
    LOAD(BufferData);
    LOAD(EnableVertexAttribArray);
    LOAD(VertexAttribPointer);
    LOAD(DeleteShader);
    LOAD(DeleteProgram);
    LOAD(DeleteBuffers);
    LOAD(DeleteVertexArrays);
#endif
#ifdef __EMSCRIPTEN__
    const char *version = "#version 300 es\nprecision highp float;\n";
#else
    const char *version = "#version 330 core\n";
#endif
    GLuint v = shader_file(a, GL_VERTEX_SHADER, "retro.vert", version);
    GLuint f = shader_file(a, GL_FRAGMENT_SHADER, "retro.frag", version);
    if (!v || !f)
        return false;
    program = glCreateProgram();
    glAttachShader(program, v);
    glAttachShader(program, f);
    glLinkProgram(program);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
        return false;
    umatrix = glGetUniformLocation(program, "matrix");
    ufog = glGetUniformLocation(program, "fog");
    udaylight = glGetUniformLocation(program, "daylight");
    ucamera = glGetUniformLocation(program, "camera");
    uui = glGetUniformLocation(program, "ui");
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(7 * sizeof(float)));
    char path[600];
    snprintf(path, sizeof(path), "%s/textures/atlas.rgba", a->asset_dir);
    FILE *file = fopen(path, "rb");
    if (!file)
        return false;
    unsigned char rgb[32 * 16 * 4];
    if (fread(rgb, 1, sizeof(rgb), file) != sizeof(rgb)) {
        fclose(file);
        return false;
    }
    fclose(file);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgb);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    snprintf(path, sizeof(path), "%s/fonts/hearth.fnt", a->asset_dir);
    file = fopen(path, "r");
    if (!file)
        return false;
    char line[100];
    while (fgets(line, sizeof(line), file)) {
        int ch = 0;
        char bits[36];
        if (sscanf(line, "%d %35s", &ch, bits) == 2 && ch > 0 && ch < 128)
            for (int k = 0; k < 35; k++)
                font[ch][k] = bits[k] == '1';
    }
    fclose(file);
    glEnable(GL_DEPTH_TEST);
    return true;
}
void wc_render(WcApp *a) {
    art = &a->art;
    count = 0;
    local_mode = 0;
    origin = a->game.s.indoor ? a->game.s.castle : a->game.s.player;
    origin.y = 0;
    double phase = fmod(9 + a->game.s.time / 30, 24);
    day = (float)fmax(.08, fmin(1, sin((phase - 6) * WC_PI / 12) * 1.4));
    sky = (WcColor){.09f + day * .48f, .13f + day * .58f, .20f + day * .57f};
    glClearColor(sky.r, sky.g, sky.b, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!a->xr)
        glViewport(0, 0, a->width, a->height);
    terrain(a);
    scenery(a);
    castle(a);
    float matrix[16];
    WcVec eye;
    camera_matrix(a, matrix, &eye);
    flush(matrix, 0, eye);
    construction_preview(a, matrix, eye);
    hud(a);
}
void wc_render_destroy(void) {
    glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
}
bool wc_capture(WcApp *a, const char *path) {
    SDL_Surface *s = SDL_CreateSurface(a->width, a->height, SDL_PIXELFORMAT_RGBA32);
    if (!s)
        return false;
    glReadPixels(0, 0, a->width, a->height, GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);
    SDL_FlipSurface(s, SDL_FLIP_VERTICAL);
    bool ok = SDL_SaveBMP(s, path);
    SDL_DestroySurface(s);
    return ok;
}
