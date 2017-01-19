/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
#ifndef _IFX_TYPES_H
#define _IFX_TYPES_H

/** \file
   Basic data types.
*/

/** This is the chracter datatype. */
typedef char            IFX_char_t;
/** This is the unsigned 8-bit datatype. */
typedef unsigned char   IFX_uint8_t;
/** This is the signed 8-bit datatype. */
typedef signed char     IFX_int8_t;
/** This is the unsigned 16-bit datatype. */
typedef unsigned short  IFX_uint16_t;
/** This is the signed 16-bit datatype. */
typedef signed short    IFX_int16_t;
/** This is the unsigned 32-bit datatype. */
typedef unsigned long   IFX_uint32_t;
/** This is the signed 32-bit datatype. */
typedef signed long     IFX_int32_t;
/** This is the float datatype. */
typedef float           IFX_float_t;
/** This is the void datatype. */
typedef void            IFX_void_t;

/** This is the volatile unsigned 8-bit datatype. */
typedef volatile IFX_uint8_t  IFX_vuint8_t;
/** This is the volatile signed 8-bit datatype. */
typedef volatile IFX_int8_t   IFX_vint8_t;
/** This is the volatile unsigned 16-bit datatype. */
typedef volatile IFX_uint16_t IFX_vuint16_t;
/** This is the volatile signed 16-bit datatype. */
typedef volatile IFX_int16_t  IFX_vint16_t;
/** This is the volatile unsigned 32-bit datatype. */
typedef volatile IFX_uint32_t IFX_vuint32_t;
/** This is the volatile signed 32-bit datatype. */
typedef volatile IFX_int32_t  IFX_vint32_t;
/** This is the volatile float datatype. */
typedef volatile IFX_float_t  IFX_vfloat_t;


/* A type for handling boolean issues. */
typedef enum {
   /** false */
   IFX_FALSE = 0,
   /** true */
   IFX_TRUE = 1
} IFX_boolean_t;


/**
   This type is used for parameters that should enable
   and disable a dedicated feature. */
typedef enum {
   /** disable */
   IFX_DISABLE = 0,
   /** enable */
   IFX_ENABLE = 1
} IFX_enDis_t;

/**
   This type is used for parameters that should enable
   and disable a dedicated feature. */
typedef IFX_enDis_t IFX_operation_t;

/**
   This type has two states, even and odd.
*/
typedef enum {
   /** even */
   IFX_EVEN = 0,
   /** odd */
   IFX_ODD = 1
} IFX_evenOdd_t;


/**
   This type has two states, high and low.
*/
typedef enum {
   IFX_LOW = 0,
   IFX_HIGH = 1
} IFX_highLow_t;

/**
   This type has two states, success and error
*/
typedef enum {
   IFX_ERROR   = (-1),
   IFX_SUCCESS = 0
} IFX_return_t;


#define IFX_NULL         ((void *)0)

#endif /* _IFX_TYPES_H */

