/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * Notifications and licenses are retained for attribution purposes only.
 */
/*
 * Copyright (c) 2002-2006 Sam Leffler, Errno Consulting
 * Copyright (c) 2005-2006 Atheros Communications, Inc.
 * Copyright (c) 2010, Atheros Communications Inc. 
 * 
 * Redistribution and use in source and binary forms are permitted
 * provided that the following conditions are met:
 * 1. The materials contained herein are unmodified and are used
 *    unmodified.
 * 2. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following NO
 *    ''WARRANTY'' disclaimer below (''Disclaimer''), without
 *    modification.
 * 3. Redistributions in binary form must reproduce at minimum a
 *    disclaimer similar to the Disclaimer below and any redistribution
 *    must be conditioned upon including a substantially similar
 *    Disclaimer requirement for further binary redistribution.
 * 4. Neither the names of the above-listed copyright holders nor the
 *    names of any contributors may be used to endorse or promote
 *    product derived from this software without specific prior written
 *    permission.
 * 
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT,
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGES.
 */

#include "ol_if_athvar.h"

#include "athdefs.h"
#include "ol_defines.h"
#include "ol_if_ath_api.h"
#include "ol_helper.h"
#include "wlan_defs.h"

#include "ol_regdomain.h"

/* used throughout this file... */
#define    N(a)    (sizeof (a) / sizeof (a[0]))

/* 10MHz is half the 11A bandwidth used to determine upper edge freq
   of the outdoor channel */
#define HALF_MAXCHANBW        10

/* Mask to check whether a domain is a multidomain or a single
   domain */

#define MULTI_DOMAIN_MASK 0xFF00

#define    WORLD_SKU_MASK        0x00F0
#define    WORLD_SKU_PREFIX    0x0060

/* Global configuration overrides */
static    const int countrycode = -1;
static    const int xchanmode = -1;
static    const int ath_outdoor = AH_FALSE;        /* enable outdoor use */
static    const int ath_indoor  = AH_FALSE;        /* enable indoor use  */

int
wmi_unified_pdev_set_regdomain(wmi_unified_t wmi_handle, struct ol_regdmn *ol_regdmn_handle)
{
    wmi_pdev_set_regdomain_cmd *cmd;
    wmi_buf_t buf;

    int len = sizeof( wmi_pdev_set_regdomain_cmd);

    buf = wmi_buf_alloc(wmi_handle, len);
    if (!buf) {
        printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
        return -1;
    }

    cmd = (wmi_pdev_set_regdomain_cmd *)wmi_buf_data(buf);

    cmd->reg_domain = ol_regdmn_handle->ol_regdmn_currentRDInUse ;
    cmd->reg_domain_2G = ol_regdmn_handle->ol_regdmn_currentRD2G;
    cmd->reg_domain_5G = ol_regdmn_handle->ol_regdmn_currentRD5G;
    cmd->conformance_test_limit_2G = ol_regdmn_handle->ol_regdmn_ctl_2G;
    cmd->conformance_test_limit_5G = ol_regdmn_handle->ol_regdmn_ctl_5G;
    cmd->dfs_domain = ol_regdmn_handle->ol_regdmn_dfsDomain;

    return wmi_unified_cmd_send(wmi_handle, buf, len, 
                                WMI_PDEV_SET_REGDOMAIN_CMDID);
}

bool ol_regdmn_set_regdomain(struct ol_regdmn* ol_regdmn_handle, REGDMN_REG_DOMAIN regdomain)
{
    ol_regdmn_handle->ol_regdmn_current_rd = regdomain;
    
    return true;
}

bool ol_regdmn_set_regdomain_ext(struct ol_regdmn* ol_regdmn_handle, REGDMN_REG_DOMAIN regdomain)
{
    ol_regdmn_handle->ol_regdmn_current_rd_ext = regdomain;
    return true;
}

bool ol_regdmn_get_regdomain(struct ol_regdmn* ol_regdmn_handle, REGDMN_REG_DOMAIN *regdomain)
{
    *regdomain = ol_regdmn_handle->ol_regdmn_current_rd;
    return true;
}
/* Helper function to get hardware wireless capabilities */
u_int ol_regdmn_getWirelessModes(struct ol_regdmn* ol_regdmn_handle)
{
    return ol_regdmn_handle->scn_handle->hal_reg_capabilities.wireless_modes;
}

#ifdef OL_ATH_DUMP_RD_SPECIFIC_CHANNELS

void inline
ol_ath_dump_channel_entry(
	struct ieee80211_channel * channel
	)
{
	printk(
		"%4d %08X      %02X     %03d         %02d       %02d" 
		"       %02X         %02d     %02d   %02d   %02d ",
		channel->ic_freq,
		channel->ic_flags,
		channel->ic_flagext,
		channel->ic_ieee,
		channel->ic_maxregpower,
		channel->ic_maxpower,
		channel->ic_minpower,
		channel->ic_regClassId,
		channel->ic_antennamax,
		channel->ic_vhtop_ch_freq_seg1,
		channel->ic_vhtop_ch_freq_seg2
		);

	if( channel->ic_flags & IEEE80211_CHAN_TURBO )
	{
		printk("TURBO ");
	}

	if( channel->ic_flags & IEEE80211_CHAN_CCK )
	{
		printk("CCK ");
	}

	if( channel->ic_flags & IEEE80211_CHAN_OFDM )
	{
		printk("OFDM ");
	}

	if( channel->ic_flags & IEEE80211_CHAN_2GHZ )
	{
		printk("2G ");
	}

	if( channel->ic_flags & IEEE80211_CHAN_5GHZ )
	{
		printk("5G ");
	}

	if( channel->ic_flags & IEEE80211_CHAN_PASSIVE )
	{
		printk("PSV ");
	}
	
	if( channel->ic_flags & IEEE80211_CHAN_DYN )
	{
		printk("DYN ");
	}

	if( channel->ic_flags & IEEE80211_CHAN_GFSK )
	{
		printk("GFSK ");
	}
	
	if( channel->ic_flags & IEEE80211_CHAN_RADAR )
	{
		printk("RDR ");
	}

	if( channel->ic_flags & IEEE80211_CHAN_STURBO )
	{
		printk("STURBO ");
	}

	if( channel->ic_flags & IEEE80211_CHAN_HALF )
	{
		printk("HALF ");
	}

	if( channel->ic_flags & IEEE80211_CHAN_QUARTER )
	{
		printk("QUARTER ");
	}

	
	if( channel->ic_flags & IEEE80211_CHAN_HT20 )
	{
		printk("HT20 ");
	}

	if( channel->ic_flags & IEEE80211_CHAN_HT40PLUS )
	{
		printk("HT40+ ");
	}
	
	if( channel->ic_flags & IEEE80211_CHAN_HT40MINUS )
	{
		printk("HT40- ");
	}
	
	if( channel->ic_flags & IEEE80211_CHAN_HT40INTOL )
	{
		printk("HT40INTOL ");
	}

	if( channel->ic_flags & IEEE80211_CHAN_VHT20 )
	{
		printk("VHT20 ");
	}

	if( channel->ic_flags & IEEE80211_CHAN_VHT40PLUS )
	{
		printk("VHT40+ ");
	}

	if( channel->ic_flags & IEEE80211_CHAN_VHT40MINUS )
	{
		printk("VHT40- ");
	}

	
	if( channel->ic_flags & IEEE80211_CHAN_VHT80 )
	{
		printk("VHT80 ");
	}
	
	printk("\n");

}

void inline
ol_ath_dump_rd_specific_channels(
	struct ieee80211com *ic,
	struct ol_regdmn *ol_regdmn_handle
	)
{
	struct ieee80211_channel *chans = ic->ic_channels;
	a_uint32_t i;

	printk("\tXXXXXX RegDomain specific channel list XXXXXX\n");
	
	printk(
		"RD: %X, RD(in use): %X, CCODE: %s (%X)\n",
		ol_regdmn_handle->ol_regdmn_current_rd,
		ol_regdmn_handle->ol_regdmn_currentRDInUse,
		ol_regdmn_handle->ol_regdmn_iso,
		ol_regdmn_handle->ol_regdmn_countryCode
		);
	
	printk(
		"freq    flags flagext IEEE No. maxregpwr " 
		"maxtxpwr mintxpwr regclassID antmax "
		"seg1 seg2\n"
		);
	
	for( i = 0; i < ic->ic_nchans; i++ )
	{
		ol_ath_dump_channel_entry( &chans[i] );
	}
}

#endif

int
ol_regdmn_getchannels(struct ol_regdmn *ol_regdmn_handle, u_int cc,
                bool outDoor, bool xchanMode, IEEE80211_REG_PARAMETERS *reg_parm)
{
    ol_scn_t scn_handle;
    struct ieee80211com *ic;
    struct ieee80211_channel *chans;
    int nchan;
    u_int8_t regclassids[ATH_REGCLASSIDS_MAX];
    u_int nregclass = 0;
    u_int wMode;
    u_int netBand;
    u_int i;
    
    scn_handle = ol_regdmn_handle->scn_handle;
    ic = &scn_handle->sc_ic;

    wMode = reg_parm->wModeSelect;

    if (!(wMode & REGDMN_MODE_11A)) {
        wMode &= ~(REGDMN_MODE_TURBO|REGDMN_MODE_108A|REGDMN_MODE_11A_HALF_RATE);
    }
    if (!(wMode & REGDMN_MODE_11G)) {
        wMode &= ~(REGDMN_MODE_108G);
    }
    
    netBand = reg_parm->netBand;
    if (!(netBand & REGDMN_MODE_11A)) {
        netBand &= ~(REGDMN_MODE_TURBO|REGDMN_MODE_108A|REGDMN_MODE_11A_HALF_RATE);
    }
    if (!(netBand & REGDMN_MODE_11G)) {
        netBand &= ~(REGDMN_MODE_108G);
    }
    wMode &= netBand;
    
    chans = ic->ic_channels;

    if (chans == NULL) {
        printk("%s: unable to allocate channel table\n", __func__);
        return -ENOMEM;
    }
    
    /*
     * remove some of the modes based on different compile time
     * flags.
     */

    if (!ol_regdmn_init_channels(ol_regdmn_handle, chans, IEEE80211_CHAN_MAX, (u_int *)&nchan,
                               regclassids, ATH_REGCLASSIDS_MAX, &nregclass,
                               cc, wMode, outDoor, xchanMode)) {
        u_int32_t rd;

        rd = ol_regdmn_handle->ol_regdmn_current_rd;
        printk("%s: unable to collect channel list from regdomain; "
               "regdomain likely %u country code %u\n",
               __func__, rd, cc);
        return -EINVAL;
    }
    
    for (i=0; i<nchan; i++) {
        chans[i].ic_maxpower = scn_handle->max_tx_power;
        chans[i].ic_minpower = scn_handle->min_tx_power;
    }

    ol_80211_channel_setup(ic, CLIST_UPDATE,
                           chans, nchan, regclassids, nregclass,
                           CTRY_DEFAULT);

#ifdef OL_ATH_DUMP_RD_SPECIFIC_CHANNELS
	ol_ath_dump_rd_specific_channels(ic, ol_regdmn_handle);
#endif

    wmi_unified_pdev_set_regdomain(scn_handle->wmi_handle, ol_regdmn_handle);
    
    return 0;
}

