/* minigl_test — valida a libminigl no OvsbOS
 *
 * Desenha um triângulo colorido na tela usando a API minigl.
 * Instalar em ::/BIN/MINIGL_TEST.
 */

#include <stdio.h>
#include "../../libs/minigl/minigl.h"

int main(void) {
    printf("[MINIGL-TEST] iniciando...\n");

    /* Inicializa com 1280x800 (tamanho padrão do VirtIO-GPU do OvsbOS) */
    if (minigl_init(1280, 800, NULL) < 0) {
        printf("[MINIGL-TEST] falha ao inicializar\n");
        return 1;
    }

    printf("[MINIGL-TEST] fb=%p w=%d h=%d\n",
           minigl_get_fb(), minigl_get_width(), minigl_get_height());

    /* Limpa a tela com cor de fundo */
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Desenha um triângulo colorido no centro */
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f);   /* vermelho */
    glVertex2f(-0.6f, -0.4f);
    glColor3f(0.0f, 1.0f, 0.0f);   /* verde */
    glVertex2f(0.6f, -0.4f);
    glColor3f(0.0f, 0.0f, 1.0f);   /* azul */
    glVertex2f(0.0f, 0.6f);
    glEnd();

    /* Desenha um quadrado branco no canto superior esquerdo */
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(-0.9f, 0.7f);
    glVertex2f(-0.7f, 0.7f);
    glVertex2f(-0.7f, 0.9f);
    glVertex2f(-0.9f, 0.9f);
    glEnd();

    glFlush();
    minigl_swap_buffers();

    printf("[MINIGL-TEST] desenho concluído. swap_buffers chamado.\n");
    printf("[MINIGL-TEST] OK\n");

    minigl_shutdown();
    return 0;
}
