#ifndef WC_INPUT_H
#define WC_INPUT_H
#include "core.h"
#include <SDL3/SDL.h>
typedef struct {
    bool held[SDL_SCANCODE_COUNT];
    bool quit,save,load,recover,new_journey;
    bool jump_edge;
    int width,height;
    SDL_FingerID left_finger,right_finger;
    float touch_x,touch_y,look_x,look_y;
    double touch_forward,touch_strafe;
    bool touch_active,touch_jump;
    unsigned events;
} WCControls;
void wc_controls_clear(WCControls *in);
void wc_event(WCControls *in,WCState *s,const WCConfig *cfg,const SDL_Event *event);
WCInput wc_controls_sample(WCControls *in);
void wc_menu_action(WCControls *in,WCState *s,const WCConfig *cfg,int action);
#endif
