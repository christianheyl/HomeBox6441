#ifndef _LINK_RX_STAT_H_
#define _LINK_RX_STAT_H_

extern void LinkRxStatClear();

extern void LinkRxStatExtractDut(void *rxStatus, int rate);

extern void LinkRxStatFinish();
extern struct rxStats *Ar6KLinkRxStatFetch(int rate);

#endif //_LINK_RX_STAT_H_