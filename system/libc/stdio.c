#include <stdio.h>
#include <stdarg.h>
#include <string.h>
FILE __stdin_file={.fd=0},__stdout_file={.fd=1},__stderr_file={.fd=2};
static long _syscall(long n,long a1,long a2,long a3,long a4){long r;__asm__ volatile("int $0x80":"=a"(r):"a"(n),"D"(a1),"S"(a2),"d"(a3),"c"(a4):"r11","memory");return r;}
int open(const char*p,int f){return _syscall(5,(long)p,f,0,0);}
int close(int f){return _syscall(6,f,0,0,0);}
int read(int f,void*b,int c){return _syscall(3,f,(long)b,c,0);}
int write(int f,const void*b,int c){return _syscall(4,f,(long)b,c,0);}
int lseek(int f,int o,int w){return _syscall(200,f,o,w,0);}
int unlink(const char*p){return _syscall(10,(long)p,0,0,0);}
int mkdir(const char*p){return _syscall(136,(long)p,0,0,0);}
int rmdir(const char*p){return _syscall(137,(long)p,0,0,0);}
int stat(const char*p,struct stat*b){return _syscall(188,(long)p,(long)b,0,0);}
int fstat(int f,struct stat*b){return _syscall(189,f,(long)b,0,0);}
int kbhit(void){return _syscall(198,0,0,0,0);}
void putchar(char c){write(1,&c,1);}
void puts(const char*s){write(1,s,strlen(s));write(1,"\n",1);}
char getchar(void){char c;read(0,&c,1);return c;}
char *gets(char*buf){int i=0;while(1){char c=getchar();if(c=='\n')break;if(c=='\b'){if(i>0)i--;continue;}buf[i++]=c;}buf[i]=0;return buf;}
FILE *fopen(const char*p,const char*m){(void)m;int f=open(p,0);if(f<0)return 0;static FILE x;x.fd=f;return &x;}
int fclose(FILE*f){return close(f->fd);}
int fread(void*b,int s,int c,FILE*f){int r=read(f->fd,b,s*c);if(r<=0)return 0;return r/s;}
int fwrite(const void*b,int s,int c,FILE*f){int r=write(f->fd,b,s*c);if(r<=0)return 0;return r/s;}
char *fgets(char*b,int n,FILE*f){int i=0;while(i<n-1){char c;if(read(f->fd,&c,1)<=0)break;b[i++]=c;if(c=='\n')break;}b[i]=0;return i>0?b:0;}
int fputs(const char*s,FILE*f){return write(f->fd,s,strlen(s))>0?0:-1;}
int fputc(int c,FILE*f){char ch=c;return write(f->fd,&ch,1)==1?c:-1;}
static void _pd(unsigned long n){char b[20],*p=b;do{*p++='0'+(n%10);n/=10;}while(n);while(p>b)putchar(*--p);}
static void _ph(unsigned long n){const char*h="0123456789abcdef";char b[20],*p=b;do{*p++=h[n&0xF];n>>=4;}while(n);while(p>b)putchar(*--p);}
int vsnprintf(char*buf,int n,const char*fmt,va_list ap){if(!buf||n<=0)return 0;int p=0;for(const char*f=fmt;*f&&p<n-1;f++){if(*f!='%'){buf[p++]=*f;continue;}f++;int l=0;while(*f=='l'){l=1;f++;}switch(*f){case 'd':{long v=l?va_arg(ap,long):(long)va_arg(ap,int);if(v<0){buf[p++]='-';v=-v;}char t[24],*q=t;unsigned long uv=(unsigned long)v;do{*q++='0'+(uv%10);uv/=10;}while(uv);while(q>t&&p<n-1)buf[p++]=*--q;break;}case 'u':{unsigned long v=l?va_arg(ap,unsigned long):(unsigned long)va_arg(ap,unsigned);char t[24],*q=t;do{*q++='0'+(v%10);v/=10;}while(v);while(q>t&&p<n-1)buf[p++]=*--q;break;}case 'x':case 'X':{unsigned long v=l?va_arg(ap,unsigned long):(unsigned long)va_arg(ap,unsigned);const char*h="0123456789abcdef";char t[24],*q=t;do{*q++=h[v&0xF];v>>=4;}while(v);while(q>t&&p<n-1)buf[p++]=*--q;break;}case 's':{const char*s=va_arg(ap,const char*);if(!s)s="(null)";while(*s&&p<n-1)buf[p++]=*s++;break;}case 'c':{int c=va_arg(ap,int);buf[p++]=c;break;}case '%':buf[p++]='%';break;default:buf[p++]='%';if(*f)buf[p++]=*f;break;}}buf[p]=0;return p;}
int sprintf(char*b,const char*f,...){va_list ap;va_start(ap,f);int n=vsnprintf(b,4096,f,ap);va_end(ap);return n;}
int printf(const char*f,...){va_list ap;va_start(ap,f);for(const char*p=f;*p;p++){if(*p!='%'){putchar(*p);continue;}p++;switch(*p){case 'd':{int v=va_arg(ap,int);if(v<0){putchar('-');v=-v;}_pd((unsigned)v);break;}case 'u':_pd(va_arg(ap,unsigned));break;case 'x':case 'X':_ph(va_arg(ap,unsigned));break;case 's':{const char*s=va_arg(ap,const char*);while(*s)putchar(*s++);break;}case 'c':putchar(va_arg(ap,int));break;case '%':putchar('%');break;}}va_end(ap);return 0;}
int fprintf(FILE*f,const char*fmt,...){va_list ap;va_start(ap,fmt);for(const char*p=fmt;*p;p++){if(*p!='%'){fputc(*p,f);continue;}p++;switch(*p){case 'd':{int v=va_arg(ap,int);if(v<0){fputc('-',f);v=-v;}char b[12],*q=b;unsigned u=v;do{*q++='0'+(u%10);u/=10;}while(q>b);while(q>b)fputc(*--q,f);break;}case 's':{const char*s=va_arg(ap,const char*);while(*s)fputc(*s++,f);break;}case 'c':fputc(va_arg(ap,int),f);break;case '%':fputc('%',f);break;}}va_end(ap);return 0;}
int sscanf(const char*s,const char*fmt,...){va_list ap;va_start(ap,fmt);int m=0;while(*fmt){if(*fmt==' '){fmt++;continue;}if(*fmt!='%'){if(*s==*fmt){s++;fmt++;continue;}break;}fmt++;switch(*fmt){case 'd':{int*v=va_arg(ap,int*);while(*s==' ')s++;int sign=1;if(*s=='-'){sign=-1;s++;}else if(*s=='+')s++;int n=0;while(*s>='0'&&*s<='9'){n=n*10+(*s-'0');s++;}*v=sign*n;m++;fmt++;break;}case 'c':{char*v=va_arg(ap,char*);*v=*s++;m++;fmt++;break;}default:fmt++;break;}}va_end(ap);return m;}
