

//
// unload the dll
//
extern void DeviceUnload();


//
// loads the device control dll 
//
extern int DeviceLoad(char *dllname);


//
// returns name of dll file
//
extern char *DeviceFullName(void);

