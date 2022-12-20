#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int clock_index;

/* The page to evict is chosen using the Clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {

    // loop through coremap
    while (1){

        // check if the ref bit is off, if not we need to turn it off
        // set ref bit from 1 -> 0
        if (coremap[clock_index].pte->frame & PG_REF){
            coremap[clock_index].pte->frame = coremap[clock_index].pte->frame & (~PG_REF);
        }
        else{
            return clock_index; // return the index we found with ref bit 0
        }
        clock_index = (clock_index + 1) % memsize;
    }
}

/* This function is called on every access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {

    p->frame |= PG_REF; // set ref bit to 1
}

/* Initialize any data structures needed for this replacement
 * algorithm.
 */
void clock_init() {
    clock_index = 0;
}
