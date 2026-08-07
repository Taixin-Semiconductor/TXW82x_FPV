#include "basic_include.h"

#include "hal/i2c.h"
#include "hal/timer_device.h"
#include "hal/pwm.h"
#include "hal/capture.h"
#include "hal/netdev.h"
#include "hal/spi.h"
#include "hal/spi_nor.h"

#include "lib/ota/fw.h"
#include "lib/rpc/cpurpc.h"

#include "dev/uart/hguart_v2.h"
#include "dev/uart/hguart_v4.h"
#include "dev/dma/dw_dmac.h"
#include "dev/gpio/hggpio_v4.h"
#include "dev/dma/hg_m2m_dma.h"
#include "dev/crc/hg_crc.h"
#include "dev/timer/hgtimer_v4.h"
#include "dev/vpp/hgvpp.h"
#include "dev/prc/hgprc.h"
#include "dev/csi/hgdvp.h"
#include "dev/of/hgof.h"
#include "dev/csc/hgcsc.h"
#include "dev/lcdc/hglcdc.h"
#include "dev/lcdc/hgdsi.h"
#include "dev/jpg/hgjpg.h"
#include "dev/scale/hgscale.h"
#include "lib/rpc/cpurpc.h"
#include "hal/i2c.h"
#include "dev/gen/hggen420.h"
#include "dev/gen/hggen422.h"
#include "dev/spi/hgspi_v3.h"
#include "dev/spi/hgspi_xip.h"
#include "dev/h264/hg264.h"
#include "dev/dual/hgdual_org.h"
#include "dev/mipi_csi/hgmipi_csi.h"
#include "dev/i2c/hgi2c_v1.h"
#include "dev/osd_enc/hgosd_enc.h"
#include "lib/sdhost/sdhost.h"
#include "dev/isp/hgisp_v0.h"
//#include "dev/isp/hgftusb3_v0.h"
#include "dev/pwm/hgpwm_v0.h"
#include "dev/dma2d/hg_dma2d_v0.h"
#include "lib/syscfg/syscfg.h"
#include "syscfg.h"
#include "dev/usb/hgusb20_v1_dev_api.h"
#include "dev/sysaes/hg_sysaes_v3.h"
#include "dev/adc/hgadc_v1.h"
#include "dev/usb/usb11_v0/hgusb11_v0_dev_api.h"
#include "dev/usb/usb11_v0/hgusb11_v0_host_api.h"
#include "dev/audio/components/hg_audio_v0.h"
#include "dev/para_in/hgpara_in.h"
#include "dev/sha/hgsha_v1.h"
#include "dev/xspi/hg_xspi_psram.h"



#define DEV_SENSOR_MASTER_IIC_DEVID     (ISP_CSI0_ID)
#define DEV_SENSOR_SLAVE_IIC_DEVID      (ISP_CSI1_ID)

extern struct dma_device *m2mdma;
extern void *console_handle;
extern void device_burst_set(void);


struct hgusb20_dev usb20_dev = {
    .usb_hw = (void *)USB20_BASE,
    .ep_irq_num = USB20MC_IRQn,
    .dma_irq_num = USB20DMA_IRQn,
    //.flags = BIT(HGUSB20_FLAGS_FULL_SPEED),
};

struct hgusb11_dev usb11_dev = {
    .usb_hw = (void *)USB11_BASE,
    .ep_irq_num = USB11_MC_IRQn,
    .dma_irq_num = USB11_DMA_IRQn,
    .p_comm = &usb11_dev.comm_dat,
};

struct hgusb11_host usb11_host = {
    .usb_hw = (void *)USB11_BASE,
    .ep_irq_num = USB11_MC_IRQn,
    .dma_irq_num = USB11_DMA_IRQn,
    .p_comm = &usb11_dev.comm_dat,
};

struct hgpara_in para_in = {
    .hw = (void *)PARA_IN_BASE,
    .irq_num = PARA_IN_IRQn,
};

struct hg_crc crc32_module = {
    .hw = (void *)CRC_BASE,
    .irq_num = CRC_IRQn,
};

struct hg_sysaes_v3 sysaes = {
    .hw = (void *)SYS_AES_BASE,
    .irq_num = SYS_AES_IRQn,
};

struct hguart_v2 uart0 = {
    .hw      = UART0_BASE,
    .irq_num = UART0_IRQn
};

