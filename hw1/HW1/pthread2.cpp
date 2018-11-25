#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <cstdlib>
#include <iostream>
#define MAX 100
using namespace std;
//stage zero: wait to be in
//stage one : in the first cross
//stage two : in the second cross

//s0: mutex of the step before cross
pthread_mutex_t zero_east_m;
pthread_mutex_t zero_west_m;
pthread_mutex_t zero_north_m;
pthread_mutex_t zero_south_m;

//s1: bundle for priority
pthread_mutex_t first_south_m;
pthread_mutex_t first_north_m;
pthread_mutex_t first_east_m;
pthread_mutex_t first_west_m;
pthread_cond_t first_east;
pthread_cond_t first_west;
pthread_cond_t first_north;
pthread_cond_t first_south;
int first_n;
int first_e;
int first_w;
int first_s;

//s2: bundle for priority
pthread_mutex_t second_south_m;
pthread_mutex_t second_north_m;
pthread_mutex_t second_east_m;
pthread_mutex_t second_west_m;
pthread_cond_t second_west;
pthread_cond_t second_south;
pthread_cond_t second_north;
pthread_cond_t second_east;
int second_e;
int second_s;
int second_w;
int second_n;

//bundle for count of available spaces
pthread_mutex_t all_num_lock;
int all_num;

//bundle for deadlock
pthread_mutex_t deadlock_m;
pthread_cond_t cond_deadlock;
int deadlock = 0;

//number of cars
int south_num;
int west_num;
int north_num;
int east_num;

//save all the thread
pthread_t car[MAX];

//the queue of car
queue <int>car_south;
queue <int>car_east;
queue <int>car_north;
queue <int>car_west;


