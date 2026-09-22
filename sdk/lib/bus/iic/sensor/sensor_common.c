#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/i2c.h"
#include "syscfg.h"
#include "app_iic/app_iic.h"

#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif
#include "syscfg.h"



#if DEV_SENSOR_OV7725
extern const _Sensor_Ident_ ov7725_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ ov7725_cmd;
#endif


#if DEV_SENSOR_OV7670
extern const _Sensor_Ident_ ov7670_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ ov7670_cmd;
#endif

#if DEV_SENSOR_GC0308
extern const _Sensor_Ident_ gc0308_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc0308_cmd;
#endif

#if DEV_SENSOR_JXV3
extern const _Sensor_Ident_ jxv3_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ jxv3_cmd;
#endif


#if DEV_SENSOR_BF20A6
extern const _Sensor_Ident_ bf20a6_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ bf20a6_cmd;
extern const _Sensor_Ident_ bf20a6_spi_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ bf20a6_spi_cmd;
#endif

#if DEV_SENSOR_GC0309
extern const _Sensor_Ident_ gc0309_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc0309_cmd;
#endif

#if DEV_SENSOR_GC0328
extern const _Sensor_Ident_ gc0328_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc0328_cmd;
#endif

#if DEV_SENSOR_GC0311
extern const _Sensor_Ident_ gc0311_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc0311_cmd;
#endif

#if DEV_SENSOR_GC0329
extern const _Sensor_Ident_ gc0329_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc0329_cmd;
#endif

#if DEV_SENSOR_GC0312
extern const _Sensor_Ident_ gc0312_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc0312_cmd;
#endif


#if DEV_SENSOR_BF3A03
extern const _Sensor_Ident_ bf3a03_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ bf3a03_cmd;
#endif

#if DEV_SENSOR_BF3703
extern const _Sensor_Ident_ bf3703_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ bf3703_cmd;
#endif

#if DEV_SENSOR_BF2013
extern const _Sensor_Ident_ bf2013_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ bf2013_cmd;
#endif

#if DEV_SENSOR_OV2640
extern const _Sensor_Ident_ ov2640_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ ov2640_cmd;
#endif

#if DEV_SENSOR_XC7016_H63
extern const _Sensor_Ident_ xc7016_h63_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ xc7016_h63_cmd;
#endif

#if DEV_SENSOR_XC7011_H63
extern const _Sensor_Ident_ xc7011_h63_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ xc7011_h63_cmd;
#endif

#if DEV_SENSOR_XC7011_GC1054
extern const _Sensor_Ident_ xc7011_gc1054_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ xc7011_gc1054_cmd;
#endif

#if DEV_SENSOR_XCG532
extern const _Sensor_Ident_ xcg532_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ xcg532_cmd;
#endif

#if DEV_SENSOR_GC2145
extern const _Sensor_Ident_ gc2145_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc2145_cmd;
#endif

#if DEV_SENSOR_SP0718
extern const _Sensor_Ident_ sp0718_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ sp0718_cmd;
#endif

#if DEV_SENSOR_SP0A19
extern const _Sensor_Ident_ sp0a19_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ sp0a19_cmd;
#endif

#if DEV_SENSOR_BF3720
extern const _Sensor_Ident_ bf3720_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ bf3720_cmd;
#endif

#if DEV_SENSOR_GC032A
extern const _Sensor_Ident_ gc032a_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc032a_cmd;
#endif

#if DEV_SENSOR_BF3A01
extern const _Sensor_Ident_ bf3a01_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ bf3a01_cmd;
#endif

#if DEV_SENSOR_BF30A2
extern const _Sensor_Ident_ bf30a2_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ bf30a2_cmd;
#endif

#if DEV_SENSOR_H62
extern const _Sensor_Ident_ h62_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ h62_cmd;
#endif

#if DEV_SENSOR_H63P
extern const _Sensor_Ident_ h63p_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ h63p_cmd;
#endif

#if DEV_SENSOR_H66
extern const _Sensor_Ident_ h66_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ h66_cmd;
#endif

#if DEV_SENSOR_SC1336
extern const _Sensor_Ident_ sc1336_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ sc1336_cmd;
#endif

#if DEV_SENSOR_SC1346
extern const _Sensor_Ident_ sc1346_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ sc1346_cmd;
#endif

