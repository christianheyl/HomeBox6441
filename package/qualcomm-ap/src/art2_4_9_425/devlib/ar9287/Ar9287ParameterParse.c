#include <string.h>
#include "ParameterConfigDef.h"


void Ar9287_calTGTpwrDataRateTitle(int iMode, unsigned char *title)
{
	if (iMode==legacy_CCK) 
		strcpy(title, "1,2,5.5,11:");
	else if (iMode==HT20 || iMode==HT40) 
		strcpy(title, "MCS0,MCS1,MCS2,MCS3,MCS4,MCS5,MCS6,MCS7:");
	else
		strcpy(title, "r6_24,r36,r48,r54:");

}