void *car_from_south(void *arg) {
 //this car id
    int id = 0;
    //step0: enter the waiting section
    pthread_mutex_lock(&zero_south_m);
    //get this car into queue
    south_num ++;
    car_south.push(south_num);
    id = south_num;
    //first car enter unconditionally
    if(id != 1 &&  first_s == 0)
        //basically the queue is stucked here waiting to enter.
        pthread_cond_wait(&first_south, &zero_south_m);
    //mutex protects the queue, once the queue has no use, unlock;
    pthread_mutex_unlock(&zero_south_m);
    usleep(100);
    //step0 to 1: enter when proper conditions
    pthread_mutex_lock(&all_num_lock);
    //once decided this car is the one to enter
    //set second_west to 0 to give this car high priority
    //basically to stop the west car from entering
    printf("------------------------------------------------\n");
    printf("%d south\n",id);
    pthread_mutex_lock(&second_south_m);
    second_s = 0;
    pthread_mutex_unlock(&second_south_m);

    //check for deadlock possibility
    if (all_num == 3)
    {
        //condition bundle for deadlock
        printf("%d south\n",id);
        pthread_mutex_lock(&deadlock_m);
        printf("%d south\n",id);
        pthread_mutex_lock(&second_south_m);
        second_s = 1;
        pthread_cond_signal(&second_south);
        pthread_mutex_unlock(&second_south_m);
        //wait for signal
        printf("car %d from South waiting to prevent a deadlock\n", id);
        pthread_cond_wait(&cond_deadlock, &deadlock_m);
        deadlock = 1;
        pthread_mutex_unlock(&deadlock_m);
        pthread_mutex_lock(&all_num_lock);
        all_num --;
        pthread_mutex_unlock(&all_num_lock);
        goto step1;
    }
    if (all_num == 0 || all_num == 1 || all_num == 2)
    {
        step1:
        //step1: enter south section
        printf("%d south\n",id);
        pthread_mutex_lock(&first_south_m);
            first_s = 0;
        printf("car %d from South arrives at crossing\n", id);
        all_num ++;
        //allow to step inside
        pthread_mutex_unlock(&all_num_lock);

        //step2: check conditions for car to enter east section
        printf("%d south\n",id);
        pthread_mutex_lock(&deadlock_m);

        //condition1: if dead_locked then go
        if(deadlock != 0)
        {
            printf("south %d entered deaclock mutex lock\n", id);
            //unlock and go
            pthread_mutex_unlock(&deadlock_m);
        }
        //condition2: if not dead_locked then need a signal to let it go
        else
        {
            //unlock and check other conditions
            pthread_mutex_unlock(&deadlock_m);
            //check condition
            printf("%d south\n",id);
            pthread_mutex_lock(&second_east_m);
            //second_e = 0 means that no priority issues in section east
            //second_e = 1 means need to wait for east car to go first
            if (second_e == 0)
                {printf("south %d enter priority lock\n", id);
                pthread_cond_wait(&second_east, &second_east_m);
                printf("south %d leave priority lock\n", id);}
            pthread_mutex_unlock(&second_east_m);
        }
    }


    //enjoy the moment of success
    printf("%d south\n",id);
    pthread_mutex_lock(&first_east_m);
    pthread_mutex_unlock(&first_east_m);

    //step3: finally enter when needs are met
    pthread_mutex_unlock(&first_south_m);
    printf("car %d from South leaves at crossing\n", id);

    //1)solve priority issue
    printf("%d south\n",id);
    pthread_mutex_lock(&second_south_m);
    pthread_cond_signal(&second_south);
    second_s = 1;
    pthread_mutex_unlock(&second_south_m);

    //step4: aftermath:
    //0)signal deadlock: meaning there is no risks for deadlocks
    printf("%d south\n",id);
      pthread_mutex_lock(&deadlock_m);
    if(deadlock != 0)
        {pthread_cond_signal(&cond_deadlock);
        deadlock = 0;}
    else
        {pthread_mutex_lock(&all_num_lock);
        all_num --;
        pthread_mutex_unlock(&all_num_lock);}
    pthread_mutex_unlock(&deadlock_m);


    printf("--------------------end%d----------------------------\n",id);

    //2)pop the queue
    pthread_mutex_lock(&first_south_m);
        first_s = 1;
    car_south.pop();
    pthread_cond_signal(&first_south);
    pthread_mutex_unlock(&first_south_m);

    //3)rest for a while
    usleep(2000);
    //4)r.i.p
}
void *car_from_east(void *arg) {
    //this car id
    int id = 0;
    //step0: enter the waiting section
    pthread_mutex_lock(&zero_east_m);
    //get this car into queue
    east_num ++;
    car_east.push(east_num);
    id = east_num;

    //first car enter unconditionally
    if(id != 1 && first_e == 0)
        //basically the queue is stucked here waiting to enter.
        pthread_cond_wait(&first_east, &zero_east_m);
    //mutex protects the queue, once the queue has no use, unlock;
    pthread_mutex_unlock(&zero_east_m);
    usleep(100);
    //step0 to 1: enter when proper conditions
    pthread_mutex_lock(&all_num_lock);
    //once decided this car is the one to enter
    //set second_west to 0 to give this car high priority
    //basically to stop the west car from entering
    printf("------------------------------------------------\n");
    printf("%d east\n",id);
    pthread_mutex_lock(&second_east_m);
    second_e = 0;
    pthread_mutex_unlock(&second_east_m);

    //check for deadlock possibility
    if (all_num == 3)
    {
        //condition bundle for deadlock
        printf("%d east\n",id);
        pthread_mutex_lock(&deadlock_m);
        printf("%d east\n",id);
        pthread_mutex_lock(&second_east_m);
        pthread_cond_signal(&second_east);
        second_e = 1;
        pthread_mutex_unlock(&second_east_m);
        //wait for signal
        printf("car %d from East waiting to prevent a deadlock\n", id);
        pthread_cond_wait(&cond_deadlock, &deadlock_m);
        deadlock = 2;
        pthread_mutex_unlock(&deadlock_m);
        pthread_mutex_lock(&all_num_lock);
        all_num --;
        pthread_mutex_unlock(&all_num_lock);

        goto step1;
    }
    if (all_num == 0 || all_num == 1 || all_num == 2)
    {
        step1:
        //step1: enter south section
        printf("%d east\n",id);
        pthread_mutex_lock(&first_east_m);
        first_e = 0;
        printf("car %d from East arrives at crossing\n", id);
        all_num ++;
        //allow to step inside
        pthread_mutex_unlock(&all_num_lock);

        //step2: check conditions for car to enter east section
        printf("%d east\n",id);
        pthread_mutex_lock(&deadlock_m);
        //condition1: if dead_locked then go
        if(deadlock != 0)
        {
            //unlock and go
            pthread_mutex_unlock(&deadlock_m);
        }
        //condition2: if not dead_locked then need a signal to let it go
        else
        {
            //unlock and check other conditions
            pthread_mutex_unlock(&deadlock_m);
            //check condition
            printf("%d east\n",id);
            pthread_mutex_lock(&second_north_m);

            //second_e = 0 means that no priority issues in section east
            //second_e = 1 means need to wait for east car to go first
            if (second_n == 0)
                {printf("east %d entered priority lock\n", id);
                pthread_cond_wait(&second_north, &second_north_m);
                printf("east %d leave priority lock\n", id);}
            pthread_mutex_unlock(&second_north_m);
        }
    }

    //step3: finally eer when needs are met
    printf("%d east\n",id);
    pthread_mutex_lock(&first_north_m);
    pthread_mutex_unlock(&first_north_m);
    //enjoy the moment of success
    pthread_mutex_unlock(&first_east_m);
    printf("car %d from East leaves at crossing\n", id);


        //1)solve priority issue
    printf("%d east\n",id);
    pthread_mutex_lock(&second_east_m);
    pthread_cond_signal(&second_east);
    second_e = 1;
    pthread_mutex_unlock(&second_east_m);
    //step4: aftermath:
    //0)signal deadlock: meaning there is no risks for deadlocks
    printf("%d east\n",id);
      pthread_mutex_lock(&deadlock_m);
    if(deadlock != 0)
        {pthread_cond_signal(&cond_deadlock);
        deadlock = 0;}
    else
        {pthread_mutex_lock(&all_num_lock);
        all_num --;
        pthread_mutex_unlock(&all_num_lock);}
    pthread_mutex_unlock(&deadlock_m);


   printf("--------------------end%d----------------------------\n",id);

    //2)pop the queue
    pthread_mutex_lock(&first_east_m);
    first_e = 1;
    car_east.pop();
    pthread_cond_signal(&first_east);
    pthread_mutex_unlock(&first_east_m);

    //3)rest for a while
    usleep(2000);
    //4)r.i.p
}
void *car_from_north(void *arg) {
    //this car id
    int id = 0;
    //step0: enter the waiting section
    pthread_mutex_lock(&zero_north_m);
    //get this car into queue
    north_num ++;
    car_north.push(north_num);
    id = north_num;

    //first car enter unconditionally
    if(id != 1 && first_n == 0)
        //basically the queue is stucked here waiting to enter.
        pthread_cond_wait(&first_north, &zero_north_m);
    //mutex protects the queue, once the queue has no use, unlock;
    pthread_mutex_unlock(&zero_north_m);
    usleep(100);
    //step0 to 1: enter when proper conditions
    pthread_mutex_lock(&all_num_lock);
    //once decided this car is the one to enter
    //set second_west to 0 to give this car high priority
    //basically to stop the west car from entering
    printf("------------------------------------------------\n");
    printf("%d north\n",id);
    pthread_mutex_lock(&second_north_m);
    second_n = 0;
    pthread_mutex_unlock(&second_north_m);

    //check for deadlock possibility
    if (all_num == 3)
    {
        //condition bundle for deadlock
        printf("%d north\n",id);
        pthread_mutex_lock(&deadlock_m);
        printf("%d north\n",id);
        pthread_mutex_lock(&second_north_m);
        pthread_cond_signal(&second_north);
        second_n = 1;
        pthread_mutex_unlock(&second_north_m);
        //wait for signal
        printf("car %d from North waiting to prevent a deadlock\n", id);
        pthread_cond_wait(&cond_deadlock, &deadlock_m);
        deadlock = 3;
        pthread_mutex_unlock(&deadlock_m);

        pthread_mutex_lock(&all_num_lock);
        all_num --;
        pthread_mutex_unlock(&all_num_lock);
        goto step1;
    }
    if (all_num == 0 || all_num == 1 || all_num == 2)
    {
        step1:
        //step1: enter south section
        printf("%d north\n",id);
        pthread_mutex_lock(&first_north_m);
        first_n = 0;
        printf("car %d from North arrives at crossing\n", id);
        all_num ++;
        //allow to step inside
        pthread_mutex_unlock(&all_num_lock);

        //step2: check conditions for car to enter east section
        printf("%d north\n",id);
        pthread_mutex_lock(&deadlock_m);
        //condition1: if dead_locked then go
        if(deadlock != 0)
        {
            //unlock and go
            pthread_mutex_unlock(&deadlock_m);
        }
        //condition2: if not dead_locked then need a signal to let it go
        else
        {

            //unlock and check other conditions
            pthread_mutex_unlock(&deadlock_m);
            //check condition
            printf("%d north\n",id);
            pthread_mutex_lock(&second_west_m);
            //second_e = 0 means that no priority issues in section east
            //second_e = 1 means need to wait for east car to go first
            if (second_w == 0)
                {printf("north %d entered priority lock\n", id);
                pthread_cond_wait(&second_west, &second_west_m);
                printf("north %d leave priority lock\n", id);}
            pthread_mutex_unlock(&second_west_m);
        }
    }

    //step3: finally enter when needs are met
    printf("%d north\n",id);
    pthread_mutex_lock(&first_west_m);
    //enjoy the moment of success
    pthread_mutex_unlock(&first_west_m);
    pthread_mutex_unlock(&first_north_m);
    printf("car %d from North leaves at crossing\n", id);

    //step4: aftermath:
    //0)signal deadlock: meaning there is no risks for deadlocks
    //1)solve priority issue
    printf("%d north\n",id);
    pthread_mutex_lock(&second_north_m);
    pthread_cond_signal(&second_north);
    second_n = 1;
    pthread_mutex_unlock(&second_north_m);

    printf("%d north\n",id);
    pthread_mutex_lock(&deadlock_m);
    if(deadlock != 0)
        {pthread_cond_signal(&cond_deadlock);
        deadlock = 0;}
    else
        {pthread_mutex_lock(&all_num_lock);
        all_num --;
        pthread_mutex_unlock(&all_num_lock);}
    pthread_mutex_unlock(&deadlock_m);


printf("--------------------end%d----------------------------\n",id);
    //2)pop the queue
    pthread_mutex_lock(&first_north_m);
    first_n = 1;
    car_north.pop();
    pthread_cond_signal(&first_north);
    pthread_mutex_unlock(&first_north_m);

    //3)rest for a while
    usleep(2000);
    //4)r.i.p
}

