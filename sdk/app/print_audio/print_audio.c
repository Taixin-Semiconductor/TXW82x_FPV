#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "hal/uart.h"
#include "audio_msi/audio_adc.h"

typedef struct {
    int8 uart_index;
    uint8 enable;
    uint8 destroy;
    struct msi *src_msi;
} PRINT_AUDIO_STRUCT;

static void *audio_print_task = NULL;

void audio_print_thread(void *d)
{
    struct msi *msi = NULL;
    PRINT_AUDIO_STRUCT *printf_audio_s = NULL;
	struct uart_device *uart = NULL;
    struct framebuff *frame_buf = NULL;

    while(1) {
        for(uint32 i=0; i<4; i++) {
            switch(i) {
                case 0:msi = msi_find("audio_print0", 1);break;
                case 1:msi = msi_find("audio_print1", 1);break;
                case 2:msi = msi_find("audio_print2", 1);break;
                case 3:msi = msi_find("audio_print3", 1);break;
                default:break;
            }   
            if(msi) {
                msi_put(msi);
                printf_audio_s = (PRINT_AUDIO_STRUCT*)msi->priv;
				uart = (void *)dev_get(HG_UART0_DEVID + printf_audio_s->uart_index);
                if(printf_audio_s->enable == 0) {
                    if(printf_audio_s->destroy == 0) {
                        printf_audio_s->destroy = 1;
                        msi_destroy(msi);
                    }
                    msi = NULL;
                    goto find_next_channel;
                }
                break;
            }
find_next_channel:
			__NOP();
        } 
        if(msi) {
            frame_buf = msi_get_fb(msi, 0);
        } 
        else {
            audio_print_task = NULL;
            break;
        }      
        if(frame_buf) {
            uart_puts(uart, frame_buf->data, frame_buf->len);
            msi_delete_fb(msi, frame_buf);
            frame_buf = NULL;
        }
        else {
            os_sleep_ms(5);
        }
    }
}

static int32_t audio_print_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    PRINT_AUDIO_STRUCT *printf_audio_s = (PRINT_AUDIO_STRUCT*)msi->priv;
    switch(cmd_id) {
        case MSI_CMD_TRANS_FB:
        {
            ret = RET_ERR;
            struct framebuff *frame_buf = (struct framebuff *)param1;
            if(frame_buf->mtype == F_AUDIO) {
                ret = RET_OK;
            } 
            break;
        }    
        case MSI_CMD_PRE_DESTROY:
        {
            if(printf_audio_s) {
                printf_audio_s->enable = 0;
            }
            break;
        }     
        case MSI_CMD_POST_DESTROY:
        {
            if(printf_audio_s) {
				if(printf_audio_s->uart_index >= 0) {
					struct uart_device *uart = (void *)dev_get(HG_UART0_DEVID + printf_audio_s->uart_index);
					uart_close(uart);
				}
                os_free(printf_audio_s);
            }
            break;                
        }			
        default:
            break;    
    }
    return ret;
}

static int32 print_audio_irqhdl(uint32 irq_flag, uint32 irq_data, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    switch (irq_flag) {
        case UART_IRQ_FLAG_DMA_RX_DONE: 
            break;
        default:
            break;
    }
    return ret;
}

