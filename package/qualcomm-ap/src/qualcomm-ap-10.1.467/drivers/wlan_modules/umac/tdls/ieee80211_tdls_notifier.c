/*
 *  Copyright (c) 2011 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

#include <ieee80211_var.h>
#include <ieee80211.h>

#if UMAC_SUPPORT_TDLS

#include "ieee80211_tdls_notifier.h"

int
ieee80211_tdls_register_notification_handler(
    ieee80211_tdls_notifier             *notifier,
    ieee80211_tdls_notification_handler handler,
    void                                *arg)
{
    int    i;

    spin_lock(&notifier->lock);

    /*
     * Verify that event handler is not yet registered.
     */
    for (i = 0; i < IEEE80211_MAX_TDLS_NOTIFICATION_HANDLERS; ++i) {
        if ((notifier->handlers[i] == handler) &&
            (notifier->arg[i]      == arg)) {
            printk("%s: handler=%p arg=%p already registered\n",
                __func__, handler, arg);

            spin_unlock(&notifier->lock);

            return -EEXIST;    /* already exists */
        }
    }

    /*
     * Verify that the list of event handlers is not full.
     */
    ASSERT(notifier->num_handlers < IEEE80211_MAX_TDLS_NOTIFICATION_HANDLERS);
    
    if (notifier->num_handlers >= IEEE80211_MAX_TDLS_NOTIFICATION_HANDLERS) {
        printk("%s: ERROR: No more space: handler=%p arg=%p\n",
            __func__, handler, arg);

        spin_unlock(&notifier->lock);

        return -ENOSPC;
    }

    /*
     * Add new handler and increment number of registered handlers.
     * Registered event handlers are guaranteed to occupy entries 0..(n-1) in
     * the table, so we can safely assume entry 'n' is available.
     */
    notifier->handlers[notifier->num_handlers] = handler;
    notifier->arg[notifier->num_handlers++]    = arg;

    printk("%s: registered handler=%p arg=%p index=%d\n",
        __func__, handler, arg, notifier->num_handlers);

    spin_unlock(&notifier->lock);

    return EOK;
}

int
ieee80211_tdls_unregister_notification_handler(
    ieee80211_tdls_notifier             *notifier,
    ieee80211_tdls_notification_handler handler,
    void                                *arg)
{
    int    i;

    spin_lock(&notifier->lock);
    for (i = 0; i < IEEE80211_MAX_TDLS_NOTIFICATION_HANDLERS; ++i) {
        if ((notifier->handlers[i] == handler) &&
            (notifier->arg[i]      == arg)) {

            printk("%s: unregistered handler=%p arg=%p index=%d\n",
                __func__, handler, arg, i);

            /*
             * Replace event handler being deleted with the last one in the list
             */
            notifier->handlers[i] = notifier->handlers[notifier->num_handlers - 1];
            notifier->arg[i]      = notifier->arg[notifier->num_handlers - 1];

            /*
             * Clear last event handler in the list
             */
            notifier->handlers[notifier->num_handlers - 1] = NULL;
            notifier->arg[notifier->num_handlers - 1]      = NULL;
            notifier->num_handlers--;

            spin_unlock(&notifier->lock);

            return EOK;
        }
    }
    spin_unlock(&notifier->lock);

    printk("%s: Failed to unregister handler=%p arg=%p\n",
        __func__, handler, arg);

    return -ENXIO;
}

void
notifier_post_event(
    struct ieee80211_node   *ni,
    ieee80211_tdls_notifier *source_notifier,
    u_int16_t               notification_type,
    u_int16_t               notification_data_len,
    void                    *notification_data)
{
    ieee80211_tdls_notifier    local_notifier;
    int                        i;

    printk("%s: type=%d data_len=%d data=%p\n",
        __func__, notification_type, notification_data_len, notification_data);

    spin_lock(&source_notifier->lock);

    /*
     * make a local copy of event handlers list to avoid
     * the call back modifying the list while we are traversing it.
     */
    local_notifier = *source_notifier;

    for (i = 0; i < local_notifier.num_handlers; ++i) {
        if ((local_notifier.handlers[i] != source_notifier->handlers[i]) ||
            (local_notifier.arg[i]      != source_notifier->arg[i]))
        {
            /*
             * There is a change in the event list.
             * Traverse the original list to see this event is still valid.
             */
            int     k;
            bool    found = false;

            for (k = 0; k < source_notifier->num_handlers; k++) {
                if ((local_notifier.handlers[i] == source_notifier->handlers[k]) &&
                    (local_notifier.arg[i]      == source_notifier->arg[k]))
                {
                    /* Found a match */
                    found = true;
                    break;
                }
            }

            if (!found) {
                /* Did not find a match. Skip this event call back. */
                printk("%s: Skip event handler since it is unreg. Type=%d. cb=%p, arg=%p\n",
                    __func__, notification_type, local_notifier.handlers[i], local_notifier.arg[i]);

                continue;
            }
        }

        if (local_notifier.handlers[i] == NULL) {
            printk("%s: local_notifier.handlers[%d]==NULL arg=%p local_notifier.num_handlers=%d/%d\n",
                __func__,
                i, local_notifier.arg[i],
                local_notifier.num_handlers, source_notifier->num_handlers);

            continue;
        }

        /* Calling the event handler without the lock */
        spin_unlock(&source_notifier->lock);

        (local_notifier.handlers[i])(
                ni,
                notification_type,
                notification_data_len,
                notification_data,
                local_notifier.arg[i]);

        /* Reacquire lock to check next event handler */
        spin_lock(&source_notifier->lock);
    }

    /* Calling the event handler without the lock */
    spin_unlock(&source_notifier->lock);
}

#endif /* UMAC_SUPPORT_TDLS */
