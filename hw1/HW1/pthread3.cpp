

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <cstdlib>
#include <iostream>
#include <string>
#define SLEEP_MS(ms) usleep((ms)*1000)

using namespace::std;

// Combine locks and values
template<typename T> struct MutexpValue
{
    // value to protect
    T v;
    // mutex
    pthread_mutex_t mutex;
    // cond
    pthread_cond_t cond;
    MutexpValue(){
        pthread_mutex_init(&mutex,NULL);
        pthread_cond_init(&cond,NULL);
    }

    ~MutexpValue(){
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }
};

// queue
MutexpValue< queue<int> > queue_n, queue_s, queue_w, queue_e;

// for priority
MutexpValue<bool> first_n, first_s, first_w, first_e;

// road mutex
MutexpValue<bool> road_a, road_b, road_c, road_d;

// important variable for detection of deadlock
MutexpValue<int> all_num;

// signal to stop deaklock
MutexpValue<bool> deadlock;

// just for lucid expression
enum Direction{
    NORTH = 0,EAST = 1,SOUTH= 2,WEST = 3
};

void init_car(){
// set initial cars
    if(queue_n.v.size()>0){ //if there are v waiting in the queue, pop one and let it go
        pthread_cond_broadcast(&queue_n.cond); //use broadcast to make sure that every car get the chance to check
    }
    if(queue_s.v.size()>0){
        pthread_cond_broadcast(&queue_s.cond);
    }
    if(queue_w.v.size()>0){
        pthread_cond_broadcast(&queue_w.cond);
    }
    if(queue_e.v.size()>0){
        pthread_cond_broadcast(&queue_e.cond);
    }
}


int step1(Direction dir,const long long &car_id){
// This method is to step into the section1
    MutexpValue<bool> *road;
    MutexpValue<bool> *isin;
    int cur_num = 0;
    string direction;
    switch(dir){
        case NORTH:
            road = &road_c;
            direction = "North";
            break;
        case EAST:
            road = &road_b;
            direction = "East";
            break;
        case SOUTH:
            road = &road_a;
            direction = "South";
            break;
        case WEST:
            road = &road_d;
            direction = "West";
            break;
    }
    // step1
    pthread_mutex_lock(&(road->mutex));
    printf("car %lld from %s arrives at crossing\n",car_id,direction.c_str());

    // record
    road->v = true; //mark that there is car in north direction
    pthread_mutex_lock(&all_num.mutex);
    all_num.v--;//let the all_num minus one
    cur_num = all_num.v;
    pthread_mutex_unlock(&all_num.mutex);// the reason why we can unlock all_num here
                                         // is because when cur_num == 0, the 4 cars natually stucked
                                         // together and all_num will not change unless deadlock is solved.

    return cur_num; // cur_num = 0 means deadlock
}
void detect_deadlock(Direction dir,int cur_num){
// This method is to detect deadlock
    // no deaklock ;
    if(cur_num!=0) return ;
    // warning
    string direction;

    // init ;
    MutexpValue<bool> *road;
    MutexpValue<bool> *first, *isin;

    // which direction are you?
    switch(dir){
        case NORTH:
            direction = "East";  road = &road_c;first = &first_e; break;
        case EAST:
            direction = "South"; road = &road_b;first = &first_s; break;
        case SOUTH:
            direction = "West"; road = &road_a;first = &first_w; break;
        case WEST:
            direction = "North"; road = &road_d;first = &first_n; break;
    }
    printf("DEADLOCK car jam detected. signal %s to go\n",direction.c_str());

    // let the last car that cause the deadlock conceed.
    pthread_mutex_unlock(&(road->mutex)); //release the road
    road->v = false; // only cars in the same direction can change this
    pthread_cond_signal(&(first->cond));// send the signal to left car

    // wait deadlock loop
    pthread_mutex_lock(&deadlock.mutex);
    deadlock.v = true;
    while(deadlock.v==true)// for spurious wake-up
        pthread_cond_wait(&deadlock.cond,&deadlock.mutex);
    pthread_mutex_unlock(&deadlock.mutex);

    // when recover from deadlock, get on road again
    pthread_mutex_lock(&(road->mutex));
    road->v = true;
}

