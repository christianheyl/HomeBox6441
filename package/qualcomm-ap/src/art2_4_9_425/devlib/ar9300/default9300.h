static OSPREY_EEPROM default9300=
{

	2, //  eeprom_version;

    2, //  template_version;

    {1,2,3,4,5,6}, //macAddr[6];

    //static  A_UINT8   custData[OSPREY_CUSTOMER_DATA_SIZE]=

	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},

//static OSPREY_BASE_EEP_HEADER base_eep_header=

	{

		    0,0x1f,	//   regDmn[2]; //Does this need to be outside of this structure, if it gets written after calibration
		    0x77,	//   txrxMask;  //4 bits tx and 4 bits rx
		    0,0,	//   opCapFlags;
		    0,		//   rfSilent;
		    0,		//   blue_tooth_options;
		    0,		//   deviceCap;
		    5,		//   deviceType; // takes lower byte in eeprom location
		    OSPREY_PWR_TABLE_OFFSET,	//    pwrTableOffset; // offset in dB to be added to beginning of pdadc table in calibration
			0,0,	//   params_for_tuning_caps[2];  //placeholder, get more details from Don
            0xc,     //feature_enable; //bit0 - enable tx temp comp 
                             //bit1 - enable tx volt comp
                             //bit2 - enable fastClock - default to 1
                             //bit3 - enable doubling - default to 1
    		0,       //misc_configuration: bit0 - turn down drivestrength
            0,0,0,0,0,0,0,0,0	//   futureBase[9];
	},


	//static OSPREY_MODAL_EEP_HEADER modal_header_2g=
	{

		    0x110,			//  ant_ctrl_common;                         // 4   idle, t1, t2, b (4 bits per setting)
		    0x22222,		//  ant_ctrl_common2;                        // 4    ra1l1, ra2l1, ra1l2, ra2l2, ra12
		    0x10,0x10,0x10,	//  antCtrlChain[OSPREY_MAX_CHAINS];       // 6   idle, t, r, rx1, rx12, b (2 bits each)
		    0,0,0,			//   xatten1DB[OSPREY_MAX_CHAINS];           // 3  //xatten1_db for merlin (0xa20c/b20c 5:0)
		    0,0,0,			//   xatten1_margin[OSPREY_MAX_CHAINS];          // 3  //xatten1_margin for merlin (0xa20c/b20c 16:12
			0,				//    tempSlope;
			0,				//    voltSlope;
		    0,0,0,0,0, // spurChans[OSPREY_EEPROM_MODAL_SPURS];  // spur channels in usual fbin coding format
		    0,0,0,			//    noiseFloorThreshCh[OSPREY_MAX_CHAINS]; // 3    //Check if the register is per chain
		    1, 1, 1,		//  ob - 3 chain;     
		    1, 1, 1,		//  db_stage2 3 chain ;
		    0, 0, 0,		//  don't exist for 2G 
		    0, 0, 0,		//  don't exist for 2G 
		    0,				//   xpaBiasLvl;                            // 1
		    0x0e,			//   txFrameToDataStart;                    // 1
		    0x0e,			//   txFrameToPaOn;                         // 1
		    3,				//   txClip;                                     // 4 bits tx_clip, 4 bits dac_scale_cck
		    0,				//    antennaGain;                           // 1
		    0x2c,			//   switchSettling;                        // 1
		    -30,			//    adcDesiredSize;                        // 1
		    0,				//   txEndToXpaOff;                         // 1
		    0x2,			//   txEndToRxOn;                           // 1
		    0xe,			//   txFrameToXpaOn;                        // 1
		    28,				//   thresh62;                              // 1
    		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0    //futureModal[32];

	},

	//static A_UINT8 calFreqPier2G[OSPREY_NUM_2G_CAL_PIERS]=
	{
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},

	//static OSP_CAL_DATA_PER_FREQ_OP_LOOP calPierData2G[OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS]=

	{	0,0,0,0,0,0,  0,0,0,0,0,0,  0,0,0,0,0,0,
		0,0,0,0,0,0,  0,0,0,0,0,0,  0,0,0,0,0,0,
		0,0,0,0,0,0,  0,0,0,0,0,0,  0,0,0,0,0,0,
	},

	//A_UINT8 cal_target_freqbin_cck[OSPREY_NUM_2G_CCK_TARGET_POWERS];

	{
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2484, 1)
	},

	//static CAL_TARGET_POWER_LEG calTarget_freqbin_2G[OSPREY_NUM_2G_20_TARGET_POWERS]
	{
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},

	//static   OSP_CAL_TARGET_POWER_HT  calTarget_freqbin_2GHT20[OSPREY_NUM_2G_20_TARGET_POWERS]
	{
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},

	//static   OSP_CAL_TARGET_POWER_HT  calTarget_freqbin_2GHT40[OSPREY_NUM_2G_40_TARGET_POWERS]
	{
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},

	//static CAL_TARGET_POWER_LEG calTargetPowerCck[OSPREY_NUM_2G_CCK_TARGET_POWERS]=
	{
		20,20,20,20,
	 	20,20,20,20
	 },

	//static CAL_TARGET_POWER_LEG calTargetPower2G[OSPREY_NUM_2G_20_TARGET_POWERS]=
	{
		20,20,20,10,
	 	20,20,20,10,
		20,20,20,10
	},

	//static   OSP_CAL_TARGET_POWER_HT  calTargetPower2GHT20[OSPREY_NUM_2G_20_TARGET_POWERS]=
	{
		20,20,10,10,0,0,10,10,0,0,10,10,0,0,
		20,20,10,10,0,0,10,10,0,0,10,10,0,0,
		20,20,10,10,0,0,10,10,0,0,10,10,0,0
	},

	//static    OSP_CAL_TARGET_POWER_HT  calTargetPower2GHT40[OSPREY_NUM_2G_40_TARGET_POWERS]=
	{
		20,20,10,10,0,0,10,10,0,0,10,10,0,0,
		20,20,10,10,0,0,10,10,0,0,10,10,0,0,
		20,20,10,10,0,0,10,10,0,0,10,10,0,0
	},

