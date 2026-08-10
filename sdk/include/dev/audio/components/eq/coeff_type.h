#ifndef _COEFF_TYPE_H
#define _COEFF_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif


/*!
 * @brief coefficient configuration
 * @note
 *     only for software.
 *     a example of coefficient for 5 band:
 *     struct aueq_cfg cfg[5];
 *     cfg[0].b[0] = 0.1;
 *     cfg[0].b[1] = 0.3;
 *     ...
 *     cfg[1].b[1] = 0.6;
 */
struct aueq_cfg_5band{
  double b[3];
  double a[2];
};


/*!
 * @brief coefficient configuration
 * @note
 *      only for chip.
 */
struct aueq_cfg_10band{
  int32 coeff[50];
};



#ifdef __cplusplus
}
#endif
#endif
