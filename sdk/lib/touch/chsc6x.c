#include "lib/touch/chsc6x.h"
#include "user_work.h"


#define MALLOC                  os_malloc
#define ZALLOC                  os_zalloc
#define FREE                    os_free

#define CHSC6X_I2C_ADDR         (0x15)  //(0x2E)
#define TP_I2C_BUS              (HG_I2C1_DEVID)

#define SWAP_XY                 0
#define REVERSE_X               0
#define REVERSE_Y               0
#define X_RESOLUTION            320
#define Y_RESOLUTION            240

#define CHSC6X_TOUCH_DATA_ADDR  (0x0000)

#define CHSC6X_HEADER_SIZE      (3)
#define CHSC6X_POINT_SIZE       (6)
#define CHSC6X_MAX_POINTS       (2)
#define CHSC6X_FRAME_SIZE       (CHSC6X_HEADER_SIZE + CHSC6X_MAX_POINTS * CHSC6X_POINT_SIZE)

#define CHSC6X_EVENT_DOWN       (0x00)
#define CHSC6X_EVENT_CONTACT    (0x80)
#define CHSC6X_EVENT_UP         (0x40)

#define EVENT_TOUCH_INT         (1 << 0)

#define CHSC6X_DEBUG_EN         0

#if CHSC6X_DEBUG_EN
#define CHSC6X_LOG(fmt, ...)    os_printf("[CHSC6X] " fmt "\r\n", ##__VA_ARGS__)
#else
#define CHSC6X_LOG(fmt, ...)
#endif

static struct hyn_ts_data *g_chsc6x_data = NULL;

static void chsc6x_i2c_pin_claim(struct i2c_device *i2c_dev)
{
    gpio_driver_strength(MACRO_PIN(PIN_TP_I2C_SCL), GPIO_DS_G3);
    gpio_driver_strength(MACRO_PIN(PIN_TP_I2C_SDA), GPIO_DS_G3);
    gpio_set_mode(MACRO_PIN(PIN_TP_I2C_SCL), GPIO_OPENDRAIN_PULL_UP, GPIO_PULL_LEVEL_4_7K);
    gpio_set_mode(MACRO_PIN(PIN_TP_I2C_SDA), GPIO_OPENDRAIN_PULL_UP, GPIO_PULL_LEVEL_4_7K);

    if (i2c_dev == (struct i2c_device *)dev_get(HG_I2C1_DEVID))
    {
        gpio_iomap_inout(MACRO_PIN(PIN_TP_I2C_SCL),
                         GPIO_IOMAP_IN_SPI1_SCK_IN, GPIO_IOMAP_OUT_SPI1_SCK_OUT);
        gpio_iomap_inout(MACRO_PIN(PIN_TP_I2C_SDA),
                         GPIO_IOMAP_IN_SPI1_IO0_IN, GPIO_IOMAP_OUT_SPI1_IO0_OUT);
    }
    else
    {
        gpio_iomap_inout(MACRO_PIN(PIN_TP_I2C_SCL),
                         GPIO_IOMAP_IN_SPI2_SCK_IN, GPIO_IOMAP_OUT_SPI2_SCK_OUT);
        gpio_iomap_inout(MACRO_PIN(PIN_TP_I2C_SDA),
                         GPIO_IOMAP_IN_SPI2_IO0_IN, GPIO_IOMAP_OUT_SPI2_IO0_OUT);
    }
}

static void chsc6x_rst(void)
{
    uint8_t rst = MACRO_PIN(PIN_TP_RST);

    if (rst == 255)
    {
        return;
    }

    gpio_iomap_output(rst, GPIO_IOMAP_OUTPUT);
    gpio_set_mode(rst, GPIO_PULL_NONE, GPIO_PULL_LEVEL_NONE);
    gpio_set_dir(rst, GPIO_DIR_OUTPUT);
    gpio_driver_strength(MACRO_PIN(PIN_TP_RST), GPIO_DS_G2);

    gpio_set_val(rst, 0);
    os_sleep_ms(20);
    gpio_set_val(rst, 1);
    os_sleep_ms(50);
}

