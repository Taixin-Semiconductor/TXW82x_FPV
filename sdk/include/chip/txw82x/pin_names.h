// Revision History
// V1.0.0  06/01/2019  First Release, copy from 4001a project
// V1.0.1  07/27/2019  change uart0 to A2/A3
// V1.0.2  02/07/2020  change PIN_SPI_CS to PIN_SPI0_CS
// V1.1.0  03/02/2020  add pa/pa-vmode pin

#ifndef _PIN_NAMES_DEF_H_
#define _PIN_NAMES_DEF_H_

//#include "sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------*/
/*----------UART PIN DEFINITION----------*/
/*---------------------------------------*/

/* UART0 */
#ifndef PIN_UART0_TX
#define PIN_UART0_TX 255
#endif

#ifndef PIN_UART0_RX
#define PIN_UART0_RX 255
#endif

#ifndef PIN_UART0_DE
#define PIN_UART0_DE 255
#endif

#ifndef PIN_UART0_RE
#define PIN_UART0_RE 255
#endif

/* UART1 */
#ifndef PIN_UART1_TX
#define PIN_UART1_TX 255
#endif

#ifndef PIN_UART1_RX
#define PIN_UART1_RX 255
#endif

/* UART4 */
#ifndef PIN_UART4_TX
#define PIN_UART4_TX 255
#endif

#ifndef PIN_UART4_RX
#define PIN_UART4_RX 255
#endif

#ifndef PIN_UART4_DE
#define PIN_UART4_DE 255
#endif

#ifndef PIN_UART4_RE
#define PIN_UART4_RE 255
#endif

/* UART5 */
#ifndef PIN_UART5_TX
#define PIN_UART5_TX 255
#endif

#ifndef PIN_UART5_RX
#define PIN_UART5_RX 255
#endif

#ifndef PIN_UART5_DE
#define PIN_UART5_DE 255
#endif

#ifndef PIN_UART5_RE
#define PIN_UART5_RE 255
#endif

/* UART6 */
#ifndef PIN_UART6_TX
#define PIN_UART6_TX 255
#endif

#ifndef PIN_UART6_RX
#define PIN_UART6_RX 255
#endif

#ifndef PIN_UART6_DE
#define PIN_UART6_DE 255
#endif

#ifndef PIN_UART6_RE
#define PIN_UART6_RE 255
#endif


/*---------------------------------------*/
/*----------IIC PIN DEFINITION-----------*/
/*---------------------------------------*/

/* IIC1 */
#ifndef PIN_IIC1_SCL
#define PIN_IIC1_SCL 255
#endif

#ifndef PIN_IIC1_SDA
#define PIN_IIC1_SDA 255
#endif

/* IIC2 */
#ifndef PIN_IIC2_SCL
#define PIN_IIC2_SCL 255
#endif

#ifndef PIN_IIC2_SDA
#define PIN_IIC2_SDA 255
#endif


/*---------------------------------------*/
/*----------IIS PIN DEFINITION-----------*/
/*---------------------------------------*/

/* IIS0 */
#ifndef PIN_IIS0_MCLK
#define PIN_IIS0_MCLK 255
#endif

#ifndef PIN_IIS0_BCLK
#define PIN_IIS0_BCLK 255
#endif

#ifndef PIN_IIS0_WCLK
#define PIN_IIS0_WCLK 255
#endif

#ifndef PIN_IIS0_DATA
#define PIN_IIS0_DATA 255
#endif

/* IIS1 */
#ifndef PIN_IIS1_MCLK
#define PIN_IIS1_MCLK 255
#endif

#ifndef PIN_IIS1_BCLK
#define PIN_IIS1_BCLK 255
#endif

#ifndef PIN_IIS1_WCLK
#define PIN_IIS1_WCLK 255
#endif

#ifndef PIN_IIS1_DATA
#define PIN_IIS1_DATA 255
#endif


/*---------------------------------------*/
/*----------PDM PIN DEFINITION-----------*/
/*---------------------------------------*/

/* PDM */
#ifndef PIN_PDM_MCLK
#define PIN_PDM_MCLK 255
#endif

#ifndef PIN_PDM_DATA
#define PIN_PDM_DATA 255
#endif


