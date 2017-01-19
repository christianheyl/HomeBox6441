#define KPI_TESTLOOP

/*******************************************************************************

                              Copyright (c) 2013
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

/**
   \file drv_tapi_kpi.c
   This file contains the implementation of the "Kernel Packet Interface" (KPI).
   The KPI is used to exchange packetised data with other drivers.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_tapi.h"

#if (TAPI_CFG_FEATURES & TAPI_FEAT_KPI)

#ifdef LINUX
#ifndef KPI_TASKLET
   #warning KPI TASKLET mode disabled!!!
#endif /*!KPI_TASKLET*/
#ifndef LINUX_2_6
#include <linux/threads.h>
#include <linux/spinlock.h>
#endif /* LINUX_2_6 */
#include <linux/interrupt.h>
#endif /* LINUX */

#include "drv_tapi_kpi.h"
#include <ifxos_thread.h>
#include "lib_bufferpool.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/** Get group which is coded in the upper 4 bit of the channel parameter. */
#define KPI_GROUP_GET(channel)    (((channel) >> 12) & 0x000F)
/** Get the channel number without the group number in the upper 4 bits. */
#define KPI_CHANNEL_GET(channel)  ((channel) & 0x0FFF)
/** Definition of maximum KPI group that can be used (allowed range: 1 - 15). */
#define IFX_TAPI_KPI_MAX_GROUP              15

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/** Struct that holds all data for one KPI group. A group is the interface
    towards one specific driver. */
typedef struct
{
   /** egress fifo */
   FIFO_ID             *pEgressFifo;
   /** egress fifo protection */
   TAPI_OS_mutex_t      semProtectEgressFifo;
   /** congestion state of the egress fifo */
   IFX_boolean_t        bEgressFifoCongested;
   /** ingress fifo */
   FIFO_ID             *pIngressFifo;
   /** ingress fifo protection */
   TAPI_OS_mutex_t      semProtectIngressFifo;
   /** congestion state of the ingress fifo */
   IFX_boolean_t        bIngressFifoCongested;
   /** Map from KPI channel to the corresponding TAPI channel.
       The KPI channel is the index to this map. */
   TAPI_CHANNEL        *channel_map[IFX_TAPI_KPI_MAX_CHANNEL_PER_GROUP];
   /** Map from KPI channel to the stream that packets belong to. */
   IFX_TAPI_KPI_STREAM_t stream_map[IFX_TAPI_KPI_MAX_CHANNEL_PER_GROUP];
   /** optional KPI egress tasklet for this group */
#ifdef KPI_TASKLET
   IFX_void_t          *pEgressTasklet;
#endif /* KPI_TASKLET */
} IFX_TAPI_KPI_GROUP_t;

/** Struct that is put as an element into the fifos and keeps the data
    together with the channel information. The fields for the channels
    are used depending on the direction. In ingress direction only the
    TAPI_CHANNEL is valid and the IFX_TAPI_KPI_CH_t is undefined. In
    egress direction only the IFX_TAPI_KPI_CH_t is valid and the
    TAPI_CHANNEL field is undefined. */
typedef struct
{
   /** KPI channel this buffer is sent on (egress direction) */
   IFX_TAPI_KPI_CH_t    nKpiCh;
   /** TAPI channel this buffer is for (ingress direction) */
   TAPI_CHANNEL        *pTapiCh;
   /** Pointer to a buffer from lib-bufferpool with the payload data */
   IFX_void_t          *pBuf;
   /** Reserved for future use. Pointer to first data in the buffer */
   IFX_void_t          *pData ;
   /** Length of data in the buffer counted in bytes  */
   IFX_uint32_t         nDataLength;
   /** Stream that this packet is for */
   IFX_TAPI_KPI_STREAM_t nStream;
} IFX_TAPI_KPI_FIFO_ELEM_t;


/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/** Array with all KPI group specific data */
static IFX_TAPI_KPI_GROUP_t *kpi_group[IFX_TAPI_KPI_MAX_GROUP];
/** Array of semaphores that signals data in the egress fifo of a group */
static TAPI_OS_lock_t        semWaitOnEgressFifo[IFX_TAPI_KPI_MAX_GROUP];
/** One semaphore to signal data in any ingress fifo */
static TAPI_OS_lock_t        semWaitOnIngressFifo;
/** Semaphore to protect the wrapper bufferpool against concurrent access */
static TAPI_OS_mutex_t       semProtectWrapperBufferpool;
/** Handle of the bufferpool used to allocate wrapper buffers from */
static BUFFERPOOL           *wrapperBufferpool;
/** Hold information of the ingress worker thread */
static TAPI_OS_ThreadCtrl_t  ingressThread;
#ifdef KPI_TESTLOOP
static TAPI_OS_ThreadCtrl_t  loopbackThread;
#endif /* KPI_TESTLOOP */
#ifdef KPI_TASKLET
/** global variable used to configure the ingress packet handling via
    insmod option */
extern IFX_int32_t           block_ingress_tasklet;
#endif /* KPI_TASKLET */
/** Translate KPI streams into TAPI streams */
const static IFX_TAPI_STREAM_t translateStreamTable[IFX_TAPI_KPI_STREAM_MAX] =
{
   /* IFX_TAPI_KPI_STREAM_COD  -> */ IFX_TAPI_STREAM_COD,
   /* IFX_TAPI_KPI_STREAM_DECT -> */ IFX_TAPI_STREAM_DECT,
   /* IFX_TAPI_KPI_STREAM_HDLC -> */ IFX_TAPI_STREAM_HDLC
};

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_void_t ifx_tapi_KPI_IngressHandler (IFX_ulong_t foo);
#ifdef KPI_TASKLET
DECLARE_TASKLET(tl_kpi_ingress, ifx_tapi_KPI_IngressHandler, 0L);
#endif /* KPI_TASKLET */
static IFX_int32_t ifx_tapi_KPI_IngressThread (TAPI_OS_ThreadParams_t *pThread);
#ifdef KPI_TESTLOOP
IFX_int32_t ifx_tapi_KPI_TestloopThread (TAPI_OS_ThreadParams_t *pThread);
#endif /* KPI_TESTLOOP */
static IFX_return_t ifx_tapi_KPI_GroupInit(IFX_uint32_t nKpiGroup);


/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */
/*lint -esym(529,lock)  lint cannot see that this is used in assembler code */