static int chsc6x_read_touchdata(struct hyn_ts_data *p_dev,
                                 uint8_t *buf, uint32_t len)
{
    int ret;

    os_memset(buf, 0, len);

    ret = hyn_wr_reg(p_dev, CHSC6X_TOUCH_DATA_ADDR, 2, buf, CHSC6X_FRAME_SIZE);
    if (ret != RET_OK)
    {
        CHSC6X_LOG("read touchdata failed, ret=%d", ret);
    }

    return ret;
}

static int chsc6x_parse_frame(struct hyn_ts_data *p_dev,
                              const uint8_t *buf, uint32_t len)
{
    uint8_t point_num;
    uint8_t touch_down = 0;
    uint8_t touch_up = 0;
    uint8_t valid_num = 0;
    uint8_t i;

    os_memset(&p_dev->rp_buf, 0, sizeof(p_dev->rp_buf));

    point_num = buf[2];

    if (point_num > CHSC6X_MAX_POINTS)
    {
        p_dev->rp_buf.report_need = REPORT_NONE;
        p_dev->rp_buf.rep_num = 0;
        return RET_OK;
    }

    if (point_num == 0)
    {
        p_dev->rp_buf.rep_num = 0;
        p_dev->rp_buf.report_need = REPORT_POS;
        CHSC6X_LOG("frame: pts=0 up\r\n");
        return RET_OK;
    }

    for (i = 0; i < point_num; i++)
    {
        uint32_t off = CHSC6X_HEADER_SIZE + (i * CHSC6X_POINT_SIZE);
        uint8_t event = buf[off + 0] & 0xF0;
        uint16_t x = ((uint16_t)(buf[off + 0] & 0x0F) << 8) | buf[off + 1];
        uint8_t id = (buf[off + 2] >> 4) & 0x0F;
        uint16_t y = ((uint16_t)(buf[off + 2] & 0x0F) << 8) | buf[off + 3];

        if (valid_num >= MAX_POINT_NUM)
        {
            break;
        }

        if (id == 15)
        {
            return RET_ERR;
        }

        if (event == CHSC6X_EVENT_DOWN || event == CHSC6X_EVENT_CONTACT)
        {
        p_dev->rp_buf.pos_info[valid_num].pos_id = id;
        p_dev->rp_buf.pos_info[valid_num].pos_x = x;
        p_dev->rp_buf.pos_info[valid_num].pos_y = y;
            p_dev->rp_buf.pos_info[valid_num].event = 1;
            touch_down++;
            valid_num++;
        }
        else if (event == CHSC6X_EVENT_UP)
        {
            touch_up++;
        }
    }

    p_dev->rp_buf.rep_num = valid_num;
    p_dev->rp_buf.report_need = (touch_down > 0 || touch_up > 0) ? REPORT_POS : REPORT_NONE;

    if (valid_num > 0)
    {
        CHSC6X_LOG("frame: pts=%d down=%d id0=%d ev0=%d x0=%d y0=%d\r\n",
                   valid_num,
                   touch_down,
                   p_dev->rp_buf.pos_info[0].pos_id,
                   p_dev->rp_buf.pos_info[0].event,
                   p_dev->rp_buf.pos_info[0].pos_x,
                   p_dev->rp_buf.pos_info[0].pos_y);
    }
    else
    {
        CHSC6X_LOG("frame: pts=%d down=%d", valid_num, touch_down);
    }

    return RET_OK;
}

static int chsc6x_report(struct hyn_ts_data *ts_data)
{
    uint8_t buf[CHSC6X_FRAME_SIZE] = {0};
    int ret;

    ret = chsc6x_read_touchdata(ts_data, buf, sizeof(buf));
    if (ret != RET_OK)
    {
        return ret;
    }

    return chsc6x_parse_frame(ts_data, buf, sizeof(buf));
}

