#ifndef _LINK_STAT_H_
#define _LINK_STAT_H_

#define MRETRY 16
#define MSTREAM 3
#define MEVM 100
#define MCHAIN 3
#define MRSSI 100
#define MRATE 300

typedef struct txStats 
{
	int goodPackets;
	int underruns;
	int otherError;
	int excessiveRetries;
	//
	// retry histogram
	//
	int shortRetry[MRETRY];
	int longRetry[MRETRY];

	int newThroughput;
	int startTime;
	int endTime;
	int byteCount;
    int dontCount;
	//
	// rssi histogram for good packets
	//
	int rssimin;
	int rssi[MRSSI];
	int rssic[MCHAIN][MRSSI];
	int rssie[MCHAIN][MRSSI];
	//
	// evm histogram for good packets
	//
	int evm[MSTREAM][MEVM];
	//
	// rssi histogram for bad packets
	//
	int badrssi[MRSSI];
	int badrssic[MCHAIN][MRSSI];
	int badrssie[MCHAIN][MRSSI];
	//
	// evm histogram for bad packets
	//
	int badevm[MSTREAM][MEVM];

    int temperature;

} TX_STATS_STRUCT;


typedef struct rxStats 
{
	int goodPackets;
	int otherError;
	int crcPackets;
	int singleDups;
	int multipleDups;
	int bitMiscompares;
    int bitErrorCompares;
	int ppmMin;
	int ppmMax;
	int ppmAvg;
    int decrypErrors;

	// Added for RX tput calculation
	int rxThroughPut;
	int startTime;
	int endTime;
	int byteCount;
    int dontCount;
	//
	// rssi histogram for good packets
	//
	int rssimin;
	int rssi[MRSSI];
	int rssic[MCHAIN][MRSSI];
	int rssie[MCHAIN][MRSSI];
	//
	// evm histogram for good packets
	//
	int evm[MSTREAM][MEVM];
	//
	// rssi histogram for bad packets
	//
	int badrssi[MRSSI];
	int badrssic[MCHAIN][MRSSI];
	int badrssie[MCHAIN][MRSSI];
	//
	// evm histogram for bad packets
	//
	int badevm[MSTREAM][MEVM];

	unsigned int Chain0AntSel[2];
	unsigned int Chain1AntSel[2];
	unsigned int Chain0AntReq[2];
	unsigned int Chain1AntReq[2];
	unsigned int ChainStrong[2];

} RX_STATS_STRUCT;

typedef struct rxSpurStats
{
    double freq[56];
    double spurLevel[56];

} RX_SPUR_STATS_STRUCT;

#endif //_LINK_STAT_H_

