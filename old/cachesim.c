#include "cachesim.h"


uint32_t cache_statemachine(cachedef* c, uint64_t* master_cyclecounter){
	uint32_t memory_request_retval = 0;
	if(c->state == CACHE_STATE_ERROR) {
		master_cyclecounter[0] += c->cache_time_idle;
		return c->state;
	}
	if(c->state == CACHE_STATE_IDLE) {
		master_cyclecounter[0] += c->cache_time_idle;
		return c->state;
	}

	if(c->state == CACHE_STATE_POPULATE_START){
		master_cyclecounter[0] += c->cache_time_populate_start;
		c->i = 0;
		c->current_state_consumed_cycles = 0;
		c->substate_consumed_cycles = 0;
		c->state = CACHE_STATE_POPULATE;
		c->substate = CACHE_STATE_IDLE;
		goto DO_POPULATE;
	}

	if(c->state == CACHE_STATE_POPULATE && c->substate == CACHE_STATE_IDLE){
		DO_POPULATE:;
		master_cyclecounter += c->cache_time_populate_step;
		if(c->i > c->ncachelines){
			c->state = CACHE_STATE_IDLE;
			c->substate = CACHE_STATE_IDLE;
			return c->state;
		}
		memory_request_retval = c->mread(
			c->i,
			c->cachelines + c->i * c->cacheline_sz,
			&c->active_memory_read_request_id,
			master_cyclecounter
		);
		/*We must repeat the request on the next clock tick.*/
		if(memory_request_retval == MEM_QUEUE_FULL){
			return c->state;
		}
		if(memory_request_retval == MEM_REQUEST_HANDLED)
		{
			c->i++;
			return c->state;
		}/*Wow! Proceed immediately to the next request.*/

		/*request was queud, we must continually check if the request has been filled every tick.*/
		if(memory_request_retval == MEM_REQUEST_QUEUED){
			c->substate = CACHE_STATE_WAITING_ON_READ;
			return c->state;
		}
		puts("<CACHESIM ERROR> During cache population, invalid mread");
		exit(1);
	}


	if(c->state == CACHE_STATE_POPULATE && c->substate == CACHE_STATE_WAITING_ON_READ){
		uint32_t r;
		master_cyclecounter[0] += c->cache_time_wait_on_read;
		r = c->mstat_read(c->active_memory_read_request_id, master_cyclecounter);
		if(r == MEM_REQUEST_QUEUED) return c->state;
		if(r == MEM_REQUEST_HANDLED){
			c->i++;
			c->substate = CACHE_STATE_IDLE;
			return c->state;
		}
		if(r == MEM_QUEUE_FULL){
			puts("<CACHESIM ERROR> During cache population, memory said the queue was full in a read request.");
			return c->state;
		}
		puts("<CACHESIM ERROR> During cache population, invalid mstat_read");
		exit(1);
	}
}