struct hguart_v2 uart1 = {
    .hw      = UART1_BASE,
    .irq_num = UART1_IRQn,
};

struct hguart_v4 uart4 = {
    .hw      = UART4_BASE,
    .irq_num = UART4_IRQn,
};

struct hguart_v4 uart5 = {
    .hw      = UART5_BASE,
    .irq_num = UART5_IRQn,
};

struct hguart_v4 uart6 = {
    .hw      = UART6_BASE,
    .irq_num = UART6_IRQn,
};

struct hggpio_v4 gpioa = {
    .hw      = GPIOA_BASE,
    .irq_num = GPIOA_IRQn,
    .pin_num = {PA_0, PA_15},
};

struct hggpio_v4 gpiob = {
    .hw      = GPIOB_BASE,
    .irq_num = GPIOB_IRQn,
    .pin_num = {PB_0, PB_15},
};

struct hggpio_v4 gpioc = {
    .hw      = GPIOC_BASE,
    .irq_num = GPIOC_IRQn,
    .pin_num = {PC_0, PC_15},
};

struct hggpio_v4 gpiod = {
    .hw      = GPIOD_BASE,
    .irq_num = GPIOD_IRQn,
    .pin_num = {PD_0, PD_15},
};

struct hggpio_v4 gpioe = {
    .hw      = GPIOE_BASE,
    .irq_num = GPIOE_IRQn,
    .pin_num = {PE_0, PE_3},
};

struct mem_dma_dev mem_dma = {
    .hw      = (void *)M2M_DMA_BASE,
#if CONFI_CORE_M2M_DMA
    .irq_num = {M2M_DMA0_IRQn, M2M_DMA1_IRQn},
#else
    .irq_num = {M2M_DMA0_IRQn, M2M_DMA1_IRQn, M2M_DMA2_IRQn},
#endif
};

struct hgadc_v1 adc0 = {
    .hw      = ADKEY_BASE,
    .irq_num = ADKEY0_IRQn,
};

struct hglcdc lcdc = {
    .hw = LCDC_BASE,
    .irq_num = LCD_IRQn,
};

struct hgdsi dsic = {
    .hw = MIPI_DSI_BASE,
    .irq_num = 0,
};

struct hgjpg jpg0 = {
    .hw = MJPEG0_BASE,
#ifdef FPGA_SUPPORT
    .thw = MJPEG0_TAB_BASE,
#else
    .thw = MJPEG0_TAB_BASE,
#endif
    .huf_hw = MJPEG_HUFF_BASE,

    .irq_num = MJPEG01_IRQn,
};

struct hgjpg jpg1 = {
    .hw = MJPEG1_BASE,
#ifdef FPGA_SUPPORT
    .thw = MJPEG1_TAB_BASE,
#else
    .thw = MJPEG1_TAB_BASE,
#endif
    .huf_hw = MJPEG_HUFF_BASE,
    
    .irq_num = MJPEG01_IRQn,
};

struct hgsdh sdh = {
    .hw = SDHOST_BASE,
    .irq_num = SDHOST_IRQn,
};

struct hgdvp dvp = {
    .hw      = DVP_BASE,
    .irq_num = DVP_IRQn,
};

struct hgmipi_csi csi0 = {
    .hw      = MIPI_CSI0_BASE,
    .irq_num = MIPI_CSI2_IRQn,
};

struct hgmipi_csi csi1 = {
    .hw      = MIPI_CSI1_BASE,
    .irq_num = MIPI1_CSI2_IRQn,
};

struct hgvpp vpp = {
    .hw      = VPP_BASE,
    .irq_num = VPP_IRQn,
};

struct hgprc prc = {
    .hw      = PRC_BASE,
    .irq_num = PRC_IRQn,
};

struct hgscale scale1 = {
    .hw = SCALE1_BASE,
    .irq_num = SCALE1_IRQn,
};

struct hgscale scale2 = {
    .hw = SCALE2_BASE,
    .irq_num = SCALE2_IRQn,
};

struct hgscale scale3 = {
    .hw = SCALE3_BASE,
    .irq_num = SCALE3_IRQn,
};

struct hggen420 gen420 = {
    .hw = GEN420_BASE,
    .irq_num = GEN420_IRQn,
};

struct hggen422 gen422 = {
    .hw = GEN422_BASE,
    .irq_num = GEN422_IRQn,
};

struct hg264 h264 = {
    .hw      = H264_REG_BASE,
    .irq_num = H264ENC_IRQn,
};