/*---------------------------------------*/
/*----------LED PIN DEFINITION-----------*/
/*---------------------------------------*/

/* LED */
#ifndef PIN_LED_SEG0
#define PIN_LED_SEG0 255
#endif

#ifndef PIN_LED_SEG1
#define PIN_LED_SEG1  255
#endif

#ifndef PIN_LED_SEG2
#define PIN_LED_SEG2  255
#endif

#ifndef PIN_LED_SEG3
#define PIN_LED_SEG3  255
#endif

#ifndef PIN_LED_SEG4
#define PIN_LED_SEG4  255
#endif

#ifndef PIN_LED_SEG5
#define PIN_LED_SEG5  255
#endif

#ifndef PIN_LED_SEG6
#define PIN_LED_SEG6  255
#endif

#ifndef PIN_LED_SEG7
#define PIN_LED_SEG7  255
#endif

#ifndef PIN_LED_SEG8
#define PIN_LED_SEG8  255
#endif

#ifndef PIN_LED_SEG9
#define PIN_LED_SEG9  255
#endif

#ifndef PIN_LED_SEG10 
#define PIN_LED_SEG10 255
#endif

#ifndef PIN_LED_SEG11
#define PIN_LED_SEG11 255
#endif

#ifndef PIN_LED_COM0
#define PIN_LED_COM0 255
#endif

#ifndef PIN_LED_COM1
#define PIN_LED_COM1 255
#endif

#ifndef PIN_LED_COM2
#define PIN_LED_COM2 255
#endif

#ifndef PIN_LED_COM3
#define PIN_LED_COM3 255
#endif

#ifndef PIN_LED_COM4
#define PIN_LED_COM4 255
#endif

#ifndef PIN_LED_COM5
#define PIN_LED_COM5 255
#endif

#ifndef PIN_LED_COM6
#define PIN_LED_COM6 255
#endif

#ifndef PIN_LED_COM7
#define PIN_LED_COM7 255
#endif


/*---------------------------------------*/
/*---------GMAC PIN DEFINITION-----------*/
/*---------------------------------------*/

/* GMAC */

#ifndef PIN_GMAC_RMII_REF_CLKIN
#define PIN_GMAC_RMII_REF_CLKIN 255
#endif

#ifndef PIN_GMAC_RMII_RXD0
#define PIN_GMAC_RMII_RXD0      255
#endif

#ifndef PIN_GMAC_RMII_RXD1
#define PIN_GMAC_RMII_RXD1      255
#endif

#ifndef PIN_GMAC_RMII_TXD0
#define PIN_GMAC_RMII_TXD0      255
#endif

#ifndef PIN_GMAC_RMII_TXD1
#define PIN_GMAC_RMII_TXD1      255
#endif

#ifndef PIN_GMAC_RMII_CRS_DV
#define PIN_GMAC_RMII_CRS_DV    255
#endif

#ifndef PIN_GMAC_RMII_TX_EN
#define PIN_GMAC_RMII_TX_EN     255
#endif

#ifndef PIN_GMAC_RMII_MDIO
#define PIN_GMAC_RMII_MDIO      255
#endif

#ifndef PIN_GMAC_RMII_MDC
#define PIN_GMAC_RMII_MDC       255
#endif



/*---------------------------------------*/
/*---------SDIO PIN DEFINITION-----------*/
/*---------------------------------------*/

/* SDIO */
#ifndef PIN_SDCLK
#define PIN_SDCLK  255
#endif

#ifndef PIN_SDCMD
#define PIN_SDCMD  255
#endif

#ifndef PIN_SDDAT0
#define PIN_SDDAT0 255
#endif

#ifndef PIN_SDDAT1
#define PIN_SDDAT1 255
#endif

#ifndef PIN_SDDAT2
#define PIN_SDDAT2 255
#endif

#ifndef PIN_SDDAT3
#define PIN_SDDAT3 255
#endif


/*---------------------------------------*/
/*---------QSPI PIN DEFINITION-----------*/
/*---------------------------------------*/

/* QSPI */
#ifndef PIN_QSPI_CS
#define PIN_QSPI_CS  255
#endif

#ifndef PIN_QSPI_CLK
#define PIN_QSPI_CLK 255
#endif

#ifndef PIN_QSPI_IO0
#define PIN_QSPI_IO0 255
#endif