#if DEV_SENSOR_GC1084
extern const _Sensor_Ident_ gc1084_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc1084_cmd;

extern const _Sensor_Ident_ gc1084_init2;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc1084_cmd2;
#endif

#if DEV_SENSOR_GC1054
extern const _Sensor_Ident_ gc1054_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc1054_cmd;
#endif

#if DEV_SENSOR_GC2083
extern const _Sensor_Ident_ gc2083_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc2083_cmd;
#endif

#if DEV_SENSOR_GC2053
extern const _Sensor_Ident_ gc2053_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc2053_cmd;
#endif

#if DEV_SENSOR_SC2336P
extern const _Sensor_Ident_ sc2336p_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ sc2336p_cmd;
#endif

#if DEV_SENSOR_SC233AP
extern const _Sensor_Ident_ sc233ap_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ sc233ap_cmd;
#endif

#if DEV_SENSOR_SC2331
extern const _Sensor_Ident_ sc2331_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ sc2331_cmd;
#endif

#if DEV_SENSOR_F38P
extern const _Sensor_Ident_ f38p_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ f38p_cmd;
#endif

#if DEV_SENSOR_F37P
extern const _Sensor_Ident_ f37p_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ f37p_cmd;
#endif

#if DEV_SENSOR_OV9734
extern const _Sensor_Ident_ ov9734_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ ov9734_cmd;
#endif

#if DEV_SENSOR_GC20C3
extern const _Sensor_Ident_ gc20C3_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc20C3_cmd;
#endif

#if DEV_SENSOR_H63S
extern const _Sensor_Ident_ h63s_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ h63s_cmd;
#endif

#if DEV_SENSOR_IMX219
extern const _Sensor_Ident_ imx219_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ imx219_cmd;
#endif

#if DEV_SENSOR_CV2008
extern const _Sensor_Ident_ cv2008_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ cv2008_cmd;
#endif

#if DEV_SENSOR_CV2005
extern const _Sensor_Ident_ cv2005_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ cv2005_cmd;
#endif

#if DEV_SENSOR_GC1084_CSI1
extern const _Sensor_Ident_ gc1084_init_csi1;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc1084_cmd_csi1;
#endif



#if DEV_SENSOR_XS9950
extern const _Sensor_Ident_ xs9950_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ xs9950_cmd;
#endif


static const _Sensor_Adpt_* SensorTable_CSI[] = {

#if DEV_SENSOR_JXV3
    &jxv3_cmd,
#endif

#if DEV_SENSOR_GC0308
    &gc0308_cmd,
#endif

#if DEV_SENSOR_GC0309
    &gc0309_cmd,
#endif

#if DEV_SENSOR_GC0311
    &gc0311_cmd,
#endif

#if DEV_SENSOR_GC0312
    &gc0312_cmd,
#endif

#if DEV_SENSOR_GC0328
    &gc0328_cmd,
#endif

#if DEV_SENSOR_GC0329
    &gc0329_cmd,
#endif

#if DEV_SENSOR_BF3A03
    &bf3a03_cmd,
#endif

#if DEV_SENSOR_BF3703
    &bf3703_cmd,
#endif

#if DEV_SENSOR_OV2640
    &ov2640_cmd,
#endif

#if DEV_SENSOR_OV7725
    &ov7725_cmd,
#endif

#if DEV_SENSOR_OV7670
    &ov7670_cmd,
#endif

#if DEV_SENSOR_BF2013
    &bf2013_cmd,
#endif

#if DEV_SENSOR_H62
    &h62_cmd,
#endif

#if DEV_SENSOR_H66
    &h66_cmd,
#endif

#if DEV_SENSOR_GC1084
    &gc1084_cmd,
#endif

#if DEV_SENSOR_GC1054
    &gc1054_cmd,
#endif

#if DEV_SENSOR_SC1336
    &sc1336_cmd,
#endif

#if DEV_SENSOR_SC1346
    &sc1346_cmd,
#endif

#if DEV_SENSOR_XC7016_H63
    &xc7016_h63_cmd,
#endif

#if DEV_SENSOR_XC7011_H63
    &xc7011_h63_cmd,
#endif

#if DEV_SENSOR_XC7011_GC1054
    &xc7011_gc1054_cmd,
#endif

#if DEV_SENSOR_GC2053
    &gc2053_cmd,
#endif

#if DEV_SENSOR_XCG532
    &xcg532_cmd,
#endif

#if DEV_SENSOR_GC2145
    &gc2145_cmd,
#endif

#if DEV_SENSOR_SP0718
    &sp0718_cmd,
#endif

#if DEV_SENSOR_SP0A19
    &sp0a19_cmd,
#endif

#if DEV_SENSOR_BF3720
    &bf3720_cmd,
#endif

#if DEV_SENSOR_SC2336P
    &sc2336p_cmd,
#endif

#if DEV_SENSOR_GC2083
    &gc2083_cmd,
#endif

#if DEV_SENSOR_OV9734
    &ov9734_cmd,
#endif

#if DEV_SENSOR_GC20C3
    &gc20C3_cmd,
#endif

#if DEV_SENSOR_XS9950
    &xs9950_cmd,
#endif

#if DEV_SENSOR_F37P
    &f37p_cmd,
#endif

#if DEV_SENSOR_F38P
    &f38p_cmd,
#endif

#if DEV_SENSOR_H63P
    &h63p_cmd,
#endif

#if DEV_SENSOR_H63S
    &h63s_cmd,
#endif

#if DEV_SENSOR_IMX219
    &imx219_cmd,
#endif

#if DEV_SENSOR_CV2008
    &cv2008_cmd,
#endif

#if DEV_SENSOR_CV2005
    &cv2005_cmd,
#endif

    NULL,
};


