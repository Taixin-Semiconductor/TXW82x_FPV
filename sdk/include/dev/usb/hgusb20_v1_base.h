/*****************************************************************************
* Module    : usb
* File      : usb.c
* Author    :
* Function  : USB驱动的一些公用模块
*****************************************************************************/
#ifndef USB20_H
#define USB20_H

#ifdef __cplusplus
extern "C" {
#endif








//typedef int8 bool;


#define USBIE_EN()              //NVIC_EnableIRQ(USB_CTL_IRQn);
#define USBIE_DIS()             //NVIC_DisableIRQ(USB_CTL_IRQn);

#define USB_REG_CRITICAL_ENTER()  int32 ie = disable_irq();
#define USB_REG_CRITICAL_EXIT()   enable_irq(ie); 

//写USB SIE寄存器
#define write_usb_reg(reg_addr, reg_data)   do {hw->SIE##reg_addr = reg_data;} while(0)
//void write_usb_reg_index(uint8_t index, uint32_t reg_addr, uint8_t reg_data);
#define write_usb_reg_index(index, reg_addr, reg_data)\
        do {\
            USB_REG_CRITICAL_ENTER();\
            hw->SIEM_INDEX = index;\
            *((volatile uint8_t *)((uint32_t)&hw->SIEM_FADDR + reg_addr)) = reg_data;\
            USB_REG_CRITICAL_EXIT();\
        } while(0)


//读USB SIE寄存器
#define read_usb_reg(reg_addr)              (*((volatile uint8_t *)((uint32_t)&hw->SIE##reg_addr)))
//uint8_t read_usb_reg_index(uint8_t index, uint32_t reg_addr);
#define read_usb_reg_index(index, reg_addr, reg_data)\
        do {\
            USB_REG_CRITICAL_ENTER();\
            hw->SIEM_INDEX = index;\
            reg_data = *((volatile uint8_t *)((uint32_t)&hw->SIEM_FADDR + reg_addr));\
            USB_REG_CRITICAL_EXIT();\
        } while(0)




//USB当前状态机
enum {
    USB_DEFAULT_STATE,
    USB_ADDRESS_STATE,
    USB_CONFIG_STATE
};

//EP0状态机
enum {
    IDLE_STATE,
    TRANSER_OVER_STATE,
    TRANSFERING_STATE,
    RECEIVING_STATE,
    RECEIV_OVER_STATE,
};

//接收者
enum {
    REQUEST_TO_DEVICE,
    REQUEST_TO_INTERFACE,
    REQUEST_TO_ENDPOINT
};

//设备描述符
enum {
    DEVICE_DESCRIPTOR = 1,
    CONFIGURATION_DESCRIPTOR,
    STRING_DESCRIPTOR,
    INTERFACE_DESCRIPTOR,
    ENDPOINT_DESCRIPTOR,
    DEVICE_QUALIFIER_DESCRIPTOR,
//    OTHER_SPEED_CONFIG_DESCRIPTOR,  USB 1.1没有该选项
    INTERFACE_ASSOCIATION_DESCRIPTOR = 11,

    HID_REPORT = 0x22,

    BOS_DEVICE_DESCRIPTOR = 0xF,

    WCID_DEVICE_DESCRIPTOR = 0xEE,    
};

//Feature Selector
enum {
    ENDPOINT_STALL,
    REMOTE_WAKEUP,
    TEST_MODE
};

//检测模式
enum {
    USB_CHECK_HOST,         //尝试主机检测
    USB_CHECK_OTG,          //尝试OTG检测，是否连接PC
    USB_STABLE_HOST,        //已成功检测为USB主机
    USB_STABLE_DEVICE,      //已成功检测为USB设备
};

//返回值
enum {
    USB_CONNECTED_NONE,     //未连接
    USB_CONNECTED_PC,       //连接PC，当Device
    USB_CONNECTED_DEVICE,    //外接U盘，当Host
};

//USB控制结构体
struct hgusb20_dev_ctrl {
//EP0接收的命令
    struct {
        uint8_t rtype;
        uint8_t request;
        uint16_t value;
        uint16_t index;
        uint16_t length;
    } cmd;
    
    uint8_t bus_high_speed      : 1,
            auto_tx_null_pkt    : 1,   //ep1 tx null packet for non-potocol xfer
            hdr_req       : 1,
            error               : 1,
            reserved_bit : 4;
    
    uint8_t test_mode;
    
    uint8_t reg_usb_csr0;
    uint8_t set_addr;
    
//        struct {
//            uint16_t vid;              //VID
//            uint16_t pid;              //PID
//            uint16_t ver;              //Version
//        } info;
};

struct hgusb20_dev_chk {
    uint8_t check_mode;
    uint8_t check_cnt;
    uint8_t check_delay;
} ;


#define     BYTE0(n)            ((uint8_t)((uint16_t)(n)))
#define     BYTE1(n)            ((uint8_t)(((uint16_t)(n))>>8))
#define     BYTE2(n)            ((uint8_t)(((uint16_t)(((uint32_t)(n))>>8))>>8))
#define     BYTE3(n)            ((uint8_t)(((uint16_t)(((uint32_t)(n))>>16))>>8))

#define     LOAD_U32B(p)        ((((volatile uint8_t *)(p))[0]<<24) | (((volatile uint8_t *)(p))[1]<<16) | (((volatile uint8_t *)(p))[2]<<8) | (((volatile uint8_t *)(p))[3]<<0))
#define     LOAD_U32L(p)        ((((volatile uint8_t *)(p))[0]<<0)  | (((volatile uint8_t *)(p))[1]<<8)  | (((volatile uint8_t *)(p))[2]<<16)| (((volatile uint8_t *)(p))[3]<<24))
#define     LOAD_U16B(p)        ((((volatile uint8_t *)(p))[0]<<8) | (((volatile uint8_t *)(p))[1]<<0))
#define     LOAD_U16L(p)        ((((volatile uint8_t *)(p))[0]<<0) | (((volatile uint8_t *)(p))[1]<<8))

#define SAVE_U32B(p, num)   do { \
                                ((volatile uint8_t *)(p))[0] = BYTE3(num); \
                                ((volatile uint8_t *)(p))[1] = BYTE2(num); \
                                ((volatile uint8_t *)(p))[2] = BYTE1(num); \
                                ((volatile uint8_t *)(p))[3] = BYTE0(num); \
                            } while(0)

