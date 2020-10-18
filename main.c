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

BUFFER_ITEM * buffer[STAGE2_BUFFER_SIZE] = {NULL};

pthread_attr_t  attr;

pthread_mutex_t is_end_mutex;
int is_end = 0;

/* stage 1 sync structures */
pthread_mutex_t mutex12;
pthread_cond_t full12, empty12;

BUFFER_ITEM * transfer_1_to_2_stage = NULL;

/* stage 2 sync structures */
pthread_mutex_t mutex2;
pthread_cond_t full2, empty2;

int buffer_cnt = 0;

/* stage 3 structures */
pthread_mutex_t mutex23;
pthread_cond_t full23, empty23;

BUFFER_ITEM * transfer_2_to_3_stage = NULL;

/* stage 4 structures */
pthread_mutex_t mutex34;
pthread_cond_t full34, empty34;

BUFFER_ITEM * transfer_3_to_4_stage = NULL;


void *stage_1(void *attr) {
    int generated_matrices = 0;

    while (generated_matrices < MATRIX_COUNT) {
        pthread_mutex_lock(&mutex12); // musi tu byt?

        while (transfer_1_to_2_stage != NULL) {
            pthread_cond_wait(&empty12, &mutex12);
        }
        // generate matrix
        BUFFER_ITEM * generated_item = (BUFFER_ITEM *) malloc(sizeof(BUFFER_ITEM));
        generated_item->id = generated_matrices;

        printf("S1: generated %p with data %d\n", generated_item, generated_item->id);

        transfer_1_to_2_stage = generated_item;
        // end generate matrix

        generated_matrices++;

        pthread_cond_signal(&full12);

        pthread_mutex_unlock(&mutex12);
    }

    pthread_mutex_lock(&is_end_mutex);
    is_end = 1;
    pthread_mutex_unlock(&is_end_mutex);

    printf("S1: exitting\n");

    pthread_exit(NULL);
}

void *stage_2_master(void *attr) {
    int transfered_matrices = 0;

    int is_putting_into_buffer = 0;

    BUFFER_ITEM * tmp = NULL;

    while (is_end == 0) {
        if (is_putting_into_buffer) {
            //producer part
            pthread_mutex_lock(&mutex2);

            while (buffer_cnt == STAGE2_BUFFER_SIZE) {
                pthread_cond_wait(&full2, &mutex2);
            }

            // add
            for (int i = 0; i < STAGE2_BUFFER_SIZE; i++) {
                if (buffer[i] == NULL) {
                    buffer[i] = tmp;

                    printf("S2M: puts %p at buffer(%d)\n", tmp, i);

                    break;
                }
            }
            buffer_cnt++;

            pthread_cond_signal(&empty2);

            pthread_mutex_unlock(&mutex2);

            tmp = NULL;

            transfered_matrices++;

            is_putting_into_buffer = 0;
        } else {
            // consumer part
            pthread_mutex_lock(&mutex12);

            while (transfer_1_to_2_stage == NULL) {
                pthread_cond_wait(&full12, &mutex12);
            }

            tmp = transfer_1_to_2_stage;

            printf("S2M: received %p with data %d\n", tmp, tmp->id);

            is_putting_into_buffer = 1;

            transfer_1_to_2_stage = NULL;

            pthread_cond_signal(&empty12);

            pthread_mutex_unlock(&mutex12);
        }
    }

    printf("S2M: exitting\n");

    pthread_exit(NULL);
}

BUFFER_ITEM * solve2(BUFFER_ITEM * item) {
    // TODO implement
    printf("Solving %p with value %d\n", item, item->id);

    return item; // TODO change the return type
}

