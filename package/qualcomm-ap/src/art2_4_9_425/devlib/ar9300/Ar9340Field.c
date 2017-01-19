

#include "opt_ah.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

//#if AH_SUPPORT_WASP
#include "Field.h"
#include "Ar9300Field.h"
//#include "osprey_reg_map_ART_template.h"
#include "wasp_reg_map_ART_template.h"
void Ar9340FieldSelect()
{
	FieldSelect(_F,sizeof(_F)/sizeof(_F[0]));
}
/*
#else
void Ar9340FieldSelect()
{
	return;
}
#endif
*/
