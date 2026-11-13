#include "render.h"
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
#endif
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Explicitly loaded GL3 functions; no fixed-function compatibility dependency. */
#define GL_FUNCTIONS(X) \
 X(GLuint,CreateShader,(GLenum)) \
 X(void,ShaderSource,(GLuint,GLsizei,const GLchar *const*,const GLint*)) \
 X(void,CompileShader,(GLuint)) \
 X(void,GetShaderiv,(GLuint,GLenum,GLint*)) \
 X(void,GetShaderInfoLog,(GLuint,GLsizei,GLsizei*,GLchar*)) \
 X(GLuint,CreateProgram,(void)) \
 X(void,AttachShader,(GLuint,GLuint)) \
 X(void,LinkProgram,(GLuint)) \
 X(void,GetProgramiv,(GLuint,GLenum,GLint*)) \
 X(void,GetProgramInfoLog,(GLuint,GLsizei,GLsizei*,GLchar*)) \
 X(void,DeleteShader,(GLuint)) \
 X(void,DeleteProgram,(GLuint)) \
 X(void,UseProgram,(GLuint)) \
 X(GLint,GetUniformLocation,(GLuint,const GLchar*)) \
 X(void,UniformMatrix4fv,(GLint,GLsizei,GLboolean,const GLfloat*)) \
 X(void,Uniform1i,(GLint,GLint)) \
 X(void,Uniform1f,(GLint,GLfloat)) \
 X(void,Uniform3f,(GLint,GLfloat,GLfloat,GLfloat)) \
 X(void,GenVertexArrays,(GLsizei,GLuint*)) \
 X(void,BindVertexArray,(GLuint)) \
 X(void,DeleteVertexArrays,(GLsizei,const GLuint*)) \
 X(void,GenBuffers,(GLsizei,GLuint*)) \
 X(void,BindBuffer,(GLenum,GLuint)) \
 X(void,BufferData,(GLenum,GLsizeiptr,const void*,GLenum)) \
 X(void,DeleteBuffers,(GLsizei,const GLuint*)) \
 X(void,EnableVertexAttribArray,(GLuint)) \
 X(void,VertexAttribPointer,(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*))
#ifndef APIENTRY
#define APIENTRY
#endif
#define DECLARE(ret,name,args) static ret (APIENTRY *p##name) args;
GL_FUNCTIONS(DECLARE)

typedef struct { float p[3],color[3],normal[3],uv[2]; } Vertex;
typedef struct { WCVec p[256];int count; } Mesh;
static Mesh cube,roof,crown;
static Vertex vertices[240000];static int count;
static GLuint program,vbo,vao,texture;
static GLint matrix_loc,mode_loc,sky_loc,day_loc;
static WCVec eye;static double view_yaw,view_pitch;
static unsigned char font[128][7],font_width[128];
static const float cream[3]={.93f,.89f,.74f},gold[3]={.91f,.68f,.34f},muted[3]={.59f,.69f,.68f};