//static    A_UINT8            ctl_index_2g[OSPREY_NUM_CTLS_2G]=

	{

		    0x11,
    		0x12,
    		0x15,
    		0x17,
    		0x41,
    		0x42,
   			0x45,
    		0x47,
   			0x31,
    		0x32,
    		0x35,
    		0x37

    },

//A_UINT8   ctl_freqbin_2G[OSPREY_NUM_CTLS_2G][OSPREY_NUM_BAND_EDGES_2G];

	{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2457, 1),
			FREQ2FBIN(2462, 1),

			FREQ2FBIN(2412, 1),
		    FREQ2FBIN(2417, 1),
		    FREQ2FBIN(2462, 1),
		    0xFF,

		    FREQ2FBIN(2412, 1),
		    FREQ2FBIN(2417, 1),
		    FREQ2FBIN(2462, 1),
		    0xFF,

		    FREQ2FBIN(2422, 1),
		    FREQ2FBIN(2427, 1),
		    FREQ2FBIN(2447, 1),
		    FREQ2FBIN(2452, 1),

		    /*Data[4].ctlEdges[0].bChannel*/FREQ2FBIN(2412, 1),
		    /*Data[4].ctlEdges[1].bChannel*/FREQ2FBIN(2417, 1),
		    /*Data[4].ctlEdges[2].bChannel*/FREQ2FBIN(2472, 1),
		    /*Data[4].ctlEdges[3].bChannel*/FREQ2FBIN(2484, 1),

		    /*Data[5].ctlEdges[0].bChannel*/FREQ2FBIN(2412, 1),
		    /*Data[5].ctlEdges[1].bChannel*/FREQ2FBIN(2417, 1),
		    /*Data[5].ctlEdges[2].bChannel*/FREQ2FBIN(2472, 1),
		    0,

		    /*Data[6].ctlEdges[0].bChannel*/FREQ2FBIN(2412, 1),
		    /*Data[6].ctlEdges[1].bChannel*/FREQ2FBIN(2417, 1),
		    FREQ2FBIN(2472, 1),
		    0,

		    /*Data[7].ctlEdges[0].bChannel*/FREQ2FBIN(2422, 1),
		    /*Data[7].ctlEdges[1].bChannel*/FREQ2FBIN(2427, 1),
		    /*Data[7].ctlEdges[2].bChannel*/FREQ2FBIN(2447, 1),
		    /*Data[7].ctlEdges[3].bChannel*/FREQ2FBIN(2462, 1),

		    /*Data[8].ctlEdges[0].bChannel*/FREQ2FBIN(2412, 1),
		    /*Data[8].ctlEdges[1].bChannel*/FREQ2FBIN(2417, 1),
		    /*Data[8].ctlEdges[2].bChannel*/FREQ2FBIN(2472, 1),
		    0,

		    /*Data[9].ctlEdges[0].bChannel*/FREQ2FBIN(2412, 1),
		    /*Data[9].ctlEdges[1].bChannel*/FREQ2FBIN(2417, 1),
		    /*Data[9].ctlEdges[2].bChannel*/FREQ2FBIN(2472, 1),
		    0,

		    /*Data[10].ctlEdges[0].bChannel*/FREQ2FBIN(2412, 1),
		    /*Data[10].ctlEdges[1].bChannel*/FREQ2FBIN(2417, 1),
		    /*Data[10].ctlEdges[2].bChannel*/FREQ2FBIN(2472, 1),
		    0,

		    /*Data[11].ctlEdges[0].bChannel*/FREQ2FBIN(2422, 1),
		    /*Data[11].ctlEdges[1].bChannel*/FREQ2FBIN(2427, 1),
		    /*Data[11].ctlEdges[2].bChannel*/FREQ2FBIN(2447, 1),
		    /*Data[11].ctlEdges[3].bChannel*/FREQ2FBIN(2462, 1)
	},


