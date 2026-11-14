#include "content.h"
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static double field(lua_State *l, const char *key, double fallback) {
    lua_getfield(l, -1, key);
    double v = lua_isnumber(l, -1) ? lua_tonumber(l, -1) : fallback;
    if (!isfinite(v) || fabs(v) > 1000000)
        v = fallback;
    lua_pop(l, 1);
    return v;
}
static void string_field(lua_State *l, const char *key, char *out, size_t n) {
    lua_getfield(l, -1, key);
    snprintf(out, n, "%s", lua_isstring(l, -1) ? lua_tostring(l, -1) : "?");
    lua_pop(l, 1);
}
static int quantities(lua_State *l, const char *name, int out[WC_ITEMS]) {
    lua_getfield(l, -1, name);
    int valid = lua_istable(l, -1);
    for (int j = 0; j < ITEM_COUNT && valid; j++) {
        lua_rawgeti(l, -1, j + 1);
        out[j] = (int)lua_tointeger(l, -1);
        if (out[j] < 0 || out[j] > 80)
            valid = 0;
        lua_pop(l, 1);
    }
    lua_pop(l, 1);
    return valid;
}
static lua_State *open_file(const char *path, char *error, size_t cap) {
    lua_State *l = luaL_newstate();
    if (!l) {
        snprintf(error, cap, "Lua allocation failed");
        return NULL;
    }
    /* Content can compute definitions, but cannot access OS, files or networks. */
    luaL_requiref(l, "_G", luaopen_base, 1);
    lua_pop(l, 1);
    luaL_requiref(l, "math", luaopen_math, 1);
    lua_pop(l, 1);
    lua_pushnil(l);
    lua_setglobal(l, "dofile");
    lua_pushnil(l);
    lua_setglobal(l, "loadfile");
    if (luaL_dofile(l, path) || !lua_istable(l, -1)) {
        snprintf(error, cap, "Lua content: %s", lua_tostring(l, -1));
        lua_close(l);
        return NULL;
    }
    return l;
}
bool wc_load_content(WcDefs *defs, const char *path, char *error, size_t cap) {
    lua_State *l = open_file(path, error, cap);
    if (!l)
        return false;
    WcDefs d = {0};
    int valid = 1;
    d.speed = (float)field(l, "speed", 5);
    d.turbo = (float)field(l, "turbo", 5);
    d.turn = (float)field(l, "turn", .38);
    lua_getfield(l, -1, "items");
    if (!lua_istable(l, -1) || lua_rawlen(l, -1) != ITEM_COUNT)
        valid = 0;
    for (int i = 0; i < ITEM_COUNT && valid; i++) {
        lua_rawgeti(l, -1, i + 1);
        if (!lua_istable(l, -1)) {
            valid = 0;
            lua_pop(l, 1);
            break;
        }
        string_field(l, "name", d.items[i].name, 32);
        d.items[i].buy = (int)field(l, "buy", 0);
        d.items[i].sell = (int)field(l, "sell", 0);
        if (d.items[i].buy < 0 || d.items[i].sell < 0 || d.items[i].sell > d.items[i].buy)
            valid = 0;
        lua_pop(l, 1);
    }
    lua_pop(l, 1);
    lua_getfield(l, -1, "recipes");
    if (!lua_istable(l, -1))
        goto invalid;
    d.recipe_count = (int)lua_rawlen(l, -1);
    if (d.recipe_count < 1 || d.recipe_count > WC_RECIPES)
        valid = 0;
    for (int i = 0; i < d.recipe_count && valid; i++) {
        lua_rawgeti(l, -1, i + 1);
        if (!lua_istable(l, -1))
            goto invalid;
        WcRecipe *r = &d.recipes[i];
        string_field(l, "name", r->name, 48);
        r->output = (int)field(l, "output", -1);
        r->quantity = (int)field(l, "quantity", 1);
        r->unlock = (int)field(l, "unlock", 0);
        lua_getfield(l, -1, "cost");
        if (!lua_istable(l, -1))
            valid = 0;
        for (int j = 0; j < ITEM_COUNT && valid; j++) {
            lua_rawgeti(l, -1, j + 1);
            r->cost[j] = (int)lua_tointeger(l, -1);
            if (r->cost[j] < 0 || r->cost[j] > 80)
                valid = 0;
            lua_pop(l, 1);
        }
        lua_pop(l, 1);
        if (r->output < 0 || r->output >= ITEM_COUNT || r->quantity < 1 || r->quantity > 40)
            valid = 0;
        lua_pop(l, 1);
    }
    lua_pop(l, 1);
    lua_getfield(l, -1, "crops");
    if (!lua_istable(l, -1))
        goto invalid;
    for (int i = 0; i < 2; i++) {
        lua_rawgeti(l, -1, i + 1);
        if (!lua_istable(l, -1))
            goto invalid;
        d.growth[i] = field(l, "seconds", 0);
        d.yield[i] = (int)field(l, "yield", 0);
        if (d.growth[i] <= 0 || d.growth[i] > 3600 || d.yield[i] < 1 || d.yield[i] > 12)
            valid = 0;
        lua_pop(l, 1);
    }
    lua_pop(l, 1);
    valid &= quantities(l, "room_cost", d.room_cost);
    valid &= quantities(l, "salvage", d.salvage);
    valid &= quantities(l, "reserve", d.reserve);
    lua_getfield(l, -1, "equipment");
    if (!lua_istable(l, -1))
        goto invalid;
    for (int i = 0; i < 2; i++) {
        lua_rawgeti(l, -1, i + 1);
        if (!lua_istable(l, -1))
            goto invalid;
        WcGear *gear = i ? &d.spear : &d.staff;
        gear->damage = (int)field(l, "damage", 0);
        gear->power = (int)field(l, "power", 0);
        gear->armor = (float)field(l, "armor", 0);
        if (gear->damage < 1 || gear->damage > 100 || gear->power < 1 || gear->armor < 0 ||
            gear->armor > .8)
            valid = 0;
        lua_pop(l, 1);
    }
    lua_pop(l, 1);
    if (!isfinite(d.speed) || d.speed <= 0 || d.speed > 5 || d.turbo <= 0 || d.turbo > 5 ||
        !isfinite(d.turn) || d.turn <= 0 || d.turn > 1)
        valid = 0;
    lua_close(l);
    if (!valid) {
        snprintf(error, cap, "Invalid content definitions; original state retained");
        return false;
    }
    *defs = d;
    return true;
invalid:
    lua_close(l);
    snprintf(error, cap, "Invalid content table; original definitions retained");
    return false;
}
bool wc_load_art(WcArt *art, const char *path, char *error, size_t cap) {
    lua_State *l = open_file(path, error, cap);
    if (!l)
        return false;
    WcArt a = {0};
    string_field(l, "title", a.title, sizeof(a.title));
    lua_getfield(l, -1, "palette");
    if (!lua_istable(l, -1))
        goto invalid_art;
    for (int i = 0; i < 16; i++) {
        lua_rawgeti(l, -1, i + 1);
        if (!lua_istable(l, -1))
            goto invalid_art;
        a.palette[i] =
            (WcColor){(float)field(l, "r", .5), (float)field(l, "g", .5), (float)field(l, "b", .5)};
        lua_pop(l, 1);
    }
    lua_pop(l, 1);
    lua_getfield(l, -1, "home");
    if (!lua_istable(l, -1))
        goto invalid_art;
    a.box_count = (int)lua_rawlen(l, -1);
    if (a.box_count > 256) {
        lua_close(l);
        snprintf(error, cap, "Too many art objects");
        return false;
    }
    for (int i = 0; i < a.box_count; i++) {
        lua_rawgeti(l, -1, i + 1);
        if (!lua_istable(l, -1))
            goto invalid_art;
        WcArtBox *b = &a.home[i];
        b->p = (WcVec){field(l, "x", 0), field(l, "y", 0), field(l, "z", 0)};
        b->size = (WcVec){field(l, "w", 1), field(l, "h", 1), field(l, "d", 1)};
        b->material = (int)field(l, "m", 0);
        if (b->material < 0 || b->material > 15)
            b->material = 0;
        lua_pop(l, 1);
    }
    lua_close(l);
    *art = a;
    return true;
invalid_art:
    lua_close(l);
    snprintf(error, cap, "Invalid art table; original art retained");
    return false;
}
