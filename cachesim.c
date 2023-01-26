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
		c->i = 0;
		c->current_state_consumed_cycles = 0;
		c->substate_consumed_cycles = 0;
		c->state = CACHE_STATE_POPULATE;
		goto DO_POPULATE;
	}

	if(c->state == CACHE_STATE_POPULATE && c->state == CACHE_STATE_IDLE){
		DO_POPULATE:;
		master_cyclecounter += c->cache_time_populate_step;
		if(c->i > c->ncachelines){
			c->state = CACHE_STATE_IDLE;
			c->substate = CACHE_STATE_IDLE;
			return c->state;
		}
		memory_request_retval = c->mread(c->i,c->cachelines + c->i * c->cacheline_sz, &c->active_memory_read_request_id);
		if(memory_request_retval == MEM_REQUEST_HANDLED)
			{
				c->i++;
			}/*Wow! Proceed immediately to the next request.*/
		/*We must repeat the request on the next clock tick.*/
		if(memory_request_retval == MEM_QUEUE_FULL){
			return c->state;
		}
		/*request was queud, we must continually check if the request has been filled every tick.*/
		if(memory_request_retval == MEM_REQUEST_QUEUED){
			c->substate = CACHE_STATE_WAITING_ON_READ;
			return c->state;
		}
		return c->state;
	}


	if(c->state == CACHE_STATE_POPULATE && c->substate == CACHE_STATE_WAITING_ON_READ){
		uint32_t r;
		r = c->mstat_read(c->active_memory_read_request_id);
		if(r == MEM_REQUEST_QUEUED) return;
		
		return c->state;
	}
}
