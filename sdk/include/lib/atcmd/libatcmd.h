#ifndef _HUGEIC_ATCMD_H_
#define _HUGEIC_ATCMD_H_

#ifdef __cplusplus
extern "C" {
#endif

enum ATCMD_RESULT{
    ATCMD_RESULT_ERR         = -1, // 小于0，atcmd执行失败，并返回错误码, 打印"ERROR: ret = -1"
    ATCMD_RESULT_OK          =  0, // atcmd执行成功并退出，需要打印"OK"
    ATCMD_RESULT_CONTINUE    =  1, // atcmd执行成功未退出, 进入continue模式
    ATCMD_RESULT_DONE        =  2, // atcmd执行成功并退出，不需要打印任何信息
};

typedef void(*hgic_atcmd_outhdl)(void *priv, uint8 *data, int32 len);
typedef int32(*hgic_atcmd_hdl)(const char *cmd, char *argv[], uint32 argc);

struct atcmd_settings{
    uint8  args_count, separator;
    uint16 static_cmdcnt;
    uint16 printbuf_size;
    uint16 mute:1, rev:15;
    const struct hgic_atcmd *static_atcmds;
};

struct hgic_atcmd{
    const char *name;
    hgic_atcmd_hdl hdl;
};

struct atcmd_dataif {
    int32(*write)(struct atcmd_dataif *dataif, uint8 *buf, int32 len);
    int32(*read)(struct atcmd_dataif  *dataif, uint8 *buf, int32 len);
};


#define atcmd_dbg(fmt, ...)   //os_printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define atcmd_err(fmt, ...)   os_printf(fmt, ##__VA_ARGS__)

#define atcmd_resp(fmt, ...)  atcmd_printf("%s:"fmt"\r\n", cmd+2, ##__VA_ARGS__)
#define atcmd_ok              atcmd_printf("OK\r\n")
#define atcmd_error           atcmd_printf("ERROR\r\n")
#define atcmd_query()        (argc == 1 && argv[0][0] == '?')
#define atcmd_atoi(idx)      (argc>(idx) ? os_atoi(argv[idx]) : 0)

int32 atcmd_init(struct atcmd_dataif *dataif, struct atcmd_settings *settings);
int32 atcmd_uart_init(uint32 uart_id, uint32 baudrate, uint8 rx_tmo, struct atcmd_settings *settings);
int32 atcmd_recv(uint8 *data, int32 len);
int32 atcmd_register(const char *cmd, hgic_atcmd_hdl hdl);
int32 atcmd_unregister(const char *cmd);
void  atcmd_output(uint8 *data, int32 len);
void  atcmd_output_hdl(hgic_atcmd_outhdl output, void *arg);
void  atcmd_printf(const char *format, ...);
void  atcmd_dumphex(char *prestr, uint8 *data, int32 len, uint8 newline);

#ifdef __cplusplus
}
#endif
#endif
