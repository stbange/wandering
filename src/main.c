#include "app.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <emscripten.h>
#define EXPORT EMSCRIPTEN_KEEPALIVE
EM_JS(void, web_ready, (), {
    document.getElementById('loading').hidden = true;
    window.wanderingReady = true;
    window.dispatchEvent(new Event('wandering-ready'));
});
#else
#define EXPORT
#endif

static WcApp app;
static void frame(void);
void wc_input_reset(WcApp *a) {
    a->touch_forward = a->touch_side = 0;
    a->jump = 0;
    memset(a->keys, 0, sizeof(a->keys));
}
void wc_panel(WcApp *a, int panel) {
    a->panel = panel;
    a->game.paused = panel != PANEL_NONE || !a->focused;
    a->accumulator = 0;
    a->menu_focus = 1;
    wc_input_reset(a);
    bool capture = panel == PANEL_NONE && a->focused && !a->touch;
    SDL_SetWindowRelativeMouseMode(a->window, capture);
    a->captured = capture;
    if (panel == PANEL_NONE && a->game.s.helm)
        snprintf(a->game.message, sizeof(a->game.message), "Helm ready. WASD steer; E release.");
}
static void perform(WcApp *a, WcCommand command) {
    if (wc_command(&a->game, command))
        wc_platform_sound(a);
    else
        a->attack_flash = 0;
}
void wc_action(WcApp *a, int action, int x, int y, int z) {
    WcState *s = &a->game.s;
    switch (action) {
    case UI_RESUME:
        wc_panel(a, PANEL_NONE);
        break;
    case UI_HELP:
        a->help = !a->help;
        break;
    case UI_PANEL:
        if (x == PANEL_STORAGE)
            a->selection = y;
        if (x == PANEL_TRADE)
            a->trade_item = (a->trade_item + y + ITEM_COUNT) % ITEM_COUNT;
        wc_panel(a, x);
        break;
    case UI_COMMAND:
        perform(a, (WcCommand){.type = (WcCommandType)x, .a = y, .b = z});
        break;
    case UI_SAVE:
        wc_platform_save(a);
        break;
    case UI_LOAD:
        wc_platform_load(a);
        break;
    case UI_QUIT:
        a->running = 0;
        break;
    case UI_NEW:
        wc_panel(a, PANEL_NEW);
        break;
    case UI_CONFIRM_NEW: {
        uint32_t seed = (uint32_t)wc_hash(SDL_GetPerformanceCounter() ^ (uint64_t)time(NULL));
        wc_init(&a->game, seed);
        wc_panel(a, PANEL_NONE);
        break;
    }
    case UI_RECIPE:
        a->selection = x;
        perform(a, (WcCommand){.type = CMD_CRAFT, .a = x});
        break;
    case UI_DEPOSIT:
    case UI_WITHDRAW:
        perform(a, (WcCommand){.type = action == UI_DEPOSIT ? CMD_DEPOSIT : CMD_WITHDRAW,
                               .a = x,
                               .b = y,
                               .c = a->storage});
        break;
    case UI_PLANT:
        perform(a, (WcCommand){.type = CMD_PLANT, .a = x, .b = y});
        break;
    case UI_HARVEST:
        perform(a, (WcCommand){.type = CMD_HARVEST, .a = x});
        break;
    case UI_TEND:
        perform(a, (WcCommand){.type = CMD_TEND, .a = x});
        break;
    case UI_BUY:
    case UI_SELL:
        perform(a, (WcCommand){.type = action == UI_BUY ? CMD_BUY : CMD_SELL, .a = x, .b = y});
        break;
    case UI_BUILD:
        perform(a, (WcCommand){.type = CMD_BUILD,
                               .a = a->build_parent,
                               .b = a->build_direction,
                               .c = a->build_kind * 4 + a->build_rotation});
        break;
    case UI_BUILD_DIR:
        a->build_direction = (a->build_direction + 1) % 5;
        break;
    case UI_BUILD_KIND:
        a->build_kind = a->build_kind == ROOM_HALL ? ROOM_ANNEX : ROOM_HALL;
        break;
    case UI_BUILD_PARENT:
        a->build_parent = (a->build_parent + 1) % s->room_count;
        break;
    case UI_ROTATE:
        a->build_rotation = (a->build_rotation + 1) % 4;
        break;
    case UI_REPLACE:
        perform(a, (WcCommand){.type = CMD_REPLACE, .a = a->storage, .b = x});
        break;
    default:
        break;
    }
}
void wc_interact(WcApp *a) {
    if (a->panel && a->xr && a->menu_focus < a->button_count) {
        WcButton b = a->buttons[a->menu_focus];
        wc_action(a, b.action, b.a, b.b, b.c);
        return;
    }
    if (a->panel)
        return;
    if (a->game.s.helm) {
        perform(a, (WcCommand){.type = CMD_HELM});
        return;
    }
    WcHit h = a->hit;
    WcCommand cmd = {.target = h.id};
    int run = 1;
    switch (h.kind) {
    case HIT_HELM:
        cmd.type = CMD_HELM;
        break;
    case HIT_GATE:
    case HIT_BELL:
        cmd.type = CMD_GATE;
        break;
    case HIT_BED:
        wc_panel(a, PANEL_BED);
        run = 0;
        break;
    case HIT_PANTRY:
        a->storage = 0;
        wc_panel(a, PANEL_STORAGE);
        run = 0;
        break;
    case HIT_CABINET:
        a->storage = h.index + 1;
        wc_panel(a, PANEL_STORAGE);
        run = 0;
        break;
    case HIT_WORKSHOP:
    case HIT_HEARTH:
        wc_panel(a, PANEL_WORKSHOP);
        run = 0;
        break;
    case HIT_PLANTER:
        a->garden = h.index;
        wc_panel(a, PANEL_GARDEN);
        run = 0;
        break;
    case HIT_TRADER:
        wc_panel(a, PANEL_TRADE);
        run = 0;
        break;
    case HIT_RESOURCE:
        cmd.type = CMD_GATHER;
        break;
    case HIT_NOTES:
        cmd.type = CMD_NOTES;
        perform(a, cmd);
        wc_panel(a, PANEL_NOTES);
        run = 0;
        break;
    case HIT_PLATE:
        cmd.type = CMD_WEDGE;
        break;
    case HIT_BRAKE:
        cmd.type = CMD_BRAKE;
        break;
    case HIT_SALVAGE:
        cmd.type = CMD_SALVAGE;
        break;
    case HIT_RESERVE:
        cmd.type = CMD_RESERVE;
        break;
    case HIT_CLUE:
        cmd.type = CMD_CLUE;
        break;
    case HIT_CACHE:
        cmd.type = CMD_RECOVER;
        cmd.a = h.index;
        break;
    default:
        run = 0;
        break;
    }
    if (run)
        perform(a, cmd);
}
static void strike(WcApp *a, int power) {
    if (a->panel)
        return;
    perform(a, (WcCommand){.type = power ? CMD_POWER : CMD_ATTACK, .target = a->hit.id});
    a->attack_flash = 8;
}
EXPORT void wc_web_action(int action, int x, int y, int z) {
    WcApp *a = &app;
    if (action == -1)
        wc_interact(a);
    else if (action == -2)
        a->jump = 1;
    else if (action == -3)
        strike(a, 0);
    else if (action == -4)
        wc_panel(a, a->panel ? PANEL_NONE : PANEL_PAUSE);
    else if (action == -5)
        wc_panel(a, a->panel ? PANEL_NONE : PANEL_INVENTORY);
    else if (action == -6)
        a->touch = 1;
    else if (action == -7)
        a->keys[SDL_SCANCODE_LSHIFT] = (x != 0);
    else if (action == -9 && a->panel && a->button_count)
        a->menu_focus = (a->menu_focus + 1) % a->button_count;
    else if (action == -99) {
        a->game.s.storage_failed = 1;
        snprintf(a->game.message, sizeof(a->game.message),
                 "Browser storage failed. Exploration halted; export your save before closing.");
    } else
        wc_action(a, action, x, y, z);
}
EXPORT void wc_web_look(float x, float y) {
    if (!app.panel)
        wc_look(&app.game.s, x, y);
}
EXPORT void wc_web_move(float f, float side) {
    app.touch_forward = f;
    app.touch_side = side;
}
EXPORT const char *wc_web_snapshot(void) {
    static char json[16384];
    WcState *s = &app.game.s;
    char escaped[320];
    size_t k = 0;
    for (const char *p = app.game.message; *p && k < sizeof(escaped) - 3; p++) {
        if (*p == '"' || *p == '\\')
            escaped[k++] = '\\';
        if ((unsigned char)*p >= 32)
            escaped[k++] = *p;
    }
    escaped[k] = 0;
    int n = snprintf(
        json, sizeof(json),
        "{\"frame\":%d,\"panel\":%d,\"indoor\":%d,\"helm\":%d,\"gate\":%d,\"time\":%.5f,\"yaw\":%."
        "8f,\"pitch\":%"
        ".8f,\"health\":%.2f,\"stamina\":%.2f,\"seed\":%u,\"rooms\":%d,\"speed\":%.2f,\"x\":%.4f,"
        "\"y\":%.4f,\"z\":%.4f,\"cx\":%.4f,\"cz\":%.4f,\"help\":%d,\"hit\":%d,\"hitId\":\"%llu\","
        "\"coins\":%d,\"bag\":%d,\"message\":\"%s\",\"items\":[",
        app.frame_count, app.panel, s->indoor, s->helm, s->gate, s->time, s->yaw, s->pitch,
        s->health, s->stamina, s->seed, s->room_count, s->speed,
        s->indoor ? s->local.x : s->player.cx * 128.0 + s->player.x,
        s->indoor ? s->local.y : s->player.y,
        s->indoor ? s->local.z : s->player.cz * 128.0 + s->player.z,
        s->castle.cx * 128.0 + s->castle.x, s->castle.cz * 128.0 + s->castle.z, app.help,
        app.hit.kind, (unsigned long long)app.hit.id, s->coins, wc_inventory_used(&s->backpack),
        escaped);
    for (int i = 0; i < ITEM_COUNT; i++)
        n += snprintf(json + n, sizeof(json) - n, "%s%d", i ? "," : "", s->backpack.n[i]);
    n += snprintf(json + n, sizeof(json) - n, "],\"buttons\":[");
    for (int i = 0; i < app.button_count && n < (int)sizeof(json) - 250; i++) {
        WcButton *b = &app.buttons[i];
        n += snprintf(json + n, sizeof(json) - n,
                      "%s{\"label\":\"%s\",\"x\":%.1f,\"y\":%.1f,\"w\":%.1f,\"h\":%.1f}",
                      i ? "," : "", b->label, b->x, b->y, b->w, b->h);
    }
    snprintf(json + n, sizeof(json) - n, "]}");
    return json;
}
#if !defined(__EMSCRIPTEN__) || WC_WEBXR
EXPORT void wc_xr_view(float *projection, float *view, int x, int y, int w, int h) {
#ifdef __EMSCRIPTEN__
    if (!app.xr) {
        app.xr_base_yaw = app.game.s.yaw;
        app.xr = 1;
    }
    app.game.s.yaw = remainder(app.xr_base_yaw + atan2(-view[2], view[10]), WC_PI * 2);
    app.game.s.pitch = asin(fmax(-1, fmin(1, -view[6])));
    memcpy(app.xr_projection, projection, 64);
    memcpy(app.xr_view, view, 64);
    app.width = w;
    app.height = h;
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, w, h);
    glViewport(x, y, w, h);
    app.hit = wc_raycast(&app.game.s, &app.scene, 3.5);
    wc_render(&app);
    glDisable(GL_SCISSOR_TEST);
