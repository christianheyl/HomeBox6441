

#include <stdio.h>
#include <stdlib.h>

#include "Field.h"
#include "ParameterSelect.h"
#include "Card.h"

#include "AquilaNewmaMapping.h"

#include "wlantype.h"

#ifndef MDK_AP
#include "osdep.h"
#endif

#include "mEepStruct9300.h"
#include "Ar9300CalibrationApply.h"


//
// hal header files
//
#include "ah.h"
#include "ah_internal.h"
#include "ar9300eep.h"

#include "Ar9300Device.h"


#define MCHAIN 3



//
// Stuff calibration number into chip
//
AR9300DLLSPEC int Ar9300PowerControlOverride(int frequency, int *correction, int *voltage, int *temperature)
{
	ar9300_power_control_override(AH, frequency, correction, voltage, temperature);
    return 0;
}