#ifndef PIN_QSPI_IO1
#define PIN_QSPI_IO1 255
#endif

#ifndef PIN_QSPI_IO2
#define PIN_QSPI_IO2 255
#endif

#ifndef PIN_QSPI_IO3
#define PIN_QSPI_IO3 255
#endif

#ifndef PIN_QSPI_CS1
#define PIN_QSPI_CS1 255
#endif


/*---------------------------------------*/
/*---------SPI PIN DEFINITION------------*/
/*---------------------------------------*/

/* SPI0 */
#ifndef PIN_SPI0_CS
#define PIN_SPI0_CS  255
#endif

#ifndef PIN_SPI0_CLK
#define PIN_SPI0_CLK 255
#endif

#ifndef PIN_SPI0_IO0
#define PIN_SPI0_IO0 255
#endif

#ifndef PIN_SPI0_IO1
#define PIN_SPI0_IO1 255
#endif

#ifndef PIN_SPI0_IO2
#define PIN_SPI0_IO2 255
#endif

#ifndef PIN_SPI0_IO3
#define PIN_SPI0_IO3 255
#endif


/* SPI1 */
#ifndef PIN_SPI1_CS
#define PIN_SPI1_CS  255
#endif

#ifndef PIN_SPI1_CLK
#define PIN_SPI1_CLK 255
#endif

#ifndef PIN_SPI1_IO0
#define PIN_SPI1_IO0 255
#endif

#ifndef PIN_SPI1_IO1
#define PIN_SPI1_IO1 255
#endif

#ifndef PIN_SPI1_IO2
#define PIN_SPI1_IO2 255
#endif

#ifndef PIN_SPI1_IO3
#define PIN_SPI1_IO3 255
#endif

/* SPI5 */
#ifndef PIN_SPI5_CLK
#define PIN_SPI5_CLK 255
#endif

#ifndef PIN_SPI5_IO0
#define PIN_SPI5_IO0 255
#endif

#ifndef PIN_SPI5_IO1
#define PIN_SPI5_IO1 255
#endif

/* SPI6 */
#ifndef PIN_SPI6_CLK
#define PIN_SPI6_CLK 255
#endif

#ifndef PIN_SPI6_IO0
#define PIN_SPI6_IO0 255
#endif

#ifndef PIN_SPI6_IO1
#define PIN_SPI6_IO1 255
#endif


/*---------------------------------------*/
/*---------ADC PIN DEFINITION------------*/
/*---------------------------------------*/

/* ADC */
#ifndef PIN_ADC
#define PIN_ADC 255
#endif


/*---------------------------------------*/
/*---------DVP PIN DEFINITION------------*/
/*---------------------------------------*/

/* DVP */
#ifndef PIN_DVP_HSYNC
#define PIN_DVP_HSYNC  PB_7
#endif

#ifndef PIN_DVP_VSYNC
#define PIN_DVP_VSYNC  PB_6
#endif

#ifndef PIN_DVP_PCLK
#define PIN_DVP_PCLK   PB_12
#endif

#ifndef PIN_DVP_MCLK
#define PIN_DVP_MCLK   PB_9
#endif

#ifndef PIN_DVP_DATA0
#define PIN_DVP_DATA0  PB_14
#endif

#ifndef PIN_DVP_DATA1
#define PIN_DVP_DATA1  PC_0
#endif

#ifndef PIN_DVP_DATA2
#define PIN_DVP_DATA2  PC_1
#endif

#ifndef PIN_DVP_DATA3
#define PIN_DVP_DATA3  PB_15
#endif

#ifndef PIN_DVP_DATA4
#define PIN_DVP_DATA4  PB_13
#endif

#ifndef PIN_DVP_DATA5
#define PIN_DVP_DATA5  PB_11
#endif

#ifndef PIN_DVP_DATA6
#define PIN_DVP_DATA6  PB_10
#endif

#ifndef PIN_DVP_DATA7
#define PIN_DVP_DATA7  PB_8
#endif

#ifndef PIN_DVP_DATA8
#define PIN_DVP_DATA8  255
#endif

#ifndef PIN_DVP_DATA9
#define PIN_DVP_DATA9  255
#endif