void
ol_regdmn_start(struct ol_regdmn *ol_regdmn_handle, IEEE80211_REG_PARAMETERS *reg_parm )
{
    ol_scn_t scn_handle;
    REGDMN_REG_DOMAIN rd;
    int error;
    
    scn_handle = ol_regdmn_handle->scn_handle;
      
    ol_regdmn_get_regdomain(ol_regdmn_handle, &rd);

	printk(
		"%s: reg-domain param: regdmn=%X, countryName=%s, wModeSelect=%X, netBand=%X, extendedChanMode=%X.\n",
		__func__,
		reg_parm->regdmn,
    	reg_parm->countryName,
    	reg_parm->wModeSelect,
    	reg_parm->netBand,
    	reg_parm->extendedChanMode
		);

    if (reg_parm->regdmn) {
        // Set interface regdmn configuration
        ol_regdmn_set_regdomain(ol_regdmn_handle, reg_parm->regdmn);
    }

    ol_regdmn_handle->ol_regdmn_countryCode = CTRY_DEFAULT;

    if (countrycode != -1) {
        ol_regdmn_handle->ol_regdmn_countryCode = countrycode;
    }
      
    if ((rd == 0) && reg_parm->countryName[0]) {
        ol_regdmn_handle->ol_regdmn_countryCode = ol_regdmn_findCountryCode((u_int8_t *)reg_parm->countryName);        
    }

   if (xchanmode != -1) {
        ol_regdmn_handle->ol_regdmn_xchanmode = xchanmode;
    } else {
        ol_regdmn_handle->ol_regdmn_xchanmode = reg_parm->extendedChanMode;
    }
   
    error = ol_regdmn_getchannels(ol_regdmn_handle, ol_regdmn_handle->ol_regdmn_countryCode,
                            ath_outdoor, ol_regdmn_handle->ol_regdmn_xchanmode, reg_parm);
                            
    if (error != 0) {
        printk( "%s[%d]: Failed to get channel information! error[%d]\n", __func__, __LINE__, error ); 
    }
     
}

void
ol_regdmn_detach(struct ol_regdmn* ol_regdmn_handle)
{
    if (ol_regdmn_handle != NULL) {
        OS_FREE(ol_regdmn_handle);
        ol_regdmn_handle = NULL;
    }
}

/*
 * Store the channel edges for the requested operational mode
 */
bool
ol_get_channel_edges(struct ol_regdmn* ol_regdmn_handle,
    u_int16_t flags, u_int16_t *low, u_int16_t *high)
{
    HAL_REG_CAPABILITIES *p_cap = &ol_regdmn_handle->scn_handle->hal_reg_capabilities;

    if (flags & IEEE80211_CHAN_5GHZ) {
        *low = p_cap->low_5ghz_chan;
        *high = p_cap->high_5ghz_chan;
        return true;
    }
    if ((flags & IEEE80211_CHAN_2GHZ)) {
        *low = p_cap->low_2ghz_chan;
        *high = p_cap->high_2ghz_chan;

        return true;
    }
    return false;
}

static int
chansort(const void *a, const void *b)
{
#define CHAN_FLAGS    (IEEE80211_CHAN_ALL)
    const struct ieee80211_channel *ca = a;
    const struct ieee80211_channel *cb = b;

    return (ca->ic_freq == cb->ic_freq) ?
        (ca->ic_flags & CHAN_FLAGS) -
            (cb->ic_flags & CHAN_FLAGS) :
        ca->ic_freq - cb->ic_freq;
#undef CHAN_FLAGS
}

typedef int ath_hal_cmp_t(const void *, const void *);
static    void ath_hal_sort(void *a, u_int32_t n, u_int32_t es, ath_hal_cmp_t *cmp);
static const COUNTRY_CODE_TO_ENUM_RD* findCountry(REGDMN_CTRY_CODE countryCode, REGDMN_REG_DOMAIN rd);
static bool getWmRD(struct ol_regdmn* ol_regdmn_handle, int regdmn, u_int16_t channelFlag, REG_DOMAIN *rd);

#define isWwrSKU(_ol_regdmn_handle) (((getEepromRD((_ol_regdmn_handle)) & WORLD_SKU_MASK) == WORLD_SKU_PREFIX) || \
               (getEepromRD(_ol_regdmn_handle) == WORLD))

#define isWwrSKU_NoMidband(_ol_regdmn_handle) ((getEepromRD((_ol_regdmn_handle)) == WOR3_WORLD) || \
                (getEepromRD(_ol_regdmn_handle) == WOR4_WORLD) || \
                (getEepromRD(_ol_regdmn_handle) == WOR5_ETSIC))
#define isUNII1OddChan(ch) ((ch == 5170) || (ch == 5190) || (ch == 5210) || (ch == 5230))

/*
 * By default, the regdomain tables reference the common tables
 * from ah_regdomain_common.h.  These default tables can be replaced
 * by calls to populate_regdomain_tables functions.
 */

#if !_MAVERICK_STA_
HAL_REG_DMN_TABLES ol_regdmn_Rdt = {
    ahCmnRegDomainPairs,    /* regDomainPairs */
#ifdef notyet
    ahCmnRegDmnVendorPairs, /* regDmnVendorPairs */
#else
    AH_NULL,                /* regDmnVendorPairs */
#endif
    ahCmnAllCountries,      /* allCountries */
    AH_NULL,                /* customMappings */
    ahCmnRegDomains,        /* regDomains */

    N(ahCmnRegDomainPairs),    /* regDomainPairsCt */
#ifdef notyet
    N(ahCmnRegDmnVendorPairs), /* regDmnVendorPairsCt */
#else
    0,                         /* regDmnVendorPairsCt */
#endif
    N(ahCmnAllCountries),      /* allCountriesCt */
    0,                         /* customMappingsCt */
    N(ahCmnRegDomains),        /* regDomainsCt */
};
#else
/* Maverick will assign the regdomain tables at attach time. */
HAL_REG_DMN_TABLES ol_regdmn_Rdt;
#endif


static u_int16_t
getEepromRD(struct ol_regdmn* ol_regdmn_handle)
{
    return ol_regdmn_handle->ol_regdmn_current_rd &~ WORLDWIDE_ROAMING_FLAG;
}

/*
 * Test to see if the bitmask array is all zeros
 */
static bool
isChanBitMaskZero(u_int64_t *bitmask)
{
    int i;

    for (i=0; i<BMLEN; i++) {
        if (bitmask[i] != 0)
            return false;
    }
    return true;
}

/*
 * Return whether or not the regulatory domain/country in EEPROM
 * is acceptable.
 */
static bool
isEepromValid(struct ol_regdmn* ol_regdmn_handle)
{
    u_int16_t rd = getEepromRD(ol_regdmn_handle);
    int i;

    if (rd & COUNTRY_ERD_FLAG) {
        u_int16_t cc = rd &~ COUNTRY_ERD_FLAG;
        for (i = 0; i < ol_regdmn_Rdt.allCountriesCt; i++)
            if (ol_regdmn_Rdt.allCountries[i].countryCode == cc)
                return true;
    } else {
        for (i = 0; i < ol_regdmn_Rdt.regDomainPairsCt; i++)
            if (ol_regdmn_Rdt.regDomainPairs[i].regDmnEnum == rd)
                return true;
    }
    printk("%s: invalid regulatory domain/country code 0x%x\n",
        __func__, rd);
    return false;
}

#if !(_MAVERICK_STA_ || __NetBSD_) /* Apple EEPROMs do not have the Midband bit set */
/*
 * Return whether or not the FCC Mid-band flag is set in EEPROM
 */
static bool
isFCCMidbandSupported(struct ol_regdmn* ol_regdmn_handle)
{
    u_int32_t regcap;
    ol_scn_t scn_handle;
		
    scn_handle = ol_regdmn_handle->scn_handle;

    regcap = scn_handle->hal_reg_capabilities.regcap2;

    if (regcap & REGDMN_EEPROM_EEREGCAP_EN_FCC_MIDBAND) {
        return true;
    }
    else {
        return false;
    }
}
#endif /* !(_MAVERICK_STA_ || __NetBSD_) */

static const REG_DMN_PAIR_MAPPING *
getRegDmnPair(REGDMN_REG_DOMAIN reg_dmn)
{
    int i;
    for (i = 0; i < ol_regdmn_Rdt.regDomainPairsCt; i++) {
        if (ol_regdmn_Rdt.regDomainPairs[i].regDmnEnum == reg_dmn) {
            return &ol_regdmn_Rdt.regDomainPairs[i];
        }
    }
    return AH_NULL;
}

/*
 * Returns whether or not the specified country code
 * is allowed by the EEPROM setting
 */
