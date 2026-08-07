#include "lib/touch/cst226se.h"

#if CST226SE_TOUCH_PAD

#define MALLOC                  os_malloc
#define ZALLOC                  os_zalloc
#define FREE                    os_free

#define CST226SE_I2C_ADDR       (0x5A)
#define TP_I2C_BUS              (HG_I2C1_DEVID)
#define PIN_TP_I2C_SCL          PC_11  
#define PIN_TP_I2C_SDA          PC_12
#define PIN_TP_INT              PC_10
#define PIN_TP_RST              255     //学习板默认硬件拉高 (PC_9 / PC_8)

#define SWAP_XY                 0
#define REVERSE_X               0    
#define REVERSE_Y               0
#define X_RESOLUTION            480
#define Y_RESOLUTION            800

#define EVENT_TOUCH_CST226SE    (1 << 0)

static struct hyn_ts_data *g_hyn_226data = NULL;
static const uint8_t gest_map_tbl[33] = {0xff,4,1,3,2,5,12,6,7,7,9,11,10,13,12,7,7,6,10,6,5,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,14};

static void cst226se_rst(void)
{
    uint8_t rst = MACRO_PIN(PIN_TP_RST);

    if (rst != 255)
    {
        gpio_iomap_output(rst, GPIO_IOMAP_OUTPUT);
        gpio_set_val(rst, 0);
        os_sleep_ms(10);
        gpio_set_val(rst, 1);
        os_sleep_ms(50);
    }
}

static int cst226se_enter_boot(void* p_dev)
{
    struct hyn_ts_data *hyn_226data = (struct hyn_ts_data *)p_dev;
    int retry = 5,ret = 0;
    uint8_t buf[4] = {0};
    while(++retry<17){
        cst226se_rst();
        os_sleep_ms(retry);
        ret = hyn_wr_reg(hyn_226data,0xA001AA,3,buf,0);
        if(ret != 0){
            continue;
        }
        ret = hyn_wr_reg(hyn_226data,0xA003,2,buf,1);
        if(ret == 0 &&  buf[0] == 0x55){
            return 0;
        }
    }
    return -1;
}

static uint32_t cst226se_read_checksum(void* p_dev)
{
    struct hyn_ts_data *hyn_226data = (struct hyn_ts_data *)p_dev;
    int ret;
    uint8_t buf[4],retry = 5;
    hyn_226data->boot_is_pass = 0;
    while(retry--){
        ret = hyn_wr_reg(hyn_226data,0xA000,2,buf,1);
        if(ret){
			os_sleep_ms(2);
			continue;
        }
        if(buf[0]!=0) break;
        os_sleep_ms(2);
    }
    os_sleep_ms(1);
    if(buf[0] == 0x01){
        os_memset(buf,0,sizeof(buf));
        ret = hyn_wr_reg(hyn_226data,0xA008,2,buf,4);
        hyn_226data->boot_is_pass = 1;
    }
    return U8TO32(buf[3],buf[2],buf[1],buf[0]);
}

static uint32_t cst226se_check_esd(void* p_dev)
{
    struct hyn_ts_data *hyn_226data = (struct hyn_ts_data *)p_dev;
    int ret = 0;
    uint8_t buf[6];
    ret = hyn_wr_reg(hyn_226data,0xD040,2,buf,6);
    if(ret ==0){
        uint16_t checksum = buf[0]+buf[1]+buf[2]+buf[3]+0xA5;
        if(checksum != ( (buf[4]<<8)+ buf[5])){
            ret = -1;
        }
    }
    return ret ? 0:(buf[3]+(buf[2]<<8)+(buf[1]<<16)+(buf[0]<<24));
}

