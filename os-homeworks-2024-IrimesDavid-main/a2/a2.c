#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

//GLOBALS
int *syncP6P7;
sem_t sem7;
sem_t start4;
sem_t end2;

sem_t sem5;
pthread_barrier_t barrier;
int threadCounter = 0;
sem_t waitForBros;

sem_t sem6;
sem_t *t5Sem;
sem_t *t1Sem;
//

//---------------------------------------------------------------------------------------------
void *thread_function5(void *used) {
    int t = *(int*) used;

    sem_wait(&sem5);
    info(BEGIN, 5, t);
    if(t != 14){
        threadCounter++;
        if(threadCounter <= 4){
            if(threadCounter == 4)
                sem_post(&waitForBros);
            pthread_barrier_wait(&barrier);
        }
    }
    if(t == 14){
        sem_wait(&waitForBros);
        info(END, 5, t);
        sem_post(&sem5);
        pthread_barrier_wait(&barrier);
        return NULL;
    }
    info(END, 5, t);
    sem_post(&sem5);
        
    return NULL;
}
void *thread_function6(void *used) {
    int t = *(int*) used;
    if(t == 1){
        sem_wait(t1Sem);
        info(BEGIN, 6, t);
        info(END, 6, t);
    }
    else if(t == 4){
        info(BEGIN, 6, t);
        info(END, 6, t);
        sem_post(t5Sem);
        
    }else{
    sem_wait(&sem6);
    info(BEGIN, 6, t);
    info(END, 6, t);
    sem_post(&sem6);
    }

    return NULL;
}
void *thread_function7(void *used) {
    int t = *(int*) used;
    if(t == 5){
        sem_wait(t5Sem);
        info(BEGIN, 7, t);
        info(END, 7, t);
        sem_post(t1Sem);
    }else if(t != 2 && t != 4){
        sem_wait(&sem7);
        info(BEGIN, 7, t);
        info(END, 7, t);
        sem_post(&sem7);
    }
    else{
        if(t == 2){
            sem_wait(&start4);
            info(BEGIN, 7, t);
            info(END, 7, t);
            sem_post(&end2);
        }
        if(t == 4){
            info(BEGIN, 7, t);
            sem_post(&start4);
            sem_wait(&end2);
            info(END, 7, t);
        }
    }
    return NULL;
}
//---------------------------------------------------------------------------------------------
int main(){
    init();

    t5Sem = sem_open("/t5Sem",O_CREAT,0664,0);
    t1Sem = sem_open("/t1Sem",O_CREAT,0664,0);

    info(BEGIN, 1, 0);

    if(fork() == 0){
        info(BEGIN, 2, 0);

        if(fork() == 0){
            info(BEGIN, 7, 0);

            pthread_t threads[5];
            int thread_ids[5];
            int i;
            sem_init(&sem7, 0, 1);
            sem_init(&start4, 0, 0);
            sem_init(&end2, 0, 0);

            for (i = 0; i < 5; i++) {
                thread_ids[i] = i + 1;
                pthread_create(&threads[i], NULL, thread_function7, (void *)&thread_ids[i]);
            }

            for (i = 0; i < 5; i++) {
                pthread_join(threads[i], NULL);
            }
            sem_destroy(&sem7);
            sem_destroy(&start4);
            sem_destroy(&end2);
            sem_close(t5Sem);
            sem_unlink("/t5Sem");

            info(END, 7, 0);
            return 0;
        }

        if(fork() == 0){
            info(BEGIN, 5, 0);
            
            pthread_t threads[43];
            int thread_ids[43];
            int i;

            sem_init(&sem5, 0, 5);
            sem_init(&waitForBros, 0, 0);
            pthread_barrier_init(&barrier, NULL, 5);

            for (i = 0; i < 43; i++) {
                thread_ids[i] = i + 1;
                pthread_create(&threads[i], NULL, thread_function5, (void *)&thread_ids[i]);
            }

            for (i = 0; i < 43; i++) {
                pthread_join(threads[i], NULL);
            }
            sem_destroy(&sem5);
            sem_destroy(&waitForBros);
            pthread_barrier_destroy(&barrier);

            info(END, 5, 0);
            return 0;
        }
        
        wait(NULL);
        wait(NULL);
        info(END, 2, 0);
        return 0;
    }

    if(fork() == 0){
        info(BEGIN, 3, 0);

        if(fork() == 0){
            info(BEGIN, 4, 0);

            if(fork() == 0){
                info(BEGIN, 6, 0);

                pthread_t threads[5];
                int thread_ids[5];
                int i;
                sem_init(&sem6, 0, 1);

                for (i = 0; i < 5; i++) {
                    thread_ids[i] = i + 1;
                    pthread_create(&threads[i], NULL, thread_function6, (void *)&thread_ids[i]);
                }

                for (i = 0; i < 5; i++) {
                    pthread_join(threads[i], NULL);
                }
                sem_destroy(&sem6);
                sem_close(t1Sem);
                sem_unlink("/t1Sem");

                info(END, 6, 0);
                return 0;
            }

            wait(NULL);
            info(END, 4, 0);
            return 0;
        }

        wait(NULL);
        info(END, 3, 0);
        return 0;
    }

    wait(NULL);
    wait(NULL);
    info(END, 1, 0);
    return 0;
}
