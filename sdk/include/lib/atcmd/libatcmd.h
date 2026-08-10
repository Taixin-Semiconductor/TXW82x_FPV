#ifndef _HUGEIC_ATCMD_H_
#define _HUGEIC_ATCMD_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AT 命令处理函数的返回值定义
 * 
 * 控制命令执行后的行为流程（是否打印 OK/ERROR，是否保持会话）。
 */
enum ATCMD_RESULT {
    /** 
     * @brief 执行失败 
     * 值 < 0。框架将打印 "ERROR: ret = <返回值>" 并结束当前命令处理。
     * 用于指示具体的错误代码。
     */
    ATCMD_RESULT_ERR         = -1, 

    /** 
     * @brief 执行成功并结束 
     * 值 = 0。框架将打印标准的 "OK\r\n" 并退出当前命令处理流程。
     * 这是最常用的一般成功返回码。
     */
    ATCMD_RESULT_OK          =  0, 

    /** 
     * @brief 执行成功并进入连续模式 (Continue Mode)
     * 值 = 1。命令处理成功，但**不**打印 "OK" 也不退出。
     * 通常用于需要持续输出数据或多轮交互的场景（如透传模式、持续监控），
     * 直到外部条件触发退出或收到特定终止符。
     */
    ATCMD_RESULT_CONTINUE    =  1, 

    /** 
     * @brief 执行成功并静默结束 
     * 值 = 2。命令处理成功，但**不**打印任何响应（包括 "OK"）。
     * 适用于内部调试命令或不需要反馈给用户的操作。
     */
    ATCMD_RESULT_DONE        =  2  
};

/**
 * @brief 自定义输出回调函数类型
 * 
 * 当需要将 AT 命令的响应重定向到非标准输出（如网络 socket、特定缓冲区）时使用。
 * @param priv 用户私有数据指针
 * @param data 输出数据缓冲区
 * @param len 数据长度
 */
typedef void(*hgic_atcmd_outhdl)(void *priv, uint8 *data, int32 len);

/**
 * @brief AT 命令处理函数类型
 * 
 * 用户实现的具体命令逻辑入口。
 * @param cmd 原始命令字符串指针 (例如 "AT+GMR")
 * @param argv 参数列表数组 (已分割好的字符串指针)
 * @param argc 参数个数
 * @return int32 返回 @ref ATCMD_RESULT 枚举值
 * 
 * @note 
 * - argv[0] 通常是命令本身或第一个参数，具体取决于解析器实现。
 * - 可使用宏 @ref atcmd_query 判断是否为查询操作。
 */
typedef int32(*hgic_atcmd_hdl)(const char *cmd, char *argv[], uint32 argc);

/* ================= 配置与数据结构 ================= */

/**
 * @brief AT 命令系统配置结构
 * 
 * 用于初始化时的参数设定。
 */
struct atcmd_settings {
    uint8  args_count;      /**< [配置] 最大支持的参数个数 */
    uint8  separator;       /**< [配置] 参数分隔符 (通默逗号 ',') */
    uint16 static_cmdcnt;   /**< [配置] 静态注册命令表中的命令数量 */
    uint16 printbuf_size;   /**< [配置] 内部打印缓冲区的大小 */
    
    /**
     * @brief 标志位
     * - mute (1 bit): 静音模式。若置 1，禁止所有的 ATCMD_RESULT 打印。
     * - rev (15 bits): 保留位，需填 0。
     */
    uint16 mute:1, rev:15;
    
    /** 
     * @brief 静态命令表指针 
     * 指向编译期确定的命令列表 (struct hgic_atcmd 数组)。
     */
    const struct hgic_atcmd *static_atcmds;
};

/**
 * @brief 单个 AT 命令定义结构
 * 
 * 用于构建命令查找表。
 */
struct hgic_atcmd {
    const char *name;       /**< [配置] 命令名称 (不含 "AT" 前缀，如 "+GMR") */
    hgic_atcmd_hdl hdl;     /**< [配置] 对应的处理函数指针 */
};

/**
 * @brief 数据输入输出接口抽象层
 * 
 * 允许 AT 命令系统运行在不同的传输介质上（如 UART, USB, TCP）。
 */
struct atcmd_dataif {
    /**
     * @brief 写操作函数指针
     * @param dataif 自身指针 (用于面向对象风格调用)
     * @param buf 发送缓冲区
     * @param len 发送长度
     * @return int32 实际发送的字节数
     */
    int32(*write)(struct atcmd_dataif *dataif, uint8 *buf, int32 len);
    
    /**
     * @brief 读操作函数指针
     * @param dataif 自身指针
     * @param buf 接收缓冲区
     * @param len 最大读取长度
     * @return int32 实际读取的字节数
     */
    int32(*read)(struct atcmd_dataif  *dataif, uint8 *buf, int32 len);
};

/* ================= 调试与辅助宏 ================= */

/**
 * @brief 调试打印宏
 * @note 当前被注释掉，若需开启可取消注释并启用 os_printf。
 */
