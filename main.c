#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// stage 1 settings
#define MATRIX_COUNT 10
#define MATRIX_DIM 2

// stage 2 settings
#define STAGE2_BUFFER_SIZE 3
#define STAGE2_WORKERS_COUNT 5

struct buffer_item_t {
    int id;
} typedef BUFFER_ITEM;

/* buffer definition */
//typedef int buffer_item;

BUFFER_ITEM buffer[STAGE2_BUFFER_SIZE];

pthread_attr_t  attr;

/* stage 1 sync structures */
pthread_mutex_t mutex;
pthread_cond_t full12, empty12;

BUFFER_ITEM * transfer_1_to_2_phaze = NULL;


/* stage 2 sync structures */

/* stage 3 structures */

/* stage 4 structures */

void *stage_1(void *attr) {
    int generated_matrices = 0;

    while (generated_matrices < MATRIX_COUNT) {
        pthread_mutex_lock(&mutex); // musi tu byt?

        while (transfer_1_to_2_phaze != NULL) {
            pthread_cond_wait(&empty12, &mutex);
        }
        // generate matrix
        BUFFER_ITEM * generated_item = (BUFFER_ITEM *) malloc(sizeof(BUFFER_ITEM));
        generated_item->id = generated_matrices;

        printf("Generated %p\n", generated_item);

        transfer_1_to_2_phaze = generated_item;
        // end generate matrix

        generated_matrices++;

        pthread_cond_signal(&full12);

        pthread_mutex_unlock(&mutex);
    }

    pthread_exit(NULL);
}

void *stage_2_master(void *attr) {
    int transfered_matrices = 0;

    int is_putting_into_buffer = 0;

    BUFFER_ITEM * tmp = NULL;

    while (transfered_matrices < MATRIX_COUNT) {
        if (is_putting_into_buffer) {
            //producer part


        } else {
            // consumer part
            pthread_mutex_lock(&mutex);

            while (transfer_1_to_2_phaze == NULL) {
                pthread_cond_wait(&full12, &mutex);
            }

            tmp = transfer_1_to_2_phaze;
            transfer_1_to_2_phaze = NULL;

            printf("Got %p\n", tmp);

            is_putting_into_buffer = 1;

            pthread_cond_signal(&empty12);

            pthread_mutex_unlock(&mutex);
        }
    }

    pthread_exit(NULL);
}

void *stage_2_worker(void *attr) {

    pthread_exit(NULL);
}

void *stage_3 (void *attr) {

    pthread_exit(NULL);
}

void *stage_4(void *attr) {

    pthread_exit(NULL);
}

int main() {
    // init
    pthread_t thr_stage_1;
    pthread_t thr_stage_2_master;
    pthread_t thr_stage_2_workers[STAGE2_WORKERS_COUNT];
    pthread_t thr_stage_3;
    pthread_t thr_stage_4;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_cond_init(&full12, NULL);
    pthread_cond_init(&empty12, NULL);

    pthread_mutex_init(&mutex, NULL);

    // create threads
    int is_thr_stage_1 = pthread_create(&thr_stage_1, &attr, (void * (*) (void *)) stage_1, NULL);
    if (is_thr_stage_1) {
        printf("Error: stage 1 thread isn't created\n");
    }

    int is_thr_stage_2_master = pthread_create(&thr_stage_2_master, &attr, (void * (*) (void *)) stage_2_master, NULL);
    if (is_thr_stage_2_master) {
        printf("Error: stage 2 master thread isn't created\n");
    }

    for (int i = 0; i < STAGE2_WORKERS_COUNT; i++) {
        int is_thr_stage_2_worker = pthread_create(&thr_stage_2_master, &attr, (void * (*) (void *)) stage_2_master, NULL);
        if (is_thr_stage_2_worker) {
            printf("Error: stage 2 thread num %d isn't created\n", i);
        }
    }

    int is_thr_stage_3 = pthread_create(&thr_stage_3, &attr, (void * (*) (void *)) stage_3, NULL);
    if (is_thr_stage_3) {
        printf("Error: stage 3 thread isn't created\n");
    }

    int is_thr_stage_4 = pthread_create(&thr_stage_4, &attr, (void * (*) (void *)) stage_4, NULL);
    if (is_thr_stage_4) {
        printf("Error: stage 4 thread isn't created\n");
    }

    // exit - threads join, destroy/clean up
    pthread_join(thr_stage_1, NULL);
    pthread_join(thr_stage_2_master, NULL);
    for (int i = 0; i < STAGE2_WORKERS_COUNT; i++) {
        pthread_join(thr_stage_2_workers[i], NULL);
    }
    pthread_join(thr_stage_3, NULL);
    pthread_join(thr_stage_4, NULL);

    pthread_attr_destroy(&attr);

    return 0;
}
