/*
 * Copyright (c) 2002-2006 Sam Leffler, Errno Consulting
 * Copyright (c) 2005-2006 Atheros Communications, Inc.
 * Copyright (c) 2010, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wlantype.h"
#include "Channel.h"
#include "DevDeviceFunction.h"
/*
 * When building the HAL proper we use no GPL-contaminated include
 * files and must define these types ourself.  Beware of these being
 * mismatched against the contents of <linux/types.h>
 */

#ifdef LINUX
/* NB: arm defaults to unsigned so be explicit */
typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;

typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned int u_int32_t;
typedef unsigned long long u_int64_t;

typedef unsigned int size_t;
typedef unsigned int u_int;
typedef unsigned long u_long;
#endif //LINUX

#ifdef _WINDOWS
typedef char int8_t;
typedef short int16_t;
typedef long int32_t;
typedef __int64 int64_t;		// th 090715 this is wrong, but we dont seem to have a long long type

typedef unsigned char u_int8_t, uint8_t;
typedef unsigned short u_int16_t, uint16_t;
typedef unsigned long u_int32_t, uint32_t;
typedef unsigned __int64 u_int64_t;// th 090715 this is wrong, but we dont seem to have a long long type
typedef unsigned int u_int;

/* shorthands */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned __int64 u64;// th 090715 this is wrong, but we dont seem to have a long long type
typedef unsigned long u32;
#endif //_WINDOWS

#ifdef __APPLE__
/* NB: arm defaults to unsigned so be explicit */
typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;

typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned int u_int32_t;
typedef unsigned long long u_int64_t;

typedef unsigned long size_t;
typedef unsigned int u_int;
typedef unsigned long u_long;
#endif //__APPLE__

typedef enum {
    false = 0,
    true  = 1
} bool;

#include "Qc9KRegDomain.h"
#include "qc9k_regdomain.h"

#include "DevDeviceFunction.h"
#include "Qc9KEeprom.h"

#include "UserPrint.h"

/* used throughout this file... */
#define N(a)    (sizeof (a) / sizeof (a[0]))


/* 10MHz is half the 11A bandwidth used to determine upper edge freq
   of the outdoor channel */
#define HALF_MAXCHANBW                          10

/* Mask to check whether a domain is a multidomain or a single
   domain */
#define MULTI_DOMAIN_MASK                       0xFF00

#define WORLD_SKU_MASK                          0x00F0
#define WORLD_SKU_PREFIX                        0x0060

#define QC9K_RDEXT_DEFAULT                      0x1F
#define QC9K_MAX_RATE_POWER                     63

#define AR_PHY_CCA_NOM_VAL_QC9K_2GHZ            -110
#define AR_PHY_CCA_NOM_VAL_QC9K_5GHZ            -115
#define AR_PHY_CCA_MIN_GOOD_VAL_QC9K_2GHZ       -125
#define AR_PHY_CCA_MIN_GOOD_VAL_QC9K_5GHZ       -125
#define AR_PHY_CCA_MAX_GOOD_VAL_QC9K_2GHZ       -95
#define AR_PHY_CCA_MAX_GOOD_VAL_QC9K_5GHZ       -100

typedef int ath_hal_cmp_t(const void *, const void *);

static void ath_hal_sort(void *a, A_UINT32 n, A_UINT32 es, ath_hal_cmp_t *cmp);

#define isWwrSKU (((getEepromRD() & WORLD_SKU_MASK) == WORLD_SKU_PREFIX) || (getEepromRD() == WORLD))

#define isUNII1OddChan(ch) ((ch == 5170) || (ch == 5190) || (ch == 5210) || (ch == 5230))

/*
 * By default, the regdomain tables reference the common tables
 * from ah_regdomain_common.h.  These default tables can be replaced
 * by calls to populate_regdomain_tables functions.
 */
#if !_APPLE_DARWIN_
HAL_REG_DMN_TABLES Rdt = {
    ahCmnRegDomainPairs,    /* regDomainPairs */
#ifdef notyet
    ahCmnRegDmnVendorPairs, /* regDmnVendorPairs */
#else
    NULL,                /* regDmnVendorPairs */
#endif
    qc9k_AllCountries,      /* allCountries */
    NULL,                /* customMappings */
    ahCmnRegDomains,        /* regDomains */
    N(ahCmnRegDomainPairs),    /* regDomainPairsCt */
#ifdef notyet
    N(ahCmnRegDmnVendorPairs), /* regDmnVendorPairsCt */
#else
    0,                         /* regDmnVendorPairsCt */
#endif
    N(qc9k_AllCountries),      /* allCountriesCt */
    0,                         /* customMappingsCt */
    N(ahCmnRegDomains),        /* regDomainsCt */
};
#else
/* Apple will assign the regdomain tables at attach time. */
HAL_REG_DMN_TABLES Rdt;
#endif

typedef struct Qc9KRegDomainT
{
    /*
     * State for regulatory domain handling.
     */
    A_UINT16        ah_current_rd;       /* Current regulatory domain */
    A_UINT16        ah_current_rd_ext;    /* Regulatory domain Extension reg from EEPROM*/
    A_UINT16        ah_countryCode;     /* current country code */
    A_UINT16        ah_currentRDInUse;  /* Current 11d domain in used */
    A_UINT16        ah_currentRD5G;     /* Current 5G regulatory domain */
    A_UINT16        ah_currentRD2G;     /* Current 2G regulatory domain */
    char            ah_iso[4];          /* current country iso + NULL */       
    HAL_DFS_DOMAIN  ah_dfsDomain;       /* current dfs domain */
    //START_ADHOC_OPTION  ah_adHocMode;    /* ad-hoc mode handling */
    //bool            ah_commonMode;      /* common mode setting */
    /* NB: 802.11d stuff is not currently used */
    //bool            ah_cc11d;       /* 11d country code */
    //COUNTRY_INFO_LIST   ah_cc11dInfo;     /* 11d country code element */
    //u_int               ah_nchan;       /* valid channels in list */
    //QC9K_CHANNEL_INTERNAL *ah_curchan;   /* current channel */
    //u_int8_t            ah_coverageClass;       /* coverage class */
    //bool                ah_regdomainUpdate;     /* regdomain is updated? */
    //u_int64_t           ah_tsf_channel;         /* tsf @ which last channel change happened */
    //bool                ah_cwCalRequire;
    A_UINT32            hal_wireless_modes;
    A_UINT8             ath_hal_ht_enable;
    A_UINT16            hal_reg_cap;
    A_UINT16            hal_low_2ghz_chan;
    A_UINT16            hal_high_2ghz_chan;
    A_UINT16            hal_low_5ghz_chan;
    A_UINT16            hal_high_5ghz_chan;
    HAL_OPMODE          ah_opmode;
    QC9K_CHANNEL_INTERNAL ah_channels[2000];
    struct noise_floor_limits nf_2GHz;
    struct noise_floor_limits nf_5GHz;
} QC9K_REG_DOMAIN;

QC9K_REG_DOMAIN Qc9KRegDomainInfo;

static const struct cmode acmodes[] = {
	{ HAL_MODE_TURBO,	            CHANNEL_ST},	/* TURBO means 11a Static Turbo */
	{ HAL_MODE_11A,		            CHANNEL_A},
	{ HAL_MODE_11B,		            CHANNEL_B},
	{ HAL_MODE_11G,		            CHANNEL_G},
	{ HAL_MODE_11G_TURBO,	        CHANNEL_108G},
	{ HAL_MODE_108A,	            CHANNEL_108A},
    { HAL_MODE_11NG_HT20,           CHANNEL_G_HT20},
    { HAL_MODE_11NG_HT40PLUS,       CHANNEL_G_HT40PLUS},
    { HAL_MODE_11NG_HT40MINUS,      CHANNEL_G_HT40MINUS},
    { HAL_MODE_11NA_HT20,           CHANNEL_A_HT20},
    { HAL_MODE_11NA_HT40PLUS,       CHANNEL_A_HT40PLUS},
    { HAL_MODE_11NA_HT40MINUS,      CHANNEL_A_HT40MINUS},
    { HAL_MODE_11A_HALF_RATE,       CHANNEL_A_HALF_RATE},
    { HAL_MODE_11A_QUARTER_RATE,    CHANNEL_A_QUARTER_RATE},
    { HAL_MODE_11AC_VHT20_2G,       CHANNEL_G_VHT20},
    { HAL_MODE_11AC_VHT40PLUS_2G,   CHANNEL_G_VHT40PLUS},
    { HAL_MODE_11AC_VHT40MINUS_2G,  CHANNEL_G_VHT40MINUS},
    { HAL_MODE_11AC_VHT20_5G,       CHANNEL_A_VHT20},
    { HAL_MODE_11AC_VHT40PLUS_5G,   CHANNEL_A_VHT40PLUS},
    { HAL_MODE_11AC_VHT40MINUS_5G,  CHANNEL_A_VHT40MINUS},
    { HAL_MODE_11AC_VHT80_0,        CHANNEL_A_VHT80_0},
    { HAL_MODE_11AC_VHT80_1,        CHANNEL_A_VHT80_1},
    { HAL_MODE_11AC_VHT80_2,        CHANNEL_A_VHT80_2},
    { HAL_MODE_11AC_VHT80_3,        CHANNEL_A_VHT80_3},
};