/**
   Initialise the Kernel Packet Interface (KPI)

   \return Return values are defined within the \ref IFX_return_t definition
   - IFX_SUCCESS  in case of success
   - IFX_ERROR if operation failed
*/
IFX_return_t IFX_TAPI_KPI_Init (void)
{
   IFX_uint8_t  i;

   /* set array of groups to NULL */
   memset(kpi_group, 0x00, sizeof(kpi_group));
   /* the groups are allocated later on configuration */

   /* create semaphore to signal data is in the ingress fifos */
   TAPI_OS_LockInit (&semWaitOnIngressFifo);
   /* inital state of the semaphore should be locked so take it */
   TAPI_OS_LockGet (&semWaitOnIngressFifo);

   /* Loop over all groups in the KPI */
   for (i = 0; i < IFX_TAPI_KPI_MAX_GROUP; i++)
   {
      /* create semaphore to signal data in the egress fifo */
      TAPI_OS_LockInit (&semWaitOnEgressFifo[i]);
      /* inital state of the semaphore should be locked so take it */
      TAPI_OS_LockGet (&semWaitOnEgressFifo[i]);
   }

   /* Create a bufferpool for wrapper structs passed in the fifos.
      The initial number of elements in the pool is set so that all slots of
      one fifo can be filled. The pool grows if needed by the same number
      of elements in every step. */
   if (!wrapperBufferpool)
   {
      wrapperBufferpool = bufferPoolInit(sizeof(IFX_TAPI_KPI_FIFO_ELEM_t),
                                         (IFX_TAPI_KPI_EGRESS_FIFO_SIZE +
                                          IFX_TAPI_KPI_INGRESS_FIFO_SIZE),
                                         (IFX_TAPI_KPI_EGRESS_FIFO_SIZE +
                                          IFX_TAPI_KPI_INGRESS_FIFO_SIZE),
                                         IFX_TAPI_KPI_GROWTH_LIMIT);
      bufferPoolIDSet (wrapperBufferpool, 22);
   }

   /* create semaphore to protect access to the wrapper buffer pool */
   TAPI_OS_MutexInit (&semProtectWrapperBufferpool);

   /* start a thread working on the ingress queues */
   return (0 == TAPI_OS_ThreadInit (&ingressThread, "TAPIkpi_in",
                              ifx_tapi_KPI_IngressThread,
                              0, TAPI_OS_THREAD_PRIO_HIGHEST, 0, 0)) ?
                              IFX_SUCCESS : IFX_ERROR;
}

/**
   Retrieve the overall number of elements of the voice-packet KPI wrapper
   bufferpool.

   \return the overall number of elements
*/
IFX_int32_t IFX_TAPI_KPI_VoiceBufferPool_ElementCountGet(void)
{
   TAPI_OS_INTSTAT lock;
   IFX_int32_t  elements;

   if (!wrapperBufferpool)
      return 0;

   if (!TAPI_OS_IN_INTERRUPT())
   {
      TAPI_OS_MutexGet (&semProtectWrapperBufferpool);
   }
   TAPI_OS_LOCKINT(lock);

   elements = bufferPoolSize( wrapperBufferpool );

   TAPI_OS_UNLOCKINT(lock);
   if (!TAPI_OS_IN_INTERRUPT())
   {
      TAPI_OS_MutexRelease (&semProtectWrapperBufferpool);
   }

   return elements;
}

/**
   Retrieve the available (free) number of elements of the
   voice-packet KPI wrapper bufferpool.

   \return the number of available elements
*/
IFX_int32_t IFX_TAPI_KPI_BufferPool_ElementAvailCountGet(void)
{
   TAPI_OS_INTSTAT lock;
   IFX_int32_t  elements;

   if (!wrapperBufferpool)
      return 0;

   if (!TAPI_OS_IN_INTERRUPT())
   {
      TAPI_OS_MutexGet (&semProtectWrapperBufferpool);
   }
   TAPI_OS_LOCKINT(lock);

   elements = bufferPoolAvail( wrapperBufferpool );

   TAPI_OS_UNLOCKINT(lock);
   if (!TAPI_OS_IN_INTERRUPT())
   {
      TAPI_OS_MutexRelease (&semProtectWrapperBufferpool);
   }

   return elements;
}


/**
   Clean-up the Kernel Packet Interface (KPI)

   \return none
   \remarks
   There is currently no protection here during the cleanup phase. So the read
   and write functions may crash when done while the cleanup is called.
   So first shut down all clients using the KPI before calling the cleanup.
   If driver crashes on unload protection could be added later.
*/
IFX_void_t IFX_TAPI_KPI_Cleanup (void)
{
   IFX_TAPI_KPI_FIFO_ELEM_t *pElem;
   IFX_uint8_t               i, j;
   IFX_TAPI_KPI_STREAM_t     nStream;
   TAPI_OS_INTSTAT           lock;
   TAPI_CHANNEL             *pTapiCh;

   /* stop the task working on the ingress queues */
   TAPI_OS_THREAD_KILL (&ingressThread);
#ifdef KPI_TESTLOOP
   if (kpi_group[0] != IFX_NULL)
   {
      /* loopback thread is only running if group 1 is initialised (1-1=0) */
      TAPI_OS_THREAD_KILL (&loopbackThread);
      //TAPI_OS_ThreadDelete (&loopbackThread, KPI_THREAD_TO);
   }
#endif /* KPI_TESTLOOP */

   /* Loop over all groups in the KPI */
   for (i = 0; i < IFX_TAPI_KPI_MAX_GROUP; i++)
   {
      if (kpi_group[i] != IFX_NULL)
      {
         /* Reset all stream switch structs to stop traffic into fifos of this
            KPI group. Loop over all KPI channels in this group. */
         for (j=0; j < IFX_TAPI_KPI_MAX_CHANNEL_PER_GROUP; j++)
         {
            /* lookup the tapi channel associated with a KPI channel */
            pTapiCh = kpi_group[i]->channel_map[j];
            if (pTapiCh != IFX_NULL)
            {
               /* take protection semaphore */
               TAPI_OS_MutexGet (&pTapiCh->semTapiChDataLock);
               /* global irq lock */
               TAPI_OS_LOCKINT(lock);

               /* send all streams to the application */
               for (nStream = IFX_TAPI_KPI_STREAM_COD;
                    nStream < IFX_TAPI_KPI_STREAM_MAX; nStream++)
               {
                  pTapiCh->pKpiStream[nStream].nKpiCh = 0;
                  pTapiCh->pKpiStream[nStream].pEgressFifo = IFX_NULL;
               }
               /* global irq unlock */
               TAPI_OS_UNLOCKINT(lock);
               /* release protection semaphore */
               TAPI_OS_MutexRelease (&pTapiCh->semTapiChDataLock);
            }
         }

         /* delete semaphores for protecting the fifos */
         TAPI_OS_MutexDelete (&kpi_group[i]->semProtectEgressFifo);
         TAPI_OS_MutexDelete (&kpi_group[i]->semProtectIngressFifo);

         /* flush the data fifos for egress and ingress direction */
         while ((pElem = fifoGet (kpi_group[i]->pEgressFifo, NULL)) != IFX_NULL)
         {
            if (IFX_TAPI_VoiceBufferPut(pElem->pBuf) != IFX_SUCCESS)
            {
               /* This should never happen! Warn but do not stop here. */
               TRACE (TAPI_DRV, DBG_LEVEL_HIGH,
                      ("\nBuffer put-back error(1a)\n"));
            }
            if (bufferPoolPut(pElem) != IFX_SUCCESS)
            {
               /* This should never happen! Warn but do not stop here. */
               TRACE (TAPI_DRV, DBG_LEVEL_HIGH,
                      ("\nBuffer put-back error(1b)\n"));
            }
         }
         while ((pElem = fifoGet(kpi_group[i]->pIngressFifo, NULL)) != IFX_NULL)
         {
            if (IFX_TAPI_VoiceBufferPut(pElem->pBuf) != IFX_SUCCESS)
            {
               /* This should never happen! Warn but do not stop here. */
               TRACE (TAPI_DRV, DBG_LEVEL_HIGH,
                      ("\nBuffer put-back error(2a)\n"));
            }
            if (bufferPoolPut(pElem) != IFX_SUCCESS)
            {
               /* This should never happen! Warn but do not stop here. */
               TRACE (TAPI_DRV, DBG_LEVEL_HIGH,
                      ("\nBuffer put-back error(2b)\n"));
            }
         }

         /* delete data fifos for ingress and egress direction */
         fifoFree (kpi_group[i]->pEgressFifo);
         fifoFree (kpi_group[i]->pIngressFifo);

         /* free the allocated group structures */
         TAPI_OS_Free (kpi_group[i]);
         kpi_group[i] = IFX_NULL;
      }
      /* delete semaphore to signal data in the egress fifo */
      TAPI_OS_LockDelete (&semWaitOnEgressFifo[i]);
   }

   TAPI_OS_LockDelete (&semWaitOnIngressFifo);

   /* free the buffer pool for the wrapper structs. */
   bufferPoolFree(wrapperBufferpool);
   /* delete semaphore protecting the wrapper buffer pool */
   TAPI_OS_MutexDelete (&semProtectWrapperBufferpool);
}


