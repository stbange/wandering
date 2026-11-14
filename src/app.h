#ifndef WC_APP_H
#define WC_APP_H
#include "content.h"
#include "wc.h"
#include <SDL3/SDL.h>

enum {
    PANEL_NONE,
    PANEL_TITLE,
    PANEL_PAUSE,
    PANEL_INVENTORY,
    PANEL_WORKSHOP,
    PANEL_STORAGE,
    PANEL_GARDEN,
    PANEL_BED,
    PANEL_TRADE,
    PANEL_TECH,
    PANEL_BUILD,
    PANEL_FURNITURE,
    PANEL_NOTES,
    PANEL_NEW
};
typedef struct {
    float x, y, w, h;
    int action, a, b, c;
    char label[140];
} WcButton;
typedef struct {
    WcGame game;
    WcArt art;
    SDL_Window *window;
    SDL_GLContext context;
    int running, panel, help, focused, captured, width, height, selection, storage, garden;
    bool keys[SDL_SCANCODE_COUNT];
    int build_parent, build_direction, build_rotation, build_kind, trade_item;
    int touch, jump, attack_flash, button_count, menu_focus, frame_count;
    WcButton buttons[64];
    WcHit hit;
    WcScene scene;
    float touch_forward, touch_side;
    double previous, accumulator;
    char asset_dir[512], save_path[512];
    SDL_AudioStream *audio;
    unsigned char *sound;
    Uint32 sound_length;
    SDL_AudioSpec sound_spec;
    int xr, xr_eye;
    double xr_base_yaw;
    float xr_projection[16], xr_view[16];
    int test_frames;
    char capture_path[512];
} WcApp;
enum {
    UI_RESUME = 100,
    UI_PANEL,
    UI_COMMAND,
    UI_SAVE,
    UI_LOAD,
    UI_QUIT,
    UI_NEW,
    UI_CONFIRM_NEW,
    UI_HELP,
    UI_RECIPE,
    UI_DEPOSIT,
    UI_WITHDRAW,
    UI_PLANT,
    UI_HARVEST,
    UI_TEND,
    UI_BUY,
    UI_SELL,
    UI_BUILD,
    UI_BUILD_DIR,
    UI_BUILD_KIND,
    UI_BUILD_PARENT,
    UI_ROTATE,
    UI_REPLACE
};
void wc_panel(WcApp *a, int panel);
void wc_action(WcApp *a, int action, int x, int y, int z);
void wc_interact(WcApp *a);
bool wc_platform_save(WcApp *a);
bool wc_platform_load(WcApp *a);
void wc_platform_sound(WcApp *a);
bool wc_render_init(WcApp *a);
void wc_render(WcApp *a);
void wc_render_destroy(void);
bool wc_capture(WcApp *a, const char *path);
void wc_input_reset(WcApp *a);
#endif
