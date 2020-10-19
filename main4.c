#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_cond_t cond_var;

pthread_mutex_t mutex;


struct data_t {
    pthread_t data;
};

void *sleeper(void * attr) {

        printf("S: Before pthread_cond_wait\n");

        //pthread_cond_wait(&cond_var, &mutex);

        printf("S: After pthread_cond_wait\n");


    pthread_exit(NULL);
}

void *killer(void * attr) {
    struct data_t * thr_attr_data = (struct data_t *) attr;

    printf("K: Sleeping\n");

    sleep(2);

    printf("K: Canceling\n");

    pthread_cancel(thr_attr_data->data);

    printf("K: Exiting\n");

    pthread_exit(NULL);
}

int main () {
    struct data_t thr_data;

    pthread_mutex_init(&mutex, NULL);

    pthread_cond_init(&cond_var, NULL);

    pthread_attr_t thr_attr;
    pthread_attr_init(&thr_attr);
    pthread_attr_setdetachstate(&thr_attr, PTHREAD_CREATE_JOINABLE);

    pthread_t thr_sleeper;
    pthread_t thr_killer;

    pthread_create(&thr_sleeper, &thr_attr, (void * (*) (void *)) sleeper, NULL);

    thr_data.data = thr_sleeper;

    pthread_create(&thr_killer, &thr_attr, (void * (*) (void *)) killer, &thr_data);


    pthread_join(thr_sleeper, NULL);
    pthread_join(thr_killer, NULL);

    pthread_attr_destroy(&thr_attr);

    return 0;
}