#ifndef __VRX218_PTM_COMMON_H__
#define __VRX218_PTM_COMMON_H__

/* This h file is created to share the address mappings and functions between ptm .c files  */

/*
 * ####################################
 *         External Function Declaration
 * ####################################
 */
extern int32_t get_qid_by_priority(uint32_t , uint32_t);
extern int32_t get_class_by_qid(uint32_t , int32_t);
extern int eth_xmit(struct sk_buff *skb, unsigned int port, int ch, int spid, int class, int flags);

/*
 * ####################################
 *         Address Mapping Macro Declaration
 * ####################################
 */

#define PSAVE_CFG(base)                         ((volatile struct psave_cfg *)              SOC_ACCESS_VRX218_SB(__PSAVE_CFG, base))

/*
 * ####################################
 *         definition
 * ####################################
 */

#define TEST_SOC_DMA_WRITE                      0


#endif //__VRX218_PTM_COMMON_H__
