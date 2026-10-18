/* pthread_min — teste mínimo: 1 thread, sem mutex */
#include <stdio.h>
#include <stdint.h>
#include "../../libs/pthread/pthread.h"

static void *worker(void *arg) {
    long id = (long)arg;
    op_pthread_t me = op_pthread_self();
    printf("[thread %ld] self=%lu started OK\n", id, (unsigned long)me);
    return (void *)(intptr_t)(id * 10);
}

int main(void) {
    op_pthread_t t;
    int rc = op_pthread_create(&t, NULL, worker, (void *)(intptr_t)1);
    printf("main: create rc=%d tid=%lu\n", rc, (unsigned long)t);
    if (rc != 0) { printf("main: FAIL\n"); return 1; }
    void *rv = NULL;
    rc = op_pthread_join(t, &rv);
    printf("main: join rc=%d rv=%p\n", rc, rv);
    printf("main done\n");
    return 0;
}
