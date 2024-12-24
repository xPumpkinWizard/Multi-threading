/*DO NOT MODIFY THIS FILE*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "scheduler.h"
#include "sched_threads.h"

void tests(sched_queue_t *queue, char *argv[]);

int main(int argc, char *argv[]){

    // Checking argument count is correct
    if(argc < 1){
        fprintf(stderr, "Usage ./scheduler -schedAlgorithm QUEUE_SIZE TIME_SLICE(required only in -rr), i.e. ./scheduler -rr 5 10\n");
        return EXIT_FAILURE;
    }
    else if(!strcmp(argv[1], "-rr") && argc < 3){
        fprintf(stderr, "Usage ./scheduler -schedAlgorithm QUEUE_SIZE TIME_SLICE(required only in -rr), i.e. ./scheduler -rr 5 10\n");
        return EXIT_FAILURE;
    }

    // Some variables to collect the program execution call
    int QUEUE_SIZE = 0;
    float TIME_SLICE = 0;

    // the short_term_scheduler thread schedules the execution of processes on the CPU
    pthread_t short_term_scheduler_thread;
    // the long_term_scheduler thread reads the file and create processes to add into the scheduler queue
    pthread_t long_term_scheduler_thread;

    // initialize the scheduler_queue structure
    sched_queue_t *queue = (sched_queue_t*) malloc(sizeof(sched_queue_t));
   
    // test runner, add test after either -fifo or -rr, i.e. ./scheduler -fifo test
    if(argc >= 4 && !strcmp(argv[1], "-test")){
        tests(queue, argv);
        return EXIT_SUCCESS;
    }

    // add operations and initialize queue's variables
    // check if algorithm is valid
    if(!strcmp(argv[1], "-fifo")){
	    queue->ops = dispatch_fifo;
    }
    else if(!strcmp(argv[1], "-rr")){
        queue->ops = dispatch_rr;
    }
    else{
    	fprintf(stderr, "Must provide scheduling algorithm -fifo or -rr\n");
        return EXIT_FAILURE;
    }
    // get queue size from argv
    if(atoi(argv[2])){
        QUEUE_SIZE = atoi(argv[2]);
    }
    else{
        fprintf(stderr, "Must provide a whole number as the QUEUE_SIZE\n");
        return EXIT_FAILURE;
    }
    // get time_slice from argv (only for round robin)
    if(!strcmp(argv[1], "-rr") && atof(argv[3]) > 0){
        TIME_SLICE = atof(argv[3]);
    }
    else if(!strcmp(argv[1], "-rr")){
        fprintf(stderr, "Must provide a positive float number as the TIME_SLICE\n");
        return EXIT_FAILURE;
    }
    
    // Create the queue
    queue->ops.sched_ops.init_sched_queue(queue, QUEUE_SIZE, TIME_SLICE);

    // create threads and assign their correponding function
    if(pthread_create(&long_term_scheduler_thread, NULL, long_term_scheduler, (void*)queue)){
        return EXIT_FAILURE;
    }
    if(pthread_create(&short_term_scheduler_thread, NULL, short_term_scheduler, (void*)queue)){
        return EXIT_FAILURE;
    }

    // detach threads so they can execute freely
    pthread_detach(long_term_scheduler_thread);

    // wait until all processes finish their execution
    pthread_join(short_term_scheduler_thread, NULL);

    // destroy queue
    queue->ops.sched_ops.destroy_sched_queue(queue);

    return EXIT_SUCCESS;
}