void Qc9KRegDomainInit()
{
    int opFlags, opFlags2;

    Qc9KRegDomainInfo.ah_current_rd = DevDeviceRegulatoryDomainGet();
    Qc9KRegDomainInfo.ah_current_rd_ext = DevDeviceRegulatoryDomain1Get() | QC9K_RDEXT_DEFAULT;

    Qc9KRegDomainInfo.nf_2GHz.nominal = AR_PHY_CCA_NOM_VAL_QC9K_2GHZ;
    Qc9KRegDomainInfo.nf_2GHz.max     = AR_PHY_CCA_MAX_GOOD_VAL_QC9K_2GHZ;
    Qc9KRegDomainInfo.nf_2GHz.min     = AR_PHY_CCA_MIN_GOOD_VAL_QC9K_2GHZ;
    Qc9KRegDomainInfo.nf_5GHz.nominal = AR_PHY_CCA_NOM_VAL_QC9K_5GHZ;
    Qc9KRegDomainInfo.nf_5GHz.max     = AR_PHY_CCA_MAX_GOOD_VAL_QC9K_5GHZ;
    Qc9KRegDomainInfo.nf_5GHz.min     = AR_PHY_CCA_MIN_GOOD_VAL_QC9K_5GHZ;
 
    Qc9KRegDomainInfo.ath_hal_ht_enable = 1;

    /* Construct wireless mode from EEPROM */
    Qc9KRegDomainInfo.hal_wireless_modes = 0;
    opFlags = DevDeviceOpFlagsGet();
    opFlags2 = DevDeviceOpFlags2Get();

    /*if (eeval & WHAL_OPFLAGS_11A) 
    {
        Qc9KRegDomainInfo.hal_wireless_modes |= HAL_MODE_11A |
            ((!Qc9KRegDomainInfo.ath_hal_ht_enable ||
              (eeval & WHAL_OPFLAGS_5G_HT20)) ?  0 :
                (HAL_MODE_11NA_HT20 | ((eeval & WHAL_OPFLAGS_5G_HT40) ? 0 :
                    (HAL_MODE_11NA_HT40PLUS | HAL_MODE_11NA_HT40MINUS))));
    }
    if (eeval & WHAL_OPFLAGS_11G) 
    {
        Qc9KRegDomainInfo.hal_wireless_modes |= HAL_MODE_11B | HAL_MODE_11G |
            ((!Qc9KRegDomainInfo.ath_hal_ht_enable ||
              (eeval & WHAL_OPFLAGS_2G_HT20)) ?  0 :
                (HAL_MODE_11NG_HT20 | ((eeval & WHAL_OPFLAGS_2G_HT40) ? 0 :
                    (HAL_MODE_11NG_HT40PLUS | HAL_MODE_11NG_HT40MINUS))));
    }*/
   

    if (opFlags & WHAL_OPFLAGS_11A) {
        Qc9KRegDomainInfo.hal_wireless_modes |= HAL_MODE_11A;
        if (opFlags & WHAL_OPFLAGS_5G_HT20) {
            Qc9KRegDomainInfo.hal_wireless_modes |= HAL_MODE_11NA_HT20;
        }
        if (opFlags & WHAL_OPFLAGS_5G_HT40) {
            Qc9KRegDomainInfo.hal_wireless_modes |= (HAL_MODE_11NA_HT40PLUS
                                                  | HAL_MODE_11NA_HT40MINUS);
        }
        if (opFlags2 & WHAL_OPFLAGS2_5G_VHT20) {
            Qc9KRegDomainInfo.hal_wireless_modes |= HAL_MODE_11AC_VHT20_5G;
        }
        if (opFlags2 & WHAL_OPFLAGS2_5G_VHT40) {
            Qc9KRegDomainInfo.hal_wireless_modes |= (HAL_MODE_11AC_VHT40PLUS_5G
                                                  | HAL_MODE_11AC_VHT40MINUS_5G);
        }
        if (opFlags2 & WHAL_OPFLAGS2_5G_VHT80) {
            Qc9KRegDomainInfo.hal_wireless_modes |= (HAL_MODE_11AC_VHT80_0
                                                    | HAL_MODE_11AC_VHT80_1
                                                    | HAL_MODE_11AC_VHT80_2
                                                    | HAL_MODE_11AC_VHT80_3);
        }
    }
    if (opFlags & WHAL_OPFLAGS_11G) {
        Qc9KRegDomainInfo.hal_wireless_modes |= (HAL_MODE_11B 
                                              | HAL_MODE_11G);
        if (opFlags & WHAL_OPFLAGS_2G_HT20) {
            Qc9KRegDomainInfo.hal_wireless_modes |= HAL_MODE_11NG_HT20;
        }
        if (opFlags & WHAL_OPFLAGS_2G_HT40) {
            Qc9KRegDomainInfo.hal_wireless_modes |= (HAL_MODE_11NG_HT40PLUS
                                                  | HAL_MODE_11NG_HT40MINUS);
        }
        if (opFlags2 & WHAL_OPFLAGS2_2G_VHT20) {
            Qc9KRegDomainInfo.hal_wireless_modes |= HAL_MODE_11AC_VHT20_2G;
        }
        if (opFlags2 & WHAL_OPFLAGS2_2G_VHT40) {
            Qc9KRegDomainInfo.hal_wireless_modes |= (HAL_MODE_11AC_VHT40PLUS_2G
                                                  | HAL_MODE_11AC_VHT40MINUS_2G);
        }
    }
    if (DevDeviceHalfRate())
    {
        Qc9KRegDomainInfo.hal_wireless_modes |= HAL_MODE_11A_HALF_RATE;
    }
    if (DevDeviceQuarterRate())
    {
        Qc9KRegDomainInfo.hal_wireless_modes |= HAL_MODE_11A_QUARTER_RATE;
    }
    Qc9KRegDomainInfo.hal_low_2ghz_chan = 2312;
    Qc9KRegDomainInfo.hal_high_2ghz_chan = 2732;
    Qc9KRegDomainInfo.hal_low_5ghz_chan = 4920;
    Qc9KRegDomainInfo.hal_high_5ghz_chan = 6100;

    /* Read regulatory domain flag */
    if (Qc9KRegDomainInfo.ah_current_rd_ext & (1 << REG_EXT_JAPAN_MIDBAND)) 
    {
        /*
         * If REG_EXT_JAPAN_MIDBAND is set, turn on U1 EVEN, U2, and MIDBAND.
         */
        Qc9KRegDomainInfo.hal_reg_cap =
            AR_EEPROM_EEREGCAP_EN_KK_NEW_11A |
            AR_EEPROM_EEREGCAP_EN_KK_U1_EVEN |
            AR_EEPROM_EEREGCAP_EN_KK_U2      |
            AR_EEPROM_EEREGCAP_EN_KK_MIDBAND;
    } 
    else 
    {
        Qc9KRegDomainInfo.hal_reg_cap =
            AR_EEPROM_EEREGCAP_EN_KK_NEW_11A | AR_EEPROM_EEREGCAP_EN_KK_U1_EVEN;
    }

    /* For AR9300 and above, midband channels are always supported */
    Qc9KRegDomainInfo.hal_reg_cap |= AR_EEPROM_EEREGCAP_EN_FCC_MIDBAND;
}

void Qc9KRegulatoryDomainOverride(unsigned int regdmn)
{
    Qc9KRegDomainInfo.ah_current_rd = regdmn;
}

