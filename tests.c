// DO NOT MODIFY THIS FILE

#ifndef __SCHED__TESTS__
#define __SCHED__TESTS__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <semaphore.h>
#include <pthread.h>
#include <math.h>

#include "scheduler.h"
#include "sched_threads.h"
#include "list.h"

int testValue;
int semValue;
int QUEUE_SIZE;
int TIME_SLICE;

// FILE for processes
FILE *completedProcesses;

// Function prototypes
void *test_wait_for_queue(void *arg);
void *process_function(void *arg);
void *short_term_scheduler(void *arg);
void *long_term_scheduler(void *arg);

// Test prototypes
void init_queue_test(sched_queue_t *queue, char *argv[]);
void signal_process_test(sched_queue_t *queue, char *argv[]);
void wait_for_process_test(sched_queue_t *queue, char *argv[]);
void next_process_test(sched_queue_t *queue, char *argv[]);
void wait_for_queue_test(sched_queue_t *queue, char *argv[]);
void init_process_test(sched_queue_t *queue);
void destroy_process_test();
void terminate_process_test(sched_queue_t *queue);
void return_to_sched_queue_test(sched_queue_t *queue, list_elem_t *elt);

int tests(sched_queue_t *queue, char *argv[]){
    int test_sched, test_type = 0;

     // check if algorithm is valid
    if(!strcmp(argv[2], "-fifo")){
	    queue->ops = dispatch_fifo;
        test_sched = 0;
    }
    else if(!strcmp(argv[2], "-rr")){
        queue->ops = dispatch_rr;
        test_sched = 1;
    }
    else{
    	fprintf(stderr, "Must provide scheduling algorithm -fifo or -rr\n");
        return EXIT_FAILURE;
    }
    // get queue size from argv
    if(atoi(argv[3])){
        QUEUE_SIZE = atoi(argv[3]);
    }
    else{
        fprintf(stderr, "Must provide a whole number as the QUEUE_SIZE\n");
        return EXIT_FAILURE;
    }
    // get time_slice from argv (only for round robin)
    if(test_sched && atof(argv[4]) > 0){
        TIME_SLICE = atof(argv[4]);
        queue->ops.sched_ops.init_sched_queue(queue, QUEUE_SIZE, TIME_SLICE);
    }
    else if(test_sched){
        fprintf(stderr, "Must provide a positive float number as the TIME_SLICE\n");
        return EXIT_FAILURE;
    }
    else{
        TIME_SLICE = 1;
        queue->ops.sched_ops.init_sched_queue(queue, QUEUE_SIZE, TIME_SLICE);
    }

    int test_name_loc = test_sched ? 5 : 4;
    if(argv[test_name_loc] != NULL){
        if(!strcmp(argv[test_name_loc], "init_queue")) test_type = 1;
        if(!strcmp(argv[test_name_loc], "init_process")) test_type = 2;
        if(!strcmp(argv[test_name_loc], "terminate_process")) test_type = 3;
        if(!strcmp(argv[test_name_loc], "signal_process")) test_type = 4;
        if(!strcmp(argv[test_name_loc], "wait_for_process")) test_type = 5;
        if(!strcmp(argv[test_name_loc], "wait_for_queue")) test_type = 6;
        if(!strcmp(argv[test_name_loc], "next_process")) test_type = 7;
    }

    
    if(test_type == 1 || !test_type)
        // Tests inital queue values
        init_queue_test(queue, argv);       // 5 pts

    if(test_type == 2 || !test_type)
        // Tests initial process values
        init_process_test(queue);           // 5 pts

    if(test_type == 3 || !test_type)
        // Tests the correct termination of the process
        terminate_process_test(queue);      // 5 pts
    if(test_type == 4 || !test_type) 
        // Test for signal_process 
        signal_process_test(queue, argv);   // 5 pts

    if(test_type == 5 || !test_type)
        // Test for wait_for_process
        wait_for_process_test(queue, argv); // 5 pts

    if(test_type == 6 || !test_type)
        // Test for wait_for_queue
        wait_for_queue_test(queue, argv);   // 5 pts

    if(test_type == 7 || !test_type)
        // Test for next_process (depends on next_process and process_function)
        next_process_test(queue, argv);   // 20 pts

    if(!test_type)
        // If all passes this will print
        fprintf(stdout, "Congratulations! you passed all my tests, this doesn't mean the program is 100 percent correct but it means you are doing good\nyou need to manually check if the long_term_scheduler is adding processes to the queue when they arrive, waiting for processes to arrive, and terminating properly at the end of the file\nalso, the long_term_scheduler and short_term_scheduler functions are accessing the queue mutually excluded\n\n");

    // Untested parts of the project, this will be testing with the graph
    // long_term_scheduler 
    // short_term_scheduler 
    return EXIT_SUCCESS;
}

