


#include <stdio.h>
#include "Field.h"
#include "Qc98xxDevice.h"
#include "Qc98xxField.h"
#include "peregrine_reg_map_ART_template.h"
#include "peregrine_2p0_reg_map_ART_template.h"

void Qc98xxFieldSelect()
{
    if (Qc98xxIsVersion1())
    {
	    FieldSelect(_F,sizeof(_F)/sizeof(_F[0]));
    }
    else
    {
	    FieldSelect(_F_peregrine_2p0,sizeof(_F_peregrine_2p0)/sizeof(_F_peregrine_2p0[0]));
    }
}


