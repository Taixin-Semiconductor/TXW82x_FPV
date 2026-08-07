/**
  ******************************************************************************
  * @file    User\AppLoader\include\al_typedef.h
  * @author  TaiXin-Semi Application Team
  * @version V1.0.0
  * @date    18-08-2025
  * @brief   This file contains all the loader typedef.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2025 TaiXin-Semi</center></h2>
  *
  *
  *
  ******************************************************************************
  */ 
#ifndef __AL_TYPEDEF_H
#define __AL_TYPEDEF_H

#ifdef __cplusplus
 extern "C" {
#endif
     
/** @defgroup AL_Typedef
  * @{
  */

  /**
 * @brief 生成设备组ID的宏
 * @param from 源设备ID (enum obj_id)
 * @param to 目标设备ID (enum obj_id) 
 * @return 合成的16位组ID (高8位:源设备, 低8位:目标设备)
 *
 * @note 用于快速生成设备传输组合的标识符
 * @warning 设备ID值必须小于256
 *
 * 使用示例：
 * @code
 * uint16 group = GROUP_ID(SD, FLASH); // SD卡到Flash的传输组
 * @endcode
 */
#define GROUP_ID(from, to)      ((from<<8)|(to))

/**
 * @brief 存储设备标识枚举
 * 
 * 定义系统支持的各种存储设备类型标识，用于区分不同存储介质。
 */
enum obj_id {
    SRAM,     /**< 静态随机存储器(SRAM) */
    SD,       /**< SD卡存储设备 */
    NAND,     /**< NAND Flash存储器 */
    PSRAM,    /**< 伪静态随机存储器(PSRAM) */
    FLASH,    /**< 并行NOR Flash存储器 */ 
    SPIFLASH, /**< SPI接口Flash存储器 */
};

/**
 * @brief 操作模式枚举定义
 * 
 * 定义数据传输和存储操作的各种模式标志，可采用位或(|)组合使用。
 */
enum mode {
    MODE_NONE     = 0,      /**< 无特殊模式，默认操作 */
    FLS_ENCRYPT   = BIT(0), /**< 加密模式：数据在写入/读取时自动加密 */
    FLS_ERASE     = BIT(1), /**< 擦除模式：写入前自动擦除目标区域 */
    UNCOMPRESS    = BIT(2), /**< 解压模式：自动解压源数据 */
    LOADER_UPDATE = BIT(3), /**< 加载器更新模式标志位 */
};

/**
 * @enum verify_mode
 * @brief 数据校验模式枚举
 * 
 * 定义支持的各种CRC校验算法类型，
 * 用于固件验证和数据完整性检查。
 */
enum verify_mode {
    CRC5,          /**< 5位CRC校验 */
    CRC8,          /**< 标准8位CRC校验 */
    CRC8ITU,       /**< ITU-T标准的8位CRC（X.25）*/
    CRC16MODBUS,   /**< Modbus协议使用的16位CRC */
    CRC32,         /**< 32位CRC校验 */
};


/* 固件标志和功能码定义 */
#define SPI_BOOT_VALID_FLAG             (0x5a69) /**< 有效的启动标志 */
#define SPI_BOOT_FUNC_SPI_INFOR         (0x1)    /**< SPI配置功能码 */
#define SPI_BOOT_FUNC_FW_INFOR          (0x2)    /**< 固件信息功能码 */ 
#define SPI_BOOT_FUNC_END               (0xFF)   /**< 结束标志功能码 */

#define XZ_HEADER_MAGIC		            "ZHTX-XZ-FILE" /**< XZ压缩文件头标志 */

/**
 * @struct xz_info
 * @brief XZ压缩文件头信息
 */
struct xz_info {
	uint8_t magic[12];			/**< 文件标识："ZHTX-XZ-FILE" */
	uint32_t bin_header_bytes;  /**< Bin文件头信息长度（字节） */
	uint32_t file_crc16;	    /**< 文件CRC16校验值 */
	uint32_t compress_bytes;	/**< 文件的压缩后的大小，不含xz_info,bin_header */
	uint32_t uncompress_bytes;	/**< 文件的未压缩的大小，不含xz_info,bin_header */
	uint32_t xz_info_crc16;     /**< 本结构体的CRC16校验 */
};

/**
 * @struct fw_info_header_read_cfg  
 * @brief SPI Flash读取配置
 */
struct fw_info_header_read_cfg {
	uint8   read_cmd;               /**< 读命令码 */
	uint8   cmd_dummy_cycles : 4,   /**< 读命令dummy周期数 */
		    clock_mode       : 2,   /**< 时钟模式(CPOL/CPHA) */
		    spec_sequnce_en  : 1,   /**< 特殊序列使能 */
		    quad_wire_en     : 1;   /**< 四线模式使能 */

	uint8   wire_mode_cmd    : 2,   /**< 命令传输线数 */
		    wire_mode_addr   : 2,   /**< 地址传输线数 */ 
		    wire_mode_data   : 2,   /**< 数据传输线数 */
		    quad_wire_select : 2;   /**< 四线模式选择 */

	uint8  reserved3;               /**< 保留字段 */
	uint16 sample_delay;            /**< RX采样延迟(0~clk_divor) */
} /** \cond */ __attribute__((packed))/** \endcond */;

/**
 * @struct fw_info_header_spi_info
 * @brief SPI硬件配置头
 */
struct fw_info_header_spi_info {
    uint8 func_code;                         /**< 功能码(0x1) */
    uint8 size;                              /**< 头部大小 */
    struct fw_info_header_read_cfg read_cfg; /**< 读配置 */
    uint8  spiflash_spec_sequnce[64];        /**< 特殊命令序列 */
    uint16 header_crc16;                     /**< 本结构体的CRC16校验 */
} /** \cond */ __attribute__((packed))/** \endcond */;

/**
 * @struct fw_info_header_fw_info  
 * @brief 固件版本信息头
 */
struct fw_info_header_fw_info {
    uint8  func_code;       /**< 功能码(0x2) */
    uint8  size;            /**< 头部大小 */
    uint32 sdk_version;     /**< SDK版本号 */
    uint32 svn_version;     /**< SVN版本号 */ 
    uint32 date;            /**< 编译日期 */
    uint16 chip_id;         /**< 芯片ID */
    uint8  cpu_id;          /**< CPU ID */
    uint32 code_crc32;      /**< 代码CRC32校验 */
    uint16 param_crc16;     /**< 参数CRC16校验 */
    uint16 crc16;           /**< 本结构体的CRC16校验 */
} /** \cond */ __attribute__((packed))/** \endcond */;

/**
 * @struct fw_info_header_boot
 * @brief 启动配置头
 */
struct fw_info_header_boot {
    uint16 boot_flag;             /**< 启动标志(0x5a69) */
    uint8  version;               /**< 头部版本 */
    uint8  size;                  /**< 头部大小 */
    uint32 boot_to_sram_addr;     /**< 加载到的地址 */
    uint32 run_sram_addr;         /**< 代码运行地址 */
    uint32 boot_code_offset_addr; /**< 代码偏移地址 */
    uint32 boot_from_flash_len;   /**< 加载的长度 */
    uint16 boot_data_crc;         /**< 代码CRC校验 */
    uint16 flash_blk_size;        /**< Flash块大小(64KB(version > 1),   512B(version == 0)) */
    
    /** SPI时钟配置 */
    uint16 boot_baud_mhz   : 14,  /**< SPI时钟频率(MHz(version > 1),  KHz(version == 0)) */
           driver_strength : 2;   /**< IO驱动强度 */
    
    /** 启动模式配置 */
    struct {
        uint16 pll_src : 8,       /**< PLL源时钟(MHz) */
               pll_en : 1,        /**< PLL使能 */
               debug : 1,         /**< 调试输出使能 */
               aes_en : 1,        /**< AES加密使能 */
               crc_en : 1,        /**< CRC校验使能 */
               reserved: 4;       /**< 保留位 */
    } mode /** \cond */ __attribute__((packed))/** \endcond */;
   
    
    uint16 reserved;              /**< 保留字段 */
    uint16 head_crc16;            /**< 本结构体的CRC16校验 */
} /** \cond */ __attribute__((packed))/** \endcond */;

/**
 * @struct fw_info_header_func
 * @brief 功能头基类
 */
struct fw_info_header_func {
    uint8 func_code;       /**< 功能码 */
    uint8 size;            /**< 头部大小 */
} /** \cond */ __attribute__((packed))/** \endcond */;

/**
 * @struct fw_info_user
 * @brief 用户信息
 */
struct fw_info_user {
    uint32 fw_addr[2];      /**< 固件地址[0:主,1:备] */
    uint32 fw_version[2];   /**< 固件版本 */
    uint32 fw_is_cipher[2]; /**< 是否加密标志 */
    uint32 fw_valid_num;    /**< 有效固件数量 */
    uint32 target_fw_index; /**< 目标固件索引 */
    struct xz_info xz_info; /**< XZ压缩信息 */
    void *user_data;
};

/**
 * @struct fw_info_header
 * @brief 完整的固件头信息
 */
struct fw_info_header {
    struct fw_info_header_boot      header_boot;     /**< 启动头 */
    struct fw_info_header_spi_info  header_spi_info; /**< SPI配置头 */
    struct fw_info_header_fw_info   header_fw_info;  /**< 固件信息头 */
};

/**
 * @struct firmware_info  
 * @brief 完整的固件信息结构
 */
struct firmware_info {
    struct fw_info_user     user;      /**< 用户区信息 */
    struct fw_info_header   header[2]; /**< 双备份头信息 */
};
typedef struct firmware_info * firmware_info_t;

/**
 * @struct system_info
 * @brief 系统综合信息结构体
 */
struct system_info {
    uint8 firmware_has_run;   /**< 固件曾经成功运行过 */
};

struct fls_user_data {
    uint32 fw_magic;        /**< 固件魔数(0xA5A55A5A) */
    uint32 loader_bytes;
    uint32 loader_steps;
    uint32 spi_density_id;
};
typedef struct fls_user_data* fls_user_data_t;

/**
  * @}
  */

#endif //AL_TYPEDEF_H

/*************************** (C) COPYRIGHT 2025 TaiXin-Semi ***** END OF FILE *****/