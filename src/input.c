#include "input.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

void wc_controls_clear(WCControls *in) {
    memset(in->held,0,sizeof(in->held));in->jump_edge=false;
    in->touch_forward=in->touch_strafe=0;in->touch_jump=false;in->left_finger=0;in->right_finger=0;
}
static void result(WCState *s,WCResult r) { snprintf(s->message,sizeof(s->message),"%s",wc_result_text(r)); }
static void interact(WCState *s,const WCConfig *cfg) {
    if(s->attached) { result(s,wc_interact(s,cfg));return; }
    int site=-1;double best=2.8*2.8;
    for(int i=0;i<16;i++) {
        double x=(i%4)*7.0-10-s->player.x,z=(i/4)*7.0+23-s->player.z;
        double d=x*x+z*z;
        /* Outdoor harvesting also requires aiming toward the nearby site. */
        if(d<best && x*sin(s->yaw)-z*cos(s->yaw)>.5) {site=i;best=d;}
    }
    result(s,site>=0?wc_gather(s,site):WC_TOO_FAR);
}
void wc_menu_action(WCControls *in,WCState *s,const WCConfig *cfg,int action) {
    (void)cfg;
    if(s->panel==0) {
        if(action==0) wc_pause(s,false);
        else if(action==1) in->save=true;
        else if(action==2) in->load=true;
        else if(action==3) in->recover=true;
        else if(action==4) s->panel=10;
        else if(action==5) in->quit=true;
    } else if(s->panel==10) {
        if(action==0) in->new_journey=true;
        else if(action==1) s->panel=0;
    } else if(s->panel==WC_BED) {
        if(action==0) result(s,wc_rest(s));else if(action==1) in->save=true;
    } else if(s->panel==WC_WORKSHOP) result(s,wc_craft(s,action));
    else if(s->panel==WC_PLANTER) result(s,wc_garden(s,s->selection,action));
    else if(s->panel==WC_PANTRY) {
        if(action==0) s->selection=(s->selection+1)%WC_ITEMS;
        else if(action==1) result(s,wc_transfer(s,s->selection,1,true));
        else if(action==2) result(s,wc_transfer(s,s->selection,1,false));
    }
    wc_controls_clear(in);
}
void wc_event(WCControls *in,WCState *s,const WCConfig *cfg,const SDL_Event *e) {
    in->events++;
    if(e->type==SDL_EVENT_QUIT) in->quit=true;
    else if(e->type==SDL_EVENT_WINDOW_FOCUS_LOST) {wc_focus(s,false);wc_controls_clear(in);}
    else if(e->type==SDL_EVENT_WINDOW_FOCUS_GAINED) {wc_focus(s,true);wc_controls_clear(in);}
    else if(e->type==SDL_EVENT_KEY_UP) in->held[e->key.scancode]=false;
    else if(e->type==SDL_EVENT_KEY_DOWN && !e->key.repeat) {
        SDL_Scancode key=e->key.scancode;
        if(key==SDL_SCANCODE_ESCAPE) { if(s->context==WC_MENU && s->panel!=0) s->panel=0;else wc_pause(s,s->context!=WC_MENU);wc_controls_clear(in); }
        else if(key==SDL_SCANCODE_F1 || (key==SDL_SCANCODE_TAB && s->context!=WC_MENU)) s->help=!s->help;
        else if(key==SDL_SCANCODE_F5) in->save=true;
        else if(key==SDL_SCANCODE_F9) in->load=true;
        else if(s->context==WC_MENU) {
            if(key>=SDL_SCANCODE_1 && key<=SDL_SCANCODE_6) wc_menu_action(in,s,cfg,key-SDL_SCANCODE_1);
            else if(key==SDL_SCANCODE_TAB || key==SDL_SCANCODE_DOWN) in->look_y=(float)(((int)in->look_y+1)%6);
            else if(key==SDL_SCANCODE_RETURN) wc_menu_action(in,s,cfg,(int)in->look_y);
        } else {
            in->held[key]=true;
            if(key==SDL_SCANCODE_SPACE) in->jump_edge=true;
            if(key==SDL_SCANCODE_E) {interact(s,cfg);wc_controls_clear(in);}
            if(key==SDL_SCANCODE_I) {s->panel=9;wc_pause(s,true);wc_controls_clear(in);}
            if(key==SDL_SCANCODE_B || key==SDL_SCANCODE_F || key==SDL_SCANCODE_K || key==SDL_SCANCODE_Q || key==SDL_SCANCODE_V || key==SDL_SCANCODE_C || key==SDL_SCANCODE_X)
                snprintf(s->message,sizeof(s->message),"This feature is on the roadmap; see the feature checklist.");
        }
    } else if(e->type==SDL_EVENT_MOUSE_MOTION && s->context!=WC_MENU && e->motion.which!=SDL_TOUCH_MOUSEID) {
        wc_look(s,e->motion.xrel*.0025,-e->motion.yrel*.0025);
    } else if(e->type==SDL_EVENT_MOUSE_BUTTON_DOWN && e->button.which!=SDL_TOUCH_MOUSEID) {
        double x=e->button.x*960/in->width,y=e->button.y*540/in->height;
        if(s->context==WC_MENU && x>798 && y<52) s->help=!s->help;
        else if(s->context==WC_MENU && x>=270 && x<=690 && y>=170 && y<410) wc_menu_action(in,s,cfg,(int)((y-170)/40));
    } else if(e->type==SDL_EVENT_FINGER_DOWN) {
        in->touch_active=true;
        if(s->context==WC_MENU) {
            if(e->tfinger.x>.83 && e->tfinger.y<.1) s->help=!s->help;
            else if(e->tfinger.x>.28 && e->tfinger.x<.72 && e->tfinger.y>.315 && e->tfinger.y<.76)
                wc_menu_action(in,s,cfg,(int)((e->tfinger.y*540-170)/40));
        } else if(e->tfinger.y<.1 && e->tfinger.x>.83) {wc_pause(s,true);wc_controls_clear(in);}
        else if(e->tfinger.x>.82 && e->tfinger.y>.72) interact(s,cfg);
        else if(e->tfinger.x>.65 && e->tfinger.y>.82) in->jump_edge=true;
        else if(e->tfinger.x<.45) {in->left_finger=e->tfinger.fingerID;in->touch_x=e->tfinger.x;in->touch_y=e->tfinger.y;}
        else in->right_finger=e->tfinger.fingerID;
    } else if(e->type==SDL_EVENT_FINGER_MOTION) {
        if(e->tfinger.fingerID==in->left_finger) {in->touch_strafe=(e->tfinger.x-in->touch_x)*8;in->touch_forward=(in->touch_y-e->tfinger.y)*8;}
        else if(e->tfinger.fingerID==in->right_finger) wc_look(s,e->tfinger.dx*3,-e->tfinger.dy*3);
    } else if(e->type==SDL_EVENT_FINGER_UP || e->type==SDL_EVENT_FINGER_CANCELED) {
        if(e->tfinger.fingerID==in->left_finger) {in->left_finger=0;in->touch_strafe=in->touch_forward=0;}
        if(e->tfinger.fingerID==in->right_finger) in->right_finger=0;
    }
}
WCInput wc_controls_sample(WCControls *in) {
    WCInput sample={0};
    sample.forward=in->held[SDL_SCANCODE_W]-in->held[SDL_SCANCODE_S]+in->touch_forward;
    sample.strafe=in->held[SDL_SCANCODE_D]-in->held[SDL_SCANCODE_A]+in->touch_strafe;
    sample.look_x=(in->held[SDL_SCANCODE_RIGHT]-in->held[SDL_SCANCODE_LEFT])*1.6;
    sample.look_y=(in->held[SDL_SCANCODE_UP]-in->held[SDL_SCANCODE_DOWN])*1.3;
    sample.turbo=in->held[SDL_SCANCODE_LSHIFT] || in->held[SDL_SCANCODE_RSHIFT];
    sample.jump=in->jump_edge;in->jump_edge=false;return sample;
}
