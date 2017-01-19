#ifndef _CAL_DEF_H_
#define _CAL_DEF_H_

#define _DESIRED_SCALE_MCS7                 0xE
#define POWER_T10(power)                    ((A_INT32)((power) * 10))
#define OLPCGAINDELTA_T10(power,gain)       (POWER_T10(power) - ((gain) * 5) + (_DESIRED_SCALE_MCS7 * 5))
#define OLPCGAINDELTA_T10X(power_t10,gain)  ((power_t10) - ((gain) * 5) + (_DESIRED_SCALE_MCS7 * 5))

#endif //_CAL_DEF_H_