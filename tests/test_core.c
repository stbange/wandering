#include "core.h"
#include "content.h"
#include "save.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks,failures;
#define CHECK(x) do { checks++; if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);failures++; } } while(0)
#define NEAR(a,b,e) CHECK(fabs((a)-(b))<(e))
static WCConfig cfg;
static WCState fresh(void) { WCState s;wc_init(&s,&cfg,271828u);return s; }
static double flat(double x,double z,void *u) { (void)x;(void)z;(void)u;return 0; }
static double slope(double x,double z,void *u) { return (x*.6+z*.8)*(*(double *)u); }
static double unavailable(double x,double z,void *u) { (void)x;(void)z;(void)u;return NAN; }
static void ticks(WCState *s,WCInput in,int count) { for(int i=0;i<count;i++) wc_tick(s,&cfg,in,WC_DT,flat,NULL); }
static void motion(void) {
    WCState s=fresh();CHECK(wc_inside(&s));CHECK(wc_validate(&s));
    WCVec w=wc_world_player(&s),p=wc_local(&s,w);NEAR(p.z,s.player.z,1e-9);
    s.heading=1.1;w=wc_world_player(&s);p=wc_local(&s,w);NEAR(p.x,s.player.x,1e-9);NEAR(p.z,s.player.z,1e-9);
    s.context=WC_HELM;WCVec start=s.player;double yaw=s.yaw;
    ticks(&s,(WCInput){.forward=1,.strafe=.6,.turbo=true},120*60);
    NEAR(s.player.x,start.x,1e-9);NEAR(s.player.y,start.y,1e-9);NEAR(s.player.z,start.z,1e-9);NEAR(s.yaw,yaw,1e-9);NEAR(s.speed,25,1e-9);
    CHECK(wc_interact(&s,&cfg)==WC_OK);CHECK(s.context==WC_FOOT);CHECK(s.speed==0);
    WCVec castle=s.castle;ticks(&s,(WCInput){0},120);NEAR(s.castle.x,castle.x,1e-9);NEAR(s.castle.z,castle.z,1e-9);
    for(int angle=20;angle<=45;angle+=(angle==20?15:10)) {
        double grade=tan(angle*WC_PI/180);s=fresh();s.context=WC_HELM;
        for(int i=0;i<2400;i++) {
            wc_tick(&s,&cfg,(WCInput){.forward=i<1200?1:-1,.strafe=.7,.turbo=true},WC_DT,slope,&grade);
            CHECK(s.castle.y> slope(s.castle.x,s.castle.z,&grade)+4);
        }
        NEAR(s.player.x,0,1e-9);NEAR(s.player.z,3.8,1e-9);
    }
    s=fresh();ticks(&s,(WCInput){.strafe=1},600);CHECK(s.player.x<=5.7);
    s=fresh();ticks(&s,(WCInput){.forward=1},600);CHECK(s.player.z>=-7.7);CHECK(s.player.z> -5.6);
    s=fresh();ticks(&s,(WCInput){.forward=-1},600);CHECK(s.player.z<=7.7);
    s=fresh();ticks(&s,(WCInput){.jump=true},1);CHECK(!s.grounded && s.player.y>0);
    ticks(&s,(WCInput){0},180);CHECK(s.grounded);NEAR(s.player.y,0,1e-9);
    s=fresh();s.player.x=2;ticks(&s,(WCInput){.forward=1},360);CHECK(s.player.y>.5);
    s=fresh();ticks(&s,(WCInput){.strafe=.5,.forward=.5,.turbo=true},20);CHECK(s.player.x>0);
    s=fresh();s.vertical=100;s.grounded=false;ticks(&s,(WCInput){0},10);CHECK(s.player.y<=3.7);
    s=fresh();s.player=(WCVec){-4.5,1.2,0};s.grounded=false;ticks(&s,(WCInput){0},120);
    NEAR(s.player.y,.65,1e-9);CHECK(s.grounded);
    ticks(&s,(WCInput){.strafe=1},120);CHECK(s.player.x> -3.6);NEAR(s.player.y,0,1e-9);
    s=fresh();s.context=WC_BUILD;WCState copy=s;ticks(&s,(WCInput){.forward=1},1);CHECK(s.player.z==copy.player.z);
    s=fresh();copy=s;wc_tick(&s,&cfg,(WCInput){0},0,flat,NULL);wc_tick(&s,&cfg,(WCInput){0},NAN,flat,NULL);wc_tick(&s,&cfg,(WCInput){0},1,flat,NULL);CHECK(memcmp(&s,&copy,sizeof(s))==0);
}
static void input_contexts(void) {
    WCState a=fresh(),b=a;
    wc_look(&a,1,.4);for(int i=0;i<100;i++) wc_look(&b,.01,.004);
    NEAR(a.yaw,b.yaw,1e-12);NEAR(a.pitch,b.pitch,1e-12);
    wc_look(&a,0,100);NEAR(a.pitch,1.45,1e-9);wc_look(&a,0,-.01);NEAR(a.pitch,1.44,1e-9);
    wc_look(&a,0,-100);wc_look(&a,0,.01);NEAR(a.pitch,-1.44,1e-9);
    a=fresh();WCVec p=a.player;ticks(&a,(WCInput){.look_x=1,.look_y=.2},120);NEAR(a.yaw,1,1e-9);NEAR(a.player.z,p.z,1e-9);
    a.context=WC_HELM;wc_pause(&a,true);double clock=a.clock,yaw=a.yaw;
    ticks(&a,(WCInput){.forward=1},10);wc_look(&a,10,10);NEAR(a.clock,clock,1e-9);NEAR(a.yaw,yaw,1e-9);
    wc_pause(&a,true);CHECK(a.prior_context==WC_HELM);wc_pause(&a,false);CHECK(a.context==WC_HELM);
    wc_focus(&a,false);CHECK(a.context==WC_MENU);wc_pause(&a,false);CHECK(a.context==WC_MENU);
    wc_focus(&a,true);CHECK(a.context==WC_MENU);wc_pause(&a,false);CHECK(a.context==WC_HELM);
    wc_look(&a,NAN,0);wc_look(&a,0,NAN);NEAR(a.yaw,yaw,1e-9);
}
static void interaction(void) {
    WCState s=fresh();CHECK(wc_interact(&s,&cfg)==WC_TOO_FAR);
    s.player=(WCVec){0,.6,-3.8};s.pitch=-.2;double dist;
    int hit=wc_target(&s,&cfg,&dist);CHECK(hit>=0);CHECK(dist<2.8);
    CHECK(wc_interact(&s,&cfg)==WC_OK);CHECK(s.context==WC_HELM);
    CHECK(wc_gate(&s,flat,NULL)==WC_UNAVAILABLE);
    CHECK(wc_interact(&s,&cfg)==WC_OK);s.gate_open=true;CHECK(wc_interact(&s,&cfg)==WC_BLOCKED);
    s.context=WC_MENU;CHECK(wc_interact(&s,&cfg)==WC_UNAVAILABLE);CHECK(wc_target(&s,&cfg,NULL)==-1);
    s=fresh();s.attached=false;CHECK(wc_target(&s,&cfg,NULL)==-1);
    WCVec mn={-1,-1,-1},mx={1,1,1};
    NEAR(wc_ray_box((WCVec){0,0,5},(WCVec){0,0,-1},mn,mx,10),4,1e-9);
    CHECK(wc_ray_box((WCVec){5,0,5},(WCVec){0,0,-1},mn,mx,10)<0);
    CHECK(wc_ray_box((WCVec){0,0,5},(WCVec){0,0,1},mn,mx,10)<0);
    CHECK(wc_ray_box((WCVec){0,0,5},(WCVec){1,0,-1},mn,mx,10)<0);
    for(int i=2;i<cfg.object_count;i++) {
        WCObject o=cfg.objects[i];s=fresh();s.player=(WCVec){(o.min.x+o.max.x)/2,0,o.max.z+1.4};s.pitch=-.65;
        if(o.kind!=WC_GATE) { CHECK(wc_interact(&s,&cfg)==WC_OK);CHECK(s.context==WC_MENU);CHECK(s.panel==o.kind); }
    }
    WCConfig decorated=cfg;decorated.objects[0].kind=WC_NONE;
    s=fresh();s.player=(WCVec){0,.6,-3.8};s.pitch=-.2;CHECK(wc_interact(&s,&decorated)==WC_UNAVAILABLE);
}
static void boarding(void) {
    WCState s=fresh();CHECK(wc_gate(&s,flat,NULL)==WC_OK);CHECK(s.gate_open);
    CHECK(s.stair_drop/s.stair_length<=.8);
    ticks(&s,(WCInput){.forward=-1},750);CHECK(!s.attached);CHECK(s.player.y==0);
    double z=s.player.z;ticks(&s,(WCInput){.forward=1},750);CHECK(s.attached);CHECK(wc_inside(&s));CHECK(s.player.z<z);
    s.player.z=9;CHECK(wc_gate(&s,flat,NULL)==WC_BLOCKED);s.player.z=3;
    CHECK(wc_gate(&s,flat,NULL)==WC_OK);CHECK(!s.gate_open);
    CHECK(wc_gate(&s,unavailable,NULL)==WC_UNAVAILABLE);
    s.castle.y=100;CHECK(wc_gate(&s,flat,NULL)==WC_BLOCKED);
    s=fresh();double cross=1;CHECK(wc_gate(&s,slope,&cross)==WC_BLOCKED);
    s=fresh();s.speed=1;CHECK(wc_gate(&s,flat,NULL)==WC_UNAVAILABLE);
    s=fresh();s.attached=false;s.player=(WCVec){1,0,10};CHECK(wc_gate(&s,flat,NULL)==WC_BLOCKED);
}
static void economy(void) {
    WCState s=fresh();s.pack.items[WC_TIMBER]=1;s.pantry.items[WC_TIMBER]=1;
    CHECK(wc_available(&s,WC_TIMBER)==2);CHECK(wc_available(&s,-1)==0);
    CHECK(wc_craft(&s,&cfg,0)==WC_OK);CHECK(s.pack.items[WC_PLANK]==2);CHECK(wc_available(&s,WC_TIMBER)==0);
    s.pack.items[WC_FIBER]=3;CHECK(wc_craft(&s,&cfg,1)==WC_OK);CHECK(s.pack.items[WC_TWINE]==2);
    CHECK(wc_craft(&s,&cfg,2)==WC_OK);CHECK(s.pack.capacity==40);CHECK(wc_craft(&s,&cfg,2)==WC_UNAVAILABLE);
    CHECK(wc_craft(&s,&cfg,3)==WC_INVALID);
    WCState before=s;CHECK(wc_craft(&s,&cfg,0)==WC_SUPPLIES);CHECK(memcmp(&s,&before,sizeof(s))==0);
    int cost[WC_ITEMS]={0},reward[WC_ITEMS]={0};cost[0]=-1;CHECK(wc_exchange(&s,cost,reward,false)==WC_INVALID);
    cost[0]=0;reward[0]=-1;CHECK(wc_exchange(&s,cost,reward,false)==WC_INVALID);reward[0]=10001;CHECK(wc_exchange(&s,cost,reward,false)==WC_INVALID);
    reward[0]=0;cost[0]=10001;CHECK(wc_exchange(&s,cost,reward,false)==WC_INVALID);
    s=fresh();s.pack.items[WC_STONE]=24;s.pantry.items[WC_TIMBER]=2;before=s;
    CHECK(wc_craft(&s,&cfg,0)==WC_CAPACITY);CHECK(memcmp(&s,&before,sizeof(s))==0);
    s.pack.items[WC_STONE]=22;s.pack.items[WC_TIMBER]=2;CHECK(wc_craft(&s,&cfg,0)==WC_OK);CHECK(wc_used(&s.pack)==24);
    CHECK(wc_transfer(&s,WC_STONE,4,true)==WC_OK);CHECK(s.pantry.items[WC_STONE]==4);
    CHECK(wc_transfer(&s,WC_STONE,4,false)==WC_OK);CHECK(wc_transfer(&s,WC_SEED,1,false)==WC_CAPACITY);
    CHECK(wc_transfer(&s,-1,1,true)==WC_INVALID);CHECK(wc_transfer(&s,0,0,true)==WC_INVALID);
    CHECK(wc_transfer(&s,WC_TIMBER,1,true)==WC_SUPPLIES);
    s.attached=false;CHECK(wc_craft(&s,&cfg,0)==WC_UNAVAILABLE);CHECK(wc_transfer(&s,0,1,true)==WC_UNAVAILABLE);
    for(int i=0;i<WC_ITEMS;i++) CHECK(strlen(wc_item_name(i))>0);CHECK(strcmp(wc_item_name(99),"Unknown")==0);
    for(int i=0;i<=WC_TOO_FAR;i++) CHECK(strlen(wc_result_text((WCResult)i))>0);CHECK(strlen(wc_result_text((WCResult)100))>0);
}
static void garden_gather_vitals(void) {
    WCState s=fresh();CHECK(wc_garden(&s,&cfg,-1,0)==WC_INVALID);CHECK(wc_garden(&s,&cfg,0,3)==WC_INVALID);
    CHECK(wc_garden(&s,&cfg,0,0)==WC_OK);CHECK(wc_garden(&s,&cfg,0,1)==WC_BLOCKED);
    CHECK(wc_garden(&s,&cfg,1,1)==WC_OK);CHECK(wc_garden(&s,&cfg,0,2)==WC_UNAVAILABLE);
    wc_pause(&s,true);ticks(&s,(WCInput){0},100);CHECK(s.clock==0);wc_pause(&s,false);
    s.clock=45;s.pack.items[WC_STONE]=24;WCState before=s;
    CHECK(wc_garden(&s,&cfg,0,2)==WC_CAPACITY);CHECK(memcmp(&s,&before,sizeof(s))==0);
    s.pack.items[WC_STONE]=0;CHECK(wc_garden(&s,&cfg,0,2)==WC_OK);CHECK(s.pack.items[WC_SUNROOT]==3);
    CHECK(wc_garden(&s,&cfg,0,2)==WC_UNAVAILABLE);s.clock=600;CHECK(wc_garden(&s,&cfg,1,2)==WC_OK);
    s.pantry.items[WC_SEED]=0;s.pack.items[WC_SEED]=0;CHECK(wc_garden(&s,&cfg,0,0)==WC_SUPPLIES);
    CHECK(wc_gather(&s,0)==WC_UNAVAILABLE);s.attached=false;CHECK(wc_garden(&s,&cfg,0,0)==WC_UNAVAILABLE);
    CHECK(wc_rest(&s)==WC_UNAVAILABLE);CHECK(wc_gather(&s,-1)==WC_INVALID);CHECK(wc_gather(&s,0)==WC_TOO_FAR);
    s.player=(WCVec){-10,0,23};CHECK(wc_gather(&s,0)==WC_OK);CHECK(wc_gather(&s,0)==WC_UNAVAILABLE);
    s.clock+=120;s.pack.items[WC_STONE]=20;before=s;CHECK(wc_gather(&s,0)==WC_CAPACITY);CHECK(memcmp(&s,&before,sizeof(s))==0);
    s=fresh();s.attached=false;s.player=(WCVec){100,0,100};s.stamina=1;s.rested=120;s.meal=180;
    ticks(&s,(WCInput){.forward=1,.turbo=true},30);CHECK(s.stamina<1);CHECK(s.sprint_latched);CHECK(s.rested<120 && s.meal<180);
    double effort=s.rested;ticks(&s,(WCInput){0},240);CHECK(s.stamina>20);CHECK(!s.sprint_latched);NEAR(s.rested,effort,1e-9);
    s.rested=0;ticks(&s,(WCInput){0},240);CHECK(s.stamina>40);
    ticks(&s,(WCInput){.jump=true},1);CHECK(!s.grounded);ticks(&s,(WCInput){0},180);CHECK(s.grounded);
    s=fresh();s.health=10;s.stamina=1;double clock=s.clock;CHECK(wc_rest(&s)==WC_OK);CHECK(s.health==100 && s.stamina==100 && s.rested==120);CHECK(s.clock==clock);
}
static void persistence(void) {
    WCState s=fresh();s.clock=33;s.player=(WCVec){0,-1,10};s.gate_open=true;s.context=WC_HELM;s.speed=25;
    s.satchel=true;s.pack.capacity=40;s.crops[0]=(WCCrop){0,1,45,true};s.pack.items[WC_STONE]=12;s.help=false;
    unsigned char bytes[WC_SAVE_MAX];size_t size=wc_encode(&s,bytes);CHECK(size>0);
    WCState loaded=fresh();loaded.help=false;CHECK(wc_decode(&loaded,bytes,size));CHECK(loaded.seed==s.seed);CHECK(loaded.gate_open);
    CHECK(loaded.context==WC_FOOT && loaded.speed==0);CHECK(loaded.pack.items[WC_STONE]==12);CHECK(loaded.crops[0].treated);CHECK(!loaded.help);
    CHECK(loaded.player.z==10 && loaded.player.y==-1);
    WCState before=loaded;
    for(size_t i=0;i<size;i++) { bytes[i]^=0x80;CHECK(!wc_decode(&loaded,bytes,size));CHECK(memcmp(&loaded,&before,sizeof(loaded))==0);bytes[i]^=0x80; }
    CHECK(!wc_decode(&loaded,bytes,size-1));CHECK(!wc_decode(&loaded,bytes,0));
    loaded.focused=false;CHECK(wc_decode(&loaded,bytes,size));CHECK(loaded.context==WC_MENU);
    char err[160];CHECK(wc_save_file(&s,"test-journey.sav",err,sizeof(err)));s.clock=50;CHECK(wc_save_file(&s,"test-journey.sav",err,sizeof(err)));
    CHECK(wc_load_file(&loaded,"test-journey.sav",false,err,sizeof(err)));CHECK(loaded.clock==50);
    CHECK(wc_load_file(&loaded,"test-journey.sav",true,err,sizeof(err)));CHECK(loaded.clock==33);
    FILE *f=fopen("test-journey.sav","wb");fputs("interrupted",f);fclose(f);before=loaded;
    CHECK(!wc_load_file(&loaded,"test-journey.sav",false,err,sizeof(err)));CHECK(memcmp(&loaded,&before,sizeof(loaded))==0);
    CHECK(wc_load_file(&loaded,"test-journey.sav",true,err,sizeof(err)));
    CHECK(wc_save_file(&s,"test-journey.sav",err,sizeof(err)));
    CHECK(!wc_save_file(&s,"missing-dir/journey.sav",err,sizeof(err)));CHECK(!wc_load_file(&loaded,"missing-dir/journey.sav",false,err,sizeof(err)));
    char huge[1100];memset(huge,'a',1099);huge[1099]=0;CHECK(!wc_save_file(&s,huge,err,sizeof(err)));CHECK(!wc_load_file(&s,huge,false,err,sizeof(err)));
    s.health=NAN;CHECK(wc_encode(&s,bytes)==0);CHECK(!wc_save_file(&s,"test-journey.sav",err,sizeof(err)));
    remove("test-journey.sav");remove("test-journey.sav.prev");
}
static void validation(void) {
    WCState s=fresh();s.clock=-1;CHECK(!wc_validate(&s));s=fresh();s.pitch=2;CHECK(!wc_validate(&s));
    s=fresh();s.player.x=8;CHECK(!wc_validate(&s));s=fresh();s.pack.items[0]=-1;CHECK(!wc_validate(&s));
    s=fresh();s.pack.items[0]=25;CHECK(!wc_validate(&s));s=fresh();s.crops[0].crop=4;CHECK(!wc_validate(&s));
    s=fresh();s.depleted[0]=NAN;CHECK(!wc_validate(&s));s=fresh();s.health=101;CHECK(!wc_validate(&s));
    uint32_t seed=1,other=99;
    NEAR(wc_terrain(0,0,&seed),0,1e-9);NEAR(wc_terrain(63,-63,&other),0,1e-9);
    CHECK(wc_terrain(200,-200,&seed)!=wc_terrain(200,-200,&other));
    double a=wc_terrain(-128-1e-7,256,&seed),b=wc_terrain(-128+1e-7,256,&seed);NEAR(a,b,1e-5);
    CHECK(isfinite(wc_terrain(1e10,-1e10,&seed)));NEAR(wc_terrain(-700,890,&seed),wc_terrain(-700,890,&seed),1e-12);
}
static void bad_content(const char *source) {
    FILE *f=fopen("bad-content.lua","wb");fputs(source,f);fclose(f);
    WCConfig c=cfg,before=c;char error[256];CHECK(!wc_load_content(&c,"bad-content.lua",error,sizeof(error)));CHECK(memcmp(&c,&before,sizeof(c))==0);remove("bad-content.lua");
}

