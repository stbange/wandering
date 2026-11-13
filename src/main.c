#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "content.h"
#include "save.h"
#include "render.h"
#include "input.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef struct {
    SDL_Window *window;SDL_GLContext gl;
    WCConfig cfg;WCState state;WCControls controls;
    char save_path[1024];double accumulator;Uint64 previous;
    bool test;
} App;
static App app;
static uint32_t new_seed(void) { Uint64 n=SDL_GetPerformanceCounter();return (uint32_t)(n^(n>>32)^SDL_GetTicksNS()); }
static void pump(App *a) {
    SDL_Event e;while(SDL_PollEvent(&e))wc_event(&a->controls,&a->state,&a->cfg,&e);
}
static void persistence(App *a) {
    char error[WC_MESSAGE];bool changed=false;
    if(a->controls.save) {
        bool saved=wc_save_file(&a->state,a->save_path,error,sizeof(error));
        snprintf(a->state.message,sizeof(a->state.message),"%s",error);a->controls.save=false;
#ifdef __EMSCRIPTEN__
        if(saved) EM_ASM({FS.syncfs(false,function(e){if(e)console.error('Save sync failed',e);});});
#else
        (void)saved;
#endif
    }
    if(a->controls.load || a->controls.recover) {
        changed=wc_load_file(&a->state,a->save_path,a->controls.recover,error,sizeof(error));
        snprintf(a->state.message,sizeof(a->state.message),"%s",error);a->controls.load=a->controls.recover=false;
    }
    if(a->controls.new_journey) {bool help=a->state.help;wc_init(&a->state,&a->cfg,new_seed());a->state.help=help;changed=true;a->controls.new_journey=false;}
    if(changed) {wc_controls_clear(&a->controls);a->accumulator=0;}
}
static void draw(App *a) {
    int w,h;SDL_GetWindowSizeInPixels(a->window,&w,&h);if(w<1||h<1)return;
    int lw,lh;SDL_GetWindowSize(a->window,&lw,&lh);a->controls.width=lw;a->controls.height=lh;
    wc_render(&a->state,&a->cfg,&a->controls,w,h);
}
static void frame(void *ptr) {
    App *a=ptr;pump(a);persistence(a);
    bool capture=a->state.context!=WC_MENU && a->state.focused && !a->controls.touch_active;
    if(SDL_GetWindowRelativeMouseMode(a->window)!=capture) SDL_SetWindowRelativeMouseMode(a->window,capture);
    Uint64 now=SDL_GetPerformanceCounter();double dt=(double)(now-a->previous)/SDL_GetPerformanceFrequency();a->previous=now;
    if(a->state.context==WC_MENU) a->accumulator=0;else a->accumulator+=fmin(dt,.1);
    while(a->accumulator>=WC_DT) {wc_tick(&a->state,&a->cfg,wc_controls_sample(&a->controls),WC_DT,wc_terrain,&a->state.seed);a->accumulator-=WC_DT;}
    draw(a);SDL_GL_SwapWindow(a->window);
}
static void key(App *a,SDL_Scancode code,bool down) {
    SDL_Event e;SDL_zero(e);e.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;e.key.scancode=code;e.key.down=down;SDL_PushEvent(&e);pump(a);
}
static void press(App *a,SDL_Scancode code) {key(a,code,true);key(a,code,false);}
static void sim(App *a,int frames) {
    for(int i=0;i<frames;i++) {wc_tick(&a->state,&a->cfg,wc_controls_sample(&a->controls),WC_DT,wc_terrain,&a->state.seed);if(i%60==0) {draw(a);SDL_GL_SwapWindow(a->window);}}
}
static bool game_test(App *a) {
    int failed=0;
#define EXPECT(condition,label) do {if(!(condition)){fprintf(stderr,"FAIL: %s\n",label);failed++;}else printf("PASS: %s\n",label);}while(0)
    SDL_Event event;while(SDL_PollEvent(&event)){}
    a->state.focused=true;a->controls.width=1280;a->controls.height=720;
    draw(a);EXPECT(wc_screenshot("game-home.bmp",1280,720),"capture real GL framebuffer");
    key(a,SDL_SCANCODE_W,true);sim(a,330);key(a,SDL_SCANCODE_W,false);
    EXPECT(a->state.player.z< -3.8 && a->state.player.y>.4,"walk actual cabin collision and command ramp");
    SDL_zero(event);event.type=SDL_EVENT_MOUSE_MOTION;event.motion.yrel=240;SDL_PushEvent(&event);pump(a);
    press(a,SDL_SCANCODE_E);EXPECT(a->state.context==WC_HELM,"aimed E reaches physical helm");
    WCVec local=a->state.player;double yaw=a->state.yaw;
    key(a,SDL_SCANCODE_W,true);key(a,SDL_SCANCODE_D,true);key(a,SDL_SCANCODE_LSHIFT,true);sim(a,240);
    EXPECT(hypot(a->state.castle.x,a->state.castle.z)>45,"real W/D/Shift moves and turns castle at turbo");
    EXPECT(fabs(a->state.player.x-local.x)<1e-8&&fabs(a->state.player.z-local.z)<1e-8,"attached player has no platform drift");
    EXPECT(a->state.yaw==yaw,"travel preserves local camera orientation");
    press(a,SDL_SCANCODE_SPACE);sim(a,20);EXPECT(a->state.player.y>local.y+.2,"jump while piloting a moving castle");sim(a,120);
    key(a,SDL_SCANCODE_RIGHT,true);sim(a,20);key(a,SDL_SCANCODE_RIGHT,false);EXPECT(a->state.yaw>yaw,"arrow look while piloting");
    press(a,SDL_SCANCODE_E);EXPECT(a->state.context==WC_FOOT&&a->state.speed==0,"helm release brakes and clears held movement");
    WCVec before=a->state.player;sim(a,30);EXPECT(a->state.player.z==before.z,"held movement cleared after interaction");
    press(a,SDL_SCANCODE_ESCAPE);double time=a->state.clock;sim(a,120);EXPECT(a->state.clock==time,"pause freezes simulation");
    press(a,SDL_SCANCODE_F1);bool help=a->state.help;
    SDL_zero(event);event.type=SDL_EVENT_WINDOW_FOCUS_LOST;SDL_PushEvent(&event);pump(a);press(a,SDL_SCANCODE_ESCAPE);
    EXPECT(a->state.context==WC_MENU&&!a->state.focused,"focus loss pauses and refuses unfocused resume");
    event.type=SDL_EVENT_WINDOW_FOCUS_GAINED;SDL_PushEvent(&event);pump(a);press(a,SDL_SCANCODE_ESCAPE);
    EXPECT(a->state.context==WC_FOOT,"focus recovery restores usable context");
    press(a,SDL_SCANCODE_F5);persistence(a);double saved=a->state.player.z;
    key(a,SDL_SCANCODE_S,true);sim(a,30);press(a,SDL_SCANCODE_F9);persistence(a);
    EXPECT(fabs(a->state.player.z-saved)<1e-8&&a->state.context==WC_FOOT,"F9 restores actual saved journey on foot");
    EXPECT(a->state.help==help,"help preference survives restore");
    double restored_yaw=a->state.yaw;key(a,SDL_SCANCODE_LEFT,true);sim(a,10);key(a,SDL_SCANCODE_LEFT,false);
    EXPECT(a->state.yaw<restored_yaw,"arrow input still works after F9");
    wc_init(&a->state,&a->cfg,271828);wc_controls_clear(&a->controls);a->state.player=(WCVec){0,0,6};a->state.yaw=WC_PI;
    EXPECT(wc_gate(&a->state,wc_terrain,&a->state.seed)==WC_OK,"deploy entrance before walking route");
    key(a,SDL_SCANCODE_W,true);sim(a,420);key(a,SDL_SCANCODE_W,false);EXPECT(!a->state.attached,"walk visible stair collision to ground without jumping");
    a->state.yaw=0;key(a,SDL_SCANCODE_W,true);sim(a,420);key(a,SDL_SCANCODE_W,false);EXPECT(wc_inside(&a->state),"walk same stairs back into sanctuary without jumping");
    /* Exterior evidence remains first-person: parked at the foot of the entrance. */
    a->state.attached=false;a->state.player=(WCVec){18,0,23};a->state.yaw=-.65;a->state.pitch=.12;
    draw(a);EXPECT(wc_screenshot("game-exterior.bmp",1280,720),"capture castle exterior from on-foot view");
    printf("Game input checks complete: %d failures, %u SDL events.\n",failed,a->controls.events);return failed==0;
}
int main(int argc,char **argv) {
    uint32_t seed=0;bool explicit_seed=false;const char *capture=NULL;
    for(int i=1;i<argc;i++) {
        if(strcmp(argv[i],"--game-test")==0) app.test=true;
        else if(strcmp(argv[i],"--seed")==0 && i+1<argc){seed=(uint32_t)strtoul(argv[++i],NULL,10);explicit_seed=true;}
        else if(strcmp(argv[i],"--capture")==0 && i+1<argc)capture=argv[++i];
        else {fprintf(stderr,"Usage: wandering [--seed UINT32] [--game-test] [--capture file.bmp]\n");return 2;}
    }
    if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_GAMEPAD)){fprintf(stderr,"SDL: %s\n",SDL_GetError());return 1;}