#ifndef PIN_DVP_DATA10
#define PIN_DVP_DATA10  255
#endif

#ifndef PIN_DVP_DATA11
#define PIN_DVP_DATA11  255
#endif


#ifndef PIN_DVP_RESET
#define PIN_DVP_RESET  255
#endif

#ifndef PIN_DVP_PDN
#define PIN_DVP_PDN    255
#endif

/*---------------------------------------*/
/*---------PARA IN PIN DEFINITION------------*/
/*---------------------------------------*/

/* BT */
#ifndef PIN_BT_HSYNC
#define PIN_BT_HSYNC  255
#endif

#ifndef PIN_BT_VSYNC
#define PIN_BT_VSYNC  255
#endif

#ifndef PIN_BT_PCLK
#define PIN_BT_PCLK   255
#endif

#ifndef PIN_BT_DATA0
#define PIN_BT_DATA0  255
#endif

#ifndef PIN_BT_DATA1
#define PIN_BT_DATA1  255
#endif

#ifndef PIN_BT_DATA2
#define PIN_BT_DATA2  255
#endif

#ifndef PIN_BT_DATA3
#define PIN_BT_DATA3  255
#endif

#ifndef PIN_BT_DATA4
#define PIN_BT_DATA4  255
#endif

#ifndef PIN_BT_DATA5
#define PIN_BT_DATA5  255
#endif

#ifndef PIN_BT_DATA6
#define PIN_BT_DATA6  255
#endif

#ifndef PIN_BT_DATA7
#define PIN_BT_DATA7  255
#endif

#ifndef PIN_BT_BLANK
#define PIN_BT_BLANK  255
#endif

#ifndef PIN_BT_RESET
#define PIN_BT_RESET  255
#endif

#ifndef PIN_BT_PDN
#define PIN_BT_PDN    255
#endif

/*---------------------------------------*/
/*---------SDH PIN DEFINITION------------*/
/*---------------------------------------*/

/* SDH */
#ifndef PIN_SDH_CLK
#define PIN_SDH_CLK  255
#endif

#ifndef PIN_SDH_CLK_DRI_STRENGTH
#define PIN_SDH_CLK_DRI_STRENGTH    GPIO_DS_G1
#endif

#ifndef PIN_SDH_CMD
#define PIN_SDH_CMD  255
#endif

#ifndef PIN_SDH_DAT0
#define PIN_SDH_DAT0 255
#endif

#ifndef PIN_SDH_DAT1
#define PIN_SDH_DAT1 255
#endif

#ifndef PIN_SDH_DAT2
#define PIN_SDH_DAT2 255
#endif

#ifndef PIN_SDH_DAT3
#define PIN_SDH_DAT3 255
#endif

/*---------------------------------------*/
/*---------LCD PIN DEFINITION------------*/
/*---------------------------------------*/
#ifndef DOTCLK_RWR
#define DOTCLK_RWR 255
#endif
#ifndef DE_ERD
#define DE_ERD 255
#endif
#ifndef VS_CS
#define VS_CS  255
#endif
#ifndef HS_DC
#define HS_DC  255
#endif
#ifndef LCD_TE
#define LCD_TE 255
#endif
#ifndef LCD_D0
#define LCD_D0 255
#endif
#ifndef LCD_D1
#define LCD_D1 255
#endif
#ifndef LCD_D2
#define LCD_D2 255
#endif
#ifndef LCD_D3
#define LCD_D3 255
#endif
#ifndef LCD_D4
#define LCD_D4 255
#endif
#ifndef LCD_D5
#define LCD_D5 255
#endif
#ifndef LCD_D6
#define LCD_D6 255
#endif
#ifndef LCD_D7
#define LCD_D7 255
#endif
#ifndef LCD_D8
#define LCD_D8 255
#endif
#ifndef LCD_D9
#define LCD_D9 255
#endif
#ifndef LCD_D10
#define LCD_D10 255
#endif
#ifndef LCD_D11
#define LCD_D11 255
#endif
#ifndef LCD_D12
#define LCD_D12 255
#endif
#ifndef LCD_D13
#define LCD_D13 255
#endif
#ifndef LCD_D14
#define LCD_D14 255
#endif
#ifndef LCD_D15
#define LCD_D15 255
#endif
#ifndef LCD_D16
#define LCD_D16 255
#endif
#ifndef LCD_D17
#define LCD_D17 255
#endif
#ifndef LCD_D18
#define LCD_D18 255
#endif
#ifndef LCD_D19
#define LCD_D19 255
#endif
#ifndef LCD_D20
#define LCD_D20 255
#endif
#ifndef LCD_D21
#define LCD_D21 255
#endif
#ifndef LCD_D22
#define LCD_D22 255
#endif
#ifndef LCD_D23
#define LCD_D23 255
#endif