#else
    (void)projection;
    (void)view;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
#endif
}
EXPORT void wc_xr_frame(void) {
    if (!app.xr) {
        app.xr_base_yaw = app.game.s.yaw;
        app.xr = 1;
    }
    frame();
}
EXPORT void wc_xr_end(void) {
    app.xr = 0;
    wc_panel(&app, PANEL_PAUSE);
}
#endif
static void key(WcApp *a, SDL_Keycode k) {
    if (k == SDLK_F1 || (k == SDLK_TAB && !a->panel)) {
        a->help = !a->help;
        return;
    }
    if (k == SDLK_F5) {
        wc_platform_save(a);
        return;
    }
    if (k == SDLK_F9) {
        wc_platform_load(a);
        return;
    }
    if (k == SDLK_ESCAPE) {
        wc_panel(a, a->panel ? PANEL_NONE : PANEL_PAUSE);
        return;
    }
    if (a->panel) {
        if (k == SDLK_TAB || k == SDLK_DOWN)
            a->menu_focus = (a->menu_focus + 1) % a->button_count;
        if (k == SDLK_UP)
            a->menu_focus = (a->menu_focus - 1 + a->button_count) % a->button_count;
        if (k == SDLK_RETURN && a->menu_focus < a->button_count) {
            WcButton b = a->buttons[a->menu_focus];
            wc_action(a, b.action, b.a, b.b, b.c);
        }
        if (k == SDLK_R && a->panel == PANEL_BUILD)
            wc_action(a, UI_ROTATE, 0, 0, 0);
        return;
    }
    if (k == SDLK_E)
        wc_interact(a);
    if (k == SDLK_SPACE)
        a->jump = 1;
    if (k == SDLK_I)
        wc_panel(a, PANEL_INVENTORY);
    if (k == SDLK_K)
        wc_panel(a, PANEL_TECH);
    if (k == SDLK_B) {
        if (a->game.s.indoor && !a->game.s.helm)
            wc_panel(a, PANEL_BUILD);
        else
            snprintf(a->game.message, sizeof(a->game.message),
                     "Park and build from inside your home.");
    }
    if (k == SDLK_F) {
        if (a->hit.kind == HIT_CABINET) {
            a->storage = a->hit.index;
            wc_panel(a, PANEL_FURNITURE);
        } else
            snprintf(a->game.message, sizeof(a->game.message),
                     "Aim at an eligible room cabinet to replace its furnishing.");
    }
    if (k == SDLK_X)
        strike(a, 0);
    if (k == SDLK_C)
        strike(a, 1);
    if (k == SDLK_Q)
        perform(a, (WcCommand){.type = CMD_WARD, .target = a->hit.id});
    if (k == SDLK_V)
        perform(a, (WcCommand){.type = CMD_SECOND_WIND});
}
static void frame(void) {
    WcApp *a = &app;
    double now = SDL_GetTicks() / 1000.0, dt = now - a->previous;
    a->previous = now;
    if (dt > .15)
        dt = .15;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT)
            a->running = 0;
        if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
            a->focused = 0;
            if (a->panel == PANEL_NONE)
                wc_panel(a, PANEL_PAUSE);
            else
                wc_panel(a, a->panel);
        }
        if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
            a->focused = 1;
            wc_input_reset(a);
        }
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.scancode < SDL_SCANCODE_COUNT)
                a->keys[event.key.scancode] = true;
            if (!event.key.repeat)
                key(a, event.key.key);
        }
        if (event.type == SDL_EVENT_KEY_UP && event.key.scancode < SDL_SCANCODE_COUNT)
            a->keys[event.key.scancode] = false;
        if (event.type == SDL_EVENT_MOUSE_MOTION && !a->panel && a->captured)
            wc_look(&a->game.s, event.motion.xrel * .0028, -event.motion.yrel * .0028);
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
            float x = event.button.x / a->width * 960, y = event.button.y / a->height * 540;
            int used = 0;
            for (int i = 0; i < a->button_count; i++) {
                WcButton b = a->buttons[i];
                if (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h) {
                    wc_action(a, b.action, b.a, b.b, b.c);
                    used = 1;
                    break;
                }
            }
            if (!used && !a->panel) {
                if (!a->captured)
                    wc_panel(a, PANEL_NONE);
                else
                    strike(a, 0);
            }
        }
    }
    if (!a->xr)
        SDL_GetWindowSizeInPixels(a->window, &a->width, &a->height);
    a->game.paused = a->panel != PANEL_NONE || !a->focused;
    const bool *keys = a->keys;
    if (!a->game.paused) {
        wc_look(&a->game.s, (keys[SDL_SCANCODE_RIGHT] - keys[SDL_SCANCODE_LEFT]) * 1.6 * dt,
                (keys[SDL_SCANCODE_UP] - keys[SDL_SCANCODE_DOWN]) * 1.3 * dt);
        WcInput input = {.forward = keys[SDL_SCANCODE_W] - keys[SDL_SCANCODE_S] + a->touch_forward,
                         .side = keys[SDL_SCANCODE_D] - keys[SDL_SCANCODE_A] + a->touch_side,
                         .sprint = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT],
                         .jump = a->jump};
        a->accumulator += dt;
        while (a->accumulator >= 1.0 / 60) {
            wc_step(&a->game, input, 1.0 / 60);
            a->accumulator -= 1.0 / 60;
            input.jump = 0;
            a->jump = 0;
        }
    } else
        a->accumulator = 0;
    wc_scene(&a->game.s, &a->scene);
    a->hit = wc_raycast(&a->game.s, &a->scene, 3.5);
    if (!a->xr) {
        wc_render(a);
        if (a->test_frames && a->frame_count + 1 == a->test_frames && a->capture_path[0])
            wc_capture(a, a->capture_path);
        SDL_GL_SwapWindow(a->window);
    }
    a->frame_count++;
    if (a->test_frames && a->frame_count >= a->test_frames)
        a->running = 0;
}
int main(int argc, char **argv) {
    WcApp *a = &app;
    memset(a, 0, sizeof(*a));
    a->running = 1;
    a->focused = 1;
    a->width = 1280;
    a->height = 720;
    a->help = 0;
    a->build_kind = ROOM_HALL;
    uint32_t seed = (uint32_t)wc_hash((uint64_t)time(NULL));
    const char *env = getenv("WANDERING_ASSETS");
    snprintf(a->asset_dir, sizeof(a->asset_dir), "%s", env ? env : "assets");
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--seed") && i + 1 < argc)
            seed = (uint32_t)strtoul(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--assets") && i + 1 < argc)
            snprintf(a->asset_dir, sizeof(a->asset_dir), "%s", argv[++i]);
        else if (!strcmp(argv[i], "--test-frames") && i + 1 < argc)
            a->test_frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--capture") && i + 1 < argc)
            snprintf(a->capture_path, sizeof(a->capture_path), "%s", argv[++i]);
    }
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }
    SDL_InitSubSystem(SDL_INIT_AUDIO);
