/* dl_test — valida dlopen() no OvsbMk
 *
 * Compilar static, instalar em ::/BIN/DL_TEST.
 * Rodar no OvsbOS: deve imprimir "dlopen OK" e "dl_test done".
 */

#include <stdio.h>
#include "../../libs/dl/dl.h"

int main(void) {
    void *h = dlopen("libanswer.so");
    if (h == NULL) {
        printf("dlopen: FALHA (retornou NULL)\n");
        return 1;
    }
    printf("dlopen OK (handle=%p)\n", h);

    void *sym = dlsym(h, "answer");
    printf("dlsym(answer) = %p\n", sym);

    int rc = dlclose(h);
    printf("dlclose = %d\n", rc);

    printf("dl_test done\n");
    return 0;
}
