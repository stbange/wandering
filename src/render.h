#ifndef WC_RENDER_H
#define WC_RENDER_H
#include "core.h"
#include "input.h"
bool wc_render_init(const char *base,char *error,size_t size);
void wc_render(const WCState *s,const WCConfig *cfg,const WCControls *controls,int width,int height);
bool wc_screenshot(const char *path,int width,int height);
void wc_render_shutdown(void);
#endif
