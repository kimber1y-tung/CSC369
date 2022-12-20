// ------------
// This code is provided solely for the personal and private use of
// students taking the CSC369H5 course at the University of Toronto.
// Copying for purposes other than this use is expressly prohibited.
// All forms of distribution of this code, whether as given or with
// any changes, are expressly prohibited.
//
// Authors: Bogdan Simion
//
// All of the files in this directory and all subdirectories are:
// Copyright (c) 2019 Bogdan Simion
// -------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "executor.h"

extern struct executor tassadar;


/**
 * Populate the job lists by parsing a file where each line has
 * the following structure:
 *
 * <id> <type> <num_resources> <resource_id_0> <resource_id_1> ...
 *
 * Each job is added to the queue that corresponds with its job type.
 */
void parse_jobs(char *file_name) {
    int id;
    struct job *cur_job;
    struct admission_queue *cur_queue;
    enum job_type jtype;
    int num_resources, i;
    FILE *f = fopen(file_name, "r");

    /* parse file */
    while (fscanf(f, "%d %d %d", &id, (int*) &jtype, (int*) &num_resources) == 3) {

        /* construct job */
        cur_job = malloc(sizeof(struct job));
        cur_job->id = id;
        cur_job->type = jtype;
        cur_job->num_resources = num_resources;
        cur_job->resources = malloc(num_resources * sizeof(int));

        int resource_id; 
				for(i = 0; i < num_resources; i++) {
				    fscanf(f, "%d ", &resource_id);
				    cur_job->resources[i] = resource_id;
				    tassadar.resource_utilization_check[resource_id]++;
				}
				
				assign_processor(cur_job);

        /* append new job to head of corresponding list */
        cur_queue = &tassadar.admission_queues[jtype];
        cur_job->next = cur_queue->pending_jobs;
        cur_queue->pending_jobs = cur_job;
        cur_queue->pending_admission++;
    }

    fclose(f);
}

/*
 * Magic algorithm to assign a processor to a job.
 */
void assign_processor(struct job* job) {
    int i, proc = job->resources[0];
    for(i = 1; i < job->num_resources; i++) {
        if(proc < job->resources[i]) {
            proc = job->resources[i];
        }
    }
    job->processor = proc % NUM_PROCESSORS;
}


void do_stuff(struct job *job) {
    /* Job prints its id, its type, and its assigned processor */
    printf("%d %d %d\n", job->id, job->type, job->processor);
}



/**
 * TODO: Fill in this function
 *
 * Do all of the work required to prepare the executor
 * before any jobs start coming
 *
 * #define NUM_QUEUES 4
 * #define QUEUE_LENGTH 10
 * #define NUM_PROCESSORS 4
 * #define NUM_RESOURCES 8
 */
void init_executor() {

    // job executor: resource_locks
    for (int i = 0; i < NUM_RESOURCES; i++){
        pthread_mutex_init(&tassadar.resource_locks[i], NULL);
    }

    // admission queues: lock, admission_cv, execution_cv
    for (int j = 0; j < NUM_QUEUES; j++){
        pthread_mutex_init(&tassadar.admission_queues[j].lock, NULL);
        pthread_cond_init(&tassadar.admission_queues[j].admission_cv, NULL);
        pthread_cond_init(&tassadar.admission_queues[j].execution_cv, NULL);

        /* list of jobs that are pending admission into this queue */
        tassadar.admission_queues[j].pending_jobs = malloc(sizeof(struct job) * QUEUE_LENGTH);

        /* number of jobs pending admission into this queue; */
        tassadar.admission_queues[j].pending_admission = 0;

        /* circular list implementation storing the jobs that are admitted */
        tassadar.admission_queues[j].admitted_jobs = malloc(sizeof(struct job) * QUEUE_LENGTH);

        /* maximum number of elements in the admitted_jobs list */
        tassadar.admission_queues[j].capacity = QUEUE_LENGTH;

        /* number of elements currently in the admitted_jobs list */
        tassadar.admission_queues[j].num_admitted = 0;

        /* index of the first element in the admitted_jobs list */
        tassadar.admission_queues[j].head = 0;

        /* index of the last element in the admitted_jobs list */
        tassadar.admission_queues[j].tail = 0;
    }

    // processor records
    for (int k = 0; k < NUM_PROCESSORS; k++){
        pthread_mutex_init(&tassadar.processor_records[k].lock, NULL);

        /* number of jobs which have been completed on this processor. */
        tassadar.processor_records[k].num_completed = 0;
    }


}