static WCRules *rules_source(const char *source) {
    FILE *f=fopen("test-rules.lua","wb");fputs(source,f);fclose(f);char error[256];
    WCRules *r=wc_rules_open("test-rules.lua",error,sizeof(error));remove("test-rules.lua");return r;
}
static void scripts(const char *content_path) {
    char path[1024],error[256];snprintf(path,sizeof(path),"%s",content_path);
    char *slash=strrchr(path,'/');if(!slash)slash=strrchr(path,'\\');CHECK(slash!=NULL);strcpy(slash+1,"rules.lua");
    WCRules *r=wc_rules_open(path,error,sizeof(error));CHECK(r!=NULL);bool save;
    WCState s=fresh();s.health=3;CHECK(wc_rules_action(r,&s,&cfg,WC_BED,0,0,&save)==WC_OK);CHECK(s.health==100);
    CHECK(wc_rules_action(r,&s,&cfg,WC_BED,1,0,&save)==WC_OK && save);
    CHECK(wc_rules_action(r,&s,&cfg,WC_PANTRY,0,0,&save)==WC_OK);CHECK(s.selection==1);
    s.pack.items[WC_STONE]=2;CHECK(wc_rules_action(r,&s,&cfg,WC_PANTRY,1,0,&save)==WC_OK);
    CHECK(wc_rules_action(r,&s,&cfg,WC_PANTRY,2,0,&save)==WC_OK);CHECK(s.pack.items[WC_STONE]==2);
    s.pack.items[WC_TIMBER]=2;CHECK(wc_rules_action(r,&s,&cfg,WC_WORKSHOP,0,0,&save)==WC_OK);
    CHECK(wc_rules_action(r,&s,&cfg,WC_PLANTER,0,0,&save)==WC_OK);
    CHECK(wc_rules_action(r,&s,&cfg,99,0,0,&save)==WC_INVALID);
    WCState before=s;s.attached=false;CHECK(wc_rules_action(r,&s,&cfg,WC_BED,0,0,&save)==WC_INVALID);s=before;
    wc_rules_close(r);CHECK(wc_rules_action(NULL,&s,&cfg,0,0,0,&save)==WC_UNAVAILABLE);wc_rules_close(NULL);
    CHECK(rules_source("return 2")==NULL);CHECK(rules_source("return {}")==NULL);CHECK(rules_source("syntax!")==NULL);
    const char *bad[]={"return {on_action=function() while true do end end}","return {on_action=function() return 4 end}",
        "return {on_action=function() return {command='craft'} end}","return {on_action=function() return {command='craft',id=-1} end}",
        "return {on_action=function() return {command='unsupported'} end}"};
    for(int i=0;i<5;i++) {r=rules_source(bad[i]);CHECK(r!=NULL);before=s;CHECK(wc_rules_action(r,&s,&cfg,0,0,0,&save)==WC_INVALID);CHECK(memcmp(&s,&before,sizeof(s))==0);wc_rules_close(r);}
    /* A script edit changes behavior, while C still owns the transaction. */
    r=rules_source("return {on_action=function(panel,index,slot) return {command='craft',id=1} end}");s=fresh();s.pack.items[WC_FIBER]=3;
    CHECK(wc_rules_action(r,&s,&cfg,WC_WORKSHOP,0,0,&save)==WC_OK);CHECK(s.pack.items[WC_TWINE]==2);wc_rules_close(r);
    WCConfig changed=cfg;changed.recipes[0].cost[WC_TIMBER]=1;changed.recipes[0].reward[WC_PLANK]=4;
    s=fresh();s.pack.items[WC_TIMBER]=1;CHECK(wc_craft(&s,&changed,0)==WC_OK);CHECK(s.pack.items[WC_PLANK]==4);
}
int main(int argc,char **argv) {
    if(argc!=2) return 2;char error[256];wc_defaults(&cfg);
    CHECK(wc_load_content(&cfg,argv[1],error,sizeof(error)));CHECK(cfg.object_count==8);
    if(failures) { fprintf(stderr,"Content error: %s\n",error);return 1; }
    motion();input_contexts();interaction();boarding();economy();garden_gather_vitals();persistence();validation();scripts(argv[1]);
    bad_content("return 3");bad_content("return {tuning={}}");bad_content("while true do end");bad_content("!syntax");
    bad_content("return {tuning={speed=5,turbo=5,turn=.38,walk=3,sprint=5,jump=5,gravity=15,eye=1.62},objects={5}}");
    bad_content("return {tuning={speed=5,turbo=5,turn=.38,walk=3,sprint=5,jump=5,gravity=15,eye=1.62},objects={{id=1,kind=0,slot=0,min={0,0,0},max={1,1,1},color={2,0,0}}}}");
    CHECK(!wc_load_content(&cfg,"missing.lua",error,sizeof(error)));
    printf("Core checks: %d; failures: %d\n",checks,failures);return failures?1:0;
}