#define SAVE_U32L(p, num)   do { \
                                ((volatile uint8_t *)(p))[0] = BYTE0(num); \
                                ((volatile uint8_t *)(p))[1] = BYTE1(num); \
                                ((volatile uint8_t *)(p))[2] = BYTE2(num); \
                                ((volatile uint8_t *)(p))[3] = BYTE3(num); \
                            } while(0)

#define SAVE_U16B(p, num)   do { \
                                ((volatile uint8_t *)(p))[0] = BYTE1(num); \
                                ((volatile uint8_t *)(p))[1] = BYTE0(num); \
                            } while(0)

#define SAVE_U16L(p, num)   do { \
                                ((volatile uint8_t *)(p))[0] = BYTE0(num); \
                                ((volatile uint8_t *)(p))[1] = BYTE1(num); \
                            } while(0)



void hgusb20_write_usb_reg_index(uint32_t usb_dev_hw,uint32_t index,uint32_t reg_addr,uint32_t reg_data);
void hgusb20_read_usb_reg_index(uint32_t usb_dev_hw,uint32_t index,uint32_t reg_addr,uint32_t *reg_data);
uint32_t hgusb20_write_usb_reg(uint32_t usb_dev_hw,uint8_t reg_addr,uint8_t reg_data);
uint32_t hgusb20_read_usb_reg(uint32_t usb_dev_hw,uint32_t reg_addr);
void hgusb20_send_empty_packet(uint32_t usb_dev_hw);
#ifdef __cplusplus
}
#endif

#endif
