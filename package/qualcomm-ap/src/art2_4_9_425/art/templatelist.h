

#define MALLOWED 100


enum
{
	TemplatePreference=1000,
	TemplateAllowed,
	TemplateMemory,
	TemplateSize,
	TemplateCompress,
	TemplateOverwrite,
	TemplateInstall,
    TemplateFile,
    TemplateSection,
};

static int TemplateMemoryDefault=DeviceCalibrationDataEeprom;
static int TemplateMemoryDefaultRead=DeviceCalibrationDataNone;
static int TemplateMemoryMinimum=DeviceCalibrationDataNone;
static int TemplateMemoryMaximum=DeviceCalibrationDataFile;

static struct _ParameterList TemplateMemoryParameter[]=
{
	{DeviceCalibrationDataDontLoad,{"none",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{DeviceCalibrationDataNone,{"automatic",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{DeviceCalibrationDataFlash,{"flash",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{DeviceCalibrationDataEeprom,{"eeprom",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{DeviceCalibrationDataOtp,{"otp",0,0},0,0,0,0,0,0,0,0,0,0,0},
    {DeviceCalibrationDataFile,{"file",0,0},0,0,0,0,0,0,0,0,0,0,0},
};

static int TemplateSizeDefault=1024;
static int TemplateSizeDefaultRead=0;
static int TemplateSizeMinimum=0;
static int TemplateSizeMaximum=8192;

static struct _ParameterList TemplateSizeParameter[]=
{
	{0,{"automatic",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{1024,{"1K",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{2048,{"2K",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{4096,{"4K",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{8192,{"8K",0,0},0,0,0,0,0,0,0,0,0,0,0},
};

// FIX LATER
enum Ar9300EepromTemplate
{
	ar9300_eeprom_template_generic        = 2,
	ar9300_eeprom_template_hb112          = 3,
	ar9300_eeprom_template_hb116          = 4,
	ar9300_eeprom_template_xb112          = 5,
	ar9300_eeprom_template_xb113          = 6,
	ar9300_eeprom_template_xb114          = 7,
	ar9300_eeprom_template_tb417          = 8,
	ar9300_eeprom_template_ap111          = 9,
	ar9300_eeprom_template_ap121          = 10,
	ar9300_eeprom_template_hornet_generic = 11,
    ar9300_eeprom_template_wasp_2         = 12,
    ar9300_eeprom_template_wasp_k31       = 13,
    ar9300_eeprom_template_osprey_k31     = 14,

	qc98xx_eeprom_template_generic        = 20,
	//qc98xx_eeprom_template_cus220		  = 21,
	qc98xx_eeprom_template_cus223		  = 22,
	qc98xx_eeprom_template_wb342	      = 23,
	qc98xx_eeprom_template_xb340		  = 24,
	//qc98xx_eeprom_template_cus226		  = 25,
	qc98xx_eeprom_template_cus226    	  = 26,
	qc98xx_eeprom_template_xb141 	      = 27,
	qc98xx_eeprom_template_xb143 	      = 28,
};

#define ar9300_eeprom_template_default 2
#define qc98xx_eeprom_template_default 20

static int TemplatePreferenceDefault=ar9300_eeprom_template_default;
static int TemplatePreferenceMinimum=ar9300_eeprom_template_default;

static struct _ParameterList TemplatePreferenceParameter[]=
{
	{ar9300_eeprom_template_generic,{"ar938x",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{ar9300_eeprom_template_generic,{"ar939x",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{ar9300_eeprom_template_hb112,{"hb112",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{ar9300_eeprom_template_hb116,{"hb116",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{ar9300_eeprom_template_xb112,{"xb112",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{ar9300_eeprom_template_xb113,{"xb113",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{ar9300_eeprom_template_xb114,{"xb114",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{ar9300_eeprom_template_tb417,{"tb417",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{ar9300_eeprom_template_ap111,{"ap111",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{ar9300_eeprom_template_ap121,{"ap121",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{ar9300_eeprom_template_hornet_generic,{"ar9330",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{qc98xx_eeprom_template_generic,{"qc98xx",0,0},0,0,0,0,0,0,0,0,0,0,0},
	//{qc98xx_eeprom_template_cus220,{"cus220",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{qc98xx_eeprom_template_cus223,{"cus223",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{qc98xx_eeprom_template_wb342,{"wb342",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{qc98xx_eeprom_template_xb340,{"xb340",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{qc98xx_eeprom_template_cus226,{"cus226",0,0},0,0,0,0,0,0,0,0,0,0,0},
	//{qc98xx_eeprom_template_cus226_030,{"cus226_030",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{qc98xx_eeprom_template_xb141,{"xb141",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{qc98xx_eeprom_template_xb143,{"xb143",0,0},0,0,0,0,0,0,0,0,0,0,0},
};

static int TemplateCompressionDefault=0;
static int TemplateOverwriteDefault=1;
static int TemplateFileDefault=1;

static struct _ParameterList TemplateLogicalParameter[]=
{
	{0,{"no",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{1,{"yes",0,0},0,0,0,0,0,0,0,0,0,0,0},
};

static int TemplateInstallDefault=0;

static struct _ParameterList TemplateInstallParameter[]=
{
	{0,{"no",0,0},"never install the template",0,0,0,0,0,0,0,0,0,0},
	{1,{"yes",0,0},"always install the template",0,0,0,0,0,0,0,0,0,0},
	{2,{"blank",0,0},"install on a blank card",0,0,0,0,0,0,0,0,0,0},
};

static int TemplateSectionDefault=0;
static struct _ParameterList TemplateSectionParameter[]=
{
	{DeviceEepromSaveSectionAll, {"all","a",0},"all sections (default)",0,0,0,0,0,0,0,0,0,0},
	{DeviceEepromSaveSectionId, {"id",0,0},"ID section",0,0,0,0,0,0,0,0,0,0},
	{DeviceEepromSaveSectionMac, {"mac",0,0},"MAC address",0,0,0,0,0,0,0,0,0,0},
	{DeviceEepromSaveSection2GCal, {"2gcal",0,0},"2GHz calibration data",0,0,0,0,0,0,0,0,0,0},
	{DeviceEepromSaveSection5GCal, {"5gcal",0,0},"5GHz calibration data",0,0,0,0,0,0,0,0,0,0},
	{DeviceEepromSaveSection2GCtl, {"2gctl",0,0},"2GHz control data",0,0,0,0,0,0,0,0,0,0},
	{DeviceEepromSaveSection5GCtl, {"5gctl",0,0},"5GHz control data",0,0,0,0,0,0,0,0,0,0},
	{DeviceEepromSaveSectionConfig, {"config","cfg",0},"configiguration data",0,0,0,0,0,0,0,0,0,0},
	{DeviceEepromSaveSectionCustomer, {"customer","cus",0},"customer data",0,0,0,0,0,0,0,0,0,0},
	{DeviceEepromSaveSectionUSBID, {"usbid",0,0}, "usb VID and PID",0,0,0,0,0,0,0,0,0,0},
	{DeviceEepromSaveSectionSDIOID, {"sdioid",0,0}, "sdio PID(13 bits)",0,0,0,0,0,0,0,0,0,0},
	{DeviceEepromSaveSectionXTAL, {"xtal",0,0}, "Xtal calibration",0,0,0,0,0,0,0,0,0,0},
	{DeviceEepromSaveSection5GRxCal, {"5grxcal",0,0}, "5GHz Rx calibration data",0,0,0,0,0,0,0,0,0,0},
	{DeviceEepromSaveSection2GRxCal, {"2grxcal",0,0}, "2GHz Rx calibration data",0,0,0,0,0,0,0,0,0,0},
};

#define TEMPLATE_PREFERENCE	{TemplatePreference,{"preference","default",0},"the prefered starting template",'d',0,1,1,1,&TemplatePreferenceMinimum,0,&TemplatePreferenceDefault,	\
	    sizeof(TemplatePreferenceParameter)/sizeof(TemplatePreferenceParameter[0]),TemplatePreferenceParameter}

#define TEMPLATE_ALLOWED {TemplateAllowed,{"allow",0,0},"which templates may be used",'d',0,MALLOWED,1,1,&TemplatePreferenceMinimum,0,&TemplatePreferenceDefault,	\
	    sizeof(TemplatePreferenceParameter)/sizeof(TemplatePreferenceParameter[0]),TemplatePreferenceParameter}

#define TEMPLATE_MEMORY {TemplateMemory,{"memory","caldata",0},"memory type used for calibration data",'z',0,1,1,1,&TemplateMemoryMinimum,&TemplateMemoryMaximum,&TemplateMemoryDefault,	\
	    sizeof(TemplateMemoryParameter)/sizeof(TemplateMemoryParameter[0]),TemplateMemoryParameter}

#define TEMPLATE_MEMORY_READ {TemplateMemory,{"memory","caldata",0},"memory type used for calibration data",'z',0,1,1,1,&TemplateMemoryMinimum,&TemplateMemoryMaximum,&TemplateMemoryDefaultRead,	\
	    sizeof(TemplateMemoryParameter)/sizeof(TemplateMemoryParameter[0]),TemplateMemoryParameter}

#define TEMPLATE_SIZE {TemplateSize,{"size",0,0},"memory size used for calibration data",'z',0,1,1,1,&TemplateSizeMinimum,&TemplateSizeMaximum,&TemplateSizeDefault,	\
	    sizeof(TemplateSizeParameter)/sizeof(TemplateSizeParameter[0]),TemplateSizeParameter}

#define TEMPLATE_SIZE_READ {TemplateSize,{"size",0,0},"memory size used for calibration data",'z',0,1,1,1,&TemplateSizeMinimum,&TemplateSizeMaximum,&TemplateSizeDefaultRead,	\
	    sizeof(TemplateSizeParameter)/sizeof(TemplateSizeParameter[0]),TemplateSizeParameter}

#define TEMPLATE_COMPRESS {TemplateCompress,{"compress",0,0},"use compression?",'z',0,1,1,1,0,0,&TemplateCompressionDefault,	\
	    sizeof(TemplateLogicalParameter)/sizeof(TemplateLogicalParameter[0]),TemplateLogicalParameter}

#define TEMPLATE_OVERWRITE {TemplateOverwrite,{"overwrite",0,0},"overwrite existing data?",'z',0,1,1,1,0,0,&TemplateOverwriteDefault,	\
	    sizeof(TemplateLogicalParameter)/sizeof(TemplateLogicalParameter[0]),TemplateLogicalParameter}

#define TEMPLATE_INSTALL {TemplateInstall,{"install",0,0},"install tempalte?",'z',0,1,1,1,0,0,&TemplateInstallDefault,	\
	    sizeof(TemplateInstallParameter)/sizeof(TemplateInstallParameter[0]),TemplateInstallParameter}

#define TEMPLATE_FILE {TemplateFile,{"file",0,0},"eeprom bin file",'z',0,1,1,1,0,0,&TemplateFileDefault,  \
        sizeof(TemplateLogicalParameter)/sizeof(TemplateLogicalParameter[0]),TemplateLogicalParameter}
        
#define TEMPLATE_SECTION(MALLOWED) {TemplateSection,{"section",0,0},"section(s) to commit",'z',0,MALLOWED,1,1,0,0,&TemplateSectionDefault,  \
        sizeof(TemplateSectionParameter)/sizeof(TemplateSectionParameter[0]),TemplateSectionParameter}
