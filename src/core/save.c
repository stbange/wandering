#include "wc.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static bool inv_valid(const WcInventory *v, int capacity) {
    if (v->capacity != capacity)
        return false;
    for (int i = 0; i < WC_ITEMS; i++)
        if (v->n[i] < 0 || v->n[i] > capacity || (i >= ITEM_COUNT && v->n[i]))
            return false;
    return wc_inventory_used(v) <= capacity;
}
static bool pos_valid(WcPos p) {
    return p.cx > INT64_C(-1000000000000) && p.cx < INT64_C(1000000000000) &&
           p.cz > INT64_C(-1000000000000) && p.cz < INT64_C(1000000000000) && isfinite(p.x) &&
           p.x >= 0 && p.x < 128 && isfinite(p.z) && p.z >= 0 && p.z < 128 && isfinite(p.y) &&
           fabs(p.y) < 1e6;
}
bool wc_validate(const WcState *s) {
    if (s->generation != WC_GEN_VERSION || s->content != WC_CONTENT_VERSION ||
        !pos_valid(s->castle) || !pos_valid(s->player))
        return false;
    const double d[] = {s->time,   s->castle_yaw, s->yaw,      s->pitch,   s->vertical,
                        s->speed,  s->attack_cd,  s->recovery, s->health,  s->stamina,
                        s->rested, s->meal,       s->local.x,  s->local.y, s->local.z};
    for (size_t i = 0; i < sizeof(d) / sizeof(*d); i++)
        if (!isfinite(d[i]) || fabs(d[i]) > 1e12)
            return false;
    if (s->time < 0 || s->health < 0 || s->health > 100 || s->stamina < 0 || s->stamina > 100 ||
        s->rested < 0 || s->rested > 120 || s->meal < 0 || s->meal > 180 || fabs(s->pitch) > 1.48 ||
        fabs(s->speed) > 25.01)
        return false;
    if (s->room_count < 1 || s->room_count > WC_ROOMS || s->mutation_count < 0 ||
        s->mutation_count > WC_MUTATIONS || s->coins < 0 || s->coins > 100000000 || s->xp < 0 ||
        s->xp > 100000000 || s->points < 0 || s->points > 3 || s->awarded < 0 || s->awarded > 3 ||
        (s->techniques & ~7))
        return false;
    const int flags[] = {s->indoor,
                         s->helm,
                         s->grounded,
                         s->exhausted,
                         s->shift_released,
                         s->gate,
                         s->satchel,
                         s->hearth,
                         s->tempered,
                         s->spear,
                         s->hoe,
                         s->wedge_recipe,
                         s->foundry_notes,
                         s->foundry_trap,
                         s->foundry_brake,
                         s->foundry_reserve,
                         s->storage_failed,
                         s->stairs.deployed,
                         s->stairs.turned};
    for (size_t i = 0; i < sizeof(flags) / sizeof(*flags); i++)
        if (flags[i] < 0 || flags[i] > 1)
            return false;
    if ((s->helm && !s->indoor) || s->gate != s->stairs.deployed)
        return false;
    if (!inv_valid(&s->backpack, s->satchel ? 40 : 24) || !inv_valid(&s->pantry, 80))
        return false;
    for (int i = 0; i < WC_ITEMS; i++)
        if (s->eligible[i] < 0 || s->eligible[i] > s->backpack.n[i])
            return false;
    for (int i = 0; i < s->room_count; i++) {
        WcRoom r = s->rooms[i];
        if (r.id != i + 1 || r.parent > i - 1 || r.parent < -1 ||
            (i == 0 ? r.parent != -1 : r.parent < 0) || r.tier < 0 || r.tier > 3 ||
            r.rotation < 0 || r.rotation > 3 || r.kind < 0 || r.kind > 2 || r.furniture < 0 ||
            r.furniture > 1 || r.gx < -r.tier || r.gx > r.tier || r.gz < -r.tier || r.gz > r.tier)
            return false;
        if (i == 0 && (r.kind != ROOM_HOME || r.tier || r.gx || r.gz))
            return false;
        WcVec center = wc_room_center(&r), dimensions = wc_room_dimensions(r.kind, r.rotation);
        if (i) {
            WcRoom parent = s->rooms[r.parent];
            int up = r.tier - parent.tier, dx = r.gx - parent.gx, dz = r.gz - parent.gz;
            if (r.kind == ROOM_HOME || up < 0 || up > 1 ||
                (up ? (dx || dz) : (abs(dx) + abs(dz) != 1)) || !r.tier)
                return false;
            for (int j = 0; j < i; j++) {
                WcRoom other = s->rooms[j];
                WcVec p = wc_room_center(&other),
                      d = wc_room_dimensions(other.kind, other.rotation);
                if (other.tier == r.tier &&
                    fabs(center.x - p.x) < (dimensions.x + d.x) * .5 - .01 &&
                    fabs(center.z - p.z) < (dimensions.z + d.z) * .5 - .01)
                    return false;
            }
        }
        if (!inv_valid(&r.cabinet, r.furniture == FURN_CABINET ? 40 : 12))
            return false;
    }
    for (int i = 0; i < WC_CROPS; i++) {
        WcCrop c = s->crops[i];
        if (c.kind < -1 || c.kind > 1 || c.room < 0 || c.room >= s->room_count || c.treated < 0 ||
            c.treated > 1 || !isfinite(c.planted) || !isfinite(c.duration))
            return false;
        if (c.kind >= 0 &&
            (c.duration <= 0 || c.duration > 3600 || c.planted < 0 || c.planted > s->time))
            return false;
    }
    for (int i = 0; i < s->mutation_count; i++) {
        WcMutation m = s->mutations[i];
        if (!m.id || !isfinite(m.ready) || m.ready < 0 || m.ready > 1e15 || m.stock < -100 ||
            m.stock > 100000)
            return false;
        for (int j = 0; j < i; j++)
            if (s->mutations[j].id == m.id)
                return false;
    }
    for (int i = 0; i < WC_CACHES; i++) {
        WcCache c = s->caches[i];
        if (c.active < 0 || c.active > 1 || (c.active && !pos_valid(c.pos)))
            return false;
        int total = 0;
        for (int j = 0; j < WC_ITEMS; j++) {
            if (c.items[j] < 0 || c.items[j] > 40)
                return false;
            total += c.items[j];
        }
        if (total > 40)
            return false;
    }
    const double stairs[] = {s->stairs.length1,   s->stairs.length2,   s->stairs.width,
                             s->stairs.bend.x,    s->stairs.bend.y,    s->stairs.bend.z,
                             s->stairs.landing.x, s->stairs.landing.y, s->stairs.landing.z};
    for (size_t i = 0; i < sizeof(stairs) / sizeof(*stairs); i++)
        if (!isfinite(stairs[i]) || fabs(stairs[i]) > 128)
            return false;
    if (s->stairs.deployed && (s->stairs.length1 <= 0 ||
                               s->stairs.length1 + s->stairs.length2 > 48 || s->stairs.width < 1))
        return false;
    return true;
}
typedef struct {
    unsigned char *p;
    size_t size, at;
    int read, ok;
} Stream;
static uint64_t number(Stream *b, uint64_t value) {
    if (b->at + 8 > b->size) {
        b->ok = 0;
        return 0;
    }
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        if (b->read)
            result |= (uint64_t)b->p[b->at + i] << (i * 8);
        else
            b->p[b->at + i] = (unsigned char)(value >> (i * 8));
    }
    b->at += 8;
    return b->read ? result : value;
}
static void integer(Stream *b, int *v) {
    uint64_t n = number(b, (uint64_t)(int64_t)*v);
    int64_t signed_n;
    memcpy(&signed_n, &n, 8);
    if (signed_n < INT_MIN || signed_n > INT_MAX)
        b->ok = 0;
    if (b->read)
        *v = (int)signed_n;
}
static void u32(Stream *b, uint32_t *v) {
    uint64_t n = number(b, *v);
    if (n > UINT32_MAX)
        b->ok = 0;
    if (b->read)
        *v = (uint32_t)n;
}
static void real(Stream *b, double *v) {
    uint64_t n;
    memcpy(&n, v, 8);
    n = number(b, n);
    if (b->read)
        memcpy(v, &n, 8);
}
static void flt(Stream *b, float *v) {
    double n = *v;
    real(b, &n);
    if (b->read)
        *v = (float)n;
}
static void pos(Stream *b, WcPos *p) {
    uint64_t n = number(b, (uint64_t)p->cx);
    if (b->read)
        memcpy(&p->cx, &n, 8);
    n = number(b, (uint64_t)p->cz);
    if (b->read)
        memcpy(&p->cz, &n, 8);
    real(b, &p->x);
    real(b, &p->y);
    real(b, &p->z);
}
static void vec(Stream *b, WcVec *v) {
    real(b, &v->x);
    real(b, &v->y);
    real(b, &v->z);
}
static void inventory(Stream *b, WcInventory *v) {
    integer(b, &v->capacity);
    for (int i = 0; i < WC_ITEMS; i++)
        integer(b, &v->n[i]);
}
#define INTS(M)                                                                                    \
    M(indoor)                                                                                      \
    M(helm)                                                                                        \
    M(grounded)                                                                                    \
    M(exhausted)                                                                                   \
    M(shift_released)                                                                              \
    M(gate) M(coins) M(xp) M(points) M(awarded) M(techniques) M(satchel) M(hearth) M(tempered)     \
        M(spear) M(hoe) M(wedge_recipe) M(foundry_notes) M(foundry_trap) M(foundry_brake)          \
            M(foundry_reserve) M(storage_failed) M(room_count) M(mutation_count) M(cache_serial)
