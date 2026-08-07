#ifndef HAL_OF_H
#define HAL_OF_H
#ifdef __cplusplus
extern "C" {
#endif

enum of_instr_mode {
    CUSADA8_INIT_ZERO,
	CUSADA8_INIT_ACC,
	CUHADD8_AVG,		
};

enum of_ioctl_cmd {
    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	OF_INSTR_MODE,

	CAL_PIXEL_NUM,

	CAL_ROW_NUM,

	SET_ROW_SIZE,

	SET_OF_ADDR,

	GET_CAL_ACC,
};

struct of_device {
    struct dev_obj dev;
};

struct of_hal_ops {
	struct devobj_ops ops;
	int32(*init)(struct of_device *of_dev);
	int32(*deinit)(struct of_device *of_dev);
    int32(*ioctl)(struct of_device *of_dev, enum of_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
};


int32 of_init(struct of_device *p_of);
int32 of_deinit(struct of_device *p_of);
int32 of_set_instr_mode(struct of_device *p_of,enum of_instr_mode instr_format);
int32 of_set_pixel_num(struct of_device *p_of,uint32 pixel_num);
int32 of_set_row_num(struct of_device *p_of,uint32 row_num);
int32 of_set_row_size(struct of_device *p_of,uint32 row_size);
int32 of_set_addr(struct of_device *p_of,uint32 addr0,uint32 addr1);
int32 of_get_acc(struct of_device *p_of);


#ifdef __cplusplus
}
#endif
#endif