static bool
isCountryCodeValid(struct ol_regdmn* ol_regdmn_handle, REGDMN_CTRY_CODE cc)
{
    u_int16_t  rd;
    int i, support5G, support2G;
    u_int modesAvail;
    const COUNTRY_CODE_TO_ENUM_RD *country = AH_NULL;
    const REG_DMN_PAIR_MAPPING *regDmn_eeprom, *regDmn_input;

    /* Default setting requires no checks */
    if (cc == CTRY_DEFAULT)
        return true;
#ifdef AH_DEBUG_COUNTRY
    if (cc == CTRY_DEBUG)
        return true;
#endif
    rd = getEepromRD(ol_regdmn_handle);
    printk("%s: EEPROM regdomain 0x%x\n", __func__, rd);

    if (rd & COUNTRY_ERD_FLAG) {
        /* EEP setting is a country - config shall match */
        printk("%s: EEPROM setting is country code %u\n",
            __func__, rd &~ COUNTRY_ERD_FLAG);
        return (cc == (rd & ~COUNTRY_ERD_FLAG));
    }

#if _MAVERICK_STA_

    for (i=0; i < ol_regdmn_Rdt.customMappingsCt; i++) {
        if (ol_regdmn_Rdt.customMappings[i].regDmnEnum == rd) {
            int j=0;
            while(ol_regdmn_Rdt.customMappings[i].countryMappings[j].isoName != NULL) {
                if (cc == ol_regdmn_Rdt.customMappings[i].countryMappings[j].countryCode)
                    return true;
                ++j;
            }
        }
    }
#endif

    for (i = 0; i < ol_regdmn_Rdt.allCountriesCt; i++) {
        if (cc == ol_regdmn_Rdt.allCountries[i].countryCode) {
#ifdef REGDMN_SUPPORT_11D
            if ((rd & WORLD_SKU_MASK) == WORLD_SKU_PREFIX) {
                return true;
            }
#endif
            country = &ol_regdmn_Rdt.allCountries[i];
            if (country->regDmnEnum == rd ||
                rd == DEBUG_REG_DMN || rd == NO_ENUMRD) {
                return true;
            }
        }
    }
    if (country == AH_NULL) return false;

    /* consider device capability and look into the regdmn for 2G&5G */
    modesAvail = ol_regdmn_getWirelessModes(ol_regdmn_handle);
    support5G = modesAvail & (REGDMN_MODE_11A | REGDMN_MODE_TURBO |
                REGDMN_MODE_108A | REGDMN_MODE_11A_HALF_RATE |
                REGDMN_MODE_11A_QUARTER_RATE |
                REGDMN_MODE_11NA_HT40PLUS |
                REGDMN_MODE_11NA_HT40MINUS);
    support2G = modesAvail & (REGDMN_MODE_11G | REGDMN_MODE_11B | REGDMN_MODE_PUREG |
                REGDMN_MODE_108G |
                REGDMN_MODE_11NG_HT40PLUS |
                REGDMN_MODE_11NG_HT40MINUS);

    regDmn_eeprom = getRegDmnPair(rd);
    regDmn_input = getRegDmnPair(country->regDmnEnum);
    if (!regDmn_eeprom || !regDmn_input)
        return false;

    if (support5G && support2G) {
        /* force a strict binding between regdmn and countrycode */
        return false;
    } else if (support5G) {
        if (regDmn_eeprom->regDmn5GHz == regDmn_input->regDmn5GHz)
            return true;
    } else if (support2G) {
        if (regDmn_eeprom->regDmn2GHz == regDmn_input->regDmn2GHz)
            return true;
    } else {
        printk("%s: neither 2G nor 5G is supported\n", __func__);
    }
    return false;
}

/*
 * Return the mask of available modes based on the hardware
 * capabilities and the specified country code and reg domain.
 */
static u_int
ol_regdmn_getwmodesnreg(struct ol_regdmn* ol_regdmn_handle, const COUNTRY_CODE_TO_ENUM_RD *country,
             REG_DOMAIN *rd5GHz)
{
    u_int modesAvail;

    /* Get modes that HW is capable of */
    modesAvail = ol_regdmn_getWirelessModes(ol_regdmn_handle);    

    /* Check country regulations for allowed modes */
#ifndef ATH_NO_5G_SUPPORT
    if ((modesAvail & (REGDMN_MODE_11A_TURBO|REGDMN_MODE_TURBO)) &&
        (!country->allow11aTurbo))
        modesAvail &= ~(REGDMN_MODE_11A_TURBO | REGDMN_MODE_TURBO);
#endif
    if ((modesAvail & REGDMN_MODE_11G_TURBO) &&
        (!country->allow11gTurbo))
        modesAvail &= ~REGDMN_MODE_11G_TURBO;
    if ((modesAvail & REGDMN_MODE_11G) &&
        (!country->allow11g))
        modesAvail &= ~REGDMN_MODE_11G;
#ifndef ATH_NO_5G_SUPPORT
    if ((modesAvail & REGDMN_MODE_11A) &&
        (isChanBitMaskZero(rd5GHz->chan11a)))
        modesAvail &= ~REGDMN_MODE_11A;
#endif

    if ((modesAvail & REGDMN_MODE_11NG_HT20) &&
       (!country->allow11ng20))
        modesAvail &= ~REGDMN_MODE_11NG_HT20;

    if ((modesAvail & REGDMN_MODE_11NA_HT20) &&
       (!country->allow11na20))
        modesAvail &= ~REGDMN_MODE_11NA_HT20;

    if ((modesAvail & REGDMN_MODE_11NG_HT40PLUS) &&
       (!country->allow11ng40))
        modesAvail &= ~REGDMN_MODE_11NG_HT40PLUS;

    if ((modesAvail & REGDMN_MODE_11NG_HT40MINUS) &&
       (!country->allow11ng40))
        modesAvail &= ~REGDMN_MODE_11NG_HT40MINUS;

    if ((modesAvail & REGDMN_MODE_11NA_HT40PLUS) &&
       (!country->allow11na40))
        modesAvail &= ~REGDMN_MODE_11NA_HT40PLUS;

    if ((modesAvail & REGDMN_MODE_11NA_HT40MINUS) &&
       (!country->allow11na40))
        modesAvail &= ~REGDMN_MODE_11NA_HT40MINUS;

    if ((modesAvail & REGDMN_MODE_11AC_VHT20) &&
       (!country->allow11na20))
        modesAvail &= ~REGDMN_MODE_11AC_VHT20;

    if ((modesAvail & REGDMN_MODE_11AC_VHT40PLUS) &&
        (!country->allow11na40))
        modesAvail &= ~REGDMN_MODE_11AC_VHT40PLUS;
    
    if ((modesAvail & REGDMN_MODE_11AC_VHT40MINUS) &&
        (!country->allow11na40))
        modesAvail &= ~REGDMN_MODE_11AC_VHT40MINUS;

    if ((modesAvail & REGDMN_MODE_11AC_VHT80) &&
        (!country->allow11na80))
        modesAvail &= ~REGDMN_MODE_11AC_VHT80;

    return modesAvail;
}

/*
 * Return the mask of available modes based on the hardware
 * capabilities and the specified country code.
 */

u_int __ahdecl
ol_regdmn_getwirelessmodes(struct ol_regdmn* ol_regdmn_handle, REGDMN_CTRY_CODE cc)
{
    const COUNTRY_CODE_TO_ENUM_RD *country=AH_NULL;
    u_int mode=0;
    REG_DOMAIN rd;

    country = findCountry(cc, getEepromRD(ol_regdmn_handle));
    if (country != AH_NULL) {
        if (getWmRD(ol_regdmn_handle, country->regDmnEnum, ~IEEE80211_CHAN_2GHZ, &rd))
            mode = ol_regdmn_getwmodesnreg(ol_regdmn_handle, country, &rd);
    }
    return(mode);
}

/*
 * Return if device is public safety.
 */
bool __ahdecl
ol_regdmn_ispublicsafetysku(struct ol_regdmn* ol_regdmn_handle)
{
    u_int16_t rd;

    rd = getEepromRD(ol_regdmn_handle);

    switch (rd) {
        case FCC4_FCCA:
        case (CTRY_UNITED_STATES_FCC49 | COUNTRY_ERD_FLAG):
            return true;

        case DEBUG_REG_DMN:
        case NO_ENUMRD:
            if (ol_regdmn_handle->ol_regdmn_countryCode == 
                        CTRY_UNITED_STATES_FCC49) {
                return true;
            }
            break;
    }

    return false;
}


/*
 * Find the country code.
 */
REGDMN_CTRY_CODE __ahdecl
ol_regdmn_findCountryCode(u_int8_t *countryString)
{
    int i;

    for (i = 0; i < ol_regdmn_Rdt.allCountriesCt; i++) {
        if ((ol_regdmn_Rdt.allCountries[i].isoName[0] == countryString[0]) &&
            (ol_regdmn_Rdt.allCountries[i].isoName[1] == countryString[1]))
            return (ol_regdmn_Rdt.allCountries[i].countryCode);
    }
    return (0);        /* Not found */
}

/*
 * Find the conformance_test_limit by country code.
 */
u_int8_t __ahdecl
ol_regdmn_findCTLByCountryCode(REGDMN_CTRY_CODE ol_regdmn_countrycode, bool is2G)
{
    int i = 0, j = 0, k = 0;
    
    if  (ol_regdmn_countrycode == 0) {
        return NO_CTL;
    }
    
     for (i = 0; i < ol_regdmn_Rdt.allCountriesCt; i++) {
        if (ol_regdmn_Rdt.allCountries[i].countryCode == ol_regdmn_countrycode) {
            for (j = 0; j < ol_regdmn_Rdt.regDomainPairsCt; j++) {
                if (ol_regdmn_Rdt.regDomainPairs[j].regDmnEnum == ol_regdmn_Rdt.allCountries[i].regDmnEnum) {
                    for (k = 0; k < ol_regdmn_Rdt.regDomainsCt; k++) {
                        if (is2G) {
                             if (ol_regdmn_Rdt.regDomains[k].regDmnEnum == ol_regdmn_Rdt.regDomainPairs[j].regDmn2GHz)
                                 return ol_regdmn_Rdt.regDomains[k].conformance_test_limit;                
                        } else {
                             if (ol_regdmn_Rdt.regDomains[k].regDmnEnum == ol_regdmn_Rdt.regDomainPairs[j].regDmn5GHz)
                                 return ol_regdmn_Rdt.regDomains[k].conformance_test_limit;                
                        }
                    }
                }
            }
        }
    }

    return NO_CTL;
}


/*
 * Find the pointer to the country element in the country table
 * corresponding to the country code
 */