static int check_sensor_id(uint8_t devid,const _Sensor_Ident_ *p_sensor_ident)
{
	int8 u8Buf[3];
	uint8_t tablebuf[16];
	uint32 id= 0;
	uint32 k = 0;
    uint8 u8SensorwriteID2;

	u8SensorwriteID2 = p_sensor_ident->w_cmd;

	tablebuf[0] = p_sensor_ident->addr_num;
	tablebuf[1] = p_sensor_ident->data_num;
	tablebuf[2] = u8SensorwriteID2>>1;

	u8Buf[0] = p_sensor_ident->id_reg;
	if(p_sensor_ident->addr_num == 2)
	{
		u8Buf[0] = p_sensor_ident->id_reg>>8;
		u8Buf[1] = p_sensor_ident->id_reg;
	}
	k = 0;
	tablebuf[3+k] = u8Buf[0];
	k++;
	if(p_sensor_ident->addr_num == 2){
		tablebuf[3+k] = u8Buf[1];
		k++;
	}

	tablebuf[3+k] = tablebuf[4+k] = tablebuf[5+k] = tablebuf[6+k] = 0;
	//i2c_read(p_iic,u8Buf,p_sensor_ident->addr_num,(int8*)&id,p_sensor_ident->data_num);
	wake_up_iic_queue(devid,tablebuf,0,0,(uint8_t*)NULL);
	while(iic_devid_finish(devid) != 1){
		os_sleep_ms(1);
	}
	
	id = tablebuf[3+k] | (tablebuf[4+k]<<8) | (tablebuf[5+k]<<16) | (tablebuf[6+k]<<24);
	os_printf("SID: %x, %x, %x, %x,%x\r\n",id,p_sensor_ident->id,u8SensorwriteID2,p_sensor_ident->r_cmd,p_sensor_ident->id_reg);
	if(id == p_sensor_ident->id){
		iic_devid_set_addr(devid,u8SensorwriteID2>>1);
		return 1;
	}
	else{
		return -1;
	}
}

