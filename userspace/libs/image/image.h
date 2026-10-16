/* image.h — biblioteca de imagens OvsbOS (sem dependencias externas).
 * BMP 24/32-bit sem compressao + PPM P6 + (opcional) PNG.
 * Pixel: 0x00RRGGBB. SEM SIMD (kernel nao salva xmm).
 */
#ifndef OVSB_IMAGE_H
#define OVSB_IMAGE_H

typedef struct {
    int w, h;
    unsigned int *px; /* w*h, 0x00RRGGBB */
} img_t;

img_t *img_new(int w, int h);
void img_free(img_t *im);

/* API unificada (por extensao: .bmp .ppm .png). Retorna NULL em erro. */
img_t *img_load(const char *path);
int img_save(const img_t *im, const char *path);

/* Formatos individuais (FILE* ja aberto em modo binario). */
img_t *img_bmp_load(void *fp);
int img_bmp_save(const img_t *im, void *fp);
img_t *img_ppm_load(void *fp);
int img_ppm_save(const img_t *im, void *fp);
#ifdef IMG_WITH_PNG
img_t *img_png_load(void *fp);
#endif

/* Ops (retornam nova imagem; NULL em erro). */
img_t *img_scale(const img_t *im, int nw, int nh); /* vizinho mais proximo */
img_t *img_crop(const img_t *im, int x, int y, int w, int h);
/* copia src sobre dst em (dx,dy), alpha global 0..255. */
void img_blend(img_t *dst, const img_t *src, int dx, int dy, int alpha);

/* thumbnail ASCII em stdout (maxw colunas). Para preview em fm/editor. */
void img_ascii(const img_t *im, int maxw);

#endif
