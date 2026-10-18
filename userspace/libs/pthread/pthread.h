#ifndef _OP_PTHREAD_H
#define _OP_PTHREAD_H

#include <stdint.h>

typedef uint64_t op_pthread_t;

typedef struct { int __unused; } op_pthread_attr_t;

typedef struct {
    volatile int __lock;
} op_pthread_mutex_t;

#define OP_PTHREAD_CREATE_JOINABLE 0

int  op_pthread_create(op_pthread_t *thread_, const op_pthread_attr_t *attr_,
                    void *(*start_routine_)(void *), void *arg_);
void op_pthread_exit(void *retval_);
int  op_pthread_join(op_pthread_t thread_, void **retval_);

int  op_pthread_mutex_init(op_pthread_mutex_t *mutex_, const void *attr_);
int  op_pthread_mutex_destroy(op_pthread_mutex_t *mutex_);
int  op_pthread_mutex_lock(op_pthread_mutex_t *mutex_);
int  op_pthread_mutex_unlock(op_pthread_mutex_t *mutex_);

op_pthread_t op_pthread_self(void);

#endif /* _OP_PTHREAD_H */
