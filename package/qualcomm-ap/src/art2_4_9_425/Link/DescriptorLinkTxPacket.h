


#define MAGGREGATE 64


extern int LinkTxLoopBuffer(int aggregate);


extern int LinkTxLoopBufferSize(int aggregate);


extern int LinkTxBarBuffer();


extern int LinkTxBarBufferSize();


//
// creates a block acknowledgement request (BAR) 802.11 message.
// return the size of the message
//
extern int LinkTxBarCreate(unsigned char *source, unsigned char *destination);



extern int LinkTxAggregatePacketCreate(
    unsigned int dataBodyLength,
    unsigned char *dataPattern,
    unsigned int dataPatternLength,
    unsigned char *bssid, unsigned char *source, unsigned char *destination,
	int specialTx100Pkt,
    int wepEnable,
	int agg, int bar);


extern int LinkTxPacketCreate(
    unsigned int dataBodyLength,
    unsigned char *dataPattern,
    unsigned int dataPatternLength,
    unsigned char *bssid, unsigned char *source, unsigned char *destination,
	int specialTx100Pkt,
    int wepEnable);