/*******************************************************************************
* Function Name  : sensor_reset
* Description    : for sensor reset before start
* Input          : nop number
* Output         : None
* Return         : None
*******************************************************************************/
static void sensor_power_on(uint32_t csi_dev_id)
{
    uint8_t rsn, pdn;

    if (csi_dev_id == HG_MIPI_CSI_DEVID) {
        /* CSI0 PDN */
        pdn = MACRO_PIN(PIN_CSI0_PDN);
        if (pdn != 255) {
            gpio_iomap_output(pdn, GPIO_IOMAP_OUTPUT);
            gpio_set_val(pdn, 0);
            os_sleep_ms(2);
            gpio_set_val(pdn, 1);
        }

        /* CSI0 RESET */
        rsn = MACRO_PIN(PIN_CSI0_RESET);
        if (rsn != 255) {
            gpio_iomap_output(rsn, GPIO_IOMAP_OUTPUT);
            gpio_set_val(rsn, 1);
            os_sleep_ms(50);
            gpio_set_val(rsn, 0);
            os_sleep_ms(20);
            gpio_set_val(rsn, 1);

        }

    } else if (csi_dev_id == HG_MIPI1_CSI_DEVID) {
        /* CSI1 PDN */
        pdn = MACRO_PIN(PIN_CSI1_PDN);
        if (pdn != 255) {
            gpio_iomap_output(pdn, GPIO_IOMAP_OUTPUT);
            gpio_set_val(pdn, 0);
            os_sleep_ms(2);
            gpio_set_val(pdn, 1);
        }

        /* CSI1 RESET */
        rsn = MACRO_PIN(PIN_CSI1_RESET);
        if (rsn != 255) {
            gpio_iomap_output(rsn, GPIO_IOMAP_OUTPUT);
            gpio_set_val(rsn, 1);
            os_sleep_ms(50);
            gpio_set_val(rsn, 0);
            os_sleep_ms(20);
            gpio_set_val(rsn, 1);

        }
    }else if (csi_dev_id == HG_DVP_DEVID) {
        /* CSI1 PDN */
        pdn = MACRO_PIN(PIN_CSI1_PDN);
        if (pdn != 255) {
            gpio_iomap_output(pdn, GPIO_IOMAP_OUTPUT);
            gpio_set_val(pdn, 0);
            os_sleep_ms(2);
            gpio_set_val(pdn, 1);
        }

        /* CSI1 RESET */
        rsn = MACRO_PIN(PIN_CSI1_RESET);
        if (rsn != 255) {
            gpio_iomap_output(rsn, GPIO_IOMAP_OUTPUT);
            gpio_set_val(rsn, 1);
            os_sleep_ms(50);
            gpio_set_val(rsn, 0);
            os_sleep_ms(20);
            gpio_set_val(rsn, 1);

        }
    }
}

_Sensor_Adpt_ *sensorAutoCheck(uint8_t csi_dev_id, uint8_t iic_devid)
{
    _Sensor_Adpt_ *matched_entry = NULL;

    sensor_power_on(csi_dev_id);

    for (uint8_t i = 0; SensorTable_CSI[i] != NULL; i++) {
        const _Sensor_Adpt_  *entry = SensorTable_CSI[i];
        const _Sensor_Ident_ *ident = &entry->sensor_iic;

        if (check_sensor_id(iic_devid, ident) >= 0) {
            os_printf("sensor id=0x%x index=%d\n", ident->id, i);
            matched_entry = (_Sensor_Adpt_ *)entry;
            break;
        }
    }

    if (!matched_entry) {
        os_printf("Er: unknown sensor!\n");
		// devSensorInit2 = (_Sensor_Ident_ *)&null_init2;
    }

    return matched_entry;
}

int sensor_write_reg_table(_Sensor_Adpt_ *sensor_cmd, uint8_t index ,uint8_t mipi_csi_iic)
{
	scatter_data init_table = {0};
	uint32_t i 				= 0;
	uint32_t table_size 	= 0;

    const uint8_t *flash_init_table = sensor_cmd->supported_modes[index].reg_list;
    uint8 addr_num = sensor_cmd->sensor_iic.addr_num;
    uint8 data_num = sensor_cmd->sensor_iic.data_num;
    uint8 cmd_len = data_num + addr_num;

    if (!flash_init_table || cmd_len == 0) {
        return -1;
    }

	for(i = 0 ; ; i+=cmd_len)
	{
		if((flash_init_table[i]==0xFF)&&(flash_init_table[i+1]==0xFF)){
			table_size = i;
			break;
		}
		if(i > 0xffff) {
			os_printf(KERN_ERR"sensor init table too large\r\n");
			return -1;
		}
	}	

	init_table.size = table_size;
    init_table.addr = os_malloc(table_size);
    if (!init_table.addr) {
        os_printf(KERN_ERR"%s: malloc fail\r\n",__func__);
        return -1;
    }

    os_memcpy(init_table.addr, flash_init_table, table_size);

	wake_up_iic_queue(mipi_csi_iic,(uint8_t*)&init_table,cmd_len,2,(uint8_t*)NULL);
	while(iic_devid_finish(mipi_csi_iic) != 1){
		os_sleep_ms(1);
	}
	os_free(init_table.addr);

	os_printf("Sensor initialization table write completed ,size:%d\r\n",table_size);
	return 0;
}