void *test_wait_for_queue(void *arg){
	sched_queue_t *queue = (sched_queue_t*)arg;
	sleep(5);
	testValue = 1;
	sem_post(&queue->ready_sem);	
	pthread_exit(0);
}


void init_queue_test(sched_queue_t *queue, char *argv[]){
    // list test
    assert(&queue->lst != NULL && list_size(&queue->lst) == 0 && "Linked List cannot be NULL and must start empty"); // is the brand new list empty?

    // queue cpu_sem test
    sem_getvalue(&queue->cpu_sem, &semValue);
    #ifdef DEBUG
        printf("Initial cpu_sem value %d", semValue);
    #endif
    assert(semValue == 1 && "Schedulers should have access to CPU"); // is the CPU sem "free"?

    // ready_sem test
    sem_getvalue(&queue->ready_sem, &semValue);
    assert(semValue == 0 && "There are no processes in the queue initially"); // is the ready_sem representing the amount of ready processes in the queue?

    // sched_queue_sem test
    sem_getvalue(&queue->sched_queue_sem, &semValue);
    assert(semValue == QUEUE_SIZE && "There should be QUEUE_SIZE spaces available in the LL"); // is the sched_queue_sem representing the max size of the queue?

    // mutex test
    assert(pthread_mutex_lock(&queue->lock) == 0 && "No one is using the queue yet, it should be unlocked"); // is the queue "free" initially?
    pthread_mutex_unlock(&queue->lock);

    // time_slice test
    if(!strcmp(argv[2], "-rr"))
        assert(queue->time_slice == TIME_SLICE && "The queue assigned time_slice should be given by the user"); // is the time_slice of the scheduler set equal to what was given? (check Makefile)

    // global time starts at 0
    assert(queue->global_time == 0);

    fprintf(stdout, "Init queue \t\t*pass*\n");
}

void signal_process_test(sched_queue_t *queue, char *argv[]){
    process_t *testprocess = (process_t*) malloc(sizeof(process_t));
    sem_init(&testprocess->cpu_sem, 0, 0);
    queue->ops.sched_ops.signal_process(testprocess);
    sem_getvalue(&testprocess->cpu_sem, &semValue);
    assert(semValue == 1 && "The scheduler should signal the process cpu semaphore"); // is the process receiving the signal from the queue in its own cpu_sem? 
    fprintf(stdout, "Signal process \t\t*pass*\n");
}


void wait_for_process_test(sched_queue_t *queue, char *argv[]){
    queue->ops.sched_ops.wait_for_process(queue);
    sem_getvalue(&queue->cpu_sem, &semValue);
    assert(semValue == 0 && "The scheduler should wait on its own cpu semaphore"); // is the queue waiting on the process in the queue's cpu_sem?
    sem_post(&queue->cpu_sem);
    fprintf(stdout, "Wait for process \t*pass*\n");
}


void wait_for_queue_test(sched_queue_t *queue, char *argv[]){
    testValue = 0;
    pthread_t wait_for_queue_thread;
    pthread_create(&wait_for_queue_thread, 0, test_wait_for_queue, (void*)queue);
    queue->ops.sched_ops.wait_for_queue(queue);
    assert(testValue == 1 && "The scheduler needs to wait for a single process to enter the queue if this is failing your scheduler does not actually stop for a process"); // is the scheduler waiting on the ready_sem?
    sem_getvalue(&queue->ready_sem, &semValue);
    assert(semValue == 1 && "The scheduler should wait, but not modify permanently the value of the number of processes in the queue"); // is the ready_sem representing the actual number of ready processes in the queue at all times?
    sem_wait(&queue->ready_sem);
    fprintf(stdout, "Wait for queue \t\t*pass*\n");
}

