/* dk_irq.c - this file contains the interrupt handler for the dk driver */

#include <asm/ptrace.h>
#include <linux/kernel.h>

#include "dk.h"
#include "client.h"
#include "dk_event.h"

BOOLEAN dk_intr_handler
(
 	INT32 irq,
	VOID *param,
	struct pt_regs *regs
)
{
	p_event_struct p_event;
	p_event_struct p_event_copy;
	p_event_struct p_event_to_push;
	p_event_queue p_isr_event_q;
	p_event_queue p_trigered_event_q;
	UINT32 priIsr;
	UINT32 secIsr[5];
	UINT32 i;
	BOOLEAN bValidEvent;
	p_atheros_dev p_client;
	UINT32 cli_id;
#ifndef AP83
	INT32 reg_val;
        UINT32 macRev;
#endif

	p_client = (p_atheros_dev)param;
	cli_id = p_client->cli_id;
#ifdef AP83
       cli_reg_write(cli_id,0x0024,0x0); // disabling INT register..
                /* Read the isr */
                cli_reg_read(cli_id,0x00c0,&priIsr);
               if(!priIsr) {
                        //this might be first gen
                        cli_reg_read(cli_id, 0x1c,&priIsr);
                }

#ifdef DK_DEBUG
           printk("DK::Got interrupt:ISR = %x \n",priIsr);
#endif

                cli_reg_read(cli_id,0x00c4,&secIsr[0]);
                cli_reg_read(cli_id,0x00c8,&secIsr[1]);
                cli_reg_read(cli_id,0x00cc,&secIsr[2]);
                cli_reg_read(cli_id,0x00d0,&secIsr[3]);
                cli_reg_read(cli_id,0x00d4,&secIsr[4]);

//        }


        // catch spurios interrupts that comes before the client is registered
        if (!BUSY_CLIENT(p_client)) return FALSE;

        // start to scan the current queue of events
        p_isr_event_q = &(p_client->isr_event_q);
        p_trigered_event_q = &(p_client->trigered_event_q);
        p_event = p_isr_event_q->pHead;

        while ( p_event ) {

                bValidEvent = FALSE;
                switch ( p_event->type ) {
                        case ISR_INTERRUPT: {
                            if (priIsr & p_event->param1 ) {
                                        // return the complete ISR value
                                        // not anded with the mask
                                        p_event->result[0] = priIsr;
                                        for (i=0;i<5;i++) {
                                                p_event->result[1+i] = secIsr[i];
                                        }
                                bValidEvent    = TRUE;
                             }
                             break;
                        }
                        default:
                                printk("DK::Ilegal event type found in ISR event queue!\n");
                                break;
                } // switch

                if ( !bValidEvent ) {
                        p_event = p_event->pNext;
                        continue;
                }

                // put the event in the event triggered Q,
                // either move or copy event, depending
                // on the persistent flag
                if ( p_event->persistent ) {
                        p_event_copy = copyEvent(p_event);
                        if( !p_event_copy ) {
                                printk("DK::Unable to copy event in interrupt\n");
                                break;
                        }
                        p_event_to_push = p_event_copy;
                } else {
                        if (!removeEvent(p_event, p_isr_event_q, FALSE) ) {
                                printk("DK::Unable to remove event from ISR queue \n");
                                break;
                        }
                        p_event_to_push = p_event;
                }

                // push the event onto the triggered queue
                if (!pushEvent(p_event_to_push, p_trigered_event_q,FALSE) ) {
                        printk("DK::Unable to push event onto triggered queue\n");
                        break;
                }

                // increment to next event
                p_event = p_event->pNext;
        } // while ( p_event )

        return TRUE;

#else  // end AP83

        if (p_client->device_class == SIMPLE_COMM_CLASS) {
	    cli_reg_read(cli_id, 0x8, &priIsr);
#ifdef DK_DEBUG
	    printk("DK::Got interrupt from simple comm class \n");
#endif
	}
	
	if (p_client->device_class == NETWORK_CLASS) {
	   /* Read the interrupt pending register */
	   
                cli_reg_read(cli_id, 0x4020, &macRev); 
	        printk("macRev is 0x%x  \n", macRev);
                if (((macRev >= 0xc0) && (macRev <= 0xdf)) || ((macRev & 0xff) == 0xff)) {
                    UINT32 h_intr_enable;
                    UINT32 h_intr_cause;

                    cli_reg_read(cli_id, 0x403c,&h_intr_enable); // async intr enable
                    cli_reg_read(cli_id, 0x4038,&h_intr_cause); // async intr cause
                    if (!(h_intr_enable & h_intr_cause))  return FALSE;
                }
                else  {
                    cli_reg_read(cli_id,0x4008,&reg_val);
     	            if ((reg_val & 0x00000001) == 0x0) return FALSE;
                }
 	
	        /* Read the isr */
	        cli_reg_read(cli_id,0x00c0,&priIsr);
               if(!priIsr) {
                        //this might be first gen
                        cli_reg_read(cli_id, 0x1c,&priIsr);
                }

#ifdef DK_DEBUG
	   printk("DK::Got interrupt:ISR = %x \n",priIsr);
#endif

	        cli_reg_read(cli_id,0x00c4,&secIsr[0]);
	        cli_reg_read(cli_id,0x00c8,&secIsr[1]);
	        cli_reg_read(cli_id,0x00cc,&secIsr[2]);
	        cli_reg_read(cli_id,0x00d0,&secIsr[3]);
	        cli_reg_read(cli_id,0x00d4,&secIsr[4]);
	
	}
    
	// catch spurios interrupts that comes before the client is registered
	if (!BUSY_CLIENT(p_client)) return FALSE; 
	
	// start to scan the current queue of events
	p_isr_event_q = &(p_client->isr_event_q);
	p_trigered_event_q = &(p_client->trigered_event_q);
	p_event = p_isr_event_q->pHead;

	while ( p_event ) {

		bValidEvent = FALSE;
		switch ( p_event->type ) {
			case ISR_INTERRUPT: {

	                    if (priIsr & p_event->param1 ) {
					// return the complete ISR value
					// not anded with the mask
					p_event->result[0] = priIsr;
					for (i=0;i<5;i++) {
						p_event->result[1+i] = secIsr[i];
					}
				bValidEvent    = TRUE;
			     }
	                     break;
                        }
			default:
				printk("DK::Ilegal event type found in ISR event queue!\n"); 
				break;
		} // switch

		if ( !bValidEvent ) {
			p_event = p_event->pNext;
			continue;
		}

		// put the event in the event triggered Q,
		// either move or copy event, depending
		// on the persistent flag
		if ( p_event->persistent ) {
			p_event_copy = copyEvent(p_event);
			if( !p_event_copy ) {
				printk("DK::Unable to copy event in interrupt\n");
				break;
			}
			p_event_to_push = p_event_copy;
		} else {
			if (!removeEvent(p_event, p_isr_event_q, FALSE) ) {
				printk("DK::Unable to remove event from ISR queue \n");
				break;
			}
			p_event_to_push = p_event;
		}

		// push the event onto the triggered queue
		if (!pushEvent(p_event_to_push, p_trigered_event_q,FALSE) ) {
			printk("DK::Unable to push event onto triggered queue\n");
			break;
		}

		// increment to next event
		p_event = p_event->pNext;
	} // while ( p_event )
		
	return TRUE;
#endif
}