struct hgdual dual = {
    .hw      = DUAL_ORG_BASE,
    .irq_num = DUAL_ORG_IRQn,
};

struct hgosd osdenc = {
    .hw      = OSD_ENC_BASE,
    .irq_num = OSD_ENC_IRQn,
};

struct hgcsc csc = {
    .hw      = CSC_BASE,
    .irq_num = CSC_IRQn,
};

struct hgtimer_v4 timer0 = {
    .hw      = TIMER0_BASE,
    .irq_num = TIM0_IRQn,
};

struct hgtimer_v4 timer1 = {
    .hw      = TIMER1_BASE,
    .irq_num = TIM1_IRQn,
};

struct hgtimer_v4 timer2 = {
    .hw      = TIMER2_BASE,
    .irq_num = TIM2_IRQn,
};

struct hgtimer_v4 timer3 = {
    .hw      = TIMER3_BASE,
    .irq_num = TIM3_IRQn,
};

struct hgspi_v3 spi0 = {
    .hw      = SPI0_BASE,
    .irq_num = SPI0_IRQn,
};

struct hgspi_v3 spi1 = {
    .hw      = SPI1_BASE,
    .irq_num = SPI1_IRQn,
};

struct hgspi_v3 spi2 = {
    .hw      = SPI2_BASE,
    .irq_num = SPI2_IRQn,
};

struct hgspi_xip spi7 = {
    .hw      = QSPI_BASE,
    .ddr     = 0,
};

struct hgcqspi cqspi = {
    .hw = (void *)QSPI_BASE,
    .irq_num = QSPI_IRQn,
    .opened = 0,
};

struct hgi2c_v1 iic1 = {
    .hw      = IIC1_BASE,
    .irq_num = SPI1_IRQn,
};

struct hgi2c_v1 iic2 = {
    .hw      = IIC2_BASE,
    .irq_num = SPI2_IRQn,
};

struct hgpwm_v0 pwm = {
    .channel[0] = (void *) &timer0,
    .channel[1] = (void *) &timer1,
    .channel[2] = (void *) &timer2,
};

struct hgisp_v0 isp = {
    .hw           = IMAGE_ISP_BASE,
    .data_hw      = ISP_R_GAMMA_BASE,
    .irq_num      = IMG_ISP_IRQn,
};

struct hgdma2d_v0 dma2d = {
    .hw      = DMA2D_BASE,
    .fgclut  = DMA2D_FGCLUT_BASE,
    .bgclut  = DMA2D_BGCLUT_BASE,
    .irq_num = DMA2D_IRQn,
};

struct spi_nor_flash flash0 = {
    .spidev      = (struct spi_device *)&spi7,
    .spi_config  = {12000000, SPI_CLK_MODE_0, SPI_WIRE_NORMAL_MODE, 0},
    .vendor_id   = 0,
    .product_id  = 0,
    .size        = 0x200000, /* Special External-Flash Size */
    .block_size  = 0x10000,
    .sector_size = 0x1000,
    .page_size   = 4096,
    .mode        = SPI_NOR_XIP_MODE,

};

struct hg_audio_v0 auadc = {
    .hw       = AUDIO_BASE,
    .irq_num  = AUDIO_SUBSYS1_IRQn,
    .p_comm   = (void *)&auadc.comm_dat,
//    .comm_dat.comm_bits.ana_driver_version = 1,
    .dev_type = AUDIO_TYPE_AUADC,
};

struct hg_audio_v0 audac = {
    .hw       = AUDIO_BASE,
    .irq_num  = AUDIO_SUBSYS0_IRQn,
    .p_comm   = (void *)&auadc.comm_dat,
    .dev_type = AUDIO_TYPE_AUDAC,
};

struct hg_audio_v0 auasrc = {
    .hw       = AUDIO_ASRC_BASE,
    .p_comm   = (void *)&auadc.comm_dat,
    .dev_type = AUDIO_TYPE_AUASRC,
};

struct hg_audio_v0 aueq = {
    .hw       = AUDIO_EQ_BASE,
    .p_comm   = (void *)&auadc.comm_dat,
    .dev_type = AUDIO_TYPE_AUEQ,
};

struct hg_audio_v0 aufade = {
    .hw       = AUDIO_BASE,
    .p_comm   = (void *)&auadc.comm_dat,
    .dev_type = AUDIO_TYPE_AUFADE,
};

