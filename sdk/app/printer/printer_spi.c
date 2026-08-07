#include "basic_include.h"
#include "hal/dvp.h"
#include "hal/vpp.h"
#include "hal/adc.h"
#include "hal/spi.h"
#include "hal/pwm.h"
#include "hal/timer_device.h"
#include "gpio/hggpio_v4_hw.h"

#if PRINTER_EN
/* 
config.cfg 文件引脚定义 

//打印机的CLK、DATA引脚
PIN_SPI1_CLK 	= PB_11
PIN_SPI1_IO0 	= PB_13

//步进电机引脚
PIN_SPI2_IO0 	= PB_14
PIN_SPI2_IO1 	= PB_15
PIN_SPI2_IO2 	= PC_1
PIN_SPI2_IO3 	= PC_0

*/
#define GPIOA                   ((struct hggpio_v4_hw       *) GPIOA_BASE)
#define GPIOB                   ((struct hggpio_v4_hw       *) GPIOB_BASE)
#define GPIOC                   ((struct hggpio_v4_hw       *) GPIOC_BASE)
#define GPIOD                   ((struct hggpio_v4_hw       *) GPIOD_BASE)

/*==================================打印机引脚定义==================================*/
#define  MOTO_SLEEP_PIN	     	PB_7		    // LOW sleep , HIGH run

#define  PRINTER_DATA_PIN	 	PB_13
#define  PRINTER_CLK_PIN	 	PB_11
#define  PRINTER_LATCH_PIN	 	PB_8
#define  PRINTER_TM_PIN	 	 	PC_5
#define  PRINTER_PSENSOR_PIN 	PA_0
#define  PRINTER_STB_PIN	 	PB_10
#define  PRINTER_EN_PIN	     	PA_7

#define  PRINTER_DATA_PORT   	GPIOB
#define  PRINTER_DATA_NUM    	13
#define  PRINTER_CLK_PORT    	GPIOB
#define  PRINTER_CLK_NUM     	11
#define  PRINTER_STB_PORT    	GPIOB
#define  PRINTER_STB_NUM     	10

#define PRINTER_W            	384
#define PRINTER_H            	512

// HEAT_BYTES_PER_LINE 表示每行用于控制加热单元的字节数（48 字节 * 8 位 = 384 点）
#define HEAT_BYTES_PER_LINE  	48

/*====================================第一组 PWM====================================*/

#define SPI_SEND_EN 		 	1			   //使用SPI时序发送数据，可控发送速率
#define STB_PIN_PWM_EN 		 	0              //STB 使用PWM波形，使能置1否则置0

#define SPI_MOTO_EN          	1
#define SPI_MOTO_CLK         	5000           //4000:12ms 5000:9.6ms     
#define TEST_MOTO_BUF_COUNT  	6              
#define HEAT_TIMES_THRESHOLD 	128

#define TOTAL_TICK           	258            //打印每行调用printer_heat_transfer_data函数的次数

#define SPI_CLK_FREQ		 	5300000		   //SPI时钟频率8M ，CLK脚高电平稳定时间约为50-60ns, 调整建议不大于8M，频率越高加热时间越短
#define STB_PIN_PWM_PERIOD 	 	1000           
#define STB_PIN_PWM_DUTY 	 	900            //改变PWM的占空比可以改变加热时长

#define PRINTER_DITHERING_EN	0			   //半色调散点特效
#define PRINTER_TEST_PATTER_EN	1			   //打印机测试模式

#if STB_PIN_PWM_EN
static struct hgpwm_v0 *moto_hgpwm   = NULL;
#endif
static struct spi_device *spi0_dev   = NULL;
static struct spi_device *spi1_dev   = NULL;
static uint32_t current_heat_count   = 0;
static uint8_t* halfstep_buf 	     = NULL;
static uint8_t* halfstep_buf_cache   = NULL;
static uint8_t* printer_buf 		 = NULL;
static uint8_t* printer_buf_odd 	 = NULL;
static uint8_t* printer_buf_even 	 = NULL;
static volatile uint8_t last_ret     = 0;
static uint8_t pre_heat_flag 	     = 0;
static volatile uint8_t printer_done = 1;