//OSP_CAL_CTL_DATA_2G   ctl_power_data_2g[OSPREY_NUM_CTLS_2G];

	{
		    60,
		    0,

		    60,
		    1,

		    60,
		    0,

		    60,
		    0,

		    60,
		    0,

		    60,
		    1,

		    60,
		    0,

		    60,
		    0,

		    60,
		    1,

		    60,
		    0,

		    60,
		    0,

		    60,
		    1,

		    /*Data[3].ctlEdges[2].tPower*/60,
		    /*Data[3].ctlEdges[2].flag*/1,

		    /*Data[3].ctlEdges[3].tPower*/60,
		    /*Data[3].ctlEdges[3].flag*/0,

		    /*Data[4].ctlEdges[0].tPower*/60,
		    /*Data[4].ctlEdges[0].flag*/0,

		    /*Data[4].ctlEdges[1].tPower*/60,
		    /*Data[4].ctlEdges[1].flag*/1,

		    /*Data[4].ctlEdges[2].tPower*/60,
		    /*Data[4].ctlEdges[2].flag*/0,

		    /*Data[4].ctlEdges[3].tPower*/60,
		    /*Data[4].ctlEdges[3].flag*/0,

		    /*Data[5].ctlEdges[0].tPower*/60,
		    /*Data[5].ctlEdges[0].flag*/0,

		    /*Data[5].ctlEdges[1].tPower*/60,
		    /*Data[5].ctlEdges[1].flag*/1,

		    /*Data[5].ctlEdges[2].tPower*/60,
		    /*Data[5].ctlEdges[2].flag*/0,

		    /*Data[5].ctlEdges[3].tPower*/0,
		    /*Data[5].ctlEdges[3].flag*/0,

		    /*Data[6].ctlEdges[0].tPower*/60,
		    /*Data[6].ctlEdges[0].flag*/0,

		    /*Data[6].ctlEdges[1].tPower*/60,
		    /*Data[6].ctlEdges[1].flag*/1,

		    /*Data[6].ctlEdges[2].tPower*/60,
		    /*Data[6].ctlEdges[2].flag*/0,

		    /*Data[6].ctlEdges[3].tPower*/0,
		    /*Data[6].ctlEdges[3].flag*/0,

		    /*Data[7].ctlEdges[0].tPower*/60,
		    /*Data[7].ctlEdges[0].flag*/0,

		    /*Data[7].ctlEdges[1].tPower*/60,
		    /*Data[7].ctlEdges[1].flag*/1,

		    /*Data[7].ctlEdges[2].tPower*/60,
		    /*Data[7].ctlEdges[2].flag*/1,

		    /*Data[7].ctlEdges[3].tPower*/60,
		    /*Data[7].ctlEdges[3].flag*/0,

		    /*Data[8].ctlEdges[0].tPower*/60,
		    /*Data[8].ctlEdges[0].flag*/0,
		    /*Data[8].ctlEdges[1].tPower*/60,
		    /*Data[8].ctlEdges[1].flag*/1,

		    /*Data[8].ctlEdges[2].tPower*/60,
		    /*Data[8].ctlEdges[2].flag*/0,

		    /*Data[8].ctlEdges[3].tPower*/0,
		    /*Data[8].ctlEdges[3].flag*/0,

		    /*Data[9].ctlEdges[0].tPower*/60,
		    /*Data[9].ctlEdges[0].flag*/0,

		    /*Data[9].ctlEdges[1].tPower*/60,
		    /*Data[9].ctlEdges[1].flag*/1,

		    /*Data[9].ctlEdges[2].tPower*/60,
		    /*Data[9].ctlEdges[2].flag*/0,

		    /*Data[9].ctlEdges[3].tPower*/0,
		    /*Data[9].ctlEdges[3].flag*/0,

		    /*Data[10].ctlEdges[0].tPower*/60,
		    /*Data[10].ctlEdges[0].flag*/0,

		    /*Data[10].ctlEdges[1].tPower*/60,
		    /*Data[10].ctlEdges[1].flag*/1,

		    /*Data[10].ctlEdges[2].tPower*/60,
		    /*Data[10].ctlEdges[2].flag*/0,

		    /*Data[10].ctlEdges[3].tPower*/0,
		    /*Data[10].ctlEdges[3].flag*/0,

		    /*Data[11].ctlEdges[0].tPower*/60,
		    /*Data[11].ctlEdges[0].flag*/0,

		    /*Data[11].ctlEdges[1].tPower*/60,
		    /*Data[11].ctlEdges[1].flag*/1,

		    /*Data[11].ctlEdges[2].tPower*/60,
		    /*Data[11].ctlEdges[2].flag*/1,

		    /*Data[11].ctlEdges[3].tPower*/60,
    		/*Data[11].ctlEdges[3].flag*/1
	},

