#include "wc.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool fail(WcGame *g, const char *why) {
    snprintf(g->message, sizeof(g->message), "%s", why);
    return false;
}
static WcMutation *record(WcState *s, uint64_t id) {
    for (int i = 0; i < s->mutation_count; i++)
        if (s->mutations[i].id == id)
            return &s->mutations[i];
    if (s->mutation_count == WC_MUTATIONS)
        return NULL;
    WcMutation *m = &s->mutations[s->mutation_count++];
    *m = (WcMutation){.id = id, .stock = 20};
    return m;
}
static bool near(const WcState *s, WcPos p, double distance) {
    WcVec d = wc_relative(s->player, p);
    return !s->indoor && hypot(d.x, d.z) <= distance && fabs(d.y) < 4;
}
static bool at_foundry(const WcState *s, double x, double z) {
    return near(s, wc_pos(-30 + x, 0, 28 + z), 3.8);
}
static bool feature_target(const WcState *s, uint64_t id, WcFeature *out) {
    WcScene scene;
    wc_scene(s, &scene);
    for (int i = 0; i < scene.count; i++)
        if (scene.f[i].id == id) {
            *out = scene.f[i];
            return true;
        }
    return false;
}
static bool attack_target(WcState *s, uint64_t id, int damage, int ward) {
    WcFeature f;
    if (id == 2)
        f = (WcFeature){.p = wc_pos(-30, 0, 20), .id = 2, .type = FEATURE_PROWLER};
    else if (!feature_target(s, id, &f) || f.type != FEATURE_PROWLER)
        return false;
    if (!near(s, f.p, ward ? 7 : 3.2))
        return false;
    WcVec d = wc_relative(f.p, s->player);
    double len = hypot(d.x, d.z);
    if (!ward && len > .01 && (d.x * sin(s->yaw) - d.z * cos(s->yaw)) / len < .72)
        return false;
    for (double t = .2; t < len - .3; t += .2) {
        WcPos p = s->player;
        wc_pos_add(&p, d.x * t / len, 1, d.z * t / len);
        if (wc_outdoor_blocked(s, p))
            return false;
    }
    WcMutation *m = record(s, id);
    if (!m || m->ready > s->time)
        return false;
    if (ward) {
        if (id == 2)
            return false;
        m->ready = s->time + 15;
        return true;
    }
    if (id == 2) {
        if (s->foundry_brake || s->foundry_reserve || fmod(s->time, 3) < 1.8)
            return false;
        if (!(m->flags & 1)) {
            m->stock = 100;
            m->flags |= 1;
        }
        m->stock -= damage;
        if (m->stock <= 0) {
            s->foundry_brake = 1;
            m->ready = s->time + 1e12;
            wc_xp(s, 6, 1u << 6);
        }
    } else {
        if (!(m->flags & 1)) {
            m->stock = 35;
            m->flags |= 1;
        }
        m->stock -= damage;
        if (m->stock <= 0) {
            m->ready = s->time + 120;
            m->flags = 0;
            wc_xp(s, 2, 1u << 7);
        }
    }
    return true;
}
bool wc_command(WcGame *g, WcCommand cmd) {
    WcState next = g->s, *s = &next;
    int cost[WC_ITEMS] = {0};
    const char *ok = "Done.";
    switch (cmd.type) {
    case CMD_CRAFT: {
        if (!s->indoor)
            return fail(g, "Craft at the home workshop. Shared home supplies pay.");
        if (cmd.a < 0 || cmd.a >= g->defs.recipe_count)
            return fail(g, "Unknown recipe.");
        WcRecipe *r = &g->defs.recipes[cmd.a];
        if ((r->unlock == 1 && !s->wedge_recipe) || (r->unlock == 2 && !s->hearth))
            return fail(g, "Discover this recipe or install its facility first.");
        if (r->output == SATCHEL && s->satchel)
            return fail(g, "Field satchel is already installed.");
        if (!wc_pay(s, r->cost, 1))
            return fail(g, "Not enough shared home supplies.");
        if (r->output == SATCHEL) {
            s->satchel = 1;
            s->backpack.capacity = 40;
        } else if (!wc_add(&s->backpack, r->output, r->quantity))
            return fail(g, "Backpack full after ingredient consumption.");
        wc_xp(s, 3, 1u << 0);
        ok = "Crafted. Paid from backpack, pantry, then room cabinets.";
        break;
    }
    case CMD_PLANT: {
        if (!s->indoor || !s->hoe || cmd.a < 0 || cmd.a >= WC_CROPS || cmd.a >= s->room_count * 2 ||
            cmd.b < 0 || cmd.b > 1)
            return fail(g, "Use a home planter with your reusable hoe.");
        WcCrop *crop = &s->crops[cmd.a];
        if (crop->kind >= 0)
            return fail(g, "Planter occupied.");
        cost[cmd.b == 0 ? SUN_SEED : MINT_SEED] = 1;
        if (!wc_pay(s, cost, 1))
            return fail(g, "No seed in shared home supplies.");
        *crop = (WcCrop){cmd.a / 2, cmd.b, 0, s->time, g->defs.growth[cmd.b]};
        ok = "Planted. Growth uses active time; mature crops wait forever.";
        break;
    }
    case CMD_TEND: {
        if (!s->indoor || cmd.a < 0 || cmd.a >= WC_CROPS)
            return fail(g, "Tend a home planter.");
        WcCrop *c = &s->crops[cmd.a];
        if (c->kind < 0 || c->treated || s->time - c->planted >= c->duration)
            return fail(g, "Crop cannot receive another treatment.");
        if (!(s->techniques & CAREFUL_TENDING)) {
            cost[COMPOST] = 1;
            if (!wc_pay(s, cost, 1))
                return fail(g, "Need compost or Careful Tending.");
        }
        c->duration *= .5;
        c->treated = 1;
        ok = "Tended once. Growth duration halved.";
        break;
    }
    case CMD_HARVEST: {
        if (!s->indoor || cmd.a < 0 || cmd.a >= WC_CROPS)
            return fail(g, "Harvest a home planter.");
        WcCrop *c = &s->crops[cmd.a];
        if (c->kind < 0 || s->time - c->planted < c->duration)
            return fail(g, "Crop is not mature yet.");
        if (!wc_add(&s->backpack, c->kind == 0 ? SUNROOT : MINT, g->defs.yield[c->kind]) ||
            !wc_add(&s->backpack, c->kind == 0 ? SUN_SEED : MINT_SEED, 1))
            return fail(g, "Need room for crop and replacement seed.");
        c->kind = -1;
        wc_xp(s, 5, 1u << 1);
        ok = "Harvested with a replacement seed. No crop loss or spoilage.";
        break;
    }
    case CMD_GATHER: {
        WcFeature f;
        if (!feature_target(s, cmd.target, &f) || f.type < FEATURE_TIMBER ||
            f.type > FEATURE_SEED || !near(s, f.p, 3.5))
            return fail(g, "Leave home and aim at a nearby outdoor resource.");
        WcMutation *m = record(s, cmd.target);
        if (!m)
            return fail(g, "Persistent journal full. Return home and save.");
        if (m->ready > s->time)
            return fail(g, "Site replenishes after 120 active seconds.");
        int item = f.type == FEATURE_TIMBER  ? TIMBER
                   : f.type == FEATURE_STONE ? STONE
                   : f.type == FEATURE_FIBER ? FIBER
                                             : SUN_SEED;
        int qty = f.type == FEATURE_SEED ? 2 : 3;
        if (!wc_add(&s->backpack, item, qty))
            return fail(g, "Backpack full. Nothing was depleted.");
        s->eligible[item] += qty;
        m->ready = s->time + 120;
        wc_xp(s, 3, 1u << 2);
        ok = "Gathered. Return home to secure this haul.";
        break;
    }
    case CMD_DEPOSIT:
    case CMD_WITHDRAW: {
        if (!s->indoor || cmd.a < 0 || cmd.a >= ITEM_COUNT || cmd.b <= 0 || cmd.b > 1000 ||
            cmd.c < 0 || cmd.c > s->room_count)
            return fail(g, "Choose an item and home storage.");
        WcInventory *storage = cmd.c ? &s->rooms[cmd.c - 1].cabinet : &s->pantry;
        WcInventory *from = cmd.type == CMD_DEPOSIT ? &s->backpack : storage,
                    *to = cmd.type == CMD_DEPOSIT ? storage : &s->backpack;
        if (from->n[cmd.a] < cmd.b || !wc_add(to, cmd.a, cmd.b))
            return fail(g, "Insufficient stock or destination capacity.");
        from->n[cmd.a] -= cmd.b;
        s->eligible[cmd.a] = 0;
        break;
    }
    case CMD_BUY:
    case CMD_SELL: {
        if (!near(s, wc_pos(17, 0, 20), 4) || cmd.a < 0 || cmd.a >= ITEM_COUNT || cmd.b <= 0 ||
            cmd.b > 80)
            return fail(g, "Trade with the exterior merchant.");
        WcMutation *m = record(s, UINT64_C(0xf0000000) + cmd.a);
        if (!m)
            return fail(g, "Persistent journal full.");
        if (s->time >= m->ready && m->ready > 0) {
            m->stock = 20;
            m->ready = 0;
        }
        WcItem *item = &g->defs.items[cmd.a];
        int price = (cmd.type == CMD_BUY ? item->buy : item->sell) * cmd.b;
        if (price <= 0)
            return fail(g, "Merchant does not trade this item.");
        if (cmd.type == CMD_BUY) {
            if (cmd.a == SATCHEL && (s->satchel || cmd.b != 1))
                return fail(g, "A single field satchel installs permanently; it cannot stack.");
            if (m->stock < cmd.b || s->coins < price || !wc_add(&s->backpack, cmd.a, cmd.b))
                return fail(g, "Need coins, merchant stock and backpack capacity.");
            s->coins -= price;
            m->stock -= cmd.b;
            if (!m->ready)
                m->ready = s->time + 120;
            if (cmd.a == SATCHEL) {
                s->backpack.n[SATCHEL]--;
                s->satchel = 1;
                s->backpack.capacity = 40;
            }
            if (cmd.a == ALLOY) {
                s->foundry_notes = 1;
                s->wedge_recipe = 1;
                wc_xp(s, 6, 1u << 3);
            }
        } else {
            cost[cmd.a] = cmd.b;
            if (!wc_pay(s, cost, 0))
                return fail(g, "Not enough carried stock.");
            s->coins += price;
        }
        ok = "Trade complete. Purchased goods are protected on defeat.";
        break;
    }
    case CMD_BUILD: {
        if (!s->indoor)
            return fail(g, "Build from inside the home.");
        char why[120];
        int kind = cmd.c / 4, rot = cmd.c % 4;
        if (!wc_room_valid(s, cmd.a, cmd.b, kind, rot, why, sizeof(why)))
            return fail(g, why);
        if (!wc_pay(s, g->defs.room_cost, 1))
            return fail(g, "Room needs shared supplies: 6 planks, 2 twine, 4 stone.");
        WcRoom p = s->rooms[cmd.a];
        int id = s->room_count;
        s->rooms[id] = (WcRoom){.id = id + 1,
                                .parent = cmd.a,
                                .kind = kind,
                                .tier = p.tier + (cmd.b == 0),
                                .gx = p.gx + (cmd.b == 2) - (cmd.b == 1),
                                .gz = p.gz + (cmd.b == 4) - (cmd.b == 3),
                                .rotation = rot,
                                .furniture = FURN_CABINET,
                                .cabinet = {.capacity = 40}};
        s->room_count++;
        wc_xp(s, 6, 1u << 4);
        ok = "Furnished room added. Stairs, sanctuary and travel clearance extended.";
        break;
    }
    case CMD_REPLACE: {
        if (!s->indoor || cmd.a < 0 || cmd.a >= s->room_count || cmd.b < 0 || cmd.b > 1)
            return fail(g, "Choose an eligible home cabinet.");
        WcRoom *r = &s->rooms[cmd.a];
        if (r->furniture == cmd.b)
            return fail(g, "That furnishing is already installed.");
        int capacity = cmd.b == FURN_CABINET ? 40 : 12;
        if (wc_inventory_used(&r->cabinet) > capacity)
            return fail(g, "New cabinet cannot preserve stored contents.");
        int packed = cmd.b == FURN_CABINET ? PACK_CABINET : PACK_COMPACT;
        if (wc_available(s, packed) > 0)
            cost[packed] = 1;
        else {
            cost[PLANK] = 2;
            cost[STONE] = 1;
        }
        if (!wc_pay(s, cost, 1))
            return fail(g, "Need packed furnishing or 2 planks + 1 stone.");
        if (!wc_add(&s->backpack, r->furniture == FURN_CABINET ? PACK_CABINET : PACK_COMPACT, 1))
            return fail(g, "Backpack needs space for the packed old cabinet.");
        r->furniture = cmd.b;
        r->cabinet.capacity = capacity;
        ok = "Replaced. Contents and room records preserved; old cabinet packed.";
        break;
    }
    case CMD_REST:
        if (!s->indoor)
            return fail(g, "Rest at home.");
        s->health = s->stamina = 100;
        s->rested = 120;
        s->exhausted = 0;
        ok = "Rested: health and stamina restored. 120 effort seconds.";
        break;
    case CMD_EAT:
        if (s->meal > 0)
            return fail(g, "Meal benefit is already active. No food consumed.");
        cost[STEW] = 1;
        if (!wc_pay(s, cost, s->indoor))
            return fail(g, "No trail stew available.");
        s->meal = 180;
        ok = "Trail stew: 30% lower effort costs for 180 outdoor effort seconds.";
        break;
    case CMD_LEARN:
        if ((cmd.a != SECOND_WIND && cmd.a != POWER_STRIKE && cmd.a != CAREFUL_TENDING) ||
            s->points < 1 || (s->techniques & cmd.a))
            return fail(g, "Need an unspent technique point and an unlearned technique.");
        s->points--;
        s->techniques |= cmd.a;
        break;
    case CMD_SECOND_WIND:
        if (s->indoor || !(s->techniques & SECOND_WIND) || s->rested < 15 || s->stamina >= 100)
            return fail(
                g, "Second Wind: outdoors, learned, 15 rested effort seconds, missing stamina.");
        s->rested -= 15;
        s->stamina = fmin(100, s->stamina + 40);
        break;
    case CMD_ATTACK:
    case CMD_POWER:
    case CMD_WARD: {
        int power = cmd.type == CMD_POWER, ward = cmd.type == CMD_WARD;
        double effort = (power ? 25 : 8) * (s->meal > 0 ? .7 : 1);
        if (s->indoor || s->attack_cd > 0 || (power && !(s->techniques & POWER_STRIKE)) ||
            (!ward && s->stamina < effort))
            return fail(g, "Attack unavailable: sanctuary, cooldown, technique or stamina.");
        if (ward && !cmd.target) {
            WcScene scene;
            wc_scene(s, &scene);
            for (int i = 0; i < scene.count; i++)
                if (scene.f[i].type == FEATURE_PROWLER && near(s, scene.f[i].p, 7)) {
                    cmd.target = scene.f[i].id;
                    break;
                }
        }
        if (!attack_target(
                s, cmd.target,
                (s->spear ? g->defs.spear.damage : g->defs.staff.damage) * (power ? 2 : 1), ward))
            return fail(g, "No exposed outdoor target in aim, range and clear sight.");
        if (!ward) {
            s->stamina -= effort;
            s->rested = fmax(0, s->rested - .4);
            s->meal = fmax(0, s->meal - .4);
            s->recovery = .8;
        }
        s->attack_cd = 1;
        ok = ward ? "Prowler repelled. No reward."
                  : "Strike landed. Retreat during the warning; strike during recovery.";
        break;
    }
    case CMD_HEARTH:
    case CMD_TEMPER:
    case CMD_SPEAR:
        if (!s->indoor)
            return fail(g, "Install and craft at home.");
        if (cmd.type == CMD_HEARTH) {
            if (s->hearth)
                return fail(g, "Hearth already installed.");
            cost[PLANK] = 2;
            cost[STONE] = 2;
        }
        if (cmd.type == CMD_TEMPER) {
            if (!s->foundry_notes || s->tempered)
                return fail(g, "Discover foundry plans; workshop installs once.");
            cost[ALLOY] = 2;
            cost[PLANK] = 4;
            cost[STONE] = 4;
        }
        if (cmd.type == CMD_SPEAR) {
            if (!s->tempered || s->spear)
                return fail(g, "Need tempered workshop; spear equips once.");
            cost[ALLOY] = 2;
            cost[PLANK] = 2;
            cost[TWINE] = 1;
        }
        if (!wc_pay(s, cost, 1))
            return fail(g, "Insufficient shared home supplies.");
        if (cmd.type == CMD_HEARTH)
            s->hearth = 1;
        if (cmd.type == CMD_TEMPER)
            s->tempered = 1;
        if (cmd.type == CMD_SPEAR)
            s->spear = 1;
        break;
    case CMD_NOTES:
        if (!at_foundry(s, -5, 5) || s->foundry_notes)
            return fail(g, "Read the nearby unclaimed foundry notes.");
        s->foundry_notes = 1;
        s->wedge_recipe = 1;
        wc_xp(s, 6, 1u << 3);
        ok = "Learned mechanism wedge and tempered workshop. Maintenance route is peaceful.";
        break;
    case CMD_WEDGE:
        if (!at_foundry(s, 0, 3) || s->foundry_trap)
            return fail(g, "Approach the active pressure plate.");
        cost[WEDGE] = 1;
        if (!wc_pay(s, cost, 0))
            return fail(g, "Carry one mechanism wedge.");
        s->foundry_trap = 1;
        wc_xp(s, 3, 1u << 5);
        break;
    case CMD_BRAKE:
        if (!at_foundry(s, 6, -5) || s->foundry_brake)
            return fail(g, "Reach the maintenance brake on the marked side passage.");
        s->foundry_brake = 1;
        wc_xp(s, 6, 1u << 6);
        ok = "Brake engaged. Steam and guardian stopped. Reserve unlocked peacefully.";
        break;
    case CMD_SALVAGE:
    case CMD_RESERVE: {
        int reserve = cmd.type == CMD_RESERVE;
        if (!at_foundry(s, reserve ? 0 : -5, reserve ? -8 : 2) ||
            (reserve ? s->foundry_reserve : !!(s->achievements & (1u << 8))) ||
            (reserve && !s->foundry_brake))
            return fail(g, "Reserve locked, reward already claimed, or out of reach.");
        const int *quantities = reserve ? g->defs.reserve : g->defs.salvage;
        for (int i = 0; i < WC_ITEMS; i++)
            if (quantities[i]) {
                if (!wc_add(&s->backpack, i, quantities[i]))
                    return fail(g, "Backpack cannot hold the entire one-time reward.");
                s->eligible[i] += quantities[i];
            }
        if (reserve)
            s->foundry_reserve = 1;
        else
            s->achievements |= 1u << 8;
        wc_xp(s, 4, reserve ? 1u << 9 : 1u << 10);
        ok = "Salvage secured in backpack. Return now or continue with the haul at risk.";
        break;
    }
    case CMD_RECOVER: {
        if (cmd.a < 0 || cmd.a >= WC_CACHES || !s->caches[cmd.a].active ||
            !near(s, s->caches[cmd.a].pos, 3.5))
            return fail(g, "Approach a persistent recovery cache.");
        WcCache *c = &s->caches[cmd.a];
        for (int i = 0; i < WC_ITEMS; i++)
            if (c->items[i] && !wc_add(&s->backpack, i, c->items[i]))
                return fail(g, "Backpack cannot hold the entire cache. Nothing moved.");
        c->active = 0;
        memset(c->items, 0, sizeof(c->items));
        ok = "Cache recovered. These supplies are protected on future defeat.";
        break;
    }
    case CMD_GATE: {
        if (s->speed != 0 || s->helm)
            return fail(g, "Park and leave the helm before deploying the entrance.");
        WcVec p = s->indoor ? s->local : wc_world_local(s, s->player);
        if (s->gate) {
            int support = 0;
            wc_support(s, p, &support);
            if ((p.z > 7.1 && support) || (p.z > 7 && fabs(p.x) < 2 && p.y > -3))
                return fail(g, "Step clear: the gate or stairs are supporting you.");
            s->gate = 0;
            s->stairs.deployed = 0;
            ok = "Entrance closed.";
        } else {
            WcStairs stairs;
            if (!wc_solve_stairs(s, &stairs))
                return fail(g, "No safe stair route: terrain, obstruction, landing or grade.");
            WcState candidate = *s;
            candidate.stairs = stairs;
            candidate.gate = 1;
            int support = 0;
            double y = wc_support(&candidate, p, &support);
            if (!s->indoor && support && p.y < y + 1.8 && p.y > y - 1.8)
                return fail(g, "Step sideways clear of the stairs before opening.");
            s->stairs = stairs;
            s->gate = 1;
            ok = "Oak gate open. Adaptive stairs and ground bell ready.";
        }
        break;
    }
    case CMD_HELM:
        if (!s->indoor || hypot(s->local.x, s->local.z + 5.8) > 3.5)
            return fail(g, "Stand at the physical command helm.");
        s->helm = !s->helm;
        s->speed = 0;
        ok = s->helm ? "Helm occupied. WASD steer; Shift turbo; E release."
                     : "Helm released. Castle parked.";
        break;
    case CMD_CLUE: {
        WcFeature f;
        if (!feature_target(s, cmd.target, &f) || !near(s, f.p, 4))
            return fail(g, "Reach the outdoor discovery marker.");
        WcMutation *m = record(s, cmd.target);
        if (!m || m->flags & 2)
            return fail(g, "Discovery already recorded or journal full.");
        if (!wc_add(&s->backpack, ALLOY, 1) || !wc_add(&s->backpack, PLANK, 2))
            return fail(g, "Need three backpack units for this discovery.");
        s->eligible[ALLOY]++;
        s->eligible[PLANK] += 2;
        m->flags |= 2;
        wc_xp(s, 4, 0);
        ok = "Discovery recorded. Alloy, plans and a readable return route.";
        break;
    }
    default:
        return fail(g, "Unknown command.");
    }
    g->s = next;
    snprintf(g->message, sizeof(g->message), "%s", ok);
    return true;
}