void detect_priority(Direction dir){
// This method is to judge if there is cars to wait
    MutexpValue<bool> *isin;
    MutexpValue<bool> *road;
    MutexpValue<bool> *first;
    switch(dir){
        case NORTH:
            road = &road_d;first = &first_n; break;
        case EAST:
            road = &road_c;first = &first_e; break;
        case SOUTH:
            road = &road_b;first = &first_s; break;
        case WEST:
            road = &road_a;first = &first_w; break;
    }

    // all the car stuck here
    pthread_mutex_lock(&(first->mutex));
    while(road->v)// for spurious wake-up
                  // road-v needs no protection because
                  // only 1 thread can change it
                  // and that's car in the same lane
                  // since only one car/lane , this is safe.
        pthread_cond_wait(&(first->cond),&(first->mutex));
    pthread_mutex_unlock(&(first->mutex));
}

void step2(Direction dir,const long long & car_id){
//This method is to enter the second section
    string direction;
    MutexpValue<bool> *r1,*r2,*r3;
    MutexpValue<bool> *isin,*first;
    switch(dir){
        case NORTH:
            r1 = &road_c; r2 = &road_d; r3 = &road_b; first = &first_e; direction = "North";
            break;
        case EAST:
            r1 = &road_b; r2 = &road_c; r3 = &road_a;first = &first_s; direction = "East";
            break;
        case SOUTH:
            r1 = &road_a; r2 = &road_b; r3 = &road_d;first = &first_w; direction = "South";
            break;
        case WEST:
            r1 = &road_d; r2 = &road_a; r3 = &road_c;first = &first_n; direction = "West";
            break;
    }
    // goto stage2
    pthread_mutex_lock(&(r2->mutex));

    // step0) stage1
    pthread_mutex_unlock(&(r1->mutex));

    // step1)source++ (source means cars in stage 1
    // only cars in stage 1 is likely to cause a deadlock)
    pthread_mutex_lock(&all_num.mutex);
    all_num.v++;
    pthread_mutex_unlock(&all_num.mutex);

//    pthread_mutex_lock(&all_num.mutex);
//    all_num.v++;
//    int fuck = all_num.v;
//    pthread_mutex_unlock(&all_num.mutex);
//
//    pthread_mutex_lock(&deadlock.mutex);
//    if (deadlock.v == true)
//    {
//        if(fuck==3)
//        {
//
//            printf("dasd");
//            deadlock.v = false;
//            pthread_cond_signal(&deadlock.cond);
//            pthread_mutex_unlock(&deadlock.mutex);
//            pthread_mutex_lock(&(first_e.mutex));
//            pthread_mutex_lock(&(first_s.mutex));
//            pthread_mutex_lock(&(first_w.mutex));
//            pthread_mutex_lock(&(first_n.mutex));
//            road_a.v = false; //the road doesn't have car
//            road_b.v = false;
//            road_c.v = false;
//            road_d.v = false;
//            pthread_cond_signal(&(first_e.cond));
//            pthread_cond_signal(&(first_s.cond));
//            pthread_cond_signal(&(first_w.cond));
//            pthread_cond_signal(&(first_n.cond));
//            pthread_mutex_unlock(&(first_e.mutex));
//            pthread_mutex_unlock(&(first_s.mutex));
//            pthread_mutex_unlock(&(first_w.mutex));
//            pthread_mutex_unlock(&(first_n.mutex));
//        }
//    }
//    else
//    {
//    printf("fuck");
//        pthread_mutex_unlock(&deadlock.mutex);
//        pthread_mutex_lock(&(first->mutex));
//        r1->v = false; //the road doesn't have car
//        pthread_cond_signal(&first->cond); //send signal to left car
//        pthread_mutex_unlock(&(first->mutex));
//    }
//    pthread_mutex_unlock(&(r2->mutex));
//    pthread_mutex_unlock(&deadlock.mutex);

    // step2)once a car is out, set the deadlock free.
    pthread_mutex_lock(&deadlock.mutex);
    deadlock.v = false;
    pthread_cond_signal(&deadlock.cond);
    pthread_mutex_unlock(&deadlock.mutex);

    // step3)mutex_unlock
    pthread_mutex_unlock(&(r2->mutex));

    // step4)when left the area signal car to go.
    pthread_mutex_lock(&(first->mutex));
    r1->v = false; //the road doesn't have car
    pthread_cond_signal(&first->cond); //send signal to left car
    pthread_mutex_unlock(&(first->mutex));

    // finally
    printf("car %lld from %s leaving crossing\n",car_id,direction.c_str());
}