static char *read_file(const char *path) {
    FILE *f=fopen(path,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);
    if(n<0 || n>4*1024*1024) {fclose(f);return NULL;}
    char *s=malloc((size_t)n+1);if(!s){fclose(f);return NULL;}
    if(fread(s,1,(size_t)n,f)!=(size_t)n){free(s);fclose(f);return NULL;}s[n]=0;fclose(f);return s;
}
static GLuint shader(GLenum type,const char *source,char *error,size_t size) {
#ifdef __EMSCRIPTEN__
    const char *header="#version 300 es\nprecision highp float;\n";
#else
    const char *header="#version 330 core\n";
#endif
    GLuint id=pCreateShader(type);const char *parts[]={header,source};pShaderSource(id,2,parts,NULL);pCompileShader(id);
    GLint ok;pGetShaderiv(id,GL_COMPILE_STATUS,&ok);if(!ok) {pGetShaderInfoLog(id,(GLsizei)size,NULL,error);pDeleteShader(id);return 0;}return id;
}
static bool mesh_load(Mesh *m,const char *path) {
    FILE *f=fopen(path,"r");if(!f) return false;WCVec points[128];int n=0;char line[256];m->count=0;
    while(fgets(line,sizeof(line),f)) {
        WCVec p;int a,b,c;
        if(sscanf(line,"v %lf %lf %lf",&p.x,&p.y,&p.z)==3 && n<128) points[n++]=p;
        else if(sscanf(line,"f %d %d %d",&a,&b,&c)==3) {
            if(a<1||b<1||c<1||a>n||b>n||c>n||m->count>252){fclose(f);return false;}
            m->p[m->count++]=points[a-1];m->p[m->count++]=points[b-1];m->p[m->count++]=points[c-1];
        }
    }
    fclose(f);return m->count>0;
}
bool wc_render_init(const char *base,char *error,size_t size) {
#define LOAD(ret,name,args) do { p##name=(ret(APIENTRY*)args)SDL_GL_GetProcAddress("gl"#name);if(!p##name){snprintf(error,size,"OpenGL function unavailable: gl"#name);return false;} } while(0);
    GL_FUNCTIONS(LOAD)
    char path[1024];snprintf(path,sizeof(path),"%sassets/shaders/world.vert",base);char *vs=read_file(path);
    snprintf(path,sizeof(path),"%sassets/shaders/world.frag",base);char *fs=read_file(path);
    if(!vs||!fs){free(vs);free(fs);snprintf(error,size,"Missing external shaders in assets/shaders");return false;}
    GLuint v=shader(GL_VERTEX_SHADER,vs,error,size),f=shader(GL_FRAGMENT_SHADER,fs,error,size);free(vs);free(fs);
    if(!v||!f) return false;
    program=pCreateProgram();pAttachShader(program,v);pAttachShader(program,f);pLinkProgram(program);pDeleteShader(v);pDeleteShader(f);
    GLint ok;pGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok){pGetProgramInfoLog(program,(GLsizei)size,NULL,error);return false;}
    pUseProgram(program);matrix_loc=pGetUniformLocation(program,"uMatrix");mode_loc=pGetUniformLocation(program,"uMode");
    sky_loc=pGetUniformLocation(program,"uSky");day_loc=pGetUniformLocation(program,"uDay");
    pGenVertexArrays(1,&vao);pBindVertexArray(vao);pGenBuffers(1,&vbo);pBindBuffer(GL_ARRAY_BUFFER,vbo);
    for(GLuint i=0;i<4;i++) pEnableVertexAttribArray(i);
    pVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,p));
    pVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,color));
    pVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,normal));
    pVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,uv));
    snprintf(path,sizeof(path),"%sassets/meshes/cube.obj",base);bool loaded=mesh_load(&cube,path);
    snprintf(path,sizeof(path),"%sassets/meshes/roof.obj",base);loaded=loaded&&mesh_load(&roof,path);
    snprintf(path,sizeof(path),"%sassets/meshes/crown.obj",base);loaded=loaded&&mesh_load(&crown,path);
    if(!loaded){snprintf(error,size,"Missing or invalid external OBJ meshes");return false;}
    snprintf(path,sizeof(path),"%sassets/textures/grain.ppm",base);FILE *tex=fopen(path,"r");
    if(!tex){snprintf(error,size,"Missing grain.ppm");return false;}
    int tw,th,max;char format[4];
    if(fscanf(tex,"%3s %d %d %d",format,&tw,&th,&max)!=4 || strcmp(format,"P3") || tw!=16 || th!=16 || max!=255){fclose(tex);snprintf(error,size,"Invalid grain.ppm");return false;}
    unsigned char pixels[16*16*3];for(int i=0;i<16*16*3;i++){int n;if(fscanf(tex,"%d",&n)!=1){fclose(tex);return false;}pixels[i]=(unsigned char)n;}fclose(tex);
    glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,16,16,0,GL_RGB,GL_UNSIGNED_BYTE,pixels);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    snprintf(path,sizeof(path),"%sassets/ui/font.txt",base);FILE *ft=fopen(path,"r");if(!ft){snprintf(error,size,"Missing pixel font");return false;}
    int ch,w,row[7];
    while(fscanf(ft,"%d %d %d %d %d %d %d %d %d",&ch,&w,&row[0],&row[1],&row[2],&row[3],&row[4],&row[5],&row[6])==9)
        if(ch>=0&&ch<128){font_width[ch]=(unsigned char)w;for(int y=0;y<7;y++)font[ch][y]=(unsigned char)row[y];}
    fclose(ft);return true;
}
static WCVec add(WCVec a,WCVec b){return(WCVec){a.x+b.x,a.y+b.y,a.z+b.z};}
static WCVec sub(WCVec a,WCVec b){return(WCVec){a.x-b.x,a.y-b.y,a.z-b.z};}
static WCVec rotate(WCVec p,double yaw){double c=cos(yaw),s=sin(yaw);return(WCVec){c*p.x+s*p.z,p.y,-s*p.x+c*p.z};}
static void vertex(WCVec p,WCVec n,const float color[3],float u,float v,bool ui) {
    if(count>=240000) return;
    if(!ui) {
        p=sub(p,eye);double cy=cos(view_yaw),sy=sin(view_yaw),cp=cos(view_pitch),sp=sin(view_pitch);
        p=(WCVec){cy*p.x+sy*p.z,-sy*sp*p.x+cp*p.y+cy*sp*p.z,-sy*cp*p.x-sp*p.y+cy*cp*p.z};
    }
    Vertex *out=&vertices[count++];out->p[0]=(float)p.x;out->p[1]=(float)p.y;out->p[2]=(float)p.z;
    memcpy(out->color,color,sizeof(out->color));out->normal[0]=(float)n.x;out->normal[1]=(float)n.y;out->normal[2]=(float)n.z;
    out->uv[0]=u;out->uv[1]=v;
}
static void triangle(WCVec a,WCVec b,WCVec c,const float color[3]) {
    WCVec u=sub(b,a),v=sub(c,a),n={u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
    double len=sqrt(n.x*n.x+n.y*n.y+n.z*n.z);if(len<1e-9)return;n.x/=len;n.y/=len;n.z/=len;
    vertex(a,n,color,0,0,false);vertex(b,n,color,1,0,false);vertex(c,n,color,1,1,false);
}
static void model(Mesh *m,WCVec center,WCVec scale,double yaw,const float color[3]) {
    for(int i=0;i<m->count;i+=3) {
        WCVec p[3];for(int j=0;j<3;j++){WCVec v=m->p[i+j];v.x*=scale.x;v.y*=scale.y;v.z*=scale.z;p[j]=add(center,rotate(v,yaw));}
        triangle(p[0],p[1],p[2],color);
    }
}
static void box(WCVec p,WCVec size,double yaw,const float color[3]) {model(&cube,p,size,yaw,color);}
static WCVec local_world(const WCState *s,WCVec p){return add(s->castle,rotate(p,s->heading));}
static void home_box(const WCState *s,double x,double y,double z,double sx,double sy,double sz,const float color[3]){
    box(local_world(s,(WCVec){x,y,z}),(WCVec){sx,sy,sz},s->heading,color);
}
static void beam(WCVec a,WCVec b,double width,const float color[3]) {
    WCVec d=sub(b,a),side={-d.z,0,d.x};double len=hypot(side.x,side.z);
    if(len<.001){side=(WCVec){width/2,0,0};}else{side.x*=width/(2*len);side.z*=width/(2*len);}
    WCVec up={0,width/2,0};WCVec corners[8]={add(add(a,side),up),add(sub(a,side),up),sub(sub(a,side),up),sub(add(a,side),up),
        add(add(b,side),up),add(sub(b,side),up),sub(sub(b,side),up),sub(add(b,side),up)};
    int faces[]={0,1,5,0,5,4,1,2,6,1,6,5,2,3,7,2,7,6,3,0,4,3,4,7};
    for(int i=0;i<24;i+=3)triangle(corners[faces[i]],corners[faces[i+1]],corners[faces[i+2]],color);
}
static void terrain(const WCState *s) {
    double cx=floor(eye.x/16)*16,cz=floor(eye.z/16)*16;
    for(int x=-22;x<22;x++)for(int z=-22;z<22;z++) {
        double xx=cx+x*16,zz=cz+z*16;
        WCVec a={xx,wc_terrain(xx,zz,(void*)&s->seed),zz},b={xx+16,wc_terrain(xx+16,zz,(void*)&s->seed),zz};
        WCVec c={xx+16,wc_terrain(xx+16,zz+16,(void*)&s->seed),zz+16},d={xx,wc_terrain(xx,zz+16,(void*)&s->seed),zz+16};
        float color[3]={.31f+(float)sin(xx*.07+zz*.08)*.025f,.48f+(float)cos(xx*.03)*.025f,.29f};
        triangle(a,d,c,color);triangle(a,c,b,color);
    }
    const float bark[3]={.32f,.26f,.19f};
    for(int x=-12;x<=12;x++)for(int z=-12;z<=12;z++) {
        int64_t ix=(int64_t)floor(eye.x/24)+x,iz=(int64_t)floor(eye.z/24)+z;
        uint32_t h=(uint32_t)ix*73856093u ^ (uint32_t)iz*19349663u ^ s->seed;
        double xx=ix*24.0+3+(h%17),zz=iz*24.0+3+((h>>8)%17);
        if(fabs(xx)<20&&fabs(zz)<57)continue;
        double y=wc_terrain(xx,zz,(void*)&s->seed),height=4+(h%6);
        float leaf[3]={.19f+(h%3)*.035f,.34f+(h%4)*.025f,.23f};
        box((WCVec){xx,y+height*.32,zz},(WCVec){.45,height*.64,.45},0,bark);
        model(&crown,(WCVec){xx,y+height*.3,zz},(WCVec){height*.7,height*.8,height*.7},h%6,leaf);
        model(&crown,(WCVec){xx,y+height*.6,zz},(WCVec){height*.5,height*.65,height*.5},h%6,leaf);
    }
    for(int i=0;i<16;i++) {
        double x=(i%4)*7.0-10,z=(i/4)*7.0+23;
        if(s->clock-s->depleted[i]<120) continue;
        float color[3]={i%3==0?.50f:.40f,i%3==2?.65f:.43f,i%3==1?.50f:.25f};
        box((WCVec){x,.4,z},(WCVec){1,.8,1},i*.7,color);
        model(&crown,(WCVec){x,.8,z},(WCVec){.25,.65,.25},0,gold);
    }
}
static void castle(const WCState *s,const WCConfig *cfg) {
    const float wood[3]={.42f,.29f,.19f},beam_color[3]={.24f,.19f,.14f},plaster[3]={.73f,.69f,.52f},stone[3]={.34f,.37f,.36f},metal[3]={.22f,.28f,.29f},copper[3]={.45f,.53f,.47f};
    home_box(s,0,-.7,0,12.5,1.2,16.5,stone);home_box(s,0,-1.5,0,9,.8,13,metal);
    for(int x=0;x<24;x++) {
        float plank[3]={.49f+(x%3)*.018f,.36f+(x%3)*.015f,.24f};
        home_box(s,-5.75+x*.5,-.08,0,.485,.16,16,plank);
    }
    /* Physical ramp and raised command deck share the core support profile. */
    WCVec a=local_world(s,(WCVec){-5.6,0,-3}),b=local_world(s,(WCVec){5.6,0,-3}),c=local_world(s,(WCVec){5.6,.6,-5}),d=local_world(s,(WCVec){-5.6,.6,-5});
    triangle(a,b,c,wood);triangle(a,c,d,wood);home_box(s,0,.3,-6.5,11.2,.6,3,wood);
    for(int side=-1;side<=1;side+=2) {
        home_box(s,side*6,.42,0,.25,.84,16,plaster);home_box(s,side*6,3.7,0,.28,1.0,16,plaster);
        home_box(s,side*5.9,.92,0,.32,.16,16,wood);
        for(int z=-8;z<=8;z+=4) {
            home_box(s,side*6,2,z,.32,4,.32,beam_color);
            home_box(s,side*5.96,2.05,z+2,.12,2.2,.11,wood);
        }
        home_box(s,side*6,2.18,0,.18,.1,16,wood);
    }
    home_box(s,0,.65,-8,12,1.3,.25,plaster);home_box(s,0,3.65,-8,12,1.1,.25,plaster);
    for(int x=-6;x<=6;x+=3)home_box(s,x,2.35,-8,.20,2.2,.28,beam_color);
    home_box(s,0,2.2,-8,12,.1,.26,wood);
    home_box(s,-3.65,1.9,8,4.7,3.8,.3,plaster);home_box(s,3.65,1.9,8,4.7,3.8,.3,plaster);
    home_box(s,0,3.5,8,2.7,.8,.4,beam_color);
    if(!s->gate_open) {home_box(s,-.65,1.5,8,1.28,3,.25,wood);home_box(s,.65,1.5,8,1.28,3,.25,wood);}
    else {
        home_box(s,-1.4,1.5,8.65,.18,3,1.3,wood);home_box(s,1.4,1.5,8.65,.18,3,1.3,wood);
        int steps=(int)ceil(s->stair_length/.32);
        for(int i=0;i<steps;i++) {
            double t=(i+.5)/steps,z=8+t*s->stair_length,y=-t*s->stair_drop;
            home_box(s,0,y-.08,z,2.5,.16,s->stair_length/steps+.02,wood);
            if(i%4==0)for(int side=-1;side<=1;side+=2)home_box(s,side*1.2,y+.5,z,.1,1,.1,metal);
        }
        for(int side=-1;side<=1;side+=2)beam(local_world(s,(WCVec){side*1.2,1,8}),local_world(s,(WCVec){side*1.2,1-s->stair_drop,8+s->stair_length}),.11,metal);
        home_box(s,0,-s->stair_drop-.1,9+s->stair_length,3,.2,2,wood);
        home_box(s,1.8,.6-s->stair_drop,8.5+s->stair_length,.15,1.2,.15,metal);
        home_box(s,1.8,1.2-s->stair_drop,8.5+s->stair_length,.4,.4,.4,gold);
    }
    for(int z=-6;z<=6;z+=4)home_box(s,0,4,z,12.1,.22,.25,beam_color);
    model(&roof,local_world(s,(WCVec){0,4.2,0}),(WCVec){13,2,17},s->heading,copper);
    home_box(s,4.4,5.3,3.4,.8,3,.8,stone);home_box(s,4.4,6.85,3.4,1.1,.25,1.1,metal);
    /* Six independently grounded three-segment legs. */
    for(int side=-1;side<=1;side+=2)for(int n=0;n<3;n++) {
        double z=-5+n*5,phase=s->clock*5+n*2+(side>0?WC_PI:0);
        double gait=fabs(s->speed)>.1?sin(phase)*.5:0;
        WCVec hip=local_world(s,(WCVec){side*5.6,-.8,z}),knee=local_world(s,(WCVec){side*8,-2.1,z+gait});
        WCVec foot=local_world(s,(WCVec){side*8.7,-s->castle.y,z+gait});foot.y=wc_terrain(foot.x,foot.z,(void*)&s->seed)+.35;
        beam(hip,knee,.65,metal);beam(knee,foot,.4,wood);box(knee,(WCVec){.9,.9,.9},s->heading,gold);
        box(foot,(WCVec){1.8,.6,2},s->heading,metal);
    }
    for(int i=0;i<cfg->object_count;i++) {
        WCObject o=cfg->objects[i];WCVec mid={(o.min.x+o.max.x)/2,(o.min.y+o.max.y)/2,(o.min.z+o.max.z)/2};
        WCVec dim=sub(o.max,o.min);box(local_world(s,mid),dim,s->heading,o.color);
        if(o.kind==WC_BED) {
            home_box(s,mid.x,.7,mid.z,dim.x,.16,dim.z,cream);home_box(s,mid.x,.82,o.min.z+.4,1.4,.18,.6,plaster);
            const float blanket[3]={.35f,.45f,.52f};home_box(s,mid.x,.81,mid.z+.5,dim.x,.12,1.5,blanket);
        } else if(o.kind==WC_PANTRY) {
            home_box(s,mid.x,o.max.y,mid.z,dim.x+.1,.12,dim.z+.1,wood);
            home_box(s,mid.x,.55,o.max.z+.025,.2,.3,.05,gold);
        } else if(o.kind==WC_WORKSHOP) {
            home_box(s,mid.x,1.05,mid.z,dim.x+.2,.15,dim.z+.2,wood);
            home_box(s,mid.x,1.23,mid.z,.7,.24,.5,metal);
            home_box(s,mid.x-.2,1.34,mid.z+.8,.6,.08,.12,gold);
        } else if(o.kind==WC_PLANTER) {
            const float soil[3]={.18f,.14f,.09f};home_box(s,mid.x,.82,mid.z,dim.x-.15,.05,dim.z-.15,soil);
            WCCrop crop=s->crops[o.slot];if(crop.crop>=0) {
                double maturity=fmin(1,(s->clock-crop.planted)/crop.duration);float leaf[3]={.34f,.63f,.26f};
                for(int k=0;k<3;k++)model(&crown,local_world(s,(WCVec){mid.x-.5+k*.5,.86,mid.z}),(WCVec){.4,.2+maturity*.7,.4},s->heading,leaf);
            }
        } else if(o.kind==WC_MAP) {
            home_box(s,mid.x,o.max.y+.03,mid.z,dim.x-.18,.04,dim.z-.18,cream);
            home_box(s,mid.x,o.max.y+.1,mid.z,.18,.12,.18,gold);
        } else if(o.kind==WC_HELM_OBJECT) {
            const float chart[3]={.17f,.28f,.29f};home_box(s,0,1.75,-5.5,1.3,.35,.12,chart);
            for(int n=0;n<12;n++){double a=n*WC_PI/6,b=(n+1)*WC_PI/6;beam(local_world(s,(WCVec){sin(a)*.52,1.8+cos(a)*.52,-5.35}),local_world(s,(WCVec){sin(b)*.52,1.8+cos(b)*.52,-5.35}),.08,gold);}
            home_box(s,0,1.8,-5.35,.16,.16,.2,metal);
        }
    }
    /* Rug and warm lanterns form a visible domestic center. */
    const float rug[3]={.37f,.27f,.29f};home_box(s,0,.015,1.6,3.8,.025,4.5,rug);
    for(int side=-1;side<=1;side+=2)for(int z=-6;z<=6;z+=6) {
        home_box(s,side*5.65,2.7,z,.32,.55,.32,metal);home_box(s,side*5.65,2.72,z,.35,.31,.35,gold);
    }
}
static void rect(double x,double y,double w,double h,const float color[3]) {
    WCVec a={x,y,0},b={x+w,y,0},c={x+w,y+h,0},d={x,y+h,0},n={0,0,1};
    vertex(a,n,color,0,0,true);vertex(b,n,color,0,0,true);vertex(c,n,color,0,0,true);
    vertex(a,n,color,0,0,true);vertex(c,n,color,0,0,true);vertex(d,n,color,0,0,true);
}
static void text(double x,double y,const char *s,double scale,const float color[3]) {
    double left=x;
    for(;*s;s++) {
        unsigned char ch=(unsigned char)toupper((unsigned char)*s);
        if(ch=='\n'){y+=10*scale;x=left;continue;}if(ch==' '){x+=4*scale;continue;}if(ch>=128)continue;
        int width=font_width[ch]?font_width[ch]:5;
        for(int row=0;row<7;row++)for(int col=0;col<5;col++)if(font[ch][row]&(1<<(4-col)))rect(x+col*scale,y+row*scale,scale,scale,color);
        x+=(width+1)*scale;
    }
}
static void button(int row,const char *label,bool selected) {
    float bg[3]={selected?.20f:.12f,selected?.27f:.18f,selected?.27f:.19f};
    rect(270,170+row*40,420,34,bg);text(282,181+row*40,label,1.5,cream);
}
static void hud(const WCState *s,const WCConfig *cfg,const WCControls *controls) {
    const float panel[3]={.065f,.105f,.12f},accent[3]={.29f,.58f,.44f};char line[256];
    rect(18,18,280,57,panel);rect(18,18,3,57,gold);text(32,29,"WANDERING CASTLE",2,cream);text(33,53,"HEARTHSTRIDER / FIELD JOURNAL 01",1,muted);
    rect(800,18,142,32,panel);text(813,29,s->help?"HIDE HELP [F1]":"SHOW HELP [F1]",1.3,cream);
    double hour=fmod(9+s->clock/30,24);snprintf(line,sizeof(line),"MEADOWS  /  %02d:%02d",(int)hour,(int)(hour*60)%60);text(380,28,line,1.4,cream);
    snprintf(line,sizeof(line),"SEED %u",s->seed);text(380,48,line,1,cream);
    rect(18,425,241,96,panel);snprintf(line,sizeof(line),"HEALTH %03.0f / 100",s->health);text(30,437,line,1.2,cream);
    rect(30,451,216,5,muted);rect(30,451,s->health*2.16,5,accent);
    snprintf(line,sizeof(line),"STAMINA %03.0f / 100",s->stamina);text(30,466,line,1.2,cream);
    rect(30,480,216,5,muted);rect(30,480,s->stamina*2.16,5,gold);
    snprintf(line,sizeof(line),"PACK %d/%d  REST %.0fs",wc_used(&s->pack),s->pack.capacity,s->rested);text(30,499,line,1.1,muted);
    rect(703,435,239,86,panel);text(716,447,s->context==WC_HELM?"MANUAL HELM":"YOUR HOME IS SAFE",1.4,gold);
    snprintf(line,sizeof(line),"%s  %.1f M/S",s->speed==0?"PARKED":fabs(s->speed)>5?"TURBO":"TRAVEL",fabs(s->speed));text(716,470,line,1.3,cream);
    snprintf(line,sizeof(line),"HEADING %03.0f  COINS %u",fmod(-s->heading*180/WC_PI+360,360),s->coins);text(716,495,line,1.1,muted);
    rect(477,267,6,1,cream);rect(479,265,1,6,cream);
    int target=wc_target(s,cfg,NULL);
    if(s->context==WC_HELM)snprintf(line,sizeof(line),"E  RELEASE HELM");
    else if(target>=0){const char *names[]={"","HELM","BED","PANTRY","WORKSHOP","PLANTER","GATE LEVER","CHART TABLE"};snprintf(line,sizeof(line),"E  %s",names[cfg->objects[target].kind]);}
    else snprintf(line,sizeof(line),"%s",s->attached?"AT HOME":"E  GATHER NEARBY SUPPLIES");
    text(338,396,line,1.4,cream);
    rect(275,467,410,54,panel);text(287,479,s->message,1,cream);
    if(s->help && s->context!=WC_MENU) {
        rect(18,89,239,151,panel);text(31,102,"MAKE YOURSELF AT HOME",1.3,gold);
        text(31,124,"WASD  WALK / STEER\nMOUSE OR ARROWS  LOOK\nE  INTERACT / RELEASE HELM\nSPACE  JUMP  SHIFT  SPRINT\nI  BACKPACK  ESC  PAUSE\nF5  SAVE  F9  RESTORE\nCLOSE GATE BEFORE STEERING",1.1,cream);
    }
    if(controls->touch_active && s->context!=WC_MENU){rect(30,310,120,70,panel);text(45,337,"DRAG TO MOVE",1.2,cream);rect(795,340,130,55,panel);text(814,360,"INTERACT",1.4,gold);text(658,400,"JUMP",1.5,cream);}
    if(s->context!=WC_MENU)return;
    rect(250,91,460,354,panel);rect(250,91,460,3,gold);
    const char *title=s->panel==0?"A MOMENT AT HOME":s->panel==WC_BED?"REST AND REMEMBER":s->panel==WC_WORKSHOP?"THE WORKSHOP":s->panel==WC_PANTRY?"SHARED HOME SUPPLIES":s->panel==WC_PLANTER?"WINDOW GARDEN":s->panel==9?"YOUR BACKPACK":s->panel==10?"BEGIN A NEW JOURNEY?":"THE OPEN ROAD";
    text(271,110,title,2,gold);text(271,140,"SIMULATION PAUSED / ESC TO RETURN",1.2,muted);
    if(s->panel==0) {
        const char *labels[]={"1  RESUME JOURNEY","2  SAVE","3  RESTORE","4  RECOVER PREVIOUS SAVE","5  NEW JOURNEY...","6  QUIT"};
        for(int i=0;i<6;i++)button(i,labels[i],(int)controls->look_y==i);
    } else if(s->panel==WC_BED){button(0,"1  REST: RECOVER + 120 EFFORT SECONDS",false);button(1,"2  SAVE JOURNEY",false);}
    else if(s->panel==10){button(0,"1  CONFIRM NEW SEED AND FRESH PROGRESS",false);button(1,"2  KEEP CURRENT JOURNEY",false);text(275,280,"EXISTING SAVE STAYS UNTIL YOU SAVE AGAIN.",1.1,cream);}
    else if(s->panel==WC_WORKSHOP) {
        snprintf(line,sizeof(line),"1  2 PLANKS  /  TIMBER %d/2",wc_available(s,WC_TIMBER));button(0,line,false);
        snprintf(line,sizeof(line),"2  2 TWINE   /  FIBER %d/3",wc_available(s,WC_FIBER));button(1,line,false);
        snprintf(line,sizeof(line),"3  SATCHEL / PLANK %d/2 TWINE %d/2",wc_available(s,WC_PLANK),wc_available(s,WC_TWINE));button(2,line,false);
        text(275,309,"PAYS BACKPACK FIRST, THEN PANTRY.\nOUTPUT GOES TO BACKPACK.\nSATCHEL PERMANENTLY ADDS 16 UNITS.",1.2,muted);
    } else if(s->panel==WC_PLANTER) {
        WCCrop c=s->crops[s->selection];snprintf(line,sizeof(line),"1  PLANT SUNROOT / SEED %d/1",wc_available(s,WC_SEED));button(0,line,false);
        snprintf(line,sizeof(line),"2  PLANT MINT / SEED %d/1",wc_available(s,WC_MINT_SEED));button(1,line,false);button(2,"3  HARVEST INTO BACKPACK",false);
        snprintf(line,sizeof(line),"%s  %.0f ACTIVE SECONDS REMAIN",c.crop<0?"EMPTY":c.crop==0?"SUNROOT":"MINT",c.crop<0?0:fmax(0,c.duration-(s->clock-c.planted)));text(275,310,line,1.2,cream);
        text(275,339,"SEEDS: SHARED BACKPACK + PANTRY\nGROWTH RESUMES WHEN THIS PANEL CLOSES.",1.1,muted);
    } else if(s->panel==WC_PANTRY) {
        snprintf(line,sizeof(line),"1  SELECT: %s",wc_item_name(s->selection));button(0,line,false);
        button(1,"2  DEPOSIT ONE FROM BACKPACK",false);button(2,"3  WITHDRAW ONE INTO BACKPACK",false);
        snprintf(line,sizeof(line),"PACK %d   PANTRY %d   PANTRY TOTAL %d/80",s->pack.items[s->selection],s->pantry.items[s->selection],wc_used(&s->pantry));text(275,310,line,1.1,cream);
    } else if(s->panel==9) {
        for(int i=0;i<WC_ITEMS;i++){snprintf(line,sizeof(line),"%s  %d",wc_item_name(i),s->pack.items[i]);text(275,173+i*25,line,1.3,cream);}
    } else text(275,180,"THE FIRST MILESTONE IS YOUR MOVING HOME.\nEXPLORE THE MEADOW SUPPLY SITES,\nGARDEN, STORE SUPPLIES AND MAKE A SATCHEL.\n\nCONSTRUCTION, DESTINATIONS AND COMBAT\nARE TRACKED IN THE FEATURE CHECKLIST.",1.2,cream);
}
static void flush(void) {pBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)(count*sizeof(Vertex)),vertices,GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,count);count=0;}
void wc_render(const WCState *s,const WCConfig *cfg,const WCControls *controls,int width,int height) {
    eye=wc_world_player(s);eye.y+=cfg->eye;view_yaw=s->attached?s->yaw-s->heading:s->yaw;view_pitch=s->pitch;
    double hour=fmod(9+s->clock/30,24),day=fmax(.03,sin((hour-6)/12*WC_PI));
    float sky[3]={(float)(.08+.43*day),(float)(.11+.57*day),(float)(.17+.57*day)};
    glViewport(0,0,width,height);glClearColor(sky[0],sky[1],sky[2],1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
    pUseProgram(program);pBindVertexArray(vao);pBindBuffer(GL_ARRAY_BUFFER,vbo);glBindTexture(GL_TEXTURE_2D,texture);
    float aspect=(float)width/height,f=1.0f/tanf(.62f),clip_near=.08f,clip_far=900;
    float projection[16]={f/aspect,0,0,0,0,f,0,0,0,0,(clip_far+clip_near)/(clip_near-clip_far),-1,0,0,2*clip_far*clip_near/(clip_near-clip_far),0};
    pUniformMatrix4fv(matrix_loc,1,GL_FALSE,projection);pUniform1i(mode_loc,0);pUniform3f(sky_loc,sky[0],sky[1],sky[2]);pUniform1f(day_loc,(float)day);
    count=0;terrain(s);castle(s,cfg);
    float cloud[3]={.85f,.88f,.81f};
    for(int i=0;i<14;i++){double x=eye.x+sin(i*12.7)*230+fmod(s->clock*.7,90),z=eye.z+cos(i*7.3)*240;box((WCVec){x,85+i%4*6,z},(WCVec){35+i%3*14,4,15+i%5*7},.2,cloud);}
    float sun[3]={1,.87f,.55f};box((WCVec){eye.x+cos(hour/12*WC_PI)*250,eye.y+sin((hour-6)/12*WC_PI)*220,eye.z-240},(WCVec){12,12,5},0,sun);
    flush();glDisable(GL_DEPTH_TEST);pUniform1i(mode_loc,1);
    float ui[16]={2.0f/960,0,0,0,0,-2.0f/540,0,0,0,0,1,0,-1,1,0,1};pUniformMatrix4fv(matrix_loc,1,GL_FALSE,ui);
    hud(s,cfg,controls);flush();
}
bool wc_screenshot(const char *path,int width,int height) {
    unsigned char *pixels=malloc((size_t)width*height*3),*flipped=malloc((size_t)width*height*3);if(!pixels||!flipped){free(pixels);free(flipped);return false;}
    glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels);
    for(int y=0;y<height;y++)memcpy(flipped+(size_t)y*width*3,pixels+(size_t)(height-y-1)*width*3,(size_t)width*3);
    SDL_Surface *surface=SDL_CreateSurfaceFrom(width,height,SDL_PIXELFORMAT_RGB24,flipped,width*3);
    bool ok=surface&&SDL_SaveBMP(surface,path);SDL_DestroySurface(surface);free(pixels);free(flipped);return ok;
}
void wc_render_shutdown(void) {if(vbo)pDeleteBuffers(1,&vbo);if(vao)pDeleteVertexArrays(1,&vao);if(texture)glDeleteTextures(1,&texture);if(program)pDeleteProgram(program);}
