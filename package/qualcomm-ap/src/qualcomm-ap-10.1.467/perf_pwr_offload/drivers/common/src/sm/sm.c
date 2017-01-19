/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */

#include <sm.h>
#include <queue.h>
#include <pool_mgr_api.h>

struct sm_event_info {
    void *smpriv;
    void *ctx;
    A_UINT16 event_type;
    A_UINT16 event_data_len;
    STAILQ_ENTRY(sm_event_info) link_event;
    /* keep last! */
    void *event_data[0];
};

struct state_machine_shared {
    void *sm_pool;
    void *event_pool;
    STAILQ_HEAD(, sm_event_info)  event_list;
    A_UINT16 event_data_len;

    const sm_state_info *state_info;
    A_UINT8 num_states;
    A_UINT8 init_state;

    /* debug info */
    A_UINT8 sm_shared_id; 
    void (*sm_debug_print) (void *ctx, const char *fmt,...);
    void (*sm_get_dbglog_data)(void *ctx, struct state_machine_dbglog_data *dbglog_data);
};

#if defined(IRAM_TID)
typedef struct state_machine {
    /* we can't have 8 bit bit-fields aligned on a byte boundary when in IRAM 
     * as that causes a byte read/write causing an access exception 
     */ 
    A_UINT32                       in_state_transition:1,
                                   is_running:1,
                                   cur_state:10,
                                   next_state:10,    /* different from cur_state in the middle of sm_transition_to */
                                   prev_state:10;
}sm_info ;
#else
typedef struct state_machine {
    A_UINT32                       cur_state:8,
                                   next_state:8,    /* different from cur_state in the middle of sm_transition_to */
                                   prev_state:8,
                                   in_state_transition:1,
                                   is_running:1;
}__ATTRIB_PACK sm_info ;
#endif


#define SM_DPRINTF(shared, ctx, fmt, ...)  do {             \
        if (shared->sm_debug_print) {                       \
            shared->sm_debug_print(ctx, fmt, __VA_ARGS__);  \
        }                                                   \
    } while (0)

/*
 * State machine framework DBGLOG message format
 *
 * Bytes   3      2      1      0
 *      *---------------------------*
 *      | type | arg1 | arg2 | arg3 |
 *      *---------------------------*
 *
 * State transition
 *  type = 0
 *  arg1 = old state
 *  arg2 = new state
 *  arg3 = reserved
 *
 * Dispatch event
 *  type = 1
 *  arg1 = current state
 *  arg2 = event id
 *  arg3 = { 0: immediate, 1: removed from queue }
 *
 * Warning
 *  type = 2
 *  arg1 = error type
 *  arg2 = reserved     // could have special meaning based on the error type
 *  arg3 = reserved
 *
 *      Warning (Event not handled)
 *      type = 2
 *      arg1 = 0
 *      arg2 = current state
 *      arg3 = event
 */

enum sm_dbglog_msg_type {
    SM_DBGLOG_MSG_TYPE_STATE_TRANSITION = 0,
    SM_DBGLOG_MSG_TYPE_DISPATCH_EVENT   = 1,
    SM_DBGLOG_MSG_TYPE_WARNING          = 2,
};

enum sm_dbglog_msg_warning {
    SM_DBGLOG_MSG_WARNING_EVENT_NOT_HANDLED = 0,
};

#define MAKE_SM_DBGLOG_MSG(type, arg1, arg2, arg3) \
    (((type << 24) & 0xff000000) | \
     ((arg1 << 16) & 0x00ff0000) | \
     ((arg2 <<  8) & 0x0000ff00) | \
     ((arg3 <<  0) & 0x000000ff))

