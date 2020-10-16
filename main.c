#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// stage 1 settings - dimension of randomly generated matrices
#define MATRIX_COUNT 10
#define MATRIX_DIM 2

// stage 2 settings
#define STAGE2_BUFFER_SIZE 3
#define STAGE2_WORKERS_COUNT 5

/* buffer definition */
typedef int buffer_item;

buffer_item buffer[STAGE2_BUFFER_SIZE];

/* stage 1 sync structures */

/* stage 2 sync structures */

/* stage 3 structures */

/* stage 4 structures */

int main() {
    printf("Hello, World!\n");
    return 0;
}
