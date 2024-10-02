#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#define SYNC_P6P7_KEY 1234
#define T5SEM_KEY 5678
#define T1SEM_KEY 91011

//GLOBALS
int *syncP6P7;
sem_t sem7;
int start4 = 0;
int end2 = 0;
sem_t sem5;
pthread_barrier_t barrier;
int waitFor14 = 0;
int brosLeft = 4;
sem_t sem6;
int *t5Flg;
int *t1Flg;
//

//---------------------------------------------------------------------------------------------
void *thread_function5(void *used) {
    int t = *(int*) used;
    
    sem_wait(&sem5);
    info(BEGIN, 5, t);
    info(END, 5, t);
    sem_post(&sem5);

    if(waitFor14 < brosLeft) {
        waitFor14++;
        pthread_barrier_wait(&barrier);
        sleep(1);
        return NULL;
    }
    if(t == 14){
        waitFor14++;
        brosLeft++;
        pthread_barrier_wait(&barrier);
    }
        
    return NULL;
}
void *thread_function6(void *used) {
    int t = *(int*) used;
    if(t == 1){
        while(*t1Flg == 0){
            //wait
        }
        info(BEGIN, 6, t);
        info(END, 6, t);
    }
    else if(t == 4){
        info(BEGIN, 6, t);
        info(END, 6, t);
        *t5Flg = 1;
        
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
        while(*t5Flg == 0){
            //wait
        }
        info(BEGIN, 7, t);
        info(END, 7, t);
        *t1Flg = 1;
    }else if(t != 2 && t != 4){
        sem_wait(&sem7);
        info(BEGIN, 7, t);
        info(END, 7, t);
        sem_post(&sem7);
    }
    else{
        if(t == 2){
            while(!start4){
                //wait
            }
            info(BEGIN, 7, t);
            info(END, 7, t);
            end2 = 1;
        }
        if(t == 4){
            info(BEGIN, 7, t);
            start4 = 1;
            while(!end2){
                //wait
            }
            info(END, 7, t);
        }
    }
    return NULL;
}
//---------------------------------------------------------------------------------------------
int main(){
    init();
    int shm_id = shmget(SYNC_P6P7_KEY, sizeof(int), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        return 1;
    }
    int shm_id1 = shmget(T5SEM_KEY, sizeof(int), IPC_CREAT | 0666);
    if (shm_id1 == -1) {
        perror("shmget");
        return 1;
    }
    int shm_id2 = shmget(T1SEM_KEY, sizeof(int), IPC_CREAT | 0666);
    if (shm_id2 == -1) {
        perror("shmget");
        return 1;
    }

    syncP6P7 = (int *)shmat(shm_id, NULL, 0);
    if (syncP6P7 == (int *)(-1)) {
        perror("shmat");
        return 1;
    }
    *syncP6P7 = 0;
    t5Flg = (int *)shmat(shm_id1, NULL, 0);
    if (t5Flg == (int *)(-1)) {
        perror("shmat");
        return 1;
    }
    *t5Flg = 0;
    t1Flg = (int *)shmat(shm_id2, NULL, 0);
    if (t1Flg == (int *)(-1)) {
        perror("shmat");
        return 1;
    }
    *t1Flg = 0;

    info(BEGIN, 1, 0);

    if(fork() == 0){
        info(BEGIN, 2, 0);

        if(fork() == 0){
            *syncP6P7 = *syncP6P7 + 1;
            while(*syncP6P7 < 2){
                //wait
            }
            info(BEGIN, 7, 0);

            pthread_t threads[5];
            int thread_ids[5];
            int i;
            sem_init(&sem7, 0, 1);

            for (i = 0; i < 5; i++) {
                thread_ids[i] = i + 1;
                pthread_create(&threads[i], NULL, thread_function7, (void *)&thread_ids[i]);
            }

            for (i = 0; i < 5; i++) {
                pthread_join(threads[i], NULL);
            }
            sem_destroy(&sem7);

            info(END, 7, 0);
            return 0;
        }

        if(fork() == 0){
            info(BEGIN, 5, 0);
            
            pthread_t threads[43];
            int thread_ids[43];
            int i;

            sem_init(&sem5, 0, 5);
            pthread_barrier_init(&barrier, NULL, 5);


            for (i = 0; i < 43; i++) {
                thread_ids[i] = i + 1;
                pthread_create(&threads[i], NULL, thread_function5, (void *)&thread_ids[i]);
            }

            for (i = 0; i < 43; i++) {
                pthread_join(threads[i], NULL);
            }
            sem_destroy(&sem5);
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
                *syncP6P7 = *syncP6P7 + 1;
                while(*syncP6P7 < 2){
                    //wait
                }
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