volatile uint8  printer_action 		 = 0;
volatile uint32_t printer_h 		 = 0;

const unsigned char *printer_picture_addr;

static uint32_t printer_heat_transfer_data();

extern const unsigned char printer_demo[PRINTER_W * PRINTER_H];                                 //384 x 512 只包含Y数据的图片

static void printer_moto_delay(uint32_t n)
{
	volatile uint32_t i=500*n;
	while(i--)
	{
	}
}

static void printer_delay(uint32_t n)
{
	volatile uint32_t i=20*n;
	while(i--)
	{
	}
}

void printer_moto_enable(uint8_t en)
{
	if(en)
	{
		gpio_set_val(MOTO_SLEEP_PIN,1);
		printer_moto_delay(20);
	}
	else
	{
		gpio_set_val(MOTO_SLEEP_PIN,0);
	}
}

int32_t get_printer_tm(){
	uint32 vol = 0;
	struct hgadc_v0* hgadc_tm = NULL;
	printer_delay(100);
	hgadc_tm = (struct hgadc_v0*)dev_get(HG_ADC0_DEVID);
	adc_open((struct adc_device *)(hgadc_tm));	
	adc_add_channel((struct adc_device *)hgadc_tm, PRINTER_TM_PIN);
	adc_get_value((struct adc_device *)hgadc_tm, PRINTER_TM_PIN, &vol);
	os_printf("temperature:%d\r\n",vol);
	return vol;
}

uint8_t is_paper_insert(){
	return gpio_get_val(PRINTER_PSENSOR_PIN);
}

/*
半步驱动-低位在前
(1) A1:1 A2:1 B1:1 B2:0     0xEE
(2) A1:1 A2:0 B1:1 B2:0     0xAA
(3) A1:1 A2:0 B1:1 B2:1     0xBB
(4) A1:1 A2:0 B1:0 B2:1     0x99
(5) A1:1 A2:1 B1:0 B2:1     0xDD
(6) A1:0 A2:1 B1:0 B2:1     0x55
(7) A1:0 A2:1 B1:1 B2:1     0x77
(8) A1:0 A2:1 B1:1 B2:0     0x66
*/

void printer_get_four_halfstep(uint8 *halfstep_buf, uint32_t n)
{
	static const uint8_t halfstep_table[8] = {0xEE, 0xAA, 0xBB, 0x99, 0xDD, 0x55, 0x77, 0x66};
    static uint8_t current_step = 0;

    int i = 0, j = 0;
    int k = n / 4;

    //调试打印会影响打印时间
    // os_printf("halfstep_buf: "); 

    for (i = 0; i < 4; i++)
    {
        uint8_t index = (i + current_step) % 8;
        for (j = 0; j < k; j++)
        {
            halfstep_buf[i*k+j] = halfstep_table[index];
        }
        // printf("%02X ", halfstep_buf[index]);
    }

    //最后1byte为0,用于hold住spi数据线为0
    halfstep_buf[n] = 0;
    // printf("%02X \n", halfstep_buf[n]);
    current_step = (current_step + 4) % 8;
}

