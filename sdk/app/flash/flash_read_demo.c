#include "flash_read_demo.h"

// extern struct os_mutex *get_syscfg_lock();

uint8_t flash_read_uuid_demo(uint32_t addr)
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	uint8_t id[128] = {0};
    struct hgxip_flash_custom_read flash_param;
    // struct os_mutex *mutex = get_syscfg_lock();
	
	struct spi_nor_flash *flash = (void *)dev_get(HG_FLASH0_DEVID);
    spi_nor_open(flash);
	
    //设置读取flash的参数命令
    flash_param.dummys = 8;
    flash_param.cmd = 0x4b; //flash读取id的命令
    flash_param.size = 128;  //id正常是3byte,如果超过3byte,则会循环读取
    flash_param.addr = addr;
    flash_param.buf = id;
    // os_mutex_lock(mutex, osWaitForever);
    spi_nor_open(flash);
    spi_nor_custom_read(flash, (void*)&flash_param);

    spi_nor_close(flash);
    // os_mutex_unlock(mutex);

    for(int i=0;i<128;i++)
    {
        if(i%16 == 0)
        {
            _os_printf("\r\n");
        }
        _os_printf("%02X ",id[i]);
    }
    _os_printf("\r\n");
    return 0;
}


uint8_t flash_security_demo(uint32_t addr)
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	uint8_t id[128] = {0};
    // struct os_mutex *mutex = get_syscfg_lock();
    
	struct spi_nor_regval rval;
	
    // os_mutex_lock(mutex, osWaitForever);
	struct spi_nor_flash *flash = (void *)dev_get(HG_FLASH0_DEVID);
    spi_nor_open(flash);
	
    //擦除
    spi_nor_sec_erase(flash, addr);
	
//	os_sleep_ms(1000);
//    //写入
    uint8_t buf[32] = "1234657890";
	rval.buff = buf;
	rval.size = 10;
    spi_nor_sec_program(flash, addr, &rval);

    //读取
	rval.buff = id;
	rval.size = 128;
    spi_nor_sec_read(flash, addr, &rval);

    spi_nor_close(flash);
    // os_mutex_unlock(mutex);

    for(int i=0;i<128;i++)
    {
        if(i%16 == 0)
        {
            _os_printf("\r\n");
        }
        _os_printf("%02X ",id[i]);
    }
    _os_printf("\r\n");
    return 0;
}

//通过自定义方式去操作安全区(可能不同flash命令不一样)
uint8_t flash_custom_security_demo(uint32_t addr)
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	uint8_t id[128] = {0};
    struct hgxip_flash_custom_read flash_param;
    // struct os_mutex *mutex = get_syscfg_lock();

	struct spi_nor_flash *flash = (void *)dev_get(HG_FLASH0_DEVID);
    spi_nor_open(flash);

    //设置读取flash的参数命令
    flash_param.dummys = 0;
    flash_param.cmd = 0x42; //flash读取id的命令
    flash_param.size = 128;  //id正常是3byte,如果超过3byte,则会循环读取
    flash_param.addr = addr;
    flash_param.buf = id;
    // os_mutex_lock(mutex, osWaitForever);
    spi_nor_open(flash);


    //擦除只需要命令和地址(接口暂时支持这个,有特殊flash需要作兼容)
    flash_param.cmd = 0x44;
    spi_nor_custom_erase(flash, (void *)&flash_param);

    //写入数据到对应区域,需要设置dummys buf  长度  以及命令
    memcpy(id,"abcdefghij",10);
    flash_param.size = 10;
    flash_param.cmd = 0x42;
    flash_param.dummys = 0;
    spi_nor_custom_write(flash, (void *)&flash_param);

    //读取数据,需要设置dummys  命令 buf  size
    memset(id,0,sizeof(id));
    flash_param.cmd = 0x48;
    flash_param.dummys = 8;
    flash_param.size = sizeof(id);
    spi_nor_custom_read(flash, (void *)&flash_param);


    spi_nor_close(flash);
    // os_mutex_unlock(mutex);

    for(int i=0;i<128;i++)
    {
        if(i%16 == 0)
        {
            _os_printf("\r\n");
        }
        _os_printf("%02X ",id[i]);
    }
    _os_printf("\r\n");
    return 0;
}

uint8_t flash_rdwr_demo(uint32_t addr)
{
	uint32_t sector_addr = addr & (~0xfff);
	uint8_t buf[512];
	struct spi_nor_flash *flash = (void *)dev_get(HG_FLASH0_DEVID);
    spi_nor_open(flash);
	printf("erase addr 0x%x\r\n", sector_addr);
	spi_nor_sector_erase(flash, sector_addr);
	spi_nor_read(flash, sector_addr, buf, sizeof(buf));
	for(int i = 0;i<sizeof(buf);i++)
	{
		buf[i] = i & 0xff;
	}
	spi_nor_write(flash, sector_addr, buf, sizeof(buf));
	memset(buf, 0, sizeof(buf));
	spi_nor_read(flash, sector_addr, buf, sizeof(buf));
	for(int i = 0;i<512;i++)
	{
		if (buf[i] != (i&0xff)) {
			printf("flash rdwr fail\r\n");
			break;
		}
	}
	
    for(int i=0;i<128;i++)
    {
        if(i%16 == 0)
        {
            _os_printf("\r\n");
        }
        _os_printf("%02X ",buf[i]);
    }
	
	return 0;
}


uint8_t flash_reg_opt_demo(void)
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    uint8_t buf[4] = {0};
    struct hgxip_flash_reg_opt_param opt_param;
    // struct os_mutex *mutex = get_syscfg_lock();
	struct spi_nor_flash *flash = (void *)dev_get(HG_FLASH0_DEVID);
    spi_nor_open(flash);
	
    // os_mutex_lock(mutex, osWaitForever);
    spi_nor_open(flash);
    //read flash status reg
    opt_param.buf = buf;
    opt_param.cmd = 0x5;
    opt_param.pre_cmd = 0xFF; 
    opt_param.len = 4;
    spi_ioctl(flash->spidev, SPI_XIP_REG_OPT, (uint32)&opt_param, 0);

    
    //write flash status reg : write protect
    buf[0] |= 0x1c;
    opt_param.buf = buf;
    opt_param.cmd = 0x01;
    opt_param.pre_cmd = 0x50; 
    opt_param.len = 1;
    spi_ioctl(flash->spidev, SPI_XIP_REG_OPT, (uint32)&opt_param, 0);

    spi_nor_close(flash);
    // os_mutex_unlock(mutex);
    return 0;
}
