#ifndef _AR6K_LINK_TX_STAT_H_
#define _AR6K_LINK_TX_STAT_H_

extern void LinkTxStatFinish();
extern void LinkTxStatClear();

extern struct txStats *Ar6KLinkTxStatFetch(int rate);
extern void Ar6KLinkTxStatExtract(TX_STATS_STRUCT_UTF *txStatus_utf, int rate);
extern int Ar6KLinkTxStatTemperatureGet();

#endif //_AR6K_LINK_TX_STAT_H_