//中断处理的效率会影响加热时间
static void spi0_irq(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2)
{
    uint8_t *halfstep_buf = (uint8_t *)irq_data;

    current_heat_count ++;

    gpio_set_val(PRINTER_LATCH_PIN,0);
    gpio_set_val(PRINTER_LATCH_PIN,1);

    switch(last_ret)
    {
        //一帧图片打印结束
        case 0:
            #if STB_PIN_PWM_EN
            pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY,STB_PIN_PWM_PERIOD,0);
            #else
            gpio_set_val(PRINTER_STB_PIN,0);    //关加热
            #endif
            os_printf("kick moto spi done\n");
            printer_get_four_halfstep(halfstep_buf, TEST_MOTO_BUF_COUNT*4);
            printer_get_four_halfstep(halfstep_buf_cache, TEST_MOTO_BUF_COUNT*4);
            spi_ioctl((struct spi_device*)spi1_dev, SPI_KICK_DMA_TX, (uint32)halfstep_buf, TEST_MOTO_BUF_COUNT*4+1);
            spi_release_irq(spi0_dev,SPI_IRQ_FLAG_TX_DONE);
            printer_done = 2;

        break;

        //加热期间spi传送数据
        case 1:
            last_ret = printer_heat_transfer_data();
        break;

        //一行打印结束，电机走四步
        case 2:
            #if STB_PIN_PWM_EN
            pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY,STB_PIN_PWM_PERIOD,0);
            #else
            gpio_set_val(PRINTER_STB_PIN,0);    //关加热
            #endif
            printer_get_four_halfstep(halfstep_buf, TEST_MOTO_BUF_COUNT*4);
            spi_ioctl((struct spi_device*)spi1_dev, SPI_KICK_DMA_TX, (uint32)halfstep_buf, TEST_MOTO_BUF_COUNT*4+1);
            spi_release_irq(spi0_dev,SPI_IRQ_FLAG_TX_DONE);
        break;

        default:
        break;
    };


    if(current_heat_count >= HEAT_TIMES_THRESHOLD)
    {
        current_heat_count = 0;
    }

    
}

static void spi1_irq(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2)
{
    static uint32_t moto_cycle_count = 250;

    if (printer_done == 2) {
        //加热打印结束，电机送纸部分
        if(moto_cycle_count == 0) {
            printer_done = 1;
            moto_cycle_count = 250;
            spi_close(spi1_dev);
        } else {
            if(moto_cycle_count % 2 == 0)
            {
                spi_ioctl((struct spi_device*)spi1_dev, SPI_KICK_DMA_TX, (uint32)halfstep_buf_cache, TEST_MOTO_BUF_COUNT*4);
            }
            else
            {
                spi_ioctl((struct spi_device*)spi1_dev, SPI_KICK_DMA_TX, (uint32)halfstep_buf, TEST_MOTO_BUF_COUNT*4);
            }
            moto_cycle_count--;
        }
    } else {
        //行打印结束，电机走一行
        #if STB_PIN_PWM_EN
		pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY,STB_PIN_PWM_PERIOD,STB_PIN_PWM_DUTY);
        #else
		gpio_set_val(PRINTER_STB_PIN,1);	//加热
        #endif
        spi_request_irq(spi0_dev, SPI_IRQ_FLAG_TX_DONE, spi0_irq, (uint32)halfstep_buf);
        last_ret = printer_heat_transfer_data();
    }
}

static void stb_timer_cb(uint32 cb_data, uint32 irq_flag)
{
    //预加热结束
    os_printf("stb timer callback\n");
    #if STB_PIN_PWM_EN
    pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY,STB_PIN_PWM_PERIOD,0);
    #else
    gpio_set_val(PRINTER_STB_PIN,0);    //关加热
    #endif
    pre_heat_flag = 1;
}

static void* printer_stb_timer_open()
{
    struct timer_device *stb_timer0 = (struct timer_device *)dev_get(HG_TIMER0_DEVID);
    timer_device_open(stb_timer0, TIMER_TYPE_ONCE, 0);
    os_printf("STB timer open\n");
    return stb_timer0;
}

static void printer_stb_timer_close(void* stb_timer0)
{
    timer_device_close((struct timer_device *)stb_timer0);
}

static void printer_stb_start(void* stb_timer0, uint32_t ms)
{
    #if STB_PIN_PWM_EN
		pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY,STB_PIN_PWM_PERIOD,STB_PIN_PWM_DUTY);
    #else
        gpio_set_val(PRINTER_STB_PIN,1);	//加热
    #endif
	uint32_t sys_clk = system_clock_get();
    timer_device_start((struct timer_device *)stb_timer0, ms*(sys_clk/1000), stb_timer_cb, 0);   // cnt = ms * (240M/1000)
}