static int cst226se_set_workmode(struct hyn_ts_data * p_dev, enum work_mode mode, uint8_t enable)
{
    int ret = 0;
    struct hyn_ts_data *hyn_226data = (struct hyn_ts_data *)p_dev;
    hyn_226data->work_mode = mode;
    if(mode != NOMAL_MODE)
        hyn_esdcheck_switch(hyn_226data,DISABLE);
    switch(mode){
        case NOMAL_MODE:
            hyn_irq_set(hyn_226data,1);
            hyn_esdcheck_switch(hyn_226data,enable);
            hyn_wr_reg(hyn_226data,0xD10B,2,NULL,0); //soft rst
            hyn_wr_reg(hyn_226data,0xD109,2,NULL,0);
            break;
        case GESTURE_MODE:
            hyn_wr_reg(hyn_226data,0xD04C80,3,NULL,0);
            break;
        case LP_MODE:
            break;
        case DIFF_MODE:
            hyn_wr_reg(hyn_226data,0xD10B,2,NULL,0);
            hyn_wr_reg(hyn_226data,0xD10D,2,NULL,0);
            break;
        case RAWDATA_MODE:
            hyn_wr_reg(hyn_226data,0xD10B,2,NULL,0);
            hyn_wr_reg(hyn_226data,0xD10A,2,NULL,0);
            break;
        case FAC_TEST_MODE:
            hyn_wr_reg(hyn_226data,0xD10B,2,NULL,0);
            hyn_wr_reg(hyn_226data,0xD119,2,NULL,0);
            os_sleep_ms(50); //wait  switch to fac mode
            break;
        case DEEPSLEEP:
            hyn_irq_set(hyn_226data,0);
            hyn_wr_reg(hyn_226data,0xD105,2,NULL,0);
            break;
        case ENTER_BOOT_MODE:
            ret |= cst226se_enter_boot(hyn_226data);
            break;
        default :
            hyn_esdcheck_switch(hyn_226data,ENABLE);
            hyn_226data->work_mode = NOMAL_MODE;
            break;
    }
    return ret;
}

static int cst226se_supend(void* p_dev)
{
    struct hyn_ts_data *hyn_226data = (struct hyn_ts_data *)p_dev;
    HYN_ENTER();
    cst226se_set_workmode(hyn_226data, DEEPSLEEP,0);
    return 0;
}

static int cst226se_resum(void* p_dev)
{
    struct hyn_ts_data *hyn_226data = (struct hyn_ts_data *)p_dev;
    HYN_ENTER();
    cst226se_rst();
    os_sleep_ms(50);
    cst226se_set_workmode(hyn_226data, NOMAL_MODE,0);
    return 0;
}

static int cst226se_updata_tpinfo(void* p_dev)
{
    struct hyn_ts_data *hyn_226data = (struct hyn_ts_data *)p_dev;
    uint8_t buf[28];
    struct tp_info *ic = &hyn_226data->hw_info;
    int ret = 0,retry = 5;
    while(--retry){
        ret = hyn_wr_reg(hyn_226data,0xD101,2,buf,0);
        os_sleep_ms(1);
        ret |= hyn_wr_reg(hyn_226data,0xD1F4,2,buf,28);
        cst226se_set_workmode(hyn_226data, NOMAL_MODE,0);
        os_printf("buf[18]:0x%x, buf[19]:0x%x\n",buf[18],buf[19]);
        if(ret ==0 &&  U8TO16(buf[19],buf[18])==0x00a8){
            break;
        }
        os_sleep_ms(1);
    }

    if(ret || retry==0){
        HYN_ERROR("cst226se_updata_tpinfo failed");
        return -1;
    }
    ic->fw_sensor_txnum = buf[0];
    ic->fw_sensor_rxnum = buf[2];
    ic->fw_key_num = buf[3];
    ic->fw_res_y = (buf[7]<<8)|buf[6];
    ic->fw_res_x = (buf[5]<<8)|buf[4];
    ic->fw_project_id = (buf[17]<<8)|buf[16];
    ic->fw_chip_type = U8TO16(buf[19],buf[18]);
    ic->fw_ver = (buf[23]<<24)|(buf[22]<<16)|(buf[21]<<8)|buf[20];

    HYN_INFO("IC_info fw_project_id:%04x ictype:%04x fw_ver:%x checksum:%#x",ic->fw_project_id,ic->fw_chip_type,ic->fw_ver,ic->ic_fw_checksum);
    return 0;
}