/**
   Sleep until data is available for reading with \ref IFX_TAPI_KPI_ReadData.

   \param  nKpiGroup    KPI group to wait on for new data.

   \return Returns value as follows:
   - IFX_SUCCESS:  Data is now available for reading in the specified
                        KPI group.
   - IFX_ERROR:    If invalid parameters were given or interrupted by
                        signal.
*/
IFX_return_t IFX_TAPI_KPI_WaitForData( IFX_TAPI_KPI_CH_t nKpiGroup )
{
   /* Get the KPI-group number */
   nKpiGroup = KPI_GROUP_GET(nKpiGroup);
   /* Reject group values which are out of the configured range. */
   if ((nKpiGroup == 0) || (nKpiGroup > IFX_TAPI_KPI_MAX_GROUP))
      return IFX_ERROR;
   /* Take the signalling semaphore - this is blocking until data is available
      or a signal is sent to the process. */
   if (TAPI_OS_LOCK_GET_INTERRUPTIBLE (&semWaitOnEgressFifo[nKpiGroup-1]) != 0)
   {
      /* interrupted by signal */
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
}


/**
   Read function for KPI clients to read a packet from TAPI KPI.

   \param  nKpiGroup    KPI group where to read data from.
   \param  *nKpiChannel Returns the KPI channel number within the given
                        group where the packet was received.
   \param  **pPacket    Returns a pointer to a bufferpool element with the
                        received data. The ownership of the returned bufferpool
                        element is passed to the client calling this interface.
                        It is responsibility of the client to free this element
                        by calling \ref IFX_TAPI_VoiceBufferPut after having
                        processed the data.
   \param  *nPacketLength  Returns the length of the received data. If the
                        returned length is 0, it means that no packets were
                        available for reading.
   \param  *nMore       Returns whether more packets are ready to be read
                        within the same KPI group. 0 means no more packets
                        ready, 1 means more packets available for reading.

   \return
   Returns the number of data bytes successfully read or IFX_ERROR otherwise.
*/
IFX_int32_t IFX_TAPI_KPI_ReadData( IFX_TAPI_KPI_CH_t nKpiGroup,
                                   IFX_TAPI_KPI_CH_t *nKpiChannel,
                                   IFX_void_t **pPacket,
                                   IFX_uint32_t *nPacketLength,
                                   IFX_uint8_t *nMore)
{
   TAPI_OS_INTSTAT           lock;
   IFX_TAPI_KPI_FIFO_ELEM_t *pElem  = IFX_NULL;
   IFX_TAPI_KPI_CH_t         nKpiChannelOnly;
   IFX_int32_t               ret;

   /* clean return values */
   *pPacket = IFX_NULL;
   *nPacketLength = 0;
   *nMore = 0;

   /* Get the KPI-group number */
   nKpiGroup = KPI_GROUP_GET(nKpiGroup);
   /* Reject group values which are out of configured range or for groups
      which have not been configured yet. */
   if ((nKpiGroup == 0) || (nKpiGroup > IFX_TAPI_KPI_MAX_GROUP) ||
       (kpi_group[nKpiGroup-1] == IFX_NULL))
      return IFX_ERROR;
   /* Adjust group values from channel notation to internal representation */
   nKpiGroup--;

   /* The read access to the fifo is protected in two ways:
       First it is protected from concurrent reads with this function
       second it is protected from writing of new data in irq context. */

   /* take protection semaphore */
   if (!TAPI_OS_IN_INTERRUPT())
      TAPI_OS_MutexGet (&kpi_group[nKpiGroup]->semProtectEgressFifo);
   /* global irq lock */
   TAPI_OS_LOCKINT(lock);
   /* read element from fifo */
    pElem = fifoGet (kpi_group[nKpiGroup]->pEgressFifo, NULL);
   /* set the more flag */
   *nMore = fifoEmpty(kpi_group[nKpiGroup]->pEgressFifo) ? 0 : 1;
   /* global irq unlock */
   TAPI_OS_UNLOCKINT(lock);

   /* when there was data in the fifo return values and discard wrapper */
   if (pElem != NULL)
   {
      /* store return values in the parameters */
      *nKpiChannel = pElem->nKpiCh;
      *pPacket = pElem->pBuf;
      *nPacketLength = pElem->nDataLength;
      /* strip the group from the channel parameter */
      nKpiChannelOnly = KPI_CHANNEL_GET (pElem->nKpiCh);
      /* check range and update the statistic */
      if (nKpiChannelOnly < IFX_TAPI_KPI_MAX_CHANNEL_PER_GROUP)
      {
         IFX_TAPI_Stat_Add(kpi_group[nKpiGroup]->channel_map[nKpiChannelOnly],
                           translateStreamTable[
                           kpi_group[nKpiGroup]->stream_map[nKpiChannelOnly]],
                           TAPI_STAT_COUNTER_EGRESS_DELIVERED, 1);
      }
      /* protect the access to the bufferpool for wrappers */
      if (!TAPI_OS_IN_INTERRUPT())
         TAPI_OS_MutexGet (&semProtectWrapperBufferpool);
      /* global irq lock */
      TAPI_OS_LOCKINT(lock);
      /* release the wrapping structure for the data buffer */
      ret = bufferPoolPut(pElem);
      /* global irq unlock */
      TAPI_OS_UNLOCKINT(lock);
      /* if ! in irq release protection semaphore */
      if (!TAPI_OS_IN_INTERRUPT())
         TAPI_OS_MutexRelease (&semProtectWrapperBufferpool);
      /* warn if returning the wrapper buffer failed */
      if (ret != IFX_SUCCESS)
      {
         /* This should never happen! */
         TRACE (TAPI_DRV, DBG_LEVEL_HIGH, ("\nBuffer put-back error(3)\n"));
      }
   }

   /* release protection semaphore */
   if (!TAPI_OS_IN_INTERRUPT())
      TAPI_OS_MutexRelease (&kpi_group[nKpiGroup]->semProtectEgressFifo);

   /* pElem is still valid but the buffer it points to is not valid any more */
   /* taking in consideration size of IFX_int32_t casting below should be safe */
   return (pElem != NULL) ? (IFX_int32_t) *nPacketLength : IFX_ERROR;
}

/**
   Schedule handler for KPI ingress direction

   \return
      None
*/
IFX_void_t IFX_TAPI_KPI_ScheduleIngressHandling (void)
{
   /* signal the event that there is data in one of the ingress fifos */
   TAPI_OS_LockRelease (&semWaitOnIngressFifo);
}

/**
   Write function for KPI clients to write a packet to TAPI KPI.

   \param  nKpiChannel  KPI channel the data is written to.
   \param  pPacket      Pointer to a bufferpool element with the data to be
                        written. Bufferpool element must be from the TAPI
                        voice buffer pool. Use \ref IFX_TAPI_VoiceBufferGet
                        to get a bufferpool element.
   \param  nPacketLength  Length of the data to be written in bytes.

   \return
   On success, the number of bytes written is returned (zero indicates
   nothing was written). On error, IFX_ERROR is returned.

   \remarks
   The ownership of the bufferpool element is only passed to the KPI if this
   call successfuly wrote all data. This is only the case if this function
   returns the same value as given in the parameter nPacketLength. When this
   write fails or fewer data than given was written the buffer still
   belongs to the caller and this has to discard it or write it again.
*/
IFX_int32_t IFX_TAPI_KPI_WriteData( IFX_TAPI_KPI_CH_t nKpiChannel,
                                    IFX_void_t *pPacket,
                                    IFX_uint32_t nPacketLength)
{
   IFX_TAPI_KPI_FIFO_ELEM_t *pElem;
   TAPI_CHANNEL             *pTapiCh;
   TAPI_OS_INTSTAT           lock;
   IFX_uint32_t              space;
   IFX_int32_t               ret;

   /* Get the KPI-group number */
   IFX_TAPI_KPI_CH_t nKpiGroup = KPI_GROUP_GET(nKpiChannel);
   /* Reject group values which are out of configured range or for groups
      which have not been configured yet. */
   if ((nKpiGroup == 0) || (nKpiGroup > IFX_TAPI_KPI_MAX_GROUP) ||
       (kpi_group[nKpiGroup-1] == IFX_NULL))
      return IFX_ERROR;
   /* strip the group from the channel parameter */
   nKpiChannel = KPI_CHANNEL_GET(nKpiChannel);
   /* reject channel values which are out of the configured range */
   if (nKpiChannel >= IFX_TAPI_KPI_MAX_CHANNEL_PER_GROUP)
      return IFX_ERROR;
   /* Adjust group values from channel notation to internal representation */
   nKpiGroup--;

   /* lookup the tapi channel associated with the kpi channel */
   pTapiCh = kpi_group[nKpiGroup]->channel_map[nKpiChannel];
   /* abort if no channel association exists */
   if (pTapiCh == IFX_NULL)
      return IFX_ERROR;

   /* protect the access to the bufferpool for wrappers */
   if (!TAPI_OS_IN_INTERRUPT())
      TAPI_OS_MutexGet (&semProtectWrapperBufferpool);
   /* global irq lock */
   TAPI_OS_LOCKINT(lock);
   /* allocate a wrapper struct from the bufferpool for wrappers */
   pElem = bufferPoolGet (wrapperBufferpool);
   /* global irq unlock */
   TAPI_OS_UNLOCKINT(lock);
   /* if ! in irq release protection semaphore */
   if (!TAPI_OS_IN_INTERRUPT())
      TAPI_OS_MutexRelease (&semProtectWrapperBufferpool);

   /* abort if buffer for the wrapping struct cannot be allocated */
   if (pElem == NULL)
   {
      IFX_TAPI_Stat_Add(pTapiCh,
         translateStreamTable[kpi_group[nKpiGroup]->stream_map[nKpiChannel]],
         TAPI_STAT_COUNTER_INGRESS_CONGESTED, 1);
      return IFX_ERROR;
   }

   /* The write access to the fifo is protected in two ways:
       First it is protected from concurrent reads by the ingress thread
       second it is protected from writing of new data in irq context. */

   /* store the tapi channel and pointer to the data buffer in the wrapper */
   pElem->pTapiCh = pTapiCh;
   pElem->nKpiCh = nKpiChannel;
   pElem->pBuf = pElem->pData = pPacket;
   pElem->nDataLength = nPacketLength;

   /* if ! in irq take protection semaphore */
   if (!TAPI_OS_IN_INTERRUPT())
      TAPI_OS_MutexGet (&kpi_group[nKpiGroup]->semProtectIngressFifo);

   /* here we are protected from changes from the ioctl so copy this data */
   pElem->nStream = kpi_group[nKpiGroup]->stream_map[nKpiChannel];

   /* global irq lock */
   TAPI_OS_LOCKINT(lock);
   /* store data to fifo */
   ret = fifoPut(kpi_group[nKpiGroup]->pIngressFifo, (IFX_void_t *)pElem, 0);
   /* free slots in the fifo for checking versus the threshold below */
   space = fifoSize(kpi_group[nKpiGroup]->pIngressFifo) -
           fifoElements(kpi_group[nKpiGroup]->pIngressFifo);
   /* global irq unlock */
   TAPI_OS_UNLOCKINT(lock);
   /* if ! in irq release protection semaphore */
   if (!TAPI_OS_IN_INTERRUPT())
      TAPI_OS_MutexRelease (&kpi_group[nKpiGroup]->semProtectIngressFifo);

   /* if putting to fifo succeeded set flag that there is data in one of
      the ingress fifos otherwise cleanup by releasing the wrapper buffer */
   if (ret == IFX_SUCCESS)
   {
#ifdef TAPI_PACKET_OWNID
      if (pElem->nStream == IFX_TAPI_KPI_STREAM_HDLC)
         IFX_TAPI_VoiceBufferChOwn (pPacket, IFX_TAPI_BUFFER_OWNER_HDLC_KPI);
      else if (pElem->nStream == IFX_TAPI_KPI_STREAM_COD)
         IFX_TAPI_VoiceBufferChOwn (pPacket, IFX_TAPI_BUFFER_OWNER_COD_KPI);
#endif /* TAPI_PACKET_OWNID */

#ifdef KPI_TASKLET
      if (TAPI_OS_IN_INTERRUPT() && !block_ingress_tasklet)
      {
         tasklet_hi_schedule(&tl_kpi_ingress);
      }
      else
#endif /* KPI_TASKLET */
      {
         /* signal the event that there is data in one of the ingress fifos */
         IFX_TAPI_KPI_ScheduleIngressHandling();
      }
      /* Wait for some free slots in the fifo before resetting the congestion
         indication. This stops a too fast oscillation which occurs when it is
         reset upon the first free slot. */
      if (space > IFX_TAPI_KPI_INGRESS_FIFO_SIZE / 2)
      {
         /* fifo state is: not congested */
         kpi_group[nKpiGroup]->bIngressFifoCongested = IFX_FALSE;
      }
   }
   else
   {
      /* zero bytes handled */
      nPacketLength = 0;

      /* This case happens if the ingress thread is not fast enough to read
         the data from the fifo and put it into the firmware data mailbox.
         Send an event to the application to notify that we were too slow. */
      /* Report congestion of the fifo to the application - but only once */
      if (kpi_group[nKpiGroup]->bIngressFifoCongested == IFX_FALSE) {
         IFX_TAPI_EVENT_t  tapiEvent;
         memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
         tapiEvent.id = IFX_TAPI_EVENT_KPI_INGRESS_FIFO_FULL;
         tapiEvent.module = IFX_TAPI_MODULE_TYPE_NONE;
         IFX_TAPI_Event_Dispatch(pTapiCh, &tapiEvent);
         /* fifo state is: congested */
         kpi_group[nKpiGroup]->bIngressFifoCongested = IFX_TRUE;
      }
      IFX_TAPI_Stat_Add(pTapiCh, translateStreamTable[pElem->nStream],
                        TAPI_STAT_COUNTER_INGRESS_CONGESTED, 1);
      /* protect the access to the bufferpool for wrappers */
      if (!TAPI_OS_IN_INTERRUPT())
         TAPI_OS_MutexGet (&semProtectWrapperBufferpool);
      /* global irq lock */
      TAPI_OS_LOCKINT(lock);
      /* release the wrapping structure for the data buffer */
      /* Note: ret is used again to reflect the result of this last put.
         The final return of the function is now either 0 or IFX_ERROR. */
      ret = bufferPoolPut(pElem);
      /* global irq unlock */
      TAPI_OS_UNLOCKINT(lock);
      /* if ! in irq release protection semaphore */
      if (!TAPI_OS_IN_INTERRUPT())
         TAPI_OS_MutexRelease (&semProtectWrapperBufferpool);
      /* warn if returning the wrapper buffer failed */
      if (ret != IFX_SUCCESS)
      {
         /* This should never happen! */
         TRACE (TAPI_DRV, DBG_LEVEL_HIGH, ("\nBuffer put-back error(4)\n"));
      }
      /* TRACE (TAPI_DRV, DBG_LEVEL_LOW,
                ("INFO: KPI-group 0x%X ingress fifo full\n", nKpiGroup+1)); */
   }

   /* taking in consideration size of IFX_int32_t casting below should be safe */
   return (ret == IFX_SUCCESS) ? (IFX_int32_t) nPacketLength : IFX_ERROR;
}


/** Report an event from KPI client to TAPI.

   \param  nKpiChannel  KPI channel the data is written to.
   \param  tapiEvent    Pointer to TAPI event to report.

   \remarks
   The function will overwrite the TAPI channel in the given event with the
   TAPI channel that is associated with the given KPI channel.
*/
extern IFX_void_t   IFX_TAPI_KPI_ReportEvent (IFX_TAPI_KPI_CH_t nKpiChannel,
                                              IFX_TAPI_EVENT_t *pTapiEvent)
{
   TAPI_CHANNEL             *pTapiCh;

   /* Get the KPI-group number */
   IFX_TAPI_KPI_CH_t nKpiGroup = KPI_GROUP_GET(nKpiChannel);
   /* Reject group values which are out of configured range or for groups
      which have not been configured yet. */
   if ((nKpiGroup == 0) || (nKpiGroup > IFX_TAPI_KPI_MAX_GROUP) ||
       (kpi_group[nKpiGroup-1] == IFX_NULL))
      return;
   /* strip the group from the channel parameter */
   nKpiChannel = KPI_CHANNEL_GET(nKpiChannel);
   /* reject channel values which are out of the configured range */
   if (nKpiChannel >= IFX_TAPI_KPI_MAX_CHANNEL_PER_GROUP)
      return;
   /* Adjust group values from channel notation to internal representation */
   nKpiGroup--;

   /* lookup the tapi channel associated with the kpi channel */
   pTapiCh = kpi_group[nKpiGroup]->channel_map[nKpiChannel];
   /* abort if no channel association exists */
   if (pTapiCh == IFX_NULL)
      return;

   IFX_TAPI_Event_Dispatch(pTapiCh, pTapiEvent);
}


/**
   Function to put a packet from irq context into an KPI egress fifo.

   \param  pChannel     Handle to TAPI_CHANNEL structure.
   \param  stream       Stream Type id.
   \param  *pPacket     Pointer to a bufferpool element with the data to be
                        written.
   \param  nPacketLength  Length of the data to be written.

   \return
   Return values are defined within the \ref IFX_return_t definition:
   - IFX_SUCCESS  if buffer was successfully sent
   - IFX_ERROR    if buffer was not sent.

   \remarks
   In case of error the caller still owns the buffer and has to take care to it.
*/
IFX_int32_t irq_IFX_TAPI_KPI_PutToEgress(TAPI_CHANNEL *pChannel,
                                          IFX_TAPI_KPI_STREAM_t stream,
                                          IFX_void_t *pPacket,
                                          IFX_uint32_t nPacketLength)
{
   IFX_TAPI_KPI_STREAM_SWITCH *pStreamSwitch = &pChannel->pKpiStream[stream];
   IFX_TAPI_KPI_FIFO_ELEM_t   *pElem;
   TAPI_OS_INTSTAT             lock;
   IFX_int32_t                 ret = IFX_ERROR;
   IFX_TAPI_KPI_CH_t           k_grp;

   /* protect fifo access and access to the bufferpool for wrappers from
      concurrent writes by other drivers in irq context by locking the
      irq globally. */
   TAPI_OS_LOCKINT(lock);

   /* get a buffer for the wrapping struct from the bufferpool */
   pElem = bufferPoolGet (wrapperBufferpool);

   if (pElem)
   {
      k_grp = KPI_GROUP_GET(pStreamSwitch->nKpiCh)-1;

      /* fill the wrapping struct with data */
      pElem->nKpiCh = pStreamSwitch->nKpiCh;
      pElem->pBuf = pElem->pData = pPacket;
      pElem->nDataLength = nPacketLength;

      /* store data to fifo */
      ret = fifoPut(pStreamSwitch->pEgressFifo, (IFX_void_t *)pElem, 0);

      /* signal the event that now there is data in the fifo
         Even if the put failed because the fifo is full trigger the
         processing to avoid a possible deadlock caused by the client. */
#ifdef KPI_TASKLET
      if ((kpi_group[k_grp]->pEgressTasklet) && (TAPI_OS_IN_INTERRUPT()))
      {
         tasklet_hi_schedule((struct tasklet_struct*) kpi_group[k_grp]->pEgressTasklet);
      }
      else
#endif /* KPI_TASKLET */
      {
         TAPI_OS_LockRelease (&semWaitOnEgressFifo[k_grp]);
      }

      if (ret == IFX_SUCCESS)
      {
         if (kpi_group[k_grp]->bEgressFifoCongested != IFX_FALSE)
         {
            IFX_uint32_t space;
            /* free slots in the egress fifo */
            space = fifoSize(pStreamSwitch->pEgressFifo) -
                    fifoElements(pStreamSwitch->pEgressFifo);
            if (space > IFX_TAPI_KPI_EGRESS_FIFO_SIZE / 2)
            {
               /* fifo state is: not congested */
               kpi_group[k_grp]->bEgressFifoCongested = IFX_FALSE;
            }
         }
      }
      else
      {
         /* if putting to fifo failed release wrapper buffer */
         if (bufferPoolPut(pElem) != IFX_SUCCESS)
         {
            /* This should never happen! */
            TRACE (TAPI_DRV, DBG_LEVEL_HIGH, ("\nBuffer put-back error(5)\n"));
         }
         /* This happens when the client for this KPI group fails to get
            the data from the fifo. When the caller discards the packet
            voice data is lost in this case. */
         TRACE (TAPI_DRV, DBG_LEVEL_HIGH,
                ("WARN: KPI-grp 0x%X egress fifo full\n", k_grp));

         if (kpi_group[k_grp]->bEgressFifoCongested == IFX_FALSE) {
            IFX_TAPI_EVENT_t  tapiEvent;
            memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
            tapiEvent.id = IFX_TAPI_EVENT_KPI_EGRESS_FIFO_FULL;
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_NONE;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
            /* fifo state is: congested */
            kpi_group[k_grp]->bEgressFifoCongested = IFX_TRUE;
         }
      }
   }

   /* global irq unlock */
   TAPI_OS_UNLOCKINT(lock);

   /* If putting to fifo failed IFX_ERROR is returned to the caller.
      The data buffer then still belongs to the caller who has to free it. */
   return (ret == IFX_SUCCESS) ? IFX_SUCCESS : IFX_ERROR;
}


#ifdef KPI_TESTLOOP
/**
   Testloop thread.

   This thread loops all data on KPI_GROUP1 from odd to even channel numbers
   and vice versa.

   \param  *pThread     Pointer to thread struct.

   \remarks
   This function runs as a thread in an endless loop until terminated.
   This thread is just meant for test purposes.
*/
IFX_int32_t ifx_tapi_KPI_TestloopThread (IFXOS_ThreadParams_t *pThread)
{
   IFX_TAPI_KPI_CH_t         channel;
   void                     *data;
   IFX_uint32_t              data_length;
   IFX_uint8_t               more_flag;
   IFX_int32_t               ret;

   TAPI_OS_THREAD_PRIORITY_MODIFY (TAPI_OS_THREAD_PRIO_HIGHEST);

   /* loop while we are not asked to terminate */
   while ((IFX_TAPI_KPI_WaitForData( IFX_TAPI_KPI_GROUP5 ) == IFX_SUCCESS) &&
          (pThread->bShutDown == IFX_FALSE))
   {

      ret = IFX_TAPI_KPI_ReadData( IFX_TAPI_KPI_GROUP5, &channel,
                                   &data, &data_length, &more_flag );
      if (ret < 0)
      {
         TRACE (TAPI_DRV, DBG_LEVEL_HIGH, ("LOOPBACK failed to read data\n"));
      }
      else
      {

         /* toggle the LSB in the channel number for sending */
         //For sending the same channel back..
         //channel = channel ^ 0x0001;
         ret = IFX_TAPI_KPI_WriteData( IFX_TAPI_KPI_GROUP5|channel, data, data_length );
         if (ret < 0)
         {
            /* On write error we still own the buffer so we have to handle it.
               Here we just drop the data. */
            IFX_TAPI_VoiceBufferPut(data);
            TRACE (TAPI_DRV, DBG_LEVEL_HIGH, ("LOOPBACK failed to write data\n"));
         }
      }
   }
   memset(&loopbackThread,0,sizeof(loopbackThread));
   printk(KERN_INFO "ifx_tapi_KPI_TestloopThread leaves\n");
   return IFX_SUCCESS;
}
#endif /* KPI_TESTLOOP */
	

/**
   Function to handle the KPI ingress direction

   Depending on the context it might be executed in a kernel thread or
   with Linux in a tasklet context

   \param foo        unused, but required for tasklets
   \return           void
*/
static IFX_void_t ifx_tapi_KPI_IngressHandler (IFX_ulong_t foo)
{
   IFX_TAPI_KPI_FIFO_ELEM_t *pElem;
   IFX_uint8_t               nThisGroup;
   TAPI_OS_INTSTAT           lock;
   IFX_int32_t               ret;
   /* Some KPI groups can not lost packages from FIFO while error
      occurred on LL side. Therefore that group has to be skipped
      in order not to block other groups handling. */
   IFX_boolean_t             bSkipGroup;

   IFX_UNUSED (foo);

   for (nThisGroup = 0; nThisGroup < IFX_TAPI_KPI_MAX_GROUP; nThisGroup++)
   {
#ifdef KPI_TASKLET
      /* don't handle HDLC group in tasklet mode */
      if (TAPI_OS_IN_INTERRUPT() && (nThisGroup == ((IFX_TAPI_KPI_HDLC>>12)-1)))
         continue;
#endif /* KPI_TASKLET */
      bSkipGroup = IFX_FALSE;

      /* if the group do not needed to skip and configured,
         check if there is data in the fifo of this group */
      while (bSkipGroup == IFX_FALSE &&
             kpi_group[nThisGroup] &&
             !fifoEmpty(kpi_group[nThisGroup]->pIngressFifo))
      {
         /* protect fifo get access from other tasks and any irq's
            Locking individual irq's is too costly so we lock globally */
         if (!TAPI_OS_IN_INTERRUPT())
            TAPI_OS_MutexGet (&kpi_group[nThisGroup]->semProtectIngressFifo);
         TAPI_OS_LOCKINT(lock);
         /* read element from fifo */
         pElem = fifoPeek(kpi_group[nThisGroup]->pIngressFifo, NULL);
         TAPI_OS_UNLOCKINT(lock);
         if (!TAPI_OS_IN_INTERRUPT())
            TAPI_OS_MutexRelease (&kpi_group[nThisGroup]->semProtectIngressFifo);

         if (pElem)
         {
            /* we got data from the fifo */
            TAPI_CHANNEL *pTapiCh = pElem->pTapiCh;
            IFX_TAPI_DRV_CTX_t *pDrvCtx = pTapiCh->pTapiDevice->pDevDrvCtx;
            IFX_TAPI_LL_CH_t* pCh = pTapiCh->pLLChannel;
            IFX_int32_t size = 0;

            /* write data to the mailbox using LL function */
            if (pDrvCtx->Write)
            {
               size = pDrvCtx->Write(pCh, pElem->pBuf, pElem->nDataLength,
                                     (IFX_int32_t*)IFX_NULL,
                                     translateStreamTable[pElem->nStream]);

               /* if forwarding to low level write routine failed... */
               if (size <= 0)
               {
                  if (pElem->nStream == IFX_TAPI_KPI_STREAM_HDLC)
                  {
                     /* leave element in FIFO and skip that group */
                     bSkipGroup = IFX_TRUE;
                     continue; /* while (!fifoEmpty) */
                  }

                  /* The mailbox congestion event was already sent by the
                     lower layer so no need to do it here again. */
                  /* writing to mailbox failed - discard data */
                  if (IFX_TAPI_VoiceBufferPut(pElem->pBuf) != IFX_SUCCESS)
                     TRACE (TAPI_DRV, DBG_LEVEL_HIGH,
                           ("\nBuffer put-back error(6a)\n"));
               }
            }
            else
            {
               TRACE(TAPI_DRV, DBG_LEVEL_LOW,
                     ("TAPI_DRV: LL-driver does not provide packet write\n"));
            }

            /* protect the access to the bufferpool for wrappers */
            if (!TAPI_OS_IN_INTERRUPT())
               TAPI_OS_MutexGet (&semProtectWrapperBufferpool);
            TAPI_OS_LOCKINT(lock);
            /* remove element from FIFO */
            pElem = fifoGet(kpi_group[nThisGroup]->pIngressFifo, NULL);
            /* return buffer with wrapper struct back to pool */
            ret = bufferPoolPut(pElem);
            TAPI_OS_UNLOCKINT(lock);
            if (!TAPI_OS_IN_INTERRUPT())
               TAPI_OS_MutexRelease (&semProtectWrapperBufferpool);
            /* warn if returning the wrapper buffer failed */
            if (ret != IFX_SUCCESS)
            {
               /* This should never happen! */
               TRACE (TAPI_DRV, DBG_LEVEL_HIGH,
                      ("\nBuffer put-back error(6b)\n"));
            }
         }
      }
   }
}


/**
   Function to be executed from a thread to serve all ingress fifos of all
   KPI groups.

   This function is started as a thread and runs in an endless loop.
   It sleeps until data in an ingress fifo is signalled with an event.
   Upon wakeup the ingress fifos are searched in a round-robin manner
   for data and the first data found is written to the FW downstream mailbox.
   This function returns only when the thread is explicitly terminated
   on cleanup.

   \param  *pThread     Pointer to thread parameters
   \return  IFX_SUCCESS
*/
static IFX_int32_t ifx_tapi_KPI_IngressThread (TAPI_OS_ThreadParams_t *pThread)
{
   /* acquire realtime priority. */
   TAPI_OS_THREAD_PRIORITY_MODIFY (TAPI_OS_THREAD_PRIO_HIGHEST);

   /* main loop is waiting for the event that data is available */
   while ((pThread->bShutDown == IFX_FALSE) &&
          (TAPI_OS_LOCK_GET_INTERRUPTIBLE (&semWaitOnIngressFifo) ==
                                                                   IFX_SUCCESS))
   {
      ifx_tapi_KPI_IngressHandler(0L);
   }

   return IFX_SUCCESS;
}


/**

*/
static IFX_return_t ifx_tapi_KPI_GroupInit(IFX_uint32_t nKpiGroup)
{
   /* if group has not yet been initialised create it now */
   if ((nKpiGroup > 0) && (kpi_group[nKpiGroup-1] == IFX_NULL))
   {
      kpi_group[nKpiGroup-1] = (IFX_TAPI_KPI_GROUP_t *)
                               TAPI_OS_Malloc (sizeof(IFX_TAPI_KPI_GROUP_t));
      if (kpi_group[nKpiGroup-1] != IFX_NULL)
      {
         /* set structure defined to zero */
         memset (&(*kpi_group[nKpiGroup-1]), 0x00,
                 sizeof(IFX_TAPI_KPI_GROUP_t));
         /* create semaphores for protecting the fifos */
         TAPI_OS_MutexInit (&kpi_group[nKpiGroup-1]->semProtectEgressFifo);
         TAPI_OS_MutexInit (&kpi_group[nKpiGroup-1]->semProtectIngressFifo);
         /* create data fifos for ingress and egress direction */
         kpi_group[nKpiGroup-1]->pEgressFifo =
            fifoInit (IFX_TAPI_KPI_EGRESS_FIFO_SIZE);
         kpi_group[nKpiGroup-1]->pIngressFifo =
            fifoInit (IFX_TAPI_KPI_INGRESS_FIFO_SIZE);
#ifdef KPI_TESTLOOP
         /* on group 5 start the LOOPBACK TEST thread */
         if (nKpiGroup == 5)
         {
            if (strcmp(loopbackThread.thrParams.pName,"kpi_loop")==0) {
               //iprintf("KPI_TestloopThread already there!!\n");
            }
            else {
            TAPI_OS_ThreadInit (&loopbackThread, "kpi_loop",
                              ifx_tapi_KPI_TestloopThread,
                               0, TAPI_OS_THREAD_PRIO_HIGHEST, 0, 0);
            }
         }
#endif /* KPI_TESTLOOP */
      }
      else
      {
         /* error: no memory for group struct */
         return IFX_ERROR;
      }
   }
   return IFX_SUCCESS;
}


/**
   Handler for the ioctl IFX_TAPI_KPI_CH_CFG_SET.

   This function sets the internal data structures to associate a
   TAPI packet stream with a KPI channel.
   If the group has never been used before the group structure is created
   and the group specific resources are allocated.

   \param  pChannel     Handle to TAPI_CHANNEL structure.
   \param  *pCfg        Pointer to \ref IFX_TAPI_KPI_CH_CFG_t containing the
                        configuration.

   \return
   Return values are defined within the \ref IFX_return_t definition
   - IFX_SUCCESS  if configuration was successfully set
   - IFX_ERROR    on invalid values in the configuration

   \remarks
   For testing a loopback thread is started on group 1 as soon as it is
   configured for the first time.
*/
IFX_int32_t IFX_TAPI_KPI_ChCfgSet (TAPI_CHANNEL *pChannel,
                                   IFX_TAPI_KPI_CH_CFG_t const *pCfg)
{
   IFX_TAPI_KPI_CH_t         nKpiGroup,
                             nKpiChannel;
   TAPI_OS_INTSTAT           lock;

   /* Get the KPI-group number */
   nKpiGroup = KPI_GROUP_GET(pCfg->nKpiCh);
   /* reject group values which are out of the configured range */
   if (nKpiGroup > IFX_TAPI_KPI_MAX_GROUP)
      return IFX_ERROR;
   /* strip the group from the channel parameter */
   nKpiChannel = KPI_CHANNEL_GET(pCfg->nKpiCh);
   /* reject channel values which are out of the configured range */
   if (nKpiChannel >= IFX_TAPI_KPI_MAX_CHANNEL_PER_GROUP)
      return IFX_ERROR;
   /* reject source stream identifiers which are out of range */
   if (pCfg->nStream >= IFX_TAPI_KPI_STREAM_MAX)
      return IFX_ERROR;

   /* make sure the group is initialized already s*/
   if (ifx_tapi_KPI_GroupInit (nKpiGroup) != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }

   /* Currently no flushing of fifos is done. To do this we had to lock global
      interrupts during this time. This is not desirable because it will take
      quite some time to release all the buffers possibly in the fifo.
      But because the data needed to deliver the packet is stored in the
      wrapping buffer when the packet is received we still get the packets
      delivered to the correct destination. */

   /* update the stream switch struct to indicate the new target */
   /* take protection semaphore */
   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);
   /* global irq lock */
   TAPI_OS_LOCKINT(lock);
   /* Set the stream switch struct to point to the new KPI channel.
      Group 0 is reserved for sending streams to the application. In this
      group force all channels to 0 to ease checks when using. */
   pChannel->pKpiStream[pCfg->nStream].nKpiCh =
      (nKpiGroup > 0) ? pCfg->nKpiCh : 0;
   pChannel->pKpiStream[pCfg->nStream].pEgressFifo =
      (nKpiGroup > 0) ? kpi_group[nKpiGroup-1]->pEgressFifo : IFX_NULL;
   /* global irq unlock */
   TAPI_OS_UNLOCKINT(lock);
   /* release protection semaphore */
   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);

   if (nKpiGroup > 0)
   {
      /* set tapi channel reference in kpi channel reference array */
      /* take protection semaphore */
      TAPI_OS_MutexGet (&kpi_group[nKpiGroup-1]->semProtectIngressFifo);
      /* global irq lock */
      TAPI_OS_LOCKINT(lock);
      kpi_group[nKpiGroup-1]->channel_map[nKpiChannel] = pChannel;
      kpi_group[nKpiGroup-1]->stream_map[nKpiChannel] = pCfg->nStream;
      /* global irq unlock */
      TAPI_OS_UNLOCKINT(lock);
      /* release protection semaphore */
      TAPI_OS_MutexRelease (&kpi_group[nKpiGroup-1]->semProtectIngressFifo);
   }

   return IFX_SUCCESS;
}


