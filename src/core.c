#include "core.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static double clamp(double x,double a,double b) { return x<a?a:(x>b?b:x); }
static double wrap(double a) { return remainder(a,2*WC_PI); }
static WCVec to_world(const WCState *s,WCVec p) {
    double co=cos(s->heading),si=sin(s->heading);
    return (WCVec){s->castle.x+co*p.x+si*p.z,s->castle.y+p.y,s->castle.z-si*p.x+co*p.z};
}
WCVec wc_local(const WCState *s,WCVec w) {
    double x=w.x-s->castle.x,z=w.z-s->castle.z,co=cos(s->heading),si=sin(s->heading);
    return (WCVec){co*x-si*z,w.y-s->castle.y,si*x+co*z};
}
WCVec wc_world_player(const WCState *s) { return s->attached?to_world(s,s->player):s->player; }
bool wc_inside(const WCState *s) {
    return s->attached && fabs(s->player.x)<6 && fabs(s->player.z)<8 && s->player.y>=-0.5;
}
void wc_defaults(WCConfig *c) {
    memset(c,0,sizeof(*c)); c->speed=5; c->turbo=5; c->turn=.38;
    c->walk=3.3; c->sprint=5.5; c->jump=5.5; c->gravity=15; c->eye=1.62;
}
void wc_init(WCState *s,const WCConfig *c,uint32_t seed) {
    (void)c; memset(s,0,sizeof(*s)); s->seed=seed; s->castle.y=4.8;
    s->player=(WCVec){0,0,3.8}; s->attached=true; s->grounded=true;
    s->focused=true; s->help=true; s->health=100; s->stamina=100;
    s->pack.capacity=24; s->pantry.capacity=80; s->pantry.items[WC_SEED]=2;
    s->pantry.items[WC_MINT_SEED]=2; s->stair_length=8; s->stair_drop=4.8;
    for(int n=0;n<2;n++) s->crops[n].crop=-1;
    for(int n=0;n<16;n++) s->depleted[n]=-120;
    snprintf(s->message,sizeof(s->message),"Welcome home. Aim at the helm and press E.");
}
void wc_look(WCState *s,double dx,double dy) {
    if(s->context==WC_MENU || !s->focused || !isfinite(dx) || !isfinite(dy)) return;
    s->yaw=wrap(s->yaw+dx); s->pitch=clamp(s->pitch+dy,-1.45,1.45);
}
void wc_pause(WCState *s,bool pause) {
    if(pause && s->context!=WC_MENU) { s->prior_context=s->context; s->context=WC_MENU; }
    else if(!pause && s->focused && s->context==WC_MENU) { s->context=s->prior_context; s->panel=0; }
}
void wc_focus(WCState *s,bool focused) { s->focused=focused; if(!focused) wc_pause(s,true); }
static uint32_t hash(uint32_t x) { x^=x>>16; x*=0x7feb352du; x^=x>>15; x*=0x846ca68bu; return x^(x>>16); }
static double noise(double x,double z,uint32_t seed) {
    int64_t ix=(int64_t)floor(x),iz=(int64_t)floor(z);
    double tx=x-floor(x),tz=z-floor(z); tx=tx*tx*(3-2*tx); tz=tz*tz*(3-2*tz);
    double v[4];
    for(int i=0;i<4;i++) v[i]=(double)hash((uint32_t)(ix+(i&1))*374761393u ^ (uint32_t)(iz+(i>>1))*668265263u ^ seed)/4294967295.0;
    return ((v[0]*(1-tx)+v[1]*tx)*(1-tz)+(v[2]*(1-tx)+v[3]*tx)*tz)*2-1;
}
double wc_terrain(double x,double z,void *seed) {
    uint32_t n=*(const uint32_t *)seed;
    double edge=clamp((fmax(fabs(x),fabs(z))-64)/80,0,1);
    edge=edge*edge*(3-2*edge);
    return edge*(noise(x/170,z/170,n)*22+noise(x/49,z/49,n+17)*5);
}
static double floor_height(double x,double z) {
    if(fabs(x)<5.6 && z < -3) return clamp((-z-3)/2,0,1)*.6;
    return 0;
}
static bool collision(const WCState *s,const WCConfig *c,WCVec p) {
    const double r=.28;
    if(fabs(p.x)>5.7 && p.z<8.2) return true;
    if(p.z < -7.7) return true;
    if(p.z>7.7 && (!s->gate_open || fabs(p.x)>1.05)) return true;
    for(int i=0;i<c->object_count;i++) {
        WCObject o=c->objects[i];
        if(o.solid && p.x>o.min.x-r && p.x<o.max.x+r && p.z>o.min.z-r && p.z<o.max.z+r &&
           p.y<o.max.y-.05 && p.y+1.8>o.min.y+.05) return true;
    }
    return false;
}
static double support(const WCState *s,WCVec p) {
    if(p.z<=8) return floor_height(p.x,p.z);
    return -s->stair_drop*clamp((p.z-8)/s->stair_length,0,1);
}
void wc_tick(WCState *s,const WCConfig *c,WCInput in,double dt,WCSurface ground,void *user) {
    if(s->context==WC_MENU || !s->focused || !isfinite(dt) || dt<=0 || dt>.1) return;
    s->clock+=dt;
    wc_look(s,in.look_x*dt,in.look_y*dt);
    bool indoor=wc_inside(s);
    if(indoor) { s->health=100; }
    if(s->context==WC_HELM) {
        s->speed=clamp(in.forward,-1,1)*c->speed*(in.turbo?c->turbo:1);
        s->heading=wrap(s->heading-clamp(in.strafe,-1,1)*c->turn*dt);
        s->castle.x-=sin(s->heading)*s->speed*dt;
        s->castle.z-=cos(s->heading)*s->speed*dt;
        double highest=-1e20;
        for(int x=-6;x<=6;x+=2) for(int z=-8;z<=8;z+=2) {
            WCVec w=to_world(s,(WCVec){x,0,z}); highest=fmax(highest,ground(w.x,w.z,user));
        }
        s->castle.y=highest+4.8;
    }
    if(s->context!=WC_HELM) s->speed=0;
    if(s->context==WC_BUILD) return;
    double f=s->context==WC_HELM?0:clamp(in.forward,-1,1);
    double r=s->context==WC_HELM?0:clamp(in.strafe,-1,1),len=hypot(f,r);
    if(len>1) { f/=len; r/=len; }
    if(!in.turbo && s->stamina>=20) s->sprint_latched=false;
    bool sprint=in.turbo && len>0 && (indoor || (!s->sprint_latched && s->stamina>0));
    double move=(sprint?c->sprint:c->walk)*dt;
    if(sprint && !indoor) {
        double effort=18*dt*(s->meal>0?.7:1);
        s->stamina=fmax(0,s->stamina-effort); s->recovery=.8;
        s->rested=fmax(0,s->rested-dt); s->meal=fmax(0,s->meal-dt);
        if(s->stamina==0) s->sprint_latched=true;
    } else { s->recovery=fmax(0,s->recovery-dt); if(s->recovery==0) s->stamina=fmin(100,s->stamina+(s->rested>0?20:14)*dt); }
    WCVec p=s->player;
    double dx=(sin(s->yaw)*f+cos(s->yaw)*r)*move;
    double dz=(-cos(s->yaw)*f+sin(s->yaw)*r)*move;
    if(s->attached) {
        WCVec a=p; a.x+=dx; if(!collision(s,c,a)) p.x=a.x;
        a=p; a.z+=dz; if(!collision(s,c,a)) p.z=a.z;
        double level=support(s,p);
        if(s->grounded && !in.jump) p.y=level;
        if(in.jump && s->grounded) { s->vertical=c->jump; s->grounded=false; }
        s->vertical-=c->gravity*dt; p.y+=s->vertical*dt;
        if(p.y<=level) { p.y=level; s->vertical=0; s->grounded=true; }
        if(p.y>3.7) { p.y=3.7; s->vertical=fmin(0,s->vertical); }
        s->player=p;
        if(s->gate_open && p.z>8+s->stair_length) {
            s->player=to_world(s,p); s->yaw=wrap(s->yaw-s->heading); s->attached=false;
        }
        if(p.y< -60) { s->player=(WCVec){0,0,3.8}; s->vertical=0; s->grounded=true; }
    } else {
        p.x+=dx; p.z+=dz;
        double level=ground(p.x,p.z,user);
        if(in.jump && s->grounded) { s->vertical=c->jump; s->grounded=false; }
        s->vertical-=c->gravity*dt; p.y+=s->vertical*dt;
        if(p.y<=level) { p.y=level; s->vertical=0; s->grounded=true; }
        WCVec local=wc_local(s,p);
        if(s->gate_open && fabs(local.x)<1.05 && local.z>=7.7 && local.z<=8+s->stair_length+.3 &&
           fabs(local.y-support(s,local))<.45) {
            s->attached=true; p=local; p.y=support(s,p); s->yaw=wrap(s->yaw+s->heading); s->vertical=0;
        }
        s->player=p;
    }
}
double wc_ray_box(WCVec p,WCVec d,WCVec a,WCVec b,double limit) {
    double lo=0,hi=limit;
    double origin[3]={p.x,p.y,p.z},dir[3]={d.x,d.y,d.z},mn[3]={a.x,a.y,a.z},mx[3]={b.x,b.y,b.z};
    for(int n=0;n<3;n++) {
        if(fabs(dir[n])<1e-10) { if(origin[n]<mn[n] || origin[n]>mx[n]) return -1; }
        else {
            double t1=(mn[n]-origin[n])/dir[n],t2=(mx[n]-origin[n])/dir[n];
            if(t1>t2) { double t=t1;t1=t2;t2=t; }
            lo=fmax(lo,t1);hi=fmin(hi,t2); if(lo>hi) return -1;
        }
    }
    return lo;
}
int wc_target(const WCState *s,const WCConfig *c,double *distance) {
    if(!s->attached || s->context==WC_MENU) return -1;
    WCVec eye=s->player;eye.y+=c->eye;
    WCVec dir={sin(s->yaw)*cos(s->pitch),sin(s->pitch),-cos(s->yaw)*cos(s->pitch)};
    int hit=-1;double best=2.8;
    for(int i=0;i<c->object_count;i++) {
        WCObject o=c->objects[i]; double t=wc_ray_box(eye,dir,o.min,o.max,best);
        if(t>=0 && t<best) { hit=i;best=t; }
    }
    if(distance) *distance=best;
    return hit;
}
WCResult wc_interact(WCState *s,const WCConfig *c) {
    if(s->context==WC_HELM) { s->context=WC_FOOT; s->speed=0;return WC_OK; }
    if(s->context!=WC_FOOT) return WC_UNAVAILABLE;
    int target=wc_target(s,c,NULL); if(target<0) return WC_TOO_FAR;
    WCObject o=c->objects[target]; if(o.kind==WC_NONE) return WC_UNAVAILABLE;
    if(o.kind==WC_HELM_OBJECT) {
        if(s->gate_open) return WC_BLOCKED;
        s->context=WC_HELM; return WC_OK;
    }
    if(o.kind==WC_GATE) return wc_gate(s,wc_terrain,&s->seed);
    s->panel=o.kind;s->selection=o.slot;wc_pause(s,true);return WC_OK;
}
WCResult wc_gate(WCState *s,WCSurface ground,void *user) {
    if(s->context==WC_HELM || s->speed!=0) return WC_UNAVAILABLE;
    WCVec p=s->attached?s->player:wc_local(s,s->player);
    if(fabs(p.x)<1.5 && p.z>7.2 && p.z<9+s->stair_length) return WC_BLOCKED;
    if(!s->gate_open) {
        double length=8,drop=0;
        for(int n=0;n<8;n++) {
            WCVec landing=to_world(s,(WCVec){0,0,8+length});
            double y=ground(landing.x,landing.z,user);
            if(!isfinite(y)) return WC_UNAVAILABLE;
            drop=s->castle.y-y; length=fmax(3,fabs(drop)/.65);
        }
        if(length>48 || fabs(drop)/length>.8) return WC_BLOCKED;
        WCVec landing=to_world(s,(WCVec){0,0,8+length});
        if(fabs(ground(landing.x+1,landing.z,user)-ground(landing.x-1,landing.z,user))>.6) return WC_BLOCKED;
        s->stair_length=length;s->stair_drop=drop;
    }
    s->gate_open=!s->gate_open;return WC_OK;
}
WCResult wc_rest(WCState *s) {
    if(!wc_inside(s)) return WC_UNAVAILABLE;
    s->health=100;s->stamina=100;s->rested=120;s->sprint_latched=false;return WC_OK;
}
int wc_used(const WCInventory *i) { int n=0;for(int j=0;j<WC_ITEMS;j++) n+=i->items[j];return n; }
int wc_available(const WCState *s,int item) { return item>=0&&item<WC_ITEMS?s->pack.items[item]+s->pantry.items[item]:0; }
WCResult wc_exchange(WCState *s,const int cost[WC_ITEMS],const int reward[WC_ITEMS],bool shared) {
    WCInventory p=s->pack,h=s->pantry;
    for(int i=0;i<WC_ITEMS;i++) {
        if(cost[i]<0 || cost[i]>10000 || reward[i]<0 || reward[i]>10000) return WC_INVALID;
        int available=p.items[i]+(shared?h.items[i]:0);
        if(available<cost[i]) return WC_SUPPLIES;
        int paid=cost[i]<p.items[i]?cost[i]:p.items[i];
        p.items[i]-=paid;h.items[i]-=cost[i]-paid;p.items[i]+=reward[i];
    }
    if(wc_used(&p)>p.capacity) return WC_CAPACITY;
    s->pack=p;s->pantry=h;return WC_OK;
}
WCResult wc_craft(WCState *s,int recipe) {
    if(!wc_inside(s)) return WC_UNAVAILABLE;
    int cost[WC_ITEMS]={0},reward[WC_ITEMS]={0};
    if(recipe==0) { cost[WC_TIMBER]=2;reward[WC_PLANK]=2; }
    else if(recipe==1) { cost[WC_FIBER]=3;reward[WC_TWINE]=2; }
    else if(recipe==2) { if(s->satchel) return WC_UNAVAILABLE; cost[WC_PLANK]=2;cost[WC_TWINE]=2; }
    else return WC_INVALID;
    WCResult r=wc_exchange(s,cost,reward,true);
    if(r==WC_OK && recipe==2) { s->satchel=true;s->pack.capacity+=16; }
    return r;
}
WCResult wc_transfer(WCState *s,int item,int count,bool deposit) {
    if(!wc_inside(s)) return WC_UNAVAILABLE;
    if(item<0 || item>=WC_ITEMS || count<=0 || count>10000) return WC_INVALID;
    WCInventory *from=deposit?&s->pack:&s->pantry,*to=deposit?&s->pantry:&s->pack;
    if(from->items[item]<count) return WC_SUPPLIES;
    if(wc_used(to)+count>to->capacity) return WC_CAPACITY;
    from->items[item]-=count;to->items[item]+=count;return WC_OK;
}
WCResult wc_garden(WCState *s,int slot,int action) {
    if(!wc_inside(s)) return WC_UNAVAILABLE;
    if(slot<0 || slot>1 || action<0 || action>2) return WC_INVALID;
    WCCrop *crop=&s->crops[slot];int cost[WC_ITEMS]={0},reward[WC_ITEMS]={0};
    if(action<2) {
        if(crop->crop!=-1) return WC_BLOCKED;
        cost[action==0?WC_SEED:WC_MINT_SEED]=1;
        WCResult r=wc_exchange(s,cost,reward,true);if(r!=WC_OK) return r;
        crop->crop=action;crop->planted=s->clock;crop->duration=action==0?45:60;return WC_OK;
    }
    if(crop->crop<0 || s->clock-crop->planted<crop->duration) return WC_UNAVAILABLE;
    reward[crop->crop==0?WC_SUNROOT:WC_MINT]=3;reward[crop->crop==0?WC_SEED:WC_MINT_SEED]=1;
    WCResult r=wc_exchange(s,cost,reward,false);if(r!=WC_OK) return r;
    crop->crop=-1;crop->treated=false;return WC_OK;
}
WCResult wc_gather(WCState *s,int site) {
    if(wc_inside(s) || s->attached) return WC_UNAVAILABLE;
    if(site<0 || site>=16) return WC_INVALID;
    WCVec pos={(site%4)*7.0-10,0,(site/4)*7.0+23};
    if(hypot(pos.x-s->player.x,pos.z-s->player.z)>2.8) return WC_TOO_FAR;
    if(s->clock-s->depleted[site]<120) return WC_UNAVAILABLE;
    int cost[WC_ITEMS]={0},reward[WC_ITEMS]={0};reward[site%3]=3;
    WCResult r=wc_exchange(s,cost,reward,false);
    if(r==WC_OK) s->depleted[site]=s->clock;
    return r;
}
const char *wc_item_name(int item) {
    static const char *names[]={"Timber","Stone","Fiber","Sunroot seed","Mint seed","Plank","Twine","Sunroot","Mint"};
    return item>=0&&item<WC_ITEMS?names[item]:"Unknown";
}
const char *wc_result_text(WCResult result) {
    static const char *text[]={"Done.","Invalid command.","Not enough backpack or storage space.","Not enough supplies.","Not available yet.","Blocked: clear the doorway / stairs or close the gate.","Move closer and aim at an object."};
    return result>=WC_OK&&result<=WC_TOO_FAR?text[result]:"Unknown result.";
}
bool wc_validate(const WCState *s) {
    double v[]={s->clock,s->castle.x,s->castle.y,s->castle.z,s->player.x,s->player.y,s->player.z,
        s->heading,s->yaw,s->pitch,s->vertical,s->speed,s->stair_length,s->stair_drop,s->health,
        s->stamina,s->rested,s->meal,s->recovery};
    for(size_t n=0;n<sizeof(v)/sizeof(*v);n++) if(!isfinite(v[n]) || fabs(v[n])>1e12) return false;
    if(s->clock<0 || fabs(s->pitch)>1.45 || s->context<WC_FOOT || s->context>WC_MENU ||
       s->prior_context<WC_FOOT || s->prior_context>WC_BUILD || s->stair_length<3 || s->stair_length>48 ||
       s->health<0 || s->health>100 || s->stamina<0 || s->stamina>100 || s->rested<0 || s->rested>120 ||
       s->meal<0 || s->meal>180 || s->pack.capacity!=(s->satchel?40:24) || s->pantry.capacity!=80) return false;
    if(s->attached && (fabs(s->player.x)>6 || s->player.z< -8 || s->player.z>57 || s->player.y>4 || s->player.y< -50)) return false;
    for(int i=0;i<WC_ITEMS;i++) if(s->pack.items[i]<0 || s->pack.items[i]>40 || s->pantry.items[i]<0 || s->pantry.items[i]>80) return false;
    if(wc_used(&s->pack)>s->pack.capacity || wc_used(&s->pantry)>80) return false;
    for(int i=0;i<2;i++) if(s->crops[i].crop< -1 || s->crops[i].crop>1 || !isfinite(s->crops[i].planted) ||
        !isfinite(s->crops[i].duration) || s->crops[i].planted<0 || s->crops[i].planted>s->clock || s->crops[i].duration<0 || s->crops[i].duration>60) return false;
    for(int i=0;i<16;i++) if(!isfinite(s->depleted[i]) || s->depleted[i]<-120 || s->depleted[i]>s->clock) return false;
    return true;
}
