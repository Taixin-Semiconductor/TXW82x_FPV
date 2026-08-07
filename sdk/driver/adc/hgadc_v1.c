/**
 * @file hgadc_v1.c
 * @author bxd
 * @brief AD_KEY
 * @version 
 * TXW82X
 * @date 2025-04-16
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "hal/adc.h"
#include "dev/adc/hgadc_v1.h"
#include "hgadc_v1_hw.h"


#define REG_CONFIG(ADDR,BIT_NUM,POST_NUM,DATA)      do{ADDR = (ADDR & ~(((0x1 << BIT_NUM) - 1) << POST_NUM)) | (DATA << POST_NUM);}while(0)

#define MODULE_DEBUG(fmt, args...)                   _os_printf(fmt, ##args)
#define MODULE_NAME                                 "adc"
#define MODULE_INFO(fmt, ...)                       \
                                                    do{                                                 \
                                                        os_printf(MODULE_NAME);                         \
                                                        _os_printf(KERN_INFO" info:"fmt, ##__VA_ARGS__);\
                                                    }while(0);
#define MODULE_ERR(fmt, ...)                         \
                                                    do{                                                 \
                                                        os_printf(MODULE_NAME);                         \
                                                        _os_printf(KERN_ERR" err:"fmt,##__VA_ARGS__);   \
                                                        os_printf("Func:%s Line:%d LR=0x%x\r\n",        \
                                                        __func__,__LINE__, (uint32)RETURN_ADDR());      \
                                                    }while(0);
#define MODULE_WARNING(fmt, ...)                     \
                                                    do{                                                 \
                                                        os_printf(MODULE_NAME);                         \
                                                        _os_printf(KERN_WARNING" warning:"fmt,##__VA_ARGS__);\
                                                    }while(0);
#define MODULE_ASSERT(expr, info)                    \
                                                    do {                                                \
                                                        if (expr) {                                     \
                                                            AUSYS_ERR(info);                            \
                                                            return RET_ERR;                             \
                                                        }                                               \
                                                    } while(0);
#define MODULE_PRINT_ARRAY(buf, len)                do{\
                                                        uint8 *p_print = (uint8 *)buf;\
                                                        _os_printf("\r\n");\
                                                        for (uint32 __ii=1; __ii<=len; __ii++) {\
                                                            _os_printf("%02x ", p_print[__ii-1]);\
                                                            if (((__ii)%16 == 0)) _os_printf("\r\n");\
                                                        }\
                                                        _os_printf("\r\n");\
                                                    }while(0);



//单次使用的优先级通道
#define SINGLE_PRI_CHANNEL              (0)



#define ADC_CHANNEL_ENABLE              (1)
#define ADC_CHANNEL_SUSPEND             (2)
#define ADC_CHANNEL_DISABLE             (3)

/* ADC channel type, must start at 0 & unique value & less than 32*/
/* Table */

// I/O class
#define _ADC_CHANNEL_IO_CLASS           (0)

//RF sensor of temperature
#define _ADC_CHANNEL_RF_TEMPERATURE     (1)
#define _ADC_CHANNEL_RF_VDDI            (2)

//Internal voltage
#define _ADC_CHANNEL_PLL_VREF           (3)
#define _ADC_CHANNEL_RF_VTUNE           (4)
#define _ADC_CHANNEL_RF_VCO_VDD         (5)
#define _ADC_CHANNEL_RF_VDD_DIV         (6)

//PMU sensor of temperature
#define _ADC_CHANNEL_PMU_TEMPERATURE    (7)
#define _ADC_CHANNEL_CORE_VDD           (8)

#define RFATO                           (0xfffffff0)
#define ANAATO                          (0xfffffff1)


extern void delay_us(uint32 n);

/**********************************************************************************/
/*                           LOW LAYER FUNCTION                                   */
/**********************************************************************************/

/* List opreation */
static int32 hgadc_v1_list_insert(adc_channel_node *head_node, adc_channel_node *new_node) {

    adc_channel_node *temp_node = head_node;

    /* find the last node */
    while (temp_node->next) {
        temp_node = temp_node->next;
    }

    temp_node->next = new_node;
    new_node->next  = NULL;

    /* channel amount */
    head_node->channel_amount++;

    MODULE_INFO("add success: ADC channel cnt = %d, name:%d\n\r", head_node->channel_amount, new_node->data.channel);

    return RET_OK;
}

static int32 hgadc_v1_list_delete(adc_channel_node *head_node, uint32 channel) {

    adc_channel_node *temp_node   = head_node;
    adc_channel_node *delete_node = NULL;

    /* find the node */
    while (temp_node->next) {
        if (channel == temp_node->next->data.channel) {

            delete_node = temp_node->next;
            temp_node->next = temp_node->next->next;
            os_free(delete_node);

            head_node->channel_amount--;

            MODULE_INFO("delete success: ADC channel cnt = %d\n\r", head_node->channel_amount);
            
            break;
        }
        temp_node = temp_node->next;
    }

    return RET_OK;
}

static int32 hgadc_v1_list_get_by_channel(adc_channel_node *head_node, uint32 channel, adc_channel_node **get_node) {

    adc_channel_node *temp_node = head_node;

    /* find the node */
    while (temp_node->next) {
        if (channel == temp_node->next->data.channel) {
            
            *get_node = temp_node->next;
        
            return RET_OK;
        }
        temp_node = temp_node->next;
    }

    return RET_ERR;
}

static int32 hgadc_v1_list_get_by_index(adc_channel_node *head_node, uint32 index, adc_channel_node **get_node) {

    adc_channel_node *temp_node = head_node;
    uint32 i = 0;

    /* find the node */
    for (i = 0; i < head_node->channel_amount; i++) {
        if ((i == index) && (temp_node->next)) {
            *get_node = temp_node->next;
            return RET_OK;
        }

        temp_node = temp_node->next;
    }

    return RET_ERR;

}

static int32 hgadc_v1_list_check_repetition(adc_channel_node *head_node, uint32 channel) {

    adc_channel_node *temp_node = head_node;
    uint32 i = 0;

    /* find the node which repeated */
    for (i = 0; i < head_node->channel_amount; i++) {
        if (temp_node->next) {
            if (channel == temp_node->next->data.channel) {
                return RET_ERR;
            }

        } 
        temp_node = temp_node->next;
    }

    return RET_OK;
}

static int32 hgadc_v1_list_delete_all(adc_channel_node *head_node, struct hgadc_v1 *dev) {

    adc_channel_node *temp_node = head_node;
    adc_channel_node *delete_node = NULL;

    while (temp_node->next) {
        delete_node = temp_node->next;
        /* disable adc channel */
        delete_node->data.func(dev, delete_node->data.channel, ADC_CHANNEL_DISABLE);
        temp_node->next = temp_node->next->next;
        os_free(delete_node);
        head_node->channel_amount--;

        MODULE_INFO("delete success: ADC channel cnt = %d\n\r", head_node->channel_amount);
    }

    return RET_OK;
}


static int32 hgadc_v1_list_get_channel_amount(adc_channel_node *head_node) {

    return head_node->channel_amount;
}


static int32 hgadc_v1_switch_hal_adc_ioctl_cmd(enum adc_ioctl_cmd param) {

    switch (param) {
        default:
            return -1;
            break;
    }
}

static int32 hgadc_v1_switch_hal_adc_irq_flag(enum adc_irq_flag param) {

    switch (param) {
        case (ADC_IRQ_FLAG_SAMPLE_DONE):
            return 0;
            break;
        default:
            return -1;
            break;
    }
}


static int32 hgadc_v1_switch_param_channel(uint32 channel) {

    /* I/O class under 0x101*/
    if (channel < 0x101) {
        return _ADC_CHANNEL_IO_CLASS;
    }

    switch (channel) {
        case ADC_CHANNEL_RF_TEMPERATURE:
            return _ADC_CHANNEL_RF_TEMPERATURE;
            break;
        case ADC_CHANNEL_VTUNE:
            return _ADC_CHANNEL_RF_VTUNE;
            break;
        case ADC_CHANNEL_VCO_VDD:
            return _ADC_CHANNEL_RF_VCO_VDD;
            break;
        case ADC_CHANNEL_VDD_DIV:
            return _ADC_CHANNEL_RF_VDD_DIV;
            break;
        //case ADC_CHANNEL_VDDI:
        //    return _ADC_CHANNEL_RF_VDDI;
        //    break;
        case ADC_CHANNEL_PMU_TEMPERATURE:
            return _ADC_CHANNEL_PMU_TEMPERATURE;
            break;
        case ADC_CHANNEL_CORE_VDD:
            return _ADC_CHANNEL_CORE_VDD;
            break;
        default :
            return RET_ERR;
            break;
    }
}