void *stage_2_worker(void *attr) {
    int tmp_cnt = 0; // TODO delete
    int gets_from_buffer = 1;

    BUFFER_ITEM * solution = NULL;

    while (is_end == 0) {
        if (gets_from_buffer) {
            // consumer
            pthread_mutex_lock(&mutex2);

            while (buffer_cnt == 0) {
                pthread_cond_wait(&empty2, &mutex2);
            }

            BUFFER_ITEM * tmp = NULL;

            // get
            for (int i = 0; i < STAGE2_BUFFER_SIZE; i++) {
                if (buffer[i] != NULL) {
                    tmp = buffer[i];
                    buffer[i] = NULL;
                    printf("S2W: pick %p with data %d from buffer(%d)\n", tmp, tmp->id, i);
                    break;
                }
            }

            buffer_cnt--;

            pthread_cond_signal(&full2);

            pthread_mutex_unlock(&mutex2);

            // solve and switch
            solution = solve2(tmp);

            gets_from_buffer = 0; // switch to producer

        } else {
            // producer
            pthread_mutex_lock(&mutex23);

            while (transfer_2_to_3_stage != NULL) {
                pthread_cond_wait(&full23, &mutex23);
            }

            transfer_2_to_3_stage = solution;

            printf("S2W: put %p with data %d further\n", transfer_2_to_3_stage, transfer_2_to_3_stage->id);


            pthread_cond_signal(&empty23);

            pthread_mutex_unlock(&mutex23);

            // reset to consumer
            solution = NULL;
            gets_from_buffer = 1;

            tmp_cnt++; // TODO delete
        }
    }

    printf("S2W: exitting\n");

    pthread_exit(NULL);
}

BUFFER_ITEM * solve3(BUFFER_ITEM * item) {
    // TODO implement
    return item; // TODO change the return type
}

void *stage_3 (void *attr) {
    int tmp_cnt = 0; // TODO delete

    int receive_data = 1;

    BUFFER_ITEM * to_send = NULL; // TODO change data structure

    while (is_end == 0) {
        if (receive_data) {
            //consumer
            pthread_mutex_lock(&mutex23);

            while(transfer_2_to_3_stage == NULL) {
                pthread_cond_wait(&empty23, &mutex23);
            }

            BUFFER_ITEM * tmp = transfer_2_to_3_stage;

            printf("S3: get %p with data %d from S2W\n", tmp, tmp->id);

            transfer_2_to_3_stage = NULL;

            to_send = solve3(tmp);

            pthread_cond_signal(&full23);

            pthread_mutex_unlock(&mutex23);

            receive_data = 0;
        } else {
            // producer
            pthread_mutex_lock(&mutex34);

            while(transfer_3_to_4_stage != NULL) {
                pthread_cond_wait(&full34, &mutex34);
            }

            transfer_3_to_4_stage = to_send;

            printf("S3: put %p with data %d to S4\n", to_send, to_send->id);

            pthread_cond_signal(&empty34);

            pthread_mutex_unlock(&mutex34);

            receive_data = 1;
            to_send = NULL;

            tmp_cnt++; // TODO delete
        }
    }

    printf("S3: exitting\n");

    pthread_exit(NULL);
}

void *stage_4(void *attr) {
    int tmp_cnt = 0;
    while (is_end == 0) {
        pthread_mutex_lock(&mutex34);

        while (transfer_3_to_4_stage == NULL) {
            pthread_cond_wait(&empty34, &mutex34);
        }

        BUFFER_ITEM * tmp = transfer_3_to_4_stage;

        // TODO write to a file
        printf("S4: Writing %p to a file with data %d \n", tmp, tmp->id);

        free(tmp);

        tmp = NULL;

        transfer_3_to_4_stage = NULL;

        pthread_cond_signal(&full34);

        pthread_mutex_unlock(&mutex34);

        tmp_cnt++;
    }

    printf("S4: exitting\n");

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

    pthread_cond_init(&full2, NULL);
    pthread_cond_init(&empty2, NULL);

    pthread_cond_init(&full23, NULL);
    pthread_cond_init(&empty23, NULL);

    pthread_cond_init(&full34, NULL);
    pthread_cond_init(&empty34, NULL);

    pthread_mutex_init(&mutex12, NULL);
    pthread_mutex_init(&mutex2, NULL);
    pthread_mutex_init(&mutex23, NULL);
    pthread_mutex_init(&mutex34, NULL);
    pthread_mutex_init(&is_end_mutex, NULL);

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
        int is_thr_stage_2_worker = pthread_create(&thr_stage_2_workers[i], &attr, (void * (*) (void *)) stage_2_worker, NULL);
        if (is_thr_stage_2_worker) {
            printf("Error: stage 2 thread num %d isn't created\n", i);
        } else {
            printf("S2W created\n");
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

    printf("Main is exitting\n");

    return 0;
}
