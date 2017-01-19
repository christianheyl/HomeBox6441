#ifndef _RF_BB_TEST_POINT_H_
#define _RF_BB_TEST_POINT_H_

extern DEVICEDLLSPEC void RfBbTestPointStart(int frequency, int ht40, int bandwidth, int antennapair, unsigned char chainnum,
                       int mbgain, int rfgain, int coex, int sharedrx, int switchtable, unsigned char AnaOutEn,
                       int (*ison)(), int (*done)());

#endif //_RF_BB_TEST_POINT_H_