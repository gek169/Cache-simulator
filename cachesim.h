#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>


#define WRITE_IMMEDIATELY 0
#define WRITE_ON_EVICT 1


#ifndef CACHE_WRITE_MODE
#define CACHE_WRITE_MODE WRITE_ON_EVICT
#endif



typedef struct{
	uint32_t queue_entry_type;
	uint64_t addr; /*this can be a */
	uint64_t data;
}queue_entry;


typedef struct{
	uint64_t* true_addresses; /*For each cache line, keep a true address.*/
	uint8_t* cachelines; /*cached lines*/
	uint32_t (*mread)(uint64_t addr, uint8_t* data, uint32_t* rid_passthrough, uint64_t* master_cyclecounter); /*THIS IS A CACHELINE ADDRESS!!! NOT A BYTE ADDRESS!!!*/
	uint32_t (*mwrite)(uint64_t addr, uint8_t* data, uint32_t* rid_passthrough); /*THIS IS A CACHELINE ADDRESS!!! NOT A BYTE ADDRESS!!!*/
	uint32_t (*mstat_read)(uint16_t reqid); /*Check the status of a read request*/
	uint32_t (*mstat_write)(uint16_t reqid); /*Check the status of a write request*/
	/*CACHE QUEUE. Operations the cache must complete, in order.*/
	queue_entry todo_queue[65536]; 
	uint64_t nqueue_requests;
	uint64_t queue_size;
	/**/
	uint32_t active_memory_read_request_id;
	uint32_t active_memory_write_request_id;
	/*Timing information*/
	uint64_t cache_time_write; /*how many subcycles does the cache have to spend in order to write into one of its entries?*/
	uint64_t cache_time_read; /*how many subcycles does the cache have to spend in order to read one of its entries?*/
	uint64_t cache_time_decode; /*how many subcycles does the cache have to spend to decode an address into a cache line ID?*/
	uint64_t cache_time_idle; /*how many subcycles does it take to execute an idle?*/
	uint64_t cache_time_populate_step; /*how many subcycles does it take to do a populate step?*/
	uint64_t cache_time_
	/*Cache size information*/
	uint16_t ncachelines;
	uint32_t cacheline_sz;
	/*Cache state- this is a state machine, after all!*/
	uint16_t state; /*our larger action.*/
	uint16_t substate;
	uint64_t i; /*used for populating cache...*/
	uint64_t current_state_consumed_cycles;
	uint64_t substate_consumed_cycles;
	/*these are native-encoded, not necessarily BE*/
	uint8_t returned_byte;
	uint16_t returned_short;
	uint32_t returned_long;
	uint64_t returned_qword;
} cachedef;

enum{
	CACHE_STATE_IDLE=0,
	CACHE_STATE_POPULATE_START, /*Populating memory.*/
	CACHE_STATE_POPULATE, /*Populating memory.*/
	CACHE_STATE_WAITING_ON_READ,  /*waiting on a read request to be fulfilled.*/
	CACHE_STATE_WAITING_ON_WRITE, /*writing a */
	CACHE_STATE_FINISHED_READ_BYTE, /**/
	CACHE_STATE_FINISHED_READ_SHORT, /**/
	CACHE_STATE_FINISHED_READ_LONG, /**/
	CACHE_STATE_FINISHED_READ_QWORD, /**/
	CACHE_STATE_ERROR,
	CACHE_NSTATES
};


/*return value from cache requests*/
enum{
	CACHE_QUEUE_FULL=0, /*queue is full, cannot handle request. Must repeat request after a cycle.*/
	CACHE_REQUEST_QUEUED, /*request was queued.*/
	CACHE_REQUEST_HANDLED /*the request was immediately served.*/
};

/*return value from mread or write*/
enum{
	MEM_QUEUE_FULL = 0, /*memory request queue is full, cannot handle it*/
	MEM_REQUEST_QUEUED,
	MEM_REQUEST_HANDLED
};

static inline cachedef cache_init(
	uint8_t cacheline_sz_pow2,
	uint32_t ncachelines,
	uint64_t queue_size,
	uint32_t(*mread)(uint64_t addr, uint8_t* data, uint32_t* rid_passthrough), /*addr is addressed by cache line size.*/
	uint32_t(*mwrite)(uint64_t addr, uint8_t* data, uint32_t* rid_passthrough), /*Always writes cacheline_sz_pow2 bytes.*/
	uint32_t (*mstat_read)(uint16_t reqid), /*Check the status of a read request*/
	uint32_t (*mstat_write)(uint16_t reqid), /*Check the status of a write request*/
	uint64_t cache_time_write, /*how many subcycles does the cache have to spend in order to write into one of its entries?*/
	uint64_t cache_time_read, /*how many subcycles does the cache have to spend in order to read one of its entries?*/
	uint64_t cache_time_decode, /*how many subcycles does the cache have to spend to decode an address into a cache line ID?*/
	uint64_t cache_time_idle,
	uint64_t cache_time_populate_step
){
	cachedef c = {0};
	c.cacheline_sz = (((uint32_t)1)<<(uint32_t)cacheline_sz_pow2);
	c.state = CACHE_STATE_POPULATE_START;
	c.substate = CACHE_STATE_IDLE;
	c.ncachelines = ncachelines;
	c.queue_size = queue_size;
	c.cache_time_write = cache_time_write;
	c.cache_time_read = cache_time_read;
	c.cache_time_decode = cache_time_decode;
	c.cache_time_idle = cache_time_idle;
	c.cache_time_populate_step = cache_time_populate_step;
	c.current_state_consumed_cycles = 0;
	c.nqueue_requests = 0; /*At population time, there are no queued requests.*/
	c.current_state_consumed_cycles = 0;
	c.substate_consumed_cycles = 0;
	c.true_addresses = malloc(8 * ncachelines); /*simulated addresses*/
	if(cacheline_sz_pow2 > 30){
		puts("<SIMULATOR CONFIG ERROR> Invalid cache line size");
		exit(1);
	}
	c.cachelines = malloc(	c.cacheline_sz * ncachelines);
	if(c.true_addresses == NULL
		|| c.cachelines == NULL
	){
		puts("<SIMULATOR ERROR> Malloc Failed");
		exit(1);
	}
	c.mread = mread;
	c.mwrite = mwrite;
	c.mstat_read = mstat_read;
	c.mstat_write = mstat_write;
	return c;
}


uint32_t cache_statemachine(cachedef* c, uint64_t* master_cyclecounter); /*returns the current state.*/
uint32_t cache_write_request(uint64_t byte_addr, uint8_t bytevalue, uint64_t* master_cyclecounter);


