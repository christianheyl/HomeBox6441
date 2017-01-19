

#ifdef UNUSED
#define AH_SUPPORT_AR5212	1
#define AH_SUPPORT_5111	1
#define AH_SUPPORT_5112	1
#define AH_SUPPORT_2413	1
#define AH_SUPPORT_5413 1
#define AH_SUPPORT_2425	1
#endif

#define AH_BYTE_ORDER AH_BIG_ENDIAN
#define AH_SUPPORT_AR5416       1

#define AH_SUPPORT_2122
#define AH_SUPPORT_2133
#define AH_SUPPORT_5122
#define AH_SUPPORT_5133

//#ifdef UNUSED
#define AH_SUPPORT_AR9300       1
//#define AR9300_EMULATION 1
//#define AR9300_EMULATION_BB 1
//#endif
#define AH_SUPPORT_OSPREY_1_0 1
#define AH_SUPPORT_OSPREY_2_0 1
#define AH_SUPPORT_HORNET 1
#define AH_SUPPORT_WASP 1
#define AH_SUPPORT_WRITE_EEPROM 1
/*
 * Comment out one of the two AH_SUPPORT_EEPROM_xxx lines below
 * if the EEPROM map type in question is not needed in the product.
 * (Don't change "#define AH_SUPPORT_EEPROM_xxx 1" to
 * "#define AH_SUPPORT_EEPROM_xxx 0".
 * In this case "#ifdef AH_SUPPORT_EEPROM_xxx will still be true.)
 */
#define AH_SUPPORT_EEPROM_4K  1
#define AH_SUPPORT_EEPROM_DEF 1
#define AH_SUPPORT_EEPROM_AR9287 1 

/* conditional support for Sowl (AR9160, AR9161) */
#define AH_SUPPORT_SOWL 1

#if AH_SUPPORT_SOWL
#else
	#undef AH_SUPPORT_SOWL
#endif
/* Conditional support for Owl or Howl, or neither.
   (Currently only one of Owl or Howl can be supported by the WLAN driver,
   because the register initialization arrays in ar5416.ini for owl and
   ar5416_howl.ini for howl have the same names.)
 */
/* Temporarily use the AR9100 compilation flag to decide whether to
   support Owl or Howl.  Eventually the build setup should directly
   set AH_SUPPORT_OWL or AH_SUPPORT_HOWL (or neither) rather than
   inferring them from AR9100.
 */
#ifdef AR9100
  /* support just Howl */
  #define AH_SUPPORT_HOWL 1
  #ifdef AH_SUPPORT_OWL
    #undef AH_SUPPORT_OWL
  #endif
  #if AH_SUPPORT_HOWL
  #else
  	#undef AH_SUPPORT_HOWL
  #endif


#else
  /* support just Owl */
  #define AH_SUPPORT_OWL 1
  #ifdef AH_SUPPORT_HOWL
    #undef AH_SUPPORT_HOWL
  #endif
  #if AH_SUPPORT_OWL
  #else
  	#undef AH_SUPPORT_OWL
  #endif
  
#endif

#define AH_SUPPORT_KIWI 1      /* support latest revision */

#if AH_SUPPORT_KIWI#define AH_SUPPORT_KIWI_11	1
#define AH_SUPPORT_KIWI_ANY	1
#else
	#undef AH_SUPPORT_KIWI
#endif
/* Conditional support for Kite revisions.
 * To support all Kite (AR9285) revisions:
 *    #define AH_SUPPORT_KITE_ALL 1
 * To support just the latest revision:
 *    #define AH_SUPPORT_KITE     1
 * To pick and choose which revisions to support:
 *    #define AH_SUPPORT_KITE_xxx 1
 *    //#define AH_SUPPORT_KITE_yyy 1
 *    #define AH_SUPPORT_KITE_zzz 1
 */
#if 1
    #define AH_SUPPORT_KITE_ALL 1  /* to support all Kite revisions */
#else
  #if 1
    #define AH_SUPPORT_KITE 1      /* support latest revision */
  #else
    //#define AH_SUPPORT_KITE_10  1  /* revisions 1.0 and 1.1 */
    //#define AH_SUPPORT_KITE_12  1  /* revision 1.2 and later */
  #endif
#endif
/*
 * Configure support for Kite only in the conditional blocks above.
 * The blocks below set some derived #defines based on the primitive
 * #defines above.
 * Do not edit the blocks below except when adding support for new Kite
 * revisions.
 */
#if AH_SUPPORT_KITE_ALL /* all revisions */
  #define AH_SUPPORT_KITE_10 1  /* revisions 1.0 and 1.1 */
  #define AH_SUPPORT_KITE_12 1  /* revision 1.2 and later */
#else
	#undef AH_SUPPORT_KITE_ALL
#endif
#ifdef AH_SUPPORT_KITE /* latest revision - currently 1.2+ */
  #define AH_SUPPORT_KITE_12 1  /* 1.2 and later supported */
#endif
#if defined(AH_SUPPORT_KITE_12) || defined(AH_SUPPORT_KITE_10)
  #define AH_SUPPORT_KITE_ANY 1
#endif

/* Conditional support for Merlin revisions.
 * To support all Merlin (AR928x, AR922x) revisions:
 *    #define AH_SUPPORT_MERLIN_ALL 1
 * To support just the latest revision:
 *    #define AH_SUPPORT_MERLIN     1
 * To pick and choose which revisions to support:
 *    #define AH_SUPPORT_MERLIN_xxx 1
 *    //#define AH_SUPPORT_MERLIN_yyy 1
 *    #define AH_SUPPORT_MERLIN_zzz 1
 */
#if 1
    #define AH_SUPPORT_MERLIN_ALL 1  /* to support all Merlin revisions */
#else
  #if 1
    #define AH_SUPPORT_MERLIN 1      /* support latest revision */
  #else
    //#define AH_SUPPORT_MERLIN_10  1  /* revisions 1.0 up to (not incl.) 2.0 */
    //#define AH_SUPPORT_MERLIN_20  1  /* revision 2.0 and later */
  #endif
#endif
/*
 * Configure support for Merlin only in the conditional blocks above.
 * The blocks below set some derived #defines based on the primitive
 * #defines above.
 * Do not edit the blocks below except when adding support for new Merlin
 * revisions.
 */
#if AH_SUPPORT_MERLIN_ALL /* all revisions */
  #define AH_SUPPORT_MERLIN_10 1  /* revisions 1.0 up to (not incl.) 2.0 */
  #define AH_SUPPORT_MERLIN_20 1  /* revision 2.0 and later */
#else
	#undef AH_SUPPORT_MERLIN_ALL
#endif
#ifdef AH_SUPPORT_MERLIN /* latest revision - currently 2.0+ */
  #define AH_SUPPORT_MERLIN_20 1  /* 2.0 and later supported */
#endif
#if defined(AH_SUPPORT_MERLIN_20) || defined(AH_SUPPORT_MERLIN_10)
  #define AH_SUPPORT_MERLIN_ANY 1
#endif

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

