/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */

/*
 * general state machine framework.
 */
#ifndef _SM_H_
#define _SM_H_
#if defined(ATH_TARGET)
#include <athdefs.h>
#include <osapi.h>
#else
#include <a_types.h>
#include <a_osapi.h>
#endif


#define SM_STATE_NONE 0xffff /* invalid state */
#define SM_EVENT_NONE 0xffff /* invalid event */

/*
 * structure describing a state.
 */
typedef struct _sm_state_info {
    A_UINT16 state;                                             /* the state id */
    void (*sm_state_entry) ( void *ctx);                     /* entry action routine */
    void (*sm_state_exit)  ( void *ctx);                     /* exit action routine  */
    A_BOOL (*sm_state_event) ( void *ctx, A_UINT16 event,
                             A_UINT16 event_data_len, void *event_data);     /* event handler. returns true if event is handled */
} sm_state_info;

typedef struct _sm_param {
    A_UINT8             init_state;  /* initial state */
    A_UINT8             num_states;  /* number of states */
    const sm_state_info *state_info; /* array of state info elements of size num_states */
} sm_param;

#define SM_MAX_STATES 200 
#define SM_MAX_EVENTS 200 

struct state_machine_shared;
typedef struct state_machine_shared *sm_shared_t;

typedef A_UINT32 sm_t; 

struct state_machine_dbglog_data {
    A_UINT32 vap_id;
    A_UINT32 extra;
};

/**
 * create a shared state machine allocator
 *
 * @param num_sm:           total number of state machines to pre-allocate 
 * @param total_num_events: total number of events shared between all
 *                              state machines that are allocated from
 *                              these shared state machine allocator.
 * @param event_data_len:   size of the custom event data for this class
 *                              of state machines.
 * @param sm_shared_id:     a unique id for this class of state machines
 * @param sm_param:         sm info structure describing the state machine. 
 * @sm_debug_print:         if non-NULL, used to print debug messages by
 *
 * @return handle to shared state machine 
 */
sm_shared_t
sm_shared_create(
        A_UINT16 total_num_events,
        A_UINT16 event_data_len,
        A_UINT8 sm_shared_id,
        const sm_param *sm_param,
        void (*sm_debug_print) (void *ctx, const char *fmt,...),
        void (*sm_get_dbglog_data)(void *ctx, struct state_machine_dbglog_data *dbglog_data));

/**
 * initialize state machine private data area 
 *
 * @param shared:      handle to shared state machine allocator (see sm_shared_create)
 * @param smpriv:      pointer to sm private data area. caller owns the memory.   
 *
 */
void sm_init( sm_shared_t shared, sm_t *smpriv);


/**
 * send event into the state machine
 *
 * All the events should be directed via this function. The event will
 * be delivered to the SM. The implementation of the SM will call the
 * event handler pointer of the current state registered via the
 * sm_state_info array part of sm_param strucutre passed to sm_create
 * above.
 *
 * If the SM is already running, the event is queued up by allocating and
 * filling an event structure using from the shared SM event pool.
 *
 * The queued up events are handled as the recusion unwinds. e.g. 
 *
 *  sm_dispatch -> state_X_event_handler -> ... -> sm_dispatch
 *
 * @param shared:           handle to shared state machine allocator (see sm_shared_create)
 * @param smpriv:           pointer to SM private data.
 * @void *ctx:              handle to private context passed back to event handler.
 * @param event:            event to be delivered.
 * @param event_data_len:   length of the event data. 
 * @param event_data:       pointer to event_data (optional can be null and if 
 *      non null the length is specified by above param event_data_len). The 
 *      SM implementation does not interpret the contents of the event_data. 
 */
void sm_dispatch(sm_shared_t sm_shared, sm_t *smpriv, void *ctx, A_UINT16 event, A_UINT16 event_data_len, void *event_data); 

/**
 * transition to the new state. 
 *
 * @param shared:           handle to shared state machine allocator (see sm_shared_create)
 * @param smpriv:           pointer to sm private data area.
 * @void *ctx:              handle to private context passed back to event handler.
 * @param state                : transition to newstate.
 * in the process SM will call exit routine sm_state_exit pointer (if non null ) of the current state 
 * followed by the entry routine sm_state_entry pointer (if non null) of the new_state.
 */
void sm_transition_to(sm_shared_t sm_shared, sm_t *smpriv, void *ctx, A_UINT8 new_state); 

/**
 * return the current state.
 * @param sm:   handle to state machine
 * @return the current state .
 */
A_UINT8 sm_get_curstate(sm_t *smpriv);

/**
 * return the next state
 * @param sm:   handle to state machine
 * @return the next state
 */
A_UINT8 sm_get_next_state(sm_t *smpriv);


/**
 * return the previous state
 * @param sm:   handle to state machine
 * @return the previous state
 */
A_UINT8 sm_get_prev_state(sm_t *smpriv);

#endif
