#ifndef HAL_PARA_IN_H
#define HAL_PARA_IN_H
#ifdef __cplusplus
extern "C" {
#endif

typedef int32(*para_in_irq_hdl)(uint32 irq_flags, uint32 irq_data, uint32 param, uint32 param2);

enum para_in_dev_irqs {
    FRM_RX_DOWN_ISR = BIT(0),
    OFOV_ISR        = BIT(1),
    IFOV_ISR        = BIT(2),
    VSYNC_ERR_ISR   = BIT(3),
    HSYNC_ERR_ISR   = BIT(4),
    ESAV_ERR_ISR    = BIT(5),
    TIMEOUT_ISR     = BIT(6),
};

enum para_in_intf {
    PARA_IN_BT601 = 0,
    PARA_IN_BT656,
    PARA_IN_BT1120,
};

enum para_in_dev_fmt {
    PARA_IN_YUV = 0,
    PARA_IN_YCrCb,
};

// VPP/ISP 与 PARA IN 模式顺序相反
enum para_in_dev_yuv_sequence {
    U_Y1_V_Y2 = 0,
    V_Y1_U_Y2,
    Y1_U_Y2_V,
    Y1_V_Y2_U,
    U_Y2_V_Y1,
    V_Y2_U_Y1,
    Y2_U_Y1_V,
    Y2_V_Y1_U,
};

enum para_in_ioctl_cmd{
    PARA_IN_SET_PARAMS = 0,
    PARA_IN_SET_TIMEOUT_EN,
    PARA_IN_SET_TIMEOUT_TIME,
    PARA_IN_SET_VHSYNC_EN,
    PARA_IN_SET_HSYNC_POL, 
    PARA_IN_SET_VSYNC_POL,
    PARA_IN_SMAP_CLK_SEL,
    PARA_IN_INTF_SEL,
    PARA_IN_YUV_SEL,
    PARA_IN_SET_OUTPUT_SEQUENCE,
    PARA_IN_SET_INPUT_SEQUENCE,
    PARA_IN_FRT_VSYNC_SEL,
    PARA_IN_SAV_AHEAD,
    PARA_IN_RC_ST_DTC_MODE, 
    PARA_IN_ESAV_ERR_DTC_EN,
    PARA_IN_SET_CON_EN,
};

enum para_in_samp_edge{
    PARA_IN_RISING_EDGE = 0,
    PARA_IN_FALLING_EDGE,
};

typedef struct PARA_IN_SENSOR_REG_INFO
{
    unsigned char reg_addr;
    unsigned char value;
} _PARA_IN_SENSOR_REG_INFO;

#define PARA_IN_REG_END     {0xFF,0xFF}

struct para_in_params {
    uint32_t act_hblk;          //ACT_HBLK : 有效行非有效数据长度（注：若有数据含有EAV，SAV需加上EAV与SAV长度。）
    uint32_t act_lpot_num;      //ACT_LPOT_NUM : 有效行的真实数据点数（注：若一行的真实数据点数为720个，则有效数据长度为720 X 2。）
    uint32_t act_hf_frm_num;    //ACT_HF_FRM_NUM : 半帧图像的有效行数
    uint8_t frt_fblk;           //FRT_FBLK : 逐行数据的第一段消隐行数（注：至少为1）
    uint8_t sec_sblk;           //SEC_SBLK : 逐行数据的第二段消隐行数（注：至少为1）
    uint32_t width;
    uint32_t height;
    uint8_t data_format;        //para_in_intf (BT601 / BT656 / BT1120)
    uint8_t in_yuv_sequence;    //para_in_dev_yuv_sequence  输入YUV顺序
    uint8_t out_yuv_sequence;   //para_in_dev_yuv_sequence  输出YUV顺序
    uint8_t out_yuv_format;     //para_in_dev_fmt
    uint8_t smap_edge;          //para_in_samp_edge 采样边沿
    uint8_t vs_pol;             //0: 高电平有效 1：低电平有效
    uint8_t hs_pol;             //0: 高电平有效 1：低电平有效
    uint8_t i2c_dev_addr;
    uint8_t i2c_id_addr1;
    uint8_t i2c_id_addr2;
    uint16_t i2c_id; 
    _PARA_IN_SENSOR_REG_INFO *i2c_init_table;
};
typedef struct para_in_params _Sensor_Para_in_Init;

struct para_in_device {
    struct dev_obj dev;
};

struct para_in_hal_ops {
    struct devobj_ops ops;
    int32(*open)(struct para_in_device *para_in_dev);
    int32(*close)(struct para_in_device *para_in_dev);
    int32(*ioctl)(struct para_in_device *para_in_dev, enum para_in_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct para_in_device *para_in_dev, uint32 irq_flag, para_in_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct para_in_device *para_in_dev, uint32 irq_flag);	
};

int32 para_in_open(struct para_in_device *p_dev);
int32 para_in_close(struct para_in_device *p_dev);
int32 para_in_ioctl(struct para_in_device *p_dev, uint32 cmd, uint32 param1, uint32 param2);
int32 para_in_request_irq(struct para_in_device *p_dev, uint32 irq_flags, para_in_irq_hdl handle, uint32 data);
int32 para_in_release_irq(struct para_in_device *p_dev, uint32 irq_flag);

#ifdef __cplusplus
}
#endif
#endif