#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    snprintf(a->save_path, sizeof(a->save_path), "/saves/journey.save");
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    const char *base = SDL_GetBasePath();
    if (!env && base && !strcmp(a->asset_dir, "assets"))
        snprintf(a->asset_dir, sizeof(a->asset_dir), "%sassets", base);
    char *pref = SDL_GetPrefPath("WanderingCastle", "WanderingCastle");
    snprintf(a->save_path, sizeof(a->save_path), "%sjourney.save", pref ? pref : "./");
    SDL_free(pref);
#endif
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    a->window = SDL_CreateWindow("Wandering Castle — Hearthstrider", a->width, a->height,
                                 SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!a->window) {
        fprintf(stderr, "Window: %s\n", SDL_GetError());
        return 1;
    }
    a->context = SDL_GL_CreateContext(a->window);
    if (!a->context) {
        fprintf(stderr, "OpenGL: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetSwapInterval(1);
    char path[600], error[300];
    snprintf(path, sizeof(path), "%s/scripts/content.lua", a->asset_dir);
    if (!wc_load_content(&a->game.defs, path, error, sizeof(error))) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    snprintf(path, sizeof(path), "%s/scripts/home.lua", a->asset_dir);
    if (!wc_load_art(&a->art, path, error, sizeof(error))) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    wc_init(&a->game, seed);
    if (!wc_render_init(a)) {
        fprintf(stderr, "Renderer initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    snprintf(path, sizeof(path), "%s/audio/chime.wav", a->asset_dir);
    if (SDL_LoadWAV(path, &a->sound_spec, &a->sound, &a->sound_length)) {
        a->audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &a->sound_spec,
                                             NULL, NULL);
        if (a->audio)
            SDL_ResumeAudioStreamDevice(a->audio);
    }
    wc_panel(a, PANEL_TITLE);
    a->previous = SDL_GetTicks() / 1000.0;
#ifdef __EMSCRIPTEN__
    web_ready();
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (a->running) {
        frame();
        SDL_Delay(1);
    }
    wc_render_destroy();
    if (a->audio)
        SDL_DestroyAudioStream(a->audio);
    SDL_free(a->sound);
    SDL_GL_DestroyContext(a->context);
    SDL_DestroyWindow(a->window);
    SDL_Quit();
#endif
    return 0;
}
