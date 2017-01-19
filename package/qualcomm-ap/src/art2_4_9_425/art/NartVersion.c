



#include "NartVersion.h"

static int _Major=4;

static int _Minor=4;

static char _Date[7]="120314";

static char _Time[7]="130000";



FIELDDLLSPEC int NartVersionMajor()
{
    return _Major;
}

FIELDDLLSPEC int NartVersionMinor()
{
    return _Minor;
}

FIELDDLLSPEC char* NartVersionDate()
{
    return _Date;
}

FIELDDLLSPEC char* NartVersionTime()
{
    return _Time;
}