static const COUNTRY_CODE_TO_ENUM_RD*
findCountry(REGDMN_CTRY_CODE countryCode, REGDMN_REG_DOMAIN rd)
{
    int i;

#if _MAVERICK_STA_
    for (i=0; i < ol_regdmn_Rdt.customMappingsCt; i++) {
        if (ol_regdmn_Rdt.customMappings[i].regDmnEnum == rd) {
            int j=0;
            while(ol_regdmn_Rdt.customMappings[i].countryMappings[j].isoName != NULL) {
                if (countryCode == ol_regdmn_Rdt.customMappings[i].countryMappings[j].countryCode)
                    return &ol_regdmn_Rdt.customMappings[i].countryMappings[j];
                ++j;
            }
            i=ol_regdmn_Rdt.customMappingsCt;
        }
    }
#endif

    for (i = 0; i < ol_regdmn_Rdt.allCountriesCt; i++) {
        if (ol_regdmn_Rdt.allCountries[i].countryCode == countryCode)
            return (&ol_regdmn_Rdt.allCountries[i]);
    }
    return (AH_NULL);        /* Not found */
}

/*
 * Calculate a default country based on the EEPROM setting.
 */
static REGDMN_CTRY_CODE
getDefaultCountry(struct ol_regdmn* ol_regdmn_handle)
{
    u_int16_t rd;
    int i;

    rd =getEepromRD(ol_regdmn_handle);
    if (rd & COUNTRY_ERD_FLAG) {
        const COUNTRY_CODE_TO_ENUM_RD *country=AH_NULL;
        u_int16_t cc= rd & ~COUNTRY_ERD_FLAG;

        country = findCountry(cc, rd);
        if (country != AH_NULL)
            return cc;
    }
    /*
     * Check reg domains that have only one country
     */
    for (i = 0; i < ol_regdmn_Rdt.regDomainPairsCt; i++) {
        if (ol_regdmn_Rdt.regDomainPairs[i].regDmnEnum == rd) {
            if (ol_regdmn_Rdt.regDomainPairs[i].singleCC != 0) {
                return ol_regdmn_Rdt.regDomainPairs[i].singleCC;
            } else {
                i = ol_regdmn_Rdt.regDomainPairsCt;
            }
        }
    }
    return CTRY_DEFAULT;
}

/*
 * Convert Country / RegionDomain to RegionDomain
 * rd could be region or country code.
*/
REGDMN_REG_DOMAIN
ol_regdmn_getDomain(struct ol_regdmn *ol_regdmn_handle, u_int16_t rd)
{
    if (rd & COUNTRY_ERD_FLAG) {
        const COUNTRY_CODE_TO_ENUM_RD *country=AH_NULL;
        u_int16_t cc= rd & ~COUNTRY_ERD_FLAG;

        country = findCountry(cc, rd);
        if (country != AH_NULL)
            return country->regDmnEnum;
        return 0;
    }
    return rd;
}

static bool
isValidRegDmn(int reg_dmn, REG_DOMAIN *rd)
{
    int i;

    for (i = 0; i < ol_regdmn_Rdt.regDomainsCt; i++) {
        if (ol_regdmn_Rdt.regDomains[i].regDmnEnum == reg_dmn) {
            if (rd != AH_NULL) {
                OS_MEMCPY(rd, &ol_regdmn_Rdt.regDomains[i], sizeof(REG_DOMAIN));
            }
            return true;
        }
    }
    return false;
}

static bool
isValidRegDmnPair(int regDmnPair)
{
    int i;

    if (regDmnPair == NO_ENUMRD) return false;
    for (i = 0; i < ol_regdmn_Rdt.regDomainPairsCt; i++) {
        if (ol_regdmn_Rdt.regDomainPairs[i].regDmnEnum == regDmnPair) return true;
    }
    return false;
}

/*
 * Return the Wireless Mode Regulatory Domain based
 * on the country code and the wireless mode.
 */
static bool
getWmRD(struct ol_regdmn *ol_regdmn_handle, int reg_dmn, u_int16_t channelFlag, REG_DOMAIN *rd)
{
    int i, found;
    u_int64_t flags=NO_REQ;
    const REG_DMN_PAIR_MAPPING *regPair=AH_NULL;
#ifdef notyet
    VENDOR_PAIR_MAPPING *vendorPair=AH_NULL;
#endif
    int regOrg;

    regOrg = reg_dmn;
    if (reg_dmn == CTRY_DEFAULT) {
        u_int16_t rdnum;
        rdnum =getEepromRD(ol_regdmn_handle);

        if (!(rdnum & COUNTRY_ERD_FLAG)) {
            if (isValidRegDmn(rdnum, AH_NULL) ||
                isValidRegDmnPair(rdnum)) {
                reg_dmn = rdnum;
            }
        }
    }

    if ((reg_dmn & MULTI_DOMAIN_MASK) == 0) {

        for (i = 0, found = 0; (i < ol_regdmn_Rdt.regDomainPairsCt) && (!found); i++) {
            if (ol_regdmn_Rdt.regDomainPairs[i].regDmnEnum == reg_dmn) {
                regPair = &ol_regdmn_Rdt.regDomainPairs[i];
                found = 1;
            }
        }
        if (!found) {
            printk("%s: Failed to find reg domain pair %u\n",
                 __func__, reg_dmn);
            return false;
        }
        if (!(channelFlag & IEEE80211_CHAN_2GHZ)) {
            reg_dmn = regPair->regDmn5GHz;
            flags = regPair->flags5GHz;
        }
        if (channelFlag & IEEE80211_CHAN_2GHZ) {
            reg_dmn = regPair->regDmn2GHz;
            flags = regPair->flags2GHz;
        }
#ifdef notyet
        for (i=0, found=0; (i < ol_regdmn_Rdt.regDomainVendorPairsCt) && (!found); i++) {
            if ((regDomainVendorPairs[i].regDmnEnum == reg_dmn) &&
                (ol_regdmn_handle->ol_regdmn_vendor == regDomainVendorPairs[i].vendor)) {
                vendorPair = &regDomainVendorPairs[i];
                found = 1;
            }
        }
        if (found) {
            if (!(channelFlag & IEEE80211_CHAN_2GHZ)) {
                flags &= vendorPair->flags5GHzIntersect;
                flags |= vendorPair->flags5GHzUnion;
            }
            if (channelFlag & IEEE80211_CHAN_2GHZ) {
                flags &= vendorPair->flags2GHzIntersect;
                flags |= vendorPair->flags2GHzUnion;
            }
        }
#endif
    }

    /*
     * We either started with a unitary reg domain or we've found the 
     * unitary reg domain of the pair
     */

    found = isValidRegDmn(reg_dmn, rd);
    if (!found) {
        printk("%s: Failed to find unitary reg domain %u\n",
             __func__, reg_dmn);
        return false;
    } else {
        if (regPair == AH_NULL) {
            printk("%s: No set reg domain pair\n", __func__);
            return false;
        }
        rd->pscan &= regPair->pscanMask;
        if (((regOrg & MULTI_DOMAIN_MASK) == 0) &&
            (flags != NO_REQ)) {
            rd->flags = flags;
        }
        /*
         * Use only the domain flags that apply to the current mode.
         * In particular, do not apply 11A Adhoc flags to b/g modes.
         */
        rd->flags &= (channelFlag & IEEE80211_CHAN_2GHZ) ? 
            REG_DOMAIN_2GHZ_MASK : REG_DOMAIN_5GHZ_MASK;
        return true;
    }
}

static bool
IS_BIT_SET(int bit, u_int64_t *bitmask)
{
    int byteOffset, bitnum;
    u_int64_t val;

    byteOffset = bit/64;
    bitnum = bit - byteOffset*64;
    val = ((u_int64_t) 1) << bitnum;
    if (bitmask[byteOffset] & val)
        return true;
    else
        return false;
}
    
/* Add given regclassid into regclassids array upto max of maxregids */
static void
ath_add_regclassid(u_int8_t *regclassids, u_int maxregids, u_int *nregids, u_int8_t regclassid)
{
    int i;

    /* Is regclassid valid? */
    if (regclassid == 0)
        return;

    for (i=0; i < maxregids; i++) {
        if (regclassids[i] == regclassid)
            return;
        if (regclassids[i] == 0)
            break;
    }

    if (i == maxregids)
        return;
    else {
        regclassids[i] = regclassid;
        *nregids += 1;
    }

    return;
}

#if !(_MAVERICK_STA_ || __NetBSD__)
static bool
getEepromRegExtBits(struct ol_regdmn* ol_regdmn_handle, REG_EXT_BITMAP bit)
{
    return ((ol_regdmn_handle->ol_regdmn_current_rd_ext & (1<<bit))? true : false);
}
#endif


#define IS_HT40_MODE(_mode)      \
                    (((_mode == REGDMN_MODE_11NA_HT40PLUS  || \
                     _mode == REGDMN_MODE_11NG_HT40PLUS    || \
                     _mode == REGDMN_MODE_11NA_HT40MINUS   || \
                     _mode == REGDMN_MODE_11NG_HT40MINUS) ? true : false))

#define IS_VHT40_MODE(_mode)      \
                    (((_mode == REGDMN_MODE_11AC_VHT40PLUS  || \
                     _mode == REGDMN_MODE_11AC_VHT40MINUS) ? true : false))
					 
#define IS_VHT80_MODE(_mode) (_mode == REGDMN_MODE_11AC_VHT80) ? true : false)

/*
 * Setup the channel list based on the information in the EEPROM and
 * any supplied country code.  Note that we also do a bunch of EEPROM
 * verification here and setup certain regulatory-related access
 * control data used later on.
 */
#define OL_REGDMN_VHT80_CHAN_MAX 8

struct ol_regdmn_vht80_chan {
    u_int16_t   vht80_ch[OL_REGDMN_VHT80_CHAN_MAX];
    u_int16_t   n_vht80_ch;                                    
};

struct ol_regdmn_vht80_chan vht80_chans;

static void
ol_regdmn_init_vht80_chan(struct ol_regdmn_vht80_chan *ol_regdmn_vht80_chans)
{
    ol_regdmn_vht80_chans->n_vht80_ch = 0;
}