//static    OSPREY_MODAL_EEP_HEADER   modal_header_5g=

	{

		    0x110,			//  ant_ctrl_common;                         // 4   idle, t1, t2, b (4 bits per setting)
		    0x22222,		//  ant_ctrl_common2;                        // 4    ra1l1, ra2l1, ra1l2, ra2l2, ra12
		    0x10,0x10,0x10,	//  antCtrlChain[OSPREY_MAX_CHAINS];       // 6   idle, t, r, rx1, rx12, b (2 bits each)
		    0,0,0,			//   xatten1DB[OSPREY_MAX_CHAINS];           // 3  //xatten1_db for merlin (0xa20c/b20c 5:0)
		    0,0,0,			//   xatten1_margin[OSPREY_MAX_CHAINS];          // 3  //xatten1_margin for merlin (0xa20c/b20c 16:12
			0,				//    tempSlope;
			0,				//    voltSlope;
		    0,0,0,0,0, // spurChans[OSPREY_EEPROM_MODAL_SPURS];  // spur channels in usual fbin coding format
		    0,0,0,			//    noiseFloorThreshCh[OSPREY_MAX_CHAINS]; // 3    //Check if the register is per chain
		    3, 3, 3,		//  ob - 3 chain;     
		    3, 3, 3,		//  db_stage2 3 chain ;
		    3, 3, 3,		//  don't exist for 2G 
		    3, 3, 3,		//  don't exist for 2G 
		    0,				//   xpaBiasLvl;                            // 1
		    0x0e,			//   txFrameToDataStart;                    // 1
		    0x0e,			//   txFrameToPaOn;                         // 1
		    3,				//   txClip;                                     // 4 bits tx_clip, 4 bits dac_scale_cck
		    0,				//    antennaGain;                           // 1
		    0x2d,			//   switchSettling;                        // 1
		    -30,			//    adcDesiredSize;                        // 1
		    0,				//   txEndToXpaOff;                         // 1
		    0x2,			//   txEndToRxOn;                           // 1
		    0xe,			//   txFrameToXpaOn;                        // 1
		    28,				//   thresh62;                              // 1
    		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0    //futureModal[32];

	},

//static    A_UINT8            calFreqPier5G[OSPREY_NUM_5G_CAL_PIERS]=


	{
		    //pPiers[0] =
		    FREQ2FBIN(5180, 0),
		    //pPiers[1] =
		    FREQ2FBIN(5220, 0),
		    //pPiers[2] =
		    FREQ2FBIN(5320, 0),
		    //pPiers[3] =
		    FREQ2FBIN(5400, 0),
		    //pPiers[4] =
		    FREQ2FBIN(5500, 0),
		    //pPiers[5] =
		    FREQ2FBIN(5600, 0),
		    //pPiers[6] =
		    FREQ2FBIN(5725, 0),
    		//pPiers[7] =
    		FREQ2FBIN(5825, 0)
	},

//static    OSP_CAL_DATA_PER_FREQ_OP_LOOP calPierData5G[OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS]=

	{
			0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,    0,0,0,0,0,  0,0,0,0,0,
    		0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,    0,0,0,0,0,  0,0,0,0,0,
			0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,    0,0,0,0,0,  0,0,0,0,0,

	},

//static    CAL_TARGET_POWER_LEG calTarget_freqbin_5G[OSPREY_NUM_5G_20_TARGET_POWERS]=

	{
			FREQ2FBIN(5180, 0),
			FREQ2FBIN(5220, 0),
			FREQ2FBIN(5320, 0),
			FREQ2FBIN(5400, 0),
			FREQ2FBIN(5500, 0),
			FREQ2FBIN(5600, 0),
			FREQ2FBIN(5725, 0),
			FREQ2FBIN(5825, 0)
	},

//static    OSP_CAL_TARGET_POWER_HT  calTargetPower5GHT20[OSPREY_NUM_5G_20_TARGET_POWERS]=

	{
			FREQ2FBIN(5180, 0),
			FREQ2FBIN(5240, 0),
			FREQ2FBIN(5320, 0),
			FREQ2FBIN(5500, 0),
			FREQ2FBIN(5700, 0),
			FREQ2FBIN(5745, 0),
			FREQ2FBIN(5725, 0),
			FREQ2FBIN(5825, 0)
	},

//static    OSP_CAL_TARGET_POWER_HT  calTargetPower5GHT40[OSPREY_NUM_5G_40_TARGET_POWERS]=

	{
			FREQ2FBIN(5180, 0),
			FREQ2FBIN(5240, 0),
			FREQ2FBIN(5320, 0),
			FREQ2FBIN(5500, 0),
			FREQ2FBIN(5700, 0),
			FREQ2FBIN(5745, 0),
			FREQ2FBIN(5725, 0),
			FREQ2FBIN(5825, 0)
	},


