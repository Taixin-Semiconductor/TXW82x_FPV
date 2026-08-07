#include "lib/touch/touch_pad.h"

#if CST226SE_TOUCH_PAD
#include "lib/touch/cst226se.h"
#endif

touch_multipoint_pos_t *touch_pad_get_multipoint_xy()
{
    // This function retrieves the multipoint touch data from the touch pad.
    // It returns a pointer to the data structure containing the touch points.
    // The caller is responsible for freeing the memory allocated for the data.
#if TOUCH_PAD_EN
#if CST226SE_TOUCH_PAD
    return cst226se_get_multipoint_xy();
#endif

    return NULL; 
#else
    return NULL; 
#endif
}

uint32_t touch_pad_free_multipoint_xy(touch_multipoint_pos_t *data)
{
    // This function frees the memory allocated for the multipoint touch data.
    // It takes a pointer to the data structure as an argument and returns a status code.
#if TOUCH_PAD_EN
#if CST226SE_TOUCH_PAD
    return cst226se_free_multipoint_xy(data);
#endif

    return RET_OK; 
#else
    return RET_OK; 
#endif
}


void touch_pad_hareware_init()
{
    // Initialize the touch pad hardware
    // This function can be used to set up the necessary hardware configurations
    // for the touch pad, such as GPIO pins, I2C interfaces, etc.
#if TOUCH_PAD_EN
#if CST226SE_TOUCH_PAD
    cst226se_init();
#endif

#else
    return;
#endif
}