
#include "opt_ah.h"
#include "wlantype.h"
#include "athreg.h"
#include "Field.h"
#include "Ar9287Field.h"

#if AH_SUPPORT_KIWI

static struct _Field _F[2859];
static ATHEROS_REG_FILE kiwi1_2[] = {
#include "dk_002e_2.ini"
};

void Ar9287_FieldSelect(void)
{
    int kiwi1_2_size = sizeof(kiwi1_2)/sizeof(kiwi1_2[0]);
    int i;
    for (i=0; i<kiwi1_2_size; i++ )
    {
        _F[i].registerName = kiwi1_2[i].regName;
        _F[i].fieldName = kiwi1_2[i].fieldName;
        _F[i].address = (unsigned long )kiwi1_2[i].regOffset;
        _F[i].low =  (char)kiwi1_2[i].fieldStartBitPos + kiwi1_2[i].fieldSize - 1;
        _F[i].high = (char)kiwi1_2[i].fieldStartBitPos;
    }
    
    FieldSelect(_F,kiwi1_2_size);
}
#else
void Ar9287_FieldSelect(void)
{
	return;
}
#endif
//set the registers for the selected rx chain mask
int Ar9287RxChainSet(int rxChain)
{
    if(rxChain == 0x5) {
        FieldWrite("ANALOG_SWP.bb_swap_alt_chn", 0x1); 
    }
 
    FieldWrite("MULTCHN_EN.bb_rx_chain_mask", rxChain & 0x7);
    FieldWrite("CAL_MASK.bb_cal_chain_mask", rxChain & 0x7);
    return 1;
}
int Ar9287froce_dac_gain(unsigned long *value)
{
    FieldRead("bb_forced_dac_gain", value);	
    return 1;
}
