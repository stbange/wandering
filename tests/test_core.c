#include "content.h"
#include "wc.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        checks++;                                                                                  \
        if (!(x)) {                                                                                \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                           \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
#define CLOSE(a, b) CHECK(fabs((a) - (b)) < .00001)
static WcGame game;
static WcDefs defs;
static void fresh(void) {
    memset(&game, 0, sizeof(game));
    game.defs = defs;
    wc_init(&game, 42);
    CHECK(wc_validate(&game.s));
}
static bool command(WcCommandType type, int a, int b, int c, uint64_t id) {
    return wc_command(&game, (WcCommand){.type = type, .a = a, .b = b, .c = c, .target = id});
}
static void reject(WcCommandType type, int a, int b, int c, uint64_t id) {
    WcState before = game.s;
    bool ok = command(type, a, b, c, id);
    if (ok)
        fprintf(stderr, "Unexpected success: command %d (%d,%d,%d)\n", type, a, b, c);
    CHECK(!ok);
    CHECK(!memcmp(&before, &game.s, sizeof(before)));
}
static void outside(double x, double z) {
    game.s.indoor = 0;
    game.s.helm = 0;
    game.s.player = wc_pos(x, 0, z);
    game.s.player.y = wc_height(game.s.seed, game.s.player);
}
static void aim(WcPos p) {
    WcVec d = wc_relative(p, game.s.player);
    game.s.yaw = atan2(d.x, -d.z);
    game.s.pitch = 0;
}
static void tick(double seconds, WcInput in) {
    for (int i = 0; i < (int)ceil(seconds * 60); i++)
        wc_step(&game, in, 1.0 / 60);
}
static void supplies(void) {
    memset(&game.s.backpack.n, 0, sizeof(game.s.backpack.n));
    memset(&game.s.pantry.n, 0, sizeof(game.s.pantry.n));
    game.s.pantry.n[PLANK] = 30;
    game.s.pantry.n[TWINE] = 16;
    game.s.pantry.n[STONE] = 30;
}

static void test_inventory(void) {
    fresh();
    WcInventory v = {.capacity = 4};
    CHECK(wc_add(&v, TIMBER, 4));
    CHECK(!wc_add(&v, STONE, 1));
    CHECK(!wc_add(&v, -1, 1));
    CHECK(!wc_add(&v, 0, -1));
    CHECK(!wc_add(&v, 0, 10001));
    CHECK(wc_inventory_used(&v) == 4);
    CHECK(wc_available(&game.s, -1) == 0);
    int cost[WC_ITEMS] = {0};
    cost[TIMBER] = 7;
    WcState before = game.s;
    CHECK(!wc_pay(&game.s, cost, 1));
    CHECK(!memcmp(&before, &game.s, sizeof(before)));
    cost[TIMBER] = -1;
    CHECK(!wc_pay(&game.s, cost, 1));
    game.s.backpack.n[TIMBER] = 1;
    game.s.eligible[TIMBER] = 1;
    game.s.rooms[0].cabinet.n[TIMBER] = 3;
    cost[TIMBER] = 8;
    CHECK(wc_pay(&game.s, cost, 1));
    CHECK(game.s.backpack.n[TIMBER] == 0 && game.s.pantry.n[TIMBER] == 0 &&
          game.s.rooms[0].cabinet.n[TIMBER] == 2 && game.s.eligible[TIMBER] == 0);
    reject(CMD_CRAFT, -1, 0, 0, 0);
    reject(CMD_CRAFT, 100, 0, 0, 0);
    CHECK(command(CMD_CRAFT, 1, 0, 0, 0));
    CHECK(game.s.backpack.n[TWINE] == 2);
    game.s.pantry.n[TIMBER] = 0;
    game.s.rooms[0].cabinet.n[TIMBER] = 0;
    reject(CMD_CRAFT, 0, 0, 0, 0);
    game.s.pantry.n[TIMBER] = 2;
    game.s.backpack.n[STONE] = 22;
    reject(CMD_CRAFT, 0, 0, 0, 0);
    game.s.backpack.n[STONE] = 0;
    CHECK(command(CMD_CRAFT, 0, 0, 0, 0));
    CHECK(game.s.backpack.n[PLANK] == 2);
    CHECK(command(CMD_CRAFT, 3, 0, 0, 0));
    CHECK(game.s.satchel && game.s.backpack.capacity == 40);
    reject(CMD_CRAFT, 3, 0, 0, 0);
    reject(CMD_CRAFT, 4, 0, 0, 0);
    reject(CMD_CRAFT, 5, 0, 0, 0);
    outside(0, 0);
    reject(CMD_CRAFT, 0, 0, 0, 0);
    fresh();
    reject(CMD_DEPOSIT, 0, 1, 0, 0);
    CHECK(command(CMD_DEPOSIT, SUN_SEED, 1, 0, 0));
    CHECK(command(CMD_WITHDRAW, SUN_SEED, 1, 0, 0));
    reject(CMD_DEPOSIT, -1, 1, 0, 0);
    reject(CMD_DEPOSIT, 0, -1, 0, 0);
    reject(CMD_DEPOSIT, 0, 1, 99, 0);
    game.s.backpack.n[SUN_SEED] = 24;
    reject(CMD_WITHDRAW, TIMBER, 1, 0, 0);
    outside(0, 0);
    reject(CMD_DEPOSIT, SUN_SEED, 1, 0, 0);
}
static void test_garden(void) {
    fresh();
    CHECK(command(CMD_PLANT, 0, 0, 0, 0));
    reject(CMD_PLANT, 0, 1, 0, 0);
    reject(CMD_PLANT, 24, 1, 0, 0);
    reject(CMD_PLANT, 1, 2, 0, 0);
    reject(CMD_HARVEST, 0, 0, 0, 0);
    reject(CMD_TEND, 0, 0, 0, 0);
    game.s.backpack.n[COMPOST] = 1;
    CHECK(command(CMD_TEND, 0, 0, 0, 0));
    reject(CMD_TEND, 0, 0, 0, 0);
    CLOSE(game.s.crops[0].duration, 22.5);
    game.paused = 1;
    tick(60, (WcInput){0});
    CLOSE(game.s.time, 0);
    game.paused = 0;
    tick(23, (WcInput){0});
    CHECK(command(CMD_HARVEST, 0, 0, 0, 0));
    CHECK(game.s.backpack.n[SUNROOT] == 3);
    reject(CMD_HARVEST, 0, 0, 0, 0);
    CHECK(command(CMD_PLANT, 1, 1, 0, 0));
    game.s.time += 60000;
    game.s.backpack.n[STONE] = 20;
    reject(CMD_HARVEST, 1, 0, 0, 0);
    game.s.backpack.n[STONE] = 0;
    CHECK(command(CMD_HARVEST, 1, 0, 0, 0));
    game.s.techniques = CAREFUL_TENDING;
    CHECK(command(CMD_PLANT, 0, 1, 0, 0));
    CHECK(command(CMD_TEND, 0, 0, 0, 0));
    CLOSE(game.s.crops[0].duration, 30);
    reject(CMD_TEND, 23, 0, 0, 0);
    reject(CMD_TEND, -1, 0, 0, 0);
    game.s.backpack.n[SUN_SEED] = 0;
    reject(CMD_PLANT, 1, 0, 0, 0);
    outside(0, 0);
    reject(CMD_PLANT, 0, 0, 0, 0);
    reject(CMD_TEND, 0, 0, 0, 0);
    reject(CMD_HARVEST, 0, 0, 0, 0);
}
static void test_build(void) {
    fresh();
    supplies();
    reject(CMD_BUILD, 0, 1, ROOM_HALL * 4, 0);
    reject(CMD_BUILD, -1, 0, 4, 0);
    reject(CMD_BUILD, 0, 0, 20, 0);
    game.s.local = (WcVec){0, 0, 0};
    reject(CMD_BUILD, 0, 0, 4, 0);
    game.s.local = (WcVec){4, 0, 4};
    CHECK(command(CMD_BUILD, 0, 0, 4, 0));
    CHECK(game.s.room_count == 2 && game.s.rooms[1].cabinet.capacity == 40);
    reject(CMD_BUILD, 0, 0, 4, 0);
    CHECK(command(CMD_BUILD, 1, 1, 8, 0));
    CHECK(game.s.rooms[2].gx == -1 && game.s.rooms[2].tier == 1);
    reject(CMD_BUILD, 1, 3, 4, 0);
    game.s.speed = 1;
    reject(CMD_BUILD, 1, 2, 8, 0);
    game.s.speed = 0;
    game.s.pantry.n[PLANK] = 0;
    reject(CMD_BUILD, 1, 2, 8, 0);
    outside(0, 0);
    reject(CMD_BUILD, 1, 2, 8, 0);
    fresh();
    supplies();
    CHECK(command(CMD_REPLACE, 0, FURN_COMPACT, 0, 0));
    CHECK(game.s.backpack.n[PACK_CABINET] == 1 && game.s.rooms[0].cabinet.capacity == 12);
    CHECK(command(CMD_REPLACE, 0, FURN_CABINET, 0, 0));
    CHECK(game.s.backpack.n[PACK_COMPACT] == 1);
    CHECK(command(CMD_REPLACE, 0, FURN_COMPACT, 0, 0));
    reject(CMD_REPLACE, 0, FURN_COMPACT, 0, 0);
    reject(CMD_REPLACE, -1, 0, 0, 0);
    CHECK(command(CMD_REPLACE, 0, FURN_CABINET, 0, 0));
    game.s.rooms[0].cabinet.n[STONE] = 13;
    reject(CMD_REPLACE, 0, FURN_COMPACT, 0, 0);
    game.s.rooms[0].cabinet.n[STONE] = 0;
    game.s.pantry.n[PLANK] = 0;
    game.s.backpack.n[PACK_COMPACT] = 0;
    reject(CMD_REPLACE, 0, FURN_COMPACT, 0, 0);
    game.s.pantry.n[PLANK] = 2;
    game.s.backpack.n[SUN_SEED] = 24;
    reject(CMD_REPLACE, 0, FURN_COMPACT, 0, 0);
}
static void test_progress(void) {
    fresh();
    game.s.health = 10;
    game.s.stamina = 1;
    double t = game.s.time;
    CHECK(command(CMD_REST, 0, 0, 0, 0));
    CHECK(game.s.health == 100 && game.s.stamina == 100);
    CLOSE(t, game.s.time);
    reject(CMD_SECOND_WIND, 0, 0, 0, 0);
    reject(CMD_EAT, 0, 0, 0, 0);
    reject(CMD_LEARN, 1, 0, 0, 0);
    wc_xp(&game.s, 12, 1u << 20);
    wc_xp(&game.s, 12, 1u << 20);
    CHECK(game.s.xp == 12 && game.s.points == 1);
    CHECK(command(CMD_LEARN, SECOND_WIND, 0, 0, 0));
    reject(CMD_LEARN, SECOND_WIND, 0, 0, 0);
    wc_xp(&game.s, 120, 0);
    CHECK(game.s.points == 2 && game.s.awarded == 3);
    CHECK(command(CMD_LEARN, POWER_STRIKE, 0, 0, 0));
    CHECK(command(CMD_LEARN, CAREFUL_TENDING, 0, 0, 0));
    reject(CMD_LEARN, 8, 0, 0, 0);
    game.s.backpack.n[STEW] = 2;
    CHECK(command(CMD_EAT, 0, 0, 0, 0));
    reject(CMD_EAT, 0, 0, 0, 0);
    tick(10, (WcInput){.forward = 1, .sprint = 1});
    CLOSE(game.s.meal, 180);
    outside(50, 50);
    game.s.stamina = 20;
    CHECK(command(CMD_SECOND_WIND, 0, 0, 0, 0));
    CHECK(game.s.stamina == 60);
    game.s.rested = 0;
    reject(CMD_SECOND_WIND, 0, 0, 0, 0);
    reject(CMD_REST, 0, 0, 0, 0);
    fresh();
    supplies();
    CHECK(command(CMD_HEARTH, 0, 0, 0, 0));
    reject(CMD_HEARTH, 0, 0, 0, 0);
    reject(CMD_TEMPER, 0, 0, 0, 0);
    reject(CMD_SPEAR, 0, 0, 0, 0);
    game.s.foundry_notes = 1;
    game.s.pantry.n[ALLOY] = 4;
    CHECK(command(CMD_TEMPER, 0, 0, 0, 0));
    CHECK(command(CMD_SPEAR, 0, 0, 0, 0));
    reject(CMD_TEMPER, 0, 0, 0, 0);
    reject(CMD_SPEAR, 0, 0, 0, 0);
}
static void test_world(void) {
    fresh();
    WcPos p = wc_pos(-.01, 2, -128.01);
    CHECK(p.cx == -1 && p.cz == -2);
    CLOSE(p.x, 127.99);
    wc_pos_add(&p, .02, 1, .02);
    CHECK(p.cx == 0 && p.cz == -1);
    CLOSE(p.y, 3);
    WcPos a = {INT64_C(1000000000), -12345, 127.999, 0, 127.999}, b = a;
    wc_pos_add(&b, .002, 0, .002);
    CHECK(fabs(wc_height(42, a) - wc_height(42, b)) < .01);
    CHECK(wc_hash(1) != wc_hash(2));
    for (int x = -20; x < 20; x++)
        for (int z = -20; z < 20; z++) {
            p = wc_pos(x * 128 - .00001, 0, z * 128);
            b = p;
            wc_pos_add(&b, .00002, 0, 0);
            CHECK(fabs(wc_height(2, p) - wc_height(2, b)) < .001);
            CHECK(wc_biome(42, p) >= 0 && wc_biome(42, p) <= 2);
        }
    WcScene scene;
    wc_scene(&game.s, &scene);
    CHECK(scene.chunks <= WC_CHUNKS && scene.count <= 96);
    CHECK(wc_height(42, wc_pos(63, 0, 63)) == 0);
    WcStairs stairs;
    CHECK(wc_solve_stairs(&game.s, &stairs));
    CHECK(stairs.length1 + stairs.length2 <= 48);
    CHECK(fabs(stairs.landing.y) / (stairs.length1 + stairs.length2) <= .8);
    game.s.stairs = stairs;
    game.s.gate = 1;
    game.s.stairs.deployed = 1;
    for (int i = 0; i < 20; i++) {
        WcVec v = {0, 0, 8 + (stairs.landing.z - 8) * i / 19.0};
        int valid;
        double y = wc_support(&game.s, v, &valid);
        CHECK(valid);
        CHECK(isfinite(y));
    }
    WcBox boxes[64];
    CHECK(wc_site_boxes(FEATURE_KEEP, boxes, 64) > 0);
    CHECK(wc_site_boxes(FEATURE_HAMLET, boxes, 64) > 0);
    CHECK(wc_site_boxes(FEATURE_MINE, boxes, 64) > 0);
    CHECK(wc_site_boxes(0, boxes, 64) == 0);
    outside(-38, 28);
    CHECK(wc_outdoor_blocked(&game.s, game.s.player));
    outside(0, 0);
    CHECK(!wc_outdoor_blocked(&game.s, game.s.player));
    CHECK(wc_castle_blocked(&game.s, wc_pos(-30, 5, 28), 0));
    CHECK(!wc_castle_blocked(&game.s, wc_pos(0, 5, 0), 0));
}
static void test_movement(void) {
    fresh();
    wc_look(&game.s, 1, 100);
    CLOSE(game.s.pitch, 1.48);
    wc_look(&game.s, 0, -.1);
    CLOSE(game.s.pitch, 1.38);
    wc_look(&game.s, 100, -100);
    CLOSE(game.s.pitch, -1.48);
    game.s.pitch = 0;
    WcState before = game.s;
    game.paused = 1;
    wc_step(&game, (WcInput){.forward = 1}, 1);
    CHECK(!memcmp(&before, &game.s, sizeof(before)));
    game.paused = 0;
    wc_step(&game, (WcInput){0}, NAN);
    wc_step(&game, (WcInput){0}, -1);
    reject(CMD_HELM, 0, 0, 0, 0);
    game.s.local = (WcVec){0, 0, -5.4};
    CHECK(command(CMD_HELM, 0, 0, 0, 0));
    double yaw = game.s.yaw;
    WcVec local = game.s.local;
    tick(10, (WcInput){.forward = 1, .side = .25, .sprint = 1});
    CHECK(!memcmp(&local, &game.s.local, sizeof(local)));
    CLOSE(game.s.yaw, yaw);
    CHECK(game.s.castle.y >= wc_clearance(&game.s, game.s.castle, game.s.castle_yaw) - .001);
    CHECK(command(CMD_HELM, 0, 0, 0, 0));
    CHECK(game.s.speed == 0);
    game.s.storage_failed = 1;
    CHECK(command(CMD_HELM, 0, 0, 0, 0));
    WcPos parked = game.s.castle;
    tick(1, (WcInput){.forward = 1});
    CHECK(!memcmp(&parked, &game.s.castle, sizeof(parked)));
    game.s.helm = 0;
    fresh();
    game.s.local = (WcVec){0, 0, 4};
    tick(.15, (WcInput){.jump = 1});
    CHECK(game.s.local.y > 0);
    tick(2, (WcInput){0});
    CLOSE(game.s.local.y, 0);
    CHECK(game.s.grounded);
    game.s.local = (WcVec){30, -10, 30};
    tick(.1, (WcInput){0});
    CLOSE(game.s.local.y, 0);
    CLOSE(game.s.local.z, 4);
    outside(50, 50);
    game.s.rested = 120;
    game.s.meal = 180;
    game.s.stamina = 1;
    tick(.2, (WcInput){.forward = 1, .sprint = 1});
    CHECK(game.s.exhausted && game.s.stamina == 0);
    tick(3, (WcInput){0});
    CHECK(!game.s.exhausted);
    CHECK(game.s.meal < 180 && game.s.meal > 179);
    parked = game.s.castle;
    tick(3, (WcInput){.forward = 1});
    CHECK(!memcmp(&parked, &game.s.castle, sizeof(parked)));
    fresh();
    CHECK(command(CMD_GATE, 0, 0, 0, 0));
    game.s.local = (WcVec){0, 0, 7.5};
    reject(CMD_GATE, 0, 0, 0, 0);
    game.s.local = (WcVec){0, 0, 4};
    CHECK(command(CMD_GATE, 0, 0, 0, 0));
    game.s.helm = 1;
    reject(CMD_GATE, 0, 0, 0, 0);
}
static void test_expedition(void) {
    fresh();
    reject(CMD_GATHER, 0, 0, 0, 10);
    WcScene sc;
    wc_scene(&game.s, &sc);
    for (int i = 2; i < 6; i++) {
        WcFeature f = sc.f[i];
        game.s.indoor = 0;
        game.s.player = f.p;
        CHECK(command(CMD_GATHER, 0, 0, 0, f.id));
        reject(CMD_GATHER, 0, 0, 0, f.id);
    }
    WcState before = game.s;
    wc_defeat(&game);
    CHECK(game.s.indoor && game.s.health == 100);
    CHECK(game.s.coins == before.coins);
    CHECK(game.s.caches[0].active);
    CHECK(game.s.backpack.n[SUN_SEED] >= 2);
    game.s.indoor = 0;
    game.s.player = game.s.caches[0].pos;
    game.s.backpack.n[STONE] = 20;
    reject(CMD_RECOVER, 0, 0, 0, 0);
    game.s.backpack.n[STONE] = 0;
    CHECK(command(CMD_RECOVER, 0, 0, 0, 0));
    reject(CMD_RECOVER, 0, 0, 0, 0);
    CHECK(game.s.eligible[TIMBER] == 0);
    fresh();
    outside(17, 20);
    CHECK(command(CMD_BUY, STONE, 2, 0, 0));
    CHECK(game.s.coins == 8);
    CHECK(command(CMD_SELL, STONE, 1, 0, 0));
    CHECK(game.s.coins == 9);
    reject(CMD_BUY, ALLOY, 2, 0, 0);
    reject(CMD_SELL, ALLOY, 1, 0, 0);
    reject(CMD_BUY, -1, 1, 0, 0);
    reject(CMD_BUY, 0, 0, 0, 0);
    game.s.coins = 100;
    CHECK(command(CMD_BUY, ALLOY, 1, 0, 0));
    CHECK(game.s.foundry_notes && game.s.wedge_recipe);
    game.s.time += 121;
    CHECK(command(CMD_BUY, STONE, 1, 0, 0));
    fresh();
    outside(-35, 33);
    CHECK(command(CMD_NOTES, 0, 0, 0, 0));
    reject(CMD_NOTES, 0, 0, 0, 0);
    outside(-35, 30);
    CHECK(command(CMD_SALVAGE, 0, 0, 0, 0));
    reject(CMD_SALVAGE, 0, 0, 0, 0);
    outside(-30, 31);
    reject(CMD_WEDGE, 0, 0, 0, 0);
    game.s.backpack.n[WEDGE] = 1;
    CHECK(command(CMD_WEDGE, 0, 0, 0, 0));
    reject(CMD_WEDGE, 0, 0, 0, 0);
    outside(-30, 20);
    reject(CMD_RESERVE, 0, 0, 0, 0);
    outside(-24, 23);
    CHECK(command(CMD_BRAKE, 0, 0, 0, 0));
    reject(CMD_BRAKE, 0, 0, 0, 0);
    outside(-30, 20);
    CHECK(command(CMD_RESERVE, 0, 0, 0, 0));
    reject(CMD_RESERVE, 0, 0, 0, 0);
    CHECK(game.s.foundry_reserve && game.s.xp > 12);
    fresh();
    outside(400, 400);
    wc_scene(&game.s, &sc);
    for (int i = 0; i < sc.count; i++)
        if (sc.f[i].type == FEATURE_CLUE) {
            game.s.player = sc.f[i].p;
            CHECK(command(CMD_CLUE, 0, 0, 0, sc.f[i].id));
            reject(CMD_CLUE, 0, 0, 0, sc.f[i].id);
            break;
        }
}
static void test_combat(void) {
    fresh();
    reject(CMD_ATTACK, 0, 0, 0, 2);
    reject(CMD_POWER, 0, 0, 0, 2);
    outside(-30, 22);
    game.s.time = 2;
    game.s.yaw = 0;
    CHECK(command(CMD_ATTACK, 0, 0, 0, 2));
    reject(CMD_ATTACK, 0, 0, 0, 2);
    game.s.attack_cd = 0;
    game.s.techniques = POWER_STRIKE;
    game.s.spear = 1;
    CHECK(command(CMD_POWER, 0, 0, 0, 2));
    game.s.attack_cd = 0;
    CHECK(command(CMD_POWER, 0, 0, 0, 2));
    CHECK(game.s.foundry_brake);
    fresh();
    outside(300, 300);
    WcScene sc;
    wc_scene(&game.s, &sc);
    int found = 0;
    for (int i = 0; i < sc.count; i++)
        if (sc.f[i].type == FEATURE_PROWLER) {
            found = 1;
            WcFeature f = sc.f[i];
            game.s.player = f.p;
            wc_pos_add(&game.s.player, 0, 0, 2);
            aim(f.p);
            CHECK(command(CMD_WARD, 0, 0, 0, f.id));
            CHECK(game.s.xp == 0);
            game.s.attack_cd = 0;
            reject(CMD_ATTACK, 0, 0, 0, f.id);
            game.s.time += 16;
            game.s.spear = 1;
            CHECK(command(CMD_ATTACK, 0, 0, 0, f.id));
            game.s.attack_cd = 0;
            CHECK(command(CMD_ATTACK, 0, 0, 0, f.id));
            break;
        }
    CHECK(found);
}
static void test_save(void) {
    fresh();
    unsigned char *bytes = malloc(WC_SAVE_MAX);
    CHECK(bytes);
    CHECK(command(CMD_GATE, 0, 0, 0, 0));
    game.s.local = (WcVec){0, -2, 11};
    size_t n = wc_encode(&game.s, bytes, WC_SAVE_MAX);
    CHECK(n > 1000);
    WcState out = {0};
    CHECK(wc_decode(&out, bytes, n));
    CHECK(out.seed == 42 && out.stairs.deployed && out.gate);
    CLOSE(out.local.z, 11);
    CHECK(!out.helm && out.speed == 0);
    WcState before = out;
    for (size_t i = 0; i < n; i += 137) {
        bytes[i] ^= 0x80;
        CHECK(!wc_decode(&out, bytes, n));
        CHECK(!memcmp(&before, &out, sizeof(out)));
        bytes[i] ^= 0x80;
    }
    CHECK(!wc_decode(&out, bytes, 0));
    CHECK(!wc_decode(&out, bytes, n - 1));
    CHECK(!wc_decode(&out, bytes, WC_SAVE_MAX + 1));
    CHECK(wc_encode(&game.s, bytes, 8) == 0);
    CHECK(wc_encode(&game.s, bytes, 100) == 0);
    game.s.health = NAN;
    CHECK(!wc_validate(&game.s));
    CHECK(wc_encode(&game.s, bytes, WC_SAVE_MAX) == 0);
    game.s.health = 100;
    game.s.crops[0].kind = 5;
    CHECK(!wc_validate(&game.s));
    game.s.crops[0].kind = -1;
    game.s.backpack.n[STONE] = -1;
    CHECK(!wc_validate(&game.s));
    free(bytes);
}
static void test_rays(void) {
    fresh();
    WcScene sc;
    wc_scene(&game.s, &sc);
    game.s.local = (WcVec){0, 0, -4};
    game.s.yaw = 0;
    game.s.pitch = -.25;
    WcHit h = wc_raycast(&game.s, &sc, 3.5);
    CHECK(h.kind == HIT_HELM);
    game.s.local = (WcVec){0, 0, 6};
    game.s.yaw = WC_PI;
    game.s.pitch = -.2;
    h = wc_raycast(&game.s, &sc, 3.5);
    CHECK(h.kind == HIT_GATE);
    outside(17, 22);
    game.s.yaw = 0;
    game.s.pitch = -.3;
    h = wc_raycast(&game.s, &sc, 3.5);
    CHECK(h.kind == HIT_TRADER);
    game.s.yaw = WC_PI;
    h = wc_raycast(&game.s, &sc, 3.5);
    CHECK(h.kind == HIT_NONE);
}
static void test_transaction_fuzz(void) {
    fresh();
    uint64_t r = 123;
    for (int i = 0; i < 6000; i++) {
        r = wc_hash(r);
        WcCommand c = {.type = (WcCommandType)(r % 34),
                       .a = (int)((r >> 8) % 30) - 2,
                       .b = (int)((r >> 16) % 10) - 1,
                       .c = (int)((r >> 24) % 16),
                       .target = (r >> 32) % 24};
        WcState before = game.s;
        bool ok = wc_command(&game, c);
        if (!ok)
            CHECK(!memcmp(&before, &game.s, sizeof(before)));
        CHECK(wc_validate(&game.s));
    }
}
static void test_save_validation(void) {
    fresh();
    WcState valid = game.s;
#define INVALID(member, value)                                                                     \
    do {                                                                                           \
        game.s = valid;                                                                            \
        game.s.member = (value);                                                                   \
        CHECK(!wc_validate(&game.s));                                                              \
    } while (0)
    INVALID(generation, 99);
    INVALID(content, 99);
    INVALID(castle.x, 128);
    INVALID(player.y, NAN);
    INVALID(backpack.capacity, 25);
    INVALID(backpack.n[ITEM_COUNT], 1);
    INVALID(pantry.n[STONE], 81);
    INVALID(health, 101);
    INVALID(stamina, -1);
    INVALID(rested, 121);
    INVALID(meal, 181);
    INVALID(time, -1);
    INVALID(pitch, 1.5);
    INVALID(speed, 26);
    INVALID(coins, -1);
    INVALID(room_count, 0);
    INVALID(room_count, 13);
    INVALID(mutation_count, 2049);
    INVALID(indoor, 2);
    INVALID(gate, 1);
    INVALID(eligible[STONE], 1);
    INVALID(rooms[0].parent, 0);
    INVALID(rooms[0].tier, 4);
    INVALID(rooms[0].furniture, 3);
    INVALID(rooms[0].cabinet.capacity, 12);
    INVALID(crops[0].room, 1);
    INVALID(crops[0].kind, 0);
    INVALID(crops[0].duration, NAN);
    INVALID(caches[0].active, 2);
    INVALID(caches[0].items[0], 41);
    INVALID(stairs.length1, NAN);
    INVALID(stairs.landing.y, 129);
    game.s = valid;
    game.s.caches[0].items[0] = 30;
    game.s.caches[0].items[1] = 30;
    CHECK(!wc_validate(&game.s));
    game.s = valid;
    game.s.caches[0].active = 1;
    game.s.caches[0].pos.x = -1;
    CHECK(!wc_validate(&game.s));
    game.s = valid;
    game.s.gate = game.s.stairs.deployed = 1;
    game.s.stairs.length1 = 0;
    CHECK(!wc_validate(&game.s));
    game.s = valid;
    game.s.mutation_count = 1;
    game.s.mutations[0] = (WcMutation){.id = 1, .ready = 2, .stock = 2};
    CHECK(wc_validate(&game.s));
    game.s.mutations[0].ready = NAN;
    CHECK(!wc_validate(&game.s));
    game.s.mutations[0].ready = 2;
    game.s.mutation_count = 2;
    game.s.mutations[1] = game.s.mutations[0];
    CHECK(!wc_validate(&game.s));
    game.s = valid;
    unsigned char *bytes = malloc(WC_SAVE_MAX);
    CHECK(bytes);
    size_t n = wc_encode(&valid, bytes, WC_SAVE_MAX);
    /* Recompute checksums after corrupting payloads: structural validation must reject them. */
    for (size_t at = 16; at < n; at += 8) {
        unsigned char old[8];
        memcpy(old, bytes + at, 8);
        memset(bytes + at, 255, 8);
        uint32_t hash = 2166136261u;
        for (size_t i = 16; i < n; i++)
            hash = (hash ^ bytes[i]) * 16777619u;
        for (int k = 0; k < 4; k++)
            bytes[12 + k] = (unsigned char)(hash >> (k * 8));
        WcState decoded = valid;
        bool ok = wc_decode(&decoded, bytes, n);
        if (ok)
            CHECK(wc_validate(&decoded));
        else
            CHECK(!memcmp(&decoded, &valid, sizeof(valid)));
        memcpy(bytes + at, old, 8);
    }
    free(bytes);
    game.s = valid;
#undef INVALID
}
static double plane(uint32_t seed, WcPos p, void *ctx) {
    (void)seed;
    WcVec gradient = *(WcVec *)ctx;
    WcVec d = wc_relative(p, wc_pos(0, 0, 0));
    return d.x * gradient.x + d.z * gradient.z + gradient.y;
}
static void test_slope_course(void) {
    for (int degrees = 20; degrees <= 45; degrees += degrees == 20 ? 15 : 10) {
        fresh();
        supplies();
        game.s.local = (WcVec){4, 0, 4};
        CHECK(command(CMD_BUILD, 0, 0, 4, 0));
        CHECK(command(CMD_BUILD, 1, 1, 8, 0));
        WcVec gradient = {tan(degrees * WC_PI / 180), 0, 0};
        game.s.castle = wc_pos(0, 40, 0);
        game.s.local = (WcVec){0, 0, -5};
        game.s.helm = 1;
        WcVec attached = game.s.local;
        double yaw = game.s.yaw;
        for (int i = 0; i < 1200; i++) {
            wc_pilot(&game,
                     (WcInput){.forward = (i / 300) % 2 ? -1 : 1, .side = .6f, .sprint = i % 2},
                     1.0 / 60, plane, &gradient);
            CHECK(!memcmp(&attached, &game.s.local, sizeof(attached)));
            CLOSE(yaw, game.s.yaw);
            CHECK(game.s.castle.y >=
                  wc_clearance_sample(&game.s, game.s.castle, game.s.castle_yaw, plane, &gradient) -
                      .001);
        }
    }
    fresh();
    WcStairs stairs;
    WcVec gradient = {0, 0, 0};
    /* Higher and lower landings, and a cross-slope that requires a sideways flight. */
    for (int h = -15; h <= 4; h++) {
        gradient.y = h;
        CHECK(wc_solve_stairs_sample(&game.s, &stairs, plane, &gradient));
        CHECK(stairs.length1 + stairs.length2 <= 48);
    }
    gradient = (WcVec){.5, 0, 0};
    CHECK(wc_solve_stairs_sample(&game.s, &stairs, plane, &gradient));
    CHECK(stairs.turned);
    game.s.gate = 1;
    game.s.stairs = stairs;
    int valid;
    WcVec mid = stairs.landing;
    mid.x = (stairs.bend.x + mid.x) * .5;
    mid.y = (stairs.bend.y + mid.y) * .5;
    CHECK(isfinite(wc_support(&game.s, mid, &valid)));
    CHECK(valid);
    gradient = (WcVec){0, 100, 0};
    CHECK(!wc_solve_stairs_sample(&game.s, &stairs, plane, &gradient));
    game.s.room_count = 12;
    CHECK(!wc_room_valid(&game.s, 0, 0, 1, 0, NULL, 0));
}
static void test_actual_stairs_and_upper_floor(void) {
    fresh();
    CHECK(command(CMD_GATE, 0, 0, 0, 0));
    game.s.local = (WcVec){0, 0, 6};
    game.s.yaw = WC_PI;
    tick(3.5, (WcInput){.forward = 1});
    CHECK(!game.s.indoor && game.s.player.y < .1);
    WcPos parked = game.s.castle;
    game.s.eligible[TIMBER] = game.s.backpack.n[TIMBER] = 1;
    game.s.yaw = 0;
    tick(3.5, (WcInput){.forward = 1});
    CHECK(game.s.indoor);
    CHECK(game.s.eligible[TIMBER] == 0);
    CHECK(!memcmp(&parked, &game.s.castle, sizeof(parked)));
    fresh();
    supplies();
    game.s.local = (WcVec){4, 0, 4};
    CHECK(command(CMD_BUILD, 0, 0, 4, 0));
    CHECK(command(CMD_BUILD, 1, 1, 8, 0));
    game.s.local = (WcVec){0, 0, 3.1};
    game.s.yaw = 0;
    tick(1.8, (WcInput){.forward = 1});
    CHECK(game.s.local.y > 3.7);
    int valid;
    CHECK(wc_support(&game.s, (WcVec){-6, 4, 0}, &valid) == 4 && valid);
    game.s.yaw = WC_PI;
    tick(1.8, (WcInput){.forward = 1});
    CHECK(game.s.local.y < .4);
}
static void test_hazards_and_boundaries(void) {
    fresh();
    outside(-30, 31);
    game.s.time = 1.25;
    tick(.12, (WcInput){0});
    CHECK(game.s.health < 100);
    game.s.foundry_trap = 1;
    float hp = game.s.health;
    tick(.1, (WcInput){0});
    CLOSE(hp, game.s.health);
    outside(-30, 25);
    game.s.time = 2.6;
    tick(.12, (WcInput){0});
    CHECK(game.s.health < hp);
    outside(-30, 21.5);
    game.s.time = 1.42;
    game.s.health = 1;
    tick(.1, (WcInput){0});
    CHECK(game.s.indoor && game.s.health == 100);
    fresh();
    wc_step(&game, (WcInput){.forward = 1, .side = 1}, 1);
    CHECK(game.s.time == .05);
    outside(300, 300);
    WcScene scene;
    wc_scene(&game.s, &scene);
    for (int i = 0; i < scene.count; i++)
        if (scene.f[i].type == FEATURE_PROWLER) {
            game.s.player = scene.f[i].p;
            game.s.time = 4.45 - fmod((scene.f[i].id % 31) * .1, 3);
            hp = game.s.health;
            wc_step(&game, (WcInput){0}, .01);
            CHECK(game.s.health < hp);
            game.s.attack_cd = 0;
            CHECK(command(CMD_WARD, 0, 0, 0, 0));
            wc_step(&game, (WcInput){0}, .01);
            break;
        }
    fresh();
    outside(-30, 22);
    game.s.time = 2;
    game.s.yaw = WC_PI;
    reject(CMD_ATTACK, 0, 0, 0, 2);
    game.s.yaw = 0;
    reject(CMD_WARD, 0, 0, 0, 2);
    game.s.time = 0;
    reject(CMD_ATTACK, 0, 0, 0, 2);
    outside(10, 10);
    reject(CMD_ATTACK, 0, 0, 0, 2);
    reject(CMD_ATTACK, 0, 0, 0, 10);
    game.s.mutation_count = WC_MUTATIONS;
    outside(20, 0);
    reject(CMD_GATHER, 0, 0, 0, 10);
    wc_step(&game, (WcInput){.forward = 1}, .01);
    CHECK(game.s.storage_failed);
    fresh();
    outside(17, 20);
    game.s.coins = 100;
    CHECK(command(CMD_BUY, SATCHEL, 1, 0, 0));
    CHECK(game.s.satchel);
    reject(CMD_BUY, SATCHEL, 1, 0, 0);
    outside(0, 0);
    reject(CMD_HEARTH, 0, 0, 0, 0);
}
static void test_complete_room_graph(void) {
    fresh();
    supplies();
    game.s.local = (WcVec){4, 0, 4};
    CHECK(command(CMD_BUILD, 0, 0, 4, 0));
    CHECK(command(CMD_BUILD, 1, 0, 4, 0));
    CHECK(command(CMD_BUILD, 2, 1, 8, 0));
    CHECK(command(CMD_BUILD, 3, 1, 8, 0));
    CHECK(wc_room_center(&game.s.rooms[4]).x == -15);
    CHECK(wc_room_dimensions(ROOM_HALL, 1).x == 16);
    while (game.s.room_count < 12) {
        int built = 0;
        for (int parent = 0; parent < game.s.room_count && !built; parent++)
            for (int dir = 0; dir <= 4 && !built; dir++) {
                int kind = dir == 0 ? ROOM_HALL : ROOM_ANNEX;
                if (wc_room_valid(&game.s, parent, dir, kind, 0, NULL, 0)) {
                    memset(game.s.pantry.n, 0, sizeof(game.s.pantry.n));
                    game.s.pantry.n[PLANK] = 6;
                    game.s.pantry.n[TWINE] = 2;
                    game.s.pantry.n[STONE] = 4;
                    CHECK(command(CMD_BUILD, parent, dir, kind * 4, 0));
                    CHECK(wc_validate(&game.s));
                    built = 1;
                }
            }
        CHECK(built);
    }
    reject(CMD_BUILD, 0, 0, 4, 0);
    game.s.local = (WcVec){-15, 8, 0};
    game.s.player = wc_local_world(&game.s, game.s.local);
    unsigned char *bytes = malloc(WC_SAVE_MAX);
    CHECK(bytes);
    size_t n = wc_encode(&game.s, bytes, WC_SAVE_MAX);
    CHECK(n > 0);
    CHECK(wc_decode(&game.s, bytes, n));
    tick(.1, (WcInput){0});
    CLOSE(game.s.local.y, 8);
    CHECK(game.s.room_count == 12);
    game.s.rooms[2].parent = 3;
    CHECK(!wc_validate(&game.s));
    free(bytes);
    fresh();
    wc_look(&game.s, NAN, 1);
    CLOSE(game.s.yaw, 0);
    CLOSE(game.s.pitch, 0);
}
static void test_destinations(void) {
    fresh();
    int families = 0;
    for (int z = -4; z <= 4; z++)
        for (int x = -4; x <= 4; x++) {
            outside(x * 128 + 64, z * 128 + 64);
            WcScene scene;
            wc_scene(&game.s, &scene);
            int enemies = 0;
            for (int i = 0; i < scene.count; i++) {
                WcFeature f = scene.f[i];
                if (f.type == FEATURE_PROWLER)
                    enemies++;
                if (f.type >= FEATURE_KEEP && f.type <= FEATURE_HAMLET) {
                    families |= 1 << (f.type - FEATURE_KEEP);
                    const char *name = wc_destination_name(f.id, f.type);
                    CHECK(strlen(name) > 5);
                    WcPos offset = f.p;
                    wc_pos_add(&offset, 4, 0, 4);
                    CLOSE(wc_height(game.s.seed, offset), f.p.y);
                    WcBox boxes[64];
                    CHECK(wc_site_boxes(f.type, boxes, 64) > 0);
                }
            }
            CHECK(enemies <= 12 && scene.chunks <= 32 && scene.count <= 96);
        }
    CHECK(families == 7);
    CHECK(strlen(wc_destination_name(0, 0)) > 0);
}
static void test_content_failures(void) {
    const char *cases[] = {"return {}", "return 3", "not Lua !", "return {items={}}",
                           "error('broken content')"};
    for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); i++) {
        FILE *file = fopen("wc-invalid-fixture.lua", "w");
        CHECK(file);
        fputs(cases[i], file);
        CHECK(!fclose(file));
        WcDefs copy = defs;
        char error[160];
        CHECK(!wc_load_content(&copy, "wc-invalid-fixture.lua", error, sizeof(error)));
        CHECK(!memcmp(&copy, &defs, sizeof(defs)));
        WcArt art = {0};
        CHECK(!wc_load_art(&art, "wc-invalid-fixture.lua", error, sizeof(error)));
        CHECK(strlen(error) > 0);
    }
    CHECK(!remove("wc-invalid-fixture.lua"));
}
int main(int argc, char **argv) {
    char error[160];
    CHECK(argc > 1);
    CHECK(wc_load_content(&defs, argv[1], error, sizeof(error)));
    test_inventory();
    test_garden();
    test_build();
    test_progress();
    test_world();
    test_movement();
    test_expedition();
    test_combat();
    test_save();
    test_rays();
    test_transaction_fuzz();
    test_save_validation();
    test_slope_course();
    test_actual_stairs_and_upper_floor();
    test_hazards_and_boundaries();
    test_complete_room_graph();
    test_destinations();
    test_content_failures();
    printf("%d checks passed.\n", checks);
    return 0;
}
