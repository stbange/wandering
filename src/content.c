#include "content.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

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
    if(ok) *c=candidate;else snprintf(error,size,"Invalid content table: check tuning, object bounds and unique IDs");
    lua_close(l);return ok;
}
