#include "save.h"
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

static void u32(unsigned char **p,uint32_t n) { for(int i=0;i<4;i++) *(*p)++=(unsigned char)(n>>(8*i)); }
static uint32_t r32(const unsigned char **p) { uint32_t n=0;for(int i=0;i<4;i++) n|=(uint32_t)*(*p)++<<(8*i);return n; }
static void f64(unsigned char **p,double x) { uint64_t n;memcpy(&n,&x,8);for(int i=0;i<8;i++) *(*p)++=(unsigned char)(n>>(8*i)); }
static double r64(const unsigned char **p) { uint64_t n=0;double x;for(int i=0;i<8;i++) n|=(uint64_t)*(*p)++<<(8*i);memcpy(&x,&n,8);return x; }
static uint32_t checksum(const unsigned char *p,size_t n) { uint32_t c=2166136261u;for(size_t i=0;i<n;i++) c=(c^p[i])*16777619u;return c; }
size_t wc_encode(const WCState *s,unsigned char out[WC_SAVE_MAX]) {
    if(!wc_validate(s)) return 0;
    unsigned char *p=out;memcpy(p,"WCASTLE1",8);p+=8;u32(&p,WC_VERSION);u32(&p,0);u32(&p,0);
    u32(&p,s->seed);f64(&p,s->clock);
    double v[]={s->castle.x,s->castle.y,s->castle.z,s->player.x,s->player.y,s->player.z,s->heading,s->yaw,s->pitch,
        s->vertical,s->stair_length,s->stair_drop,s->health,s->stamina,s->rested,s->meal,s->recovery};
    for(size_t i=0;i<sizeof(v)/sizeof(*v);i++) f64(&p,v[i]);
    u32(&p,(s->attached?1u:0u)|(s->grounded?2u:0u)|(s->gate_open?4u:0u)|(s->sprint_latched?8u:0u)|(s->satchel?16u:0u));
    for(int i=0;i<WC_ITEMS;i++) { u32(&p,(uint32_t)s->pack.items[i]);u32(&p,(uint32_t)s->pantry.items[i]); }
    for(int i=0;i<2;i++) { u32(&p,(uint32_t)(s->crops[i].crop+1));f64(&p,s->crops[i].planted);f64(&p,s->crops[i].duration);u32(&p,s->crops[i].treated?1:0); }
    for(int i=0;i<16;i++) f64(&p,s->depleted[i]);
    u32(&p,s->knowledge);u32(&p,s->coins);u32(&p,s->claims);
    size_t size=(size_t)(p-out);p=out+12;u32(&p,(uint32_t)size);u32(&p,checksum(out+20,size-20));return size;
}
bool wc_decode(WCState *live,const unsigned char *bytes,size_t length) {
    WCConfig c;WCState candidate;wc_defaults(&c);wc_init(&candidate,&c,0);
    unsigned char reference[WC_SAVE_MAX];size_t expected=wc_encode(&candidate,reference);
    if(length!=expected || memcmp(bytes,"WCASTLE1",8)!=0) return false;
    const unsigned char *p=bytes+8;
    if(r32(&p)!=WC_VERSION || r32(&p)!=length || r32(&p)!=checksum(bytes+20,length-20)) return false;
    candidate.seed=r32(&p);candidate.clock=r64(&p);
    double *v[]={&candidate.castle.x,&candidate.castle.y,&candidate.castle.z,&candidate.player.x,&candidate.player.y,&candidate.player.z,
        &candidate.heading,&candidate.yaw,&candidate.pitch,&candidate.vertical,&candidate.stair_length,&candidate.stair_drop,
        &candidate.health,&candidate.stamina,&candidate.rested,&candidate.meal,&candidate.recovery};
    for(size_t i=0;i<sizeof(v)/sizeof(*v);i++) *v[i]=r64(&p);
    uint32_t flags=r32(&p);if(flags>31) return false;
    candidate.attached=(flags&1)!=0;candidate.grounded=(flags&2)!=0;candidate.gate_open=(flags&4)!=0;
    candidate.sprint_latched=(flags&8)!=0;candidate.satchel=(flags&16)!=0;candidate.pack.capacity=candidate.satchel?40:24;
    for(int i=0;i<WC_ITEMS;i++) { candidate.pack.items[i]=(int)r32(&p);candidate.pantry.items[i]=(int)r32(&p); }
    for(int i=0;i<2;i++) {
        uint32_t crop=r32(&p);if(crop>2) return false;candidate.crops[i].crop=(int)crop-1;
        candidate.crops[i].planted=r64(&p);candidate.crops[i].duration=r64(&p);
        uint32_t treated=r32(&p);if(treated>1) return false;candidate.crops[i].treated=treated!=0;
    }
    for(int i=0;i<16;i++) candidate.depleted[i]=r64(&p);
    candidate.knowledge=r32(&p);candidate.coins=r32(&p);candidate.claims=r32(&p);
    candidate.help=live->help;candidate.context=WC_FOOT;candidate.speed=0;
    candidate.focused=live->focused;
    if(!candidate.focused) wc_pause(&candidate,true);
    if(!wc_validate(&candidate)) return false;
    snprintf(candidate.message,sizeof(candidate.message),"Journey restored. Parked and on foot.");
    *live=candidate;return true;
}
static bool read_bytes(const char *path,unsigned char *bytes,size_t *size) {
    FILE *f=fopen(path,"rb");if(!f) return false;
    *size=fread(bytes,1,WC_SAVE_MAX,f);bool ok=!ferror(f) && feof(f);fclose(f);return ok;
}
static bool publish(const char *from,const char *to) {
#ifdef _WIN32
    return MoveFileExA(from,to,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return rename(from,to)==0;
#endif
}
static bool write_bytes(const char *path,const unsigned char *bytes,size_t size) {
    FILE *f=fopen(path,"wb");if(!f) return false;
    bool ok=fwrite(bytes,1,size,f)==size && fflush(f)==0;
#ifndef _WIN32
    if(ok) ok=fsync(fileno(f))==0;
#endif
    if(fclose(f)!=0) ok=false;
    return ok;
}
bool wc_save_file(const WCState *s,const char *path,char *error,size_t error_size) {
    unsigned char bytes[WC_SAVE_MAX],old[WC_SAVE_MAX];size_t size=wc_encode(s,bytes),old_size=0;
    char temp[1024],prev[1024],prev_temp[1024];
    if(strlen(path)>990 || size==0) { snprintf(error,error_size,"Save rejected: invalid state or path.");return false; }
    snprintf(temp,sizeof(temp),"%s.tmp",path);snprintf(prev,sizeof(prev),"%s.prev",path);snprintf(prev_temp,sizeof(prev_temp),"%s.prev.tmp",path);
    if(!write_bytes(temp,bytes,size)) { snprintf(error,error_size,"Cannot write save. Live progress is unchanged.");return false; }
    WCState check=*s;
    if(read_bytes(path,old,&old_size) && wc_decode(&check,old,old_size)) {
        if(!write_bytes(prev_temp,old,old_size) || !publish(prev_temp,prev)) {
            snprintf(error,error_size,"Cannot preserve previous save; publication cancelled.");return false;
        }
    }
    if(!publish(temp,path)) { snprintf(error,error_size,"Cannot publish save; previous journey retained.");return false; }
    snprintf(error,error_size,"Journey saved.");return true;
}
bool wc_load_file(WCState *s,const char *path,bool previous,char *error,size_t error_size) {
    char source[1024];if(strlen(path)>990) { snprintf(error,error_size,"Save path too long.");return false; }
    snprintf(source,sizeof(source),"%s%s",path,previous?".prev":"");
    unsigned char bytes[WC_SAVE_MAX];size_t size;
    if(!read_bytes(source,bytes,&size) || !wc_decode(s,bytes,size)) {
        snprintf(error,error_size,"Save missing, corrupt or incompatible. Use Recover previous in pause.");return false;
    }
    snprintf(error,error_size,"Journey restored. Parked and on foot.");return true;
}
