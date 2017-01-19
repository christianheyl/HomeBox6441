

enum LibraryType
{
    DeviceLibrary=0,
    LinkLibrary,
    CalibrationLibrary,
};

extern int osDllLoad(char *dllname, char *DeviceFullName, enum LibraryType libraryType);
extern void osDllUnload(enum LibraryType libraryType);
extern void *osGetFunctionAddress(char *function, enum LibraryType libraryType);
extern int osCheckLibraryHandle(enum LibraryType libraryType);