struct hgsha_v1 sha = {
	.hw = (void *)SHA_BASE,
	.irq_num = SHA_IRQn,
};

static void core_vdd_voltage()
{
    uint32 core_vdd_voltage = 0;
    sar_adc_sample_one_channel(ADC_CHANNEL_CORE_VDD, &core_vdd_voltage);
    os_printf("CoreVdd Voltage:%.2fV\r\n", ((double)core_vdd_voltage/2048.0)*3.0);
}

void device_init(void)
{
    extern uint32_t get_flash_cap();
    uint32_t flash_size = get_flash_cap();
    if (flash_size > 0) {
        flash0.size = flash_size;
    }

    hg_crc_attach(HG_CRC_DEVID, &crc32_module);
    hg_sysaes_v3_attach(HG_HWAES0_DEVID, &sysaes);
    hggpio_v4_attach(HG_GPIOA_DEVID, &gpioa);
    hggpio_v4_attach(HG_GPIOB_DEVID, &gpiob);
    pmu_vccsd_power_set(1, VCCSD_33);
    hggpio_v4_attach(HG_GPIOC_DEVID, &gpioc);
    hggpio_v4_attach(HG_GPIOD_DEVID, &gpiod);
    hggpio_v4_attach(HG_GPIOE_DEVID, &gpioe);
    hgadc_v1_attach(HG_ADC0_DEVID, &adc0);
    hguart_v2_attach(HG_UART0_DEVID, &uart0);
    hguart_v2_attach(HG_UART1_DEVID, &uart1);
    hgtimer_v4_attach(HG_TIMER0_DEVID, &timer0);
    hgtimer_v4_attach(HG_TIMER1_DEVID, &timer1);
    hgtimer_v4_attach(HG_TIMER2_DEVID, &timer2);
    hgtimer_v4_attach(HG_TIMER3_DEVID, &timer3);

#ifndef SINGLE_CORE
    sysctrl_cpu1_softrst_en();
    sysctrl_cpu1_clk_en();
    cpu_splock_init(CPU0_SPLCK_BASE, CPU_SPINLOCK_IRQn);
#endif
    uart_open((struct uart_device *)&uart0, 921600); //for CPU0
    uart_open((struct uart_device *)&uart1, 921600); //for CPU1

#if USB_EN
#if USB_HOST_EN
    hgusb20_host_attach(HG_USBDEV_DEVID, &usb20_dev);
#else
    hgusb20_dev_attach(HG_USBDEV_DEVID, &usb20_dev);
#endif
#endif

#if USB11_EN
    hgusb11_v0_dev_attach(HG_USB11DEV_DEVID, &usb11_dev);
    hgusb11_v0_host_attch(HG_USB11HOST_DEVID, &usb11_host);
#endif

#if LCD_EN
    hglcdc_attach(HG_LCDC_DEVID,&lcdc);
    hgdsi_attach(HG_DSI_DEVID,&dsic);
#endif
	hgosdenc_attach(HG_OSD_ENC_DEVID,&osdenc);

#if ISP_EN
    hgisp_v0_attach(HG_ISP_DEVID, &isp);
#endif

#if DVP_EN
    hgdvp_attach(HG_DVP_DEVID,&dvp);
#endif

#if MIPI_CSI_EN
    hgmipi_csi_attach(HG_MIPI_CSI_DEVID,&csi0);
    hgmipi_csi_attach(HG_MIPI1_CSI_DEVID,&csi1);
#endif

#if VPP_EN
    hgvpp_attach(HG_VPP_DEVID,&vpp);
#endif
    


#if H264_EN
    hg264_attach(HG_H264_DEVID, &h264);
#endif

#if SDH_EN
    hgsdh_attach(HG_SDIOHOST_DEVID, &sdh);
#endif

#if SCALE_EN
    hgscale1_attach(HG_SCALE1_DEVID, &scale1);
    hgscale2_attach(HG_SCALE2_DEVID, &scale2);
    hgscale3_attach(HG_SCALE3_DEVID, &scale3);
#endif

#if DMA2D_EN
    hgdma2d_v0_attach(HG_DMA2D_DEVID, &dma2d);
#endif

#if PARA_IN_EN
hgpara_in_attach(HG_PARA_IN_DEVID, &para_in);
#endif

    hgpwm_v0_attach(HG_PWM0_DEVID, &pwm);

    hgdual_attach(HG_DUALORG_DEVID,&dual);
    hgcsc_attach(HG_CSC_DEVID,&csc);

    hggen420_attach(HG_GEN420_DEVID,&gen420);
    hggen422_attach(HG_GEN422_DEVID,&gen422);
    hgprc_attach(HG_PRC_DEVID,&prc);


    console_handle = &uart0;

#if SD_MODE_TYPE != 2
    hgspi_v3_attach(HG_SPI0_DEVID, &spi0);
#endif

    hgspi_xip_attach(HG_SPI7_DEVID, &spi7);

    spi_nor_attach(&flash0, HG_FLASH0_DEVID);
    

//  hgspi_v3_attach(HG_SPI1_DEVID, &spi1);  
    hgi2c_v1_attach(HG_I2C1_DEVID, &iic1);
#if SD_MODE_TYPE != 3
    hgi2c_v1_attach(HG_I2C2_DEVID, &iic2);
#endif
    hg_audio_v0_attach(HG_AUADC_DEVID, &auadc);
    hg_audio_v0_attach(HG_AUDAC_DEVID, &audac);
    hg_audio_v0_attach(HG_AUASRC_DEVID, &auasrc);
    hg_audio_v0_attach(HG_AUEQ_DEVID, &aueq);
    hg_audio_v0_attach(HG_AUFADE_DEVID, &aufade);

    hgsha_v1_attach(HG_SHA_DEVID, &sha);
	
    hg_m2m_dma_dev_attach(HG_M2MDMA_DEVID, &mem_dma);

    m2mdma = (struct dma_device *)&mem_dma;

    void sysctrl_mipi_lcd_h264_mjpeg_clk_init(void);
    sysctrl_mipi_lcd_h264_mjpeg_clk_init();

#if JPG_EN
    hgjpg_attach(HG_JPG0_DEVID, &jpg0);
    hgjpg_attach(HG_JPG1_DEVID, &jpg1);
#endif

    extern struct hg_xspi ospi;
    hg_xspi_attach(HG_XSPI_DEVID, &ospi);

    device_burst_set(); 
    os_printf(KERN_INFO"CPU0 init!\r\n");
    core_vdd_voltage();
}



