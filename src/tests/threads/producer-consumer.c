/* Tests producer/consumer communication with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
 
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include <string.h>

 
void producer_consumer(unsigned int num_producer, unsigned int num_consumer);
void producer(void *);
void consumer(void *);

struct lock mutex;
struct condition empty, fill;

int in = 0;
int out = 0;
// int prodCount = 0;
// int consCount = 0;

int count = 0;
int loops1 = 0;
int loops2 = 0;

#define size 15
char buffer[size];

char *input = "Hello world";
 
void test_producer_consumer(void)
{
    /*producer_consumer(0, 0);
    producer_consumer(1, 0);
    producer_consumer(0, 1);
    producer_consumer(1, 1);
    producer_consumer(3, 1);
    producer_consumer(1, 3);
    producer_consumer(4, 4);
    producer_consumer(7, 2);
    producer_consumer(2, 7);*/
    producer_consumer(2, 2);
    pass();
}
 
 
void producer_consumer(UNUSED unsigned int num_producer, UNUSED unsigned int num_consumer){
 
    lock_init(&mutex);
    cond_init(&empty);
    cond_init(&fill);
 
    for (unsigned int i = 0; i < num_producer; i++) {
        char name[] = "Producer";
        thread_create(name, PRI_DEFAULT, producer, NULL);
    }
 
    for (unsigned int j = 0; j < num_consumer; j++) {
        char name[] = "Consumer";
        thread_create(name, PRI_DEFAULT, consumer, NULL);
    }
 
    return;
}
 
void producer(void *arg) {

    lock_acquire(&mutex);
    if(loops1 < strlen(input)){
    while(count == size){
        cond_wait(&empty, &mutex);
    }
    buffer[in] = input[in];
    // printf("%c", buffer[in]);
    in = (in + 1) % size;
    count ++; 
    cond_signal(&fill, &mutex);
    lock_release(&mutex);
    }
    
    while(true){

        lock_acquire(&mutex);
        if(loops2 < strlen(input)){
            while(count == size){
                cond_wait(&empty, &mutex);
            }
            buffer[in] = input[in];
            // printf("%c", buffer[in]);
            in = (in + 1) % size;
            count++; 
            loops2++;
            cond_signal(&fill, &mutex);
            lock_release(&mutex); 
        }
        else {
            lock_release(&mutex);
            thread_exit();
        }   
    }
}
 
void consumer(void *arg) {

    while(true){

        lock_acquire(&mutex);
        if(loops1 < strlen(input)){
            while(count == 0){
                cond_wait(&fill, &mutex);
            }
            printf("%c", buffer[out]);
            out = (out + 1) % size;
            count--;
            loops1++;
            cond_signal(&fill, &mutex);
            lock_release(&mutex); 
        }
        else {
            lock_release(&mutex);
            thread_exit();
        }   
    }

}

