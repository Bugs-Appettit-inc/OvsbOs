/* pthread_min — teste mínimo: 1 thread, sem mutex */
#include <stdio.h>
#include <stdint.h>
#include "../../libs/pthread/pthread.h"

static void *worker(void *arg) {
    long id = (long)arg;
    op_pthread_t me = op_pthread_self();
    printf("[thread %ld] self=%lu started OK\n", id, (unsigned long)me);
    fflush(stdout);
    return (void *)(intptr_t)(id * 10);
}

int main(void) {
    printf("[main] starting pthread_create...\n");
    fflush(stdout);
    op_pthread_t t;
    int rc = op_pthread_create(&t, NULL, worker, (void *)(intptr_t)1);
    printf("[main] clone returned rc=%d tid=%lu\n", rc, (unsigned long)t);
    fflush(stdout);
    if (rc != 0) { printf("[main] FAIL create\n"); return 1; }

    void *rv = NULL;
    printf("[main] calling pthread_join...\n");
    fflush(stdout);
    rc = op_pthread_join(t, &rv);
    printf("[main] join returned rc=%d rv=%p\n", rc, rv);
    fflush(stdout);

    printf("[main] done\n");
    fflush(stdout);
    return 0;
}