/////////////////////////MIPI CSI////////////////////////////////////////
#ifndef PIN_MIPI_CSI0_CLKN
#define PIN_MIPI_CSI0_CLKN                 255
#endif

#ifndef PIN_MIPI_CSI0_CLKP
#define PIN_MIPI_CSI0_CLKP                 255
#endif

#ifndef PIN_MIPI_CSI0_D0N
#define PIN_MIPI_CSI0_D0N                 255
#endif

#ifndef PIN_MIPI_CSI0_D0P
#define PIN_MIPI_CSI0_D0P                 255
#endif

#ifndef PIN_MIPI_CSI0_D1N_CSI1_D0N
#define PIN_MIPI_CSI0_D1N_CSI1_D0N        255
#endif

#ifndef PIN_MIPI_CSI0_D1P_CSI1_D0P
#define PIN_MIPI_CSI0_D1P_CSI1_D0P        255
#endif

#ifndef PIN_MIPI_CSI1_CLKN
#define PIN_MIPI_CSI1_CLKN                 255
#endif

#ifndef PIN_MIPI_CSI1_CLKP
#define PIN_MIPI_CSI1_CLKP                 255
#endif

/////////////////////////MIPI DSI////////////////////////////////////////
#ifndef PIN_MIPI_DSI_D0N
#define PIN_MIPI_DSI_D0N                 255
#endif
#ifndef PIN_MIPI_DSI_D0P
#define PIN_MIPI_DSI_D0P                 255
#endif
#ifndef PIN_MIPI_DSI_D1N
#define PIN_MIPI_DSI_D1N                 255
#endif
#ifndef PIN_MIPI_DSI_D1P
#define PIN_MIPI_DSI_D1P                 255
#endif
#ifndef PIN_MIPI_DSI_D2N
#define PIN_MIPI_DSI_D2N                 255
#endif
#ifndef PIN_MIPI_DSI_D2P
#define PIN_MIPI_DSI_D2P                 255
#endif
#ifndef PIN_MIPI_DSI_D3N
#define PIN_MIPI_DSI_D3N                 255
#endif
#ifndef PIN_MIPI_DSI_D3P
#define PIN_MIPI_DSI_D3P                 255
#endif
#ifndef PIN_MIPI_DSI_CLKN
#define PIN_MIPI_DSI_CLKN                255
#endif
#ifndef PIN_MIPI_DSI_CLKP
#define PIN_MIPI_DSI_CLKP                255
#endif



/*---------------------------------------*/
/*---------TIMER PIN DEFINITION----------*/
/*---------------------------------------*/

/* NORMAL TIMER0 */
#ifndef PIN_PWM_CHANNEL_0
#define PIN_PWM_CHANNEL_0 255
#endif

#ifndef PIN_CAPTURE_CHANNEL_0
#define PIN_CAPTURE_CHANNEL_0 255
#endif

/* NORMAL TIMER1 */
#ifndef PIN_PWM_CHANNEL_1
#define PIN_PWM_CHANNEL_1 255
#endif

#ifndef PIN_CAPTURE_CHANNEL_1
#define PIN_CAPTURE_CHANNEL_1 255
#endif

/* LED_TIMER0 */
#ifndef PIN_PWM_CHANNEL_2
#define PIN_PWM_CHANNEL_2 255
#endif

/* SUPTMR0 */
#ifndef PIN_PWM_CHANNEL_3
#define PIN_PWM_CHANNEL_3 255
#endif

#ifndef PIN_CSI0_PDN
#define PIN_CSI0_PDN 255
#endif

#ifndef PIN_CSI0_RESET
#define PIN_CSI0_RESET 255
#endif

#ifndef PIN_CSI1_PDN
#define PIN_CSI1_PDN 255
#endif

