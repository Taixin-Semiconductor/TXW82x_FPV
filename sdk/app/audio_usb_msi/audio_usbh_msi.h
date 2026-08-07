#ifndef _AUDIO_USB_MSI_H_
#define _AUDIO_USB_MSI_H_

#include "basic_include.h"
#include "stream_frame.h"
#include "lib/multimedia/msi.h"

enum uac_app_usb
{
    UAC_TASK_EXIT = BIT(0),
    UAC_TASK_STOP = BIT(1),
};

struct uac_msi_s
{
    struct os_task task;
    struct msi *msi;
    struct os_event evt;
    struct fbpool tx_pool;
    int* p_usb_dma_irq_time;
};


void usbmic_enum_finish(int* p_usb_dma_irq_time);
void usbspk_enum_finish(int* p_usb_dma_irq_time);

#endif