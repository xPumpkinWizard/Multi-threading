// TODO Your names: 

#ifndef __SCHEDULER__C__
#define __SCHEDULER__C__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "scheduler.h"
#include "sched_threads.h"

void init_process_info(process_t *info, sched_queue_t *queue){
    // TODO initiliaze the process:
    // Add the reference to the queue in the process info
    // Initialize the PCB with some space
    // Initialize the process PCB with a link to itself
    // Initialize the process' CPU semaphore so it must wait for the short_term_scheduler before it can get the CPU
    info->queue= queue;
    info->PCB = (list_elem_t *)malloc(sizeof(list_elem_t));
    info->PCB->context = info;
    sem_init(&info->cpu_sem, 0, 0);
}

void destroy_process_info(process_t *info){
    // TODO destrou the process
    // Destroy the context
    // Destoy the semaphore
    free(info->PCB);
    sem_destroy(&info->cpu_sem);
    free(info);
} 

void wait_for_cpu_user(process_t *info){
    // TODO Wait for the scheduler to signal this process
    sem_wait(&info->cpu_sem);
}

void wait_for_cpu_kernel(sched_queue_t *queue){
    // TODO Wait for the scheduler to signal this process
    sem_wait(&queue->cpu_sem);
}

void release_cpu(sched_queue_t *queue){
    // TODO Let the scheduler know the process is done using the cpu
    sem_post(&queue->cpu_sem);
}

void return_to_queue(sched_queue_t *queue, list_elem_t *elt){
    // TODO Add the element to the back of the list with mutual exclusion
    pthread_mutex_lock(&queue->lock);
        list_insert_tail(&queue->lst, elt);
    pthread_mutex_unlock(&queue->lock);
}

void enter_sched_queue(sched_queue_t *queue, list_elem_t *elt){
    // TODO Enter the queue for the first time: 
    // check if the queue is not full, 
    // modify the queue with mutual exclusion, 
    // and signal the queue has one more ready process
    sem_wait(&queue->sched_queue_sem);
        pthread_mutex_lock(&queue->lock);
            list_insert_tail(&queue->lst, elt);
        pthread_mutex_unlock(&queue->lock);
    sem_post(&queue->ready_sem);
}

void terminate_process(process_t *info, FILE *file){
    // TODO Write the final values about the process
    // record the info as: processID arrivalTime serviceTime completionTime
    // update the values of the ready_sem and sched_queue_sem to represent one less ready process in the queue and one more empty space in the queue. 
    info->completionTime = info->queue->global_time;
    sem_post(&(info->queue->sched_queue_sem));
    fprintf(file, "%d %d %f %f\n", (info->pid), (info->arrivalTime), (info->serviceTime), (info->completionTime)); 
    sem_wait(&(info->queue->ready_sem));
}

void init_sched_queue(sched_queue_t *queue, int queue_size, float time_slice)
{
	// TODO initialize the scheduler's semaphores, mutex, and queue
    sem_init(&(queue->sched_queue_sem), 0, queue_size);
    sem_init(&(queue->ready_sem), 0, 0);
    sem_init(&(queue->cpu_sem), 0, 1);
    queue->time_slice = time_slice;
    list_init(&(queue->lst));
    pthread_mutex_init(&(queue->lock), NULL);
}

void destroy_sched_queue(sched_queue_t *queue)
{
    // TODO destroy scheduler's semaphores, mutex, and queue
    pthread_mutex_destroy(&(queue->lock));
    sem_destroy(&(queue->cpu_sem));
    sem_destroy(&(queue->ready_sem));
    sem_destroy(&(queue->sched_queue_sem));
}

void signal_process(process_t *info)
{
    // TODO signal the process that the CPU is free
    printf("Signal process %d\n", info->pid);
    sem_post(&(info->cpu_sem));
}

void wait_for_process(sched_queue_t *queue)
{
    // TODO make the dispatcher wait until CPU is available
    printf("Waiting for process\n");
    sem_wait(&(queue->cpu_sem));
}

void wait_for_queue(sched_queue_t *queue)
{
    // TODO make the queue wait until there are ready processes in the queue. 
    // HINT: Simpler is better. You have a semaphore that tells you if there are processes ready to run.
    // Note: Do not make a spin-wait function here
    sem_wait(&(queue->ready_sem));
    sem_post(&(queue->ready_sem));
}

process_t *next_process_fifo(sched_queue_t *queue)
{
    // TODO get front item in a FIFO scheduler
	process_t *info = NULL;
	list_elem_t *elt = NULL;

    pthread_mutex_lock(&(queue->lock));
    // get the front element of the queue
	elt = list_get_head(&(queue->lst));
    // if the element is not NULL remove the element and retrieve the process data
    // ONLY IN FIFO update the queue time_slice to allow the process to run for its entire serviceTime
	if (elt != NULL) {
        list_remove_elem(&(queue->lst), elt);
        info = elt->context;
        queue->time_slice = info->serviceTime;	
        printf("%f\n", queue->time_slice);
	}
    pthread_mutex_unlock(&(queue->lock));
    // return the process info, if NULL the queue was empty
	return info;
}

process_t *next_process_rr(sched_queue_t *queue)
{
    // TODO get front item in a RR scheduler
	process_t *info = NULL;
	list_elem_t *elt = NULL;

    pthread_mutex_lock(&(queue->lock));
    // get the front element of the queue
	elt = (queue->lst.head);
    // if the element is not NULL remove the element and retrieve the process data
	if (elt != NULL) {
        list_remove_elem(&(queue->lst), elt);
        info = elt->context;
	}
    pthread_mutex_unlock(&(queue->lock));
    // return the process info, if NULL the queue was empty
	return info;
}

// Dispatcher operations, this is how the scheduler knows what functions will be called when the names on scheduler_t are used in sched_threads.c
scheduler_t dispatch_fifo = {
        {init_process_info, destroy_process_info, wait_for_cpu_user, enter_sched_queue, return_to_queue, terminate_process},
        {init_sched_queue, destroy_sched_queue, signal_process, wait_for_process, next_process_fifo, wait_for_queue, wait_for_cpu_kernel, release_cpu } 
    };
//TODO Remove comment and finish dispatch_rr scheduler definition
scheduler_t dispatch_rr = {
        {init_process_info, destroy_process_info, wait_for_cpu_user, enter_sched_queue, return_to_queue, terminate_process},
        {init_sched_queue, destroy_sched_queue, signal_process, wait_for_process, next_process_rr, wait_for_queue, wait_for_cpu_kernel, release_cpu }
    };

#endif 