sm_shared_t
sm_shared_create(
        A_UINT16 total_num_events,
        A_UINT16 event_data_len,
        A_UINT8 sm_shared_id,
        const sm_param *sm_param,
        void (*sm_debug_print) (void *ctx, const char *fmt,...),
        void (*sm_get_dbglog_data)(void *ctx, struct state_machine_dbglog_data *dbglog_data))
{
    struct state_machine_shared *shared;
    const sm_state_info *state_info = sm_param->state_info;
    A_UINT32 i;

    A_ASSERT(sm_param->num_states > 0);
    A_ASSERT(sm_param->num_states < SM_MAX_STATES);

    /* Each state needs to be in order */
    for (i = 0; i < sm_param->num_states; ++i) {
        A_ASSERT(state_info[i].state == i);
    }

    /* NB: A_ALLOCRAM or pool_init failure is a fatal error */
    shared = A_ALLOCRAM(sizeof(*shared));
    shared->event_pool = pool_init(sizeof(struct sm_event_info) +
            event_data_len, total_num_events, 0);
    shared->sm_shared_id = sm_shared_id;
    shared->sm_get_dbglog_data = sm_get_dbglog_data;
    shared->sm_debug_print = sm_debug_print;
    shared->event_data_len = event_data_len;
    shared->init_state = sm_param->init_state;
    shared->num_states = sm_param->num_states;
    shared->state_info = state_info;

    STAILQ_INIT(&shared->event_list);
    return shared;
}

void sm_init( 
      sm_shared_t shared, 
      sm_t *smpriv)
{
    sm_info *sm = (sm_info *) smpriv;

    /* Clear sm structure */
    A_MEMZERO(sm, sizeof(sm_info));

    sm->cur_state   = shared->init_state;
    sm->next_state  = shared->init_state;
    sm->prev_state  = shared->init_state;
}


A_UINT8 sm_get_curstate(sm_t *smpriv)
{
    sm_info *sm = (sm_info *) smpriv;
    return sm->cur_state;
}

A_UINT8 sm_get_next_state(sm_t *smpriv)
{
    sm_info *sm = (sm_info *) smpriv;
    return sm->next_state;
}

A_UINT8 sm_get_prev_state(sm_t *smpriv)
{
    sm_info *sm = (sm_info *) smpriv;
    return sm->prev_state;
}

static void
sm_dispatch_internal(
        sm_shared_t shared, 
        sm_t *smpriv, 
        void *ctx, 
        A_UINT16 event,
        A_UINT16 event_data_len,
        void *event_data,
        A_UINT8 was_event_queued)
{
    sm_info *sm = (sm_info *) smpriv;
    const sm_state_info *state_info = shared->state_info;
    A_UINT8 state = sm->cur_state;
    A_BOOL event_handled;
    struct state_machine_dbglog_data dbglog_data = { 
        .vap_id = -1, 
        .extra = 0,
    };

    shared->sm_get_dbglog_data(ctx, &dbglog_data);

    if (event == SM_EVENT_NONE) {
        SM_DPRINTF(shared, ctx, "SM: %d:%p : invalid event %d\n", 
                shared->sm_shared_id, smpriv, event);
        A_ASSERT(0);
        return;
    }

    DBGLOG_RECORD_LOG(shared->sm_shared_id, dbglog_data.vap_id,
            DBGLOG_DBGID_SM_FRAMEWORK_PROXY_DBGLOG_MSG, DBGLOG_VERBOSE, 2,
            MAKE_SM_DBGLOG_MSG(SM_DBGLOG_MSG_TYPE_DISPATCH_EVENT, state,
                event, was_event_queued), dbglog_data.extra);

    SM_DPRINTF(shared, ctx, "SM: %d:%p :current state %d event %d\n",
            shared->sm_shared_id, smpriv, sm->cur_state, event);

    event_handled = state_info[state].sm_state_event(ctx,
            event, event_data_len, event_data);

    if (!event_handled) {
        SM_DPRINTF(shared, ctx, 
                "SM: %d:%p : event %d not handled in state %d\n",
                shared->sm_shared_id, smpriv, event, sm->cur_state);

        DBGLOG_RECORD_LOG(shared->sm_shared_id, dbglog_data.vap_id,
                DBGLOG_DBGID_SM_FRAMEWORK_PROXY_DBGLOG_MSG, DBGLOG_VERBOSE, 2,
                MAKE_SM_DBGLOG_MSG(SM_DBGLOG_MSG_TYPE_WARNING,
                    SM_DBGLOG_MSG_WARNING_EVENT_NOT_HANDLED, state, event),
                dbglog_data.extra);
    }
}