static void
ol_regdmn_add_vht80_chan(struct ol_regdmn_vht80_chan *ol_regdmn_vht80_chans, u_int16_t vht80_ch)
{    
    if(ol_regdmn_vht80_chans->n_vht80_ch < OL_REGDMN_VHT80_CHAN_MAX) {
        ol_regdmn_vht80_chans->vht80_ch[ol_regdmn_vht80_chans->n_vht80_ch++] = vht80_ch;
    } else {
        printk("[%s][%d] vht80_ch overlimit=%d\n", __func__, __LINE__, OL_REGDMN_VHT80_CHAN_MAX);
    }  
}
static bool ol_regdmn_find_vht80_chan(struct ol_regdmn_vht80_chan *ol_regdmn_vht80_chans, u_int16_t ch, u_int16_t *vht80_ch)
{
    u_int16_t i;
    
    for( i = 0; i < ol_regdmn_vht80_chans->n_vht80_ch; i++) {
        if(ol_regdmn_vht80_chans->vht80_ch[i] > ch) {
            if((ol_regdmn_vht80_chans->vht80_ch[i] - ch) < 40) {
                *vht80_ch =  ol_regdmn_vht80_chans->vht80_ch[i];
                return true;
            }
        } else {
            if((ch - ol_regdmn_vht80_chans->vht80_ch[i]) < 40) {
                *vht80_ch =  ol_regdmn_vht80_chans->vht80_ch[i];
                return true;
            }
        }
    }
    return false;
}