#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    app.window=SDL_CreateWindow("Wandering Castle | Hearthstrider",1280,720,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|(app.test?SDL_WINDOW_HIDDEN:0));
    if(!app.window){fprintf(stderr,"Window: %s\n",SDL_GetError());SDL_Quit();return 1;}
    app.gl=SDL_GL_CreateContext(app.window);if(!app.gl){fprintf(stderr,"OpenGL: %s\n",SDL_GetError());SDL_DestroyWindow(app.window);SDL_Quit();return 1;}
    SDL_GL_SetSwapInterval(app.test?0:1);
    const char *base=SDL_GetBasePath();char path[1024],error[512];
#ifdef __EMSCRIPTEN__
    base="/";
#endif
    snprintf(path,sizeof(path),"%scontent/home.lua",base);
    if(!wc_load_content(&app.cfg,path,error,sizeof(error)) || !wc_render_init(base,error,sizeof(error))){fprintf(stderr,"Startup: %s\n",error);SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Wandering Castle",error,app.window);return 1;}
    wc_init(&app.state,&app.cfg,explicit_seed?seed:new_seed());
    char *pref=SDL_GetPrefPath("WanderingCastle","wandering");snprintf(app.save_path,sizeof(app.save_path),"%sjourney.sav",pref?pref:"");SDL_free(pref);
    if(app.test)snprintf(app.save_path,sizeof(app.save_path),"game-test.sav");
    app.controls.width=1280;app.controls.height=720;app.previous=SDL_GetPerformanceCounter();
    int status=0;
    if(app.test) status=game_test(&app)?0:1;
    else if(capture){draw(&app);status=wc_screenshot(capture,1280,720)?0:1;}
    else {
#ifdef __EMSCRIPTEN__
        emscripten_set_main_loop_arg(frame,&app,0,true);
#else
        while(!app.controls.quit)frame(&app);
#endif
    }
    wc_render_shutdown();SDL_GL_DestroyContext(app.gl);SDL_DestroyWindow(app.window);SDL_Quit();return status;
}
