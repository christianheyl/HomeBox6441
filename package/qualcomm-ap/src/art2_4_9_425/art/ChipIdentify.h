extern char *LoadDLL(char *dllName, int *devid);

extern char *DevidToLibrary(int devid);

extern char *SearchLibrary(int *devid);

//extern int ChipSelect(int devid);

extern void HALCommand();

#define ChipUnknown (-1)
#define ChipTest 1
#define ChipLinkTest 2

//extern void ChipDevidParameterSplice(struct _ParameterList *list);
extern void HALParameterSplice(struct _ParameterList *list);

#define AR5416_DEVID_AR9287_PCIE  0x002e    /* PCIE (Kiwi) */

/* AR9300 */
#define AR9300_DEVID_AR9380_PCIE  0x0030        /* PCIE (Osprey) */
#define AR9300_DEVID_EMU_PCIE     0xabcd
#define AR9300_DEVID_AR9340       0x0031        /* Wasp */
#define AR9300_DEVID_AR9485_PCIE  0x0032        /* Poseidon */
#define AR9300_DEVID_AR9580_PCIE  0x0033        /* Peacock */
#define AR9300_DEVID_AR946X_PCIE  0x0034        /* Jupiter: 2x2 DB + BT - AR9462 */
                                                /*          2x2 SB + BT - AR9463 */
                                                /*          2x2 DB      - AR9482 */
#define AR9300_DEVID_AR956X_PCIE  0x0036        /* Aphrodite: 1x1 DB + BT - AR9564 */
                                                /*            1x1 SB + BT - AR9465 */
#define AR9300_DEVID_AR9330       0x0035        /* Hornet */
#define AR9300_DEVID_AR1111_PCIE  0x0037        /* AR1111 */	// CBU_main
#define AR9300_DEVID_AR955X       0x0039        /* Scorpion */
#define AR9300_DEVID_AR956X	      0x003F    	/* Dragonfly */
#define AR9300_DEVID_AR953X       0x003d        /* Honeybee */

/* AR6004 */
#define AR6004_DEVID	          0x3b

/* AR6006 */
#define AR6006_DEVID              0x38
/* QC9888 */
#define QC98XX_DEVID              0x3c          /* Peregrine */

struct _DevidToName
{
	int devid;
	char *name;
};

// the first hal dll to be choose for the devid
static struct _DevidToName DevidToName[]=
{
	{ChipTest,"NoChip"},								// text based test device, no chip, no link
	{ChipLinkTest,"LinkNoChip"},						// text based test device, no chip, regular link
	{AR9300_DEVID_AR9380_PCIE,"ar9300"},			// osprey
	{AR9300_DEVID_AR946X_PCIE,"ar946x"},				// jupiter
	{AR9300_DEVID_AR956X_PCIE,"ar956x"},				// aphrodite
	{AR9300_DEVID_AR9580_PCIE,"ar9300"},			// peacock
	{AR9300_DEVID_AR9485_PCIE,"AR9485"},	         		// poseidon
//	{AR9300_DEVID_AR9330,"ar9300_9-3-0"},				// hornet
	{AR9300_DEVID_AR9340,"ar9300"},				// wasp
	{AR9300_DEVID_AR955X,"ar9300"},				// Scorpion
	{AR6004_DEVID,"OlcaAr6004"},							// mckinley
    {QC98XX_DEVID,"qc98xx"},                            // Peregrine
    {AR5416_DEVID_AR9287_PCIE,"ar9287"},				// Kiwi
	{AR9300_DEVID_AR953X,"ar9300"},				// Honeybee
	{AR9300_DEVID_AR956X,"ar9300"},				// Dragonfly
};

// list of all the available HAL dll
static char *HAL_Dll[]=
{
    "ar9287",
    "ar946x",
	"ar956x",
	"AR9485",
    "ar9300",					// newmastaging
	"ar9300Aquila",
    "qc98xx",
    "OlcaAr6004",
};