static int chansort(const void *a, const void *b)
{
#define CHAN_FLAGS    (CHANNEL_ALL|CHANNEL_HALF|CHANNEL_QUARTER)
    const QC9K_CHANNEL_INTERNAL *ca = a;
    const QC9K_CHANNEL_INTERNAL *cb = b;

    return (ca->channel == cb->channel) ?
                ((ca->channel_flags & CHAN_FLAGS) - (cb->channel_flags & CHAN_FLAGS)) :
                (ca->channel - cb->channel);
#undef CHAN_FLAGS
}

static A_UINT16 getEepromRD()
{
    return Qc9KRegDomainInfo.ah_current_rd &~ WORLDWIDE_ROAMING_FLAG;
}

/*
 * Test to see if the bitmask array is all zeros
 */
static bool isChanBitMaskZero(u_int64_t *bitmask)
{
    int i;

    for (i=0; i<BMLEN; i++) 
    {
        if (bitmask[i] != 0)
            return false;
    }
    return true;
}

/*
 * Return whether or not the regulatory domain/country in EEPROM
 * is acceptable.
 */
static bool isEepromValid()
{
    A_UINT16 rd = getEepromRD();
    int i;

    if (rd & COUNTRY_ERD_FLAG) 
    {
        A_UINT16 cc = rd &~ COUNTRY_ERD_FLAG;
        for (i = 0; i < Rdt.allCountriesCt; i++)
        {
            if (Rdt.allCountries[i].countryCode == cc)
            {
                return true;
            }
        }
    } 
    else 
    {
        for (i = 0; i < Rdt.regDomainPairsCt; i++)
        {
            if (Rdt.regDomainPairs[i].regDmnEnum == rd)
            {
                return true;
            }
        }
    }
    UserPrint("%Invalid regulatory domain/country code 0x%x\n", rd);
    return false;
}

#if !(_APPLE_DARWIN_ || _APPLE_NETBSD_) /* Apple EEPROMs do not have the Midband bit set */
/*
 * Return whether or not the FCC Mid-band flag is set in EEPROM
 */
static bool isFCCMidbandSupported()
{
    return ((Qc9KRegDomainInfo.hal_reg_cap & AR_EEPROM_EEREGCAP_EN_FCC_MIDBAND) ? true : false);
}
#endif /* !(_APPLE_DARWIN_ || _APPLE_NETBSD_) */

static const REG_DMN_PAIR_MAPPING *getRegDmnPair(A_UINT16 reg_dmn)
{
    int i;
    for (i = 0; i < Rdt.regDomainPairsCt; i++) 
    {
        if (Rdt.regDomainPairs[i].regDmnEnum == reg_dmn) 
        {
            return &Rdt.regDomainPairs[i];
        }
    }
    return NULL;
}

/*
 * Returns whether or not the specified country code
 * is allowed by the EEPROM setting
 */
static bool isCountryCodeValid(A_UINT16 cc)
{
    A_UINT16  rd;
    int i, support5G, support2G;
    u_int modesAvail;
    const COUNTRY_CODE_TO_ENUM_RD_AC *country = NULL;
    const REG_DMN_PAIR_MAPPING *regDmn_eeprom, *regDmn_input;

    /* Default setting requires no checks */
    if (cc == CTRY_DEFAULT)
        return true;

#ifdef AH_DEBUG_COUNTRY
    if (cc == CTRY_DEBUG)
        return true;
#endif
    rd = getEepromRD();
    UserPrint("EEPROM regdomain 0x%x\n", rd);

    if (rd & COUNTRY_ERD_FLAG) 
    {
        /* EEP setting is a country - config shall match */
        UserPrint("EEPROM setting is country code %u\n", rd &~ COUNTRY_ERD_FLAG);
        return (cc == (rd & ~COUNTRY_ERD_FLAG));
    }

#if _APPLE_DARWIN_
    for (i=0; i < Rdt.customMappingsCt; i++) {
        if (Rdt.customMappings[i].regDmnEnum == rd) {
            int j=0;
            while(Rdt.customMappings[i].countryMappings[j].isoName != NULL) {
                if (cc == Rdt.customMappings[i].countryMappings[j].countryCode)
                    return true;
                ++j;
            }
        }
    }
#endif
    for (i = 0; i < Rdt.allCountriesCt; i++) 
    {
        if (cc == Rdt.allCountries[i].countryCode) 
        {
#ifdef AH_SUPPORT_11D
            if ((rd & WORLD_SKU_MASK) == WORLD_SKU_PREFIX) {
                return true;
            }
#endif
            country = &Rdt.allCountries[i];
            if (country->regDmnEnum == rd ||
                rd == DEBUG_REG_DMN || rd == NO_ENUMRD) {
                return true;
            }
        }
    }
    if (country == NULL) return false;

    /* consider device capability and look into the regdmn for 2G&5G */
    modesAvail = Qc9KRegDomainInfo.hal_wireless_modes;
    support5G = modesAvail & (HAL_MODE_11A | HAL_MODE_TURBO |
                HAL_MODE_108A | HAL_MODE_11A_HALF_RATE |
                HAL_MODE_11A_QUARTER_RATE |
                HAL_MODE_11NA_HT40PLUS |
                HAL_MODE_11NA_HT40MINUS |
                HAL_MODE_11AC_VHT20_5G |
                HAL_MODE_11AC_VHT40PLUS_5G |
                HAL_MODE_11AC_VHT40MINUS_5G |
                HAL_MODE_11AC_VHT80_0 |
                HAL_MODE_11AC_VHT80_1 |
                HAL_MODE_11AC_VHT80_2 |
                HAL_MODE_11AC_VHT80_3);
    support2G = modesAvail & (HAL_MODE_11G | HAL_MODE_11B | HAL_MODE_PUREG |
                HAL_MODE_108G |
                HAL_MODE_11NG_HT40PLUS |
                HAL_MODE_11NG_HT40MINUS |
                HAL_MODE_11AC_VHT20_2G |
                HAL_MODE_11AC_VHT40PLUS_2G |
                HAL_MODE_11AC_VHT40MINUS_2G);

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
        UserPrint("Neither 2G nor 5G is supported\n");
    }
    return false;
}

/*
 * Return the mask of available acmodes based on the hardware
 * capabilities and the specified country code and reg domain.
 */
static u_int ath_hal_getwmodesnreg(const COUNTRY_CODE_TO_ENUM_RD_AC *country, REG_DOMAIN *rd5GHz)
{
    u_int modesAvail;

    /* Get acmodes that HW is capable of */
    modesAvail = Qc9KRegDomainInfo.hal_wireless_modes;

    /* Check country regulations for allowed acmodes */
#ifndef ATH_NO_5G_SUPPORT
    if ((modesAvail & (HAL_MODE_11A_TURBO|HAL_MODE_TURBO)) && (!country->allow11aTurbo))
    {
        modesAvail &= ~(HAL_MODE_11A_TURBO | HAL_MODE_TURBO);
    }
#endif
    if ((modesAvail & HAL_MODE_11G_TURBO) && (!country->allow11gTurbo))
    {
        modesAvail &= ~HAL_MODE_11G_TURBO;
    }
    if ((modesAvail & HAL_MODE_11G) && (!country->allow11g))
    {
        modesAvail &= ~HAL_MODE_11G;
    }
#ifndef ATH_NO_5G_SUPPORT
    if ((modesAvail & HAL_MODE_11A) && (isChanBitMaskZero(rd5GHz->chan11a)))
    {
        modesAvail &= ~HAL_MODE_11A;
    }
#endif
    if ((modesAvail & HAL_MODE_11NG_HT20) && (!country->allow11ng20))
    {
        modesAvail &= ~HAL_MODE_11NG_HT20;
    }
    if ((modesAvail & HAL_MODE_11NA_HT20) && (!country->allow11na20))
    {
        modesAvail &= ~HAL_MODE_11NA_HT20;
    }
    if ((modesAvail & HAL_MODE_11NG_HT40PLUS) && (!country->allow11ng40))
    {
        modesAvail &= ~HAL_MODE_11NG_HT40PLUS;
    }
    if ((modesAvail & HAL_MODE_11NG_HT40MINUS) && (!country->allow11ng40))
    {
        modesAvail &= ~HAL_MODE_11NG_HT40MINUS;
    }
    if ((modesAvail & HAL_MODE_11NA_HT40PLUS) && (!country->allow11na40))
    {
        modesAvail &= ~HAL_MODE_11NA_HT40PLUS;
    }
    if ((modesAvail & HAL_MODE_11NA_HT40MINUS) && (!country->allow11na40))
    {
        modesAvail &= ~HAL_MODE_11NA_HT40MINUS;
    }
    if (!country->allow11ac20_2G)
    {
        modesAvail &= ~HAL_MODE_11AC_VHT20_2G;
    }
    if (!country->allow11ac40_2G)
    {
        modesAvail &= ~(HAL_MODE_11AC_VHT40PLUS_2G | HAL_MODE_11AC_VHT40MINUS_2G);
    }
    if (!country->allow11ac20_5G)
    {
        modesAvail &= ~HAL_MODE_11AC_VHT20_5G;
    }
    if (!country->allow11ac40_5G)
    {
        modesAvail &= ~(HAL_MODE_11AC_VHT40PLUS_5G | HAL_MODE_11AC_VHT40MINUS_5G);
    }
    if (!country->allow11ac80)
    {
        modesAvail &= ~(HAL_MODE_11AC_VHT80_0 | HAL_MODE_11AC_VHT80_1 | HAL_MODE_11AC_VHT80_2 | HAL_MODE_11AC_VHT80_3);
    }
    return modesAvail;
}