void device_burst_set(void)
{
    *(unsigned int *) 0x400201d0 = 0xffffffff;
    *(unsigned int *) 0x400201d4 = 0xffffffff;
    
    // SYSCTRL->AHB2AHB_FIFO_CTRL = 0X1FFFFF7;

    ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_DUAL_ORG0_WR, DMA2AHB_BURST_SIZE_256);
    ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_DUAL_ORG1_WR, DMA2AHB_BURST_SIZE_256);
    ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_DUAL_ORG_RD, DMA2AHB_BURST_SIZE_256);
    ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_ROTATE_IN_RD, DMA2AHB_BURST_SIZE_32);

    ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_M2M0_WR, DMA2AHB_BURST_SIZE_256);
    ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_M2M0_RD, DMA2AHB_BURST_SIZE_256);  
    ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_M2M1_WR, DMA2AHB_BURST_SIZE_256);
    ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_M2M1_RD, DMA2AHB_BURST_SIZE_256); 
    ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_SDHOST1_WR, DMA2AHB_BURST_SIZE_256);
    ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_M2M2_RD, DMA2AHB_BURST_SIZE_256); 

	//ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_SCALE3_Y_WR, DMA2AHB_BURST_SIZE_256);
    //ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_SCALE3_U_WR, DMA2AHB_BURST_SIZE_256);
    //ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_SCALE3_V_WR, DMA2AHB_BURST_SIZE_256);
    ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_VPP_Y_WR, DMA2AHB_BURST_SIZE_256);
    ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_VPP_U_WR, DMA2AHB_BURST_SIZE_256);
    ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_GMAC_RX_WR, DMA2AHB_BURST_SIZE_256);

    SCHED->BW_STA_CYCLE = DEFAULT_SYS_CLK; //SCHED BW static
    SCHED->CTRL_CON |= BIT(0) | BIT(1) | BIT(6);

