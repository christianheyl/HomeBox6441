

extern FIELDDLLSPEC int GetSetDataSetup(struct _StructPrint *list, int nlist, unsigned char *data, int ndata);

extern FIELDDLLSPEC int GetParameterSplice(struct _ParameterList *list);

extern FIELDDLLSPEC int SetParameterSplice(struct _ParameterList *list);

//
// parse and then set a configuration parameter in the internal structure
//
extern FIELDDLLSPEC int SetCommand(int client);

//
// parse and then get a configuration parameter in the internal structure
//
extern FIELDDLLSPEC int GetCommand(int client);


extern FIELDDLLSPEC int GetAll(void (*print)(char *format, ...), int all)
;