#define atcmd_dbg(fmt, ...)   //os_printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

/**
 * @brief 错误打印宏
 * @note 直接映射到 os_printf，用于输出错误信息。
 */
#define atcmd_err(fmt, ...)   os_printf(fmt, ##__VA_ARGS__)

/**
 * @brief 命令响应打印宏
 * 
 * 自动添加命令名前缀和换行符。
 * @param fmt 格式字符串
 * @param ... 参数
 * @note 假设 `cmd` 是当前处理函数的局部变量 (命令字符串)。
 *       `cmd+2` 用于跳过 "AT" 前缀 (假设命令格式为 "AT+XXX")。
 *       输出格式: "<CMD_NAME>:<内容>\r\n"
 */
#define atcmd_resp(fmt, ...)  atcmd_printf("%s:"fmt"\r\n", cmd+2, ##__VA_ARGS__)

/**
 * @brief 打印成功响应 "OK"
 */
#define atcmd_ok              atcmd_printf("OK\r\n")

/**
 * @brief 打印错误响应 "ERROR"
 */
#define atcmd_error           atcmd_printf("ERROR\r\n")

/**
 * @brief 判断是否为查询命令
 * 
 * 检查参数列表是否仅包含一个 '?' 字符。
 * 用法: `if (atcmd_query()) { /* 处理查询逻辑 * / }`
 * @note 依赖局部变量 `argc` 和 `argv`。
 */
#define atcmd_query()        (argc == 1 && argv[0][0] == '?')

/**
 * @brief 安全地将参数转换为整数
 * 
 * 检查索引是否越界，若越界则返回 0，否则调用 os_atoi 转换。
 * @param idx 参数索引
 * @return int 转换后的整数值
 * @note 依赖局部变量 `argc` 和 `argv`。
 */
#define atcmd_atoi(idx)      (argc>(idx) ? os_atoi(argv[idx]) : 0)


/**
 * @brief 初始化 AT 命令系统 (通用模式)
 * 
 * @param dataif 数据收发接口实现
 * @param settings 系统配置参数
 * @return int32 0: 成功, 负值: 失败
 */
int32 atcmd_init(struct atcmd_dataif *dataif, struct atcmd_settings *settings);

/**
 * @brief 初始化 AT 命令系统 (UART 专用便捷模式)
 * 
 * 内部构建 UART 的 dataif 并初始化系统。
 * @param uart_id UART 端口号
 * @param baudrate 波特率
 * @param rx_tmo 接收超时时间 (ms 或 tick)
 * @param settings 系统配置参数
 * @return int32 0: 成功, 负值: 失败
 */
int32 atcmd_uart_init(uint32 uart_id, uint32 baudrate, uint8 rx_tmo, struct atcmd_settings *settings);

/**
 * @brief 喂入接收数据
 * 
 * 将底层接收到的字节流送入 AT 命令解析器状态机。
 * @param data 接收到的数据缓冲区
 * @param len 数据长度
 * @return int32 处理结果 (可能触发命令执行)
 */
int32 atcmd_recv(uint8 *data, int32 len);

/**
 * @brief 动态注册命令
 * 
 * 在运行时添加新的命令支持。
 * @param cmd 命令名称 (如 "+MYCMD")
 * @param hdl 处理函数
 * @return int32 0: 成功, 负值: 失败 (如名字冲突)
 */
int32 atcmd_register(const char *cmd, hgic_atcmd_hdl hdl);

/**
 * @brief 注销命令
 * 
 * 移除已注册的命令。
 * @param cmd 命令名称
 * @return int32 0: 成功, 负值: 未找到
 */
int32 atcmd_unregister(const char *cmd);

/**
 * @brief 直接输出数据
 * 
 * 向当前输出通道发送原始数据。
 * @param data 数据缓冲区
 * @param len 长度
 */
void  atcmd_output(uint8 *data, int32 len);

/**
 * @brief 设置自定义输出回调
 * 
 * 重定向所有 AT 响应输出到指定的回调函数，而非直接写入 dataif->write。
 * @param output 回调函数指针
 * @param arg 传递给回调的私有数据
 */
void  atcmd_output_hdl(hgic_atcmd_outhdl output, void *arg);

/**
 * @brief 格式化打印函数
 * 
 * 类似 printf，输出到 AT 命令响应通道。
 * @param format 格式字符串
 * @param ... 参数
 */
void  atcmd_printf(const char *format, ...);

/**
 * @brief 十六进制转储打印
 * 
 * 以易读的 hex 格式打印数据块。
 * @param prestr 前缀字符串
 * @param data 数据指针
 * @param len 长度
 * @param newline 是否每行换行 (1: 是, 0: 否)
 */
void  atcmd_dumphex(char *prestr, uint8 *data, int32 len, uint8 newline);

#ifdef __cplusplus
}
#endif
#endif /* _HUGEIC_ATCMD_H_ */