int32 print_audio_enable(uint8 print_channel, uint8 uart_index, struct msi *src_msi)
{
    uint8 isnew = 0;
    struct msi *msi = NULL;
    struct msi *exist_msi = NULL;
    PRINT_AUDIO_STRUCT *print_audio_s = NULL;

    switch(print_channel) {
        case 0:msi = msi_new("audio_print0", 8, &isnew);break;
        case 1:msi = msi_new("audio_print1", 8, &isnew);break;
        case 2:msi = msi_new("audio_print2", 8, &isnew);break;
        case 3:msi = msi_new("audio_print3", 8, &isnew);break;
        default:break;
    }
    if(msi == NULL) {
        os_printf("create audio_print channel fail!\n");
        return RET_ERR;
    }
    else if(isnew == 0) {
        os_printf("audio_print channel has created!\n");
        return RET_ERR;
    }
    msi->enable = 1;
    msi->action = (msi_action)audio_print_msi_action;
    for(uint32 i=0; i<4; i++) {
        switch(i) {
            case 0:exist_msi = msi_find("audio_print0", 1);break;
            case 1:exist_msi = msi_find("audio_print1", 1);break;
            case 2:exist_msi = msi_find("audio_print2", 1);break;
            case 3:exist_msi = msi_find("audio_print3", 1);break;
            default:break;
        }   
        if(exist_msi) {
            msi_put(exist_msi);
            if(exist_msi != msi) {
                print_audio_s = exist_msi->priv;
                if(uart_index == print_audio_s->uart_index) {
                    msi_destroy(msi);
                    os_printf("audio_print enable fail, uart%d has been used!\n",uart_index);
                    return RET_ERR;
                }
            }
        }
    }
    struct uart_device *uart = (void *)dev_get(HG_UART0_DEVID + uart_index);
    if(uart == NULL) {
        msi_destroy(msi);
        os_printf("audio_print enable fail, haven't uart%d!\n,uart_index");
        return RET_ERR;
    }
    if(msi_add_output(src_msi, NULL, msi->name) != RET_OK) {
        msi_destroy(msi);
        os_printf("audio_print enable fail, src_output fail!\n");
        return RET_ERR;  
    }
    print_audio_s = os_zalloc(sizeof(PRINT_AUDIO_STRUCT));
    if(print_audio_s == NULL) {
        msi_destroy(msi);
        os_printf("alloc print_audio_s fail!\n");
        return RET_ERR;        
    }
    msi->priv = print_audio_s;
    print_audio_s->uart_index = -1;
    print_audio_s->enable = 1;
    print_audio_s->src_msi = src_msi;
    if(uart_open(uart, 921600) == RET_ERR) {
		msi_destroy(msi);
        os_printf("alloc print_audio_s fail,uart open fail!\n");
        return RET_ERR; 
	}
	print_audio_s->uart_index = uart_index;
    uart_ioctl(uart, UART_IOCTL_CMD_USE_DMA, 1, 0);
    uart_request_irq(uart, print_audio_irqhdl, UART_IRQ_FLAG_DMA_RX_DONE | UART_IRQ_FLAG_TIME_OUT, (uint32)msi);
    if(audio_print_task == NULL) {
        audio_print_task = os_task_create(msi->name, audio_print_thread, NULL, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
    }
    if(audio_print_task == NULL) {
        msi_destroy(msi);
        os_printf("audio_print enable fail, create task fail!\n");
        return RET_ERR;        
    }
    os_printf("audio printf enable,uart:%d,channel:%d,src:%s\n",uart_index,print_channel,src_msi->name);
    return RET_OK;
}

int32 print_audio_disable(uint8 print_channel)
{
    struct msi *msi = NULL;
    switch(print_channel) {
        case 0:msi = msi_find("audio_print0", 1);break;
        case 1:msi = msi_find("audio_print1", 1);break;
        case 2:msi = msi_find("audio_print2", 1);break;
        case 3:msi = msi_find("audio_print3", 1);break;
        default:break;
    }
    if(msi) {
        msi_put(msi);
        PRINT_AUDIO_STRUCT *printf_audio_s = (PRINT_AUDIO_STRUCT*)msi->priv;
        if(printf_audio_s) {
            printf_audio_s->enable = 0;
        }
		return RET_OK;
    }
	return RET_ERR;
}

int32 atcmd_print_audio_enable(const char *cmd, char *argv[], uint32 argc)
{
    uint8 ctrl = 0;
    uint8 uart_index = 0;
    uint8 print_channel = 0;
    struct msi *src_msi = NULL;
    if(argc < 2) {
        os_printf("print_audio_enable fail, enter ctrl, print_channel, uart_index, src_msi_name!\n");
        return RET_ERR;
    }
    ctrl = os_atoi(argv[0]);
    if(ctrl == 1) {
        if(argc < 3) {
            os_printf("print_audio_enable fail, enter ctrl, print_channel, uart_index, src_msi_name!\n");
            return RET_ERR;
        }
        if(argv[3]) {
            src_msi = msi_find(argv[2], 1);
            if(src_msi) {
                msi_put(src_msi);
            }
            else {
                return RET_ERR;
            }
        }
        else {
            src_msi = get_auadc_msi(AUSYS_AUAD);
        }
        if(src_msi == NULL) {
            os_printf("print_audio_enable fail, haven't src_msi!\n");
            return RET_ERR;
        }
        print_channel = os_atoi(argv[1]);
        uart_index = os_atoi(argv[2]);
        print_audio_enable(print_channel, uart_index, src_msi);
    }
    else if(ctrl == 0) {
        print_audio_disable(print_channel);
    }
    return RET_OK;
}