//static    CAL_TARGET_POWER_LEG calTargetPower5G[OSPREY_NUM_5G_20_TARGET_POWERS]=


	{
				    //pPiers[0] =
				    20,20,20,10,
				    //pPiers[1] =
				    20,20,20,10,
				    //pPiers[2] =
				    20,20,20,10,
				    //pPiers[3] =
				    20,20,20,10,
				    //pPiers[4] =
				    20,20,20,10,
				    //pPiers[5] =
				    20,20,20,10,
				    //pPiers[6] =
				    20,20,20,10,
		    		//pPiers[7] =
    				20,20,20,10
	},

//static    OSP_CAL_TARGET_POWER_HT  calTargetPower5GHT20[OSPREY_NUM_5G_20_TARGET_POWERS]=

	{
				    //pPiers[0] =
				    20,20,10,10,0,0,10,10,0,0,10,10,0,0,
				    //pPiers[1] =
				    20,20,10,10,0,0,10,10,0,0,10,10,0,0,
				    //pPiers[2] =
				    20,20,10,10,0,0,10,10,0,0,10,10,0,0,
				    //pPiers[3] =
				    20,20,10,10,0,0,10,10,0,0,10,10,0,0,
				    //pPiers[4] =
				    20,20,10,10,0,0,10,10,0,0,10,10,0,0,
				    //pPiers[5] =
				    20,20,10,10,0,0,10,10,0,0,10,10,0,0,
				    //pPiers[6] =
				    20,20,10,10,0,0,10,10,0,0,10,10,0,0,
		    		//pPiers[7] =
    				20,20,10,10,0,0,10,10,0,0,10,10,0,0
	},

//static    OSP_CAL_TARGET_POWER_HT  calTargetPower5GHT40[OSPREY_NUM_5G_40_TARGET_POWERS]=
	{
						    //pPiers[0] =
						    20,20,10,10,0,0,10,10,0,0,10,10,0,0,
						    //pPiers[1] =
						    20,20,10,10,0,0,10,10,0,0,10,10,0,0,
						    //pPiers[2] =
						    20,20,10,10,0,0,10,10,0,0,10,10,0,0,
						    //pPiers[3] =
						    20,20,10,10,0,0,10,10,0,0,10,10,0,0,
						    //pPiers[4] =
						    20,20,10,10,0,0,10,10,0,0,10,10,0,0,
						    //pPiers[5] =
						    20,20,10,10,0,0,10,10,0,0,10,10,0,0,
						    //pPiers[6] =
						    20,20,10,10,0,0,10,10,0,0,10,10,0,0,
				    		//pPiers[7] =
    						20,20,10,10,0,0,10,10,0,0,10,10,0,0
	},

//static    A_UINT8            ctlIndex_5G[OSPREY_NUM_CTLS_5G]=

	{
		    //pCtlIndex[0] =
		    0x10,
		    //pCtlIndex[1] =
		    0x16,
		    //pCtlIndex[2] =
		    0x18,
		    //pCtlIndex[3] =
		    0x40,
		    //pCtlIndex[4] =
		    0x46,
		    //pCtlIndex[5] =
		    0x48,
		    //pCtlIndex[6] =
		    0x30,
		    //pCtlIndex[7] =
		    0x36,
    		//pCtlIndex[8] =
    		0x38
	},