bool __ahdecl
ol_regdmn_init_channels(struct ol_regdmn *ol_regdmn_handle,
              struct ieee80211_channel *chans, u_int maxchans, u_int *nchans,
              u_int8_t *regclassids, u_int maxregids, u_int *nregids,
              REGDMN_CTRY_CODE cc, u_int32_t modeSelect,
              bool enableOutdoor, bool enableExtendedChannels)
{
#define CHANNEL_HALF_BW        10
#define CHANNEL_QUARTER_BW    5
    u_int modesAvail;
    u_int16_t maxChan=7000;
    const COUNTRY_CODE_TO_ENUM_RD *country=AH_NULL;
    REG_DOMAIN rd5GHz, rd2GHz;
    const struct cmode *cm;
    int next=0,b;
    u_int8_t ctl;
    int is_quarterchan_cap, is_halfchan_cap, is_49ghz_cap;
    HAL_DFS_DOMAIN dfsDomain=0;
    int regdmn;
    u_int32_t regDmnFlags;
    u_int16_t chanSep;
    ol_scn_t scn_handle;
    struct ieee80211com *ic;
		
    scn_handle = ol_regdmn_handle->scn_handle;
    ic = &scn_handle->sc_ic;

    /*
     * We now have enough state to validate any country code
     * passed in by the caller.
     */
    if (!isCountryCodeValid(ol_regdmn_handle, cc)) {
        /* NB: Atheros silently ignores invalid country codes */
        printk("%s: invalid country code %d\n",
                __func__, cc);
        return false;
    }

    /*
     * Validate the EEPROM setting and setup defaults
     */
    if (!isEepromValid(ol_regdmn_handle)) {
        /*
         * Don't return any channels if the EEPROM has an
         * invalid regulatory domain/country code setting.
         */
        printk("%s: invalid EEPROM contents\n",__func__);
        return false;
    }

    ol_regdmn_handle->ol_regdmn_countryCode = getDefaultCountry(ol_regdmn_handle);

    if (ol_regdmn_handle->ol_regdmn_countryCode == CTRY_DEFAULT) {
        /*
         * XXX - TODO: Switch based on Japan country code
         * to new reg domain if valid japan card
         */
#if _MAVERICK_STA_
        if (ol_regdmn_handle->ol_regdmn_current_rd == APL9_WORLD)
          /*  TBD || (ol_regdmn_handle->ol_regdmn_current_rd == MKK8_MKKA)) */
            /* NB: only one country code is allowed in this SKU */
            ol_regdmn_handle->ol_regdmn_countryCode &= COUNTRY_CODE_MASK;
        else
            ol_regdmn_handle->ol_regdmn_countryCode = cc & COUNTRY_CODE_MASK;
        if (ol_regdmn_handle->ol_regdmn_current_rd == 0) /* Unknown locale, currently merlin doesn't have this value programmed --AJAYP */
            ol_regdmn_handle->ol_regdmn_current_rd = WOR4_WORLD;
#else
        ol_regdmn_handle->ol_regdmn_countryCode = cc & COUNTRY_CODE_MASK;

        if ((ol_regdmn_handle->ol_regdmn_countryCode == CTRY_DEFAULT) &&
                (getEepromRD(ol_regdmn_handle) == CTRY_DEFAULT)) {
                /* Set to DEBUG_REG_DMN for debug or set to CTRY_UNITED_STATES for testing */
                ol_regdmn_handle->ol_regdmn_countryCode = CTRY_UNITED_STATES;
        }
#endif
    }
			
#ifdef REGDMN_SUPPORT_11D
    if(ol_regdmn_handle->ol_regdmn_countryCode == CTRY_DEFAULT)
    {
        regdmn =getEepromRD(ol_regdmn_handle);
        country = AH_NULL;
    }
    else {
#endif
        /* Get pointers to the country element and the reg domain elements */
        country = findCountry(ol_regdmn_handle->ol_regdmn_countryCode, getEepromRD(ol_regdmn_handle));

        if (country == AH_NULL) {
            printk("Country is NULL!!!!, cc= %d\n",
                ol_regdmn_handle->ol_regdmn_countryCode);
            return false;
    } else {
            regdmn = country->regDmnEnum;
#if !(_MAVERICK_STA_ || __NetBSD__) /* Maverick EEPROMs do not have the Midband bit set */
#ifdef REGDMN_SUPPORT_11D
            if (((getEepromRD(ol_regdmn_handle) & WORLD_SKU_MASK) == WORLD_SKU_PREFIX) && (cc == CTRY_UNITED_STATES)) {
                if(!isWwrSKU_NoMidband(ol_regdmn_handle) && isFCCMidbandSupported(ol_regdmn_handle))
                    regdmn = FCC3_FCCA;
                else
                    regdmn = FCC1_FCCA;
            }
#endif
#endif /* !(_MAVERICK_STA_ || __NetBSD__) */
        }
#ifdef REGDMN_SUPPORT_11D
    }
#endif
	
#ifndef ATH_NO_5G_SUPPORT
    if (!getWmRD(ol_regdmn_handle, regdmn, ~IEEE80211_CHAN_2GHZ, &rd5GHz)) {
        printk("%s: couldn't find unitary 5GHz reg domain for country %u\n",
             __func__, ol_regdmn_handle->ol_regdmn_countryCode);
        return false;
    }
#endif
    if (!getWmRD(ol_regdmn_handle, regdmn, IEEE80211_CHAN_2GHZ, &rd2GHz)) {
        printk("%s: couldn't find unitary 2GHz reg domain for country %u\n",
             __func__, ol_regdmn_handle->ol_regdmn_countryCode);
        return false;
    }

#ifndef ATH_NO_5G_SUPPORT
#if !(_MAVERICK_STA_ || __NetBSD__) /* Maverick EEPROMs do not have the Midband bit set */
    if (!isWwrSKU(ol_regdmn_handle) && ((rd5GHz.regDmnEnum == FCC1) || (rd5GHz.regDmnEnum == FCC2))) {
        if (isFCCMidbandSupported(ol_regdmn_handle)) {
            /* 
             * FCC_MIDBAND bit in EEPROM is set. Map 5GHz regdmn to FCC3 to enable midband, DFS,
             * and force Unii2/midband to passive.
             */
            if (!getWmRD(ol_regdmn_handle, FCC3_FCCA, ~IEEE80211_CHAN_2GHZ, &rd5GHz)) {
                printk("%s: couldn't find unitary 5GHz reg domain for country %u\n",
                     __func__, ol_regdmn_handle->ol_regdmn_countryCode);
                return false;
            }
        }
    }
#endif /* !(_MAVERICK_STA_ || __NetBSD__) */
#endif


    if (rd5GHz.dfsMask & DFS_FCC3)
        dfsDomain = DFS_FCC_DOMAIN;
    if (rd5GHz.dfsMask & DFS_MKK4)
        dfsDomain = DFS_MKK4_DOMAIN;
    if (rd5GHz.dfsMask & DFS_ETSI)
        dfsDomain = DFS_ETSI_DOMAIN;
    if (dfsDomain != ol_regdmn_handle->ol_regdmn_dfsDomain) {
        ol_regdmn_handle->ol_regdmn_dfsDomain = dfsDomain;
    }
    if(country == AH_NULL) {
        modesAvail = ol_regdmn_getWirelessModes(ol_regdmn_handle);
    }
    else {
        modesAvail = ol_regdmn_getwmodesnreg(ol_regdmn_handle, country, &rd5GHz);

        if (cc == CTRY_DEBUG) {
            if (modesAvail & REGDMN_MODE_11N_MASK) {
                modeSelect &= REGDMN_MODE_11N_MASK;
            }
        }
		/* To skip 5GHZ channel when cc is NULL1_WORLD */
		if (cc == NULL1_WORLD) {
            modeSelect &= ~(REGDMN_MODE_11A|REGDMN_MODE_TURBO|REGDMN_MODE_108A|REGDMN_MODE_11A_HALF_RATE|REGDMN_MODE_11A_QUARTER_RATE
							|REGDMN_MODE_11NA_HT20|REGDMN_MODE_11NA_HT40PLUS|REGDMN_MODE_11NA_HT40MINUS);
        }

        if (!enableOutdoor)
            maxChan = country->outdoorChanStart;
        }

    next = 0;

    is_halfchan_cap = scn_handle->hal_reg_capabilities.regcap1 & REGDMN_CAP1_CHAN_HALF_RATE;
    is_quarterchan_cap = scn_handle->hal_reg_capabilities.regcap1 & REGDMN_CAP1_CHAN_QUARTER_RATE;
    is_49ghz_cap = scn_handle->hal_reg_capabilities.regcap1 & REGDMN_CAP1_CHAN_HAL49GHZ;

#if ICHAN_WAR_SYNCH
//    lock may need on the operartion of IEEE channel list
//    AH_PRIVATE(ah)->ah_ichan_set = true;
//    spin_lock(&AH_PRIVATE(ah)->ah_ichan_lock);
#endif
    for (cm = modes; cm < &modes[N(modes)]; cm++) {
        u_int16_t c, c_hi, c_lo;
        u_int64_t *channelBM=AH_NULL;
        REG_DOMAIN *rd=AH_NULL;
        const REG_DMN_FREQ_BAND *fband=AH_NULL,*freqs;
        int8_t low_adj=0, hi_adj=0;
        u_int16_t vht_ch_freq_seg1;

        if ((cm->mode & modeSelect) == 0) {
            printk("%s: skip mode 0x%x flags 0x%x\n",
                 __func__, cm->mode, cm->flags);
            continue;
        }
        if ((cm->mode & modesAvail) == 0) {
            printk("%s: !avail mode 0x%x (0x%x) flags 0x%x\n",
                 __func__, modesAvail, cm->mode, cm->flags);
            continue;
        }
        if (!ol_get_channel_edges(ol_regdmn_handle, cm->flags, &c_lo, &c_hi)) {
            /* channel not supported by hardware, skip it */
            printk("%s: channels 0x%x not supported by hardware\n",
                 __func__,cm->flags);
            continue;
        }

        switch (cm->mode) {
#ifndef ATH_NO_5G_SUPPORT
        case REGDMN_MODE_TURBO:
            rd = &rd5GHz;
            channelBM = rd->chan11a_turbo;
            freqs = &regDmn5GhzTurboFreq[0];
            ctl = rd->conformance_test_limit | CTL_TURBO;
            break;
        case REGDMN_MODE_11A:
        case REGDMN_MODE_11NA_HT20:
        case REGDMN_MODE_11NA_HT40PLUS:
        case REGDMN_MODE_11NA_HT40MINUS:
        case REGDMN_MODE_11AC_VHT20:
        case REGDMN_MODE_11AC_VHT40PLUS:
        case REGDMN_MODE_11AC_VHT40MINUS:
        case REGDMN_MODE_11AC_VHT80:
            rd = &rd5GHz;
            channelBM = rd->chan11a;
            freqs = &regDmn5GhzFreq[0];
            ctl = rd->conformance_test_limit;
            break;
#endif
        case REGDMN_MODE_11B:
            rd = &rd2GHz;
            channelBM = rd->chan11b;
            freqs = &regDmn2GhzFreq[0];
            ctl = rd->conformance_test_limit | CTL_11B;
            break;
        case REGDMN_MODE_11G:
        case REGDMN_MODE_11NG_HT20:
        case REGDMN_MODE_11NG_HT40PLUS:
        case REGDMN_MODE_11NG_HT40MINUS:
            rd = &rd2GHz;
            channelBM = rd->chan11g;
            freqs = &regDmn2Ghz11gFreq[0];
            ctl = rd->conformance_test_limit | CTL_11G;
            break;
        case REGDMN_MODE_11G_TURBO:
            rd = &rd2GHz;
            channelBM = rd->chan11g_turbo;
            freqs = &regDmn2Ghz11gTurboFreq[0];
            ctl = rd->conformance_test_limit | CTL_108G;
            break;
#ifndef ATH_NO_5G_SUPPORT
        case REGDMN_MODE_11A_TURBO:
            rd = &rd5GHz;
            channelBM = rd->chan11a_dyn_turbo;
            freqs = &regDmn5GhzTurboFreq[0];
            ctl = rd->conformance_test_limit | CTL_108G;
            break;
#endif
        default:
            printk("%s: Unkonwn HAL mode 0x%x\n",
                    __func__, cm->mode);
            continue;
        }
        if (isChanBitMaskZero(channelBM))
            continue;

        if ((cm->mode == REGDMN_MODE_11NA_HT40PLUS) ||
            (cm->mode == REGDMN_MODE_11NG_HT40PLUS) ||
            (cm->mode == REGDMN_MODE_11AC_VHT40PLUS)) {
                hi_adj = -20;
        }

        if ((cm->mode == REGDMN_MODE_11NA_HT40MINUS) ||
            (cm->mode == REGDMN_MODE_11NG_HT40MINUS) || 
	    (cm->mode == REGDMN_MODE_11AC_VHT40MINUS)) {
                low_adj = 20;
        }

        // Walk through the 5G band to find 80 Mhz channel
        if ((cm->mode == REGDMN_MODE_11AC_VHT80) && (rd == &rd5GHz))
        {
            ol_regdmn_init_vht80_chan(&vht80_chans);

            // Find 80 M channels
            for (b=0;b<64*BMLEN; b++) {
                if (IS_BIT_SET(b,channelBM)) {
                    fband = &freqs[b];
                    for (c=fband->lowChannel + 30;
                     ((c <= (fband->highChannel - 30)) &&
                      (c >= (fband->lowChannel + 30)));
                     c += 80) {
                        if((c <= (fband->highChannel - 30)) &&
                            (c >= (fband->lowChannel + 30)))
                            ol_regdmn_add_vht80_chan(&vht80_chans, c);
                     }
                }
            }
        }
	
        if(rd == &rd2GHz) {
            ol_regdmn_handle->ol_regdmn_ctl_2G = ctl;
        }

#ifndef ATH_NO_5G_SUPPORT
        if (rd == &rd5GHz ) {
           ol_regdmn_handle->ol_regdmn_ctl_5G = ctl;
        }
#endif

        for (b=0;b<64*BMLEN; b++) {
            if (IS_BIT_SET(b,channelBM)) {
                fband = &freqs[b];

#ifndef ATH_NO_5G_SUPPORT
                /* 
                 * MKK capability check. U1 ODD, U1 EVEN, U2, and MIDBAND bits are checked here. 
                 * Don't add the band to channel list if the corresponding bit is not set.
                 */
                if ((cm->flags & IEEE80211_CHAN_5GHZ) && (rd5GHz.regDmnEnum == MKK1 || rd5GHz.regDmnEnum == MKK2)) {
                    int i, skipband=0;
                    u_int32_t regcap;

                    for (i = 0; i < N(j_bandcheck); i++) {
                        if (j_bandcheck[i].freqbandbit == b) {
                            regcap = scn_handle->hal_reg_capabilities.regcap2;
                            
                            if ((j_bandcheck[i].eepromflagtocheck & regcap) == 0) {
                                skipband = 1;
                            }
                            break;
                        }
                    }
                    if (skipband) {
                        printk("%s: Skipping %d freq band.\n",
                                __func__, j_bandcheck[i].freqbandbit);
                        continue;
                    }
                }
#endif

                ath_add_regclassid(regclassids, maxregids,
                                   nregids, fband->regClassId);

                if ((rd == &rd5GHz) && (fband->lowChannel < 5180) && (!is_49ghz_cap))
                    continue;

                if (((IS_HT40_MODE(cm->mode)) || (IS_VHT40_MODE(cm->mode))) && (rd == &rd5GHz)) {
                    /* For 5G HT40 mode, channel seperation should be 40. */
                    chanSep = 40;

                    /*
                     * For all 4.9G Hz and Unii1 odd channels, HT40 is not allowed. 
                     * Except for CTRY_DEBUG.
                     */
                    if ((fband->lowChannel < 5180) && (cc != CTRY_DEBUG)) {
                        continue;
                    }

                    /*
                     * This is mainly for F1_5280_5320, where 5280 is not a HT40_PLUS channel.
                     * The way we generate HT40 channels is assuming the freq band starts from 
                     * a HT40_PLUS channel. Shift the low_adj by 20 to make it starts from 5300.
                     */
                    if ((fband->lowChannel == 5280) || (fband->lowChannel == 4920)) {
                        low_adj += 20;
                    }
                }
                else {
                    chanSep = fband->channelSep;

                    if (cm->mode == REGDMN_MODE_11NA_HT20 || cm->mode == REGDMN_MODE_11AC_VHT20) {
                        /*
                         * For all 4.9G Hz and Unii1 odd channels, HT20 is not allowed either. 
                         * Except for CTRY_DEBUG.
                         */
                        if ((fband->lowChannel < 5180) && (cc != CTRY_DEBUG)) {
                            continue;
                        }
                    }
                }

                for (c=fband->lowChannel + low_adj;
                     ((c <= (fband->highChannel + hi_adj)) &&
                      (c >= (fband->lowChannel + low_adj)));
                     c += chanSep) {
                         
                    struct ieee80211_channel icv;

                    if (!(c_lo <= c && c <= c_hi)) {
                        printk("%s: c %u out of range [%u..%u]\n",
                             __func__, c, c_lo, c_hi);
                        continue;
                    }
                    if ((fband->channelBW ==
                            CHANNEL_HALF_BW) &&
                        !is_halfchan_cap) {
                        printk("%s: Skipping %u half rate channel\n",
                                __func__, c);
                        continue;
                    }

                    if ((fband->channelBW ==
                        CHANNEL_QUARTER_BW) &&
                        !is_quarterchan_cap) {
                        printk("%s: Skipping %u quarter rate channel\n",
                                __func__, c);
                        continue;
                    }

                    if (((c+fband->channelSep)/2) > (maxChan+HALF_MAXCHANBW)) {
                        printk("%s: c %u > maxChan %u\n",
                             __func__, c, maxChan);
                        continue;
                    }
                    if (next >= maxchans){
                        printk("%s: too many channels for channel table\n",
                             __func__);
                        goto done;
                    }
                    if ((fband->usePassScan & IS_ECM_CHAN) &&
                        !enableExtendedChannels && (c >= 2467)) {
                        printk("Skipping ecm channel\n");
                        continue;
                    }
                    if ((rd->flags & NO_HOSTAP) &&
                        (scn_handle->sc_ic.ic_opmode == IEEE80211_M_HOSTAP)) {
                        printk("Skipping HOSTAP channel\n");
                        continue;
                    }
#if !(_MAVERICK_STA_ || __NetBSD__)
                    // This code makes sense for only the AP side. The following if's deal specifically
                    // with Owl since it does not support radar detection on extension channels.
                    // For STA Mode, the AP takes care of switching channels when radar detection
                    // happens. Also, for SWAP mode, Apple does not allow DFS channels, and so allowing
                    // HT40 operation on these channels is OK. 
                    if (IS_HT40_MODE(cm->mode) &&
                        !(getEepromRegExtBits(ol_regdmn_handle,REG_EXT_FCC_DFS_HT40)) &&
                        (fband->useDfs) && (rd->conformance_test_limit != MKK)) {
                        /*
                         * Only MKK CTL checks REG_EXT_JAPAN_DFS_HT40 for DFS HT40 support.
                         * All other CTLs should check REG_EXT_FCC_DFS_HT40
                         */
                        printk("Skipping HT40 channel (en_fcc_dfs_ht40 = 0)\n");
                        continue;
                    }
                    if (IS_HT40_MODE(cm->mode) &&
                        !(getEepromRegExtBits(ol_regdmn_handle, REG_EXT_JAPAN_NONDFS_HT40)) &&
                        !(fband->useDfs) && (rd->conformance_test_limit == MKK)) {
                        printk("Skipping HT40 channel (en_jap_ht40 = 0)\n");
                        continue;
                    }
                    if (IS_HT40_MODE(cm->mode) &&
                        !(getEepromRegExtBits(ol_regdmn_handle, REG_EXT_JAPAN_DFS_HT40)) &&
                        (fband->useDfs) && (rd->conformance_test_limit == MKK) ) {
                        printk("Skipping HT40 channel (en_jap_dfs_ht40 = 0)\n");
                        continue;
                    }
#endif

                    if((cm->mode == REGDMN_MODE_11AC_VHT80)) {
                        if((rd == &rd5GHz) && (b == F2_5660_5700)) {
                            /* 
                                For FCC6 channel 132,136,140,144 band.
                                Channel 132 and 136 shall support 80 Mhz channel.
                            */                                
                            vht_ch_freq_seg1 = 5690;
                            if( c == 5700 ) {
                                /*
                                    Skipping 140 VHT80 channel.
                                */
                                printk("Skipping VHT80 channel %d\n", c);
                                continue;                                 
                            }
                        }
                        else if(!ol_regdmn_find_vht80_chan(&vht80_chans, c, &vht_ch_freq_seg1)) {
                            printk("Skipping VHT80 channel %d\n", c);
                            continue;
                        }
                    }
                    else {
                        vht_ch_freq_seg1 = 0;
                    }
					
                    OS_MEMZERO(&icv, sizeof(icv));
                    icv.ic_freq = c;
                    icv.ic_flags = cm->flags;

                    switch (fband->channelBW) {
                        case CHANNEL_HALF_BW:
                            icv.ic_flags |= IEEE80211_CHAN_HALF;
                            break;
                        case CHANNEL_QUARTER_BW:
                            icv.ic_flags |= IEEE80211_CHAN_QUARTER;
                            break;
                    }

                    icv.ic_maxregpower = fband->powerDfs;
                    icv.ic_antennamax = fband->antennaMax;
                    regDmnFlags = rd->flags;

                    icv.ic_regClassId = fband->regClassId;
                    if (fband->usePassScan & rd->pscan)
                    {
                        /* For WG1_2412_2472, the whole band marked as PSCAN_WWR, but only channel 12-13 need to be passive. */
                        if (!(fband->usePassScan & PSCAN_EXT_CHAN) ||
                            (icv.ic_freq >= 2467))
                        {
                            icv.ic_flags |= IEEE80211_CHAN_PASSIVE;
                        }
						else if ((fband->usePassScan & PSCAN_MKKA_G) && (rd->pscan & PSCAN_MKKA_G))
						{
							icv.ic_flags |= IEEE80211_CHAN_PASSIVE;
						}
                        else
                        {
                            icv.ic_flags &= ~IEEE80211_CHAN_PASSIVE;
                        }
                    }
                    else
                    {
                        icv.ic_flags &= ~IEEE80211_CHAN_PASSIVE;
                    }
                    if (fband->useDfs & rd->dfsMask)
                        icv.ic_flagext |= IEEE80211_CHAN_DFS;
                    else
                        icv.ic_flagext = 0;
                    #if 0 /* Enabling 60 sec startup listen for ETSI */
                    /* Don't use 60 sec startup listen for ETSI yet */
                    if (fband->useDfs & rd->dfsMask & DFS_ETSI)
                        icv.ic_flagext |= IEEE80211_CHAN_DFS_CLEAR;
                    #endif

                    /* Check for ad-hoc allowableness */
                    /* To be done: DISALLOW_ADHOC_11A_TURB should allow ad-hoc */
                    if (icv.ic_flagext & IEEE80211_CHAN_DFS) {
                        icv.ic_flagext |= IEEE80211_CHAN_DISALLOW_ADHOC;
                    }
#if 0                    
                    if (icv.regDmnFlags & CHANNEL_NO_HOSTAP) {
                        icv.priv_flags |= CHANNEL_NO_HOSTAP;
                    }

                    if (regDmnFlags & ADHOC_PER_11D) {
                        icv.priv_flags |= CHANNEL_PER_11D_ADHOC;
                    }
#endif
                    if (icv.ic_flags & IEEE80211_CHAN_PASSIVE) {
                        /* Allow channel 1-11 as ad-hoc channel even marked as passive */
                        if ((icv.ic_freq < 2412) || (icv.ic_freq > 2462)) {
                            if (rd5GHz.regDmnEnum == MKK1 || rd5GHz.regDmnEnum == MKK2) {
                                u_int32_t regcap;
                                
                                regcap = scn_handle->hal_reg_capabilities.regcap2;
                                
                                if (!(regcap & (REGDMN_EEPROM_EEREGCAP_EN_KK_U1_EVEN |
                                        REGDMN_EEPROM_EEREGCAP_EN_KK_U2 |
                                        REGDMN_EEPROM_EEREGCAP_EN_KK_MIDBAND)) 
                                        && isUNII1OddChan(icv.ic_freq)) {
                                    /* 
                                     * There is no RD bit in EEPROM. Unii 1 odd channels
                                     * should be active scan for MKK1 and MKK2.
                                     */
                                    icv.ic_flags &= ~IEEE80211_CHAN_PASSIVE;
                                }
                                else {
                                    icv.ic_flagext |= IEEE80211_CHAN_DISALLOW_ADHOC;
                                }
                            }
                            else {
                                icv.ic_flagext |= IEEE80211_CHAN_DISALLOW_ADHOC;
                            }
                        }
                    }
#ifndef ATH_NO_5G_SUPPORT
                    if (cm->mode & (REGDMN_MODE_TURBO | REGDMN_MODE_11A_TURBO)) {
                        if( regDmnFlags & DISALLOW_ADHOC_11A_TURB) {
                            icv.ic_flagext |= IEEE80211_CHAN_DISALLOW_ADHOC;
                        }
                    }
#endif
                    else if (cm->mode & (REGDMN_MODE_11A | REGDMN_MODE_11NA_HT20 |
                        REGDMN_MODE_11NA_HT40PLUS | REGDMN_MODE_11NA_HT40MINUS)) {
                        if( regDmnFlags & (ADHOC_NO_11A | DISALLOW_ADHOC_11A)) {
                            icv.ic_flagext |= IEEE80211_CHAN_DISALLOW_ADHOC;
                        }
                    }

        #ifdef ATH_IBSS_DFS_CHANNEL_SUPPORT
                    /* EV 83788, 83790 support 11A adhoc in ETSI domain. Other
                     * domain will remain as their original settings for now since there
                     * is no requirement.
                     * This will open all 11A adhoc in ETSI, no matter DFS/non-DFS
                     */
                    if (dfsDomain == DFS_ETSI_DOMAIN) {
                        icv.ic_flagext &= ~IEEE80211_CHAN_DISALLOW_ADHOC;
                    }
        #endif
                    icv.ic_ieee = ol_ath_mhz2ieee(ic, icv.ic_freq, icv.ic_flags);
                    if(vht_ch_freq_seg1) {
                        icv.ic_vhtop_ch_freq_seg1 = ol_ath_mhz2ieee(ic, vht_ch_freq_seg1, icv.ic_flags);
                    }
                    else {
                        icv.ic_vhtop_ch_freq_seg1 = 0;
                    }
                    icv.ic_vhtop_ch_freq_seg2 = 0;

                    OS_MEMCPY(&chans[next++], &icv, sizeof(struct ieee80211_channel));
                }
                /* Restore normal low_adj value. */
                if (IS_HT40_MODE(cm->mode) || IS_HT40_MODE(cm->mode)) {
                    if (fband->lowChannel == 5280 || fband->lowChannel == 4920) {
                        low_adj -= 20;
                    }
                }
            }
        }
    }
done:    if (next != 0) {

        /*
         * Keep a private copy of the channel list so we can
         * constrain future requests to only these channels
         */
        ath_hal_sort(chans, next, sizeof(struct ieee80211_channel), chansort);

    }
    *nchans = next;

    ieee80211_set_nchannels(ic, next);

    /* save for later query */
    ol_regdmn_handle->ol_regdmn_currentRDInUse = regdmn;
    ol_regdmn_handle->ol_regdmn_currentRD5G = rd5GHz.regDmnEnum;
    ol_regdmn_handle->ol_regdmn_currentRD2G = rd2GHz.regDmnEnum;
    
    if(country == AH_NULL) {
        ol_regdmn_handle->ol_regdmn_iso[0] = 0;
        ol_regdmn_handle->ol_regdmn_iso[1] = 0;
    }
    else {
        ol_regdmn_handle->ol_regdmn_iso[0] = country->isoName[0];
        ol_regdmn_handle->ol_regdmn_iso[1] = country->isoName[1];
    }

#if ICHAN_WAR_SYNCH
//    lock may need on the operartion of IEEE channel list
//    AH_PRIVATE(ah)->ah_ichan_set = false;
//    spin_unlock(&AH_PRIVATE(ah)->ah_ichan_lock);
#endif

    return (next != 0);
#undef CHANNEL_HALF_BW
#undef CHANNEL_QUARTER_BW
}

