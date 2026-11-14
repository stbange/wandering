#include "wc.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void wc_init(WcGame *g, uint32_t seed) {
    WcDefs d = g->defs;
    memset(g, 0, sizeof(*g));
    g->defs = d;
    WcState *s = &g->s;
    s->seed = seed;
    s->generation = WC_GEN_VERSION;
    s->content = WC_CONTENT_VERSION;
    s->castle = wc_pos(0, 5, 0);
    s->local = (WcVec){0, 0, 4};
    s->player = wc_local_world(s, s->local);
    s->indoor = 1;
    s->grounded = 1;
    s->shift_released = 1;
    s->health = s->stamina = 100;
    s->coins = 12;
    s->hoe = 1;
    s->room_count = 1;
    s->backpack.capacity = 24;
    s->pantry.capacity = 80;
    s->rooms[0] = (WcRoom){.id = 1, .parent = -1, .kind = ROOM_HOME, .cabinet = {.capacity = 40}};
    s->pantry.n[TIMBER] = 6;
    s->pantry.n[STONE] = 6;
    s->pantry.n[FIBER] = 6;
    s->backpack.n[SUN_SEED] = 2;
    s->backpack.n[MINT_SEED] = 2;
    for (int i = 0; i < WC_CROPS; i++)
        s->crops[i].kind = -1;
    WcStairs stairs;
    if (wc_solve_stairs(s, &stairs))
        s->stairs = stairs;
    s->stairs.deployed = 0;
    snprintf(g->message, sizeof(g->message), "Welcome home. E at the helm to wander; F1 for help.");
}
static void exert(WcState *s, double amount, double dt) {
    s->stamina = fmax(0, s->stamina - amount * (s->meal > 0 ? .7 : 1));
    s->recovery = .8;
    s->rested = fmax(0, s->rested - dt);
    s->meal = fmax(0, s->meal - dt);
}
static void sanctuary(WcState *s) {
    s->indoor = 1;
    s->vertical = 0;
    s->grounded = 1;
    memset(s->eligible, 0, sizeof(s->eligible));
}
void wc_defeat(WcGame *g) {
    WcState *s = &g->s;
    int any = 0;
    for (int i = 0; i < WC_ITEMS; i++)
        any += s->eligible[i];
    if (any) {
        int slot = -1;
        for (int i = 0; i < WC_CACHES; i++)
            if (!s->caches[i].active) {
                slot = i;
                break;
            }
        /* Exhausted persistent capacity preserves the haul, never deletes it. */
        if (slot >= 0) {
            WcCache *c = &s->caches[slot];
            c->active = 1;
            c->id = ++s->cache_serial;
            c->pos = s->player;
            for (int i = 0; i < WC_ITEMS; i++) {
                c->items[i] = s->eligible[i];
                s->backpack.n[i] -= s->eligible[i];
            }
        }
    }
    s->local = (WcVec){3.5, 0, 4};
    s->player = wc_local_world(s, s->local);
    sanctuary(s);
    s->helm = 0;
    s->speed = 0;
    s->health = s->stamina = 100;
    s->rested = s->meal = 0;
    snprintf(g->message, sizeof(g->message),
             "Awoke safely at home. Your field haul waits in a recovery cache.");
}
static void danger(WcGame *g, double dt) {
    WcState *s = &g->s;
    if (s->indoor)
        return;
    dt *= 1 - (s->tempered ? g->defs.spear.armor : g->defs.staff.armor);
    WcVec f = wc_relative(s->player, wc_pos(-30, 0, 28));
    if (fabs(f.x) < 1.3 && fabs(f.z - 3) < 1.1 && !s->foundry_trap && f.y < .6) {
        double phase = fmod(s->time, 3);
        if (phase > 1.2 && phase < 1.5)
            s->health -= dt * 80;
        snprintf(g->message, sizeof(g->message),
                 "Pressure plate: red flash warns of spikes. Jump or step clear. E: wedge.");
    }
    if (fabs(f.x) < 2 && f.z < 0 && f.z > -7 && !s->foundry_brake && f.y < .7 &&
        fmod(s->time, 4) > 2.5)
        s->health -= dt * 18;
    if (hypot(f.x, f.z + 8) < 3 && !s->foundry_brake && fmod(s->time, 3) > 1.4 &&
        fmod(s->time, 3) < 1.7 && f.y < 1)
        s->health -= dt * 80;
    WcScene scene;
    wc_scene(s, &scene);
    for (int i = 0; i < scene.count; i++)
        if (scene.f[i].type == FEATURE_PROWLER) {
            WcFeature o = scene.f[i];
            int disabled = 0;
            for (int m = 0; m < s->mutation_count; m++)
                if (s->mutations[m].id == o.id && s->mutations[m].ready > s->time)
                    disabled = 1;
            WcVec d = wc_relative(s->player, o.p);
            double distance = hypot(d.x, d.z);
            double phase = fmod(s->time + (o.id % 31) * .1, 3);
            if (!disabled && distance < 2 && phase > 1.4 && phase < 1.7 && fabs(d.y) < 2)
                s->health -= dt * 45;
        }
    if (s->health <= 0)
        wc_defeat(g);
}
void wc_pilot(WcGame *g, WcInput in, double dt, WcHeightFn height, void *context) {
    WcState *s = &g->s;
    double yaw = remainder(s->castle_yaw - in.side * g->defs.turn * dt, WC_PI * 2);
    double speed = in.forward * g->defs.speed * (in.sprint ? g->defs.turbo : 1);
    WcPos next = s->castle;
    wc_pos_add(&next, sin(yaw) * speed * dt, 0, -cos(yaw) * speed * dt);
    next.y =
        height ? wc_clearance_sample(s, next, yaw, height, context) : wc_clearance(s, next, yaw);
    if (!s->storage_failed && !wc_castle_blocked(s, next, yaw)) {
        s->castle = next;
        s->castle_yaw = yaw;
        s->speed = speed;
        if (speed || in.side) {
            s->gate = 0;
            s->stairs.deployed = 0;
        }
    } else {
        s->speed = 0;
        snprintf(g->message, sizeof(g->message),
                 "Travel blocked by solid scenery or unavailable storage.");
    }
    s->player = wc_local_world(s, s->local);
}
void wc_step(WcGame *g, WcInput in, double dt) {
    if (g->paused || g->s.storage_failed || dt <= 0 || !isfinite(dt))
        return;
    if (dt > .05)
        dt = .05;
    WcState *s = &g->s;
    s->time += dt;
    s->attack_cd = fmax(0, s->attack_cd - dt);
    s->recovery = fmax(0, s->recovery - dt);
    if (s->mutation_count >= WC_MUTATIONS) {
        s->storage_failed = 1;
        snprintf(g->message, sizeof(g->message),
                 "Persistent journal capacity reached. Exploration paused to preserve progress.");
        return;
    }
    if (!in.sprint)
        s->shift_released = 1;
    if (s->exhausted && s->shift_released && s->stamina >= 20)
        s->exhausted = 0;
    if (s->helm && s->indoor) {
        wc_pilot(g, in, dt, NULL, NULL);
        return;
    }
    s->speed = 0;
    double f = in.forward, side = in.side, len = hypot(f, side);
    if (len > 1) {
        f /= len;
        side /= len;
    }
    int moving = fabs(f) + fabs(side) > .01;
    int sprint = in.sprint && moving && !s->indoor && !s->exhausted && s->stamina > 0;
    double speed = sprint ? 6 : 3.5;
    if (sprint) {
        exert(s, 18 * dt, dt);
        if (s->stamina <= 0) {
            s->exhausted = 1;
            s->shift_released = 0;
        }
    }
    if (!sprint && s->recovery == 0)
        s->stamina = fmin(100, s->stamina + (s->rested > 0 ? 20 : 14) * dt);
    double dx = (sin(s->yaw) * f + cos(s->yaw) * side) * speed * dt,
           dz = (-cos(s->yaw) * f + sin(s->yaw) * side) * speed * dt;
    if (in.jump && s->grounded) {
        s->vertical = 5.5;
        s->grounded = 0;
    }
    s->vertical -= 14 * dt;
    if (s->indoor) {
        WcVec p = s->local;
        double c = cos(s->castle_yaw), q = sin(s->castle_yaw);
        p.x += c * dx + q * dz;
        p.z += -q * dx + c * dz;
        int valid;
        double support = wc_support(s, p, &valid);
        /* Walk collision is the room graph. Gaps and sealed doors are solid. */
        if (valid && support <= s->local.y + .65) {
            s->local.x = p.x;
            s->local.z = p.z;
        }
        p = s->local;
        support = wc_support(s, p, &valid);
        s->local.y += s->vertical * dt;
        if (valid && s->local.y <= support) {
            s->local.y = support;
            s->vertical = 0;
            s->grounded = 1;
        }
        if (s->local.y < -2 && !valid) {
            s->local = (WcVec){0, 0, 4};
            s->vertical = 0;
            s->grounded = 1;
        }
        s->player = wc_local_world(s, s->local);
        if (s->local.z > 8.25 && s->gate) {
            s->indoor = 0;
            s->helm = 0;
        }
    } else {
        WcPos p = s->player;
        wc_pos_add(&p, dx, 0, dz);
        if (!s->storage_failed && !wc_outdoor_blocked(s, p)) {
            s->player.cx = p.cx;
            s->player.cz = p.cz;
            s->player.x = p.x;
            s->player.z = p.z;
        }
        double floor = wc_height(s->seed, s->player);
        WcVec local = wc_world_local(s, s->player);
        int valid = 0;
        double stair = wc_support(s, local, &valid) + s->castle.y;
        if (s->gate && s->speed == 0 && valid && stair <= s->player.y + .65)
            floor = fmax(floor, stair);
        s->player.y += s->vertical * dt;
        if (s->player.y <= floor) {
            s->player.y = floor;
            s->vertical = 0;
            s->grounded = 1;
        }
        local = wc_world_local(s, s->player);
        if (s->speed == 0 && s->gate && fabs(local.x) < 1.3 && local.z < 8 && local.z > 6.5 &&
            fabs(local.y) < .7) {
            s->local = local;
            sanctuary(s);
        }
    }
    danger(g, dt);
}
