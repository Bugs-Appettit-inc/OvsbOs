#ifndef MINIGL_H
#define MINIGL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Core OpenGL-like API (mínima, para OvsbOS) ── */

void     glClearColor(float r, float g, float b, float a);
void     glClear(unsigned int mask);
void     glViewport(int x, int y, int w, int h);
void     glColor3f(float r, float g, float b);
void     glVertex2f(float x, float y);
void     glBegin(unsigned int mode);
void     glEnd(void);
void     glFlush(void);

/* Modes */
#define GL_POINTS         0x0000
#define GL_LINES          0x0001
#define GL_LINE_LOOP      0x0002
#define GL_POLYGON        0x0009
#define GL_TRIANGLES      0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_QUADS          0x0007

/* Buffer bits */
#define GL_COLOR_BUFFER_BIT 0x00004000

/* Inicialização / encerramento (chamada pelo SO ou pela aplicação) */
int     minigl_init(int width, int height, void *fb_addr);
void    minigl_shutdown(void);
void    minigl_swap_buffers(void);

/* Obter ponteiro para o framebuffer (para compositores Wayland) */
void   *minigl_get_fb(void);
int     minigl_get_width(void);
int     minigl_get_height(void);

#ifdef __cplusplus
}
#endif

#endif /* MINIGL_H */
