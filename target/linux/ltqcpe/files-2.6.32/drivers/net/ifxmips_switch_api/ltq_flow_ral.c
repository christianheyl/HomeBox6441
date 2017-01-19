/****************************************************************************
                              Copyright (c) 2010
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************
   \file ifx_ethsw_vr9_ral.c
   \remarks implement the register access by internal bus.
 *****************************************************************************/

#include <ltq_flow_ral.h>
//#include <ltq_flow_core_reg_access.h>

/**
* \fn int IFX_Register_init_Switch_Dev(IFX_void_t)
* \brief Init and register the RAL switch device
* \param  none
* \return on Success return data read from register, else return IFX_ERROR
*/

//extern IFX_uint32_t ifx_ethsw_ll_DirectAccessRead(IFX_void_t *pDevCtx, IFX_int16_t Offset, IFX_int16_t Shift, IFX_int16_t Size, IFX_uint32_t * value);
//extern IFX_return_t ifx_ethsw_ll_DirectAccessWrite(IFX_void_t *pDevCtx, IFX_int16_t Offset, IFX_int16_t Shift, IFX_int16_t Size, IFX_uint32_t value);
/**
* \fn int ifx_switch_hw_ll_direct_access_read()
* \brief Access read low level hardware register via internal register
* \param  IFX_ETHSW_regMapper_t *RegAccessHandle
* \param  IFX_int32_t RegAddr
* \param  IFX_uint32_t * value
* \return on Success return data read from register, else return ETHSW_statusErr
*/
IFX_uint32_t ifx_ethsw_ll_DirectAccessRead(IFX_void_t *pDevCtx, IFX_int16_t Offset, IFX_int16_t Shift, IFX_int16_t Size, IFX_uint32_t * value) 
{
		IFX_uint32_t regValue, regAddr, mask;
		
		/* Prepare the register address */
		regAddr = VR9_BASE_ADDRESS + (Offset * 4);
		
//		printk("regAddr(Read) = 0x%x\n",regAddr);
		/* Read the Whole 32bit register */
		regValue = VR9_REG32_ACCESS(regAddr);

		/* Prepare the mask	*/
		mask = (1 << Size) - 1 ;

		/* Bit shifting to the exract bit field */
		regValue = (regValue >> Shift);
		
		*value = (regValue & mask);

//		printk(" >> ifx_switch_hw_ll_direct_access_read => value [%x]\n",*value);
	
		return IFX_SUCCESS;
}

/**
* \fn int ifx_switch_hw_ll_direct_access_write()
* \brief Access read low level hardware register via internal register
* \param  IFX_ETHSW_regMapper_t *RegAccessHandle
* \param  IFX_int32_t RegAddr
* \param  IFX_uint32_t value
* \return on Success return OK else return ETHSW_statusErr
*/
IFX_return_t ifx_ethsw_ll_DirectAccessWrite(IFX_void_t *pDevCtx, IFX_int16_t Offset, IFX_int16_t Shift, IFX_int16_t Size, IFX_uint32_t value) 
{
		IFX_uint32_t regValue, regAddr, mask;
		
		/* Prepare the register address */
		regAddr = VR9_BASE_ADDRESS + (Offset * 4);
		
//		printk("regAddr(Write) = 0x%x\n",regAddr);
		/* Read the Whole 32bit register */
		regValue = VR9_REG32_ACCESS(regAddr);

		/* Prepare the mask	*/
		mask = (1 << Size) - 1 ;
		mask = (mask << Shift);
		
		/* Shift the value to the right place and mask the rest of the bit*/
		value = ( value << Shift ) & mask;
		
		/*  Mask out the bit field from the read register and place in the new value */
		value = ( regValue & ~mask ) | value ;
		
//		printk("write value = 0x%x\n", value);
		/* Write into register */
		VR9_REG32_ACCESS(regAddr) = value ;
//	*((volatile IFX_uint32_t *)(RegAddr)) = value;
// 	printk(" >> ifx_switch_hw_ll_direct_access_read\n");
		
	return IFX_SUCCESS;

}
IFX_void_t *IFX_FLOW_RAL_init(IFX_FLOW_RAL_Init_t *pInit)
{

  IFX_FLOW_RAL_Dev_t *pDev;

//#ifdef IFXOS_SUPPORT
  pDev = (IFX_FLOW_RAL_Dev_t *) IFXOS_BlockAlloc (sizeof (IFX_FLOW_RAL_Dev_t));
//#else
//  pDev = (IFX_FLOW_RAL_Dev_t *) kmalloc (sizeof (IFX_FLOW_RAL_Dev_t), GFP_KERNEL);
//#endif

  if(pDev == IFX_NULL) {
    IFXOS_PRINT_INT_RAW("Error : %s memory allocation failed !!\n", __func__);
    return IFX_NULL;
  }

  if(pInit->eDev == IFX_FLOW_DEV_INT)
  {
    pDev->register_read = &ifx_ethsw_ll_DirectAccessRead;
    pDev->register_write = &ifx_ethsw_ll_DirectAccessWrite;
    pDev->nBaseAddress = VR9_BASE_ADDRESS;
  }

  pDev->eDev = pInit->eDev;
  
  return pDev;

}

