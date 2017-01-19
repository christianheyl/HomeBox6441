#ifndef _SLEEP_MODE_H_
#define _SLEEP_MODE_H_

enum
{
	SLEEP_MODE_SLEEP = 0,
	SLEEP_MODE_WAKEUP,
	SLEEP_MODE_DEEP_SLEEP,
};

extern void SleepModeWakeupSet();
extern void SleepModeParameterSplice(struct _ParameterList *list);
extern void SleepModeCommand(int client);

#endif //_SLEEP_MODE_H_