/*
 * Insertion sort.
 */
#define ath_hal_swap(_a, _b, _size) {            \
    u_int8_t *s = _b;                    \
    int i = _size;                       \
    do {                                 \
        u_int8_t tmp = *_a;              \
        *_a++ = *s;                      \
        *s++ = tmp;                      \
    } while (--i);                       \
    _a -= _size;                         \
}

static void
ath_hal_sort(void *a, u_int32_t n, u_int32_t size, ath_hal_cmp_t *cmp)
{
    u_int8_t *aa = a;
    u_int8_t *ai, *t;

    for (ai = aa+size; --n >= 1; ai += size)
        for (t = ai; t > aa; t -= size) {
            u_int8_t *u = t - size;
            if (cmp(u, t) <= 0)
                break;
            ath_hal_swap(u, t, size);
        }
}

REGDMN_CTRY_CODE __ahdecl ol_regdmn_findCountryCodeByRegDomain(REGDMN_REG_DOMAIN regdmn)
{
    int i;

    for (i = 0; i < ol_regdmn_Rdt.allCountriesCt; i++) {
        if (ol_regdmn_Rdt.allCountries[i].regDmnEnum == regdmn)
            return (ol_regdmn_Rdt.allCountries[i].countryCode);
    }
    return (0);        /* Not found */
}

