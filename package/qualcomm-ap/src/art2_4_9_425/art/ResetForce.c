



#include "ResetForce.h"

static int _ResetForce=0;


FIELDDLLSPEC void ResetForce()
{
	_ResetForce=1;
}


FIELDDLLSPEC void ResetForceClear()
{
	_ResetForce=0;
}


FIELDDLLSPEC int ResetForceGet()
{
	return _ResetForce;
}
