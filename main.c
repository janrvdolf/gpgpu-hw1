#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

// stage 1 settings
#define MATRIX_COUNT 10
#define MATRIX_DIM 2

#define RAND_DIVISOR 100000000

// stage 2 settings
#define STAGE2_BUFFER_SIZE 3
#define STAGE2_WORKERS_COUNT 5

struct threads_array_t {
    pthread_t entry[STAGE2_WORKERS_COUNT + 3]; // all threads except the first one
} typedef THREADS_ARRAY;

struct buffer_item_t {
    int id;
    double matrix[MATRIX_DIM * MATRIX_DIM + MATRIX_DIM];
    double solution[MATRIX_DIM];
    int solutions_count;
} typedef BUFFER_ITEM;

/* buffer definition */
//typedef int buffer_item;

BUFFER_ITEM * buffer[STAGE2_BUFFER_SIZE] = {NULL};

pthread_attr_t  attr;

pthread_mutex_t is_end_mutex;
int is_end = 0;

int is_stage3_end = 0;


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
    THREADS_ARRAY * thread_array = (THREADS_ARRAY *) attr;

    int generated_matrices = 0;

    while (1) {
        pthread_mutex_lock(&mutex12); // musi tu byt?

        while (transfer_1_to_2_stage != NULL) {
            pthread_cond_wait(&empty12, &mutex12);
        }
        // generate matrix
        if (generated_matrices >= MATRIX_COUNT) {
            is_end = 1;
            pthread_cond_signal(&full12);
            pthread_mutex_unlock(&mutex12);
            break;
        } else {
            BUFFER_ITEM * generated_item = (BUFFER_ITEM *) malloc(sizeof(BUFFER_ITEM));
            generated_item->id = generated_matrices;

            for (int i = 0; i < MATRIX_DIM*MATRIX_DIM+MATRIX_DIM; i++) {
                generated_item->matrix[i] = rand() % 10;
            }

            printf("S1: generated %p with data %d\n", generated_item, generated_item->id);

            transfer_1_to_2_stage = generated_item;
            // end generate matrix
        }
        pthread_cond_signal(&full12);

        generated_matrices++;

        pthread_mutex_unlock(&mutex12);

    }


    is_end = 1;

    printf("S1: exitting\n");

    pthread_exit(NULL);
}

void *stage_2_master(void *attr) {
    int transfered_matrices = 0;

    int is_putting_into_buffer = 0;

    BUFFER_ITEM * tmp = NULL;

    while (1) {
        if (is_end && !is_putting_into_buffer) {
            break;
        }

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
                    break;
                }
            }
            buffer_cnt++;
            pthread_mutex_unlock(&mutex2);
            pthread_cond_broadcast(&empty2);



            printf("S2M: puts %p at buffer(i)\n", tmp);
            tmp = NULL;
            transfered_matrices++;
            is_putting_into_buffer = 0;

        } else {
            // consumer part
            pthread_mutex_lock(&mutex12);
            while (transfer_1_to_2_stage == NULL) {
                if (is_end) {
                    printf("S2M: exitting2\n");
                    pthread_exit(NULL);
                }
                pthread_cond_wait(&full12, &mutex12);
            }
            tmp = transfer_1_to_2_stage;
            transfer_1_to_2_stage = NULL;
            pthread_cond_signal(&empty12);
            pthread_mutex_unlock(&mutex12);

            is_putting_into_buffer = 1;
            printf("S2M: received %p with data %d\n", tmp, tmp->id);
        }
    }

    printf("S2M: exitting\n");

    pthread_exit(NULL);
}

BUFFER_ITEM * solve2(BUFFER_ITEM * item) {
    // Triangulation

    printf("Solving %p with value %d\n", item, item->id);

    for (int j = 0; j < MATRIX_DIM; j++) {
        for (int i = j + 1; i < MATRIX_DIM; i++) {
            int idx_diagonal = j * (MATRIX_DIM + 1) + j;
            int idx = i * (MATRIX_DIM + 1) + j;

            double f = item->matrix[idx] / item->matrix[idx_diagonal];

            for (int k = 0; k < MATRIX_DIM + 1; k++) {
                int idx_i = i * (MATRIX_DIM + 1) + k;
                int idx_j = j * (MATRIX_DIM + 1) + k;

                item->matrix[idx_i] -= item->matrix[idx_j] * f;
            }
        }
    }

    return item;
}