static void paper_io_init()
{

	gpio_iomap_output(MOTO_SLEEP_PIN,GPIO_IOMAP_OUTPUT);

	gpio_iomap_output(PRINTER_LATCH_PIN,GPIO_IOMAP_OUTPUT);
	gpio_set_val(PRINTER_LATCH_PIN,1);

	gpio_iomap_output(PRINTER_STB_PIN,GPIO_IOMAP_OUTPUT);
	gpio_iomap_output(PRINTER_EN_PIN,GPIO_IOMAP_OUTPUT);
	
	gpio_iomap_output(PRINTER_TM_PIN,GPIO_IOMAP_OUTPUT);

	gpio_set_mode(PRINTER_PSENSOR_PIN, GPIO_PULL_NONE, GPIO_PULL_LEVEL_NONE);
	gpio_iomap_input(PRINTER_PSENSOR_PIN,GPIO_IOMAP_INPUT);

    //打印机数据输出，spi0 一线模式
	spi0_dev = (struct spi_device *)dev_get(HG_SPI1_DEVID);
	spi_ioctl(spi0_dev,SPI_CFG_SET_NONE_CS,1,0);
	spi_ioctl(spi0_dev,SPI_SET_LSB_FIRST,0,0);
	spi_ioctl(spi0_dev,SPI_KICK_DMA_EN,0,0);
	spi_open(spi0_dev,SPI_CLK_FREQ,SPI_MASTER_MODE,SPI_WIRE_SINGLE_MODE,SPI_CPOL_0_CPHA_0);
    spi_release_irq((struct spi_device*)spi0_dev,SPI_IRQ_FLAG_TX_DONE | SPI_IRQ_FLAG_RX_DONE);

    //步进电机时序，spi1 四线模式
    spi1_dev = (struct spi_device *)dev_get(HG_SPI2_DEVID);
    spi_ioctl(spi1_dev,SPI_CFG_SET_NONE_CS,1,0);
    spi_ioctl(spi1_dev,SPI_SET_LSB_FIRST,1,0);
    spi_open(spi1_dev,SPI_MOTO_CLK,SPI_MASTER_MODE,SPI_WIRE_QUAD_MODE,SPI_CPOL_0_CPHA_1);
    spi_request_irq((struct spi_device *)spi1_dev, SPI_IRQ_FLAG_TX_DONE, spi1_irq, (uint32)halfstep_buf);

#if STB_PIN_PWM_EN
    //STB加热采用PWM输出
    moto_hgpwm    = (struct hgpwm_v0 *)dev_get(HG_PWM0_DEVID);
	pwm_init((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_4, STB_PIN_PWM_PERIOD, 0);
	pwm_start((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_4);
#endif

}

static void paper_io_deinit()
{
	struct hgadc_v0* hgadc_tm = NULL;
	hgadc_tm = (struct hgadc_v0*)dev_get(HG_ADC0_DEVID);
	adc_close((struct adc_device *)(hgadc_tm));
	adc_delete_channel((struct adc_device *)(hgadc_tm),PRINTER_TM_PIN);

#if STB_PIN_PWM_EN
	pwm_deinit((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4);
#endif

	spi_close(spi0_dev);
    spi_close(spi1_dev);
}

static uint32_t printer_heat_transfer_data()
{
	#define HOTTIME  1500
	uint32_t itk = 0;
	uint32_t jtk = 0;
	static uint32_t i = 0;
	static uint32_t j = 0;
    static uint32_t tick = 0;
	static uint32_t hot_time = 0;
	static uint8_t half_s = 0;
	static uint8_t hot_add = 0;
	uint32 offset_h;
	uint8_t *pt;
	pt = printer_buf;
	if (printer_h == PRINTER_H) {
		tick = 2;
	}
	offset_h = (printer_h-1)*PRINTER_W;

	if ((tick%2) == 0) {
#if STB_PIN_PWM_EN
		pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, STB_PIN_PWM_PERIOD, STB_PIN_PWM_DUTY);	//加热
#else
		PRINTER_STB_PORT->ODAT |= BIT(PRINTER_STB_NUM);		//加热
#endif

		if (hot_time > HOTTIME)
			goto deal_end;

		if (half_s) {

            //偶数点
			spi_ioctl((struct spi_device*)spi0_dev, SPI_KICK_DMA_TX, (uint32)printer_buf_even, HEAT_BYTES_PER_LINE);

            os_memset(printer_buf_odd , 0, HEAT_BYTES_PER_LINE);
			for(jtk = 0; jtk < HEAT_BYTES_PER_LINE; jtk++)
			{
				for(itk = 1 + i; itk < 8; itk+=2)
				{
					if ((pt[offset_h+jtk*8+itk]) > 0 && (1)) {
						pt[offset_h+jtk*8+itk]--;
						printer_buf_odd[jtk] |= (0x1 << (7-itk));
					}
				}
			}

		} else {

			//奇数点
			spi_ioctl((struct spi_device*)spi0_dev, SPI_KICK_DMA_TX, (uint32)printer_buf_odd, HEAT_BYTES_PER_LINE);

            os_memset(printer_buf_even, 0, HEAT_BYTES_PER_LINE);
			for(jtk = 0; jtk < HEAT_BYTES_PER_LINE; jtk++)
			{
				for(itk = 0 + j; itk < 8; itk+=2)
				{
					if ((pt[offset_h+jtk*8+itk]) > 0 && (1)) {
						pt[offset_h+jtk*8+itk]--;
						printer_buf_even[jtk] |= (0x1 << (7-itk));
					}
				
				}
			}
		}

        half_s ^= BIT(0);
		hot_add++;
	}

deal_end:
	if ((tick % TOTAL_TICK) == 2) {
		printer_h--;        
		hot_time = 0;
		if (printer_h == 0) {
			return 0;
		} else {
            tick+=2;
	        hot_time++;
            return 2;
        }
	}
	tick+=2;
	hot_time++;
	return 1;
}

/**
 * @brief 设置打印机的图片地址，赋值给全局变量printer_picture_addr
 * 
 * @param addr 传入的图片地址
 * @return true 
 * @return false 
 */
bool printer_set_picture_addr(const unsigned char* addr)
{
	if(addr == NULL) {
		os_printf("printer demo addr invaild!!! addr:%x\n",addr);
		return 1;
	}

	printer_picture_addr = addr;
	
	return 0;
}

/**
 * @brief 打印机线程使能
 * 
 * @return true 
 */
bool printer_kick_start()
{
	printer_action = 1;
	return 0;
}

/**
 * @brief 切换打印机与摄像头的使能
 * 
 * @param printer_en 如果使能打印机、关闭摄像头则置1，否则置0
 */
void printer_dvp_en_exchange(int printer_en)
{

#if DVP_EN
	struct dvp_device *dvp_device = (struct dvp_device*)dev_get(HG_DVP_DEVID);
	struct vpp_device *vpp_device = (struct vpp_device*)dev_get(HG_VPP_DEVID);
#endif

	os_printf("%s %d\r\n",__FUNCTION__,printer_en);

	//打印机使能
	if(printer_en)
	{
		gpio_set_val(PRINTER_EN_PIN,1);

#if DVP_EN
		gpio_iomap_output(PIN_DVP_PDN,GPIO_IOMAP_OUTPUT);
		gpio_set_val(PIN_DVP_PDN,1);
		dvp_close(dvp_device);
		vpp_close(vpp_device);
#endif

	}
	//DVP使能
	else {
		gpio_set_val(PRINTER_EN_PIN, 0);

#if DVP_EN
		bool csi_ret;
		bool csi_cfg();
		bool csi_open();

		gpio_iomap_output(PIN_DVP_PDN,GPIO_IOMAP_OUTPUT);
		gpio_set_val(PIN_DVP_PDN, 0);
#if SDH_I2C2_REUSE
		uint32 sdhost_i2c2_exchange(int sdh_stop_en);
		sdhost_i2c2_exchange(1);
#endif
		csi_ret = csi_cfg();
		if(csi_ret)
			csi_open();
		vpp_open(vpp_device);
#if SDH_I2C2_REUSE
		sdhost_i2c2_exchange(0);
#endif

#endif
	}
}

static unsigned char linearInterpolation(unsigned char* pline1, unsigned char* pline2, int channels,int u, int v)
{
	int pm3 = u * v;
	int pm2 = (u << 8) - pm3;
	int pm1 = (v << 8) - pm3;
	int pm0 = (1 << 16) - pm1 - pm2 - pm3;

    unsigned char result = (unsigned char)(((pm0*pline1[0] + pm2 * pline1[channels] + pm1 * pline2[0] + pm3* pline2[channels]) >> 16)&0xff);
	
	return result;
}

void zoomImg(unsigned char *psrc, int sw, int sh, int channels, unsigned char *pdst, int dw, int dh)
{
	if (!psrc || !pdst) return;
	if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) return;
	if (channels != 1 && channels != 3) return;

	unsigned char *p = psrc,*q = pdst,*pt;
	int srcBytesPerLine = sw * channels,dstBytesPerLine = dw * channels;
	int srcy_16 = 0, v_8 = 0, srcx_16 = 0;
	int stepw = (((sw - 1) << 16) / dw +1);
	int steph = ((sh - 1) << 16) / dh + 1;
	//int stepc = ((channels - 1) << 16)/channels + 1;
	int i, j, m;

	srcy_16 = 0;
	for (j = 0; j < dh; j++)
	{
		v_8 = (srcy_16 & 0xFFFF) >> 8;
		p = psrc + srcBytesPerLine * (srcy_16 >> 16);
		srcx_16 = 0;
		for (i = 0; i < dstBytesPerLine; i+=channels)
		{
			pt = &p[(srcx_16>>16)*channels];

			for (m = 0; m < channels; m++) {
				q[i+m] = linearInterpolation(&pt[m], &pt[m] + srcBytesPerLine, channels, (srcx_16 & 0xFFFF) >> 8, v_8);
			}

			srcx_16 += stepw;
		}
		srcy_16 += steph;
	
		q += dstBytesPerLine;
	}
}

void vga_to_paper_size(uint8_t *src, uint8_t* des)
{
	uint16_t x=0,y=0;
	zoomImg(src, 640, 480, 1, des, PRINTER_H, PRINTER_W);
	
	for(y=0; y<PRINTER_W; y++)
	{
		for(x=0; x<PRINTER_H; x++)
		{
			src[(x+1)*PRINTER_W-1-y] = des[x+y*PRINTER_H];//90°
		}
	}
}

void enhanceContrast(uint8_t *image) 
{
    int minLevel = 63; 
    int maxLevel = 0;  
    
    for (int i = 0; i < PRINTER_W * PRINTER_H; i++) {
        if (image[i] < minLevel) {
            minLevel = image[i];
        }
        if (image[i] > maxLevel) {
            maxLevel = image[i];
        }
    }
    
    int dynamicRange = maxLevel - minLevel;
    
    for (int i = 0; i < PRINTER_W * PRINTER_H; i++) {
        image[i] = (image[i] - minLevel) * 63 / dynamicRange;

        if (image[i] < 0) {
            image[i] = 0;
        }
        if (image[i] > 63) {
            image[i] = 63;
        }
    }
}

unsigned char restrain_int(int input) 
{
    if (input < 0x00) {
        return 0x00;
    }

    if (input > 0xFF) {
        return 0xFF;
    }

    return input;
}

//Floyd-Steinberg算法,该函数只适用于256灰阶图像
void dithering(uint8_t *image, int width, int height) 
{
    // 对每个像素执行误差扩散
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            int index = y * width + x;
            unsigned char oldPixel = image[index];
            unsigned char newPixel = (oldPixel < 128) ? 0 : 255; // 二值化

            // 计算误差
            int error = oldPixel - newPixel;

            // 更新当前像素
            image[index] = newPixel;

            // 误差扩散到相邻像素
            if(x < width - 1) {
                image[index + 1] = restrain_int(image[index + 1] + (error * 7 / 16));
            }
            if(y < height - 1) {
                image[index + width] = restrain_int(image[index + width] + (error * 5 / 16));
            }
            if(x < width - 1 && y < height - 1) {
                image[index + width + 1] = restrain_int(image[index + width + 1] + (error * 1 / 16));
            }
            if(x > 0 && y < height - 1) {
                image[index + width - 1] = restrain_int(image[index + width - 1] + (error * 3 / 16));
            }

        }
    }
}