//    A_UINT8   ctl_freqbin_5G[OSPREY_NUM_CTLS_5G][OSPREY_NUM_BAND_EDGES_5G];

	{
	    /* Data[0].ctlEdges[0].bChannel*/FREQ2FBIN(5180, 0),
	    /* Data[0].ctlEdges[1].bChannel*/FREQ2FBIN(5260, 0),
	    /* Data[0].ctlEdges[2].bChannel*/FREQ2FBIN(5280, 0),
	    /* Data[0].ctlEdges[3].bChannel*/FREQ2FBIN(5500, 0),
	    /* Data[0].ctlEdges[4].bChannel*/FREQ2FBIN(5600, 0),
	    /* Data[0].ctlEdges[5].bChannel*/FREQ2FBIN(5700, 0),
	    /* Data[0].ctlEdges[6].bChannel*/FREQ2FBIN(5745, 0),
	    /* Data[0].ctlEdges[7].bChannel*/FREQ2FBIN(5825, 0),

	    /* Data[1].ctlEdges[0].bChannel*/FREQ2FBIN(5180, 0),
	    /* Data[1].ctlEdges[1].bChannel*/FREQ2FBIN(5260, 0),
		/* Data[1].ctlEdges[2].bChannel*/FREQ2FBIN(5280, 0),
	    /* Data[1].ctlEdges[3].bChannel*/FREQ2FBIN(5500, 0),
	    /* Data[1].ctlEdges[4].bChannel*/FREQ2FBIN(5520, 0),
	    /* Data[1].ctlEdges[5].bChannel*/FREQ2FBIN(5700, 0),
	    /* Data[1].ctlEdges[6].bChannel*/FREQ2FBIN(5745, 0),
	    /* Data[1].ctlEdges[7].bChannel*/FREQ2FBIN(5825, 0),

	    /* Data[2].ctlEdges[0].bChannel*/FREQ2FBIN(5190, 0),
	    /* Data[2].ctlEdges[1].bChannel*/FREQ2FBIN(5230, 0),
	    /* Data[2].ctlEdges[2].bChannel*/FREQ2FBIN(5270, 0),
	    /* Data[2].ctlEdges[3].bChannel*/FREQ2FBIN(5310, 0),
	    /* Data[2].ctlEdges[4].bChannel*/FREQ2FBIN(5510, 0),
	    /* Data[2].ctlEdges[5].bChannel*/FREQ2FBIN(5550, 0),
	    /* Data[2].ctlEdges[6].bChannel*/FREQ2FBIN(5670, 0),
	    /* Data[2].ctlEdges[7].bChannel*/FREQ2FBIN(5755, 0),

	    /* Data[3].ctlEdges[0].bChannel*/FREQ2FBIN(5180, 0),
	    /* Data[3].ctlEdges[1].bChannel*/FREQ2FBIN(5200, 0),
	    /* Data[3].ctlEdges[2].bChannel*/FREQ2FBIN(5260, 0),
	    /* Data[3].ctlEdges[3].bChannel*/FREQ2FBIN(5320, 0),
	    /* Data[3].ctlEdges[4].bChannel*/FREQ2FBIN(5500, 0),
	    /* Data[3].ctlEdges[5].bChannel*/FREQ2FBIN(5700, 0),
	    /* Data[3].ctlEdges[6].bChannel*/0xFF,
	    /* Data[3].ctlEdges[7].bChannel*/0xFF,

	    /* Data[4].ctlEdges[0].bChannel*/FREQ2FBIN(5180, 0),
	    /* Data[4].ctlEdges[1].bChannel*/FREQ2FBIN(5260, 0),
	    /* Data[4].ctlEdges[2].bChannel*/FREQ2FBIN(5500, 0),
	    /* Data[4].ctlEdges[3].bChannel*/FREQ2FBIN(5700, 0),
	    /* Data[4].ctlEdges[4].bChannel*/0xFF,
	    /* Data[4].ctlEdges[5].bChannel*/0xFF,
	    /* Data[4].ctlEdges[6].bChannel*/0xFF,
	    /* Data[4].ctlEdges[7].bChannel*/0xFF,

	    /* Data[5].ctlEdges[0].bChannel*/FREQ2FBIN(5190, 0),
	    /* Data[5].ctlEdges[1].bChannel*/FREQ2FBIN(5270, 0),
	    /* Data[5].ctlEdges[2].bChannel*/FREQ2FBIN(5310, 0),
	    /* Data[5].ctlEdges[3].bChannel*/FREQ2FBIN(5510, 0),
	    /* Data[5].ctlEdges[4].bChannel*/FREQ2FBIN(5590, 0),
	    /* Data[5].ctlEdges[5].bChannel*/FREQ2FBIN(5670, 0),
	    /* Data[5].ctlEdges[6].bChannel*/0xFF,
	    /* Data[5].ctlEdges[7].bChannel*/0xFF,

	    /* Data[6].ctlEdges[0].bChannel*/FREQ2FBIN(5180, 0),
	    /* Data[6].ctlEdges[1].bChannel*/FREQ2FBIN(5200, 0),
	    /* Data[6].ctlEdges[2].bChannel*/FREQ2FBIN(5220, 0),
	    /* Data[6].ctlEdges[3].bChannel*/FREQ2FBIN(5260, 0),
	    /* Data[6].ctlEdges[4].bChannel*/FREQ2FBIN(5500, 0),
	    /* Data[6].ctlEdges[5].bChannel*/FREQ2FBIN(5600, 0),
	    /* Data[6].ctlEdges[6].bChannel*/FREQ2FBIN(5700, 0),
	    /* Data[6].ctlEdges[7].bChannel*/FREQ2FBIN(5745, 0),

	    /* Data[7].ctlEdges[0].bChannel*/FREQ2FBIN(5180, 0),
	    /* Data[7].ctlEdges[1].bChannel*/FREQ2FBIN(5260, 0),
	    /* Data[7].ctlEdges[2].bChannel*/FREQ2FBIN(5320, 0),
	    /* Data[7].ctlEdges[3].bChannel*/FREQ2FBIN(5500, 0),
	    /* Data[7].ctlEdges[4].bChannel*/FREQ2FBIN(5560, 0),
	    /* Data[7].ctlEdges[5].bChannel*/FREQ2FBIN(5700, 0),
	    /* Data[7].ctlEdges[6].bChannel*/FREQ2FBIN(5745, 0),
	    /* Data[7].ctlEdges[7].bChannel*/FREQ2FBIN(5825, 0),

	    /* Data[8].ctlEdges[0].bChannel*/FREQ2FBIN(5190, 0),
	    /* Data[8].ctlEdges[1].bChannel*/FREQ2FBIN(5230, 0),
	    /* Data[8].ctlEdges[2].bChannel*/FREQ2FBIN(5270, 0),
	    /* Data[8].ctlEdges[3].bChannel*/FREQ2FBIN(5510, 0),
	    /* Data[8].ctlEdges[4].bChannel*/FREQ2FBIN(5550, 0),
	    /* Data[8].ctlEdges[5].bChannel*/FREQ2FBIN(5670, 0),
	    /* Data[8].ctlEdges[6].bChannel*/FREQ2FBIN(5755, 0),
	    /* Data[8].ctlEdges[7].bChannel*/FREQ2FBIN(5795, 0)
	},