static void hgadc_v1_chn_to_sel(uint32 channel, uint32 *SEL, uint32 *SCH_VDD)
{
    switch(channel) {
        case(PA_6 ): *SEL=0x00; *SCH_VDD=0x04; break;
        case(PA_11): *SEL=0x01; *SCH_VDD=0x04; break;
        case(PA_12): *SEL=0x02; *SCH_VDD=0x04; break;
        case(PA_13): *SEL=0x03; *SCH_VDD=0x04; break;
        case(PA_14): *SEL=0x04; *SCH_VDD=0x04; break;
        case(PA_7 ): *SEL=0x05; *SCH_VDD=0x08; break;
        case(PA_8 ): *SEL=0x06; *SCH_VDD=0x08; break;
        case(PA_15): *SEL=0x07; *SCH_VDD=0x08; break;
        case(PA_0 ): *SEL=0x08; *SCH_VDD=0x10; break;
        case(PA_1 ): *SEL=0x09; *SCH_VDD=0x10; break;
        case(PA_2 ): *SEL=0x0a; *SCH_VDD=0x10; break;
        case(PA_3 ): *SEL=0x0b; *SCH_VDD=0x10; break;
        case(PA_4 ): *SEL=0x0c; *SCH_VDD=0x10; break;
        case(PA_5 ): *SEL=0x0d; *SCH_VDD=0x10; break;
        case(PD_0 ): *SEL=0x0e; *SCH_VDD=0x10; break;
        case(PD_1 ): *SEL=0x0f; *SCH_VDD=0x10; break;
        case(PD_2 ): *SEL=0x10; *SCH_VDD=0x10; break;
        case(PD_3 ): *SEL=0x11; *SCH_VDD=0x10; break;
        case(PD_4 ): *SEL=0x12; *SCH_VDD=0x10; break;
        case(PD_5 ): *SEL=0x13; *SCH_VDD=0x10; break;
        case(PD_6 ): *SEL=0x14; *SCH_VDD=0x10; break;
        case(PD_7 ): *SEL=0x15; *SCH_VDD=0x10; break;
        case(PD_8 ): *SEL=0x16; *SCH_VDD=0x10; break;
        case(PD_9 ): *SEL=0x17; *SCH_VDD=0x10; break;
        case(PD_10): *SEL=0x18; *SCH_VDD=0x10; break;
        case(PD_11): *SEL=0x19; *SCH_VDD=0x10; break;
        case(PD_12): *SEL=0x1a; *SCH_VDD=0x10; break;
        case(PD_13): *SEL=0x1b; *SCH_VDD=0x10; break;
        case(PC_8 ): *SEL=0x1c; *SCH_VDD=0x20; break;
        case(PC_9 ): *SEL=0x1d; *SCH_VDD=0x20; break;
        case(PC_10): *SEL=0x1e; *SCH_VDD=0x20; break;
        case(PC_11): *SEL=0x1f; *SCH_VDD=0x20; break;
        case(PC_12): *SEL=0x20; *SCH_VDD=0x20; break;
        case(PC_13): *SEL=0x21; *SCH_VDD=0x20; break;
        case(PC_14): *SEL=0x22; *SCH_VDD=0x40; break;
        case(PC_15): *SEL=0x23; *SCH_VDD=0x40; break;
        case(PD_14): *SEL=0x24; *SCH_VDD=0x40; break;
        case(PD_15): *SEL=0x25; *SCH_VDD=0x40; break;
        case(PE_0 ): *SEL=0x26; *SCH_VDD=0x40; break;
        case(PE_1 ): *SEL=0x27; *SCH_VDD=0x40; break;
        case(PE_2 ): *SEL=0x28; *SCH_VDD=0x40; break;
        case(PE_3 ): *SEL=0x29; *SCH_VDD=0x40; break;
        case(PB_6 ): *SEL=0x2a; *SCH_VDD=0x80; break;
        case(PB_7 ): *SEL=0x2b; *SCH_VDD=0x80; break;
        case(PB_8 ): *SEL=0x2c; *SCH_VDD=0x80; break;
        case(PB_9 ): *SEL=0x2d; *SCH_VDD=0x80; break;
        case(PB_10): *SEL=0x2e; *SCH_VDD=0x80; break;
        case(PB_11): *SEL=0x2f; *SCH_VDD=0x80; break;
        case(PB_12): *SEL=0x30; *SCH_VDD=0x80; break;
        case(PB_13): *SEL=0x31; *SCH_VDD=0x80; break;
        case(PB_14): *SEL=0x32; *SCH_VDD=0x80; break;
        case(PB_15): *SEL=0x33; *SCH_VDD=0x80; break;
        case(PC_0 ): *SEL=0x34; *SCH_VDD=0x80; break;
        case(PC_1 ): *SEL=0x35; *SCH_VDD=0x80; break;
        case(PC_2 ): *SEL=0x36; *SCH_VDD=0x80; break;
        case(PC_3 ): *SEL=0x37; *SCH_VDD=0x80; break;
        case(PC_4 ): *SEL=0x38; *SCH_VDD=0x80; break;
        case(PC_5 ): *SEL=0x39; *SCH_VDD=0x80; break;
        case(RFATO): *SEL=0x3a; *SCH_VDD=0x01; break;
        case(ANAATO):*SEL=0x3b; *SCH_VDD=0x02; break;
        default:     *SEL=0xff; *SCH_VDD=0xff; break;
    }

}


/************模拟配置***************/
void saradc_ana_config(struct hgadc_v1_hw *hw, uint32 enable)
{
    if (enable) {
        // 内部时钟电源使能
        REG_CONFIG(hw->ADC_ANA_CTRL1, 1,16, 1); // BIAS_EN--------ADC_ANA_CTRL1[16]
        REG_CONFIG(hw->ADC_CON_EN   , 1,27, 1); // SCLK_EN--------ADC_CON_EN[27]
    } else {
        // 内部时钟电源失能
        REG_CONFIG(hw->ADC_CON_EN   , 1,27, 0); // SCLK_EN--------ADC_CON_EN[27]
        REG_CONFIG(hw->ADC_ANA_CTRL1, 1,16, 0); // BIAS_EN--------ADC_ANA_CTRL1[16]
    }

}

/************采样率配置*************/
void saradc_smp_cofig(struct hgadc_v1_hw *hw, uint8 adc_baud,uint8 conv_cyc,uint16 slow_cyc,uint16 fast_cyc)
{
    /*******************ADC采样率计算*******************/
    //时钟频率  ：ADC_CLK=240M/(ADC_BAUD+1)                            ----时钟最高不能超过16MHz
    //波特率	：ADC_BAUD	  ≥14                                     ----保证ADC时钟频率不高于16MHz
    //转换时钟数：ADC_CONV_CYC≥11                                       ----转换时钟数不小于12个ADC时钟
    //采样周期数：SMP_CYC	  ≥3                                          ----采样时钟数不小于4个ADC时钟
    //采样率    ：ADC_SAMPLE=ADC_CLK/((ADC_CONV_CYC+1)+(SMP_CYC+1))   ----采样率最高不能超过1M
    REG_CONFIG(hw->ADC_CON_FUN, 8, 0,adc_baud); // ADC_BAUD-------ADC_CON_FUN[7:0]
    REG_CONFIG(hw->ADC_CON_FUN, 8, 8,conv_cyc); // ADC_CONV_CYC---ADC_CON_FUN[15:8]
    REG_CONFIG(hw->ADC_CON_SMP,10, 0,slow_cyc); // SLOW_SMP_CYC---ADC_CON_SMP[9:0]  慢速采样率配置
    REG_CONFIG(hw->ADC_CON_SMP,10,11,fast_cyc); // FAST_SMP_CYC---ADC_CON_SMP[20:11]快速采样率配置
}

