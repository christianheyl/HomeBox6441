/* anwievent.c - functions for event handling */

/* Copyright (c) 2000 Atheros Communications, Inc., All Rights Reserved */

/*
DESCRIPTION
Contains low level functions for event handling.
*/

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/interrupt.h>

#include "dk_event.h"

//void acquireLock(p_event_queue);
//void releaseLock(p_event_queue);


spinlock_t driver_lock = SPIN_LOCK_UNLOCKED;

// initEventQueue - initialize an event queue
void initEventQueue(p_event_queue pQueue) 
{
	pQueue->pHead = pQueue->pTail = NULL;
	pQueue->queueSize = 0;
	return;
}


// deleteEventQueue - delete and cleanup a created event queue
// This routine will remove and delete all the event structures in the 
// queue. Performs any other cleanup  needed
// RETURNS: N/A
void deleteEventQueue(p_event_queue pQueue)
{
	p_event_struct pNextEvent;
	if (!pQueue) {
		return;
	}

	// acquireLock(pQueue);
	spin_lock_irq(&driver_lock);
    
	// cleanup the events in the queue 
	while (pQueue->pHead) {
		pNextEvent = pQueue->pHead->pNext;
		kfree(pQueue->pHead);
#ifdef DK_DEBUG
		printk("DK::Free event = %lx \n",(A_UINT_PTR)pQueue->pHead);
#endif
		pQueue->queueSize--;
		pQueue->pHead = pNextEvent;
	}

//	releaseLock(pQueue);
	spin_unlock_irq(&driver_lock);

	return;
}


/**************************************************************************
* createEvent - Create an event
*
* This routine will allocate memory for an event and perform any 
* initialization.  Note that this does not add the event to any Q
*
* RETURNS: pointer to the event of NULL if error
*/
p_event_struct createEvent
(
	UINT32 type,          /* the event ID */
	UINT32 persistent,    /* set if want a persistent event */
	UINT32 param1,        /* optional args */
	UINT32 param2,
	UINT32 param3,
	event_handle eventHandle  
)
{
	p_event_struct pEvent;    
	UINT32	i;

	pEvent = (p_event_struct)kmalloc(sizeof(event_struct),GFP_KERNEL);
#ifdef DK_DEBUG
	printk("DK::Create Event:Event Ptr = %x \n",(UINT32)pEvent);
#endif

	if(!pEvent) {
		printk("DK:: Unable to allocate memory in createEvent()!\n");
		return(NULL);
	}

	// perform initialization of members 
	pEvent->type = type;
	pEvent->persistent = persistent;
	pEvent->param1 = param1;
	pEvent->param2 = param2;
	pEvent->param3 = param3;
	pEvent->pNext = NULL;
	pEvent->pLast = NULL;
	for (i=0;i<6;i++) {
		pEvent->result[i]=0;
	}

	pEvent->eventHandle = eventHandle;

	return(pEvent);
}

/* copyEvent - Copy an event struct
*
* This routine will allocate memory for a new event and copy member 
* variables from existing event into new event
*
* RETURNS: pointer to the event of NULL if error
*/
p_event_struct copyEvent
(
	p_event_struct pExistingEvent     /* pointer to event to copy */
)
{
	p_event_struct pNewEvent;    /* pointer to event struct created */
	UINT32 i;

	if(!pExistingEvent) {
		printk("DK::Illegal pointer passed to copyEvent!\n");
		return(NULL);
	}
    
	//pNewEvent = (p_event_struct)kmalloc(sizeof(event_struct),GFP_KERNEL);
	pNewEvent = (p_event_struct)kmalloc(sizeof(event_struct),GFP_ATOMIC);
#ifdef DK_DEBUG
	printk("DK::Copy Event:Event Ptr = %lx \n",(A_UINT_PTR)pNewEvent);
#endif


	if(!pNewEvent) {
		printk("Error: Unable to allocate memory for new event!\n");
		return(NULL);
	}

	pNewEvent->type = pExistingEvent->type;
	pNewEvent->persistent = pExistingEvent->persistent;
	pNewEvent->param1 = pExistingEvent->param1;
	pNewEvent->param2 = pExistingEvent->param2;
	pNewEvent->param3 = pExistingEvent->param3;
	pNewEvent->pNext = NULL;
	pNewEvent->pLast = NULL;

	for (i=0;i<6;i++) {
		pNewEvent->result[i]=pExistingEvent->result[i];
	}

	pNewEvent->eventHandle.eventID = pExistingEvent->eventHandle.eventID;
	pNewEvent->eventHandle.f2Handle = pExistingEvent->eventHandle.f2Handle;

	return(pNewEvent);
}

