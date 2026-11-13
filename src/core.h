#ifndef WC_CORE_H
#define WC_CORE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WC_VERSION 1u
#define WC_DT (1.0 / 120.0)
#define WC_PI 3.14159265358979323846
#define WC_MAX_OBJECTS 64
#define WC_ITEMS 9
#define WC_MESSAGE 160
typedef enum { WC_TIMBER, WC_STONE, WC_FIBER, WC_SEED, WC_MINT_SEED,
               WC_PLANK, WC_TWINE, WC_SUNROOT, WC_MINT } WCItem;
typedef enum { WC_FOOT, WC_HELM, WC_BUILD, WC_MENU } WCContext;
typedef enum { WC_NONE, WC_HELM_OBJECT, WC_BED, WC_PANTRY, WC_WORKSHOP,
               WC_PLANTER, WC_GATE, WC_MAP } WCKind;
typedef enum { WC_OK, WC_INVALID, WC_CAPACITY, WC_SUPPLIES, WC_UNAVAILABLE,
               WC_BLOCKED, WC_TOO_FAR } WCResult;
typedef struct { double x,y,z; } WCVec;
typedef struct {
    int id, kind, slot;
    WCVec min,max;
    float color[3];
    bool solid;
} WCObject;
typedef struct {
    double speed,turbo,turn,walk,sprint,jump,gravity,eye;
    WCObject objects[WC_MAX_OBJECTS];
    int object_count;
} WCConfig;
typedef struct { int items[WC_ITEMS], capacity; } WCInventory;
typedef struct { int crop; double planted,duration; bool treated; } WCCrop;
typedef struct {
    uint32_t seed;
    double clock;
    WCVec castle, player;
    double heading,yaw,pitch,vertical,speed;
    bool attached,grounded,gate_open,focused,help,sprint_latched;
    double stair_length,stair_drop;
    WCContext context, prior_context;
    int panel,selection;
    double health,stamina,rested,meal,recovery;
    WCInventory pack,pantry;
    WCCrop crops[2];
    double depleted[16];
    unsigned knowledge,coins,claims;
    bool satchel;
    char message[WC_MESSAGE];
} WCState;
typedef struct { double forward,strafe,look_x,look_y; bool jump,turbo; } WCInput;
typedef double (*WCSurface)(double x,double z,void *user);

void wc_defaults(WCConfig *c);
void wc_init(WCState *s,const WCConfig *c,uint32_t seed);
void wc_look(WCState *s,double dx,double dy);
void wc_pause(WCState *s,bool pause);
void wc_focus(WCState *s,bool focused);
bool wc_inside(const WCState *s);
WCVec wc_world_player(const WCState *s);
WCVec wc_local(const WCState *s,WCVec world);
double wc_terrain(double x,double z,void *seed);
void wc_tick(WCState *s,const WCConfig *c,WCInput in,double dt,WCSurface ground,void *user);
double wc_ray_box(WCVec origin,WCVec direction,WCVec min,WCVec max,double limit);
int wc_target(const WCState *s,const WCConfig *c,double *distance);
WCResult wc_interact(WCState *s,const WCConfig *c);
WCResult wc_gate(WCState *s,WCSurface ground,void *user);
WCResult wc_rest(WCState *s);
int wc_used(const WCInventory *i);
int wc_available(const WCState *s,int item);
WCResult wc_exchange(WCState *s,const int cost[WC_ITEMS],const int reward[WC_ITEMS],bool shared);
WCResult wc_craft(WCState *s,int recipe);
WCResult wc_transfer(WCState *s,int item,int count,bool deposit);
WCResult wc_garden(WCState *s,int slot,int action);
WCResult wc_gather(WCState *s,int site);
const char *wc_item_name(int item);
const char *wc_result_text(WCResult result);
bool wc_validate(const WCState *s);
#endif
