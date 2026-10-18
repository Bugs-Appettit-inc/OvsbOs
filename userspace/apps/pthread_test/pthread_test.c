/* pthread_test — valida op_pthread_create/op_pthread_join no OvsbMk
 *
 * Compilar static, instalar em ::/BIN/PTHREAD (BINNAME=PTHREAD).
 * Rodar no OvsbOS: deve imprimir 4 mensagens de thread + "main done".
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../../libs/pthread/pthread.h"

static volatile int g_counter = 0;
static op_pthread_mutex_t g_mut;

static void *worker(void *arg) {
    long id = (long)arg;
    op_pthread_t me = op_pthread_self();
    printf("[thread %ld] self=%lu started\n", id, (unsigned long)me);

    for (int i = 0; i < 3; i++) {
        op_pthread_mutex_lock(&g_mut);
        g_counter++;
        printf("[thread %ld] counter=%d\n", id, g_counter);
        op_pthread_mutex_unlock(&g_mut);
    }

    printf("[thread %ld] exiting\n", id);
    return (void *)(intptr_t)(id * 10);
}

int main(void) {
    op_pthread_mutex_init(&g_mut, NULL);

    op_pthread_t t[4];
    for (long i = 0; i < 4; i++) {
        int rc = op_pthread_create(&t[i], NULL, worker, (void *)(intptr_t)i);
        printf("main: create thread %ld -> rc=%d tid=%lu\n",
               i, rc, (unsigned long)t[i]);
        if (rc != 0) { printf("main: FAIL create\n"); return 1; }
    }

    for (int i = 0; i < 4; i++) {
        void *retval = NULL;
        int rc = op_pthread_join(t[i], &retval);
        printf("main: join thread %d -> rc=%d retval=%p\n",
               i, rc, retval);
    }

    printf("main: final counter=%d (expected 12)\n", g_counter);
    printf("main done\n");
    return 0;
}