void *stage_2_worker(void *attr) {

    int gets_from_buffer = 1;

    long int id = (long int) attr;

    printf("Starting thread S2W(%ld)\n", id);

    BUFFER_ITEM * solution = NULL;

    while (1) {
//        pthread_testcancel();
//        printf("S2W(%ld): tests cancel\n", id);
//        int rNum = rand() / RAND_DIVISOR;

            //sleep(rNum);
//        if (is_end) {
//            break;
//        }

        if (gets_from_buffer) {
            // consumer
            printf("S2W(%ld): in consumer mutex\n", id);
            pthread_mutex_lock(&mutex2);
            while (buffer_cnt == 0) {
                printf("S2W(%ld): in consumer wait\n", id);
                if (is_stage3_end) {
                    printf("S2W(%ld): consumer exitting\n", id);
                    pthread_exit(NULL);
                }

                pthread_cond_wait(&empty2, &mutex2);
            }
            printf("S2W(%ld): out consumer wait\n", id);
            BUFFER_ITEM * tmp = NULL;
            // get
            for (int i = 0; i < STAGE2_BUFFER_SIZE; i++) {
                if (buffer[i] != NULL) {
                    tmp = buffer[i];
                    buffer[i] = NULL;
                    break;
                }
            }
            buffer_cnt--;
            pthread_cond_signal(&full2);
            pthread_mutex_unlock(&mutex2);
            // solve and switch
            solution = solve2(tmp);
            gets_from_buffer = 0; // switch to producer
            printf("S2W(%ld): pick %p with data %d from buffer(i)\n", id, tmp, tmp->id);
            printf("S2W(%ld): out consumer mutex\n", id);
        } else {
            // producer
            printf("S2W(%ld): in producer mutex\n", id);
            pthread_mutex_lock(&mutex23);

            while (transfer_2_to_3_stage != NULL) {
                printf("S2W(%ld): in producer wait\n", id);
                if (is_stage3_end) {
                    printf("S2W(%ld): producer exitting\n", id);
                    pthread_exit(NULL);
                }

                pthread_cond_wait(&full23, &mutex23);
            }
            printf("S2W(%ld): out producer wait\n", id);

            transfer_2_to_3_stage = solution;

            printf("S2W(%ld): put %p with data %d further\n", id, transfer_2_to_3_stage, transfer_2_to_3_stage->id);


            pthread_cond_signal(&empty23);

            // reset to consumer
            solution = NULL;
            gets_from_buffer = 1;

            pthread_mutex_unlock(&mutex23);
            printf("S2W(%ld): out producer mutex\n", id);
        }
    }

    printf("S2W(%ld): exitting\n", id);

    pthread_exit(NULL);
}

BUFFER_ITEM * solve3(BUFFER_ITEM * item) {
    int is_one_solution = 1; // TODO overit spravnost

    for (int i = 0; i < MATRIX_DIM; i++) {
        int idx = i * (MATRIX_DIM + 1) + i;

        if (fabs(item->matrix[idx]) < 0.0001) {
            is_one_solution = 0;
        }
    }

    if (is_one_solution) {
        for (int i = MATRIX_DIM - 1; i >= 0; i--) {
            double sum = 0;
            for (int k = i + 1; k < MATRIX_DIM; k++) {
                int idx = i * (MATRIX_DIM+1) + k;

                sum += item->solution[k] * item->matrix[idx];
            }

            int idx = (i+1) * (MATRIX_DIM+1) -1 ;

            item->solution[i] = (item->matrix[idx] - sum) / item->matrix[i * (MATRIX_DIM+1) + i];
        }

//        for (int i = 0; i < MATRIX_DIM; i++) {
//            printf("x_%d = %lf\n", i, item->solution[i]);
//        }
    } else {
        // zero or more than one
        item->solutions_count = 10;
    }

    return item;
}

void *stage_3 (void *attr) {
    int receive_data = 1;

    BUFFER_ITEM * to_send = NULL;

    int transfered_matrices = 0;

    while (is_end != 3) {
        if (receive_data) {
            //consumer
            printf("S3: in consumer lock\n");
            pthread_mutex_lock(&mutex23);
            while(transfer_2_to_3_stage == NULL) {
                printf("S3: consumer wait\n");
                pthread_cond_wait(&empty23, &mutex23);
            }
            BUFFER_ITEM * tmp = transfer_2_to_3_stage;
            pthread_mutex_unlock(&mutex23);
            pthread_cond_broadcast(&full23);

            printf("S3: out consumer lock\n");

            printf("S3: get %p with data %d from S2W\n", tmp, tmp->id);

            transfer_2_to_3_stage = NULL;

            to_send = solve3(tmp);

            receive_data = 0;

        } else {
            // producer
            printf("S3: in producer lock\n");
            pthread_mutex_lock(&mutex34);
            while(transfer_3_to_4_stage != NULL) {
                printf("S3: producet wait\n");
                pthread_cond_wait(&full34, &mutex34);
            }
            transfer_3_to_4_stage = to_send;
            pthread_cond_signal(&empty34);
            pthread_mutex_unlock(&mutex34);
            printf("S3: out producer lock\n");

            printf("S3: put %p with data %d to S4\n", to_send, to_send->id);
            transfered_matrices++;

            receive_data = 1;
            to_send = NULL;
        }

    }

    printf("S3: exitting\n");

    pthread_exit(NULL);
}

