#ifndef WC_CONTENT_H
#define WC_CONTENT_H
#include "wc.h"
typedef struct {
    float r, g, b;
} WcColor;
typedef struct {
    WcVec p, size;
    int material;
} WcArtBox;
typedef struct {
    WcColor palette[16];
    WcArtBox home[256];
    int box_count;
    char title[64];
} WcArt;
bool wc_load_content(WcDefs *defs, const char *path, char *error, size_t cap);
bool wc_load_art(WcArt *art, const char *path, char *error, size_t cap);
#endif