//static    OSP_CAL_CTL_DATA_5G   ctlData_5G[OSPREY_NUM_CTLS_5G]=

	{

	    /* Data[0].ctlEdges[0].tPower*/60,
	    /* Data[0].ctlEdges[0].flag*/1,
	    /* Data[0].ctlEdges[1].tPower*/60,
	    /* Data[0].ctlEdges[1].flag*/1,
	    /* Data[0].ctlEdges[2].tPower*/60,
	    /* Data[0].ctlEdges[2].flag*/1,
	    /* Data[0].ctlEdges[3].tPower*/60,
	    /* Data[0].ctlEdges[3].flag*/1,
	    /* Data[0].ctlEdges[4].tPower*/60,
	    /* Data[0].ctlEdges[4].flag*/1,
	    /* Data[0].ctlEdges[5].tPower*/60,
	    /* Data[0].ctlEdges[5].flag*/1,
	    /* Data[0].ctlEdges[6].tPower*/60,
	    /* Data[0].ctlEdges[6].flag*/1,
	    /* Data[0].ctlEdges[7].tPower*/60,
	    /* Data[0].ctlEdges[7].flag*/0,




	    /* Data[1].ctlEdges[0].tPower*/60,
	    /* Data[1].ctlEdges[0].flag*/1,
	    /* Data[1].ctlEdges[1].tPower*/60,
	    /* Data[1].ctlEdges[1].flag*/1,
	    /* Data[1].ctlEdges[2].tPower*/60,
	    /* Data[1].ctlEdges[2].flag*/1,
	    /* Data[1].ctlEdges[3].tPower*/60,
	    /* Data[1].ctlEdges[3].flag*/1,
	    /* Data[1].ctlEdges[4].tPower*/60,
	    /* Data[1].ctlEdges[4].flag*/1,
	    /* Data[1].ctlEdges[5].tPower*/60,
	    /* Data[1].ctlEdges[5].flag*/1,
	    /* Data[1].ctlEdges[6].tPower*/60,
	    /* Data[1].ctlEdges[6].flag*/1,
	    /* Data[1].ctlEdges[7].tPower*/60,
	    /* Data[1].ctlEdges[7].flag*/0,



	    /* Data[2].ctlEdges[0].tPower*/60,
	    /* Data[2].ctlEdges[0].flag*/0,
	    /* Data[2].ctlEdges[1].tPower*/60,
	    /* Data[2].ctlEdges[1].flag*/1,
	    /* Data[2].ctlEdges[2].tPower*/60,
	    /* Data[2].ctlEdges[2].flag*/0,
	    /* Data[2].ctlEdges[3].tPower*/60,
	    /* Data[2].ctlEdges[3].flag*/1,
	    /* Data[2].ctlEdges[4].tPower*/60,
	    /* Data[2].ctlEdges[4].flag*/1,
	    /* Data[2].ctlEdges[5].tPower*/60,
	    /* Data[2].ctlEdges[5].flag*/1,
	    /* Data[2].ctlEdges[6].tPower*/60,
	    /* Data[2].ctlEdges[6].flag*/1,
	    /* Data[2].ctlEdges[7].tPower*/60,
	    /* Data[2].ctlEdges[7].flag*/1,


	    /* Data[3].ctlEdges[0].tPower*/60,
	    /* Data[3].ctlEdges[0].flag*/0,
	    /* Data[3].ctlEdges[1].tPower*/60,
	    /* Data[3].ctlEdges[1].flag*/1,
	    /* Data[3].ctlEdges[2].tPower*/60,
	    /* Data[3].ctlEdges[2].flag*/1,
	    /* Data[3].ctlEdges[3].tPower*/60,
	    /* Data[3].ctlEdges[3].flag*/0,
	    /* Data[3].ctlEdges[4].tPower*/60,
	    /* Data[3].ctlEdges[4].flag*/1,
	    /* Data[3].ctlEdges[5].tPower*/60,
	    /* Data[3].ctlEdges[5].flag*/0,
	    /* Data[3].ctlEdges[6].tPower*/0,
	    /* Data[3].ctlEdges[6].flag*/0,
	    /* Data[3].ctlEdges[7].tPower*/0,
	    /* Data[3].ctlEdges[7].flag*/0,



	    /* Data[4].ctlEdges[0].tPower*/60,
	    /* Data[4].ctlEdges[0].flag*/1,
	    /* Data[4].ctlEdges[1].tPower*/60,
	    /* Data[4].ctlEdges[1].flag*/1,
	    /* Data[4].ctlEdges[2].tPower*/60,
	    /* Data[4].ctlEdges[2].flag*/1,
	    /* Data[4].ctlEdges[3].tPower*/60,
	    /* Data[4].ctlEdges[3].flag*/0,
	    /* Data[4].ctlEdges[4].tPower*/0,
	    /* Data[4].ctlEdges[4].flag*/0,
	    /* Data[4].ctlEdges[5].tPower*/0,
	    /* Data[4].ctlEdges[5].flag*/0,
	    /* Data[4].ctlEdges[6].tPower*/0,
	    /* Data[4].ctlEdges[6].flag*/0,
	    /* Data[4].ctlEdges[7].tPower*/0,
	    /* Data[4].ctlEdges[7].flag*/0,



	    /* Data[5].ctlEdges[0].tPower*/60,
	    /* Data[5].ctlEdges[0].flag*/1,
	    /* Data[5].ctlEdges[1].tPower*/60,
	    /* Data[5].ctlEdges[1].flag*/1,
	    /* Data[5].ctlEdges[2].tPower*/60,
	    /* Data[5].ctlEdges[2].flag*/1,
	    /* Data[5].ctlEdges[3].tPower*/60,
	    /* Data[5].ctlEdges[3].flag*/1,
	    /* Data[5].ctlEdges[4].tPower*/60,
	    /* Data[5].ctlEdges[4].flag*/1,
	    /* Data[5].ctlEdges[5].tPower*/60,
	    /* Data[5].ctlEdges[5].flag*/0,
	    /* Data[5].ctlEdges[6].tPower*/0,
	    /* Data[5].ctlEdges[6].flag*/0,
	    /* Data[5].ctlEdges[7].tPower*/0,
	    /* Data[5].ctlEdges[7].flag*/0,



	    /* Data[6].ctlEdges[0].tPower*/60,
	    /* Data[6].ctlEdges[0].flag*/1,
	    /* Data[6].ctlEdges[1].tPower*/60,
	    /* Data[6].ctlEdges[1].flag*/1,
	    /* Data[6].ctlEdges[2].tPower*/60,
	    /* Data[6].ctlEdges[2].flag*/1,
	    /* Data[6].ctlEdges[3].tPower*/60,
	    /* Data[6].ctlEdges[3].flag*/1,
	    /* Data[6].ctlEdges[4].tPower*/60,
	    /* Data[6].ctlEdges[4].flag*/1,
	    /* Data[6].ctlEdges[5].tPower*/60,
	    /* Data[6].ctlEdges[5].flag*/1,
	    /* Data[6].ctlEdges[6].tPower*/60,
	    /* Data[6].ctlEdges[6].flag*/1,
	    /* Data[6].ctlEdges[7].tPower*/60,
	    /* Data[6].ctlEdges[7].flag*/1,



	    /* Data[7].ctlEdges[0].tPower*/60,
	    /* Data[7].ctlEdges[0].flag*/1,
	    /* Data[7].ctlEdges[1].tPower*/60,
	    /* Data[7].ctlEdges[1].flag*/1,
	    /* Data[7].ctlEdges[2].tPower*/60,
	    /* Data[7].ctlEdges[2].flag*/0,
	    /* Data[7].ctlEdges[3].tPower*/60,
	    /* Data[7].ctlEdges[3].flag*/1,
	    /* Data[7].ctlEdges[4].tPower*/60,
	    /* Data[7].ctlEdges[4].flag*/1,
	    /* Data[7].ctlEdges[5].tPower*/60,
	    /* Data[7].ctlEdges[5].flag*/1,
	    /* Data[7].ctlEdges[6].tPower*/60,
	    /* Data[7].ctlEdges[6].flag*/1,
	    /* Data[7].ctlEdges[7].tPower*/60,
	    /* Data[7].ctlEdges[7].flag*/0,



	    /* Data[8].ctlEdges[0].tPower*/60,
	    /* Data[8].ctlEdges[0].flag*/1,
	    /* Data[8].ctlEdges[1].tPower*/60,
	    /* Data[8].ctlEdges[1].flag*/0,
	    /* Data[8].ctlEdges[2].tPower*/60,
	    /* Data[8].ctlEdges[2].flag*/1,
	    /* Data[8].ctlEdges[3].tPower*/60,
	    /* Data[8].ctlEdges[3].flag*/1,
	    /* Data[8].ctlEdges[4].tPower*/60,
	    /* Data[8].ctlEdges[4].flag*/1,
	    /* Data[8].ctlEdges[5].tPower*/60,
	    /* Data[8].ctlEdges[5].flag*/0,
	    /* Data[8].ctlEdges[6].tPower*/60,
	    /* Data[8].ctlEdges[6].flag*/1,
	    /* Data[8].ctlEdges[7].tPower*/60,
    	/* Data[8].ctlEdges[7].flag*/0
	}

};

