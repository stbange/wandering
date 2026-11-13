#include "content.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static bool number(lua_State *l,const char *key,double *out,double min,double max) {
    lua_getfield(l,-1,key);bool ok=lua_isnumber(l,-1);double n=lua_tonumber(l,-1);lua_pop(l,1);
    if(!ok || !isfinite(n) || n<min || n>max) return false;
    *out=n;return true;
}
static bool vector(lua_State *l,const char *key,WCVec *out) {
    lua_getfield(l,-1,key);if(!lua_istable(l,-1)) { lua_pop(l,1);return false; }
    double *v[]={&out->x,&out->y,&out->z};bool ok=true;
    for(int i=0;i<3;i++) { lua_rawgeti(l,-1,i+1);if(!lua_isnumber(l,-1)) ok=false;*v[i]=lua_tonumber(l,-1);if(!isfinite(*v[i]) || fabs(*v[i])>100) ok=false;lua_pop(l,1); }
    lua_pop(l,1);return ok;
}
static void instruction_limit(lua_State *l,lua_Debug *ar) { (void)ar;luaL_error(l,"Content exceeded its instruction budget"); }

static bool quantities(lua_State *l,const char *key,int out[WC_ITEMS]) {
    lua_getfield(l,-1,key);if(!lua_istable(l,-1)){lua_pop(l,1);return false;}
    bool ok=lua_rawlen(l,-1)==WC_ITEMS;
    for(int i=0;i<WC_ITEMS;i++){
        lua_rawgeti(l,-1,i+1);double n=lua_tonumber(l,-1);
        if(!lua_isinteger(l,-1)||n<0||n>1000)ok=false;else out[i]=(int)n;
        lua_pop(l,1);
    }
    lua_pop(l,1);return ok;
}
static bool definitions(lua_State *l,WCConfig *c) {
    lua_getfield(l,-1,"recipes");
    if(!lua_istable(l,-1)||lua_rawlen(l,-1)<1||lua_rawlen(l,-1)>WC_MAX_RECIPES){lua_pop(l,1);return false;}
    bool ok=true;c->recipe_count=(int)lua_rawlen(l,-1);
    for(int i=0;ok&&i<c->recipe_count;i++) {
        lua_rawgeti(l,-1,i+1);if(!lua_istable(l,-1)){lua_pop(l,1);ok=false;break;}
        WCRecipe *r=&c->recipes[i];double id;
        ok=number(l,"id",&id,1,10000)&&id==(int)id&&quantities(l,"cost",r->cost)&&quantities(l,"reward",r->reward);
        if(ok){r->id=(int)id;for(int j=0;j<i;j++)if(c->recipes[j].id==r->id)ok=false;}
        lua_getfield(l,-1,"name");const char *name=lua_tostring(l,-1);
        if(!name||strlen(name)>39)ok=false;else snprintf(r->name,sizeof(r->name),"%s",name);
        lua_pop(l,1);lua_getfield(l,-1,"satchel");r->satchel=lua_toboolean(l,-1)!=0;lua_pop(l,2);
    }
    lua_pop(l,1);lua_getfield(l,-1,"crops");
    if(!lua_istable(l,-1)||lua_rawlen(l,-1)!=2){lua_pop(l,1);return false;}
    for(int i=0;ok&&i<2;i++) {
        lua_rawgeti(l,-1,i+1);double yield;
        if(!lua_istable(l,-1)||!number(l,"duration",&c->crop_duration[i],1,60)||!number(l,"yield",&yield,1,20)||yield!=(int)yield)ok=false;
        else c->crop_yield[i]=(int)yield;
        lua_pop(l,1);
    }
    lua_pop(l,1);return ok;
}
bool wc_load_content(WCConfig *c,const char *path,char *error,size_t size) {
    lua_State *l=luaL_newstate();if(!l) { snprintf(error,size,"Cannot allocate Lua state");return false; }
    /* Content tables only: no os/io/package/debug or arbitrary filesystem access. */
    luaL_requiref(l,"_G",luaopen_base,1);lua_pop(l,1);
    lua_pushnil(l);lua_setglobal(l,"dofile");lua_pushnil(l);lua_setglobal(l,"loadfile");
    lua_sethook(l,instruction_limit,LUA_MASKCOUNT,100000);
    bool ok=luaL_loadfile(l,path)==LUA_OK && lua_pcall(l,0,1,0)==LUA_OK;
    if(!ok) { snprintf(error,size,"Lua content: %s",lua_tostring(l,-1));lua_close(l);return false; }
    WCConfig candidate;wc_defaults(&candidate);
    if(!lua_istable(l,-1)) ok=false;
    if(ok) {
        lua_getfield(l,-1,"tuning");ok=lua_istable(l,-1);
        if(ok) ok=number(l,"speed",&candidate.speed,.1,10) && number(l,"turbo",&candidate.turbo,1,5) &&
            number(l,"turn",&candidate.turn,.1,1) && number(l,"walk",&candidate.walk,1,8) &&
            number(l,"sprint",&candidate.sprint,1,12) && number(l,"jump",&candidate.jump,1,10) &&
            number(l,"gravity",&candidate.gravity,1,30) && number(l,"eye",&candidate.eye,1.4,1.8);
        lua_pop(l,1);lua_getfield(l,-1,"objects");
        if(!lua_istable(l,-1) || lua_rawlen(l,-1)>WC_MAX_OBJECTS) ok=false;
        if(ok) candidate.object_count=(int)lua_rawlen(l,-1);
        for(int i=0;ok && i<candidate.object_count;i++) {
            lua_rawgeti(l,-1,i+1);if(!lua_istable(l,-1)) {ok=false;lua_pop(l,1);break;}
            WCObject *o=&candidate.objects[i];double id,kind,slot;WCVec color;
            ok=number(l,"id",&id,1,10000) && number(l,"kind",&kind,0,WC_MAP) && number(l,"slot",&slot,0,1) &&
               vector(l,"min",&o->min) && vector(l,"max",&o->max) && vector(l,"color",&color);
            if(ok) {
                o->id=(int)id;o->kind=(int)kind;o->slot=(int)slot;
                ok=id==o->id && kind==o->kind && slot==o->slot && o->max.x>o->min.x && o->max.y>o->min.y && o->max.z>o->min.z;
                o->color[0]=(float)color.x;o->color[1]=(float)color.y;o->color[2]=(float)color.z;
                if(color.x<0 || color.x>1 || color.y<0 || color.y>1 || color.z<0 || color.z>1) ok=false;
                for(int j=0;j<i;j++) if(candidate.objects[j].id==o->id) ok=false;
                lua_getfield(l,-1,"solid");o->solid=lua_toboolean(l,-1)!=0;lua_pop(l,1);
            }
            lua_pop(l,1);
        }
    }
    if(ok) {lua_pop(l,1);ok=definitions(l,&candidate);}
    if(ok) *c=candidate;else snprintf(error,size,"Invalid content table: check tuning, objects, unique IDs, recipes and crops");
    lua_close(l);return ok;
}

