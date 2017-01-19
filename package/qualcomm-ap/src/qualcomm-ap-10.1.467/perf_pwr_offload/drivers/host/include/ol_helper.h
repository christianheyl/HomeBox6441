struct cookie {

    wbuf_t                 PacketContext;    /* Must be first field */

    HTC_PACKET             HtcPkt;       /* HTC packet wrapper */

    struct cookie *arc_list_next;

};



void ol_cookie_init(void *ar);

void ol_cookie_cleanup(void *ar);

void ol_free_cookie(void *ar, struct cookie * cookie);

struct cookie *ol_alloc_cookie(void  *ar);









    

