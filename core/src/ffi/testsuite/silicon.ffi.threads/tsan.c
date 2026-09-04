

#include "ffitest.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 20

#if defined(_POSIX_BARRIERS) && _POSIX_BARRIERS > 0
pthread_barrier_t barrier;
#endif

typedef float (*callback_fn)(float, float);

void callback(sffi_cif *cif __UNUSED__, void *ret, void **args, void *userdata __UNUSED__) {
    float a = *(float *)args[0];
    float b = *(float *)args[1];
    *(float *)ret = a / 2 + b / 2;
}

void *thread_func(void *arg) {
#if defined(_POSIX_BARRIERS) && _POSIX_BARRIERS > 0
    pthread_barrier_wait(&barrier);
#endif

    sffi_cif cif;
    sffi_type *args[2] = { &sffi_type_float, &sffi_type_float };

    if (sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 2, &sffi_type_float, args) != SFFI_OK) {
        fprintf(stderr, "sffi_prep_cif failed\n");
        return NULL;
    }

    sffi_closure *closure = sffi_closure_alloc(sizeof(sffi_closure), (void **)&arg);

    if (sffi_prep_closure_loc(closure, &cif, callback, NULL, arg) != SFFI_OK) {
        fprintf(stderr, "sffi_prep_closure_loc failed\n");
        return NULL;
    }

    callback_fn fn = (callback_fn)arg;
    (void) fn(4.0f, 6.0f);

    sffi_closure_free(closure);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];

#if defined(_POSIX_BARRIERS) && _POSIX_BARRIERS > 0
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
#endif

    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_create(&threads[i], NULL, thread_func, NULL) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

#if defined(_POSIX_BARRIERS) && _POSIX_BARRIERS > 0
    pthread_barrier_destroy(&barrier);
#endif

    printf("Completed\n");
    return 0;
}
