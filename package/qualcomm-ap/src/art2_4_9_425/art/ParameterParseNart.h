#include "ParameterConfigDef.h"

extern void paramItemSet(_PARAM_ITEM_STRUCT *param, char *itemName, int min, int max, int numValues);
extern void paramItemReSet(_PARAM_ITEM_STRUCT *param, char *itemName, int min, int max, int numValues);
extern int paramItemIsSet(_PARAM_ITEM_STRUCT *param, char *itemName, int *itemNum);
extern int ParseIntegerWithParamName(int input, char *name, _PARAM_ITEM_STRUCT *params, int client);
extern void Print_RefSet(char *name, _PARAM_ITEM_STRUCT *paramS, int client);
extern void Print_RefGet(char *name, _PARAM_ITEM_STRUCT *paramS, int client);
extern void Print_RefSetCalTGTPwrDataRate(_PARAM_ITEM_STRUCT *paramS, int client);
extern void calTGTpwrDataRateSelTitle(int iMode, int iSel, char *title);
extern void calTGTpwrDataRateTitle(int iMode, char *title);

extern int getCmdSetValue(char *name, int ip, int num, int *value, char *tValue, int isHex);