/**
   Handler for the ioctl IFX_TAPI_KPI_GRP_CFG_SET.

   This function configures the given KPI group. With this the size of the
   FIFOs can be set to individual values.
   If the group has never been used before the group structure is created
   and the group specific resources are allocated.

   NOTE: This function locks the interrupts for quite a long time and so may
   have a significant effect on system performance. It is recommended to use
   this function only then data transfer on the KPI group has stopped. For
   example during startup when nothing else is done in the sytem.

   \param  *pCfg        Pointer to \ref IFX_TAPI_KPI_GRP_CFG_t containing the
                        configuration.

   \return
   Return values are defined within the \ref IFX_return_t definition
   - IFX_SUCCESS  if configuration was successfully set
   - IFX_ERROR    on invalid values in the configuration
*/
IFX_int32_t IFX_TAPI_KPI_GrpCfgSet (IFX_TAPI_KPI_GRP_CFG_t const *pCfg)
{
   TAPI_OS_INTSTAT           lock;
   IFX_TAPI_KPI_FIFO_ELEM_t *pElem;
   IFX_TAPI_KPI_CH_t         nKpiGroup;
   IFX_uint8_t               egressSize,
                             ingressSize;

   /* Get the KPI-group number */
   nKpiGroup = KPI_GROUP_GET(pCfg->nKpiGroup);
   /* reject group values which are out of the configured range */
   if ((nKpiGroup == 0) || (nKpiGroup > IFX_TAPI_KPI_MAX_GROUP))
   {
      return IFX_ERROR;
   }

   /* when a size value of 0 is given the defaults will be used */
   egressSize  = (pCfg->nEgressFifoSize == 0) ?
                 IFX_TAPI_KPI_EGRESS_FIFO_SIZE : pCfg->nEgressFifoSize;
   ingressSize = (pCfg->nIngressFifoSize == 0) ?
                 IFX_TAPI_KPI_INGRESS_FIFO_SIZE : pCfg->nIngressFifoSize;

   /* make sure the group is already initialized */
   if (ifx_tapi_KPI_GroupInit (nKpiGroup) != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }

   if (!TAPI_OS_IN_INTERRUPT())
   {
      TAPI_OS_MutexGet (&kpi_group[nKpiGroup-1]->semProtectIngressFifo);
      TAPI_OS_MutexGet (&kpi_group[nKpiGroup-1]->semProtectEgressFifo);
      TAPI_OS_MutexGet (&semProtectWrapperBufferpool);
   }
   /* global irq lock */
   TAPI_OS_LOCKINT(lock);

   /* flush the data fifos for egress and ingress direction */
   while ((pElem = fifoGet (kpi_group[nKpiGroup-1]->pEgressFifo,
                            NULL)) != IFX_NULL)
   {
      if (IFX_TAPI_VoiceBufferPut(pElem->pBuf) != IFX_SUCCESS)
      {
         /* This should never happen! Warn but do not stop here. */
         TRACE (TAPI_DRV, DBG_LEVEL_HIGH, ("\nBuffer put-back error(7a)\n"));
      }

      if (bufferPoolPut(pElem) != IFX_SUCCESS)
      {
         /* This should never happen! Warn but do not stop here. */
         TRACE (TAPI_DRV, DBG_LEVEL_HIGH, ("\nBuffer put-back error(7b)\n"));
      }
   }
   while ((pElem = fifoGet(kpi_group[nKpiGroup-1]->pIngressFifo,
                           NULL)) != IFX_NULL)
   {
      if (IFX_TAPI_VoiceBufferPut(pElem->pBuf) != IFX_SUCCESS)
      {
         /* This should never happen! Warn but do not stop here. */
         TRACE (TAPI_DRV, DBG_LEVEL_HIGH, ("\nBuffer put-back error(8a)\n"));
      }

      if (bufferPoolPut(pElem) != IFX_SUCCESS)
      {
         /* This should never happen! Warn but do not stop here. */
         TRACE (TAPI_DRV, DBG_LEVEL_HIGH, ("\nBuffer put-back error(8b)\n"));
      }
   }

   /* delete data fifos for ingress and egress direction */
   fifoFree (kpi_group[nKpiGroup-1]->pEgressFifo);
   fifoFree (kpi_group[nKpiGroup-1]->pIngressFifo);

   /* recreate the fifos with the new size */
   kpi_group[nKpiGroup-1]->pEgressFifo = fifoInit(egressSize);
   kpi_group[nKpiGroup-1]->pIngressFifo = fifoInit(ingressSize);

   /* global irq unlock */
   TAPI_OS_UNLOCKINT(lock);

   if (!TAPI_OS_IN_INTERRUPT())
   {
      TAPI_OS_MutexRelease (&semProtectWrapperBufferpool);
      TAPI_OS_MutexRelease (&kpi_group[nKpiGroup-1]->semProtectEgressFifo);
      TAPI_OS_MutexRelease (&kpi_group[nKpiGroup-1]->semProtectIngressFifo);
   }

   return IFX_SUCCESS;
}


