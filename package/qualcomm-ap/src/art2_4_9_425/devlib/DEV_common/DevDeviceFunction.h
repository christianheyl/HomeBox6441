
#ifndef _DEV_DEVICE_FUNCTION_H_
#define _DEV_DEVICE_FUNCTION_H_

struct _DevDeviceFunction
{
	int (*RegulatoryDomainGet)(void);
	int (*RegulatoryDomain1Get)(void);
	int (*OpFlagsGet)(void);
	int (*OpFlags2Get)(void);
	int (*Is4p9GHz)(void);    
	int (*HalfRate)(void);    
	int (*QuarterRate)(void);     
	int (*CustomNameGet)(char *name);
    int (*SwapCalStruct)(void);
};

extern int DevDeviceFunctionSelect(struct _DevDeviceFunction *device);
extern void DevDeviceFunctionReset(void);

//extern int DevDeviceCheckInterface(void);
extern int DevDeviceRegulatoryDomainGet(void);
extern int DevDeviceRegulatoryDomain1Get(void);
extern int DevDeviceOpFlagsGet(void);
extern int DevDeviceOpFlags2Get(void);
extern int DevDeviceIs4p9GHz(void);
extern int DevDeviceHalfRate(void);
extern int DevDeviceQuarterRate(void);
extern int DevDeviceCustomNameGet(char *name);
extern int DevDeviceSwapCalStruct(void);

#endif //_DEV_DEVICE_FUNCTION_H_---