//    gpio_iomap_output(PC_0, GPIO_IOMAP_OUT_QSPI_OSPI_MNT_O_0); //CS
//    gpio_iomap_output(PC_1, GPIO_IOMAP_OUT_QSPI_OSPI_MNT_O_1); //CLK
//    gpio_iomap_output(PC_2, GPIO_IOMAP_OUT_QSPI_OSPI_MNT_O_2); //CS
//    gpio_iomap_output(PC_3, GPIO_IOMAP_OUT_QSPI_OSPI_MNT_O_3); //CLK
//    gpio_iomap_output(PC_4, GPIO_IOMAP_OUT_QSPI_OSPI_MNT_O_4); //CS
//    gpio_iomap_output(PC_9, GPIO_IOMAP_OUT_QSPI_OSPI_MNT_O_5); //CLK      
    //SYSCTRL->IOFUNCMASK8 |= BIT(1);
    
}

struct gpio_device *gpio_get(uint32 pin)
{
    if (pin >= gpioa.pin_num[0] && pin <= gpioa.pin_num[1]) {
        return (struct gpio_device *)&gpioa;
    } else if (pin >= gpiob.pin_num[0] && pin <= gpiob.pin_num[1]) {
        return (struct gpio_device *)&gpiob;
    } else if (pin >= gpioc.pin_num[0] && pin <= gpioc.pin_num[1]) {
        return (struct gpio_device *)&gpioc;
    } else if (pin >= gpiod.pin_num[0] && pin <= gpiod.pin_num[1]) {
        return (struct gpio_device *)&gpiod;
    }else if (pin >= gpioe.pin_num[0] && pin <= gpioe.pin_num[1]) {
        return (struct gpio_device *)&gpioe;
    }
    return NULL;
}

int32 syscfg_info_get(struct syscfg_info *pinfo, const char *name)
{
#if SYSCFG_ENABLE
    if (strcmp(name, "syscfg") == 0) {
        pinfo->flash1 = &flash0;
        pinfo->flash2 = &flash0;
        pinfo->size  = pinfo->flash1->sector_size;
        pinfo->addr1 = pinfo->flash1->size - (2 * pinfo->size);
        pinfo->addr2 = pinfo->flash1->size - pinfo->size;
        printf("syscfg: pinfo->size:%d  sys_config:%d  sector_size:%d\r\n",pinfo->size,sizeof(struct sys_config),pinfo->flash1->sector_size);
        ASSERT((pinfo->addr1 & ~(pinfo->flash1->sector_size - 1)) == pinfo->addr1);
        ASSERT((pinfo->addr2 & ~(pinfo->flash2->sector_size - 1)) == pinfo->addr2);
        ASSERT((pinfo->size >= sizeof(struct sys_config)) &&
               (pinfo->size == (pinfo->size & ~(pinfo->flash1->sector_size - 1))));
    } else if (strcmp(name, "bluetooth") == 0) {
        pinfo->flash1 = &flash0;
        pinfo->flash2 = &flash0;
        pinfo->size  = pinfo->flash1->sector_size;
        pinfo->addr1 = pinfo->flash1->size - (pinfo->size << 2);
        pinfo->addr2 = pinfo->flash1->size - ((pinfo->size << 1) + pinfo->size);
        printf("bluetooth: pinfo->size:%d sector_size:%d\r\n",pinfo->size, pinfo->flash1->sector_size);
        ASSERT((pinfo->addr1 & ~(pinfo->flash1->sector_size - 1)) == pinfo->addr1);
        ASSERT((pinfo->addr2 & ~(pinfo->flash2->sector_size - 1)) == pinfo->addr2);
    } else if (strcmp(name, "recorder") == 0) {
        pinfo->flash1 = &flash0;
        pinfo->flash2 = &flash0;
        pinfo->size  = pinfo->flash1->sector_size;
        pinfo->addr1 = pinfo->flash1->size - (6 * pinfo->size);
        pinfo->addr2 = pinfo->flash1->size - (5 * pinfo->size);
        printf("recorder: pinfo->size:%d sector_size:%d\r\n",pinfo->size, pinfo->flash1->sector_size);
        ASSERT((pinfo->addr1 & ~(pinfo->flash1->sector_size - 1)) == pinfo->addr1);
        ASSERT((pinfo->addr2 & ~(pinfo->flash2->sector_size - 1)) == pinfo->addr2);
    } else {
        return -1;
    }
    return 0;
    
#else
    return -1;
#endif  
}

int32 ota_fwinfo_get(struct ota_fwinfo *pinfo)
{
    uint32_t loader_bytes = get_boot_loader_offset();
    pinfo->flash0 = &flash0;
    pinfo->flash1 = &flash0;
    pinfo->addr0  = 0 + loader_bytes;
    pinfo->addr1  = (flash0.size / 2) + loader_bytes;
    return 0;
}
