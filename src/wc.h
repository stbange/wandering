#ifndef WC_H
#define WC_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WC_VERSION 1
#define WC_GEN_VERSION 1
#define WC_CONTENT_VERSION 1
#define WC_ITEMS 24
#define WC_RECIPES 12
#define WC_ROOMS 12
#define WC_CROPS 24
#define WC_MUTATIONS 2048
#define WC_CACHES 16
#define WC_CHUNKS 32
#define WC_ENEMIES 12
#define WC_PI 3.14159265358979323846
#define WC_CHUNK 128.0
#define WC_SAVE_MAX 262144

enum {
    TIMBER,
    STONE,
    FIBER,
    SUN_SEED,
    MINT_SEED,
    PLANK,
    TWINE,
    SUNROOT,
    MINT,
    COMPOST,
    SATCHEL,
    WEDGE,
    ALLOY,
    STEW,
    PACK_CABINET,
    PACK_COMPACT,
    ITEM_COUNT
};
enum { SECOND_WIND = 1, POWER_STRIKE = 2, CAREFUL_TENDING = 4 };
enum { ROOM_HOME, ROOM_HALL, ROOM_ANNEX };
enum { FURN_CABINET, FURN_COMPACT };
typedef struct {
    double x, y, z;
} WcVec;
typedef struct {
    int64_t cx, cz;
    double x, y, z;
} WcPos;
typedef struct {
    int n[WC_ITEMS], capacity;
} WcInventory;
typedef struct {
    char name[32];
    int buy, sell;
} WcItem;
typedef struct {
    char name[48];
    int cost[WC_ITEMS], output, quantity, unlock;
} WcRecipe;
typedef struct {
    int damage, power;
    float armor;
} WcGear;
typedef struct {
    WcItem items[WC_ITEMS];
    WcRecipe recipes[WC_RECIPES];
    int recipe_count;
    double growth[2];
    int yield[2];
    float speed, turbo, turn;
    WcGear staff, spear;
    int room_cost[WC_ITEMS], salvage[WC_ITEMS], reserve[WC_ITEMS];
} WcDefs;
typedef struct {
    int id, parent, kind, tier, gx, gz, rotation, furniture;
    WcInventory cabinet;
} WcRoom;
typedef struct {
    int room, kind, treated;
    double planted, duration;
} WcCrop;
typedef struct {
    uint64_t id;
    double ready;
    int flags, stock;
} WcMutation;
typedef struct {
    uint64_t id;
    WcPos pos;
    int items[WC_ITEMS], active;
} WcCache;
typedef struct {
    int deployed, turned;
    double length1, length2, width;
    WcVec bend, landing;
} WcStairs;
typedef struct {
    uint32_t seed, generation, content;
    double time;
    WcPos castle, player;
    WcVec local;
    double castle_yaw, yaw, pitch, vertical, speed, attack_cd, recovery;
    float health, stamina, rested, meal;
    int indoor, helm, grounded, exhausted, shift_released, gate;
    int coins, xp, points, awarded, techniques, satchel, hearth, tempered, spear, hoe;
    int wedge_recipe, foundry_notes, foundry_trap, foundry_brake, foundry_reserve;
    int storage_failed, room_count, mutation_count, cache_serial;
    WcInventory backpack, pantry;
    WcRoom rooms[WC_ROOMS];
    WcCrop crops[WC_CROPS];
    WcMutation mutations[WC_MUTATIONS];
    WcCache caches[WC_CACHES];
    int eligible[WC_ITEMS];
    uint32_t achievements;
    WcStairs stairs;
} WcState;
typedef struct {
    WcState s;
    WcDefs defs;
    int paused;
    char message[160];
} WcGame;
typedef struct {
    float forward, side, turn;
    int jump, sprint;
} WcInput;
/* Terrain providers let authored destinations and test courses share locomotion. */
typedef double (*WcHeightFn)(uint32_t seed, WcPos position, void *context);
typedef struct {
    WcPos p;
    uint64_t id;
    int type, power;
    double phase;
} WcFeature;
typedef struct {
    WcFeature f[96];
    int count, chunks;
} WcScene;
typedef enum {
    CMD_CRAFT,
    CMD_PLANT,
    CMD_TEND,
    CMD_HARVEST,
    CMD_GATHER,
    CMD_DEPOSIT,
    CMD_WITHDRAW,
    CMD_BUY,
    CMD_SELL,
    CMD_BUILD,
    CMD_REPLACE,
    CMD_REST,
    CMD_EAT,
    CMD_LEARN,
    CMD_SECOND_WIND,
    CMD_ATTACK,
    CMD_POWER,
    CMD_WARD,
    CMD_HEARTH,
    CMD_TEMPER,
    CMD_SPEAR,
    CMD_NOTES,
    CMD_WEDGE,
    CMD_BRAKE,
    CMD_SALVAGE,
    CMD_RESERVE,
    CMD_RECOVER,
    CMD_GATE,
    CMD_HELM,
    CMD_CLUE
} WcCommandType;
typedef struct {
    WcCommandType type;
    int a, b, c;
    uint64_t target;
    WcPos pos;
} WcCommand;
enum {
    FEATURE_TREE = 1,
    FEATURE_TIMBER,
    FEATURE_STONE,
    FEATURE_FIBER,
    FEATURE_SEED,
    FEATURE_TRADER,
    FEATURE_FOUNDRY,
    FEATURE_PROWLER,
    FEATURE_KEEP,
    FEATURE_MINE,
    FEATURE_HAMLET,
    FEATURE_CLUE
};
typedef struct {
    double distance;
    int kind, index;
    uint64_t id;
    WcPos pos;
} WcHit;
typedef struct {
    WcVec center, half;
    int material;
} WcBox;
enum {
    HIT_NONE,
    HIT_HELM,
    HIT_GATE,
    HIT_BELL,
    HIT_BED,
    HIT_PANTRY,
    HIT_WORKSHOP,
    HIT_PLANTER,
    HIT_CABINET,
    HIT_HEARTH,
    HIT_RESOURCE,
    HIT_TRADER,
    HIT_NOTES,
    HIT_PLATE,
    HIT_BRAKE,
    HIT_SALVAGE,
    HIT_RESERVE,
    HIT_PROWLER,
    HIT_CLUE,
    HIT_CACHE,
    HIT_SOCKET
};