void *car_from_west(void *arg) {

    //this car id
    int id = 0;
    //step0: enter the waiting section
    pthread_mutex_lock(&zero_west_m);
    //get this car into queue
    west_num ++;
    car_west.push(west_num);
    id = west_num;

    //first car enter unconditionally
    if(id != 1 && first_w == 0)
        //basically the queue is stucked here waiting to enter.
        pthread_cond_wait(&first_west, &zero_west_m);
    //mutex protects the queue, once the queue has no use, unlock;
    pthread_mutex_unlock(&zero_west_m);

    usleep(100);
    //step0 to 1: enter when proper conditions
    pthread_mutex_lock(&all_num_lock);
    //once decided this car is the one to enter
    //set second_west to 0 to give this car high priority
    //basically to stop the west car from entering
    printf("------------------------------------------------\n");
    printf("%d west\n",id);
    pthread_mutex_lock(&second_west_m);
    second_w = 0;
    pthread_mutex_unlock(&second_west_m);

    //check for deadlock possibility
    if (all_num == 3)
    {
        //condition bundle for deadlock
        printf("%d west\n",id);
        pthread_mutex_lock(&deadlock_m);
        printf("%d west\n",id);
        pthread_mutex_lock(&second_west_m);
        pthread_cond_signal(&second_west);
        second_w = 1;
        pthread_mutex_unlock(&second_west_m);
        //wait for signal
        printf("car %d from West waiting to prevent a deadlock\n", id);
        pthread_cond_wait(&cond_deadlock, &deadlock_m);
        deadlock = 4;
        pthread_mutex_unlock(&deadlock_m);

        pthread_mutex_lock(&all_num_lock);
        all_num --;
        pthread_mutex_unlock(&all_num_lock);
        goto step1;
    }
    if (all_num == 0 || all_num == 1 || all_num == 2)
    {
        step1:

        //step1: enter south section
        printf("%d west\n",id);
        pthread_mutex_lock(&first_west_m);
        first_w = 0;
        printf("car %d from West arrives at crossing\n", id);
        all_num ++;
        //allow to step inside
        pthread_mutex_unlock(&all_num_lock);

        //step2: check conditions for car to enter east section
        printf("%d west\n",id);
        pthread_mutex_lock(&deadlock_m);
        //condition1: if dead_locked then go
        if(deadlock != 0)
        {
            //unlock and go
            pthread_mutex_unlock(&deadlock_m);
        }
        //condition2: if not dead_locked then need a signal to let it go
        else
        {
            //unlock and check other conditions
            pthread_mutex_unlock(&deadlock_m);
            //check condition
            printf("%d west\n",id);
            pthread_mutex_lock(&second_south_m);
            //second_e = 0 means that no priority issues in section east
            //second_e = 1 means need to wait for east car to go first
            if (second_s == 0){
                printf("west %d enter priority lock\n", id);
                pthread_cond_wait(&second_south, &second_south_m);
                printf("west %d leave priority lock\n", id);}
            pthread_mutex_unlock(&second_south_m);

        }
    }

    //step3: finally enter when needs are met
    printf("%d west\n",id);
    pthread_mutex_lock(&first_south_m);
    pthread_mutex_unlock(&first_south_m);
    pthread_mutex_unlock(&first_west_m);
    //enjoy the moment of success
    printf("car %d from West leaves at crossing\n", id);


    printf("%d west\n",id);
    pthread_mutex_lock(&second_west_m);
    //1)solve priority issue
    pthread_cond_signal(&second_west);
    second_w = 1;
    pthread_mutex_unlock(&second_west_m);

    //step4: aftermath:
    //0)signal deadlock: meaning there is no risks for deadlocks

    printf("%d west\n",id);
    pthread_mutex_lock(&deadlock_m);
    if(deadlock != 0)
        {pthread_cond_signal(&cond_deadlock);
        deadlock = 0;}
    else
        {pthread_mutex_lock(&all_num_lock);
        all_num --;
        pthread_mutex_unlock(&all_num_lock);}
    pthread_mutex_unlock(&deadlock_m);



printf("--------------------end%d----------------------------\n",id);
    //2)pop the queue
    pthread_mutex_lock(&first_west_m);
    first_w = 1;
    car_west.pop();
    pthread_cond_signal(&first_west);
    pthread_mutex_unlock(&first_west_m);

    //3)rest for a while
    usleep(2000);
    //4)r.i.p
}
#include <stdlib.h>
#include <cstdlib>
#include <time.h>
#include <stdio.h>