static void chsc6x_queue_frame(struct hyn_ts_data *ts_data)
{
    struct ts_frame *frame;
    struct ts_frame *old_frame;

    frame = (struct ts_frame *)ZALLOC(sizeof(struct ts_frame));
    if (frame == NULL)
    {
        HYN_ERROR("alloc frame failed");
        return;
    }

    os_memcpy(frame, &ts_data->rp_buf, sizeof(struct ts_frame));

    if (os_msgq_put(&ts_data->msgque, (uint32)frame, 0) != RET_OK)
    {
        old_frame = (struct ts_frame *)os_msgq_get(&ts_data->msgque, 0);
        if (old_frame != NULL)
        {
            FREE(old_frame);
        }

        if (os_msgq_put(&ts_data->msgque, (uint32)frame, 0) != RET_OK)
        {
            CHSC6X_LOG("msgq full, replace failed pts=%d ev0=%d x0=%d y0=%d",
                       ts_data->rp_buf.rep_num,
                       ts_data->rp_buf.rep_num > 0 ? ts_data->rp_buf.pos_info[0].event : 0,
                       ts_data->rp_buf.rep_num > 0 ? ts_data->rp_buf.pos_info[0].pos_x : 0,
                       ts_data->rp_buf.rep_num > 0 ? ts_data->rp_buf.pos_info[0].pos_y : 0);
            FREE(frame);
        }
    }
}

static void chsc6x_int_handler(int32 id, enum gpio_irq_event event,
                               uint32 param1, uint32 param2)
{
    struct hyn_ts_data *ts_data = (struct hyn_ts_data *)id;

    (void)event;
    (void)param1;
    (void)param2;

    if (ts_data != NULL)
    {
        os_event_set(&ts_data->event, EVENT_TOUCH_INT, 0);
    }
}

static int32 chsc6x_int_init(struct hyn_ts_data *ts_data)
{
    uint8_t int_pin = MACRO_PIN(PIN_TP_INT);
    int32 ret;

    if (int_pin == 255)
    {
        return RET_OK;
    }

    gpio_iomap_input(int_pin, GPIO_IOMAP_INPUT);
    gpio_set_mode(int_pin, GPIO_PULL_UP, GPIO_PULL_LEVEL_4_7K);

    ret = gpio_request_pin_irq(int_pin,
                               chsc6x_int_handler,
                               (uint32)ts_data,
                               GPIO_IRQ_EVENT_FALL);
    if (ret != RET_OK)
    {
        HYN_ERROR("request irq failed, ret=%d", ret);
        return ret;
    }

    CHSC6X_LOG("int init done, pin=%d", int_pin);
    return RET_OK;
}

static int32 chsc6x_work(struct os_work *work)
{
    struct hyn_ts_data *ts_data = (struct hyn_ts_data *)work;
    uint32_t event = 0;

    if (os_event_wait(&ts_data->event,
                      EVENT_TOUCH_INT,
                      &event,
                      OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR,
                      0) == RET_OK)
    {
        if (chsc6x_report(ts_data) == RET_OK &&
            ts_data->rp_buf.report_need != REPORT_NONE)
        {
            chsc6x_queue_frame(ts_data);
        }
    }

    os_run_work_delay(work, 10);
    return RET_OK;
}

/**
 * @brief 获取多点触摸坐标数据
 * 
 * @return touch_multipoint_pos_t* 触摸数据指针，point_num=0 表示手指抬起
 */
