#include "wave400_defs.h"
#include "wave400_cnfg.h"
#include "wave400_chipreg.h"

void MT_WrReg(MT_UINT32 unit, MT_UINT32 reg, MT_UINT32 data )
{
	NONE_TRUNK_REG(unit, reg)=data;
}

MT_UINT32 MT_RdReg(MT_UINT32 unit, MT_UINT32 reg)
{
	MT_UINT32 data;
	data = NONE_TRUNK_REG(unit, reg);
	return(data);
}

void MT_WrRegMask(MT_UINT32 unit, MT_UINT32 reg, MT_UINT32 mask,MT_UINT32 data )
{
		MT_UINT32 readVal,setData;	
	
		readVal=MT_RdReg(unit,reg);
		setData= (readVal & (~mask))|data;
		MT_WrReg(unit,reg, setData );
}

