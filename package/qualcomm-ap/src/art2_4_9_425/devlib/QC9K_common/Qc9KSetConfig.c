#include <stdio.h>
#include <string.h>

#include "wlantype.h"
#include "DevSetConfig.h"

int Qc9KSetConfigCommand(void *cmd)
{
	// Init configSetup structure if any
    //strcpy(configSetup.machName, "SDIO");
#ifdef _WINDOWS
	if (configSetup.boardDataPath[0] == '\0')
	{
		strcpy(configSetup.boardDataPath, "..\\driver\\BoardData\\");
	}
#endif //_WINDOWS
    return DevSetConfigCommand(cmd);
}