/**************************************************************************
* pushEvent - Put event at tail of eventQueue
*
* This routine will put the supplied event at the end of the specified Q.
* don't want any other threads accessing the Q while doing this, so need
* to obtain the Q mutex to be able to perform this operation
*
* RETURNS: 1 if successful, 0 if not
*/
UINT16 pushEvent
(
	p_event_struct pEvent,   // pointer to event to add 
	p_event_queue pQueue,    // pointer to queue to add 
	BOOLEAN        protect      
)
{
	if(!pQueue || !pEvent) {
		printk("DK::illegal pointer passed to pushEvent()!\n");
		return(0);
	}

	if (protect) {
		// acquireLock(pQueue);
		spin_lock_irq(&driver_lock);
	}

	// add the event to the tail 
	if (NULL == pQueue->pHead) {
		// queue is empty, this will be the first item 
		pQueue->pHead = pEvent;
		pQueue->pTail = pEvent;    
	} else {
		pQueue->pTail->pNext = pEvent;
		pEvent->pLast = pQueue->pTail;
		pQueue->pTail = pEvent;
	}
	pQueue->queueSize++;

	if (protect) {
		//releaseLock(pQueue);
		spin_unlock_irq(&driver_lock);
	}

	return(1);
}

/**************************************************************************
* popEvent - Pop an event from eventQueue head
*
* This routine will pop an event from the head of the specified Q.
* Don't want any other threads accessing the Q while doing this, so need
* to obtain the Q mutex to be able to perform this operation.
*
* RETURNS: pointer to event that was poped, null if fail
*/
p_event_struct popEvent
(
	p_event_queue pQueue,        // pointer to queue to add to 
	BOOLEAN        protect      
)
{
	p_event_struct pEvent;        // event that will be poped from queue 
   
	if (protect) { 
		// acquireLock(pQueue);
		spin_lock_irq(&driver_lock);
	}

    // get the event from head and update pointers 
	pEvent = pQueue->pHead;
	if (pEvent) {
		pQueue->pHead = pEvent->pNext;    
		pEvent->pNext = NULL;            

		if(!pQueue->pHead) {
			// queue is now empty to make tail also null 
			pQueue->pTail = NULL;
		} else {
			pQueue->pHead->pLast = NULL;    // head has no previous link 
		}
		pQueue->queueSize--;
	}

	if (protect) {
		//releaseLock(pQueue);
		spin_unlock_irq(&driver_lock);
	}

    return(pEvent);
}


/**************************************************************************
* removeEvent - remove an event from eventQueue (anywhere)
*
* This routine will remove an event from anywhere (ie middle) in Q.
* The event to be removed is passed in.
*
* RETURNS: 1 if removed, 0 if not
*/ 
UINT16 removeEvent
(
	p_event_struct pEvent, // event that will be removed from queue 
	p_event_queue pQueue, // pointer to queue to add to 
	BOOLEAN        protect      
)
{
	if(!pQueue || !pEvent) {
		printk("DK::illegal pointer passed to removeEvent()\n");
		return(0);
        }

	if(protect) {
		// acquireLock(pQueue);
		spin_lock_irq(&driver_lock);
	}

	// first take care of previous member's forward link
	if ( pEvent->pLast ) {
		// this is not the head
		pEvent->pLast->pNext = pEvent->pNext;
	} else {
		// this is the head
		pQueue->pHead = pEvent->pNext;
	}

	// next take care of next member's backward link
	if ( pEvent->pNext ) {
		// this is not the tail
		pEvent->pNext->pLast = pEvent->pLast;
	} else {
		// this is the tail
		pQueue->pTail = pEvent->pLast;
	}	

	// clear event to be no longer linked 
	pEvent->pLast = NULL;
	pEvent->pNext = NULL;
	pQueue->queueSize--;

	// release the mutex 
	if(protect) {
		//releaseLock(pQueue);
		spin_unlock_irq(&driver_lock);
	}

	return(1);
}

/**************************************************************************
* CheckForEvents - Check if queue has any events
*
* This routine will check to see if an event queue has any events in it.
* Need this to be a mutually exclusive operation so need to get mutex
*
* RETURNS: Return 1 if events, 0 if not or there are errors
*/
UINT16 checkForEvents
(
	p_event_queue pQueue,    // Pointer to event queue to start scan 
	BOOLEAN        protect      
)
{
	UINT16    returnValue;

	if (!pQueue) {
		printk("DK::illegal pointer passed to checkForEvents()\n");
		return(0);
	}

	if (protect) {
//		acquireLock(pQueue);
		spin_lock_irq(&driver_lock);
		
	}

	if(!pQueue->pHead) {
		returnValue = 0; // queue is empty, there are no events 
	} else {
		returnValue = 1; // queue contains events 
	}

	if (protect) {
//		releaseLock(pQueue);
		spin_unlock_irq(&driver_lock);
	}

	return(returnValue);
}
/*
*/
