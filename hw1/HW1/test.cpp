#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <pthread.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int i = 0;

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* thread1(void *arg)
{
    for (;;)
    {
        printf("thread1 loop...\n");
        pthread_mutex_lock(&lock);
        printf("thread1 wait cond...\n");
        pthread_cond_wait(&cond, &lock);
        printf("thread1 work...%d\n", i);
        pthread_mutex_unlock(&lock);
    }

    return (void*)0;
}

void* thread2(void *arg)
{
    sleep(1);

    while (++i < 100)
    {
        printf("thread2 loop...\n");

        if (i % 10 == 0)
        {
            pthread_mutex_lock(&lock);
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&lock);
            sleep(1);
        }
    }

    printf("thread2 exit\n");
    return (void*)0;
}

int main(void)
{
    pthread_t ntid1;
    pthread_t ntid2;
    pthread_create(&ntid1, NULL, thread1, NULL);
    pthread_create(&ntid2, NULL, thread2, NULL);

    pthread_join(ntid1, NULL);
    pthread_join(ntid2, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    return EXIT_SUCCESS;
}