/************非优先级通道配置*******/
void saradc_channel_config(struct hgadc_v1_hw *hw, uint8 chanx_en,uint8 smp_mode_sel,uint8 chan_sel,uint8 trg_sel)
{
    REG_CONFIG(hw->ADC_CON_EN, 1,(2+chanx_en), 1);                   // ADC_CON_EN[7:2]

    //非优先级通道采样率选择：快速、慢速选择。优先级通道无法选择，默认为慢速的采样率。
    REG_CONFIG(hw->ADC_CON_SMP, 1,(26+chanx_en),smp_mode_sel);       // SAMP_MOD_SEL---ADC_CON_SMP[31:26]

    if(chanx_en<4){
        REG_CONFIG(hw->ADC_CON_SEL0, 7,(8*chanx_en), chan_sel);      // CHAN_SEL--ADC_CON_SEL0[31:0]
    }else{
        REG_CONFIG(hw->ADC_CON_SEL1, 7,(8*(chanx_en-4)), chan_sel); // CHAN_SEL--ADC_CON_SEL1[15:0]
    }
    REG_CONFIG(hw->ADC_CON_EXTEND0 , 4, 6,trg_sel);                  // TRG_SEL---ADC_CON_EXTEND0[9:6]
}

/************优先级通道失能*********/
void saradc_channel_disable(struct hgadc_v1_hw *hw, uint8 chanx_en)
{
    REG_CONFIG(hw->ADC_CON_EN, 1,(2+chanx_en), 0);                   // ADC_CON_EN[7:2]

    if(chanx_en<4){
        REG_CONFIG(hw->ADC_CON_SEL0, 7,(8*chanx_en), 0x7F);      // CHAN_SEL--ADC_CON_SEL0[31:0]
    }else{
        REG_CONFIG(hw->ADC_CON_SEL1, 7,(8*(chanx_en-4)), 0x7F); // CHAN_SEL--ADC_CON_SEL1[15:0]
    }
}


/************优先级通道配置*********/
void saradc_pri_channel_config(struct hgadc_v1_hw *hw, uint8 pri_chanx_en,uint8 pri_chan_sel,uint8 pri_trg_sel)
{
    REG_CONFIG(hw->ADC_CON_EXTEND0, 1,(16+pri_chanx_en), 1);             // HIGHSET_PRI--ADC_CON_EXTEND0[17:16]
    REG_CONFIG(hw->ADC_CON_SEL1   , 7,(24-(8*pri_chanx_en)), pri_chan_sel);// PRI_CHAN0_SEL--ADC_CON_SEL1[30:24]
    REG_CONFIG(hw->ADC_CON_EXTEND0, 4, 10, pri_trg_sel);                 // PRI_TRG_SEL---ADC_CON_EXTEND0[13:10]
}

/************优先级通道失能*********/
void saradc_pri_channel_disable(struct hgadc_v1_hw *hw, uint8 pri_chanx_en)
{
    REG_CONFIG(hw->ADC_CON_EXTEND0, 1,(16+pri_chanx_en), 0);        // HIGHSET_PRI--ADC_CON_EXTEND0[17:16]
    REG_CONFIG(hw->ADC_CON_SEL1   , 7,(24-(8*pri_chanx_en)), 0x7F);// PRI_CHAN0_SEL--ADC_CON_SEL1[30:24]
}


/************数字配置**************/
//多点连续采样配置
// 9bit、8bit功能不建议使用，默认关闭
// 关闭通道码输出，16bit存储格式：3bit通道码 +3bit 0 +10bit ADC数据
// ADC数字电路使能
void saradc_dig_config(struct hgadc_v1_hw *hw, uint16 chan_scan_num)
{
    REG_CONFIG(hw->ADC_CON_FUN,16,16,chan_scan_num);// CHAN_SCANS_NUM--ADC_CON_FUN[31:16]

    REG_CONFIG(hw->ADC_CON_EN,1,26,0);           // FORMAT_9BIT---ADC_CON_EN[26]
    REG_CONFIG(hw->ADC_CON_EN,1,25,0);           // FORMAT_8BIT---ADC_CON_EN[25]

    REG_CONFIG(hw->ADC_CON_EXTEND0, 1,15, 1);    // CHAN_CODE_BYPASS--ADC_CON_EXTEND0[15]
    REG_CONFIG(hw->ADC_CON_EXTEND0, 1,14, 1);    // DAT_FORMAT_SEL--ADC_CON_EXTEND0[14]

    REG_CONFIG(hw->ADC_CON_EN, 1, 0, 1);         // ADC_EN--ADC_CON_EN[0]
}

/**********非优先级通道单次采样触发***********/
//这个是单独某个通道，单次采样触发
//如果是多个非优先级通道同时采样，pending需要改为BIT(6)
static uint16 g_chan_data[3] = {0xA1A1, 0x0, 0xA5A5};
uint16 saradc_one_channel_single_kick(struct hgadc_v1_hw *hw)
{
//    uint16 chanx_data = 0;

    if ((0xA1A1 != g_chan_data[0]) || (0xA5A5 != g_chan_data[2])) {
        while(1) {
            MODULE_ERR("sram flag was changed!\r\n");
       };
    }

    //写一下DMA地址寄存器，用于复位ADKEY的DMA CNT
    hw->ADC_DMA_STADR = (uint32)&g_chan_data[1];

    //REG_CONFIG(hw->ADC_CON_KICK,1,0,1);						// ADC_KICK--ADC_CON_KICK[0]
    hw->ADC_CON_KICK = BIT(0);
//    while(!(hw->ADC_CON_STA & BIT(chan_en)));						
//    chanx_data = (hw->ADC_ANA_CTRL0 & (0xffff << 4)) >> 4;	// ADC_DATA----ADC_ANA_CTRL0[19:4]
//    //REG_CONFIG(hw->ADC_CON_STA,1,chan_en,1);
//    hw->ADC_CON_STA = BIT(chan_en);

    if ((0xA1A1 != g_chan_data[0]) || (0xA5A5 != g_chan_data[2])) {
         while(1) {
             MODULE_ERR("sram flag was changed!\r\n");
        };
    }


    return 0;
}

/**********非优先级通道获取pending***********/
uint16 saradc_channel_get_pending(struct hgadc_v1_hw *hw, uint8 chan_en)
{
    if ((hw->ADC_CON_STA & BIT(chan_en))) {
        return 1;
    } else {
        return 0;
    }
}

/**********非优先级通道清除pending***********/
void saradc_channel_clr_pending(struct hgadc_v1_hw *hw, uint8 chan_en)
{
    hw->ADC_CON_STA = BIT(chan_en);
}

/**********非优先级通道获取采样值***********/
uint16 saradc_channel_get_raw_data(struct hgadc_v1_hw *hw, uint8 chan_en)
{
    uint16 pri_chanx_data = 0;

    pri_chanx_data = (hw->ADC_ANA_CTRL0 & (0xffff << 4)) >> 4;	// ADC_DATA----ADC_ANA_CTRL0[19:4]
    return pri_chanx_data;
}


/**********优先级通道单次采样触发***********/
void saradc_pri_channel_kick(struct hgadc_v1_hw *hw)
{
    REG_CONFIG(hw->ADC_CON_KICK,1,1,1); // ADC_PRI_KICK--ADC_CON_KICK[1]
}

/**********优先级通道获取pending***********/
uint16 saradc_pri_channel_get_pending(struct hgadc_v1_hw *hw, uint8 pri_chan_sel)
{
    if ((hw->ADC_CON_STA & BIT(25-pri_chan_sel))) {
        return 1;
    } else {
        return 0;
    }
}

/**********优先级通道清除pending***********/
void saradc_pri_channel_clr_pending(struct hgadc_v1_hw *hw, uint8 pri_chan_sel)
{
    //REG_CONFIG(hw->ADC_CON_STA,1,(25-pri_chan_sel),1);
    hw->ADC_CON_STA = (1<<(25-pri_chan_sel));
}

/**********优先级通道获取采样值***********/
uint16 saradc_pri_channel_get_raw_data(struct hgadc_v1_hw *hw, uint8 pri_chan_sel)
{
    uint16 pri_chanx_data = 0;

    pri_chanx_data = (hw->ADC_CON_EXTEND1 & (0xffff<<(16-16*pri_chan_sel))) >> (16-16*pri_chan_sel);    // PRI_CHANx_DATA--ADC_CON_EXTEND1[31:0]
    return pri_chanx_data;
}

