/*
============================================================================
Filename    : function.h
Author      : PARSA, EPFL
============================================================================
*/

#define ITERATIONS 25

// https://edstem.org/eu/courses/3061/discussion/229212
static __attribute__((always_inline)) inline int triple32(int x) {
    return x * 3;
}

void compute (int *in, int *out, int chunk_size, int size){
    for (int i = 0; i < chunk_size; i++) {
        int key = in[i];
        int hash_val = ((unsigned int)triple32(key)) % size;

        out[hash_val]++;
    }
}
