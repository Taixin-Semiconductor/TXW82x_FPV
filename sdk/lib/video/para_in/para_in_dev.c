#include "basic_include.h"
#include "hal/para_in.h"
#include "lib/video/para_in/para_in_dev.h"
#include "app_iic.h"

static const _Sensor_Para_in_Init *para_in_dev_init_table[] = {

    #if DEV_SENSOR_TP9950
    #ifdef TP9950_HDA_720P_25FPS
        &tp9950_HDA_720P_25FPS_para_in_init,
    #endif
    #endif
    
        NULL,
 };
    

int32_t hg_para_in_irq_hdl(uint32 irq_flags, uint32 irq_data, uint32 param, uint32 param2)
{
    struct para_in_device *para_in = (struct para_in_device *)param;

    switch (irq_flags)
    {
        case FRM_RX_DOWN_ISR:
            _os_printf("P");
            para_in_ioctl(para_in, PARA_IN_SET_CON_EN, 0, 0);
            para_in_ioctl(para_in, PARA_IN_SET_CON_EN, 1, 0);
            break;
        case OFOV_ISR:
            os_printf("OFOV_ISR\n");
            break;
        case IFOV_ISR:
            os_printf("IFOV_ISR\n");
            break;
        case VSYNC_ERR_ISR:
            os_printf("VSYNC_ERR_ISR\n");
            break;
        case HSYNC_ERR_ISR:
            os_printf("HSYNC_ERR_ISR\n");
            break;
        case ESAV_ERR_ISR:
            os_printf("ESAV_ERR_ISR\n");
            break;
        case TIMEOUT_ISR:
            os_printf("TIMEOUT_ISR\n");
            para_in_ioctl(para_in, PARA_IN_SET_CON_EN, 0, 0);
            para_in_ioctl(para_in, PARA_IN_SET_CON_EN, 1, 0);
            break;
        default:
            break;
    }

    return 0;
}

static void para_in_sensor_reset()
{
    if (PIN_BT_RESET != 255) {
        gpio_iomap_output(PIN_BT_RESET,GPIO_DIR_OUTPUT);
        gpio_set_val(PIN_BT_RESET,1);
        os_sleep_ms(1);
        gpio_set_val(PIN_BT_RESET,0);
        os_sleep_ms(1);
        gpio_set_val(PIN_BT_RESET,1);  
    }
};

static _Sensor_Para_in_Init * para_in_auto_check_dev_id(uint32_t iic_dev,  uint8_t **init_table)
{
    uint8_t tmpbuf[16];
    uint16_t id = 0;
    _Sensor_Para_in_Init **p_init_table = (_Sensor_Para_in_Init **)init_table;
    _Sensor_Para_in_Init *p_init = *p_init_table;
    for (int i = 0; p_init != (_Sensor_Para_in_Init *)NULL; i++) {
        os_printf("p_init:%x p_init_table:%x\n",p_init,p_init_table);
        para_in_sensor_reset();

        id = 0;

        tmpbuf[0] = 0x1;            
        tmpbuf[1] = 0x1;	    
        tmpbuf[2] = p_init->i2c_dev_addr;                    //I2C dev addr

        //I2C ID 高8位
        if (p_init->i2c_id_addr1 != 0) {
            tmpbuf[3] = p_init->i2c_id_addr1;
            tmpbuf[4] = 0;
            wake_up_iic_queue(iic_dev,tmpbuf,0,0,(uint8_t*)NULL);
            while(iic_devid_finish(iic_dev) != 1){
                os_sleep_ms(1);
            }
            id = tmpbuf[4] << 8;
        }

        //I2C ID 低8位
        if (p_init->i2c_id_addr2 != 0) {
            tmpbuf[3] = p_init->i2c_id_addr2;
            tmpbuf[4] = 0;
            wake_up_iic_queue(iic_dev,tmpbuf,0,0,(uint8_t*)NULL);
            while(iic_devid_finish(iic_dev) != 1){
                os_sleep_ms(1);
            }
            id |= tmpbuf[4];   
        }

        if (id == p_init->i2c_id) {
            os_printf("%s id:0x%x \n",__FUNCTION__,id);
            return p_init;
        }
     
        os_printf("id:0x%x addr1:0x%x addr2:0x%x sensorID:0x%x\n",id, p_init->i2c_id_addr1, p_init->i2c_id_addr2, p_init->i2c_id);
        p_init++;
    }
    return NULL;
}