/**********优先级通道配置采样完成中断***********/
void saradc_pri_channel_config_done_irq(struct hgadc_v1_hw *hw, uint8 pri_chan_sel, uint8 en)
{
    if (en) {
        REG_CONFIG(hw->ADC_CON_EXTEND0,1,(0+pri_chan_sel),1);
    } else {
        REG_CONFIG(hw->ADC_CON_EXTEND0,1,(0+pri_chan_sel),0);
    }
}

/**********优先级通道获取采样完成中断使能位***********/
uint16 saradc_pri_channel_get_done_irq_en(struct hgadc_v1_hw *hw, uint8 pri_chan_sel)
{
    if (hw->ADC_CON_EXTEND0 & BIT(0+pri_chan_sel)) {
        return 1;
    } else {
        return 0;
    }
}

static int32 hgadc_v1_adc_channel_txw82x_io_class(struct hgadc_v1 *dev, uint32 channel, uint32 enable)
{
    struct hgadc_v1_hw *hw  = (struct hgadc_v1_hw *)dev->hw;
    adc_channel_node   *cur_node = dev->cur_node;
    
    if (!cur_node) {
        MODULE_ERR("cur_node pointer is NULL\r\n");
        return RET_ERR;
    }

    if (ADC_CHANNEL_ENABLE == enable) {
        //配置优先级通道，软件触发
        saradc_pri_channel_config(hw, SINGLE_PRI_CHANNEL, cur_node->data.sel, 0);
    } else if (ADC_CHANNEL_SUSPEND == enable) {
        //失能优先级通道
        saradc_pri_channel_disable(hw, SINGLE_PRI_CHANNEL);
    } else if (ADC_CHANNEL_DISABLE == enable) {
        //失能优先级通道
        saradc_pri_channel_disable(hw, SINGLE_PRI_CHANNEL);
    }

    return RET_OK;
}

