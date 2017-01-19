/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
/*
 * @File: pcihw_api.h
 * 
 * @Abstract: PCIe support
 * 
 * @Notes: 
 * 
 * Copyright (c) 2011 Atheros Communications, Inc.
 * All rights reserved.
 *
 */

#ifndef __PCIHW_API_H__
#define __PCIHW_API_H__

#if defined(CONFIG_PCIE_SUPPORT)

/*
 * PCIe Power Management Methods (see pcie_state->power_mgmt_method).
 *
 * These methods only have meaning if the PCIe Root Complex supports
 * Power Management.
 *
 * When WoW is in use and when firmware knows that the Host is
 * sleeping, PCIe is guaranteed to be idle and we transition it
 * to Low Power mode regardless of which Method is in use. So
 * these methods only have an effect when the Host exits WoW.
 *
 * The expectation is that a suitable PCIe Power Method is chosen
 * at startup time. Typically, the default method is adequate so
 * nothing needs to be done.
 *
 * Currently, the only three methods that have been shown to work
 * reliably are:
 *   PCIE_PWR_METHOD_L0,
 *   PCIE_PWR_METHOD_WMAC_AXI_L0 and
 *   PCIE_PWR_METHOD_AXI_L0
 *
 * NB: pcie_resume causes the selected PCIe Power Method
 * to take effect.
 *
 * The choice of whether or not to gate the PCIe Phy clock
 * while in L1 is orthogonal. One reasonable policy is to
 * gate the PCIe Phy clock in L1 while in network sleep
 * (because PCIe traffic is unlikely and less latency sensitive).
 * During normal opeation, it is reasonable to avoid clock
 * gating in order to guarantee lower latency over PCIe.
 * See pcie_phy_clock_gating_in_l1.
 */
#if defined(AR9888)
#define PCIE_PWR_METHOD_DEFAULT PCIE_PWR_METHOD_WMAC_AXI_L0
#else
#define PCIE_PWR_METHOD_DEFAULT PCIE_PWR_METHOD_AXI_L0
#endif

/* Method0: PCIe remains active (in L0) even when idle */
#define PCIE_PWR_METHOD_L0      0 /* PCIe stays active */

/*
 * PCIe uses a hardware timer to determine
 * when to transition to L1.
 * Note: This method does not yet work.  Do not use.
 */
#define PCIE_PWR_METHOD_TIMER   1

/*
 * Use L1 whenever WMAC is asleep.
 * As soon as WMAC wakes, wake PCIe as well.  This 
 * provides less aggressive power savings than METHOD_AXI_L0,
 * but it may be useful if there is insufficient WMAC
 * buffering to allow time for PCIe to wake.  Note
 * that as a side-effect, PCIe will be woken even
 * when WMAC is using a local Target buffer.
 * (NOT typically used)
 */
#define PCIE_PWR_METHOD_WMAC_L0 2 /* WMAC awake->PCIe active */

/*
 * Use L1 whenever there is no AXI activity.
 */
#define PCIE_PWR_METHOD_AXI_L0  3 /* AXI->PCIe active */

/*
 * Force L1 as much as possible.
 * (Useful for testing, but not intended for general use.)
 */
#define PCIE_PWR_METHOD_MIN_L0  4 /* Minimize PCIe active */

/*
 * Use L1 whenever there is no AXI activity and no WMAC activity
 */
#define PCIE_PWR_METHOD_WMAC_AXI_L0  5 /* AXI or WMAC->PCIe active */

#define PCIE_CONFIG_FLAGS_DEFAULT   PCIE_CONFIG_FLAG_ENABLE_L1

extern void pcie_pause(void);
extern void pcie_resume(void);
extern void pcie_notify(int code, void *arg);
extern void pcie_host_wake(void);
extern void pcie_host_awake_ack(void);

/*
 * PCIe Configuration Support
 *
 * A PCIe Configuration otpstream consists of
 *  1 byte otpstream_id (OTPSTREAM_ID_PCIE_CONFIG)
 * followed by a list of PCIe OTPStream Config Elements.
 *
 * Each PCIe Stream Config Element consists of
 *  1 byte meta:
 *    4-bit operation (set, and, or) (high nibble)
 *    4-bit size (1, 2, 4)           (low nibble)
 *  2 byte offset into PCIe Config Space, aligned according to sz
 *  sz bytes of data
 *
 * Example PCIe Configuration otpstream contents to alter
 * the device ID in Config Space to 0x1234:
 *
 *     0xfb, 0x12, 0x00, 0x02, 0x12, 0x34
 *     ^     ^     ^           ^
 *     |     |     |           |__new value (0x1234)
 *     |     |     |
 *     |     |     |__Config Space offset (0x0002)
 *     |     |
 *     |     |__PCIE_CONFIG_OP_SET and byte count (2)
 *     |
 *     |__OTPSTREAM_ID_PCIE_CONFIG (0xfb)
 */

#define PCIE_CONFIG_OP_SET       1
#define PCIE_CONFIG_OP_AND       2
#define PCIE_CONFIG_OP_OR        3

#endif /* CONFIG_PCIE_SUPPORT */

#endif /* __PCIHW_API_H__ */
