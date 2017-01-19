/******************************************************************************
**
** FILE NAME    : ifx_ts.h
** PROJECT      : UEIP
** MODULES      : Thermal Sensor
**
** DATE         : 16 Aug 2011
** AUTHOR       : Xu Liang
** DESCRIPTION  : Thermal Sensor driver header file
** COPYRIGHT    :       Copyright (c) 2006
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date          $Author         $Comment
** Aug 16, 2011   Xu Liang        Init Version
*******************************************************************************/



#ifndef __IFX_TS_H__
#define __IFX_TS_H__

/*!
  \defgroup IFX_TS UEIP Project - Thermal Sensor driver module
  \brief UEIP Project - Thermal Sensor driver module, support VR9.
 */

/*!
  \defgroup IFX_TS_API APIs
  \ingroup IFX_TS
  \brief APIs used by other drivers/modules.
 */

/*!
  \defgroup IFX_TS_IOCTL IOCTL Commands
  \ingroup IFX_TS
  \brief IOCTL Commands used by user application.
 */

/*!
  \defgroup IFX_TS_STRUCT Structures
  \ingroup IFX_TS
  \brief Structures used by user application.
 */

/*!
  \file ifx_ts.h
  \ingroup IFX_TS
  \brief Thermal Sensor driver header file
 */



/*!
  \addtogroup IFX_SI_STRUCT
 */
/*@{*/

/*!
  \struct ifx_si_ioctl_version_t
  \brief Structure used for query of driver version.
 */
typedef struct {
    unsigned int    major;  /*!< output, major number of driver */
    unsigned int    mid;    /*!< output, mid number of driver   */
    unsigned int    minor;  /*!< output, minor number of driver */
} ifx_ts_ioctl_version_t;

/*@}*/



/*!
  \addtogroup IFX_SI_IOCTL
 */
/*@{*/

#define IFX_TS_IOC_MAGIC            0xed
/*!
  \def IFX_TS_IOC_VERSION
  \brief Thermal Sensor IOCTL Command - Get driver version number.

   This command uses struct "ifx_ts_ioctl_version_t" as parameter to Thermal Sensor driver version number.
 */
#define IFX_TS_IOC_VERSION          _IOR(IFX_TS_IOC_MAGIC, 1, ifx_ts_ioctl_version_t)

/*!
  \def IFX_TS_IOC_TEST
  \brief Thermal Sensor IOCTL Command - Run basic driver sanity check (debug).

   No parameter is needed for this command. Only for internal sanity check purpose.
 */
#define IFX_TS_IOC_TEST             _IO(IFX_TS_IOC_MAGIC, 0)

/* For checking endpoint */
#define IFX_TS_IOC_MAXNR            2

/*@}*/



#ifdef __KERNEL__
  extern int ifx_ts_get_temp(int *p_temp);
#endif



#endif  //  __IFX_TS_H__
