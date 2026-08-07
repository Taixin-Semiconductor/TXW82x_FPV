#ifndef __HG_TOUCHKEY_H
#define __HG_TOUCHKEY_H
enum tk_ioctl_cmd
{
	//硬件寄存器配置
	TK_IOCTL_CSRST_SET,
	TK_IOCTL_CMPEN_SET,
	TK_IOCTL_CMPRB_SET,
	TK_IOCTL_RES_SET,
	TK_IOCTL_CURR_SET,
	TK_IOCTL_ANA_TEN_SET,
	TK_IOCTL_VOLT_REF_SET,
	TK_IOCTL_PSR_CNT_SET,
	TK_IOCTL_PRE_CHARGE_TIME,
	TK_IOCTL_PRE_DISCHARGE_TIME,
	TK_IOCTL_CONV_TIME,
	TK_IOCTL_CHANNLE_MODE,
	TK_IOCTL_CHARGE_FRE_DIV,
	TK_IOCTL_CHARGE_FRE0_DIV,
	TK_IOCTL_CHARGE_FRE1_DIV,
	TK_IOCTL_CHARGE_FRE2_DIV,
	TK_IOCTL_CHARGE_FRE3_DIV,
	TK_IOCTL_SHIELD_SET,
	TK_IOCTL_SSC_BW,
	TK_IOCTL_SSC_TYPE_SEL,
	TK_IOCTL_AUTO_SCAN_EN,
	TK_IOCTL_ADDR_SET,
	TK_IOCTL_KICK,

	//算法参数配置
	TK_IOCTL_ADJUST_TIME,
	TK_IOCTL_LONG_KEY_TIME,
	TK_IOCTL_ADJUST_LINE,
	TK_IOCTL_ADJUST_DIFF_VALU,
	TK_IOCTL_USE_NUM,
	TK_IOCTL_WATER_MODE,
	TK_IOCTL_NM_NUM,
	TK_IOCTL_NM_LEAVE_CNT,
	TK_IOCTL_TP_EN,
	TK_IOCTL_NOISE_VALUE,
	TK_IOCTL_NM_CM_CNT,
	TK_IOCTL_NM_CM_VALUE,

	//系统级别的配置(类似信号量,timer等等)
	TK_SCAN_SEM_HANDL,
	TK_TIMER_HANDL,
};



enum tk_irq_flag
{
	TK_SG_CONV_FLAG 	= BIT(0),
	TK_CH_DONE_FLAG 	= BIT(1),
	TK_SCAN_DONE_FLAG 	= BIT(2),
	TK_DMA_FLAG 		= BIT(3),
	TK_DMA_ERR_FLAG 	= BIT(4),
};
typedef int32 (*tk_irq_hdl)(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2);

struct tk_device {
    struct dev_obj dev;
    int32(*open)(struct tk_device *tk);
    int32(*close)(struct tk_device *tk);
    int32(*ioctl)(struct tk_device *tk, uint32 cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct tk_device *tk, enum tk_irq_flag irq_flag, tk_irq_hdl irq_hdl, uint32 data);
	int32(*release_irq)(struct tk_device *tk, enum tk_irq_flag irq_flag);
};

//tk算法私有结构体
struct hgtk_algorithm
{
	uint16 __tk_adjust_time;
	uint16 __tk_valid_time;
	uint16 __tk_adjust_line;
	uint16 __tk_adjust_diff_valu; 
	uint16 __tk_use_num;
	uint16 __tk_water_mode;
	uint16 __tk_nm_num;
	uint16 __tk_nm_leave_cnt;
	uint16 __tk_tp_en;
	uint16 __tk_noise_value;
	uint16 __tk_nm_cm_cnt;
	uint16 __tk_nm_cm_value;
};

struct hgtk_device {
    struct tk_device     dev     ;
    uint32               hw      ;
    tk_irq_hdl           irq_hdl ;
    uint32               irq_data;
    uint32               irq_num ;
    uint32               opened  ;
	void*				 timer;
	void*				 sem;
	struct hgtk_algorithm	tk_pri;
};

#endif