void *chsc6x_get_multipoint_xy(void *user_data)
{
    struct ts_frame *frame;
    struct hyn_ts_data *ts_data = (struct hyn_ts_data *)user_data;
    struct hyn_plat_data *dt;
    touch_multipoint_pos_t *data;
    uint8_t valid_count = 0;
    int i;

    if (ts_data == NULL)
    {
        return NULL;
    }

    frame = (struct ts_frame *)os_msgq_get(&ts_data->msgque, 0);
    if (frame == NULL)
    {
        return NULL;
    }

    data = (touch_multipoint_pos_t *)ZALLOC(sizeof(touch_multipoint_pos_t));
    if (data == NULL)
    {
        FREE(frame);
        return NULL;
    }

    if (frame->rep_num == 0)
    {
        data->point_num = 0;
        FREE(frame);
        return data;
    }

    dt = &ts_data->plat_data;

    for (i = 0; i < frame->rep_num && i < MAX_POINT_NUM; i++)
    {
        uint16_t xpos = frame->pos_info[i].pos_x;
        uint16_t ypos = frame->pos_info[i].pos_y;

        if (frame->pos_info[i].pos_id >= MAX_POINT_NUM)
        {
            continue;
        }

        if (dt->swap_xy)
        {
            uint16_t tmp = xpos;
            xpos = ypos;
            ypos = tmp;
        }

        if (dt->reverse_x)
        {
            xpos = (dt->swap_xy ? dt->y_resolution : dt->x_resolution) - xpos;
        }

        if (dt->reverse_y)
        {
            ypos = (dt->swap_xy ? dt->x_resolution : dt->y_resolution) - ypos;
        }

        data->pos_x[valid_count] = xpos;
        data->pos_y[valid_count] = ypos;
        valid_count++;
    }

    data->point_num = valid_count;

    FREE(frame);
    return data;
}

uint32_t chsc6x_free_multipoint_xy(void *user_data, void *data)
{
    if (data != NULL)
    {
        FREE(data);
        return RET_OK;
    }
    return RET_ERR;
}

int chsc6x_suspend(void)
{
    uint8_t val = 0x03;

    if (g_chsc6x_data == NULL)
    {
        return RET_ERR;
    }

    hyn_irq_set(g_chsc6x_data, 0);
    return hyn_wr_reg(g_chsc6x_data, 0xA5, 1, &val, 0);
}

int chsc6x_resume(void)
{
    if (g_chsc6x_data == NULL)
    {
        return RET_ERR;
    }

    chsc6x_rst();
    hyn_irq_set(g_chsc6x_data, 1);

    os_sleep_ms(50);

    return RET_OK;
}

