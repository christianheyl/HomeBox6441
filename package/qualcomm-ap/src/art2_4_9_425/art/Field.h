
#ifdef _WINDOWS
#ifdef FIELDDLL
		#define FIELDDLLSPEC __declspec(dllexport)
	#else
		#define FIELDDLLSPEC __declspec(dllimport)
	#endif
#else
	#define FIELDDLLSPEC
#endif

struct _Field
{
	char *registerName;
	char *fieldName;
	unsigned int address;
	char low;
	char high;
};


extern FIELDDLLSPEC void FieldSelect(struct _Field *field, int nfield);


extern FIELDDLLSPEC int FieldFindByAddress(unsigned int address, int low, int high, char **registerName, char **fieldName);

extern FIELDDLLSPEC int FieldFindByAddressOnly(unsigned int address, int index, int *low, int *high, char **registerName, char **fieldName);


extern FIELDDLLSPEC int FieldFind(char *name, unsigned int *address, int *low, int *high);


extern FIELDDLLSPEC int FieldWrite(char *name, unsigned int value);
extern FIELDDLLSPEC int FieldWriteNoMask(char *name, unsigned int value);


extern FIELDDLLSPEC int FieldRead(char *name, unsigned int *value);

extern FIELDDLLSPEC int FieldReadNoMask(char *name, unsigned int *value);


extern FIELDDLLSPEC int FieldList(char *pattern, void (*print)(char *name, unsigned int address, int low, int high));

extern FIELDDLLSPEC int FieldGet(char *name, unsigned int *value, unsigned int reg);
extern FIELDDLLSPEC int FieldSet(char *name, unsigned int value, unsigned int *reg);