static void printer_work_task()
{	
	uint32_t itk = 0;
	uint32_t ktk = 0;
	uint32_t jtk = 0;
	int32_t tm;
	uint32_t grep_step = 0;

    halfstep_buf = (uint8_t*)os_malloc((TEST_MOTO_BUF_COUNT*4+1));

    if(halfstep_buf == NULL)
    {
        os_printf("malloc halfstep_buf fail, printer thread end\r\n");
        goto __exit;
    }

    halfstep_buf_cache = (uint8_t*)os_malloc((TEST_MOTO_BUF_COUNT*4+1));

    if(halfstep_buf_cache == NULL)
    {
        os_printf("malloc halfstep_buf_cache fail, printer thread end\r\n");
        os_free(halfstep_buf);
        halfstep_buf = NULL;
        goto __exit;
    }    

	printer_buf = (uint8_t*)os_malloc_psram(PRINTER_W * PRINTER_H);

	if(printer_buf == NULL)
	{
		os_printf("malloc printer_buf fail, printer thread end\r\n");
		os_free_psram(printer_buf);
		printer_buf = NULL;
		goto __exit;
	}

	printer_buf_odd = (uint8_t*)os_malloc(HEAT_BYTES_PER_LINE);

	if(printer_buf_odd == NULL)
    {
        os_printf("malloc printer_buf_odd fail, printer thread end\r\n");
        os_free(printer_buf_odd);
        printer_buf_odd = NULL;
        goto __exit;
    }  

	printer_buf_even = (uint8_t*)os_malloc(HEAT_BYTES_PER_LINE);

	if(printer_buf_even == NULL)
    {
        os_printf("malloc printer_buf_even fail, printer thread end\r\n");
        os_free(printer_buf_even);
        printer_buf_even = NULL;
        goto __exit;
    }  

	while(1){
		//开始打印前，建议先关闭比spi优先级更高的中断(LCD、USB、UART)，避免其他中断占用时间长影响打印效果
		if (printer_action == 0 || printer_done == 0) {
			//printf("-->Printer0 wait!\r\n");
			os_sleep_ms(50);
			continue;
		}

		printer_action 		= 0;
        printer_done 		= 0;
        pre_heat_flag 		= 0;

		printer_h 			= PRINTER_H;
		os_printf("-->Printer0 open!\r\n");
		paper_io_init();												//打印机IO初始化
		//gpio_set_val(PRINTER_EN_PIN,1);								//enable printer power
		printer_dvp_en_exchange(1);
		printer_delay(500); 
		os_printf("-->Printer0 open  finish!\r\n");
		if(is_paper_insert())											// check paper
		{
			//gpio_set_val(PRINTER_EN_PIN,0);							//disable printer power
			paper_io_deinit();
			printer_dvp_en_exchange(0);
			os_printf("-->Printer error...no paper!!!\r\n");
			continue;
		}
		os_printf("%s %d\n",__FUNCTION__,__LINE__);
		tm = get_printer_tm();											//check tm
		if(1 == tm) // hot
		{
			//gpio_set_val(PRINTER_EN_PIN,0);							//disable printer power
			paper_io_deinit();
			printer_dvp_en_exchange(0);
			os_printf("-->Get the TM error...\r\n");
			continue;
		}
	
		os_printf("%s %d\n",__FUNCTION__,__LINE__);
		os_memset(printer_buf, 0, PRINTER_W * PRINTER_H);				//384 x 512
#if PRINTER_DITHERING_EN
		dithering(vga_room, PRINTER_W, PRINTER_H);						//半色调散点特效
#endif
#if PRINTER_TEST_PATTER_EN
		printer_set_picture_addr(printer_demo);							//打印图片 						
#else
		printer_set_picture_addr(vga_room);
#endif
		for(itk = 0; itk < PRINTER_H; itk++) {								//384 x 512
			for(ktk = 0; ktk < PRINTER_W; ktk++) {
				grep_step = 0;
				for(jtk = 0; jtk < 64; jtk++) {								//配置打印图片量化系数
					if(printer_picture_addr[itk*PRINTER_W+ktk] < (grep_step*4)/*255*/) { //64 4
						printer_buf[itk*PRINTER_W+(PRINTER_W-1)-ktk]++;
					}
					grep_step++;
				}
			}	
		}

		enhanceContrast(printer_buf);
		os_printf("%s %d\n",__FUNCTION__,__LINE__);
	
		os_memset(printer_buf_even, 0, HEAT_BYTES_PER_LINE);
		os_memset(printer_buf_odd, 0, HEAT_BYTES_PER_LINE);
	
		for(jtk = 0; jtk < HEAT_BYTES_PER_LINE; jtk++)
		{
			for(itk = 1; itk < 8; itk+=2)
			{
				if ((printer_buf[jtk*8+itk]) > 0 && (1)) {
					printer_buf[jtk*8+itk]--;
					printer_buf_odd[jtk] |= (1 << (7-itk));
				}
			}
		}
	
		os_printf("%s %d\n",__FUNCTION__,__LINE__);	

        os_memset(printer_buf_even, 0xFF, HEAT_BYTES_PER_LINE);


		spi_ioctl((struct spi_device*)spi0_dev, SPI_KICK_DMA_TX, (uint32)printer_buf_even, HEAT_BYTES_PER_LINE);
		while(!spi_ioctl((struct spi_device*)spi0_dev, SPI_GET_DMA_PENDING, 0, 0))
        {
            os_sleep_ms(1);
        };
		spi_ioctl((struct spi_device*)spi0_dev, SPI_CLEAR_DMA_PENDING, 1, 0);

		os_memset(printer_buf_even, 0, HEAT_BYTES_PER_LINE);

		gpio_set_val(PRINTER_LATCH_PIN,0);
		gpio_set_val(PRINTER_LATCH_PIN,1);
	
        //加热部分使用timer控制时间，取消关中断操作
		struct timer_device *stb_timer0 = (struct timer_device *)printer_stb_timer_open();
        printer_stb_start((void*)stb_timer0, 30);  //预加热 30ms

        while (pre_heat_flag == 0) {
            os_sleep_ms(10);
        }
	
		printer_moto_enable(1);

        spi_request_irq(spi0_dev, SPI_IRQ_FLAG_TX_DONE, spi0_irq, (uint32)halfstep_buf);
        last_ret = 1;
        //触发中断打印状态机
        spi_ioctl((struct spi_device*)spi0_dev, SPI_KICK_DMA_TX, (uint32)printer_buf_even, HEAT_BYTES_PER_LINE);

        //等待打印结束
        while (printer_done != 1)
        {
            os_sleep_ms(100);
        }

		gpio_set_val(PRINTER_EN_PIN,0);	  //disable printer power
        printer_stb_timer_close((void*)stb_timer0);
		paper_io_deinit();
		printer_dvp_en_exchange(0);
	
		os_printf("printer   stop....\r\n");

	}

    //理论上没有处理循环退出操作，此处buff未释放

__exit:

	if (halfstep_buf) {
		os_free(halfstep_buf);
		halfstep_buf = NULL;
	}

	if (halfstep_buf_cache) {
		os_free(halfstep_buf_cache);
		halfstep_buf_cache = NULL;
	}

	if (printer_buf) {
		os_free_psram(printer_buf);
		printer_buf = NULL;
	}

	if (printer_buf_odd) {
		os_free(printer_buf_odd);
		printer_buf_odd = NULL;
	}

	if (printer_buf_even) {
		os_free(printer_buf_even);
		printer_buf_even = NULL;
	}

}

int32 atcmd_printer_action(const char *cmd, char *argv[], uint32 argc)
{
	if(os_atoi(argv[0])) {
		printer_kick_start();
	}
    return 0;
}

void printer_thread_init()
{
    os_task_create("printer", printer_work_task, NULL, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
}

#endif

