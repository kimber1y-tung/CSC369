#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

linked_list_t *stack;

/* The page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	//pops frame off top of stack
	return stack->top->frame;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {

	int frame = p->frame>>PAGE_SHIFT;

	//If theres a hit, pushes frame to bottom of the stack
	if(coremap[frame].lru_pos){
		//Removes frame from stack (updating adjacent pointers to point to each other)
		ll_entry_t *cur_e = coremap[frame].lru_pos;
		cur_e->prev->next = cur_e->next;
		cur_e->next->prev = cur_e->prev;

		//edge case fix if top entry is hit
		if(stack->top == cur_e){
			stack->top = cur_e->next;
		}
		if(stack->bot == cur_e){
			stack->bot = cur_e->next;
		}

		//Adds frame to bottom of the stack (updating adjacent pointers to point to the frame)
		stack->bot->prev->next = cur_e;
		cur_e->prev = stack->bot->prev;
		cur_e->next = stack->bot;
		stack->bot->prev = cur_e;
	}
	//If there is no hit, but theres space on the stack: add frame to bottom of stack
	else if (stack->size < memsize){
		stack->bot->frame = frame;
		//coremap points to position in stack
		coremap[frame].lru_pos = stack->bot;
		stack->bot = stack->bot->next;
		stack->size++;
	}
	//If there is no hit, and no space on the stack: each entry in stack moves up one, and frame gets placed at the bottom
	else{
		//Shifts top and bottom pointers to move each entry up one on stack
		stack->top = stack->top->next;
		stack->bot = stack->bot->next;
		//frame placed at bottom
		coremap[frame].lru_pos = stack->bot;
	}

	return;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
	stack = malloc(sizeof(linked_list_t));
	stack->top = malloc(sizeof(ll_entry_t));
	stack->bot = stack->top;
	stack->size = 0;

	ll_entry_t *e;
	ll_entry_t *prev_e = stack->top;
	//Creates linked list of size memsize, with all adjacent entries pointing to each other
	int i;
	for(i=0;i<memsize-1;i++){
		e = malloc(sizeof(ll_entry_t));
		prev_e->next = e;
		e->prev = prev_e;
		e->frame = -1;
		prev_e = e;
	}
	e->next = stack->top;
	stack->top->prev = e;
	stack->top->frame = -1;

}