uint64_t wc_hash(uint64_t x);
WcPos wc_pos(double x, double y, double z);
void wc_pos_add(WcPos *p, double x, double y, double z);
WcVec wc_relative(WcPos a, WcPos origin);
WcPos wc_local_world(const WcState *s, WcVec local);
WcVec wc_world_local(const WcState *s, WcPos p);
double wc_height(uint32_t seed, WcPos p);
WcVec wc_room_dimensions(int kind, int rotation);
WcVec wc_room_center(const WcRoom *room);
const char *wc_destination_name(uint64_t id, int type);
int wc_biome(uint32_t seed, WcPos p);
void wc_scene(const WcState *s, WcScene *scene);
double wc_clearance(const WcState *s, WcPos at, double angle);
double wc_clearance_sample(const WcState *s, WcPos at, double angle, WcHeightFn height,
                           void *context);
void wc_pilot(WcGame *g, WcInput input, double dt, WcHeightFn height, void *context);
bool wc_castle_blocked(const WcState *s, WcPos at, double angle);
int wc_site_boxes(int type, WcBox *boxes, int capacity);
bool wc_outdoor_blocked(const WcState *s, WcPos p);
bool wc_solve_stairs(const WcState *s, WcStairs *out);
bool wc_solve_stairs_sample(const WcState *s, WcStairs *out, WcHeightFn height, void *context);
double wc_support(const WcState *s, WcVec local, int *valid);
WcHit wc_raycast(const WcState *s, const WcScene *scene, double max_range);
void wc_look(WcState *s, double dx, double dy);
void wc_init(WcGame *g, uint32_t seed);
void wc_step(WcGame *g, WcInput in, double dt);
bool wc_command(WcGame *g, WcCommand cmd);
void wc_defeat(WcGame *g);
int wc_inventory_used(const WcInventory *inv);
int wc_available(const WcState *s, int item);
bool wc_add(WcInventory *inv, int item, int quantity);
bool wc_pay(WcState *s, const int cost[WC_ITEMS], int home);
void wc_xp(WcState *s, int amount, uint32_t achievement);
bool wc_room_valid(const WcState *s, int parent, int direction, int kind, int rotation, char *why,
                   size_t size);
bool wc_validate(const WcState *s);
size_t wc_encode(const WcState *s, unsigned char *out, size_t capacity);
bool wc_decode(WcState *out, const unsigned char *bytes, size_t length);
#endif