/*
 * Find the pointer to the country element in the country table
 * corresponding to the country code
 */
static const COUNTRY_CODE_TO_ENUM_RD_AC* findCountry(A_UINT16 countryCode, A_UINT16 rd)
{
    int i;

#if _APPLE_DARWIN_
    for (i=0; i < Rdt.customMappingsCt; i++) {
        if (Rdt.customMappings[i].regDmnEnum == rd) {
            int j=0;
            while(Rdt.customMappings[i].countryMappings[j].isoName != NULL) {
                if (countryCode == Rdt.customMappings[i].countryMappings[j].countryCode)
                    return &Rdt.customMappings[i].countryMappings[j];
                ++j;
            }
            i=Rdt.customMappingsCt;
        }
    }
#endif
    for (i = 0; i < Rdt.allCountriesCt; i++) {
        if (Rdt.allCountries[i].countryCode == countryCode)
            return (&Rdt.allCountries[i]);
    }
    return (NULL);        /* Not found */
}

/*
 * Calculate a default country based on the EEPROM setting.
 */
static A_UINT16 getDefaultCountry()
{
    A_UINT16 rd;
    int i;

    rd =getEepromRD();
    if (rd & COUNTRY_ERD_FLAG) 
    {
        const COUNTRY_CODE_TO_ENUM_RD_AC *country=NULL;
        A_UINT16 cc= rd & ~COUNTRY_ERD_FLAG;

        country = findCountry(cc, rd);
        if (country != NULL)
        {
            return cc;
        }
    }
    /*
     * Check reg domains that have only one country
     */
    for (i = 0; i < Rdt.regDomainPairsCt; i++) 
    {
        if (Rdt.regDomainPairs[i].regDmnEnum == rd) {
            if (Rdt.regDomainPairs[i].singleCC != 0) {
                return Rdt.regDomainPairs[i].singleCC;
            } else {
                i = Rdt.regDomainPairsCt;
            }
        }
    }
    return CTRY_DEFAULT;
}

static bool isValidRegDmn(int reg_dmn, REG_DOMAIN *rd)
{
    int i;

    for (i = 0; i < Rdt.regDomainsCt; i++) {
        if (Rdt.regDomains[i].regDmnEnum == reg_dmn) {
            if (rd != NULL) {
                memcpy(rd, &Rdt.regDomains[i], sizeof(REG_DOMAIN));
            }
            return true;
        }
    }
    return false;
}

static bool isValidRegDmnPair(int regDmnPair)
{
    int i;

    if (regDmnPair == NO_ENUMRD) return false;

    for (i = 0; i < Rdt.regDomainPairsCt; i++) {
        if (Rdt.regDomainPairs[i].regDmnEnum == regDmnPair) return true;
    }
    return false;
}

/*
 * Return the Wireless Mode Regulatory Domain based
 * on the country code and the wireless mode.
 */
