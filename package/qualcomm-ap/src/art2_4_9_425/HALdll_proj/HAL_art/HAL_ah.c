#include <string.h>
#include <math.h>
#include <ctype.h>

#include "wlantype.h"
//#include "NewArt.h"
#include "smatch.h"
#include "ErrorPrint.h"
#include "NartError.h"

#include "ah.h"
#include "ah_internal.h"


#define MBUFFER 1024

int ah_N2DBM(int _x, int _y)
{
	return N2DBM(_x,_y);
}