int main(int argc, char** argv) {

//s1: mutex of the 4 cross and the cond to let them in
pthread_mutex_init( &zero_east_m, NULL);
pthread_mutex_init( &zero_west_m, NULL);
pthread_mutex_init( &zero_north_m, NULL);
pthread_mutex_init( &zero_south_m, NULL);

//s1: bundle for priority
pthread_mutex_init( &first_south_m, NULL);
pthread_mutex_init( &first_north_m, NULL);
pthread_mutex_init( &first_east_m, NULL);
pthread_mutex_init( &first_west_m, NULL);
pthread_cond_init( &first_east, NULL);
pthread_cond_init( &first_west, NULL);
pthread_cond_init( &first_north, NULL);
pthread_cond_init( &first_south, NULL);
first_n =1 ;
first_e =1 ;
first_s =1 ;
first_w =1 ;

//s2: bundle for priority
pthread_mutex_init( &second_south_m, NULL);
pthread_mutex_init( &second_north_m, NULL);
pthread_mutex_init( &second_east_m, NULL);
pthread_mutex_init( &second_west_m, NULL);
pthread_cond_init( &second_west, NULL);
pthread_cond_init( &second_south, NULL);
pthread_cond_init( &second_north, NULL);
pthread_cond_init( &second_east, NULL);
second_e = 1;
second_s = 1;
second_w = 1;
second_n = 1;

//bundle for count of available spaces
pthread_mutex_init( &all_num_lock, NULL);
all_num = 0;

//bundle for deadlock
pthread_mutex_init( &deadlock_m, NULL);
pthread_cond_t deadlock;
south_num = 0;
west_num = 0;
north_num = 0;
east_num = 0;


    //create the thread
    //decide random cars
    srand((unsigned)time(NULL));
    int rando[MAX];
    int size = 0;
    for(int i = 0; i <= MAX-1; i++)
        rando[i] = rand()%4;
    for (int i = 0; i<MAX; i++)
    {
        switch (rando[i]) {
        case 0: {
            pthread_create(&car[i],
                NULL, car_from_west, NULL);
            break;
        }
        case 1: {
            pthread_create(&car[i],
                NULL, car_from_east, NULL);
            break;
        }
        case 2: {
            pthread_create(&car[i],
                NULL, car_from_south, NULL);
            break;
        }
        case 3: {
            pthread_create(&car[i],
                NULL, car_from_north, NULL);
            break;
        }
        }
        if (i % 4 == 3)
            usleep(100);
    }


    sleep(2);
    printf("east %d",int(car_east.size()));
    printf("south %d",int(car_south.size()));
    printf("west %d",int(car_west.size()));
    printf("north %d",int(car_north.size()));
    //join the thread
    for (int i = 0; i<size; i++) {
        pthread_join(car[i], NULL);
    }
}
