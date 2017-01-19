#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "ah_devid.h"
#include "ah_internal.h"
#include "ar9300reg.h"
#include "Field.h"
#include "Ar9300Field.h"

#include "Ar9300_TuningCaps.h"
#include "AnwiDriverInterface.h"

int Ar9300_TuningCaps(int caps)
{
	FieldWrite("ch0_XTAL.xtal_capindac", caps);
	FieldWrite("ch0_XTAL.xtal_capoutdac", caps);
	return 0;
}
