#ifndef WC_SAVE_H
#define WC_SAVE_H
#include "core.h"
#define WC_SAVE_MAX 2048
size_t wc_encode(const WCState *s,unsigned char out[WC_SAVE_MAX]);
bool wc_decode(WCState *live,const unsigned char *bytes,size_t length);
bool wc_save_file(const WCState *s,const char *path,char *error,size_t error_size);
bool wc_load_file(WCState *s,const char *path,bool previous,char *error,size_t error_size);
#endif
