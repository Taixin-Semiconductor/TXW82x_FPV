#ifndef HG_LCDC_H
#define HG_LCDC_H
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/lcdc.h"


typedef enum {
	LCD_DONE_IRQ,
	LCD_TE_IRQ,
	OSD_EN_IRQ,
	SCREEN_DONE_IRQ = OSD_EN_IRQ,
	TIMEOUT_IRQ,
	LCD_IRQ_NUM
}LCD_IRQ_E;


struct hglcdc {
    struct lcdc_device   dev;
	uint32              hw;
    lcdc_irq_hdl         irq_hdl;
    uint32              irq_data;
    uint32              irq_num;
    uint32              opened:1, use_dma:1;
};
int32 hglcdc_attach(uint32 dev_id, struct hglcdc *lcdc);
#endif