void next_process_test(sched_queue_t *queue, char *argv[]){
    pthread_t *test_threads = (pthread_t *) malloc(sizeof(pthread_t) * QUEUE_SIZE);
    completedProcesses = fopen("completedProcesses.txt", "w");
    process_t *new_process[100];
    // Populate the queue with dummy processes
    for (int i = 0; i < QUEUE_SIZE; i++){
        new_process[i] = (process_t*) malloc(sizeof(process_t));
        new_process[i]->serviceTime = TIME_SLICE*3;
        new_process[i]->pid = i;
        fprintf(stdout, "Created process %d with service time %f\n", new_process[i]->pid, new_process[i]->serviceTime);
        queue->ops.process_ops.init_process_info(new_process[i], queue);
        list_elem_init(new_process[i]->PCB, (void *) new_process[i]);
        int rc = pthread_create(&test_threads[i], NULL, process_function, (void *)new_process[i]->PCB);
        if(rc || new_process[i]->pid < 0){
            fprintf(stderr, "Failed to create thread, restarting tests\n");
            fflush(stdout);
            execv(*argv, argv);
            return;
        }
        pthread_detach(test_threads[i]); // detach to let it run independently
    }
    
    // Wait for queue to be full
    do{
        sleep(1);
        sem_getvalue(&queue->sched_queue_sem, &semValue);
        printf("Waiting for queue to get all %d processes\n", QUEUE_SIZE);
    }while(semValue != 0);

    // Since queue is full ready_sem should be QUEUE_SIZE
    sem_getvalue(&queue->ready_sem, &semValue);
    assert(semValue == QUEUE_SIZE);

    // The tests below will check the process_function and the next_process functions, and whether they are able to run and terminate correctly in either fifo or rr.
    if(!strcmp(argv[2], "-fifo")){
        process_t *info;
        // loop to "schedule" processes like FIFO would do
        for (int i = 0; i < QUEUE_SIZE; i++){
                queue->ops.sched_ops.wait_for_cpu(queue);
                info = queue->ops.sched_ops.next_process(queue);
                fprintf(stdout, "Start execution of process %d\n", info->pid);
                queue->ops.sched_ops.signal_process(info);
                queue->ops.sched_ops.wait_for_process(queue);
                queue->ops.sched_ops.release_cpu(queue);
                assert(list_size(&queue->lst) == QUEUE_SIZE-1-i && "Are you getting the front of the queue? Are you removing the node when you do?"); // after scheduling 1 process, there is 1 less process in the queue
        }
	fprintf(stdout, "Process_function and next_process_fifo \t\t*pass*\n");
    }
    else if(!strcmp(argv[2], "-rr")){
        process_t *info;
        // loop to "schedule" processes like RR would do.
        for(int j = 0; j < 3; j++){
                assert(list_size(&queue->lst) == QUEUE_SIZE && "Are you returning the processes at the end of the queue after the timeslice?");
                for (int i = 0; i < QUEUE_SIZE; i++){
                        queue->ops.sched_ops.wait_for_cpu(queue);
                        info = queue->ops.sched_ops.next_process(queue);
                        fprintf(stdout, "Start execution of process %d\n", info->pid);
                        queue->ops.sched_ops.signal_process(info);
                        queue->ops.sched_ops.wait_for_process(queue);
                        queue->ops.sched_ops.release_cpu(queue);
                }
        }
        assert(list_size(&queue->lst) == 0 && "The list should be empty after all processes terminated"); // after scheduling all processes 3 times each (predetermined), the queue should be empty.
	    fprintf(stdout, "Process_function and next_process_rr \t\t*pass*\n");
    }
}

void init_process_test(sched_queue_t *queue){
    process_t *myProcess = (process_t*)malloc(sizeof(process_t));
    queue->ops.process_ops.init_process_info(myProcess, queue);

     // process cpu_sem test
    sem_getvalue(&myProcess->cpu_sem, &semValue);
    assert(semValue == 0 && "Processes must wait for the short_term_schedulers to call on them before they can access the CPU.");

    // process queue test
    assert(myProcess->queue == queue && "The process must have a pointer to the queue it belongs to.");

    // process PCB
    assert(myProcess->PCB != NULL && "The process new PCB is NULL");

    // process list element
    assert(myProcess->PCB->context == myProcess && "The PCB context holds the reference to the process itself");

    fprintf(stdout, "Init process \t\t*pass*\n");
}

void terminate_process_test(sched_queue_t *queue){
    //QUEUE_SIZE
    int QUEUE_SIZE;
    sem_getvalue(&queue->sched_queue_sem, &QUEUE_SIZE);

    // Create test process with test data
    FILE *testTerminate = fopen("test.out", "w");
    process_t *myProcess = (process_t*)malloc(sizeof(process_t));
    queue->ops.process_ops.init_process_info(myProcess, queue);
    myProcess->arrivalTime = 10;
    myProcess->serviceTime = 20;
    myProcess->pid = 1000;
    // "Add" process to queue
    sem_wait(&myProcess->queue->sched_queue_sem);
    sem_post(&myProcess->queue->ready_sem);
    queue->ops.process_ops.terminate_process(myProcess, testTerminate);
    fclose(testTerminate);

    // Check correct values: 
    testTerminate = fopen("test.out", "r");
    char *line = (char*)malloc(100);
    fgets(line, 100, testTerminate);

    assert(!strcmp(line,"1000 10 20.000000 0.000000\n") && "Save your process data in the following format 'processID arrivalTime serviceTime completionTime'");
    sem_getvalue(&myProcess->queue->ready_sem, &semValue);
    assert(semValue == 0 && "Number of ready processes decrease when a process terminates");
    sem_getvalue(&myProcess->queue->sched_queue_sem, &semValue);
    assert(semValue == QUEUE_SIZE && "Number of available locations in the queue increases when a process terminates");
    fclose(testTerminate);
    remove("test.out");

    fprintf(stdout, "Terminate process \t*pass*\n");
}


#endif