static bool getWmRD(int reg_dmn, A_UINT16 channelFlag, REG_DOMAIN *rd)
{
    int i, found;
    u_int64_t flags=NO_REQ;
    const REG_DMN_PAIR_MAPPING *regPair=NULL;
#ifdef notyet
    VENDOR_PAIR_MAPPING *vendorPair=NULL;
#endif
    int regOrg;

    regOrg = reg_dmn;

    if (reg_dmn == CTRY_DEFAULT) 
    {
        A_UINT16 rdnum;
        rdnum =getEepromRD();

        if (!(rdnum & COUNTRY_ERD_FLAG)) {
            if (isValidRegDmn(rdnum, NULL) ||
                isValidRegDmnPair(rdnum)) {
                reg_dmn = rdnum;
            }
        }
    }
    if ((reg_dmn & MULTI_DOMAIN_MASK) == 0) 
    {
        for (i = 0, found = 0; (i < Rdt.regDomainPairsCt) && (!found); i++) 
        {
            if (Rdt.regDomainPairs[i].regDmnEnum == reg_dmn) 
            {
                regPair = &Rdt.regDomainPairs[i];
                found = 1;
            }
        }
        if (!found) 
        {
            UserPrint("Failed to find reg domain pair %u\n", reg_dmn);
            return false;
        }

        if (!(channelFlag & CHANNEL_2GHZ)) 
        {
            reg_dmn = regPair->regDmn5GHz;
            flags = regPair->flags5GHz;
        }
        if (channelFlag & CHANNEL_2GHZ) 
        {
            reg_dmn = regPair->regDmn2GHz;
            flags = regPair->flags2GHz;
        }
#ifdef notyet
        for (i=0, found=0; (i < Rdt.regDomainVendorPairsCt) && (!found); i++) {
            if ((regDomainVendorPairs[i].regDmnEnum == reg_dmn) &&
                (Qc9KRegDomainInfo.ah_vendor == regDomainVendorPairs[i].vendor)) {
                vendorPair = &regDomainVendorPairs[i];
                found = 1;
            }
        }

        if (found) {
            if (!(channelFlag & CHANNEL_2GHZ)) {
                flags &= vendorPair->flags5GHzIntersect;
                flags |= vendorPair->flags5GHzUnion;
            }
            if (channelFlag & CHANNEL_2GHZ) {
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

    if (!found) 
    {
        UserPrint("Failed to find unitary reg domain %u\n", reg_dmn);
        return false;
    } 
    else 
    {
        if (regPair == NULL) 
        {
            UserPrint("No set reg domain pair\n");
            return false;
        }
        rd->pscan &= regPair->pscanMask;
        
        if (((regOrg & MULTI_DOMAIN_MASK) == 0) && (flags != NO_REQ)) 
        {
            rd->flags = (u_int32_t)flags;
        }
        /*
         * Use only the domain flags that apply to the current mode.
         * In particular, do not apply 11A Adhoc flags to b/g acmodes.
         */
        rd->flags &= (channelFlag & CHANNEL_2GHZ) ? REG_DOMAIN_2GHZ_MASK : REG_DOMAIN_5GHZ_MASK;
        return true;
    }
}

static bool IS_BIT_SET(int bit, u_int64_t *bitmask)
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
static void ath_add_regclassid(A_UINT8 *regclassids, u_int maxregids, u_int *nregids, A_UINT8 regclassid)
{
    u_int i;

    /* Is regclassid valid? */
    if (regclassid == 0)
        return;

    for (i=0; i < maxregids; i++) 
    {
        if (regclassids[i] == regclassid)
            return;
        if (regclassids[i] == 0)
            break;
    }

    if (i == maxregids)
    {
        return;
    }
    else 
    {
        regclassids[i] = regclassid;
        *nregids += 1;
    }
    return;
}

#if !(_APPLE_DARWIN_ || _APPLE_NETBSD_)
static bool getEepromRegExtBits(REG_EXT_BITMAP bit)
{
    return ((Qc9KRegDomainInfo.ah_current_rd_ext & (1<<bit))? true : false);
}
#endif

/* Initialize NF cal history buffer */
static void ath_hal_init_NF_buffer(QC9K_CHANNEL_INTERNAL *ichans, int nchans)
{
    //int i, j,;
    int next, nf_hist_len;

#ifdef ATH_NF_PER_CHAN
    nf_hist_len = HAL_NF_CAL_HIST_LEN_FULL;
#else
    nf_hist_len = HAL_NF_CAL_HIST_LEN_SMALL;
#endif

    for (next = 0; next < nchans; next++) 
    {
        int16_t noisefloor;
        noisefloor = IS_CHAN_2GHZ(&ichans[next]) ?
                        Qc9KRegDomainInfo.nf_2GHz.nominal :
                        Qc9KRegDomainInfo.nf_5GHz.nominal;
        //ichans[next].nf_cal_hist.base.curr_index = 0;
        //for (i = 0; i < NUM_NF_READINGS; i++) {
        //    ichans[next].nf_cal_hist.base.priv_nf[i] = noisefloor;
        //    for (j = 0; j < nf_hist_len; j++) {
        //        ichans[next].nf_cal_hist.nf_cal_buffer[j][i] = noisefloor;
        //    }
        //}
    }
}

static bool getChannelEdges(u_int16_t flags, u_int16_t *low, u_int16_t *high)
{
    if (flags & CHANNEL_5GHZ) 
    {
        *low = Qc9KRegDomainInfo.hal_low_5ghz_chan;
        *high = Qc9KRegDomainInfo.hal_high_5ghz_chan;
        return true;
    }
    if ((flags & CHANNEL_2GHZ)) 
    {
        *low = Qc9KRegDomainInfo.hal_low_2ghz_chan;
        *high = Qc9KRegDomainInfo.hal_high_2ghz_chan;
        return true;
    }
    return false;
}

/*
 * Insertion sort.
 */
#define ath_hal_swap(_a, _b, _size) { \
    A_UINT8 *s = _b; \
    int i = _size; \
    do { \
        A_UINT8 tmp = *_a; \
        *_a++ = *s; \
        *s++ = tmp; \
    } while (--i); \
    _a -= _size; \
}

static void ath_hal_sort(void *a, A_UINT32 n, A_UINT32 size, ath_hal_cmp_t *cmp)
{
    A_UINT8 *aa = a;
    A_UINT8 *ai, *t;

    for (ai = aa+size; --n >= 1; ai += size)
    {
        for (t = ai; t > aa; t -= size) 
        {
            A_UINT8 *u = t - size;
            if (cmp(u, t) <= 0)
                break;
            ath_hal_swap(u, t, size);
        }
    }
}

#define IS_HT40_MODE(_mode) \
                    (((_mode == HAL_MODE_11NA_HT40PLUS  || \
                     _mode == HAL_MODE_11NG_HT40PLUS    || \
                     _mode == HAL_MODE_11NA_HT40MINUS   || \
                     _mode == HAL_MODE_11NG_HT40MINUS   || \
                     _mode == HAL_MODE_11AC_VHT40PLUS_2G  || \
                     _mode == HAL_MODE_11AC_VHT40MINUS_2G || \
                     _mode == HAL_MODE_11AC_VHT40PLUS_5G  || \
                     _mode == HAL_MODE_11AC_VHT40MINUS_5G ) ? true : false))

/*
 * Setup the channel list based on the information in the EEPROM and
 * any supplied country code.  Note that we also do a bunch of EEPROM
 * verification here and setup certain regulatory-related access
 * control data used later on.
 */
int Qc9KInitChannels(int *chans, int *flags, A_UINT32 maxchans, unsigned int *nchans,
              A_UINT8 *regclassids, A_UINT32 maxregids, A_UINT32 *nregids,
              int cc, A_UINT32 modeSelect,
              int enableOutdoor, int enableExtendedChannels)
{
#define CHANNEL_HALF_BW        10
#define CHANNEL_QUARTER_BW    5

    u_int modesAvail;
    A_UINT16 maxChan=7000;
    const COUNTRY_CODE_TO_ENUM_RD_AC *country=NULL;
    REG_DOMAIN rd5GHz, rd2GHz;
    const struct cmode *cm;
    QC9K_CHANNEL_INTERNAL *ichans=&Qc9KRegDomainInfo.ah_channels[0];
    int next=0,b;
    A_UINT8 ctl;
    int is_quarterchan_cap, is_halfchan_cap, is_49ghz_cap;
    int dfsDomain=0;
    int regdmn;
    A_UINT16 chanSep;
    int userPrintFlag;

    UserPrint("cc %u mode 0x%x%s%s\n",
                cc, modeSelect, enableOutdoor? " Enable outdoor" : " ",
                enableExtendedChannels ? " Enable ecm" : "");

    /*
     * We now have enough state to validate any country code
     * passed in by the caller.
     */
    if (!isCountryCodeValid(cc)) 
    {
        /* NB: Atheros silently ignores invalid country codes */
        UserPrint("invalid country code %d\n", cc);
        return false;
    }

    /*
     * Validate the EEPROM setting and setup defaults
     */
    if (!isEepromValid()) 
    {
        /*
         * Don't return any channels if the EEPROM has an
         * invalid regulatory domain/country code setting.
         */
        UserPrint("invalid EEPROM contents\n");
        return false;
    }

    Qc9KRegDomainInfo.ah_countryCode = getDefaultCountry();

#if (AH_RADAR_CALIBRATE != 0)
    if ((Qc9KRegDomainInfo.ah_devid != AR5210_AP) ||
        (Qc9KRegDomainInfo.ah_devid != AR5210_PROD) ||
        (Qc9KRegDomainInfo.ah_devid != AR5210_DEFAULT) ||
        (Qc9KRegDomainInfo.ah_devid != AR5211_DEVID) ||
        (Qc9KRegDomainInfo.ah_devid != AR5311_DEVID) ||
        (Qc9KRegDomainInfo.ah_devid != AR5211_FPGA11B) ||
        (Qc9KRegDomainInfo.ah_devid != AR5211_DEFAULT)) {
        HDPRINTF(ah, HAL_DBG_UNMASKABLE, "Overriding country code and reg domain for DFS testing\n");
        Qc9KRegDomainInfo.ah_countryCode = CTRY_UZBEKISTAN;
        Qc9KRegDomainInfo.ah_current_rd = FCC3_FCCA;
    }
#endif

    if (Qc9KRegDomainInfo.ah_countryCode == CTRY_DEFAULT) 
    {
        /*
         * XXX - TODO: Switch based on Japan country code
         * to new reg domain if valid japan card
         */
#if _APPLE_DARWIN_
        if ((Qc9KRegDomainInfo.ah_current_rd == APL9_WORLD) ||
            (Qc9KRegDomainInfo.ah_current_rd == MKK8_MKKA))
            /* NB: only one country code is allowed in this SKU */
            Qc9KRegDomainInfo.ah_countryCode &= COUNTRY_CODE_MASK;
        else
            Qc9KRegDomainInfo.ah_countryCode = cc & COUNTRY_CODE_MASK;
        if(Qc9KRegDomainInfo.ah_current_rd == 0) /* Unknown locale, currently merlin doesn't have this value programmed --AJAYP */
            Qc9KRegDomainInfo.ah_current_rd = WOR4_WORLD;
#else
        Qc9KRegDomainInfo.ah_countryCode = cc & COUNTRY_CODE_MASK;

        if ((Qc9KRegDomainInfo.ah_countryCode == CTRY_DEFAULT) &&
                (getEepromRD() == CTRY_DEFAULT)) 
        {
                /* Set to DEBUG_REG_DMN for debug or set to CTRY_UNITED_STATES for testing */
                Qc9KRegDomainInfo.ah_countryCode = CTRY_UNITED_STATES;
        }
#endif
    }

#ifdef AH_SUPPORT_11D
    if(Qc9KRegDomainInfo.ah_countryCode == CTRY_DEFAULT)
    {
        regdmn =getEepromRD(ah);
        country = NULL;
    }
    else 
    {
#endif
        /* Get pointers to the country element and the reg domain elements */
        country = findCountry(Qc9KRegDomainInfo.ah_countryCode, getEepromRD());

        if (country == NULL) 
        {
            UserPrint("Country is NULL!!!!, cc= %d\n", Qc9KRegDomainInfo.ah_countryCode);
            return false;
        } 
        else 
        {
            regdmn = country->regDmnEnum;
#if !(_APPLE_DARWIN_ || _APPLE_NETBSD_) /* Apple EEPROMs do not have the Midband bit set */
#ifdef AH_SUPPORT_11D
            if (((getEepromRD(ah) & WORLD_SKU_MASK) == WORLD_SKU_PREFIX) && (cc == CTRY_UNITED_STATES)) {
                if(!isWwrSKU_NoMidband && isFCCMidbandSupported())
                    regdmn = FCC3_FCCA;
                else
                    regdmn = FCC1_FCCA;
            }
#endif
#endif /* !(_APPLE_DARWIN_ || _APPLE_NETBSD_) */
        }
#ifdef AH_SUPPORT_11D
    }
#endif

#ifndef ATH_NO_5G_SUPPORT
    if (!getWmRD(regdmn, ~CHANNEL_2GHZ, &rd5GHz)) 
    {
        UserPrint("Couldn't find unitary 5GHz reg domain for country %u\n", Qc9KRegDomainInfo.ah_countryCode);
        return false;
    }
#endif
    if (!getWmRD(regdmn, CHANNEL_2GHZ, &rd2GHz)) 
    {
        UserPrint("Couldn't find unitary 2GHz reg domain for country %u\n", Qc9KRegDomainInfo.ah_countryCode);
        return false;
    }

#ifndef ATH_NO_5G_SUPPORT
#if !(_APPLE_DARWIN_ || _APPLE_NETBSD_) /* Apple EEPROMs do not have the Midband bit set */
    if (!isWwrSKU && ((rd5GHz.regDmnEnum == FCC1) || (rd5GHz.regDmnEnum == FCC2))) 
    {
        if (isFCCMidbandSupported()) 
        {
            /* 
             * FCC_MIDBAND bit in EEPROM is set. Map 5GHz regdmn to FCC3 to enable midband, DFS,
             * and force Unii2/midband to passive.
             */
            if (!getWmRD(FCC3_FCCA, ~CHANNEL_2GHZ, &rd5GHz)) 
            {
                UserPrint("Couldn't find unitary 5GHz reg domain for country %u\n", Qc9KRegDomainInfo.ah_countryCode);
                return false;
            }
        }
    }
#endif /* !(_APPLE_DARWIN_ || _APPLE_NETBSD_) */
#endif

    if (rd5GHz.dfsMask & DFS_FCC3)
    {
        dfsDomain = DFS_FCC_DOMAIN;
    }
    if (rd5GHz.dfsMask & DFS_MKK4)
    {
        dfsDomain = DFS_MKK4_DOMAIN;
    }
    if (rd5GHz.dfsMask & DFS_ETSI)
    {
        dfsDomain = DFS_ETSI_DOMAIN;
    }
    if (dfsDomain != Qc9KRegDomainInfo.ah_dfsDomain) 
    {
        Qc9KRegDomainInfo.ah_dfsDomain = dfsDomain;
    }
    if(country == NULL) 
    {
        modesAvail = Qc9KRegDomainInfo.hal_wireless_modes;
    }
    else 
    {
        modesAvail = ath_hal_getwmodesnreg(country, &rd5GHz);
        if (cc == CTRY_DEBUG) 
        {
            if (modesAvail & HAL_MODE_ALL_MASK)
            {
                modeSelect &= HAL_MODE_ALL_MASK;
            }
            //if (modesAvail & HAL_MODE_11N_MASK) 
            //{
            //    modeSelect &= HAL_MODE_11N_MASK;
            //}
        }
		/* To skip 5GHZ channel when cc is NULL1_WORLD */
		if (cc == NULL1_WORLD) 
        {
            modeSelect &= ~(HAL_MODE_11A|HAL_MODE_TURBO|HAL_MODE_108A|HAL_MODE_11A_HALF_RATE|HAL_MODE_11A_QUARTER_RATE
							|HAL_MODE_11NA_HT20|HAL_MODE_11NA_HT40PLUS|HAL_MODE_11NA_HT40MINUS
                            |HAL_MODE_11AC_VHT20_2G|HAL_MODE_11AC_VHT40PLUS_2G|HAL_MODE_11AC_VHT40MINUS_2G
                            |HAL_MODE_11AC_VHT20_5G|HAL_MODE_11AC_VHT40PLUS_5G|HAL_MODE_11AC_VHT40MINUS_5G
                            |HAL_MODE_11AC_VHT80_0|HAL_MODE_11AC_VHT80_1|HAL_MODE_11AC_VHT80_2|HAL_MODE_11AC_VHT80_3);
        }
        if (!enableOutdoor)
        {
            maxChan = country->outdoorChanStart;
        }
    }
    next = 0;
    if (maxchans > N(Qc9KRegDomainInfo.ah_channels))
    {
        maxchans = N(Qc9KRegDomainInfo.ah_channels);
    }
    is_halfchan_cap = DevDeviceHalfRate();
    is_quarterchan_cap = DevDeviceQuarterRate();
    is_49ghz_cap = DevDeviceIs4p9GHz();

#if ICHAN_WAR_SYNCH
    Qc9KRegDomainInfo.ah_ichan_set = true;
    spin_lock(&Qc9KRegDomainInfo.ah_ichan_lock);
#endif
    for (cm = acmodes; cm < &acmodes[N(acmodes)]; cm++) 
    {
        A_UINT16 c, c_hi, c_lo;
        u_int64_t *channelBM=NULL;
        REG_DOMAIN *rd=NULL;
        const REG_DMN_FREQ_BAND *fband=NULL,*freqs;
        int8_t low_adj=0, hi_adj=0;

        if ((cm->mode & modeSelect) == 0) 
        {
            UserPrint("Skip mode 0x%x flags 0x%x\n", cm->mode, cm->flags);
            continue;
        }
        if ((cm->mode & modesAvail) == 0) 
        {
            UserPrint("!Avail mode 0x%x (0x%x) flags 0x%x\n", modesAvail, cm->mode, cm->flags);
            continue;
        }
        if (!getChannelEdges((u_int16_t)cm->flags, &c_lo, &c_hi)) 
        {
            /* channel not supported by hardware, skip it */
            UserPrint("Channels 0x%x not supported by hardware\n", cm->flags);
            continue;
        }
        switch (cm->mode) 
        {
#ifndef ATH_NO_5G_SUPPORT
        case HAL_MODE_TURBO:
            rd = &rd5GHz;
            channelBM = rd->chan11a_turbo;
            freqs = &regDmn5GhzTurboFreq[0];
            ctl = rd->conformance_test_limit | CTL_TURBO;
            break;

        case HAL_MODE_11A:
        case HAL_MODE_11NA_HT20:
        case HAL_MODE_11NA_HT40PLUS:
        case HAL_MODE_11NA_HT40MINUS:
        case HAL_MODE_11AC_VHT20_5G:
        case HAL_MODE_11AC_VHT40PLUS_5G:
        case HAL_MODE_11AC_VHT40MINUS_5G:
        case HAL_MODE_11AC_VHT80_0:
        case HAL_MODE_11AC_VHT80_1:
        case HAL_MODE_11AC_VHT80_2:
        case HAL_MODE_11AC_VHT80_3:
        case HAL_MODE_11A_HALF_RATE:
        case HAL_MODE_11A_QUARTER_RATE:
            rd = &rd5GHz;
            channelBM = rd->chan11a;
            freqs = &regDmn5GhzFreq[0];
            ctl = rd->conformance_test_limit;
            break;
#endif
        case HAL_MODE_11B:
            rd = &rd2GHz;
            channelBM = rd->chan11b;
            freqs = &regDmn2GhzFreq[0];
            ctl = rd->conformance_test_limit | CTL_11B;
            break;

        case HAL_MODE_11G:
        case HAL_MODE_11NG_HT20:
        case HAL_MODE_11NG_HT40PLUS:
        case HAL_MODE_11NG_HT40MINUS:
        case HAL_MODE_11AC_VHT20_2G:
        case HAL_MODE_11AC_VHT40PLUS_2G:
        case HAL_MODE_11AC_VHT40MINUS_2G:
            rd = &rd2GHz;
            channelBM = rd->chan11g;
            freqs = &regDmn2Ghz11gFreq[0];
            ctl = rd->conformance_test_limit | CTL_11G;
            break;

        case HAL_MODE_11G_TURBO:
            rd = &rd2GHz;
            channelBM = rd->chan11g_turbo;
            freqs = &regDmn2Ghz11gTurboFreq[0];
            ctl = rd->conformance_test_limit | CTL_108G;
            break;

#ifndef ATH_NO_5G_SUPPORT
        case HAL_MODE_11A_TURBO:
            rd = &rd5GHz;
            channelBM = rd->chan11a_dyn_turbo;
            freqs = &regDmn5GhzTurboFreq[0];
            ctl = rd->conformance_test_limit | CTL_108G;
            break;
#endif
        default:
            UserPrint("Unknown HAL mode 0x%x\n", cm->mode);
            continue;
        }
        if (isChanBitMaskZero(channelBM))
        {
            continue;
        }
        if ((cm->mode == HAL_MODE_11NA_HT40PLUS) || (cm->mode == HAL_MODE_11NG_HT40PLUS) ||
            (cm->mode == HAL_MODE_11AC_VHT40PLUS_2G) || (cm->mode == HAL_MODE_11AC_VHT40PLUS_5G)) 
        {
            hi_adj = -20;
        }
        else if ((cm->mode == HAL_MODE_11NA_HT40MINUS) || (cm->mode == HAL_MODE_11NG_HT40MINUS) ||
            (cm->mode == HAL_MODE_11AC_VHT40MINUS_2G) || (cm->mode == HAL_MODE_11AC_VHT40MINUS_5G)) 
        {
            low_adj = 20;
        }
        else if ((cm->mode == HAL_MODE_11AC_VHT80_0) || (cm->mode == HAL_MODE_11AC_VHT80_1)) 
        {
            hi_adj = -40;
        }
        else if ((cm->mode == HAL_MODE_11AC_VHT80_2) || (cm->mode == HAL_MODE_11AC_VHT80_3)) 
        {
            low_adj = 40;
        }

        for (b=0;b<64*BMLEN; b++) 
        {
            if (IS_BIT_SET(b,channelBM)) 
            {
                fband = &freqs[b];
#ifndef ATH_NO_5G_SUPPORT
                /* 
                 * MKK capability check. U1 ODD, U1 EVEN, U2, and MIDBAND bits are checked here. 
                 * Don't add the band to channel list if the corresponding bit is not set.
                 */
                if (rd5GHz.regDmnEnum == MKK1 || rd5GHz.regDmnEnum == MKK2) 
                {
                    int i, skipband=0;
                    A_UINT32 regcap;

                    for (i = 0; i < N(j_bandcheck); i++) 
                    {
                        if (j_bandcheck[i].freqbandbit == b) 
                        {
                            regcap = Qc9KRegDomainInfo.hal_reg_cap;
                            if ((j_bandcheck[i].eepromflagtocheck & regcap) == 0) 
                            {
                                skipband = 1;
                            }
                            break;
                        }
                    }
                    if (skipband) 
                    {
                        UserPrint("Skipping %d freq band.\n", j_bandcheck[i].freqbandbit);
                        continue;
                    }
                }
#endif
                ath_add_regclassid(regclassids, maxregids, (u_int *)nregids, fband->regClassId);

                if ((fband->lowChannel < 5180) && (!is_49ghz_cap))
                {
                    continue;
                }
                if (IS_HT40_MODE(cm->mode) && (rd == &rd5GHz)) 
                {
                    /* For 5G HT40 mode, channel seperation should be 40. */
                    chanSep = 40;

                    /*
                     * For all 4.9G Hz and Unii1 odd channels, HT40 is not allowed. 
                     * Except for CTRY_DEBUG.
                     */
                    if ((fband->lowChannel < 5180) && (cc != CTRY_DEBUG)) 
                    {
                        continue;
                    }
                    /*
                     * This is mainly for F1_5280_5320, where 5280 is not a HT40_PLUS channel.
                     * The way we generate HT40 channels is assuming the freq band starts from 
                     * a HT40_PLUS channel. Shift the low_adj by 20 to make it starts from 5300.
                     */
                    if ((fband->lowChannel == 5280) || (fband->lowChannel == 4920)) 
                    {
                        low_adj += 20;
                    }
                }
                else 
                {
                    chanSep = fband->channelSep;
                    if (cm->mode == HAL_MODE_11NA_HT20) 
                    {
                        /*
                         * For all 4.9G Hz and Unii1 odd channels, HT20 is not allowed either. 
                         * Except for CTRY_DEBUG.
                         */
                        if ((fband->lowChannel < 5180) && (cc != CTRY_DEBUG))
                        {
                            continue;
                        }
                    }
                }

                for (c=fband->lowChannel + low_adj;
                     ((c <= (fband->highChannel + hi_adj)) && (c >= (fband->lowChannel + low_adj)));
                     c += chanSep) 
                {
                    QC9K_CHANNEL_INTERNAL icv;

                    if (!(c_lo <= c && c <= c_hi)) 
                    {
                        UserPrint("c %u out of range [%u..%u]\n", c, c_lo, c_hi);
                        continue;
                    }
                    if ((fband->channelBW == CHANNEL_HALF_BW) && !is_halfchan_cap) 
                    {
                        UserPrint("Skipping %u half rate channel\n", c);
                        continue;
                    }
                    if ((fband->channelBW == CHANNEL_QUARTER_BW) && !is_quarterchan_cap) 
                    {
                        UserPrint("Skipping %u quarter rate channel\n", c);
                        continue;
                    }
                    if (((c+fband->channelSep)/2) > (maxChan+HALF_MAXCHANBW)) 
                    {
                        UserPrint("c %u > maxChan %u\n", c, maxChan);
                        continue;
                    }
                    if (next >= (int)maxchans)
                    {
                        UserPrint("Too many channels for channel table\n");
                        goto done;
                    }
                    if ((fband->usePassScan & IS_ECM_CHAN) && !enableExtendedChannels && (c >= 2467)) 
                    {
                        UserPrint("Skipping ecm channel\n");
                        continue;
                    }
                    if ((rd->flags & NO_HOSTAP) && (Qc9KRegDomainInfo.ah_opmode == HAL_M_HOSTAP)) 
                    {
                        UserPrint("Skipping HOSTAP channel\n");
                        continue;
                    }
#if !(_APPLE_DARWIN_ || _APPLE_NETBSD_)
                    // This code makes sense for only the AP side. The following if's deal specifically
                    // with Owl since it does not support radar detection on extension channels.
                    // For STA Mode, the AP takes care of switching channels when radar detection
                    // happens. Also, for SWAP mode, Apple does not allow DFS channels, and so allowing
                    // HT40 operation on these channels is OK. 

                    if (IS_HT40_MODE(cm->mode) &&
                        !(getEepromRegExtBits(REG_EXT_FCC_DFS_HT40)) &&
                        (fband->useDfs) && (rd->conformance_test_limit != MKK)) 
                    {
                        /*
                         * Only MKK CTL checks REG_EXT_JAPAN_DFS_HT40 for DFS HT40 support.
                         * All other CTLs should check REG_EXT_FCC_DFS_HT40
                         */
                        UserPrint("Skipping HT40 channel (en_fcc_dfs_ht40 = 0)\n");
                        continue;
                    }
                    if (IS_HT40_MODE(cm->mode) &&
                        !(getEepromRegExtBits(REG_EXT_JAPAN_NONDFS_HT40)) &&
                        !(fband->useDfs) && (rd->conformance_test_limit == MKK)) 
                    {
                        UserPrint("Skipping HT40 channel (en_jap_ht40 = 0)\n");
                        continue;
                    }
                    if (IS_HT40_MODE(cm->mode) &&
                        !(getEepromRegExtBits(REG_EXT_JAPAN_DFS_HT40)) &&
                        (fband->useDfs) && (rd->conformance_test_limit == MKK) ) 
                    {
                        UserPrint("Skipping HT40 channel (en_jap_dfs_ht40 = 0)\n");
                        continue;
                    }
#endif
                    memset(&icv, 0, sizeof(icv));
                    icv.channel = c;
                    icv.channel_flags = cm->flags;

                    switch (fband->channelBW) 
                    {
                        case CHANNEL_HALF_BW:
                            icv.channel_flags |= CHANNEL_HALF;
                            break;
                        case CHANNEL_QUARTER_BW:
                            icv.channel_flags |= CHANNEL_QUARTER;
                            break;
                    }
                    
                    //icv.max_reg_tx_power = fband->powerDfs;
                    //icv.antennaMax = fband->antennaMax;
                    icv.regDmnFlags = rd->flags;
                    icv.conformance_test_limit = ctl;
                    icv.regClassId = fband->regClassId;

                    if (fband->usePassScan & rd->pscan)
                    {
                        /* For WG1_2412_2472, the whole band marked as PSCAN_WWR, but only channel 12-13 need to be passive. */
                        if (!(fband->usePassScan & PSCAN_EXT_CHAN) || (icv.channel >= 2467))
                        {
                            icv.channel_flags |= CHANNEL_PASSIVE;
                        }
						else if ((fband->usePassScan & PSCAN_MKKA_G) && (rd->pscan & PSCAN_MKKA_G))
						{
							icv.channel_flags |= CHANNEL_PASSIVE;
						}
                        else
                        {
                            icv.channel_flags &= ~CHANNEL_PASSIVE;
                        }
                    }
                    else
                    {
                        icv.channel_flags &= ~CHANNEL_PASSIVE;
                    }
                    if (fband->useDfs & rd->dfsMask)
                    {
                        icv.priv_flags = CHANNEL_DFS;
                    }
                    else
                    {
                        icv.priv_flags = 0;
                    }
                    #if 0 /* Enabling 60 sec startup listen for ETSI */
                    /* Don't use 60 sec startup listen for ETSI yet */
                    if (fband->useDfs & rd->dfsMask & DFS_ETSI)
                        icv.priv_flags |= CHANNEL_DFS_CLEAR;
                    #endif

                    if (rd->flags & LIMIT_FRAME_4MS)
                    {
                        icv.priv_flags |= CHANNEL_4MS_LIMIT;
                    }

                    /* Check for ad-hoc allowableness */
                    /* To be done: DISALLOW_ADHOC_11A_TURB should allow ad-hoc */
                    if (icv.priv_flags & CHANNEL_DFS) 
                    {
                        icv.priv_flags |= CHANNEL_DISALLOW_ADHOC;
                    }

                    if (icv.regDmnFlags & ADHOC_PER_11D) 
                    {
                        icv.priv_flags |= CHANNEL_PER_11D_ADHOC;
                    }

                    if (icv.channel_flags & CHANNEL_PASSIVE) 
                    {
                        /* Allow channel 1-11 as ad-hoc channel even marked as passive */
                        if ((icv.channel < 2412) || (icv.channel > 2462)) 
                        {
                            if (rd5GHz.regDmnEnum == MKK1 || rd5GHz.regDmnEnum == MKK2) 
                            {
                                A_UINT32 regcap;

                                regcap = Qc9KRegDomainInfo.hal_reg_cap;

                                if (!(regcap & (AR_EEPROM_EEREGCAP_EN_KK_U1_EVEN |
                                        AR_EEPROM_EEREGCAP_EN_KK_U2 |
                                        AR_EEPROM_EEREGCAP_EN_KK_MIDBAND)) 
                                        && isUNII1OddChan(icv.channel)) 
                                {
                                    /* 
                                     * There is no RD bit in EEPROM. Unii 1 odd channels
                                     * should be active scan for MKK1 and MKK2.
                                     */
                                    icv.channel_flags &= ~CHANNEL_PASSIVE;
                                }
                                else 
                                {
                                    icv.priv_flags |= CHANNEL_DISALLOW_ADHOC;
                                }
                            }
                            else 
                            {
                                icv.priv_flags |= CHANNEL_DISALLOW_ADHOC;
                            }
                        }
                    }
#ifndef ATH_NO_5G_SUPPORT
                    if (cm->mode & (HAL_MODE_TURBO | HAL_MODE_11A_TURBO)) 
                    {
                        if( icv.regDmnFlags & DISALLOW_ADHOC_11A_TURB) 
                        {
                            icv.priv_flags |= CHANNEL_DISALLOW_ADHOC;
                        }
                    }
#endif
                    else if (cm->mode & (HAL_MODE_11A | HAL_MODE_11NA_HT20 |
                        HAL_MODE_11NA_HT40PLUS | HAL_MODE_11NA_HT40MINUS)) 
                    {
                        if( icv.regDmnFlags & (ADHOC_NO_11A | DISALLOW_ADHOC_11A)) 
                        {
                            icv.priv_flags |= CHANNEL_DISALLOW_ADHOC;
                        }
                    }

                    if (rd == &rd2GHz) 
                    {
                        if ((icv.channel < (fband->lowChannel + 10)) ||
                            (icv.channel > (fband->highChannel - 10))) 
                        {
                            icv.priv_flags |= CHANNEL_EDGE_CH;
                        }

                        if ((icv.channel_flags & CHANNEL_HT40PLUS) &&
                            (icv.channel > (fband->highChannel - 30))) 
                        {
                            icv.priv_flags |= CHANNEL_EDGE_CH; /* Extn Channel */
                        }

                        if ((icv.channel_flags & CHANNEL_HT40MINUS) &&
                            (icv.channel < (fband->lowChannel + 30))) 
                        {
                            icv.priv_flags |= CHANNEL_EDGE_CH; /* Extn Channel */
                        }
                    }
#ifdef ATH_IBSS_DFS_CHANNEL_SUPPORT
                    /* EV 83788, 83790 support 11A adhoc in ETSI domain. Other
                     * domain will remain as their original settings for now since there
                     * is no requirement.
                     * This will open all 11A adhoc in ETSI, no matter DFS/non-DFS
                     */
                    if (dfsDomain == DFS_ETSI_DOMAIN) 
                    {
                        icv.priv_flags &= ~CHANNEL_DISALLOW_ADHOC;
                    }
#endif
                    memcpy(&ichans[next++], &icv, sizeof(QC9K_CHANNEL_INTERNAL));
                }

                /* Restore normal low_adj value. */
                if (IS_HT40_MODE(cm->mode)) 
                {
                    if (fband->lowChannel == 5280 || fband->lowChannel == 4920) 
                    {
                        low_adj -= 20;
                    }
                }
            }
        }
    }
done:    

    if (next != 0) 
    {
        int i;

        /* XXX maxchans set above so this cannot happen? */
        if (next > N(Qc9KRegDomainInfo.ah_channels)) 
        {
            UserPrint("Too many channels %u; truncating to %u\n", next,
                     (unsigned) N(Qc9KRegDomainInfo.ah_channels));
            next = N(Qc9KRegDomainInfo.ah_channels);
        }

        /* Initialize NF cal history buffer */
        ath_hal_init_NF_buffer(ichans, next);

        /*
         * Keep a private copy of the channel list so we can
         * constrain future requests to only these channels
         */
        ath_hal_sort(ichans, next, sizeof(QC9K_CHANNEL_INTERNAL), chansort);

        //Qc9KRegDomainInfo.ah_nchan = next;
        /*
         * Copy the channel list to the public channel list
         */
        // Turn off priting on console
        userPrintFlag = UserPrintConsoleGet();
        UserPrintConsole(0);
        UserPrint("Channel list:\n");

        for (i=0; i<next; i++) 
        {   
            UserPrint("chan: %d flags: 0x%x\n", ichans[i].channel, ichans[i].channel_flags);
            chans[i] = ichans[i].channel;
            flags[i] = ichans[i].channel_flags;
            //chans[i].priv_flags = ichans[i].priv_flags;
            //chans[i].max_reg_tx_power = ichans[i].max_reg_tx_power;
            //chans[i].regClassId = ichans[i].regClassId;
        }
        UserPrintConsole(userPrintFlag);    //restore
        
        /*
         * Retrieve power limits.
         */
        //ath_hal_getpowerlimits(ah, chans, next);
        for (i=0; i<next; i++) 
        {
            ichans[i].max_tx_power = QC9K_MAX_RATE_POWER;
            ichans[i].min_tx_power = QC9K_MAX_RATE_POWER;
        }

    }
    *nchans = next;

    /* save for later query */
    Qc9KRegDomainInfo.ah_currentRDInUse = regdmn;
    Qc9KRegDomainInfo.ah_currentRD5G = rd5GHz.regDmnEnum;
    Qc9KRegDomainInfo.ah_currentRD2G = rd2GHz.regDmnEnum;

    if(country == NULL) 
    {
        Qc9KRegDomainInfo.ah_iso[0] = 0;
        Qc9KRegDomainInfo.ah_iso[1] = 0;
    }
    else
    {
        Qc9KRegDomainInfo.ah_iso[0] = country->isoName[0];
        Qc9KRegDomainInfo.ah_iso[1] = country->isoName[1];
    }
#if ICHAN_WAR_SYNCH
    Qc9KRegDomainInfo.ah_ichan_set = false;
    spin_unlock(&Qc9KRegDomainInfo.ah_ichan_lock);
#endif

    return (next != 0);

#undef CHANNEL_HALF_BW
#undef CHANNEL_QUARTER_BW
}

#undef N

