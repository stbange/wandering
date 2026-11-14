#include "wc.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint64_t wc_hash(uint64_t x) {
    x += UINT64_C(0x9e3779b97f4a7c15);
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    return x ^ (x >> 31);
}
static void normalize(WcPos *p) {
    double x = floor(p->x / WC_CHUNK), z = floor(p->z / WC_CHUNK);
    p->cx += (int64_t)x;
    p->cz += (int64_t)z;
    p->x -= x * WC_CHUNK;
    p->z -= z * WC_CHUNK;
}
WcPos wc_pos(double x, double y, double z) {
    WcPos p = {0, 0, x, y, z};
    normalize(&p);
    return p;
}
void wc_pos_add(WcPos *p, double x, double y, double z) {
    p->x += x;
    p->y += y;
    p->z += z;
    normalize(p);
}
WcVec wc_relative(WcPos a, WcPos o) {
    return (WcVec){(double)(a.cx - o.cx) * WC_CHUNK + a.x - o.x, a.y - o.y,
                   (double)(a.cz - o.cz) * WC_CHUNK + a.z - o.z};
}
WcPos wc_local_world(const WcState *s, WcVec v) {
    WcPos p = s->castle;
    double c = cos(s->castle_yaw), q = sin(s->castle_yaw);
    wc_pos_add(&p, c * v.x - q * v.z, v.y, q * v.x + c * v.z);
    return p;
}
WcVec wc_world_local(const WcState *s, WcPos p) {
    WcVec v = wc_relative(p, s->castle);
    double c = cos(s->castle_yaw), q = sin(s->castle_yaw);
    return (WcVec){c * v.x + q * v.z, v.y, -q * v.x + c * v.z};
}
static double lattice(uint32_t seed, int64_t x, int64_t z) {
    return (double)(wc_hash((uint64_t)x ^ wc_hash((uint64_t)z) ^ seed) >> 11) / 9007199254740992.0;
}
static double noise(uint32_t seed, WcPos p, int scale) {
    /* Chunk size is divisible by every octave, preserving distant precision. */
    int64_t x = p.cx * (128 / scale) + (int64_t)floor(p.x / scale);
    int64_t z = p.cz * (128 / scale) + (int64_t)floor(p.z / scale);
    double u = fmod(p.x, scale) / scale, v = fmod(p.z, scale) / scale;
    u = u * u * (3 - 2 * u);
    v = v * v * (3 - 2 * v);
    double a = lattice(seed, x, z), b = lattice(seed, x + 1, z);
    double c = lattice(seed, x, z + 1), d = lattice(seed, x + 1, z + 1);
    return (a + (b - a) * u) * (1 - v) + (c + (d - c) * u) * v;
}
int wc_biome(uint32_t seed, WcPos p) {
    if ((p.cx == -1 || p.cx == 0) && (p.cz == -1 || p.cz == 0)) {
        double x = (double)p.cx * 128 + p.x, z = (double)p.cz * 128 + p.z;
        if (fabs(x) <= 64 && fabs(z) <= 64)
            return 0;
    }
    double n = noise(seed + 91, p, 128);
    return n < 0.38 ? 1 : n > 0.65 ? 2 : 0;
}
static double raw_height(uint32_t seed, WcPos p) {
    double base = (noise(seed, p, 128) - .5) * 22 + (noise(seed + 1, p, 64) - .5) * 8 +
                  (noise(seed + 2, p, 32) - .5) * 2;
    double mountains = fmax(0, (noise(seed + 91, p, 128) - .58) * 150);
    base += mountains;
    if (p.cx >= -1 && p.cx <= 0 && p.cz >= -1 && p.cz <= 0) {
        double x = (double)p.cx * 128 + p.x, z = (double)p.cz * 128 + p.z;
        double t = fmax(fabs(x), fabs(z));
        t = fmax(0, fmin(1, (t - 64) / 24));
        base *= t * t * (3 - 2 * t);
    }
    return base;
}
static int destination(uint32_t seed, int64_t cx, int64_t cz, WcFeature *f) {
    uint64_t h = wc_hash((uint64_t)cx ^ wc_hash((uint64_t)cz) ^ seed);
    for (int i = 0; i < 7; i++)
        h = wc_hash(h);
    if (h % 4)
        return 0;
    WcPos p = {cx, cz, 44 + (double)(h % 40), 0, 44 + (double)((h >> 16) % 40)};
    WcVec v = wc_relative(p, wc_pos(0, 0, 0));
    if (fabs(v.x) < 100 && fabs(v.z) < 100)
        return 0;
    *f = (WcFeature){.p = p, .id = h, .type = FEATURE_KEEP + (int)((h >> 8) % 3)};
    return 1;
}
double wc_height(uint32_t seed, WcPos p) {
    double height = raw_height(seed, p);
    for (int dz = -1; dz <= 1; dz++)
        for (int dx = -1; dx <= 1; dx++) {
            WcFeature site;
            if (!destination(seed, p.cx + dx, p.cz + dz, &site))
                continue;
            WcVec offset = wc_relative(p, site.p);
            double radius = site.type == FEATURE_KEEP ? 34 : site.type == FEATURE_MINE ? 32 : 24;
            double distance = fmax(fabs(offset.x), fabs(offset.z));
            if (distance < radius + 10) {
                double t = fmax(0, fmin(1, (distance - radius) / 10));
                t = t * t * (3 - 2 * t);
                height = raw_height(seed, site.p) * (1 - t) + height * t;
            }
        }
    return height;
}
const char *wc_destination_name(uint64_t id, int type) {
    static const char *names[3][4] = {
        {"Briarwatch Keep", "The Amber Archive", "Old Lantern Hold", "Westwind Court"},
        {"Copperbell Adit", "The Quiet Workings", "Ashen Vein Mine", "Mossdeep Stores"},
        {"Hollow Hearths", "The Lost Orchard", "Little Copperwick", "Stillwater Hamlet"}};
    int family = type - FEATURE_KEEP;
    if (family < 0 || family > 2)
        return "Wilderness discovery";
    return names[family][id % 4];
}
WcVec wc_room_dimensions(int kind, int rotation) {
    double w = kind == ROOM_ANNEX ? 6 : 12, d = kind == ROOM_ANNEX ? 6 : 16;
    return (rotation & 1) ? (WcVec){d, 4, w} : (WcVec){w, 4, d};
}
static void feature(WcScene *sc, const WcState *s, WcPos p, int type, uint64_t id, int power) {
    if (sc->count >= 96)
        return;
    p.y = wc_height(s->seed, p);
    sc->f[sc->count++] = (WcFeature){p, id, type, power, 0};
}
void wc_scene(const WcState *s, WcScene *sc) {
    memset(sc, 0, sizeof(*sc));
    WcPos center = s->indoor ? s->castle : s->player;
    WcPos trader = wc_pos(17, 0, 20), foundry = wc_pos(-30, 0, 28);
    feature(sc, s, trader, FEATURE_TRADER, 1, 0);
    feature(sc, s, foundry, FEATURE_FOUNDRY, 2, 3);
    for (int i = 0; i < 12; i++) {
        double a = i * 2.399963;
        WcPos p = wc_pos(cos(a) * (20 + i * 1.2), 0, sin(a) * (20 + i * 1.2));
        feature(sc, s, p, FEATURE_TIMBER + i % 4, (uint64_t)(10 + i), 0);
    }
    int enemies = 0;
    for (int dz = -1; dz <= 1; dz++)
        for (int dx = -1; dx <= 1; dx++) {
            int64_t cx = center.cx + dx, cz = center.cz + dz;
            sc->chunks++;
            uint64_t h = wc_hash((uint64_t)cx ^ wc_hash((uint64_t)cz) ^ s->seed);
            for (int i = 0; i < 7; i++) {
                h = wc_hash(h);
                WcPos p = {cx, cz, 8 + (double)(h % 112), 0, 8 + (double)((h >> 16) % 112)};
                WcVec o = wc_relative(p, wc_pos(0, 0, 0));
                if (fabs(o.x) < 64 && fabs(o.z) < 64)
                    continue;
                int type = i < 3 ? FEATURE_TREE : i < 5 ? FEATURE_TIMBER + i % 4 : FEATURE_CLUE;
                int biome = wc_biome(s->seed, p);
                WcFeature site;
                int has_site = destination(s->seed, cx, cz, &site);
                if (i == 6 && has_site) {
                    p = site.p;
                    type = site.type;
                }
                if (i < 6) {
                    int reserved = 0;
                    for (int rz = -1; rz <= 1; rz++)
                        for (int rx = -1; rx <= 1; rx++) {
                            WcFeature nearby;
                            if (destination(s->seed, cx + rx, cz + rz, &nearby)) {
                                WcVec v = wc_relative(p, nearby.p);
                                double extent = nearby.type == FEATURE_KEEP   ? 38
                                                : nearby.type == FEATURE_MINE ? 35
                                                                              : 28;
                                if (fabs(v.x) < extent && fabs(v.z) < extent)
                                    reserved = 1;
                            }
                        }
                    if (reserved)
                        continue;
                }
                if (i == 5 && enemies < WC_ENEMIES && biome != 0) {
                    type = FEATURE_PROWLER;
                    enemies++;
                }
                feature(sc, s, p, type, h, biome == 2 ? 8 : 3);
            }
        }
}
WcVec wc_room_center(const WcRoom *r) {
    double x = r->gx ? (r->gx < 0 ? -1 : 1) * (9 + (abs(r->gx) - 1) * 6) : 0;
    double z = r->gz ? (r->gz < 0 ? -1 : 1) * (11 + (abs(r->gz) - 1) * 6) : 0;
    return (WcVec){x, r->tier * 4.0, z};
}
static double procedural(uint32_t seed, WcPos p, void *context) {
    (void)context;
    return wc_height(seed, p);
}
double wc_clearance(const WcState *s, WcPos at, double angle) {
    return wc_clearance_sample(s, at, angle, procedural, NULL);
}
double wc_clearance_sample(const WcState *s, WcPos at, double angle, WcHeightFn height,
                           void *context) {
    double h = -1e20, c = cos(angle), q = sin(angle);
    for (int i = 0; i < s->room_count; i++) {
        const WcRoom *r = &s->rooms[i];
        WcVec p = wc_room_center(r);
        WcVec dimensions = wc_room_dimensions(r->kind, r->rotation);
        double w = dimensions.x * .5, d = dimensions.z * .5;
        for (int x = -1; x <= 1; x++)
            for (int z = -1; z <= 1; z++) {
                double lx = p.x + x * w, lz = p.z + z * d;
                WcPos v = at;
                wc_pos_add(&v, c * lx - q * lz, 0, q * lx + c * lz);
                h = fmax(h, height(s->seed, v, context) + 5 - p.y);
            }
    }
    return h;
}
bool wc_castle_blocked(const WcState *s, WcPos at, double angle) {
    WcScene sc;
    WcState sample = *s;
    sample.castle = at;
    sample.indoor = 1;
    wc_scene(&sample, &sc);
    double c = cos(angle), q = sin(angle);
    for (int f = 0; f < sc.count; f++) {
        WcFeature *o = &sc.f[f];
        if (o->type != FEATURE_TREE && o->type != FEATURE_FOUNDRY && o->type != FEATURE_KEEP &&
            o->type != FEATURE_MINE && o->type != FEATURE_HAMLET)
            continue;
        WcVec v = wc_relative(o->p, at);
        double x = c * v.x + q * v.z, z = -q * v.x + c * v.z;
        double radius = o->type == FEATURE_TREE ? .6 : o->type == FEATURE_FOUNDRY ? 9 : 18;
        double height = o->type == FEATURE_TREE ? 10 : 14;
        for (int r = 0; r < s->room_count; r++) {
            WcVec p = wc_room_center(&s->rooms[r]);
            WcVec dimensions = wc_room_dimensions(s->rooms[r].kind, s->rooms[r].rotation);
            double w = dimensions.x * .5, d = dimensions.z * .5;
            if (fabs(x - p.x) < w + radius && fabs(z - p.z) < d + radius &&
                o->p.y + height > at.y + p.y - 3)
                return true;
        }
    }
    return false;
}
bool wc_room_valid(const WcState *s, int parent, int dir, int kind, int rotation, char *why,
                   size_t size) {
    const char *error = NULL;
    if (parent < 0 || parent >= s->room_count || dir < 0 || dir > 4 || kind < ROOM_HALL ||
        kind > ROOM_ANNEX || rotation < 0 || rotation > 3)
        error = "Unsupported room or socket";
    else if (s->room_count >= WC_ROOMS)
        error = "Twelve room safety limit";
    else {
        WcRoom p = s->rooms[parent];
        int tier = p.tier + (dir == 0), gx = p.gx, gz = p.gz;
        WcVec dimensions = wc_room_dimensions(kind, rotation);
        if (dir == 1)
            gx--;
        if (dir == 2)
            gx++;
        if (dir == 3)
            gz--;
        if (dir == 4)
            gz++;
        if ((tier == 0) || (s->room_count == 1 && (dir != 0 || kind != ROOM_HALL)))
            error = "Build upward first";
        else if (abs(gx) > tier || abs(gz) > tier || tier * 4 + 4 > 18)
            error = "Outside supported room envelope";
        else {
            double w = dimensions.x * .5, d = dimensions.z * .5;
            WcRoom proposed = {
                .gx = gx, .gz = gz, .tier = tier, .kind = kind, .rotation = rotation};
            WcVec center = wc_room_center(&proposed), parent_center = wc_room_center(&p);
            double minx = center.x - w, maxx = center.x + w, minz = center.z - d,
                   maxz = center.z + d;
            for (int i = 0; i < s->room_count; i++) {
                WcRoom r = s->rooms[i];
                WcVec existing = wc_room_dimensions(r.kind, r.rotation);
                double rw = existing.x * .5, rd = existing.z * .5;
                WcVec other = wc_room_center(&r);
                minx = fmin(minx, other.x - rw);
                maxx = fmax(maxx, other.x + rw);
                minz = fmin(minz, other.z - rd);
                maxz = fmax(maxz, other.z + rd);
                if (r.tier == tier && fabs(center.x - other.x) < w + rw - .01 &&
                    fabs(center.z - other.z) < d + rd - .01)
                    error = "Room overlaps an existing room";
            }
            if (maxx - minx > 38 || maxz - minz > 24)
                error = "Outside supported room envelope";
            if (dir == 0 && s->indoor && fabs(s->local.x - parent_center.x) < 1.3 &&
                s->local.z - parent_center.z > -3 && s->local.z - parent_center.z < 3)
                error = "Step clear of the stair opening";
            if (s->speed != 0)
                error = "Park before construction";
            WcState candidate = *s;
            candidate.rooms[s->room_count] =
                (WcRoom){.kind = kind, .tier = tier, .gx = gx, .gz = gz, .rotation = rotation};
            candidate.room_count++;
            if (wc_clearance(&candidate, s->castle, s->castle_yaw) > s->castle.y + .01 ||
                wc_castle_blocked(&candidate, s->castle, s->castle_yaw))
                error = "Room conflicts with terrain or solid scenery";
        }
    }
    if (why && size)
        snprintf(why, size, "%s", error ? error : "Valid furnished room");
    return error == NULL;
}
static int stairs_clear(const WcState *s, WcVec a, WcVec b, WcHeightFn height, void *context) {
    WcScene scene;
    wc_scene(s, &scene);
    for (int i = 0; i <= 32; i++) {
        double t = i / 32.0;
        WcVec v = {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
        WcPos p = wc_local_world(s, v);
        double ground = height(s->seed, p, context);
        if (ground > p.y + .15)
            return 0;
        for (int f = 0; f < scene.count; f++) {
            int k = scene.f[f].type;
            if (k != FEATURE_TREE && k != FEATURE_FOUNDRY)
                continue;
            WcVec d = wc_relative(p, scene.f[f].p);
            double rad = k == FEATURE_TREE ? 1.5 : 9;
            if (hypot(d.x, d.z) < rad && p.y < scene.f[f].p.y + 12)
                return 0;
        }
    }
    return 1;
}
bool wc_solve_stairs(const WcState *s, WcStairs *out) {
    return wc_solve_stairs_sample(s, out, procedural, NULL);
}
bool wc_solve_stairs_sample(const WcState *s, WcStairs *out, WcHeightFn height, void *context) {
    for (int route = 0; route < 3; route++)
        for (int n = 4; n <= 48; n++) {
            WcVec a = {0, 0, 8}, b = {0, 0, 10}, end = {0, 0, 8 + n};
            if (route) {
                end.x = (route == 1 ? 1 : -1) * (n - 2);
                end.z = 10;
            }
            WcPos p = wc_local_world(s, end);
            double h = height(s->seed, p, context);
            end.y = h - s->castle.y;
            double length = n;
            if (fabs(end.y) / length > .8)
                continue;
            b.y = route ? end.y * 2 / length : 0;
            WcVec lateral = end;
            lateral.x += route ? 0 : .8;
            lateral.z += route ? .8 : 0;
            WcPos side = wc_local_world(s, lateral);
            double h1 = height(s->seed, side, context);
            lateral.x -= route ? 0 : 1.6;
            lateral.z -= route ? 1.6 : 0;
            side = wc_local_world(s, lateral);
            double h2 = height(s->seed, side, context);
            if (fabs(h1 - h2) > .6)
                continue;
            if (!stairs_clear(s, a, route ? b : end, height, context) ||
                (route && !stairs_clear(s, b, end, height, context)))
                continue;
            *out = (WcStairs){1, route != 0, route ? 2 : length, route ? length - 2 : 0, 2, b, end};
            return true;
        }
    return false;
}
static double segment_floor(WcVec p, WcVec a, WcVec b, int *valid) {
    double dx = b.x - a.x, dz = b.z - a.z, len = dx * dx + dz * dz;
    double t = ((p.x - a.x) * dx + (p.z - a.z) * dz) / len;
    double cross = fabs((p.x - a.x) * dz - (p.z - a.z) * dx) / sqrt(len);
    if (t >= -.025 && t <= 1.025 && cross < 1.2) {
        *valid = 1;
        return a.y + fmax(0, fmin(1, t)) * (b.y - a.y);
    }
    return -1e9;
}
double wc_support(const WcState *s, WcVec p, int *valid) {
    double floor = -1e9;
    *valid = 0;
    for (int i = 0; i < s->room_count; i++) {
        WcRoom r = s->rooms[i];
        WcVec c = wc_room_center(&r);
        WcVec dimensions = wc_room_dimensions(r.kind, r.rotation);
        double w = dimensions.x * .5, d = dimensions.z * .5;
        if (fabs(p.x - c.x) < w - .18 && fabs(p.z - c.z) < d - .18 && p.y >= c.y - .6) {
            floor = fmax(floor, c.y);
            *valid = 1;
        }
        if (r.parent >= 0 && r.tier > s->rooms[r.parent].tier && fabs(p.x - c.x) < 1.1 &&
            p.z > c.z - 3 && p.z < c.z + 3) {
            double y = c.y - (p.z - (c.z - 3)) * 4 / 6;
            if (p.y >= y - .65) {
                floor = y;
                *valid = 1;
            }
        }
        if (r.parent >= 0 && r.tier == s->rooms[r.parent].tier) {
            WcVec parent = wc_room_center(&s->rooms[r.parent]);
            if (fabs(p.z - c.z) < 1.15 && p.x >= fmin(c.x, parent.x) &&
                p.x <= fmax(c.x, parent.x) && p.y >= c.y - .6) {
                floor = fmax(floor, c.y);
                *valid = 1;
            }
            if (fabs(p.x - c.x) < 1.15 && p.z >= fmin(c.z, parent.z) &&
                p.z <= fmax(c.z, parent.z) && p.y >= c.y - .6) {
                floor = fmax(floor, c.y);
                *valid = 1;
            }
        }
    }
    if (s->gate && s->stairs.deployed) {
        int v = 0;
        if (fabs(p.x) < 1.2 && p.z > 7.7 && p.z <= 8 && p.y >= -.6) {
            floor = fmax(floor, 0);
            *valid = 1;
        }
        WcVec a = {0, 0, 8}, b = s->stairs.turned ? s->stairs.bend : s->stairs.landing;
        double y = segment_floor(p, a, b, &v);
        if (v) {
            floor = fmax(floor, y);
            *valid = 1;
        }
        if (s->stairs.turned) {
            v = 0;
            y = segment_floor(p, b, s->stairs.landing, &v);
            if (v) {
                floor = fmax(floor, y);
                *valid = 1;
            }
        }
    }
    return floor;
}
void wc_look(WcState *s, double dx, double dy) {
    if (!isfinite(dx) || !isfinite(dy))
        return;
    s->yaw = remainder(s->yaw + dx, WC_PI * 2);
    s->pitch = fmax(-1.48, fmin(1.48, s->pitch + dy));
}
static void box(WcBox *b, int *n, int cap, double x, double y, double z, double w, double h,
                double d, int m) {
    if (*n < cap)
        b[(*n)++] = (WcBox){{x, y, z}, {w, h, d}, m};
}
int wc_site_boxes(int type, WcBox *b, int cap) {
    int n = 0;
    if (type == FEATURE_FOUNDRY || type == FEATURE_MINE) {
        double d = type == FEATURE_MINE ? 27 : 12;
        box(b, &n, cap, -8, 2, 0, .5, 2, d, 1);
        box(b, &n, cap, 8, 2, 0, .5, 2, d, 1);
        box(b, &n, cap, 0, 2, -d, 8, 2, .5, 1);
        box(b, &n, cap, -5, 2, d, 3, 2, .5, 1);
        box(b, &n, cap, 5, 2, d, 3, 2, .5, 1);
        for (int z = -8; z < d; z += 6) {
            box(b, &n, cap, -6.8, 1.8, z, .2, 1.8, .2, 2);
            box(b, &n, cap, 6.8, 1.8, z, .2, 1.8, .2, 2);
            box(b, &n, cap, 0, 3.6, z, 7, .2, .2, 2);
        }
        box(b, &n, cap, 0, .5, -10, 2, .5, 1, 3);
        box(b, &n, cap, -5, .5, 2, 1.3, .5, .6, 2);
    } else if (type == FEATURE_KEEP) {
        for (int side = -1; side <= 1; side += 2) {
            box(b, &n, cap, side * 26, 3, 0, 1, 3, 28, 1);
            box(b, &n, cap, side * 16, 3, 27, 10, 3, 1, 1);
            box(b, &n, cap, side * 16, 3, -27, 10, 3, 1, 1);
            box(b, &n, cap, side * 16, 1, -16, 5, 1, 1, 2);
            box(b, &n, cap, side * 15, .6, 10, 2, .6, 1, 2);
        }
        box(b, &n, cap, 0, 3, -27, 6, 3, 1, 1);
        box(b, &n, cap, 14, 3.5, -17, 6, .3, 7, 2);
        for (int i = 0; i < 14; i++)
            box(b, &n, cap, 14, .125 * i, 4 - i, 2, .125 * i + .1, .5, 1);
    } else if (type == FEATURE_HAMLET) {
        for (int i = 0; i < 4; i++) {
            double x = (i % 2 ? 1 : -1) * 10, z = (i / 2 ? 1 : -1) * 11;
            box(b, &n, cap, x - 4, 2, z, .3, 2, 4, 2);
            box(b, &n, cap, x + 4, 2, z, .3, 2, 4, 2);
            box(b, &n, cap, x, 2, z - 4, 4, 2, .3, 1);
            box(b, &n, cap, x - 2.7, 2, z + 4, 1.3, 2, .3, 1);
            box(b, &n, cap, x + 2.7, 2, z + 4, 1.3, 2, .3, 1);
            box(b, &n, cap, x - 2, .55, z - 1, 1, .55, 1.7, 2);
            box(b, &n, cap, x + 2, .7, z - 2, .8, .7, .6, 2);
        }
    }
    return n;
}
bool wc_outdoor_blocked(const WcState *s, WcPos p) {
    WcScene sc;
    wc_scene(s, &sc);
    for (int f = 0; f < sc.count; f++) {
        WcFeature o = sc.f[f];
        WcVec v = wc_relative(p, o.p);
        if (o.type == FEATURE_TREE && hypot(v.x, v.z) < .8 && v.y < 9)
            return true;
        if (o.type == FEATURE_FOUNDRY || (o.type >= FEATURE_KEEP && o.type <= FEATURE_HAMLET)) {
            WcBox boxes[64];
            int count = wc_site_boxes(o.type, boxes, 64);
            for (int i = 0; i < count; i++) {
                WcBox b = boxes[i];
                if (fabs(v.x - b.center.x) < b.half.x + .25 &&
                    fabs(v.z - b.center.z) < b.half.z + .25 && v.y < b.center.y + b.half.y - .25 &&
                    v.y + 1.8 > b.center.y - b.half.y)
                    return true;
            }
        }
    }
    return false;
}
static void ray_point(WcHit *hit, WcPos eye, WcVec dir, WcPos p, double radius, int kind, int index,
                      uint64_t id) {
    WcVec d = wc_relative(p, eye);
    double t = d.x * dir.x + d.y * dir.y + d.z * dir.z;
    double dist = d.x * d.x + d.y * d.y + d.z * d.z - t * t;
    if (t > 0 && t < hit->distance && dist < radius * radius) {
        *hit = (WcHit){t, kind, index, id, p};
    }
}
WcHit wc_raycast(const WcState *s, const WcScene *scene, double range) {
    WcHit h = {range, HIT_NONE, 0, 0, {0}};
    WcPos eye = s->indoor ? wc_local_world(s, s->local) : s->player;
    eye.y += 1.62;
    WcVec dir = {sin(s->yaw) * cos(s->pitch), sin(s->pitch), -cos(s->yaw) * cos(s->pitch)};
    if (s->indoor) {
        const WcVec points[] = {{0, 1.55, -5.8}, {0, 1, 7.8}, {4, .8, 4}, {-4, 1, 4},
                                {-4, 1, -1},     {4, .8, -3}, {4, .8, 0}, {-3, 1, -5}};
        const int kinds[] = {HIT_HELM,     HIT_GATE,    HIT_BED,     HIT_PANTRY,
                             HIT_WORKSHOP, HIT_PLANTER, HIT_PLANTER, HIT_HEARTH};
        for (int i = 0; i < 8; i++)
            ray_point(&h, eye, dir, wc_local_world(s, points[i]), i == 1 ? 1.4 : 1.0, kinds[i],
                      i == 6 ? 1 : 0, 0);
        for (int i = 1; i < s->room_count; i++) {
            WcVec v = wc_room_center(&s->rooms[i]);
            v.x -= 2;
            v.y += 1;
            ray_point(&h, eye, dir, wc_local_world(s, v), 1, HIT_CABINET, i, 0);
            v = wc_room_center(&s->rooms[i]);
            v.x += 2;
            v.y += .8;
            v.z += 2.5;
            ray_point(&h, eye, dir, wc_local_world(s, v), 1, HIT_PLANTER, i * 2 + 1, 0);
        }
    } else {
        WcVec bell = s->stairs.landing;
        bell.x += 1.4;
        bell.y += 1;
        ray_point(&h, eye, dir, wc_local_world(s, bell), .8, HIT_BELL, 0, 0);
        for (int f = 0; f < scene->count; f++) {
            WcFeature o = scene->f[f];
            int kind = HIT_NONE;
            WcPos p = o.p;
            p.y += 1;
            if (o.type >= FEATURE_TIMBER && o.type <= FEATURE_SEED)
                kind = HIT_RESOURCE;
            if (o.type == FEATURE_TRADER)
                kind = HIT_TRADER;
            if (o.type == FEATURE_PROWLER)
                kind = HIT_PROWLER;
            if (o.type == FEATURE_CLUE || (o.type >= FEATURE_KEEP && o.type <= FEATURE_HAMLET))
                kind = HIT_CLUE;
            if (kind)
                ray_point(&h, eye, dir, p, 1, kind, f, o.id);
            if (o.type == FEATURE_FOUNDRY) {
                const WcVec pts[] = {{-5, 1, 5}, {0, .2, 3}, {6, 1, -5}, {-5, 1, 2}, {0, 1, -8}};
                const int kinds[] = {HIT_NOTES, HIT_PLATE, HIT_BRAKE, HIT_SALVAGE, HIT_RESERVE};
                for (int i = 0; i < 5; i++) {
                    p = o.p;
                    wc_pos_add(&p, pts[i].x, pts[i].y, pts[i].z);
                    ray_point(&h, eye, dir, p, 1, kinds[i], 0, 2);
                }
            }
        }
        for (int i = 0; i < WC_CACHES; i++)
            if (s->caches[i].active) {
                WcPos p = s->caches[i].pos;
                p.y += .5;
                ray_point(&h, eye, dir, p, .9, HIT_CACHE, i, s->caches[i].id);
            }
        /* Occlusion uses the same solid objects as locomotion. */
        if (h.kind)
            for (double t = .2; t < h.distance - .3; t += .15) {
                WcPos p = eye;
                wc_pos_add(&p, dir.x * t, dir.y * t, dir.z * t);
                if (wc_outdoor_blocked(s, p)) {
                    h.kind = HIT_NONE;
                    break;
                }
            }
    }
    return h;
}