int32_t chsc6x_init(void **user_data)
{
    struct hyn_ts_data *p_dev;
    struct i2c_device *iic_dev;
    int app_iic_num;
    int32 ret;

    if (user_data == NULL || *user_data != NULL || g_chsc6x_data != NULL)
    {
        return -EINVAL;
    }

    p_dev = (struct hyn_ts_data *)ZALLOC(sizeof(struct hyn_ts_data));
    if (p_dev == NULL)
    {
        HYN_ERROR("alloc device failed");
        return -ENOMEM;
    }

    p_dev->iic_trx_buff = (uint8_t *)ZALLOC(32);
    if (p_dev->iic_trx_buff == NULL)
    {
        FREE(p_dev);
        HYN_ERROR("alloc trx buf failed");
        return -ENOMEM;
    }

    chsc6x_rst();

    iic_dev = (struct i2c_device *)dev_get(TP_I2C_BUS);
    if (iic_dev == NULL)
    {
        HYN_ERROR("get i2c dev failed");
        ret = -ENODEV;
        goto err_free;
    }

    chsc6x_i2c_pin_claim(iic_dev);

    app_iic_num = register_iic_queue(iic_dev,
                                     MACRO_PIN(PIN_TP_I2C_SCL),
                                     MACRO_PIN(PIN_TP_I2C_SDA),
                                     0);
    if (app_iic_num <= 0)
    {
        HYN_ERROR("register_iic_queue failed");
        ret = RET_ERR;
        goto err_free;
    }

    p_dev->app_iic_num = app_iic_num;
    p_dev->iic_addr = CHSC6X_I2C_ADDR;
    p_dev->plat_data.swap_xy = SWAP_XY;
    p_dev->plat_data.reverse_x = REVERSE_X;
    p_dev->plat_data.reverse_y = REVERSE_Y;
    p_dev->plat_data.x_resolution = X_RESOLUTION;
    p_dev->plat_data.y_resolution = Y_RESOLUTION;

    if (os_event_init(&p_dev->event) != RET_OK)
    {
        HYN_ERROR("event init failed");
        ret = RET_ERR;
        goto err_unregister_iic;
    }

    if (os_msgq_init(&p_dev->msgque, 10) != RET_OK)
    {
        os_event_del(&p_dev->event);
        HYN_ERROR("msgq init failed");
        ret = RET_ERR;
        goto err_unregister_iic;
    }

    ret = chsc6x_int_init(p_dev);
    if (ret != RET_OK)
    {
        goto err_delete_msgq;
    }

    ret = OS_WORK_INIT(&p_dev->work, chsc6x_work, 0);
    if (ret != RET_OK)
    {
        goto err_release_irq;
    }

    ret = os_run_work_delay(&p_dev->work, 30);
    if (ret != RET_OK)
    {
        goto err_release_irq;
    }

    g_chsc6x_data = p_dev;
    *user_data = p_dev;

    CHSC6X_LOG("init ok, addr=0x%02X, res=%dx%d, swap=%d rx=%d ry=%d",
               p_dev->iic_addr,
               p_dev->plat_data.x_resolution,
               p_dev->plat_data.y_resolution,
               p_dev->plat_data.swap_xy,
               p_dev->plat_data.reverse_x,
               p_dev->plat_data.reverse_y);
    return RET_OK;

err_release_irq:
    if (MACRO_PIN(PIN_TP_INT) != 255)
    {
        gpio_release_pin_irq(MACRO_PIN(PIN_TP_INT), GPIO_IRQ_EVENT_FALL);
    }
err_delete_msgq:
    os_msgq_del(&p_dev->msgque);
    os_event_del(&p_dev->event);
err_unregister_iic:
    unregister_iic_queue(p_dev->app_iic_num);
err_free:
    FREE(p_dev->iic_trx_buff);
    FREE(p_dev);
    return ret;
}

int32_t chsc6x_deinit(void *user_data)
{
    struct hyn_ts_data *p_dev = (struct hyn_ts_data *)user_data;
    struct ts_frame *frame;
    int32 ret = RET_OK;

    if (p_dev == NULL || p_dev != g_chsc6x_data)
    {
        return -EINVAL;
    }

    if (MACRO_PIN(PIN_TP_INT) != 255 &&
        gpio_release_pin_irq(MACRO_PIN(PIN_TP_INT), GPIO_IRQ_EVENT_FALL) != RET_OK)
    {
        return RET_ERR;
    }

    g_chsc6x_data = NULL;
    os_work_cancle2(&p_dev->work, 1);

    while ((frame = (struct ts_frame *)os_msgq_get(&p_dev->msgque, 0)) != NULL)
    {
        FREE(frame);
    }

    if (os_msgq_del(&p_dev->msgque) != RET_OK)
    {
        ret = RET_ERR;
    }
    if (os_event_del(&p_dev->event) != RET_OK)
    {
        ret = RET_ERR;
    }
    if (p_dev->app_iic_num > 0 &&
        unregister_iic_queue(p_dev->app_iic_num) < 0)
    {
        ret = RET_ERR;
    }

    FREE(p_dev->iic_trx_buff);
    FREE(p_dev);
    return ret;
}

const struct touch_chip_hardware_ops chsc6x_ops =
{
    .free_multipoint_xy     = chsc6x_free_multipoint_xy,
    .get_multipoint_xy      = chsc6x_get_multipoint_xy,
    .touch_chip_init        = chsc6x_init,
    .touch_chip_deinit      = chsc6x_deinit,
};