static int cst226se_updata_judge(void* p_dev, uint8_t *p_fw, uint16_t len)
{
    struct hyn_ts_data *hyn_226data = (struct hyn_ts_data *)p_dev;
    uint32_t f_check_all,f_checksum,f_fw_ver,f_ictype,f_fw_project_id;
    int ret = 0;
    uint8_t *p_data = p_fw +7680-12; 
    struct tp_info *ic = &hyn_226data->hw_info;

    f_fw_project_id = U8TO16(p_data[1],p_data[0]);
    f_ictype = U8TO16(p_data[3],p_data[2]);

    f_fw_ver = U8TO32(p_data[7],p_data[6],p_data[5],p_data[4]);
    f_checksum = U8TO32(p_data[11],p_data[10],p_data[9],p_data[8]);

    HYN_INFO("Bin_info fw_project_id:%04x ictype:%04x fw_ver:%x checksum:%#x",f_fw_project_id,f_ictype,f_fw_ver,f_checksum);

    ret = cst226se_updata_tpinfo(p_dev);
    if(ret)HYN_ERROR("get tpinfo failed");

    //check h file
    f_check_all = hyn_sum32(0x55,(uint32_t*)p_fw,(len-4)/4);
    if(f_check_all != f_checksum){
        HYN_INFO(".h file checksum erro:%04x len:%d",f_check_all,len);
        return 0; //not updata
    }
    if(hyn_226data->boot_is_pass ==0    //emty chip
    ||(hyn_226data->boot_is_pass && ret) //erro code
    || ( ret == 0 && f_checksum != ic->ic_fw_checksum  && f_fw_ver >= ic->fw_ver ) // match new ver .h file
    ){
        return 1; //need updata
    } 
    return 0;
}

static int cst226se_report(void* p_dev)
{
    struct hyn_ts_data *hyn_226data = (struct hyn_ts_data *)p_dev;
    uint8_t buf[80]={0};
    uint8_t finger_num = 0,key_flg = 0,tmp_dat;
    int len = 0;
    int ret = 0,retry = 2;
    switch(hyn_226data->work_mode){
        case NOMAL_MODE:
            retry = 2;
            while(retry--){
                ret = hyn_wr_reg(hyn_226data,0xD000,2,buf,7);
                finger_num = buf[5] & 0x7F;
                if(ret || buf[6] != 0xAB || buf[0] == 0xAB || finger_num > MAX_POINTS_REPORT){
                    ret = -2;
                    continue;
                }
                key_flg = (buf[5]&0x80) ? 1:0;
                len = 0;
                if(finger_num > 1){
                    len += (finger_num-1)*5;
                }
                if(key_flg && finger_num){
                    len += 3;
                }
                if(len > 0){
                    ret = hyn_wr_reg(hyn_226data,0xD007,2,&buf[5],len);
                }
                ret |= hyn_wr_reg(hyn_226data,0xD000AB,3,buf,0);
                if(ret){
                    ret = -3;
                    continue;
                } 
                ret = 0;
                break;
            }
            if(ret){
                hyn_wr_reg(hyn_226data,0xD000AB,3,buf,0);
                HYN_ERROR("read frame failed");
                break;
            }
            if(key_flg){ //key
                if(hyn_226data->rp_buf.report_need == REPORT_NONE){
                    hyn_226data->rp_buf.report_need |= REPORT_KEY;
                } 
                len = finger_num ? (len+5):3;
                hyn_226data->rp_buf.key_id = (buf[len-2]>>4)-1;
                hyn_226data->rp_buf.key_state = (buf[len-3]&0x0F)==0x03 ? 1:0; 
                HYN_INFO("key_id:%x state:%x",hyn_226data->rp_buf.key_id ,hyn_226data->rp_buf.key_state);
            }
            if(finger_num){
                uint16_t index = 0,i = 0;
                uint8_t touch_down = 0;
                if(hyn_226data->rp_buf.report_need == REPORT_NONE){
                    hyn_226data->rp_buf.report_need |= REPORT_POS;
                }
                hyn_226data->rp_buf.rep_num = finger_num;
                for(i = 0; i < finger_num; i++){
                    index = i*5;
                    hyn_226data->rp_buf.pos_info[i].pos_id =  (buf[index]>>4)&0x0F;
                    hyn_226data->rp_buf.pos_info[i].event =  (buf[index]&0x0F) == 0x06 ? 1 : 0;
                    hyn_226data->rp_buf.pos_info[i].pos_x = ((uint16_t)buf[index + 1]<<4) + ((buf[index + 3] >> 4) & 0x0F);
                    hyn_226data->rp_buf.pos_info[i].pos_y = ((uint16_t)buf[index + 2]<<4) + (buf[index + 3] & 0x0F);
                    hyn_226data->rp_buf.pos_info[i].pres_z = buf[index + 4];
                    if(hyn_226data->rp_buf.pos_info[i].event) touch_down++;
                    HYN_INFO("report_id = %d, xy = %d,%d",hyn_226data->rp_buf.pos_info[i].pos_id,hyn_226data->rp_buf.pos_info[i].pos_x,hyn_226data->rp_buf.pos_info[i].pos_y);
                }
                if(0== touch_down){
                    hyn_226data->rp_buf.rep_num = 0;
                }
            }
            break;
        case GESTURE_MODE:
            ret = hyn_wr_reg(hyn_226data,0xD04C,2,&tmp_dat,1);
            if((tmp_dat&0x7F) <= 32){
                tmp_dat = tmp_dat&0x7F;
                hyn_226data->gesture_id = gest_map_tbl[tmp_dat];
                hyn_226data->rp_buf.report_need |= REPORT_GES;
            }
            break;
        default:
            break;
    }
    return ret;
}