void step3(Direction dir){
// This method is to signal queue
    MutexpValue<queue<int> > *queue_;
    switch(dir){
        case NORTH:
            queue_ = &queue_n;  break;
        case EAST:
            queue_ = &queue_e;  break;
        case SOUTH:
            queue_ = &queue_s;  break;
        case WEST:
            queue_ = &queue_w;  break;
    }

    // pop the queue
    pthread_mutex_lock(&(queue_->mutex));
    queue_->v.pop();
    pthread_mutex_unlock(&(queue_->mutex));
    pthread_cond_broadcast(&(queue_->cond));
}

void * step0(Direction dir, const long long & car_id){
// basically every queue is waiting here.
    MutexpValue<queue<int> > *queue_;
    switch(dir){
        case NORTH:
            queue_ = &queue_n;  break;
        case EAST:
            queue_ = &queue_e;  break;
        case SOUTH:
            queue_ = &queue_s;  break;
        case WEST:
            queue_ = &queue_w;  break;
    }
    pthread_mutex_lock(&queue_->mutex);
    while(car_id != queue_->v.front()){
        pthread_cond_wait(&queue_->cond,&queue_->mutex);
    }
    pthread_mutex_unlock(&queue_->mutex);

}

void * car_n(void *arg){
    // get the car_id from arg
    long long car_id = reinterpret_cast<long long>(arg);
    int cur_num = 0 ;

    // wait in queue
    step0(NORTH, car_id);

    // enter
    cur_num = step1(NORTH, car_id);

    // detect deadlock
    detect_deadlock(NORTH,cur_num);

    // detect priority
    detect_priority(NORTH);

    // step2
    step2(NORTH,car_id);

    // step2_after
    step3(NORTH);

    return NULL;
}
/* the thread representing the car coming from east */
void * car_e(void *arg){
    // car id
    long long car_id = reinterpret_cast<long long>(arg);
    int cur_num = 0 ;

    // wait in queue
    step0(EAST, car_id);
    cur_num = step1(EAST,car_id);
    detect_deadlock(EAST,cur_num);
    detect_priority(EAST);
    step2(EAST,car_id);
    step3(EAST);
    return NULL;
}

/* the thread representing the car from south */
void * car_s(void *arg){
    /* get the car_id from arg*/
    long long car_id = reinterpret_cast<long long>(arg);
    int cur_num = 0 ;

    // wait in queue
    step0(SOUTH, car_id);
    cur_num = step1(SOUTH,car_id);
    detect_deadlock(SOUTH,cur_num);
    detect_priority(SOUTH);
    step2(SOUTH,car_id);
    step3(SOUTH);
    return NULL;
}

/* the thread representing the car from west */
void * car_w(void *arg){
    /* get the car_id from arg*/
    long long car_id = reinterpret_cast<long long>(arg);
    int cur_num = 0 ;

    // wait in queue
    step0(WEST, car_id);
    cur_num = step1(WEST,car_id);
    detect_deadlock(WEST,cur_num);
    detect_priority(WEST);
    step2(WEST,car_id);
    step3(WEST);
    return NULL;
}

# include <stdio.h>
# include <stdlib.h>
int main(int argc,char *argv[]){

    // set temps
    int MAX = atoi(argv[1]);
    int rando[MAX];
    pthread_t car[MAX+1];
    srand((unsigned)time(NULL));

    // set locks
    road_a.v = road_b.v = road_c.v = road_d.v = false;
    all_num.v = 4;

    for (int i = 0; i<MAX; i++)
    {
      rando[i] = rand()%4;
    }
    for (int i = 0; i<MAX; i++)
    {
        switch (rando[i]) {
        case 0: {
            queue_n.v.push(i);
            pthread_create(&car[i],NULL,car_n,reinterpret_cast<void *>(i));
            break;
        }
        case 1: {
            queue_w.v.push(i);
            pthread_create(&car[i],NULL,car_w,reinterpret_cast<void *>(i));
            break;
        }
        case 2: {
            queue_e.v.push(i);
            pthread_create(&car[i],NULL,car_e,reinterpret_cast<void *>(i));
            break;
        }
        case 3: {
            queue_s.v.push(i);
            pthread_create(&car[i],NULL,car_s,reinterpret_cast<void *>(i));
            break;
        }
       }
    }

    // wake up the car in front of queue
    init_car();

    // check every car has gone
    sleep(2);
    printf("%d %d %d %d",queue_e.v.size(),queue_s.v.size(),queue_w.v.size(),queue_n.v.size());

    // join threads
    for(int i=0; i<MAX; i++)
    {
        pthread_join(car[i],NULL);
    }
}