/**
 * TODO: Fill in this function
 *
 * Handles an admission queue passed in through the arg (see the executor.c file). 
 * Bring jobs into this admission queue as room becomes available in it. 
 * As new jobs are added to this admission queue (and are therefore ready to be taken
 * for execution), the corresponding execute thread must become aware of this.
 * 
 */
void *admit_jobs(void *arg) {
    struct admission_queue *q = arg;

    // This is just so that the code compiles without warnings
    // Remove this line once you implement this function
    //q = q;

    //acquire lock
    pthread_mutex_lock(&q->lock);

    // loop to bring jobs in pending_admission to admitted jobs
    while (q->pending_jobs != NULL && q->pending_admission > 0) {

        //if the queue is at capacity, we need to wait for room
        while (q->num_admitted == q->capacity) {
            pthread_cond_wait(&q->admission_cv, &q->lock);
        }

        // admit the first job in pending_jobs and add it to the end of admitted jobs
        q->admitted_jobs[q->tail] = q->pending_jobs;

        // update numbers
        q->num_admitted++;
        q->pending_admission--;
        q->tail = (q->tail + 1) % q->capacity;

        // update the head of the circular list to the next element
        q->pending_jobs = q->pending_jobs->next;

        // Signals execution thread that a job is ready to be removed from the queue
        pthread_cond_signal(&q->execution_cv);

    }

    // release lock
    pthread_mutex_unlock(&q->lock);
    
    return NULL;
}


/**
 * TODO: Fill in this function
 *
 * Moves jobs from a single admission queue of the executor. 
 * Jobs must acquire the required resource locks before being able to execute. 
 *
 * Note: You do not need to spawn any new threads in here to simulate the processors.
 * When a job acquires all its required resources, it will execute do_stuff.
 * When do_stuff is finished, the job is considered to have completed.
 *
 * Once a job has completed, the admission thread must be notified since room
 * just became available in the queue. Be careful to record the job's completion
 * on its assigned processor and keep track of resources utilized. 
 *
 * Note: No printf statements are allowed in your final jobs.c code, 
 * other than the one from do_stuff!
 */
void *execute_jobs(void *arg) {
    struct admission_queue *q = arg;

    // This is just so that the code compiles without warnings
    // Remove this line once you implement this function
    //q = q;

    //Acquire admission queue lock
    pthread_mutex_lock(&q->lock);

    // Loops until no jobs left in pending jobs or admission queue
    while(!((q->pending_admission == 0) && (q->num_admitted == 0))){
        //Wait until job is ready to be popped from queue
        while (q->num_admitted == 0) {
            pthread_cond_wait(&q->execution_cv, &q->lock);
        }
        
        //Pop job at top of list
        struct job* new_job = q->admitted_jobs[q->head];
        
        // Job removed from queue, so no longer needs linked list functionality
        new_job->next = NULL;
        q->num_admitted--;
        //if head pointer reaches end of list, loops back to start
        q->head = (q->head + 1) % q->capacity;

        //Signal admission thread that there is now an available spot in the queue
        pthread_cond_signal(&q->admission_cv);

        //release admission queue lock
        pthread_mutex_unlock(&q->lock);
        
        // acquire all needed resource locks
        for(int j=0; j < new_job->num_resources; j++){
            pthread_mutex_lock(&tassadar.resource_locks[new_job->resources[j]]);
            //Decrement the resource utilization check
            tassadar.resource_utilization_check[new_job->resources[j]]--;
        }

        do_stuff(new_job);

        // release all needed resource locks
        for(int k=0; k < new_job->num_resources; k++){
            pthread_mutex_unlock(&tassadar.resource_locks[new_job->resources[k]]);
        }

        //Acquire processor record lock
        pthread_mutex_lock(&tassadar.processor_records[new_job->processor].lock);
        
        //Shifts completed jobs list down by one, and places newest job at the front
        struct job* temp = tassadar.processor_records[new_job->processor].completed_jobs;
        tassadar.processor_records[new_job->processor].completed_jobs = new_job;
        new_job->next = temp;

        //Increment number of completed jobs in processor record
        tassadar.processor_records[new_job->processor].num_completed++;
        
        //Release processor record lock
        pthread_mutex_unlock(&tassadar.processor_records[new_job->processor].lock);

        // Acquire admission queue lock to check loop condition
        pthread_mutex_lock(&q->lock);
    }
    pthread_mutex_unlock(&q->lock);
    
    return NULL;
}