int sensor_mode_find_index(_Sensor_Adpt_ *sensor_cmd,uint8_t target_mode, uint8_t target_lane)
{
    for (uint8_t i = 0; i < sensor_cmd->mode_num; i++) {
        const SensorWorkMode *m = &sensor_cmd->supported_modes[i];

        if (m->mode == target_mode &&
            m->mipi.mipi_lane_num == target_lane) {
            return i;
        }
    }
    os_printf("Er: unsupported sensor mode: mode=%d lane=%d\n", target_mode, target_lane);
    return 0;
}

// 公共通用IIC写寄存器函数
static int iic_write_reg(uint8 mipi_csi_iic, uint8 addr_num, uint8 cmd_len, uint16 reg_addr, uint16 val)
{
    scatter_data table = {0};
    uint8 index = 0;

    table.size = cmd_len;
    table.addr = os_malloc(table.size);
    if (!table.addr){
        return -1;
    }
    memset(table.addr, 0, table.size);

    // 拼接寄存器地址
    if (addr_num == SENSOR_REG_WIDTH_8) {
        table.addr[index++] = reg_addr & 0xFF;
    } else{
        table.addr[index++] = reg_addr >> 8;
        table.addr[index++] = reg_addr & 0xFF;
    }
    // 写入2字节数据
    table.addr[index++] = val & 0xFF;

    wake_up_iic_queue(mipi_csi_iic, (uint8_t*)&table, cmd_len, 2, (uint8_t*)NULL);
    while (iic_devid_finish(mipi_csi_iic) != 1){
        os_sleep_ms(1);
    }

    os_free(table.addr);
    return 0;
}

/**
 * 设置帧率VTS
 */
uint16 sensor_fps_to_vts(const _Sensor_Adpt_ *sensor_cmd, uint8_t index, uint8_t mipi_csi_iic, uint8_t target_fps)
{
    if (!sensor_cmd)
        return 0;

    FpsVtsMap_t *preset_arr = sensor_cmd->supported_modes[index].fps_table;
    uint8 addr_num = sensor_cmd->sensor_iic.addr_num;
    uint8 data_num = sensor_cmd->sensor_iic.data_num;
    uint8 cmd_len = data_num + addr_num;
    uint8 reg_cnt = sensor_cmd->vts_reg_num;

    for (uint8 i = 0; i < SENSOR_PRESET_FPS_NUM; i++)
    {
        // os_printf("preset_arr[i].fps %d  vts %d\r\n",preset_arr[i].fps,preset_arr[i].vts);
        if (preset_arr[i].fps == target_fps)
        {
            uint16 vts_val = 0;
            // 循环写入所有VTS寄存器
            for (uint8 k = 0; k < reg_cnt; k++)
            {
                uint16 reg = sensor_cmd->vts_reg[k];
				vts_val = preset_arr[i].vts >> (8*(reg_cnt-k-1));
                iic_write_reg(mipi_csi_iic, addr_num, cmd_len, reg, vts_val);
            }
            return vts_val;
        }
    }

    os_printf("no support this fps:%d, use default frame rate\r\n", target_fps);
    return 0;
}


/**
 * 设置同步边沿
 */
uint8 set_sync_edge(const _Sensor_Adpt_ *sensor_cmd, uint8_t index ,uint8_t mipi_csi_iic, SyncEdgeType edge)
{
    if (!sensor_cmd)
        return 0;

    uint8 addr_num = sensor_cmd->sensor_iic.addr_num;
    uint8 data_num = sensor_cmd->sensor_iic.data_num;
    uint8 cmd_len = data_num + addr_num;

    const SlaveSyncCfg_t *sync = &sensor_cmd->supported_modes[index].sync_cfg;
    uint16 reg = sync->sync_reg;
    uint16 val = (edge == SYNC_EDGE_RISE) ? sync->edge_rise : sync->edge_fall;

    // 寄存器地址、上升沿、下降沿参数全部为0，代表无需配置
    if (reg == 0 && sync->edge_rise == 0 && sync->edge_fall == 0){
        return 0;
    }

    iic_write_reg(mipi_csi_iic, addr_num, cmd_len, reg, val);

    return 1;
}