static void para_in_i2c_init_sensor(uint32_t iic_dev,  _Sensor_Para_in_Init *sensor_dev)
{
    int i;
    uint8_t tmpbuf[16];
	_PARA_IN_SENSOR_REG_INFO *preg_info = sensor_dev->i2c_init_table;

    for (i = 0; ; i++)
    {
        if (preg_info->reg_addr == 0xFF && preg_info->value == 0xFF) {
            os_printf("%s %d init num:%d\n", __FUNCTION__, __LINE__, i);
            break;
        }

        tmpbuf[0] = 0x1;
        tmpbuf[1] = 0x1;	
        tmpbuf[2] = sensor_dev->i2c_dev_addr;
        tmpbuf[3] = preg_info->reg_addr;
        tmpbuf[4] = preg_info->value;
        wake_up_iic_queue(iic_dev,tmpbuf,0,1,(uint8_t*)NULL);
        while(iic_devid_finish(iic_dev) != 1){
            os_sleep_ms(1);
        } 
        preg_info++;  
    }
}


void para_in_hareware_init()
{
    struct para_in_device *para_in = (struct para_in_device *)dev_get(HG_PARA_IN_DEVID);
    struct i2c_device *iic_dev = (struct i2c_device *)dev_get(HG_I2C1_DEVID);
    _Sensor_Para_in_Init * p_sensor = NULL;
    volatile uint32_t app_iic = 0;
    app_iic = register_iic_queue(iic_dev,PC_15,PD_14,0);

    p_sensor = para_in_auto_check_dev_id(app_iic, (uint8_t**)&para_in_dev_init_table);
    if (p_sensor) {
        para_in_i2c_init_sensor(app_iic, p_sensor);
    } else {
        os_printf("No found para in sensor!!!\n");
        return;
    }

    os_printf("%s %d success find sensor\n",__FUNCTION__,__LINE__);

    para_in_open(para_in);

    if(p_sensor->data_format == PARA_IN_BT601) 
    {
        para_in_ioctl(para_in, PARA_IN_RC_ST_DTC_MODE, 0, 0);
        para_in_ioctl(para_in, PARA_IN_SET_VSYNC_POL, p_sensor->vs_pol, 0);
        para_in_ioctl(para_in, PARA_IN_SET_HSYNC_POL, p_sensor->hs_pol, 0);
        para_in_ioctl(para_in, PARA_IN_SET_VHSYNC_EN, 1, 0);
        para_in_ioctl(para_in, PARA_IN_ESAV_ERR_DTC_EN, 1, 0);
    }
    // BT656、BT1120
    else
    {
        para_in_ioctl(para_in, PARA_IN_RC_ST_DTC_MODE, 1, 0);
        para_in_ioctl(para_in, PARA_IN_SET_VHSYNC_EN, 0, 0);
        para_in_ioctl(para_in, PARA_IN_ESAV_ERR_DTC_EN, 1, 0);
    }

    para_in_ioctl(para_in, PARA_IN_SAV_AHEAD, 0, 0);
    para_in_ioctl(para_in, PARA_IN_FRT_VSYNC_SEL, 1, 0);
    
    //根据帧率计算超时时间，超时未获得帧，应重启模块
    para_in_ioctl(para_in, PARA_IN_SET_TIMEOUT_EN, 0, 0);
    para_in_ioctl(para_in, PARA_IN_SET_TIMEOUT_TIME, 1, 0);

    para_in_ioctl(para_in, PARA_IN_SMAP_CLK_SEL, p_sensor->smap_edge, 0);
    para_in_ioctl(para_in, PARA_IN_INTF_SEL, p_sensor->data_format, 0);
    para_in_ioctl(para_in, PARA_IN_YUV_SEL, p_sensor->out_yuv_format, 0);
    para_in_ioctl(para_in, PARA_IN_SET_INPUT_SEQUENCE, p_sensor->in_yuv_sequence, 0);
    para_in_ioctl(para_in, PARA_IN_SET_OUTPUT_SEQUENCE, p_sensor->out_yuv_sequence, 0);

    para_in_ioctl(para_in, PARA_IN_SET_PARAMS, (uint32)p_sensor, 0);
    para_in_request_irq(para_in,  FRM_RX_DOWN_ISR | OFOV_ISR | IFOV_ISR | VSYNC_ERR_ISR | HSYNC_ERR_ISR | ESAV_ERR_ISR,
                        hg_para_in_irq_hdl, (uint32)NULL);

    para_in_ioctl(para_in, PARA_IN_SET_CON_EN, 1, 0);
    os_printf("para in open\n");
}