#ifndef _INC_CHANNEL_H
#define _INC_CHANNEL_H

/* channel_flags */
// these were copied from hal,we should define our own
#define CHANNEL_CW_INT      0x00000002 /* CW interference detected on channel */
#define CHANNEL_TURBO       0x00000010 /* Turbo Channel */
#define CHANNEL_CCK         0x00000020 /* CCK channel */
#define CHANNEL_OFDM        0x00000040 /* OFDM channel */
#define CHANNEL_2GHZ        0x00000080 /* 2 GHz spectrum channel. */
#define CHANNEL_5GHZ        0x00000100 /* 5 GHz spectrum channel */
#define CHANNEL_PASSIVE     0x00000200 /* Only passive scan allowed in the channel */
#define CHANNEL_DYN         0x00000400 /* dynamic CCK-OFDM channel */
#define CHANNEL_XR          0x00000800 /* XR channel */
#define CHANNEL_STURBO      0x00002000 /* Static turbo, no 11a-only usage */
#define CHANNEL_HALF        0x00004000 /* Half rate channel */
#define CHANNEL_QUARTER     0x00008000 /* Quarter rate channel */
#define CHANNEL_HT20        0x00010000 /* HT20 channel */
#define CHANNEL_HT40PLUS    0x00020000 /* HT40 channel with extention channel above */
#define CHANNEL_HT40MINUS   0x00040000 /* HT40 channel with extention channel below */
#define CHANNEL_VHT20       0x00080000 /* HT20 channel */
#define CHANNEL_VHT40PLUS   0x00100000 /* HT40 channel with extention channel above */
#define CHANNEL_VHT40MINUS  0x00200000 /* HT40 channel with extention channel below */
#define CHANNEL_VHT80_0     0x00400000 /* VHT80 channel with primary channel below */
#define CHANNEL_VHT80_1     0x00800000 /* VHT80 channel with primary channel below */
#define CHANNEL_VHT80_2     0x01000000 /* VHT80 channel with primary channel above */
#define CHANNEL_VHT80_3     0x02000000 /* VHT80 channel with primary channel above */

//
// look though the channel list and return the first channel that matches.
// frequency<=0 matches anything
//
extern int ChannelFind(int fmin, int fmax, int bw, int *rfrequency, int *rbw);

extern int ChannelCalculate();

extern int ChannelCommand(int client);

#endif //_INC_CHANNEL_H