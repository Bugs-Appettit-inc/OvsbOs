/* ovsb-desktop — FASE 8.1: ambiente desktop (cliente Wayland, protocolo raw).
 * Wallpaper (gradiente) + barra 24px (botão Iniciar + relógio uptime).
 * FASE 8.2: menu Iniciar (layer overlay) + launcher (/BIN + /APPS, só ELF,
 * duplo fork + execve). Fecha com clique-fora. Sem Esc: o compositor NÃO
 * roteia teclado pra layers (só pointer enter/motion/button).
 * Sem libwayland (padrão layer_shell_test.c). Ids DENSOS a partir de 1
 * (wl_map_reserve_new exige; ver Fase 8.0a).
 * Ids: 1 display, 2 registry, 3 sync, 4 comp, 5 shm, 6 output,
 *       7 layer_shell, 8 seat, 9 pointer,
 *       10 bg_surf, 11 bg_layer, 12 pool, 13 buf,
 *       14 bar_surf, 15 bar_layer, 16 pool2, 17 buf2,
 *       menu (por abertura, nunca reutiliza): 18+4k surf/layer/pool/buf. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
extern char **environ;
#ifndef SYS_memfd_create
#define SYS_memfd_create 319
#endif
#ifndef SYS_clock_gettime
#define SYS_clock_gettime 228
#endif
#define CLOCK_REALTIME 0
struct d_timespec { int64_t tv_sec; int64_t tv_nsec; };
static int d_clock_realtime(struct d_timespec *ts){
    return syscall(SYS_clock_gettime, CLOCK_REALTIME, ts);
}
#define MFD_CLOEXEC 0x0001
static int my_memfd_create(const char *n, unsigned f){ return syscall(SYS_memfd_create,n,f); }
static uint32_t rd32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void wr32(uint8_t *p,uint32_t v){p[0]=v&0xFF;p[1]=(v>>8)&0xFF;p[2]=(v>>16)&0xFF;p[3]=(v>>24)&0xFF;}
static int send_msg(int fd, uint32_t sender, uint32_t opcode, const uint8_t *payload, uint32_t plen){
    uint32_t size = 8 + plen;
    static uint8_t msg[4096];
    if(size>sizeof(msg)) return -1;
    wr32(msg, sender);
    wr32(msg+4, (size<<16)|(opcode&0xFFFF));
    if(plen) memcpy(msg+8,payload,plen);
    ssize_t n = write(fd, msg, size);
    return (n==(ssize_t)size)?0:-1;
}
static int send_msg_fd(int fd, uint32_t sender, uint32_t opcode, const uint8_t *payload, uint32_t plen, int cfd){
    uint32_t size = 8 + plen;
    static uint8_t msg[4096];
    wr32(msg, sender); wr32(msg+4, (size<<16)|(opcode&0xFFFF));
    if(plen) memcpy(msg+8,payload,plen);
    struct iovec iov; iov.iov_base=msg; iov.iov_len=size;
    char cmsgbuf[32]; struct cmsghdr *cmsg=(struct cmsghdr*)cmsgbuf;
    cmsg->cmsg_len=20; cmsg->cmsg_level=1; cmsg->cmsg_type=1;
    *(int*)((char*)cmsg+16)=cfd;
    struct msghdr mh={0}; mh.msg_iov=&iov; mh.msg_iovlen=1; mh.msg_control=cmsgbuf; mh.msg_controllen=cmsg->cmsg_len;
    ssize_t n=sendmsg(fd,&mh,0);
    return (n==(ssize_t)size)?0:-1;
}
static void dlog(const char *s){ write(1,s,strlen(s)); }

/* ---- fonte bitmap 8x8 (mesma do compositor/titlebar) ---- */
static const uint8_t dfont[96][8]={
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
{0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
{0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00},
{0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00},
{0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00},
{0xC6,0xCC,0x18,0x30,0x60,0x66,0xC6,0x00},
{0x38,0x6C,0x6C,0x38,0x6D,0x66,0x3B,0x00},
{0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00},
{0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},
{0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
{0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
{0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
{0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},
{0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
{0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
{0x06,0x0C,0x0C,0x18,0x30,0x30,0x60,0x00},
{0x7C,0xC6,0xCE,0xDE,0xF6,0xE6,0x7C,0x00},
{0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
{0x7C,0xC6,0x06,0x0C,0x30,0x60,0xFE,0x00},
{0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00},
{0x0C,0x1C,0x3C,0x6C,0xFE,0x0C,0x0C,0x00},
{0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00},
{0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00},
{0xFE,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
{0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00},
{0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00},
{0x00,0x00,0x18,0x00,0x00,0x18,0x18,0x00},
{0x00,0x00,0x18,0x00,0x00,0x18,0x18,0x30},
{0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00},
{0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
{0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00},
{0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0x00},
{0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x7E,0x00},
{0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},
{0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0xFC,0x00},
{0x7C,0xC6,0xC0,0xC0,0xC0,0xC6,0x7C,0x00},
{0xF8,0xCC,0xC6,0xC6,0xC6,0xCC,0xF8,0x00},
{0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xFE,0x00},
{0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xC0,0x00},
{0x7C,0xC6,0xC0,0xDE,0xC6,0xC6,0x7E,0x00},
{0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},
{0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00},
{0x06,0x06,0x06,0x06,0x06,0xC6,0x7C,0x00},
{0xC6,0xCC,0xD8,0xF0,0xD8,0xCC,0xC6,0x00},
{0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xFE,0x00},
{0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00},
{0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00},
{0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00},
{0xFC,0xC6,0xC6,0xFC,0xC0,0xC0,0xC0,0x00},
{0x7C,0xC6,0xC6,0xC6,0xDA,0xCC,0x76,0x00},
{0xFC,0xC6,0xC6,0xFC,0xD8,0xCC,0xC6,0x00},
{0x7C,0xC6,0xC0,0x7C,0x06,0xC6,0x7C,0x00},
{0xFE,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
{0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00},
{0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x38,0x00},
{0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00},
{0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00},
{0xC6,0xC6,0xC6,0x7C,0x18,0x18,0x18,0x00},
{0xFE,0x06,0x0C,0x18,0x30,0x60,0xFE,0x00},
{0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
{0x60,0x30,0x30,0x18,0x0C,0x0C,0x06,0x00},
{0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
{0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00},
{0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},
{0x18,0x18,0x0C,0x00,0x00,0x00,0x00,0x00},
{0x00,0x00,0x7C,0x06,0x7E,0xC6,0x7E,0x00},
{0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xFC,0x00},
{0x00,0x00,0x7C,0xC6,0xC0,0xC0,0x7C,0x00},
{0x06,0x06,0x7E,0xC6,0xC6,0xC6,0x7E,0x00},
{0x00,0x00,0x7C,0xC6,0xFE,0xC0,0x7C,0x00},
{0x1C,0x36,0x30,0x78,0x30,0x30,0x30,0x00},
{0x00,0x00,0x7E,0xC6,0xC6,0x7E,0x06,0x7C},
{0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xC6,0x00},
{0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},
{0x06,0x00,0x0E,0x06,0x06,0x06,0x66,0x3C},
{0xC0,0xC0,0xCC,0xD8,0xF0,0xCC,0xC6,0x00},
{0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
{0x00,0x00,0xEC,0xFE,0xD6,0xD6,0xD6,0x00},
{0x00,0x00,0xDC,0xFC,0xC6,0xC6,0xC6,0x00},
{0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0x00},
{0x00,0x00,0xFC,0xC6,0xC6,0xFC,0xC0,0xC0},
{0x00,0x00,0x7E,0xC6,0xC6,0x7E,0x06,0x06},
{0x00,0x00,0xDC,0xFC,0xC0,0xC0,0xC0,0x00},
{0x00,0x00,0x7E,0xC0,0x7C,0x06,0x7C,0x00},
{0x30,0x30,0x7C,0x30,0x30,0x34,0x18,0x00},
{0x00,0x00,0xC6,0xC6,0xC6,0xC6,0x7E,0x00},
{0x00,0x00,0xC6,0xC6,0xC6,0x7C,0x38,0x00},
{0x00,0x00,0xC6,0xD6,0xFE,0xEE,0x44,0x00},
{0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00},
{0x00,0x00,0xC6,0xC6,0xC6,0x7E,0x06,0x7C},
{0x00,0x00,0xFE,0x0C,0x18,0x30,0xFE,0x00},
{0x0C,0x18,0x18,0x70,0x18,0x18,0x0C,0x00},
{0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
{0x30,0x18,0x18,0x0E,0x18,0x18,0x30,0x00},
{0x00,0x00,0x76,0xDC,0x00,0x00,0x00,0x00},
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};
static void dtext(uint32_t *pix, int W, int H, int x, int y, const char *t, uint32_t color){
    for(int i=0;t[i];i++){
        unsigned char c=(unsigned char)t[i];
        if(c<0x20||c>0x7E) continue;
        const uint8_t *g=dfont[c-0x20];
        for(int r=0;r<8;r++) for(int col=0;col<8;col++){
            if(g[r]&(0x80u>>col)){
                int px=x+i*8+col, py=y+r;
                if(px>=0&&px<W&&py>=0&&py<H) pix[py*W+px]=color;
            }
        }
    }
}
static void dfill(uint32_t *pix, int W, int H, int x0, int y0, int x1, int y1, uint32_t color){
    if(x0<0) x0=0; if(y0<0) y0=0; if(x1>W) x1=W; if(y1>H) y1=H;
    for(int y=y0;y<y1;y++) for(int x=x0;x<x1;x++) pix[y*W+x]=color;
}

#define SCR_W 1280
#define SCR_H 800
#define BAR_H 24
#define COL_TEXT 0xFFFFFFFFu
#define COL_BAR  0xFF2D2D30u
#define COL_BTN  0xFF3E3E42u

static void do_bind(int fd, uint32_t reg, uint32_t name, const char *iface, uint32_t ver, uint32_t nid){
    uint32_t ilen=strlen(iface)+1, pad=(4-(ilen%4))%4;
    uint8_t b[128]; int o=0;
    wr32(b+o,name);o+=4; wr32(b+o,ilen);o+=4; memcpy(b+o,iface,ilen);o+=ilen;
    memset(b+o,0,pad);o+=pad; wr32(b+o,ver);o+=4; wr32(b+o,nid);o+=4;
    send_msg(fd,reg,0,b,o);
}
static void *make_pool(int fd, uint32_t shm_id, uint32_t pool_id, int W, int H, int *sz_out){
    int sz=W*H*4;
    int mfd=my_memfd_create("desktop_shm", MFD_CLOEXEC);
    if(mfd<0) return NULL;
    if(ftruncate(mfd,sz)<0){ close(mfd); return NULL; }
    void *addr=mmap(NULL, sz, PROT_READ|PROT_WRITE, MAP_SHARED, mfd, 0);
    if(addr==MAP_FAILED||addr==NULL){ close(mfd); return NULL; }
    uint8_t b[8]; wr32(b,pool_id); wr32(b+4,sz);
    if(send_msg_fd(fd,shm_id,0,b,8,mfd)!=0){ close(mfd); return NULL; }
    *sz_out=sz;
    return addr;
}
static void layer_surf(int fd, uint32_t shell, uint32_t comp, uint32_t out,
                       uint32_t surf, uint32_t layer, uint32_t lay, const char *ns){
    uint8_t b[4]; wr32(b,surf); send_msg(fd,comp,0,b,4); // create_surface
    uint32_t nl=strlen(ns)+1, pad=(4-(nl%4))%4;
    uint8_t g[64]; int o=0;
    wr32(g+o,layer);o+=4; wr32(g+o,surf);o+=4; wr32(g+o,out);o+=4; wr32(g+o,lay);o+=4;
    wr32(g+o,nl);o+=4; memcpy(g+o,ns,nl);o+=nl; memset(g+o,0,pad);o+=pad;
    send_msg(fd,shell,0,g,o); // get_layer_surface
}

/* redesenha a barra toda (botão + relógio uptime HH:MM:SS) */
static void draw_bar(uint32_t *pix, unsigned daysec){
    dfill(pix, SCR_W, BAR_H, 0, 0, SCR_W, BAR_H, COL_BAR);
    // botão Iniciar
    dfill(pix, SCR_W, BAR_H, 4, 3, 94, 21, COL_BTN);
    dtext(pix, SCR_W, BAR_H, 12, 8, "Iniciar", COL_TEXT);
    // relógio REALTIME (epoch do kernel; RTC via -rtc base=localtime)
    unsigned h=(daysec/3600)%24, m=(daysec/60)%60, s=daysec%60;
    char clk[16];
    clk[0]='0'+h/10; clk[1]='0'+h%10; clk[2]=':';
    clk[3]='0'+m/10; clk[4]='0'+m%10; clk[5]=':';
    clk[6]='0'+s/10; clk[7]='0'+s%10; clk[8]=0;
    dtext(pix, SCR_W, BAR_H, SCR_W-8*8-8, 8, clk, COL_TEXT);
}

/* ---- FASE 8.2: menu Iniciar (layer overlay) + launcher ----
 * Overlay pequeno (MENU_W x H) ancorado TOP|LEFT com margem-top BAR_H
 * (cai em 0,24, logo abaixo da barra). Como o compositor faz blit
 * opaco (sem alpha), nada de fullscreen-transparente: clique fora no
 * wallpaper é descartado pelo compositor; clique fora na BARRA chega
 * aqui (mesmo cliente) e fecha o menu. Teclado nunca chega (layers
 * não recebem kbd no compositor) — sem Esc.
 * Ids do menu: 18+4k (surf/layer/pool/buf), nunca reutilizados
 * (destroy + ids novos a cada abertura evita race com configure). */
#define BAR_SURF_ID 14
#define MENU_W 224
#define MENU_ROW_H 16
#define MENU_PAD 4
#define MENU_MAXN 40
#define MENU_NAME_C 26 /* chars por linha: (224-16)/8 */
#define COL_MENU_BG 0xFF1E1E1Eu
#define COL_MENU_BD 0xFF5A5A5Eu

static struct { char path[80]; char name[MENU_NAME_C+1]; } menu_apps[MENU_MAXN];
static int menu_n=0;
static int menu_open=0;
static uint32_t menu_surf=0, menu_layer=0, menu_w=0, menu_h=0;
static uint32_t *menu_pix=NULL;
static int menu_sz=0;
static uint32_t next_id=18; /* primeiro bloco livre */
static int enter_surf=0;    /* surface sob o pointer (enter opcode 0) */

/* lista ELF de um diretório; retorna adicionados.
 * FASE 8.2: getdents64 RAW (sem opendir/readdir/malloc da glibc) + filtro
 * ELF pelo magic. estático, sem alocação (dbuf estático). */
#ifndef SYS_getdents64
#define SYS_getdents64 217
#endif
struct lxd { int64_t d_ino; int64_t d_off; unsigned short d_reclen; unsigned char d_type; char d_name[1]; };
/* FASE8.2-INV: modo de scan (0=full com filtro ELF; 1=só nomes).
 * Modo 1 foi diagnóstico (separar open/read de getdents); resultado:
 * ambos crasham igual → open/read inocentados. Manter 0 (produto). */
#define SCAN_NAMES_ONLY 0
static int scan_dir(const char *dir){
    static char dbuf[8192];
    int dfd=open(dir,O_RDONLY);
    if(dfd<0) return 0;
    int added=0;
    for(;;){
        long n=syscall(SYS_getdents64,dfd,dbuf,sizeof(dbuf));
        if(n<=0) break;
        long off=0;
        while(off<n && menu_n<MENU_MAXN){
            struct lxd *e=(struct lxd*)(dbuf+off);
            if(e->d_reclen<19) break;
            const char *nm=e->d_name;
            /* nome terminado dentro do recorde? (defesa contra kernel) */
            int nl=0; while(nl+19<(int)e->d_reclen && nl<255 && nm[nl]) nl++;
            if(nm[0]!='.' && nl>0 && nl<80){
                char full[80];
                int dl=0; while(dir[dl] && dl<70){ full[dl]=dir[dl]; dl++; }
                full[dl]='/'; dl++;
                int ci=0; while(ci<nl && dl<79){ full[dl]=nm[ci]; dl++; ci++; }
                full[dl]=0;
#if SCAN_NAMES_ONLY
                /* modo diagnóstico: lista tudo sem open/read (sem filtro ELF) */
                int k=0; while(full[k] && k<79){ menu_apps[menu_n].path[k]=full[k]; k++; }
                menu_apps[menu_n].path[k]=0;
                int i=0; while(nm[i] && i<MENU_NAME_C){ menu_apps[menu_n].name[i]=nm[i]; i++; }
                menu_apps[menu_n].name[i]=0;
                menu_n++; added++;
#else
                int f=open(full,O_RDONLY);
                if(f>=0){
                    uint8_t mg[4]; ssize_t r=read(f,mg,4); close(f);
                    if(r==4&&mg[0]==0x7f&&mg[1]=='E'&&mg[2]=='L'&&mg[3]=='F'){
                        int k=0; while(full[k] && k<79){ menu_apps[menu_n].path[k]=full[k]; k++; }
                        menu_apps[menu_n].path[k]=0;
                        int i=0; while(nm[i] && i<MENU_NAME_C){ menu_apps[menu_n].name[i]=nm[i]; i++; }
                        menu_apps[menu_n].name[i]=0;
                        menu_n++; added++;
                    }
                }
#endif
            }
            off+=e->d_reclen;
        }
        if(menu_n>=MENU_MAXN) break;
    }
    close(dfd);
    return added;
}
/* ordena por nome (bolha; n<=40) p/ menu determinístico */
static void sort_apps(void){
    for(int i=0;i<menu_n;i++) for(int j=i+1;j<menu_n;j++){
        if(strcmp(menu_apps[i].name,menu_apps[j].name)>0){
            char tp[80], tn[MENU_NAME_C+1];
            strcpy(tp,menu_apps[i].path); strcpy(tn,menu_apps[i].name);
            strcpy(menu_apps[i].path,menu_apps[j].path); strcpy(menu_apps[i].name,menu_apps[j].name);
            strcpy(menu_apps[j].path,tp); strcpy(menu_apps[j].name,tn);
        }
    }
}
static void draw_menu(uint32_t *pix){
    dfill(pix, menu_w, menu_h, 0, 0, menu_w, menu_h, COL_MENU_BD);
    dfill(pix, menu_w, menu_h, 1, 1, menu_w-1, menu_h-1, COL_MENU_BG);
    if(menu_n==0){
        dtext(pix, menu_w, menu_h, 8, MENU_PAD+4, "(vazio)", COL_TEXT);
        return;
    }
    for(int i=0;i<menu_n;i++)
        dtext(pix, menu_w, menu_h, 8, MENU_PAD+i*MENU_ROW_H+4, menu_apps[i].name, COL_TEXT);
}
/* duplo fork + execve (neto herda; filho sai; pai colhe o filho) */
static void do_launch(const char *path){
    char l[96]; snprintf(l,sizeof(l),"[DESKTOP] launch %s\n",path); dlog(l);
    pid_t mid=fork();
    if(mid<0){ dlog("[DESKTOP] launch fork FAIL\n"); return; }
    if(mid==0){
        if(fork()==0){
            char *const argv[]={ (char*)path, NULL };
            execve(path, argv, environ);
            _exit(127);
        }
        _exit(0);
    }
    int st=0; waitpid(mid,&st,0);
}
static void menu_close(int fd){
    if(!menu_open) return;
    send_msg(fd,menu_layer,7,NULL,0); /* layer destroy */
    send_msg(fd,menu_surf,0,NULL,0);  /* surf destroy */
    if(menu_pix && menu_pix!=(void*)-1){ munmap(menu_pix,menu_sz); menu_pix=NULL; }
    menu_open=0; menu_surf=0; menu_layer=0;
    dlog("[DESKTOP] menu fechado\n");
}
static void menu_try_open(int fd){
    menu_n=0;
    scan_dir("/BIN");
    scan_dir("/APPS");
    sort_apps();
    int rows=menu_n>0?menu_n:1;
    menu_w=MENU_W; menu_h=MENU_PAD*2+rows*MENU_ROW_H;
    if(menu_h>SCR_H-BAR_H) menu_h=SCR_H-BAR_H;
    menu_surf=next_id; menu_layer=next_id+1;
    uint32_t pool=next_id+2, buf=next_id+3;
    next_id+=4;
    layer_surf(fd,7,4,6, menu_surf,menu_layer,3,"menu");
    { uint8_t b[8]; wr32(b,menu_w); wr32(b+4,menu_h); send_msg(fd,menu_layer,0,b,8); } /* set_size */
    { uint8_t b[4]; wr32(b,5); send_msg(fd,menu_layer,1,b,4); }                        /* anchor TOP|LEFT */
    { uint8_t b[4]; wr32(b,0); send_msg(fd,menu_layer,2,b,4); }                        /* exclusive 0 */
    { uint8_t b[16]; wr32(b,BAR_H); wr32(b+4,0); wr32(b+8,0); wr32(b+12,0); send_msg(fd,menu_layer,3,b,16); } /* margin top=BAR_H */
    menu_pix=make_pool(fd,5,pool,menu_w,menu_h,&menu_sz);
    if(!menu_pix){ dlog("[DESKTOP] menu pool FAIL\n"); send_msg(fd,menu_layer,7,NULL,0); send_msg(fd,menu_surf,0,NULL,0); menu_surf=menu_layer=0; return; }
    draw_menu(menu_pix);
    { uint8_t b[24]; wr32(b,buf); wr32(b+4,0); wr32(b+8,menu_w); wr32(b+12,menu_h); wr32(b+16,menu_w*4); wr32(b+20,1); send_msg(fd,pool,0,b,24); }
    { uint8_t b[12]; wr32(b,buf); wr32(b+4,0); wr32(b+8,0); send_msg(fd,menu_surf,1,b,12); }
    send_msg(fd,menu_surf,6,NULL,0);
    menu_open=1;
    { char l[64]; snprintf(l,sizeof(l),"[DESKTOP] menu overlay criado %ux%u apps=%d\n",menu_w,menu_h,menu_n); dlog(l); }
}

int main(void){
    setvbuf(stdout, NULL, _IONBF, 0);
    dlog("[DESKTOP] start\n");
    const char *xdg=getenv("XDG_RUNTIME_DIR");
    char path[128];
    if(xdg&&xdg[0]) snprintf(path,sizeof(path),"%s/wayland-0",xdg);
    else strcpy(path,"/tmp/wayland-0");
    int fd=socket(AF_UNIX,SOCK_STREAM,0);
    if(fd<0) return 1;
    struct sockaddr_un sa; memset(&sa,0,sizeof(sa)); sa.sun_family=AF_UNIX;
    strncpy(sa.sun_path,path,sizeof(sa.sun_path)-1);
    int tries=0; while(connect(fd,(struct sockaddr*)&sa,sizeof(sa))<0){
        if(tries++>=200) return 1; usleep(100000);
    }
    dlog("[DESKTOP] connected\n");
    uint8_t pl[4]; wr32(pl,2); send_msg(fd,1,1,pl,4); // registry -> 2
    wr32(pl,3); send_msg(fd,1,0,pl,4); // sync -> 3
    uint8_t buf[8192]; int blen=0;
    int n_comp=0,n_shm=0,n_layer=0,n_output=0,n_seat=0,v_layer=0,cb_done=0,got_error=0;
    for(int iter=0; iter<120 && !(cb_done&&n_comp&&n_shm&&n_layer&&n_output&&n_seat); iter++){
        struct pollfd pf={fd,POLLIN,0};
        if(poll(&pf,1,100)>0 && (pf.revents&POLLIN)){
            ssize_t n=read(fd, buf+blen, sizeof(buf)-blen);
            if(n<=0) break;
            blen+=n;
            int off=0;
            while(off+8<=blen){
                uint32_t sender=rd32(buf+off), szop=rd32(buf+off+4);
                uint32_t size=szop>>16, op=szop&0xFFFF;
                if(size<8||off+(int)size>blen) break;
                uint8_t *pay=buf+off+8; uint32_t plen=size-8;
                if(sender==2 && op==0 && plen>=12){
                    uint32_t name=rd32(pay), ilen=rd32(pay+4);
                    if(ilen<64 && 8+ilen<=plen){
                        char iface[64]; memcpy(iface,pay+8,ilen); iface[ilen-1]=0;
                        uint32_t pad=(4-(ilen%4))%4;
                        uint32_t ver=rd32(pay+8+ilen+pad);
                        if(!strcmp(iface,"wl_compositor")) n_comp=name;
                        else if(!strcmp(iface,"wl_shm")) n_shm=name;
                        else if(!strcmp(iface,"wl_output")) n_output=name;
                        else if(!strcmp(iface,"zwlr_layer_shell_v1")){ n_layer=name; v_layer=ver; }
                        else if(!strcmp(iface,"wl_seat")) n_seat=name;
                    }
                } else if(sender==1 && op==0){ got_error=1; }
                else if(sender==3 && op==0){ cb_done=1; }
                off+=size;
            }
            if(off>0){ memmove(buf,buf+off,blen-off); blen-=off; }
        }
        if(got_error) break;
    }
    if(got_error||!n_comp||!n_shm||!n_layer||!n_output||!n_seat){ dlog("[DESKTOP] registry FAIL\n"); return 1; }
    { char l[64]; snprintf(l,sizeof(l),"[DESKTOP] registry OK layer v%u\n",v_layer); dlog(l); }
    uint32_t lv = v_layer>4?4:v_layer;
    do_bind(fd,2,n_comp,"wl_compositor",4,4);
    do_bind(fd,2,n_shm,"wl_shm",1,5);
    do_bind(fd,2,n_output,"wl_output",2,6);
    do_bind(fd,2,n_layer,"zwlr_layer_shell_v1",lv,7);
    do_bind(fd,2,n_seat,"wl_seat",7,8);
    { uint8_t b[4]; wr32(b,9); send_msg(fd,8,0,b,4); } // seat.get_pointer -> 9
    /* --- wallpaper: surface 8 + layer 9 (background) --- */
    layer_surf(fd,7,4,6, 10,11,0,"wallpaper");
    { uint8_t b[8]; wr32(b,0); wr32(b+4,0); send_msg(fd,11,0,b,8); } // set_size 0x0
    { uint8_t b[4]; wr32(b,15); send_msg(fd,11,1,b,4); } // anchor all
    int sz1=0;
    uint32_t *bg=make_pool(fd,5,12,SCR_W,SCR_H,&sz1);
    if(!bg){ dlog("[DESKTOP] bg pool FAIL\n"); return 1; }
    // gradiente vertical #262626 -> #161616
    for(int y=0;y<SCR_H;y++){
        int v=0x26-((0x26-0x16)*y)/(SCR_H-1);
        uint32_t col=0xFF000000u|(v<<16)|(v<<8)|v;
        for(int x=0;x<SCR_W;x++) bg[y*SCR_W+x]=col;
    }
    { uint8_t b[24]; wr32(b,13); wr32(b+4,0); wr32(b+8,SCR_W); wr32(b+12,SCR_H); wr32(b+16,SCR_W*4); wr32(b+20,1); send_msg(fd,12,0,b,24); }
    { uint8_t b[12]; wr32(b,13); wr32(b+4,0); wr32(b+8,0); send_msg(fd,10,1,b,12); }
    send_msg(fd,10,6,NULL,0);
    dlog("[DESKTOP] wallpaper committed\n");
    /* --- barra: surface 12 + layer 13 (top, 24px, exclusive 24) --- */
    layer_surf(fd,7,4,6, 14,15,2,"bar");
    { uint8_t b[8]; wr32(b,0); wr32(b+4,BAR_H); send_msg(fd,15,0,b,8); }
    { uint8_t b[4]; wr32(b,13); send_msg(fd,15,1,b,4); } // TOP|LEFT|RIGHT
    { uint8_t b[4]; wr32(b,24); send_msg(fd,15,2,b,4); } // exclusive_zone 24
    int sz2=0;
    uint32_t *bar=make_pool(fd,5,16,SCR_W,BAR_H,&sz2);
    if(!bar){ dlog("[DESKTOP] bar pool FAIL\n"); return 1; }
    draw_bar(bar, 0);
    { uint8_t b[24]; wr32(b,17); wr32(b+4,0); wr32(b+8,SCR_W); wr32(b+12,BAR_H); wr32(b+16,SCR_W*4); wr32(b+20,1); send_msg(fd,16,0,b,24); }
    { uint8_t b[12]; wr32(b,17); wr32(b+4,0); wr32(b+8,0); send_msg(fd,14,1,b,12); }
    send_msg(fd,14,6,NULL,0);
    dlog("[DESKTOP] bar committed\n");
    /* --- loop: ack configures + relógio REALTIME 1s + pointer --- */
    unsigned tick=0;
    int cfg=0;
    int ptr_sx=0, ptr_sy=0; // última posição (coords da surface)
    for(;;){
        struct pollfd pf={fd,POLLIN,0};
        if(poll(&pf,1,1000)>0 && (pf.revents&POLLIN)){
            ssize_t n=read(fd, buf+blen, sizeof(buf)-blen);
            if(n<=0) break;
            blen+=n;
            int off=0;
            while(off+8<=blen){
                uint32_t sender=rd32(buf+off), szop=rd32(buf+off+4);
                uint32_t size=szop>>16, op=szop&0xFFFF;
                if(size<8||off+(int)size>blen) break;
                uint8_t *pay=buf+off+8; uint32_t plen=size-8;
                if(sender==1 && op==0){ dlog("[DESKTOP] display error\n"); return 1; }
                else if((sender==11||sender==15||(menu_open&&sender==menu_layer)) && op==0 && plen>=12){
                    uint32_t serial=rd32(pay);
                    uint8_t b[4]; wr32(b,serial); send_msg(fd,sender,6,b,4);
                    send_msg(fd,sender==11?10:(sender==15?14:menu_surf),6,NULL,0);
                    cfg++;
                } else if((sender==11||sender==15) && op==1){ dlog("[DESKTOP] layer closed\n"); return 0;
                } else if(sender==9 && op==0 && plen>=16){
                    enter_surf=(int)rd32(pay+4);
                    ptr_sx=(int)rd32(pay+8)/256; ptr_sy=(int)rd32(pay+12)/256;
                } else if(sender==9 && op==1 && plen>=8){
                    enter_surf=0;
                } else if(sender==9 && op==2 && plen>=12){
                    ptr_sx=(int)rd32(pay+4)/256; ptr_sy=(int)rd32(pay+8)/256;
                } else if(sender==9 && op==3 && plen>=16){
                    uint32_t btn=rd32(pay+8), st=rd32(pay+12);
                    /* FASE8.2-fix: só PRESS age. O release do próprio clique
                     * de abertura chegava com enter na barra (!=menu) e era
                     * lido como "clique-fora" — fechava o menu na hora. */
                    if(btn!=272 || st!=1) {} else if(menu_open){
                        if(enter_surf==(int)menu_surf){
                            int row=(ptr_sy-MENU_PAD)/MENU_ROW_H;
                            if(row>=0 && row<menu_n && ptr_sx>=0 && (unsigned)ptr_sx<menu_w)
                                do_launch(menu_apps[row].path);
                            /* dentro do menu mas fora das linhas: ignora */
                        } else {
                            menu_close(fd); /* clique-fora chegou pela barra */
                        }
                    } else if(enter_surf==BAR_SURF_ID &&
                              ptr_sx>=4 && ptr_sx<94 && ptr_sy>=3 && ptr_sy<21){
                        dlog("[DESKTOP] INICIAR click\n");
                        menu_try_open(fd);
                    }
                }
                off+=size;
            }
            if(off>0){ memmove(buf,buf+off,blen-off); blen-=off; }
        }
        /* FASE8.1-clock: hora real do kernel (CLOCK_REALTIME, epoch+RTC) */
        struct d_timespec ts={0,0};
        unsigned daysec=0;
        if(d_clock_realtime(&ts)==0 && ts.tv_sec>0) daysec=(unsigned)(ts.tv_sec%86400);
        if(tick==0){ char l[64]; snprintf(l,sizeof(l),"[DESKTOP] epoch=%lld\n",(long long)ts.tv_sec); dlog(l); }
        tick++;
        draw_bar(bar, daysec);
        send_msg(fd,14,6,NULL,0); // commit (re-desenha relógio)
        if((tick&15)==0){ char l[64]; snprintf(l,sizeof(l),"[DESKTOP] tick %u cfg=%d clk=%u\n",tick,cfg,daysec); dlog(l); }
    }
    return 0;
}
