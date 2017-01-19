/******************************************************************************
**
** FILE NAME    : emulation.h
** MODULES      : BSP Basic
**
** DATE         : 27 May 2009
** AUTHOR       : Lei Chuan Hua
** DESCRIPTION  : header file for HN1
** COPYRIGHT    :       Copyright (c) 2011
**                      Lantiq Deutschland GmbH
**                      Am Campeon 3, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author         $Comment
*******************************************************************************/



#ifndef EMULATION_H
#define EMULATION_H

#ifdef CONFIG_USE_EMULATOR

//For Palladium using db.rel_106.c database the EMULATOR_CPU_SPEED is 271KHz
    #define EMULATOR_CPU_SPEED    271000
    #define PLL0_CLK_SPEED        271000

#else  /* Real chip */
    #define PLL0_CLK_SPEED        1000000000
#endif /* CONFIG_USE_EMULATOR */


#endif /* */
 /* EMULATION_H */