void __ahdecl ol_regdmn_getCurrentCountry(struct ol_regdmn* ol_regdmn_handle, IEEE80211_COUNTRY_ENTRY* ctry)
{


#if !_MAVERICK_STA_
    u_int16_t rd = getEepromRD(ol_regdmn_handle);

    ctry->isMultidomain = false;
    if(!(rd & COUNTRY_ERD_FLAG)) {
        ctry->isMultidomain = isWwrSKU(ol_regdmn_handle);
    }
#else
    ctry->isMultidomain = true;
#endif

    ctry->countryCode = ol_regdmn_handle->ol_regdmn_countryCode;
    ctry->regDmnEnum = ol_regdmn_handle->ol_regdmn_current_rd;
    ctry->regDmn5G = ol_regdmn_handle->ol_regdmn_currentRD5G;
    ctry->regDmn2G = ol_regdmn_handle->ol_regdmn_currentRD2G;
    ctry->iso[0] = ol_regdmn_handle->ol_regdmn_iso[0];
    ctry->iso[1] = ol_regdmn_handle->ol_regdmn_iso[1];
}

#undef N

void
ol_80211_channel_setup(struct ieee80211com *ic,
                           enum ieee80211_clist_cmd cmd,
                           struct ieee80211_channel *chans, int nchan,
                           const u_int8_t *regclassids, u_int nregclass,
                           int countryCode)
{

    /*
     * The DFS/NOL management is now done via ic_dfs_clist_update(),
     * rather than via the channel setup API.
     */
    if ((cmd == CLIST_DFS_UPDATE) || (cmd == CLIST_NOL_UPDATE)) {
        adf_os_print("%s: cmd=%d, should not have gotten here!\n",
          __func__,
          cmd);
    }

    if ((countryCode == CTRY_DEFAULT) || (cmd == CLIST_NEW_COUNTRY)) {
        /*
         * channel is ready.
         */
    }
    else {
        /*
         * TBD
         * Logic AND the country channel and domain channels.
         */
    }

    /*
     * Copy regclass ids
     */
    ieee80211_set_regclassids(ic, regclassids, nregclass);
}

static int
ol_ath_set_country(struct ieee80211com *ic, 
                         char *isoName, u_int16_t cc, enum ieee80211_clist_cmd cmd)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ol_regdmn *ol_regdmn_handle;
    struct ieee80211_channel *chans;
    int nchan;
    u_int8_t regclassids[ATH_REGCLASSIDS_MAX];
    u_int nregclass = 0;
    u_int wMode;
    u_int netBand;
    int outdoor = ath_outdoor;
    int indoor = ath_indoor;    
    u_int i;
    
    ol_regdmn_handle = scn->ol_regdmn_handle;
  
    if(!isoName || !isoName[0] || !isoName[1]) {
        if (cc) 
            ol_regdmn_handle->ol_regdmn_countryCode = cc;
        else
            ol_regdmn_handle->ol_regdmn_countryCode = CTRY_DEFAULT;
    } else {
        ol_regdmn_handle->ol_regdmn_countryCode = ol_regdmn_findCountryCode((u_int8_t *)isoName);
 
        /* Map the ISO name ' ', 'I', 'O' */
        if (isoName[2] == 'O') {
            outdoor = AH_TRUE;
            indoor  = AH_FALSE;
        }
        else if (isoName[2] == 'I') {
            indoor  = AH_TRUE;
            outdoor = AH_FALSE;
        }
        else if ((isoName[2] == ' ') || (isoName[2] == 0)) {
            outdoor = AH_FALSE;
            indoor  = AH_FALSE;
        }
        else
            return -EINVAL;
    }
    
    wMode = ic->ic_reg_parm.wModeSelect;
    if (!(wMode & REGDMN_MODE_11A)) {
        wMode &= ~(REGDMN_MODE_TURBO|REGDMN_MODE_108A|REGDMN_MODE_11A_HALF_RATE);
    }
    if (!(wMode & REGDMN_MODE_11G)) {
        wMode &= ~(REGDMN_MODE_108G);
    }
    netBand = ic->ic_reg_parm.netBand;
    if (!(netBand & REGDMN_MODE_11A)) {
        netBand &= ~(REGDMN_MODE_TURBO|REGDMN_MODE_108A|REGDMN_MODE_11A_HALF_RATE);
    }
    if (!(netBand & REGDMN_MODE_11G)) {
        netBand &= ~(REGDMN_MODE_108G);
    }
    wMode &= netBand;

    chans = ic->ic_channels;
                
    if (chans == NULL) {
        printk("%s: unable to allocate channel table\n", __func__);
        return -ENOMEM;
    }
    
    if (!ol_regdmn_init_channels(ol_regdmn_handle, chans, IEEE80211_CHAN_MAX, (u_int *)&nchan,
                               regclassids, ATH_REGCLASSIDS_MAX, &nregclass,
                               ol_regdmn_handle->ol_regdmn_countryCode, wMode, outdoor, 
                               ol_regdmn_handle->ol_regdmn_xchanmode)) {
        return -EINVAL;
    }

    for (i=0; i<nchan; i++) {
        chans[i].ic_maxpower = scn->max_tx_power;
        chans[i].ic_minpower = scn->min_tx_power;
    }

    ol_80211_channel_setup(ic, cmd,
                           chans, nchan, regclassids, nregclass,
                           ol_regdmn_handle->ol_regdmn_countryCode);

#ifdef OL_ATH_DUMP_RD_SPECIFIC_CHANNELS
	ol_ath_dump_rd_specific_channels(ic, ol_regdmn_handle);
#endif

    wmi_unified_pdev_set_regdomain(scn->wmi_handle, ol_regdmn_handle);
    
    return 0;
}

static void
ol_ath_get_currentCountry(struct ieee80211com *ic, IEEE80211_COUNTRY_ENTRY *ctry)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ol_regdmn *ol_regdmn_handle;
    
    ol_regdmn_handle = scn->ol_regdmn_handle;

    ol_regdmn_getCurrentCountry(ol_regdmn_handle, ctry);

    /* If HAL not specific yet, since it is band dependent, use the one we passed in.*/
    if (ctry->countryCode == CTRY_DEFAULT) {
        ctry->iso[0] = 0;
        ctry->iso[1] = 0;
    }
    else if (ctry->iso[0] && ctry->iso[1]) {
        if (ath_outdoor)
            ctry->iso[2] = 'O';
        else if (ath_indoor)
            ctry->iso[2] = 'I';
        else
            ctry->iso[2] = ' ';
    }
    return;
}

static int
ol_ath_set_regdomain(struct ieee80211com *ic, int regdomain)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ol_regdmn *ol_regdmn_handle;
    int ret;
    IEEE80211_REG_PARAMETERS *reg_parm;
    reg_parm = &ic->ic_reg_parm;

    ol_regdmn_handle = scn->ol_regdmn_handle;
 
    if (ol_regdmn_set_regdomain(ol_regdmn_handle, regdomain))
            ret = !ol_regdmn_getchannels(ol_regdmn_handle, CTRY_DEFAULT, AH_FALSE, AH_TRUE, reg_parm);
        else 
            ret = AH_FALSE;

    if (ret == AH_TRUE) 
        return 0;        
    else
        return -EIO;
}

static int
ol_ath_get_dfsdomain(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ol_regdmn *ol_regdmn_handle;
    
    ol_regdmn_handle = scn->ol_regdmn_handle;

    return ol_regdmn_handle->ol_regdmn_dfsDomain;
}

static u_int16_t 
ol_ath_find_countrycode(struct ieee80211com *ic, char* isoName)
{
    /* TBD */
    return 0;
}

/* Regdomain Initialization functions */
int 
ol_regdmn_attach(ol_scn_t scn_handle)
{
    struct ol_regdmn *ol_regdmn_handle;
    struct ieee80211com *ic = &scn_handle->sc_ic;


    ic->ic_get_currentCountry = ol_ath_get_currentCountry;
    ic->ic_set_country = ol_ath_set_country;
    ic->ic_set_regdomain = ol_ath_set_regdomain;
    ic->ic_find_countrycode = ol_ath_find_countrycode;
    ic->ic_get_dfsdomain = ol_ath_get_dfsdomain;

    ol_regdmn_handle = (struct ol_regdmn *)OS_MALLOC(scn_handle->sc_osdev, sizeof(struct ol_regdmn), GFP_ATOMIC);
    if (ol_regdmn_handle == NULL) {
        printk("allocation of ol regdmn handle failed %d \n",sizeof(struct ol_regdmn));
        return 1;
    }
    OS_MEMZERO(ol_regdmn_handle, sizeof(struct ol_regdmn));
    ol_regdmn_handle->scn_handle = scn_handle;
    ol_regdmn_handle->osdev = scn_handle->sc_osdev;
    
    scn_handle->ol_regdmn_handle = ol_regdmn_handle;
    
    ol_regdmn_handle->ol_regdmn_current_rd = 0; /* Current regulatory domain */
    ol_regdmn_handle->ol_regdmn_current_rd_ext = PEREGRINE_RDEXT_DEFAULT;    /* Regulatory domain Extension reg from EEPROM*/
    ol_regdmn_handle->ol_regdmn_countryCode = CTRY_DEFAULT;     /* current country code */
		
    return 0;
}