void 
sm_dispatch(
     sm_shared_t shared, 
     sm_t *smpriv, 
     void *ctx, 
     A_UINT16 event, 
     A_UINT16 event_data_len, 
     void *event_data) 
{
    sm_info *sm = (sm_info *) smpriv;
    struct sm_event_info *ev;

    if (!sm->is_running) {

        sm->is_running = TRUE;
        sm_dispatch_internal(shared,smpriv,ctx, event, event_data_len, event_data, FALSE);
        sm->is_running = FALSE;

        /* 
         * dispatch events from the event queue shared between all
         * instances of a state machine 'class'.
         *
         * This may dispatch events and in turn run the state machine
         * for instances other than the state machine for which this
         * function was originally called.
         */
        ev = STAILQ_FIRST(&shared->event_list);
        if (ev) {
            STAILQ_HEAD(, sm_event_info) skipped_events =
                STAILQ_HEAD_INITIALIZER(skipped_events);

            do {
                sm_info *deferred_sm = ev->smpriv;

                STAILQ_REMOVE_HEAD(&shared->event_list, link_event);

                if (deferred_sm->is_running) {
                    STAILQ_INSERT_TAIL(&skipped_events, ev, link_event);
                } else {
                    deferred_sm->is_running = TRUE;
                    sm_dispatch_internal(shared, ev->smpriv, ev->ctx,
                            ev->event_type, ev->event_data_len,
                            ev->event_data, TRUE);
                    deferred_sm->is_running = FALSE;

                    pool_free(shared->event_pool, ev);
                }

                ev = STAILQ_FIRST(&shared->event_list);
            } while (ev);

            STAILQ_CONCAT(&shared->event_list, &skipped_events);
        }
    } else {
        /* caller needs to make sure the event data will fit in event
         * pool element */
        A_ASSERT(event_data_len <= shared->event_data_len);

        ev = pool_alloc(shared->event_pool);
        if (NULL == ev) {
            SM_DPRINTF(shared, ctx, 
                    "SM: %d:%p : allocation from event pool failed for event %d\n", 
                    shared->sm_shared_id, sm, event);
            A_ASSERT(0); 
            return;
        }

        ev->event_type = event;
        ev->event_data_len = event_data_len;
        ev->smpriv = smpriv;
        ev->ctx = ctx;
        if (event_data_len) {
            A_MEMCPY(ev->event_data, event_data, event_data_len);
        }

        STAILQ_INSERT_TAIL(&shared->event_list, ev, link_event);
    }                    
}

void sm_transition_to( sm_shared_t shared, sm_t *smpriv, void *ctx, A_UINT8 state)
{
    const sm_state_info *state_info = shared->state_info;
    sm_info *sm = (sm_info *) smpriv;
    A_UINT16 cur_state = sm->cur_state;
    struct state_machine_dbglog_data dbglog_data = { 
        .vap_id = -1, 
        .extra = 0,
    };

    /* cannot change state from state entry/exit routines */
    A_ASSERT(sm->in_state_transition == 0);
    A_ASSERT(state < shared->num_states);

    shared->sm_get_dbglog_data(ctx, &dbglog_data);

    DBGLOG_RECORD_LOG(shared->sm_shared_id, dbglog_data.vap_id,
            DBGLOG_DBGID_SM_FRAMEWORK_PROXY_DBGLOG_MSG, DBGLOG_VERBOSE, 2,
            MAKE_SM_DBGLOG_MSG(SM_DBGLOG_MSG_TYPE_STATE_TRANSITION, cur_state,
                state, 0), dbglog_data.extra); 

    SM_DPRINTF(shared, ctx, 
            "SM: transition %d => %d \n", cur_state,state);

    sm->in_state_transition = TRUE;

    sm->next_state = state;

    /* call the exit function of the current state */
    if (state_info[cur_state].sm_state_exit) {
        state_info[cur_state].sm_state_exit(ctx);
    }

    cur_state = state;
    sm->prev_state = cur_state;

    /* call the entry function of the current state */
    if (state_info[cur_state].sm_state_entry) {
        state_info[cur_state].sm_state_entry(ctx);
    }

    sm->cur_state = sm->next_state;
    sm->in_state_transition = FALSE;
}

