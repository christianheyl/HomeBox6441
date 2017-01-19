#ifndef _SET_CONFIG_H_
#define _SET_CONFIG_H_

#define MAX_NUM_SET_CONFIG          100

typedef struct SetConfigHash
{
	char *pKey;
	char *pVal;
} SETCONFIG_HASH;

extern void SetConfigParameterSplice(struct _ParameterList *list);
extern void SetConfigCommand(int client);
extern void SetConfigProcess();
extern int SetConfigSet (char *Item, int ItemLen, char *Value, int ValueLen);

#endif //_SET_CONFIG_H_