#define REALS(M) M(time) M(castle_yaw) M(yaw) M(pitch) M(vertical) M(speed) M(attack_cd) M(recovery)
static void state(Stream *b, WcState *s) {
    u32(b, &s->seed);
    u32(b, &s->generation);
    u32(b, &s->content);
    pos(b, &s->castle);
    pos(b, &s->player);
    vec(b, &s->local);
#define I(n) integer(b, &s->n);
    INTS(I)
#undef I
#define D(n) real(b, &s->n);
    REALS(D)
#undef D
    flt(b, &s->health);
    flt(b, &s->stamina);
    flt(b, &s->rested);
    flt(b, &s->meal);
    u32(b, &s->achievements);
    inventory(b, &s->backpack);
    inventory(b, &s->pantry);
    for (int i = 0; i < WC_ROOMS; i++) {
        WcRoom *r = &s->rooms[i];
        integer(b, &r->id);
        integer(b, &r->parent);
        integer(b, &r->kind);
        integer(b, &r->tier);
        integer(b, &r->gx);
        integer(b, &r->gz);
        integer(b, &r->rotation);
        integer(b, &r->furniture);
        inventory(b, &r->cabinet);
    }
    for (int i = 0; i < WC_CROPS; i++) {
        WcCrop *c = &s->crops[i];
        integer(b, &c->room);
        integer(b, &c->kind);
        integer(b, &c->treated);
        real(b, &c->planted);
        real(b, &c->duration);
    }
    for (int i = 0; i < WC_MUTATIONS; i++) {
        WcMutation *m = &s->mutations[i];
        uint64_t n = number(b, m->id);
        if (b->read)
            m->id = n;
        real(b, &m->ready);
        integer(b, &m->flags);
        integer(b, &m->stock);
    }
    for (int i = 0; i < WC_CACHES; i++) {
        WcCache *c = &s->caches[i];
        uint64_t n = number(b, c->id);
        if (b->read)
            c->id = n;
        pos(b, &c->pos);
        integer(b, &c->active);
        for (int j = 0; j < WC_ITEMS; j++)
            integer(b, &c->items[j]);
    }
    for (int i = 0; i < WC_ITEMS; i++)
        integer(b, &s->eligible[i]);
    integer(b, &s->stairs.deployed);
    integer(b, &s->stairs.turned);
    real(b, &s->stairs.length1);
    real(b, &s->stairs.length2);
    real(b, &s->stairs.width);
    vec(b, &s->stairs.bend);
    vec(b, &s->stairs.landing);
}
static uint32_t checksum(const unsigned char *b, size_t n) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++)
        h = (h ^ b[i]) * 16777619u;
    return h;
}
static void put32(unsigned char *p, uint32_t n) {
    for (int i = 0; i < 4; i++)
        p[i] = (unsigned char)(n >> (i * 8));
}
static uint32_t get32(const unsigned char *p) {
    uint32_t n = 0;
    for (int i = 0; i < 4; i++)
        n |= (uint32_t)p[i] << (i * 8);
    return n;
}
size_t wc_encode(const WcState *s, unsigned char *out, size_t cap) {
    if (cap < 16 || !wc_validate(s))
        return 0;
    Stream b = {out, cap, 16, 0, 1};
    WcState copy = *s;
    state(&b, &copy);
    if (!b.ok)
        return 0;
    memcpy(out, "WCS1", 4);
    put32(out + 4, WC_VERSION);
    put32(out + 8, (uint32_t)b.at);
    put32(out + 12, checksum(out + 16, b.at - 16));
    return b.at;
}
bool wc_decode(WcState *out, const unsigned char *bytes, size_t length) {
    if (length < 16 || length > WC_SAVE_MAX || memcmp(bytes, "WCS1", 4) ||
        get32(bytes + 4) != WC_VERSION || get32(bytes + 8) != length ||
        get32(bytes + 12) != checksum(bytes + 16, length - 16))
        return false;
    WcState next = {0};
    Stream b = {(unsigned char *)bytes, length, 16, 1, 1};
    state(&b, &next);
    if (!b.ok || b.at != length || !wc_validate(&next))
        return false;
    /* Support is serialized and validated before any simulation can resume. */
    next.helm = 0;
    next.storage_failed = 0;
    next.speed = 0;
    next.vertical = 0;
    *out = next;
    return true;
}