struct WCRules { lua_State *lua; };
WCRules *wc_rules_open(const char *path,char *error,size_t size) {
    WCRules *r=calloc(1,sizeof(*r));if(!r)return NULL;
    r->lua=luaL_newstate();if(!r->lua){free(r);return NULL;}
    lua_State *l=r->lua;lua_sethook(l,instruction_limit,LUA_MASKCOUNT,100000);
    if(luaL_loadfile(l,path)!=LUA_OK || lua_pcall(l,0,1,0)!=LUA_OK) {
        snprintf(error,size,"Rules script: %s",lua_tostring(l,-1));wc_rules_close(r);return NULL;
    }
    if(!lua_istable(l,-1)){snprintf(error,size,"Rules must return a table");wc_rules_close(r);return NULL;}
    lua_getfield(l,-1,"on_action");bool valid=lua_isfunction(l,-1);lua_pop(l,1);
    if(!valid){snprintf(error,size,"Rules must define on_action");wc_rules_close(r);return NULL;}
    return r;
}
void wc_rules_close(WCRules *r) {if(r){lua_close(r->lua);free(r);}}
WCResult wc_rules_action(WCRules *r,WCState *s,const WCConfig *cfg,int panel,int index,int slot,bool *save) {
    *save=false;if(!r)return WC_UNAVAILABLE;
    lua_State *l=r->lua;lua_getfield(l,1,"on_action");
    lua_pushinteger(l,panel);lua_pushinteger(l,index);lua_pushinteger(l,slot);
    if(lua_pcall(l,3,1,0)!=LUA_OK){lua_settop(l,1);return WC_INVALID;}
    if(!lua_istable(l,-1)){lua_pop(l,1);return WC_INVALID;}
    lua_getfield(l,-1,"command");const char *cmd=lua_tostring(l,-1);
    lua_getfield(l,-2,"id");bool has_id=lua_isinteger(l,-1);lua_Integer raw=lua_tointeger(l,-1);
    int id=raw>=0&&raw<10000?(int)raw:-1;
    WCResult result=WC_INVALID;
    if(cmd && wc_inside(s)) {
        if(strcmp(cmd,"craft")==0 && has_id)result=wc_craft(s,cfg,id);
        else if(strcmp(cmd,"garden")==0 && has_id)result=wc_garden(s,cfg,slot,id);
        else if(strcmp(cmd,"rest")==0)result=wc_rest(s);
        else if(strcmp(cmd,"save")==0){*save=true;result=WC_OK;}
        else if(strcmp(cmd,"select-item")==0){s->selection=(s->selection+1)%WC_ITEMS;result=WC_OK;}
        else if(strcmp(cmd,"deposit")==0)result=wc_transfer(s,s->selection,1,true);
        else if(strcmp(cmd,"withdraw")==0)result=wc_transfer(s,s->selection,1,false);
    }
    lua_settop(l,1);return result;
}