void *stage_4(void *attr) {
    THREADS_ARRAY * thread_array = (THREADS_ARRAY *) attr;
    int thread_cnt = 0;

    while (1) {
        if (is_end == 3) {
            break;
        }

        pthread_mutex_lock(&mutex34);

        while (transfer_3_to_4_stage == NULL) {
            printf("S4: cond_wait\n");
//            if (is_end > 0) {
//                printf("S4: exitting\n");
//                pthread_exit(NULL);
//            }
            pthread_cond_wait(&empty34, &mutex34);
        }

        BUFFER_ITEM * tmp = transfer_3_to_4_stage;


        printf("S4: Writing %p to a file with data %d \n", tmp, tmp->id);

        char filename_buffer[100];

        sprintf(filename_buffer, "mat_%d.txt", tmp->id);

        FILE * f = fopen(filename_buffer, "wb");

        for (int y = 0; y < MATRIX_DIM; y++) {
            for (int x = 0; x < MATRIX_DIM +1; x++) {
                int idx = y * (MATRIX_DIM + 1) + x;
                fprintf(f, "%lf ", tmp->matrix[idx]);
            }
            fprintf(f,"\n");
        }

        fclose(f);

        free(tmp);

        tmp = NULL;

        transfer_3_to_4_stage = NULL;

        pthread_mutex_unlock(&mutex34);
        pthread_cond_signal(&full34);


        thread_cnt++;

        if (is_end > 0 && thread_cnt == MATRIX_COUNT) {
            break;
        }
    }

    printf("S4: exitting\n");



    for (int i = 1; i < STAGE2_WORKERS_COUNT + 1; i++) {

        pthread_cancel(thread_array->entry[i]);
//        pthread_kill(thread_array->entry[i], SIGKILL);

        printf("S4: Cancelling %d \n", i);
    }

    is_end = 3;

    pthread_exit(NULL);
}

int main() {
    // init
    THREADS_ARRAY thr_array;
    int thr_array_cnt = 0;

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
    int is_thr_stage_2_master = pthread_create(&thr_stage_2_master, &attr, (void * (*) (void *)) stage_2_master, NULL);
    if (is_thr_stage_2_master) {
        printf("Error: stage 2 master thread isn't created\n");
    } else {
        thr_array.entry[thr_array_cnt] = thr_stage_2_master;
        thr_array_cnt++;
    }

    for (long int i = 0; i < STAGE2_WORKERS_COUNT; i++) {
        int is_thr_stage_2_worker = pthread_create(&thr_stage_2_workers[i], &attr, (void * (*) (void *)) stage_2_worker, (void*) i);
        if (is_thr_stage_2_worker) {
            printf("Error: stage 2 thread num %ld isn't created\n", i);
        } else {
            printf("S2W(%ld) created \n", i);
            thr_array.entry[thr_array_cnt] = thr_stage_2_workers[i];
            thr_array_cnt++;
        }
    }

    int is_thr_stage_3 = pthread_create(&thr_stage_3, &attr, (void * (*) (void *)) stage_3, &thr_array);
    if (is_thr_stage_3) {
        printf("Error: stage 3 thread isn't created\n");
    } else {
        thr_array.entry[thr_array_cnt] = thr_stage_3;
        thr_array_cnt++;
    }

    int is_thr_stage_4 = pthread_create(&thr_stage_4, &attr, (void * (*) (void *)) stage_4, &thr_array);
    if (is_thr_stage_4) {
        printf("Error: stage 4 thread isn't created\n");
    } else {
        thr_array.entry[thr_array_cnt] = thr_stage_4;
    }

    int is_thr_stage_1 = pthread_create(&thr_stage_1, &attr, (void * (*) (void *)) stage_1, &thr_array);
    if (is_thr_stage_1) {
        printf("Error: stage 1 thread isn't created\n");
    }

    // exit - threads join, destroy/clean up

    pthread_join(thr_stage_1, NULL);
    printf("Join 1\n");

    pthread_join(thr_stage_2_master, NULL);
    printf("Join 2\n");

    for (int i = 0; i < STAGE2_WORKERS_COUNT; i++) {

        pthread_join(thr_stage_2_workers[i], NULL);


        printf("Join 2W %d\n", i);

    }


    pthread_join(thr_stage_3, NULL);
    printf("Join 3\n");

    pthread_join(thr_stage_4, NULL);
    printf("Join 4\n");



    pthread_attr_destroy(&attr);

    printf("Main is exitting\n");

    return 0;
}