static void cst226se_int_handler(int32 id, enum gpio_irq_event event, uint32 param1, uint32 param2)
{
    struct hyn_ts_data *hyn_226data = (struct hyn_ts_data *)id;
    os_event_set(&hyn_226data->event, EVENT_TOUCH_CST226SE, 0);
}

static void cst226se_int_init(void *p_dev)
{
    struct hyn_ts_data *hyn_226data = (struct hyn_ts_data *)p_dev;
    uint8_t int_pin = MACRO_PIN(PIN_TP_INT);
    if (int_pin != 255)
    {
        gpio_iomap_input(int_pin, GPIO_IOMAP_INPUT);
        gpio_ioctl(int_pin, GPIO_DEBUNCE, 1, 0);
        gpio_request_pin_irq(int_pin, cst226se_int_handler, (uint32)hyn_226data, GPIO_IRQ_EVENT_FALL);
    }
}


static int32 cst226se_work(struct os_work *work)
{
    int ret = 0;
    uint32_t event;
    struct hyn_ts_data *hyn_226data = (struct hyn_ts_data *)work;

    ret = os_event_wait(&hyn_226data->event, EVENT_TOUCH_CST226SE, &event, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);

    if(ret != RET_OK) {
        goto __cst226se_end;
    }
    
    if(event & EVENT_TOUCH_CST226SE)
    {
        cst226se_report(hyn_226data);
        struct ts_frame *frame = (struct ts_frame *)ZALLOC(sizeof(struct ts_frame));
        if (frame) {
            os_memcpy(frame, &hyn_226data->rp_buf, sizeof(struct ts_frame));
            if (os_msgq_put(&hyn_226data->msgque, (uint32)frame, 0)) {
                FREE(frame);
            }
        } else {
            HYN_ERROR("Failed to allocate memory for ts_frame");
        }
    }

__cst226se_end:
    os_run_work_delay(work, 100);
    return RET_OK;
}