/**
   Retrieve the KPI Channel number of a given stream on a given TAPI Channel
   \param  pChannel     Handle to TAPI_CHANNEL structure.
   \param  stream       Stream Type id.

   \return KPI Channel number
*/
IFX_TAPI_KPI_CH_t IFX_TAPI_KPI_ChGet(TAPI_CHANNEL *pChannel,
                                     IFX_TAPI_KPI_STREAM_t stream)
{
   return pChannel->pKpiStream[stream].nKpiCh;
}


/** optionally: the KPI client might register a pointer to
                an egress tasklet (Linux) structure to its group
   \param nKpiGroup        KPI Group to register the tasklet to
   \param pEgressTasklet   void pointer to a (Linux) tasklet_struct

   \return void
*/
IFX_return_t IFX_TAPI_KPI_EgressTaskletRegister (IFX_TAPI_KPI_CH_t nKpiGroup,
                                                 IFX_void_t *pEgressTasklet )
{
#ifdef KPI_TASKLET
   nKpiGroup = KPI_GROUP_GET(nKpiGroup);

   if ((nKpiGroup == 0) || (nKpiGroup > IFX_TAPI_KPI_MAX_GROUP))
   {
      return IFX_ERROR;
   }

   /* make sure the group is already initialized */
   if (ifx_tapi_KPI_GroupInit (nKpiGroup) != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }

   kpi_group[nKpiGroup-1]->pEgressTasklet = pEgressTasklet;
#else /* KPI_TASKLET */
   IFX_UNUSED(nKpiGroup);
   IFX_UNUSED(pEgressTasklet);
#endif /* KPI_TASKLET */
   return IFX_SUCCESS;
}

#endif /* (TAPI_CFG_FEATURES & TAPI_FEAT_KPI) */
