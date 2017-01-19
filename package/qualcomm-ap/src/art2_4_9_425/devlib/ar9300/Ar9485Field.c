#include "opt_ah.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "Field.h"
#include "Ar9300Field.h"

#include "poseidon_reg_map_art_template.h"

void Ar9485FieldSelect()
{
	FieldSelect(_F,sizeof(_F)/sizeof(_F[0]));
}


