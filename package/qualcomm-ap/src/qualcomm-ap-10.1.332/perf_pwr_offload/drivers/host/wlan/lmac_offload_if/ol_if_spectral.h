#ifndef	__OL_IF_SPECTRAL_H__
#define	__OL_IF_SPECTRAL_H__

#define PHY_ERROR_SPECTRAL_SCAN         0x26
#define PHY_ERROR_FALSE_RADAR_EXT       0x24

#if ATH_SUPPORT_SPECTRAL
extern int ol_if_spectral_setup(struct ieee80211com *ic);
extern int ol_if_spectral_detach(struct ieee80211com *ic);
#endif

#endif	/* __OL_IF_SPECTRAL_H__ */