#ifndef PIN_CSI1_RESET
#define PIN_CSI1_RESET 255
#endif

#ifndef PIN_LCD_RESET
#define PIN_LCD_RESET 255
#endif

#ifndef PIN_IRCUT_IN1
#define PIN_IRCUT_IN1 255
#endif

#ifndef PIN_IRCUT_IN2
#define PIN_IRCUT_IN2 255
#endif

#ifndef PIN_IRCUT_DETECT
#define PIN_IRCUT_DETECT 255
#endif

#ifndef PIN_IRCUT_LED
#define PIN_IRCUT_LED    255
#endif

#ifndef PIN_WHITE_LED
#define PIN_WHITE_LED    255
#endif

#ifndef PIN_MIPI0_IIC_CLK
#define PIN_MIPI0_IIC_CLK 255
#endif


#ifndef PIN_MIPI0_IIC_SDA
#define PIN_MIPI0_IIC_SDA 255
#endif

#ifndef PIN_MIPI1_IIC_CLK
#define PIN_MIPI1_IIC_CLK 255
#endif


#ifndef PIN_MIPI1_IIC_SDA
#define PIN_MIPI1_IIC_SDA 255
#endif

#ifndef PIN_ADKEY1
#define PIN_ADKEY1          255
#endif

#ifndef PIN_DVP0_IIC_SDA
#define PIN_DVP0_IIC_SDA    255
#endif

#ifndef PIN_DVP0_IIC_CLK
#define PIN_DVP0_IIC_CLK    255
#endif

/*---------------------------------------*/
/*---------LMAC FEM PIN DEFINITION-----------*/
/*---------------------------------------*/

/* LMAC */
#ifndef PIN_LMAC_FEM_RF_TX_EN
#define PIN_LMAC_FEM_RF_TX_EN  PC_0
#endif

#ifndef PIN_LMAC_FEM_RF_RX_EN
#define PIN_LMAC_FEM_RF_RX_EN  PC_1
#endif

enum pin_name {

    PA_0  =  0,
    PA_1,
    PA_2,
    PA_3,
    PA_4,
    PA_5,
    PA_6,
    PA_7,
    PA_8,
    PA_9,
    PA_10,
    PA_11,
    PA_12,
    PA_13,
    PA_14,
    PA_15,
 
    PB_0,
    PB_1,
    PB_2,
    PB_3,
    PB_4,
    PB_5,
    PB_6,
    PB_7,
    PB_8,
    PB_9,
    PB_10,
    PB_11,
    PB_12,
    PB_13,
    PB_14,
    PB_15,

    PC_0,
    PC_1,
    PC_2,
    PC_3,
    PC_4,
    PC_5,
    PC_6,
    PC_7,
    PC_8,
    PC_9,
    PC_10,
    PC_11,
    PC_12,
    PC_13,
    PC_14,
    PC_15,

    PD_0,
    PD_1,
    PD_2,
    PD_3,
    PD_4,
    PD_5,
    PD_6,
    PD_7,
    PD_8,
    PD_9,
    PD_10,
    PD_11,
    PD_12,
    PD_13,
    PD_14,
    PD_15,

    PE_0,
    PE_1,
    PE_2,
    PE_3,
    PE_4,
    PE_5,
    PE_6,
    PE_7,
    PE_8,
    PE_9,
    PE_10,
    PE_11,
    PE_12,
    PE_13,
    PE_14,
    PE_15,    
};

#ifdef PIN_FROM_PARAM
extern uint8_t get_sys_param_pin(uint32_t pin_enum);
#define MACRO_PIN_DEFINE(pin_macro) PARAM_##pin_macro
#define MACRO_PIN(pin_macro) get_sys_param_pin(PARAM_##pin_macro)
#define NOT_MACRO_PIN(pin_macro) get_sys_param_pin(pin_macro)
#define IOCFG_SIZE  (1024)
extern const uint16_t  iocfg_psram[IOCFG_SIZE / 2];
#define IOCFG_PARAM_ADDR ((uint32_t)iocfg_psram)

#else
#define MACRO_PIN(pin) pin
#define IOCFG_PARAM_ADDR 0
#endif


#ifdef __cplusplus
}
#endif
#endif
