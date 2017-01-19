#ifndef _QC98XX_EEPROM_STRUCT_H_
#define _QC98XX_EEPROM_STRUCT_H_

typedef enum targetPowerLegacyRates {
    LEGACY_TARGET_RATE_6_24,
    LEGACY_TARGET_RATE_36,
    LEGACY_TARGET_RATE_48,
    LEGACY_TARGET_RATE_54
}TARGET_POWER_LEGACY_RATES;

typedef enum targetPowerCckRates {
    LEGACY_TARGET_RATE_1L_5L,
    LEGACY_TARGET_RATE_5S,
    LEGACY_TARGET_RATE_11L,
    LEGACY_TARGET_RATE_11S
}TARGET_POWER_CCK_RATES;

typedef enum targetPowerVHTRates {
    VHT_TARGET_RATE_0 = 0,
    VHT_TARGET_RATE_1_2,
    VHT_TARGET_RATE_3_4,
    VHT_TARGET_RATE_5,
    VHT_TARGET_RATE_6,
    VHT_TARGET_RATE_7,
    VHT_TARGET_RATE_8,
    VHT_TARGET_RATE_9,
    VHT_TARGET_RATE_10,
    VHT_TARGET_RATE_11_12,
    VHT_TARGET_RATE_13_14,
    VHT_TARGET_RATE_15,
    VHT_TARGET_RATE_16,
    VHT_TARGET_RATE_17,
    VHT_TARGET_RATE_18,
    VHT_TARGET_RATE_19,
    VHT_TARGET_RATE_20,
    VHT_TARGET_RATE_21_22,
    VHT_TARGET_RATE_23_24,
    VHT_TARGET_RATE_25,
    VHT_TARGET_RATE_26,
    VHT_TARGET_RATE_27,
    VHT_TARGET_RATE_28,
    VHT_TARGET_RATE_29,
	VHT_TARGET_RATE_LAST,
}TARGET_POWER_VHT_RATES;

#define IS_1STREAM_TARGET_POWER_VHT_RATES(x) (((x) >= VHT_TARGET_RATE_0) && ((x) <= VHT_TARGET_RATE_9))
#define IS_2STREAM_TARGET_POWER_VHT_RATES(x) (((x) >= VHT_TARGET_RATE_10 && (x) <= VHT_TARGET_RATE_19)) 
#define IS_3STREAM_TARGET_POWER_VHT_RATES(x) (((x) >= VHT_TARGET_RATE_20 && (x) <= VHT_TARGET_RATE_29) )

#define QC98XX_CTL_MODE_M            0xF
#define QC98XX_BCHAN_UNUSED          0xFF

#define RATE_PRINT_HT_SIZE			24
#define RATE_PRINT_VHT_SIZE			30

const static char *sRatePrintHT[30] = 
{
	"MCS0 ",
	"MCS1 ", 
	"MCS2 ",
	"MCS3 ",
	"MCS4 ",
	"MCS5 ",
	"MCS6 ",
	"MCS7 ",
	"MCS8 ",
	"MCS9 ",
	"MCS10",
	"MCS11",
	"MCS12",
	"MCS13",
	"MCS14",
	"MCS15",
	"MCS16",
    "MCS17",
	"MCS18",
	"MCS19",
	"MCS20",
	"MCS21",
	"MCS22",
	"MCS23",
	"MCS24",
	"MCS25",
	"MCS26",
	"MCS27",
	"MCS28",
	"MCS29",
	};

const static char *sRatePrintVHT[30] = 
{
	"MCS0_1SS",
	"MCS1_1SS", 
	"MCS2_1SS",
	"MCS3_1SS",
	"MCS4_1SS",
	"MCS5_1SS",
	"MCS6_1SS",
	"MCS7_1SS",
	"MCS8_1SS",
	"MCS9_1SS",
	"MCS0_2SS",
	"MCS1_2SS", 
	"MCS2_2SS",
	"MCS3_2SS",
	"MCS4_2SS",
	"MCS5_2SS",
	"MCS6_2SS",
	"MCS7_2SS",
	"MCS8_2SS",
	"MCS9_2SS",
	"MCS0_3SS",
	"MCS1_3SS", 
	"MCS2_3SS",
	"MCS3_3SS",
	"MCS4_3SS",
	"MCS5_3SS",
	"MCS6_3SS",
	"MCS7_3SS",
	"MCS8_3SS",
	"MCS9_3SS",
	};


const static char *sRatePrintLegacy[4] = 
{
	"6-24",
	" 36 ", 
	" 48 ", 
	" 54 "
};

const static char *sRatePrintCck[4] = 
{
	"1L-5L",
	" 5S  ", 
	" 11L ", 
	" 11S "
};

const static char *sDeviceType[] = 
{
    "UNKNOWN",
    "Cardbus",
    "PCI    ",
    "MiniPCI",
    "AP     ",
    "PCIE   ",
    "UNKNOWN",
    "UNKNOWN",
    "USB    ",
};

const static char *sCtlType[] = 
{
    "[ 11A base mode ]",
    "[ 11B base mode ]",
    "[ 11G base mode ]",
    "[ INVALID       ]",
    "[ INVALID       ]",
    "[ 2G HT20 mode  ]",
    "[ 5G HT20 mode  ]",
    "[ 2G HT40 mode  ]",
    "[ 5G HT40 mode  ]",
    "[ 5G VHT80 mode ]",
};

const static int mapRate2Index[24]=
{
    0,1,1,1,2,
    3,4,5,0,1,
    1,1,6,7,8,
    9,0,1,1,1,
    10,11,12,13
};

	
#endif //_QC98XX_EEPROM_STRUCT_H_
