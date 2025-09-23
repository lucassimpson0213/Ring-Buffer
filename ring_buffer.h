#ifndef BUFFER_H
#define BUFFER_H
#define BUFFER_SIZE 8
#include <semaphore.h>
#include <stdio.h>
#include <pthread.h>


struct RingBuffer {
   int ring_array [BUFFER_SIZE];
   int start;
   int end;

} buffer;


sem_t count, spaces;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct RingBuffer RingBuffer;

void init() {
    sem_init(&count, 0, 0);
    sem_init(&spaces, 0, BUFFER_SIZE);
}


void init_buffer(RingBuffer * buffer) {
    buffer->start = 0;
    buffer->end = 0;
    for(int i = 0; i < BUFFER_SIZE; i++) {
        buffer->ring_array[i] = 0;
    }
}


int is_empty(RingBuffer * buffer) {
    if(buffer->start == buffer->end) {
        return 1;
    }
    else {
        return 0;
    }
}


//enqueue is O(1) because of constant time incrementing of value and addition
void enqueue(RingBuffer * buffer, unsigned int value) {

    sem_wait(&spaces);

    printf("%i", buffer->end - buffer->start);
    if((buffer->end - buffer->start) < BUFFER_SIZE) {
        pthread_mutex_lock(&mutex);
        buffer->ring_array[(buffer->end++ ) & (BUFFER_SIZE - 1) ] = value;
        pthread_mutex_unlock(&mutex);
        sem_post(&count);
    }
    else {
        printf("buffer is max size right now check back later");
    }

}
//deque is O(1) due to constant time incrementing and removal of value
void dequeue(RingBuffer * buffer) {
  
    sem_wait(&count);  
    pthread_mutex_lock(&mutex);
    buffer->ring_array[buffer->start & (BUFFER_SIZE - 1)] = 0;
    buffer->start++;
    sem_post(&spaces);
   
    pthread_mutex_unlock(&mutex);
}

void print_buffer(RingBuffer * buffer) {
    for(int i = 0; i < BUFFER_SIZE; i++) {
        printf("%i", buffer->ring_array[i]);
        printf("\n");
    }
} 



#endif

