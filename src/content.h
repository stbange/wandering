#ifndef WC_CONTENT_H
#define WC_CONTENT_H
#include "core.h"
bool wc_load_content(WCConfig *c,const char *path,char *error,size_t size);
typedef struct WCRules WCRules;
WCRules *wc_rules_open(const char *path,char *error,size_t size);
void wc_rules_close(WCRules *rules);
WCResult wc_rules_action(WCRules *rules,WCState *s,const WCConfig *cfg,int panel,int index,int slot,bool *save);
#endif