static int32 hgadc_v1_adc_channel_txw82x_rf_temperature(struct hgadc_v1 *dev, uint32 channel, uint32 enable)
{
    #define GPIOC(offset)   (*((uint32 *)(GPIOC_BASE+offset)))
    #define IO_NUM          (0)
    
    struct hgadc_v1_hw *hw  = (struct hgadc_v1_hw *)dev->hw;
    adc_channel_node *cur_node = dev->cur_node;
    uint32 mask = 0;
    uint32 i = 0;
    
    if (!cur_node) {
        MODULE_ERR("cur_node pointer is NULL\r\n");
        return RET_ERR;
    }

    /* Close the interrupt to protect the RF register opreation */
    mask = disable_irq();


    if (ADC_CHANNEL_ENABLE == enable) {

        /* connect to rf_temperature channel */
        *((uint32 *)(0x40019000 + 0x18))  = ( *((uint32 *)(0x40019000 + 0x18)) & ~(0xf<<27) ) | 0x8<<27;
        
        #if 0
        *((uint32 *)(0x40019000 + 0x1C)) |= 0x1;
        
        //IO MODE = analog
        GPIOC(0x00) |=  (3 << (IO_NUM *2));
        
        //IO MODE = analog
        GPIOC(0x50) |=  (1 << (IO_NUM *1));
        
        /*!
         * only PC0-PC5 by hw->ADKEY_CON.BIT8
         * **/
        //RF_TOUT & IO_OUT
        //hw->ADKEY_CON = ( hw->ADKEY_CON &~ (0x7F << 4) ) | ((0x1<<10) | (0x1<<9));//PC8 
        #endif

        //配置优先级通道，软件触发
        saradc_pri_channel_config(hw, SINGLE_PRI_CHANNEL, cur_node->data.sel, 0);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_SUSPEND == enable) {

        /*!
         * 关闭通路
         */
        *((uint32 *)(0x40019000 + 0x18))  &= ~(0xf<<27);

        //失能优先级通道
        saradc_pri_channel_disable(hw, SINGLE_PRI_CHANNEL);

    } else if (ADC_CHANNEL_DISABLE == enable) {

        /*!
         * 关闭通路
         */
        *((uint32 *)(0x40019000 + 0x18))  &= ~(0xf<<27);

        //失能优先级通道
        saradc_pri_channel_disable(hw, SINGLE_PRI_CHANNEL);
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;

}

static int32 hgadc_v1_adc_channel_txw82x_pmu_temperature(struct hgadc_v1 *dev, uint32 channel, uint32 enable)
{
    struct hgadc_v1_hw *hw  = (struct hgadc_v1_hw *)dev->hw;
    adc_channel_node *cur_node = dev->cur_node;
    uint32 mask = 0;
    uint32 i = 0;
    
    if (!cur_node) {
        MODULE_ERR("cur_node pointer is NULL\r\n");
        return RET_ERR;
    }

    /* Close the interrupt to protect the RF register opreation */
    mask = disable_irq();


    if (ADC_CHANNEL_ENABLE == enable) {

        /* connect to rf_temperature channel */
        //bit13为温度传感器使能位，bit14为温度传感器采样通道使能
        pmu_reg_write((uint32)&PMU->PMUCON6, (PMU->PMUCON6 | (0x1 <<13) | (0x1 <<14)));

        //配置优先级通道，软件触发
        saradc_pri_channel_config(hw, SINGLE_PRI_CHANNEL, cur_node->data.sel, 0);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_SUSPEND == enable) {

        /*!
         * 关闭通路
         */
        //bit13为温度传感器使能位，bit14为温度传感器采样通道使能
        pmu_reg_write((uint32)&PMU->PMUCON6, (PMU->PMUCON6 &~ ((0x1 <<13) | (0x1 <<14))));

        //失能优先级通道
        saradc_pri_channel_disable(hw, SINGLE_PRI_CHANNEL);

    } else if (ADC_CHANNEL_DISABLE == enable) {

        /*!
         * 关闭通路
         */
        //bit13为温度传感器使能位，bit14为温度传感器采样通道使能
        pmu_reg_write((uint32)&PMU->PMUCON6, (PMU->PMUCON6 &~ ((0x1 <<13) | (0x1 <<14))));

        //失能优先级通道
        saradc_pri_channel_disable(hw, SINGLE_PRI_CHANNEL);
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;


}

static int32 hgadc_v1_adc_channel_txw82x_core_vdd(struct hgadc_v1 *dev, uint32 channel, uint32 enable)
{
    struct hgadc_v1_hw *hw  = (struct hgadc_v1_hw *)dev->hw;
    adc_channel_node *cur_node = dev->cur_node;
    uint32 mask = 0;
    uint32 i = 0;
    
    if (!cur_node) {
        MODULE_ERR("cur_node pointer is NULL\r\n");
        return RET_ERR;
    }

    /* Close the interrupt to protect the RF register opreation */
    mask = disable_irq();


    if (ADC_CHANNEL_ENABLE == enable) {
		pmu_reg_write((uint32)&PMU->PMUCON6, (PMU->PMUCON6&~(7<<6))|(2<<6));
        //配置优先级通道，软件触发
       saradc_pri_channel_config(hw, SINGLE_PRI_CHANNEL, cur_node->data.sel, 0);
        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_SUSPEND == enable) {

        /*!
         * 关闭通路
         */
        pmu_reg_write((uint32)&PMU->PMUCON6, (PMU->PMUCON6&~(7<<6))|(0<<6));

        //失能优先级通道
        saradc_pri_channel_disable(hw, SINGLE_PRI_CHANNEL);

    } else if (ADC_CHANNEL_DISABLE == enable) {

        /*!
         * 关闭通路
         */
        pmu_reg_write((uint32)&PMU->PMUCON6, (PMU->PMUCON6&~(7<<6))|(0<<6));

        //失能优先级通道
        saradc_pri_channel_disable(hw, SINGLE_PRI_CHANNEL);
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;
}


#if 0
static int32 hgadc_v1_adc_channel_txw82x_rf_vtune(struct hgadc_v1 *dev, uint32 channel, uint32 enable)
{
    struct hgadc_v1_hw *hw  = (struct hgadc_v1_hw *)dev->hw;
    uint32 mask = 0;
    uint32 i = 0;

    /* Close the interrupt to protect the RF register opreation */
    mask = disable_irq();

    if (ADC_CHANNEL_ENABLE == enable) {
        
        //LO的模拟测试使能信号开启  select vtune to RF_TOUT (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7)) | 0x1<<7 | 0x4<<8;

        //AUXPEN选择  RF_TOUT放到ADKEY0
        hw->ADKEY_CON = (hw->ADKEY_CON &~ (0xF << 8) ) | (0x4 << 8);

        /* Wait stable */
        for (i = 0; i < 50; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_SUSPEND == enable) {

        //关闭测试通路，清0
        //LO的模拟测试使能信号关闭 (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7));
        
        hw->ADKEY_CON &= ~(0xF << 8);
        /* clear the data */
        hw->ADKEY_DATA &= ~(0xFFF);

        /* Wait stable */
        for (i = 0; i < 50; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_DISABLE == enable) {

        //关闭测试通路，清0
        //LO的模拟测试使能信号关闭 (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7));

        hw->ADKEY_CON &= ~(0xF << 8);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;

}


static int32 hgadc_v1_adc_channel_txw82x_rf_vco_vdd(struct hgadc_v1 *dev, uint32 channel, uint32 enable)
{
    struct hgadc_v1_hw *hw  = (struct hgadc_v1_hw *)dev->hw;
    uint32 mask = 0;
    uint32 i = 0;

    /* Close the interrupt to protect the RF register opreation */
    mask = disable_irq();

    if (ADC_CHANNEL_ENABLE == enable) {
        
        /*ADKEY0采样vco_vdd电压*/	
        (*(uint32 *)(0x40019000)) = ((*(uint32 *)(0x40019000)) & ~(15<< 7)) | 0x1<<7 | 0x0<<8;

        //AUXPEN选择  RF_TOUT放到ADKEY0
        hw->ADKEY_CON = (hw->ADKEY_CON &~ (0xF << 8) ) | (0x4 << 8);

        /* Wait stable */
        for (i = 0; i < 50; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_SUSPEND == enable) {

        //关闭测试通路，清0
        //LO的模拟测试使能信号关闭 (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7));
        
        hw->ADKEY_CON &= ~(0xF << 8);
        /* clear the data */
        hw->ADKEY_DATA &= ~(0xFFF);

        /* Wait stable */
        for (i = 0; i < 50; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_DISABLE == enable) {

        //关闭测试通路，清0
        //LO的模拟测试使能信号关闭 (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7));

        hw->ADKEY_CON &= ~(0xF << 8);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;

}

static int32 hgadc_v1_adc_channel_txw82x_rf_vdd_div(struct hgadc_v1 *dev, uint32 channel, uint32 enable)
{
    struct hgadc_v1_hw *hw  = (struct hgadc_v1_hw *)dev->hw;
    uint32 mask = 0;
    uint32 i = 0;

    /* Close the interrupt to protect the RF register opreation */
    mask = disable_irq();

    if (ADC_CHANNEL_ENABLE == enable) {
        
        /*ADKEY0采样vco_vdd电压*/	
        (*(uint32 *)(0x40019000)) = ((*(uint32 *)(0x40019000)) & ~(15<< 7)) | 0x1<<7 | 0x2<<8;

        //AUXPEN选择  RF_TOUT放到ADKEY0
        hw->ADKEY_CON = (hw->ADKEY_CON &~ (0xF << 8) ) | (0x4 << 8);

        /* Wait stable */
        for (i = 0; i < 50; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_SUSPEND == enable) {

        //关闭测试通路，清0
        //LO的模拟测试使能信号关闭 (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7));

        
        hw->ADKEY_CON &= ~(0xF << 8);
        /* clear the data */
        hw->ADKEY_DATA &= ~(0xFFF);

        /* Wait stable */
        for (i = 0; i < 50; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_DISABLE == enable) {

        //关闭测试通路，清0
        //LO的模拟测试使能信号关闭 (RFSYS_REG0:0x40019000)
        (*(uint32 *)0x40019000) = ((*(uint32 *)0x40019000) & ~(15<< 7));


        hw->ADKEY_CON &= ~(0xF << 8);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;

}


static int32 hgadc_v1_adc_channel_txw82x_rf_vddi_config(struct hgadc_v1 *dev)
{
    #define SOFT_RFIF_CON   (*((uint32 *)0x4001d0cc))
    #define RFSYS_REG7      (*((uint32 *)0x4001901C))
    #define RFSYS_REG4      (*((uint32 *)0x40019010))
    #define RFIDLEDIS0      (*((uint32 *)0x40019054))

    struct _rf_pmu_dc_for_adc p_pmu;


    //没开VDDI，则要开VDDI
    if ((0==( (SOFT_RFIF_CON) & BIT(0) ) ) && (dev->rf_vddi_en==0) ) {
        os_printf("Open VDDI!\r\n");
        sysctrl_unlock();

        SYSCTRL->SYS_CON3 &= ~(1 << 3); //RF_POR=0 to reset RFDIG register
        SYSCTRL->SYS_CON3 |= (1 << 3);  //RF_POR=1 to wakeup RFDIG

        (SOFT_RFIF_CON) |= 0x7f<<7; //software control //这里是将RF的关键控制信号切换成软件控制
        (SOFT_RFIF_CON) |= BIT(0); //RF_EN为1

        (SOFT_RFIF_CON) |= BIT(25); // bbgclk is always generated


        if(rf_pmu_dc_efuse_read_for_adc(&p_pmu)==RET_OK) {
            if(get_chip_pack_for_adc() == 0) {   //for QFN58 RFDC config
                p_pmu.rf_vref    = 8;
                p_pmu.rf_ibpt    = 0xa;
                p_pmu.rf_ibct    = 0xa;
                p_pmu.rf_lo_vref = 0x8;
            }

            (RFSYS_REG7) &= ~((0xf<<9)|(0xf<<5)|(0xf<<1));
            (RFSYS_REG7) |= (p_pmu.rf_vref<<9)|(p_pmu.rf_ibpt<<5)|(p_pmu.rf_ibct<<1);

            (RFSYS_REG4) = 0x2a6f7c3c; //LO_VREFCP_VDD=10, LO_VREFLO_VDD=11
            (RFSYS_REG4) &= ~(0xf<<11);
            (RFSYS_REG4) |= (p_pmu.rf_lo_vref<<11);
        }
        else {
            (RFSYS_REG7) = 0x13099f10; //RF_VREF=15
        }

        (RFIDLEDIS0) = 0x02000803;
        //disable status
        (SOFT_RFIF_CON) &= ~ BIT(0); //RF_EN为0

        //enable mac & rf
        SYSCTRL->SYS_CON3 |= 1<<3 | 1<<5;

        dev->rf_vddi_en = 1;
    }

#if 0
    lo_dc_test(dev);
#endif

    return RET_OK;
}


static int32 hgadc_v1_adc_channel_txw82x_rf_vddi(struct hgadc_v1 *dev, uint32 channel, uint32 enable) {

    #define RFSYS_REG7      (*((uint32 *)0x4001901C))
    #define RFSYS_REG6      (*((uint32 *)0x40019018))


    struct hgadc_v1_hw *hw  = (struct hgadc_v1_hw *)dev->hw;
    uint32 mask = 0;
    uint32 i = 0;

    /* Close the interrupt to protect the RF register opreation */
    mask = disable_irq();


    if (ADC_CHANNEL_ENABLE == enable) {

        /*
         * 判断VDDI是否已经开启
         */
        hgadc_v1_adc_channel_txw82x_rf_vddi_config(dev);

        //os_printf("ADC module info: vddi gears: %d\r\n", ((*((uint32 *)(0x40019000 + 0x1C))) & (0xF << 9) ) >> 9);

        /* connect to rf_vddi channel */
        RFSYS_REG6  = ( RFSYS_REG6 & ~(0xf<<27) ) | 0x9<<27 | 0x1<<31;
        
        //vddi to PC0
        //RFSYS_REG7 |= 0x1;
    
        /* RF_TOUT */
        hw->ADKEY_CON = ( hw->ADKEY_CON &~ (0xF << 8) ) | (0x4 << 8);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_SUSPEND == enable) {

		/*!
	     * 关闭通路
	     */
		RFSYS_REG6  &= ~((0xf<<27) | (0x1<<31));

        hw->ADKEY_CON &= ~(0xF << 8);
        /* clear the data */
        hw->ADKEY_DATA &= ~(0xFFF);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    } else if (ADC_CHANNEL_DISABLE == enable) {

		/*!
	     * 关闭通路
	     */
		RFSYS_REG6  &= ~((0xf<<27) | (0x1<<31));

        
        hw->ADKEY_CON &= ~(0xF << 8);

        /* Wait stable */
        for (i = 0; i < 20; i++) {
            __NOP();
        }
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;
}
#endif

void hgadc_v1_txw82x_raw_data_handle(struct hgadc_v1 *dev, uint32 channel, uint32 *adc_data)
{
    #define RAW_DATA_DBG    (0)
    
    volatile int32  data_temp = 0;
    uint32 base_temp = dev->ref_temp;
      

      if (ADC_CHANNEL_RF_TEMPERATURE == channel) {


        /*!
         * formula:
         * { {vref/adkey_range} * diff } / 0.004
         */
            
        #if RAW_DATA_DBG
            os_printf("tsensor  ldo-> = %d\r\n", dev->refer_adda_vref);
            os_printf("tsensor  tsd-> = %d\r\n", dev->refer_tsensor);
            os_printf("tsensor  raw-> = %d\r\n", *adc_data);
        #endif

        //diff = tsensor - (effuse_tsensor)
        data_temp = (int32)*adc_data - (int32)(dev->refer_tsensor);
        #if RAW_DATA_DBG
            os_printf("tsensor diff-> = %d\r\n", data_temp);
        #endif

        //{(vref*1024)*diff*1000}
        data_temp = dev->refer_adda_vref * 1024 * data_temp * 1000;
        #if RAW_DATA_DBG
            os_printf("tsensor 1----> = %d\r\n", data_temp);
        #endif

        // {(vref*1024)*diff*1000} / {adkey_range*4}
        data_temp = data_temp / (dev->adkey_range * 4);
        #if RAW_DATA_DBG
            os_printf("tsensor 2----> = %d\r\n", data_temp);
        #endif

        // {(vref*1024)*diff*1000} / {adkey_range*4} / {1024}
        data_temp = data_temp >> 10;
        #if RAW_DATA_DBG
            os_printf("tsensor 3----> = %d\r\n", data_temp);
        #endif

        //室温25度 + 5度（待定）
        data_temp = data_temp + base_temp;
        #if RAW_DATA_DBG
            os_printf("tsensor 4----> = %d\r\n", data_temp);
        #endif
        
        *adc_data = data_temp;
    
    }else if (ADC_CHANNEL_PMU_TEMPERATURE == channel) {

        /*!
         * 计算公式
         * 25 + [(-1) * (delta/range) * ref * (1/3mV)]
         */
         
       #if RAW_DATA_DBG
            os_printf("pmu tsensor ldo-> = %d\r\n", dev->refer_adda_vref);
            os_printf("pmu tsensor tsd-> = %d\r\n", dev->refer_pmu_tsensor);
            os_printf("pmu tsensor raw-> = %d\r\n", *adc_data);
        #endif

        //delta = tsensor - (efuse_tsensor)
        data_temp = (int32)*adc_data - (int32)(dev->refer_pmu_tsensor);
        #if RAW_DATA_DBG
            os_printf("pmu tsensor delta-> = %d\r\n", data_temp);
        #endif

        //vref * delta * 1000
        data_temp = dev->refer_adda_vref * data_temp * 1000;
        #if RAW_DATA_DBG
            os_printf("pmu tsensor 1----> = %d\r\n", data_temp);
        #endif

        // vref * delta * 1000 * (1/3) * (1/range)
        // 3mV代表1摄氏度
        data_temp = data_temp / (dev->adkey_range * 3);
        #if RAW_DATA_DBG
            os_printf("pmu tsensor 2----> = %d\r\n", data_temp);
        #endif

        // (-1) * vref * delta * 1000 * (1/3) * (1/range)
        data_temp = data_temp * (-1);
        #if RAW_DATA_DBG
            os_printf("pmu tsensor 3----> = %d\r\n", data_temp);
        #endif

        //室温25度 + 5度（待定）
        data_temp = data_temp + base_temp;
        #if RAW_DATA_DBG
            os_printf("pmu tsensor 4----> = %d\r\n", data_temp);
        #endif
        
        *adc_data = data_temp;
    } else if ((ADC_CHANNEL_VCO_VDD == channel) || (ADC_CHANNEL_VDD_DIV == channel)) {
          //os_printf("1----> = %d\r\n", data_temp);  //A'
          //os_printf("2----> = %d\r\n", *adc_data);  //B'
    
          //返回的是：电压值*256*256
//          data_temp = ((((dev->refer_vddi) * (*adc_data)) * 256) / data_temp);
//    
//          *adc_data = data_temp;
          
          //os_printf("3----> = %drn", data_temp);
    } else if (ADC_CHANNEL_VTUNE == channel) {
    
    /*!
     * formula:
     * { {2.7*65536} * adc_vtune } / 4096
     */

        *adc_data = (((*adc_data) * (dev->refer_adda_vref) * (64))/4096);
    } else {

        /*!
         * fromulation: (adda_ref / 2.7) * raw_data
         */

//        //(adda_ref*10 / 27)
//        data_temp = ((dev->refer_adda_vref * 10) / (27));
//
//        //(adda_ref*10 / 27) * raw_data
//        data_temp = data_temp * (*adc_data);
//
//        //((adda_ref*10 / 27) * raw_data) / 1024
//        *adc_data = (data_temp) >> 10;
//
        if (*adc_data > 2047) {
            *adc_data = 2047;
        }

        //os_printf("adkey raw data:%d\n\r", *adc_data);


    }


}

void sar_adc_sample_one_channel(uint32 channel, uint32 *raw_data)
{
    struct hgadc_v1_hw *hw  = (struct hgadc_v1_hw *)ADKEY_BASE;
    uint32 saradc_gain      = sysctrl_efuse_saradc_gain();
    uint32 saradc_offset    = sysctrl_efuse_saradc_offset();
    uint32 i = 0;
    uint32 timeout = 1000;

    sysctrl_adkey_clk_open();

    saradc_ana_config(hw, 1);

    saradc_dig_config(hw, 0);

    if (saradc_gain==0 && saradc_offset==0) {
        saradc_gain   = 2048;
        saradc_offset = 0;
    }

    //gain
    REG_CONFIG(hw->ADC_ANA_CTRL1,12, 0,saradc_gain);
    //offset
    REG_CONFIG(hw->ADC_ANA_CTRL1, 9,17,saradc_offset);

    //ADC_CON_EN[1]----CALIB_EN
    REG_CONFIG(hw->ADC_CON_EN, 1,1,1);

    //配置采样率500k
    saradc_smp_cofig(hw, (15-1), (12-1), (20-1), (20-1));

    //等待ADKEY稳定
    delay_us(1);

    /*!
     * 打开通道
     */
    switch (channel) {
        case (ADC_CHANNEL_CORE_VDD):
             pmu_reg_write((uint32)&PMU->PMUCON6, (PMU->PMUCON6&~(7<<6))|(2<<6));
             //配置优先级通道，软件触发
             saradc_pri_channel_config(hw, SINGLE_PRI_CHANNEL, 0x3B, 0);
             /* Wait stable */
             for (i = 0; i < 20; i++) {
                 __NOP();
             }

            break;
    }


    /* 清除通道的pending */
    saradc_pri_channel_clr_pending(hw, SINGLE_PRI_CHANNEL);

    //kick start to sample
    saradc_pri_channel_kick(hw);

    i = 0;
    while (!saradc_pri_channel_get_pending(hw, SINGLE_PRI_CHANNEL)) {
        delay_us(1);
        i++;
        if (i>timeout) {break;}
    }

    saradc_pri_channel_clr_pending(hw, SINGLE_PRI_CHANNEL);

    *raw_data = saradc_pri_channel_get_raw_data(hw, SINGLE_PRI_CHANNEL);

    /*!
     * 关闭通道
     */
    switch (channel) {
        case (ADC_CHANNEL_CORE_VDD):
             pmu_reg_write((uint32)&PMU->PMUCON6, (PMU->PMUCON6&~(7<<6))|(0<<6));
            break;
    }
    
    //关闭ADKEY
    saradc_ana_config(hw, 0);

    sysctrl_adkey_clk_close();
}

void hgadc_v1_txw82x_open_handler(struct hgadc_v1 *dev)
{
    struct hgadc_v1_hw *hw  = (struct hgadc_v1_hw *)dev->hw;
    uint32 saradc_gain   = sysctrl_efuse_saradc_gain();
    uint32 saradc_offset = sysctrl_efuse_saradc_offset();

    dev->refer_adda_vref   = 3;
    dev->refer_tsensor     = sysctrl_efuse_tsensor_get();
    dev->refer_pmu_tsensor = sysctrl_efuse_pmu_tsensor_get();
    dev->adkey_range       = 2048;
    dev->ref_temp          = 30;

    sysctrl_adkey_clk_open();

    saradc_ana_config(hw, 1);

    saradc_dig_config(hw, 0);

    if (saradc_gain==0 && saradc_offset==0) {
        saradc_gain   = 2048;
        saradc_offset = 0;
        MODULE_INFO("calib invalid, gain=%d, offset=%d\n\r", saradc_gain, saradc_offset);
    }

    //gain
    REG_CONFIG(hw->ADC_ANA_CTRL1,12, 0,saradc_gain);
    //offset
    REG_CONFIG(hw->ADC_ANA_CTRL1, 9,17,saradc_offset);

    //ADC_CON_EN[1]----CALIB_EN
    REG_CONFIG(hw->ADC_CON_EN, 1,1,1);

    MODULE_INFO("calib valid, gain=%d, offset=%d, vptat:%d, pmu_vptat:%d\n\r", saradc_gain, saradc_offset, dev->refer_tsensor, dev->refer_pmu_tsensor);

    //配置采样率500k
    saradc_smp_cofig(hw, (15-1), (12-1), (20-1), (20-1));

    //配置优先级通道采样完成中断
    saradc_pri_channel_config_done_irq(hw, SINGLE_PRI_CHANNEL, 1);
}

void hgadc_v1_txw82x_close_handler(struct hgadc_v1 *dev)
{
    struct hgadc_v1_hw *hw  = (struct hgadc_v1_hw *)dev->hw;
    
    saradc_ana_config(hw, 0);

    sysctrl_adkey_clk_close();
}


/**********************************************************************************/
/*                             ATTCH FUNCTION                                     */
/**********************************************************************************/

static int32 hgadc_v1_open(struct adc_device *adc) {

    struct hgadc_v1    *dev = (struct hgadc_v1 *)adc;
    uint32 mask = 0;

    mask = disable_irq();

    if (dev->opened) {
        /* Enable interrupt */
        enable_irq(mask);
        return -EBUSY;
    }

    hgadc_v1_txw82x_open_handler(dev);

    irq_enable(dev->irq_num);

    /* init head node */
    dev->head_node.channel_amount = 0;
    dev->head_node.data.channel   = -1;
    dev->head_node.data.func      = NULL;
    dev->head_node.next           = NULL;

    dev->opened     = 1;
    dev->irq_en     = 0;
    dev->rf_vddi_en = 0;
    
    /* Enable interrupt */
    enable_irq(mask);

    MODULE_INFO("open success!\n\r");

    return RET_OK;
}

static int32 hgadc_v1_close(struct adc_device *adc) {

    uint32 mask = 0;
    struct hgadc_v1 *dev = (struct hgadc_v1 *)adc;

    if (!dev->opened) {
        return RET_OK;
    }

    /* Close the interrupt to protect the list opreation */
    mask = disable_irq();

    /* keep ADC open, when channel is still in use */
    if (hgadc_v1_list_get_channel_amount(&dev->head_node)) {
        
        /* Enable interrupt */
        enable_irq(mask);

        return RET_OK;
    }

    hgadc_v1_list_delete_all(&dev->head_node, dev);

    /* Enable interrupt */
    enable_irq(mask);

    irq_disable(dev->irq_num);

    hgadc_v1_txw82x_close_handler(dev);

    dev->head_node.channel_amount = 0;
    dev->head_node.next           = NULL;
    dev->head_node.data.channel   = -1;
    dev->head_node.data.func      = NULL;
    dev->refer_tsensor = 0;
    dev->refer_vddi    = 0;
    dev->irq_en        = 0;
    dev->opened        = 0;
    dev->rf_vddi_en    = 0;
    dev->refer_vddi_adc_data = 0;

    return RET_OK;
}


static int32 hgadc_v1_add_channel(struct adc_device *adc, uint32 channel) {

    int32  _class = 0;
    uint32 mask = 0;
    struct hgadc_v1 *dev = (struct hgadc_v1 *)adc;
    adc_channel_node *new_node = NULL;
    uint32 sel = 0, sch_vdd = 0;

    if (!dev->opened) {
        return RET_ERR;
    }

    /* Close the interrupt to protect the list opreation */
    mask = disable_irq();

    /* Check for the channel which repeated  */
    if (RET_ERR == hgadc_v1_list_check_repetition(&dev->head_node, channel)) {
        enable_irq(mask);
        MODULE_ERR("channel repeat\n\r");
        return RET_OK;
    }

    _class = hgadc_v1_switch_param_channel(channel);

    if (RET_ERR == _class) {
        enable_irq(mask);
        MODULE_ERR("%x channel don't support\n\r", channel);
        return RET_ERR;
    }

    /* save the adkey configuration by channel */
    switch (_class) {
        case _ADC_CHANNEL_IO_CLASS:
            hgadc_v1_chn_to_sel(channel, &sel, &sch_vdd);
            if (sel==0xFF || sch_vdd==0xFF) {MODULE_ERR("%x io don't support\n\r", channel);}
    
            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }

            new_node->data.func    = hgadc_v1_adc_channel_txw82x_io_class;
            new_node->data.channel = channel;
            new_node->data.sel     = sel;
            new_node->data.sch_vdd = sch_vdd;
            new_node->next         = NULL;
            hgadc_v1_list_insert(&dev->head_node, new_node);
            break;
        case _ADC_CHANNEL_RF_TEMPERATURE:
            hgadc_v1_chn_to_sel(RFATO, &sel, &sch_vdd);
            if (sel==0xFF || sch_vdd==0xFF) {MODULE_ERR("%x don't support\n\r", channel);}
            
            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }
            
            new_node->data.func    = hgadc_v1_adc_channel_txw82x_rf_temperature;
            new_node->data.channel = channel;
            new_node->data.sel     = sel;
            new_node->data.sch_vdd = sch_vdd;
            new_node->next         = NULL;
            hgadc_v1_list_insert(&dev->head_node, new_node);
            break;
         case _ADC_CHANNEL_RF_VTUNE:
            hgadc_v1_chn_to_sel(RFATO, &sel, &sch_vdd);
            if (sel==0xFF || sch_vdd==0xFF) {MODULE_ERR("%x don't support\n\r", channel);}

            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }
            
            new_node->data.func    = NULL;//hgadc_v1_adc_channel_txw82x_rf_vtune;
            new_node->data.channel = channel;
            new_node->data.sel     = sel;
            new_node->data.sch_vdd = sch_vdd;
            new_node->next         = NULL;
            hgadc_v1_list_insert(&dev->head_node, new_node);
            break;
         case _ADC_CHANNEL_RF_VCO_VDD:
            hgadc_v1_chn_to_sel(RFATO, &sel, &sch_vdd);
            if (sel==0xFF || sch_vdd==0xFF) {MODULE_ERR("%x don't support\n\r", channel);}

            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }
            
            new_node->data.func    = NULL;//hgadc_v1_adc_channel_txw82x_rf_vco_vdd;
            new_node->data.channel = channel;
            new_node->data.sel     = sel;
            new_node->data.sch_vdd = sch_vdd;
            new_node->next         = NULL;
            hgadc_v1_list_insert(&dev->head_node, new_node);
            break;
         case _ADC_CHANNEL_RF_VDD_DIV:
            hgadc_v1_chn_to_sel(RFATO, &sel, &sch_vdd);
            if (sel==0xFF || sch_vdd==0xFF) {MODULE_ERR("%x don't support\n\r", channel);}
            
            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }
            
            new_node->data.func    = NULL;//hgadc_v1_adc_channel_txw82x_rf_vdd_div;
            new_node->data.channel = channel;
            new_node->data.sel     = sel;
            new_node->data.sch_vdd = sch_vdd;
            new_node->next         = NULL;
            hgadc_v1_list_insert(&dev->head_node, new_node);
            break;
         case _ADC_CHANNEL_RF_VDDI:
            hgadc_v1_chn_to_sel(RFATO, &sel, &sch_vdd);
            if (sel==0xFF || sch_vdd==0xFF) {MODULE_ERR("%x don't support\n\r", channel);}
            
            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }
            
            new_node->data.func    = NULL;//hgadc_v1_adc_channel_txw82x_rf_vddi_config;
            new_node->data.channel = channel;
            new_node->data.sel     = sel;
            new_node->data.sch_vdd = sch_vdd;
            new_node->next         = NULL;
            hgadc_v1_list_insert(&dev->head_node, new_node);
            break;
         case (_ADC_CHANNEL_PMU_TEMPERATURE):
            hgadc_v1_chn_to_sel(ANAATO, &sel, &sch_vdd);
            if (sel==0xFF || sch_vdd==0xFF) {MODULE_ERR("%x don't support\n\r", channel);}
            
            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }
            
            new_node->data.func    = hgadc_v1_adc_channel_txw82x_pmu_temperature;
            new_node->data.channel = channel;
            new_node->data.sel     = sel;
            new_node->data.sch_vdd = sch_vdd;
            new_node->next         = NULL;
            hgadc_v1_list_insert(&dev->head_node, new_node);
            break;
		case (_ADC_CHANNEL_CORE_VDD):
            hgadc_v1_chn_to_sel(ANAATO, &sel, &sch_vdd);
            if (sel==0xFF || sch_vdd==0xFF) {MODULE_ERR("%x don't support\n\r", channel);}
            
            new_node = (adc_channel_node *)os_malloc(sizeof(adc_channel_node));
            if (!new_node) {
                enable_irq(mask);
                return RET_ERR;
            }
            
            new_node->data.func    = hgadc_v1_adc_channel_txw82x_core_vdd;
            new_node->data.channel = channel;
            new_node->data.sel     = sel;
            new_node->data.sch_vdd = sch_vdd;
            new_node->next         = NULL;
            hgadc_v1_list_insert(&dev->head_node, new_node);
            break;
    }

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;

}

static int32 hgadc_v1_delete_channel(struct adc_device *adc, uint32 channel) {

    uint32 mask = 0;
    adc_channel_node *get_node = NULL;
    struct hgadc_v1 *dev = (struct hgadc_v1 *)adc;

    if (!dev->opened) {
        return RET_ERR;
    }

    /* Close the interrupt to protect the list opreation */
    mask = disable_irq();

    if (RET_ERR == hgadc_v1_list_get_by_channel(&dev->head_node, channel, &get_node)) {
        MODULE_ERR("No this channel\n\r");
        enable_irq(mask);
        return RET_ERR;
    }

    get_node->data.func(dev, channel, ADC_CHANNEL_DISABLE);

    hgadc_v1_list_delete(&dev->head_node, channel);

    /* Enable interrupt */
    enable_irq(mask);

    return RET_OK;
}

static int32 hgadc_v1_get_value(struct adc_device *adc, uint32 channel, uint32 *raw_data) {

    struct hgadc_v1    *dev = (struct hgadc_v1 *)adc;
    struct hgadc_v1_hw *hw  = (struct hgadc_v1_hw *)dev->hw;
    adc_channel_node *get_node = NULL;
    uint32 mask = 0;

    if (!dev->opened) {
        return RET_ERR;
    }

    if (!raw_data) {
        os_printf("raw_data var is NULL!\r\n");
        return RET_ERR;
    }

    os_mutex_lock(&dev->adc_lock, osWaitForever);

    if (RET_ERR == hgadc_v1_list_get_by_channel(&dev->head_node, channel, &get_node)) {
        MODULE_ERR("No this channel\n\r");
        os_mutex_unlock(&dev->adc_lock);
        return RET_ERR;
    }

    //指向当前节点，便于后续获取当前节点中的配置
    dev->cur_node = get_node;

    /* config current channel */
    if (get_node->data.func) {
        get_node->data.func(dev, channel, ADC_CHANNEL_ENABLE);
    } else {
        os_mutex_unlock(&dev->adc_lock);
        return RET_ERR;
    }

    //防止kick的时候，没有done。但系统复位了，导致adc重新open无法工作
    mask = disable_irq();

    /* 清除通道的pending */
    saradc_pri_channel_clr_pending(hw, SINGLE_PRI_CHANNEL);

    //kick start to sample
    saradc_pri_channel_kick(hw);

    enable_irq(mask);

    /* 超时5s */
    if (os_sema_down(&dev->adc_done, 5000) <= 0){
        /* 清除通道的pending */
        saradc_pri_channel_clr_pending(hw, SINGLE_PRI_CHANNEL);
        MODULE_ERR("adkey sample timeout\r\n");
    }
    //os_printf("***  down!!\n\r");

    *raw_data = saradc_pri_channel_get_raw_data(hw, SINGLE_PRI_CHANNEL);

    //os_printf("** %d channel raw_data: %d\n\r", get_node->data.channel, *raw_data);

    get_node->data.func(dev, channel, ADC_CHANNEL_SUSPEND);

    hgadc_v1_txw82x_raw_data_handle(dev, channel, raw_data);

    if (dev->irq_en && dev->irq_hdl) {
        dev->irq_hdl(ADC_IRQ_FLAG_SAMPLE_DONE, get_node->data.channel, *raw_data);
    }

    os_mutex_unlock(&dev->adc_lock);

    return RET_OK;
}

static int32 hgadc_v1_ioctl(struct adc_device *adc, enum adc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2)
{
    struct hgadc_v1  *dev = (struct hgadc_v1 *)adc;

    if (!dev->opened) {
        return RET_ERR;
    }

    switch (ioctl_cmd) {
        case (ADC_IOCTL_SET_REF_TMP):
            dev->ref_temp = param1;
            break;

        case (ADC_IOCTL_GET_VREF):
            *(uint32 *)param1 = dev->refer_adda_vref << 8;
            break;
    }

    return RET_OK;
}

static void hgadc_v1_irq_handler(void *data) {

    struct hgadc_v1    *dev = (struct hgadc_v1 *)data;
    struct hgadc_v1_hw *hw  = (struct hgadc_v1_hw *)dev->hw;

    //os_printf("***  interrupt!!\n\r");

    //优先级通道0
    if ((saradc_pri_channel_get_pending(hw, SINGLE_PRI_CHANNEL)) && 
        (saradc_pri_channel_get_done_irq_en(hw, SINGLE_PRI_CHANNEL))) {
        
        saradc_pri_channel_clr_pending(hw, SINGLE_PRI_CHANNEL);

        os_sema_up(&dev->adc_done);
        //os_printf("***  up!!\n\r");
    }
}

static int32 hgadc_v1_request_irq(struct adc_device *adc, enum adc_irq_flag irq_flag, adc_irq_hdl irq_hdl, uint32 irq_data) {

    struct hgadc_v1    *dev = (struct hgadc_v1 *)adc;
    
    dev->irq_hdl  = irq_hdl;
    dev->irq_data = irq_data;

    if (irq_flag & ADC_IRQ_FLAG_SAMPLE_DONE) {
         dev->irq_en = 1;
    }

    return RET_OK;
}

static int32 hgadc_v1_release_irq(struct adc_device *adc, enum adc_irq_flag irq_flag) {

    struct hgadc_v1 *dev = (struct hgadc_v1 *)adc;

    if (irq_flag & ADC_IRQ_FLAG_SAMPLE_DONE) {
        dev->irq_en = 0;
    }

    return RET_OK;
}

static const struct adc_hal_ops adcops = {
    .open           = hgadc_v1_open,
    .close          = hgadc_v1_close,
    .add_channel    = hgadc_v1_add_channel,
    .delete_channel = hgadc_v1_delete_channel,
    .get_value      = hgadc_v1_get_value,
    .ioctl          = hgadc_v1_ioctl,
    .request_irq    = hgadc_v1_request_irq,
    .release_irq    = hgadc_v1_release_irq,
};

int32 hgadc_v1_attach(uint32 dev_id, struct hgadc_v1 *adc)
{
    adc->opened             = 0;
    adc->irq_en             = 0;
    adc->refer_vddi         = 0;
    adc->refer_tsensor      = 0;
    adc->refer_adda_vref    = 0;
    adc->refer_vddi_adc_data= 0;
    adc->rf_vddi_en         = 0;
    adc->irq_hdl            = NULL;
    adc->cur_node           = NULL;
    adc->adkey_range        = 0;
    adc->irq_data           = 0;
    adc->dev.dev.ops        = (const struct devobj_ops *)&adcops;

    os_mutex_init(&adc->adc_lock);
    os_sema_init(&adc->adc_done, 0);

    request_irq(adc->irq_num, hgadc_v1_irq_handler, adc);
    irq_enable(adc->irq_num);
    dev_register(dev_id, (struct dev_obj *)adc);
    
    return RET_OK;
}