void *cst226se_get_multipoint_xy()
{
    struct ts_frame *frame = NULL;
    struct hyn_ts_data *hyn_226data = (struct hyn_ts_data *)g_hyn_226data;
    struct hyn_plat_data *dt = &hyn_226data->plat_data;
    uint16_t xpos,ypos;
    frame = (struct ts_frame *)os_msgq_get(&hyn_226data->msgque, 0);
    if (frame) {
        touch_multipoint_pos_t *data = (touch_multipoint_pos_t *)ZALLOC(sizeof(touch_multipoint_pos_t));
        if (data)
        {
            data->point_num = frame->rep_num;
            for (int i = 0; i < frame->rep_num && i < MAX_POINT_NUM; i++) {
                if(dt->swap_xy){
                    xpos = frame->pos_info[i].pos_y;
                    ypos = frame->pos_info[i].pos_x;
                    if(xpos > dt->y_resolution || ypos > dt->x_resolution){
                        HYN_ERROR("Please check dts or FW config,maybe resolution or origin issue!!!");
                    }
                }
                else{
                    xpos = frame->pos_info[i].pos_x;
                    ypos = frame->pos_info[i].pos_y;
                    if(ypos > dt->y_resolution || xpos > dt->x_resolution){
                        HYN_ERROR("Please check dts or FW config,maybe resolution or origin issue!!!");
                    }
                }
                if(frame->pos_info[i].pos_id >= MAX_POINT_NUM){
                    continue;
                }
                if(dt->reverse_x){
                    if (dt->swap_xy) {
                        xpos = dt->y_resolution-xpos;
                    } else {
                        xpos = dt->x_resolution-xpos;
                    }
                }
                if(dt->reverse_y){
                    if (dt->swap_xy) {
                        ypos = dt->x_resolution-ypos;
                    } else {
                        ypos = dt->y_resolution-ypos;
                    }
                }
                data->pos_x[i] = xpos;
                data->pos_y[i] = ypos;
                HYN_INFO("cst226se_get_multipoint_xy i:%d data->xpos:%d data->ypos:%d\n",i,xpos,ypos);
            }
        }
        FREE(frame);
        return data;
    } else {
        // HYN_ERROR("Failed to get ts_frame from message queue");
        return NULL;
    }
}

uint32_t cst226se_free_multipoint_xy(void *data)
{
    if (data) {
        FREE(data);
        return RET_OK;
    }
    HYN_ERROR("Data pointer is NULL");
    return RET_ERR;
}

void cst226se_init()
{
    int ret = 0;
    int app_iic_num = 0;
    struct i2c_device *iic_dev  = NULL;
    struct hyn_ts_data *p_dev   = NULL;
    
    p_dev = (struct hyn_ts_data *)ZALLOC(sizeof(struct hyn_ts_data));
    if (p_dev == NULL)
    {
        os_printf("%s %d\n",__FUNCTION__,__LINE__);
        return;
    }
    p_dev->iic_trx_buff = (uint8_t *)ZALLOC(80);
    if (p_dev->iic_trx_buff == NULL)
    {
        HYN_ERROR("Failed to allocate memory for iic_trx_buff");
        FREE(p_dev);
        return;
    }

    g_hyn_226data = p_dev;

    cst226se_rst();

    iic_dev = (struct i2c_device*)dev_get(TP_I2C_BUS);
    app_iic_num = register_iic_queue(iic_dev, MACRO_PIN(PIN_TP_I2C_SCL), MACRO_PIN(PIN_TP_I2C_SDA), 0);

    p_dev->app_iic_num = app_iic_num;
    p_dev->iic_addr    = CST226SE_I2C_ADDR;
    p_dev->plat_data.swap_xy = SWAP_XY;
    p_dev->plat_data.reverse_x = REVERSE_X;
    p_dev->plat_data.reverse_y = REVERSE_Y;
    p_dev->plat_data.x_resolution = X_RESOLUTION;
    p_dev->plat_data.y_resolution = Y_RESOLUTION;

    ret = os_event_init(&p_dev->event);
    if (ret != RET_OK) {
        HYN_ERROR("Failed to initialize event");
        FREE(p_dev);
        return;
    }
    ret = os_msgq_init(&p_dev->msgque, 10);
    if (ret != RET_OK) {
        HYN_ERROR("Failed to initialize message queue");
        os_event_del(&p_dev->event);
        FREE(p_dev);
        return;
    }

    cst226se_set_workmode(p_dev, NOMAL_MODE, 1);
    cst226se_int_init(p_dev);

    OS_WORK_INIT(&p_dev->work, cst226se_work, 0);
    os_run_work_delay(&p_dev->work, 30);
}


#endif