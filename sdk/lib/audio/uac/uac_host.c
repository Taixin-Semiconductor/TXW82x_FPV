#include "lib/audio/uac/uac_host.h"

static uint8_t *empty_buf = NULL;
uint32 usbmic_packet_len = 16;
uint8 *usbmic_buf = NULL;
uint8 *usbspk_buf = NULL;

#define UAC_FRAME_NUM      4
#define UAC_FRAME_LEN      1024

volatile UAC_MANAGE usbmic_manage[UAC_FRAME_NUM];
volatile UAC_MANAGE usbspk_manage[UAC_FRAME_NUM];

struct list_head usbmic_free_list;
struct list_head usbmic_use_list;

struct list_head usbspk_free_list;
struct list_head usbspk_use_list;

static uint8 check_usbmic_room = 0;
static uint8 check_usbspk_room = 0;

uint8_t uac_run = 0;
uint8_t uac_open = 0;
uint8 *get_uac_frame_data(UAC_MANAGE *uac_manage)
{
    return uac_manage->data_addr;
}
uint8 get_uac_frame_sta(UAC_MANAGE *uac_manage)
{
    return uac_manage->sta;
}
void set_uac_frame_sta(UAC_MANAGE *uac_manage, uint8 sta)
{
    uac_manage->sta = sta;
}
uint32 get_uac_frame_datalen(UAC_MANAGE *uac_manage)
{
    return uac_manage->data_len;
}
void set_uac_frame_datalen(UAC_MANAGE *uac_manage, uint32 len)
{
    uac_manage->data_len = len;
}
uint32 get_uac_frame_respace_len(UAC_MANAGE *uac_manage)
{
    return uac_manage->respace_len;
}

uint32 get_uac_frame_num(struct list_head *head)
{
	int count = 0;
	struct list_head *first = (struct list_head *)head;
	while(head->next != first)
	{
		head = head->next;
		count++;
	}
 
	return count;	
}


void usbmic_frame_init(void)
{
    for(uint8 i=0; i<UAC_FRAME_NUM; i++)
    {
        usbmic_manage[i].data_addr = NULL;
        usbmic_manage[i].data_len = 0;
        usbmic_manage[i].respace_len = UAC_FRAME_LEN;
        usbmic_manage[i].offset = 0;
        usbmic_manage[i].sta = 0;
    }
}

void usbspk_frame_init(void)
{
    for(uint8 i=0; i<UAC_FRAME_NUM; i++)
    {
        usbspk_manage[i].data_addr = NULL;
        usbspk_manage[i].data_len = 0;
        usbspk_manage[i].respace_len = UAC_FRAME_LEN;
        usbspk_manage[i].offset = 0;
        usbspk_manage[i].sta = 0;
    }
}

void usbmic_room_init(void)
{
    usbmic_frame_init();

    INIT_LIST_HEAD((struct list_head *)&usbmic_free_list);
	INIT_LIST_HEAD((struct list_head *)&usbmic_use_list);
	
	if(check_usbmic_room == 0) {
		usbmic_buf = (uint8*)os_zalloc(UAC_FRAME_LEN*UAC_FRAME_NUM);
        if(usbmic_buf)
		    check_usbmic_room = 1;
	}
	
	if(!usbmic_buf)
	{
		os_printf("malloc usbmic room err\n");
		return;
	}
    for(uint8 i=0; i<UAC_FRAME_NUM; i++)
	{
		usbmic_manage[i].data_addr = usbmic_buf+i*UAC_FRAME_LEN;
		list_add_tail((struct list_head *)&usbmic_manage[i].list,(struct list_head *)&usbmic_free_list); 
	}
	os_printf("usbmic_room_init\n");
}

void usbmic_room_del(void)
{
    if(usbmic_buf) {
        os_free(usbmic_buf);
        os_printf("usbmic_room_del\n");
		usbmic_buf = NULL;
        check_usbmic_room = 0;
    }
}

void usbspk_room_init(uint32_t empty_buf_len)
{
    usbspk_frame_init();

    INIT_LIST_HEAD((struct list_head *)&usbspk_free_list);
	INIT_LIST_HEAD((struct list_head *)&usbspk_use_list);

    if(empty_buf_len == 0)
    {
        empty_buf_len = 96;     //48K采样率
    }

    empty_buf = (uint8_t*)os_zalloc(empty_buf_len);
	if(!empty_buf)
    {
        os_printf("malloc empty buf err\n");
        return;
    }

	if(check_usbspk_room == 0) {
		usbspk_buf = (uint8*)os_zalloc(UAC_FRAME_LEN*UAC_FRAME_NUM);
        if(usbspk_buf)
		    check_usbspk_room = 1;
	}
	
	if(!usbspk_buf)
	{
		os_printf("malloc usbspk room err\n");
		return;
	}
    for(uint8 i=0; i<UAC_FRAME_NUM; i++)
	{
		usbspk_manage[i].data_addr = usbspk_buf+i*UAC_FRAME_LEN;
		list_add_tail((struct list_head *)&usbspk_manage[i].list,(struct list_head *)&usbspk_free_list); 
	}
	os_printf("usbspk_room_init\n");
}

void usbspk_room_del(void)
{
    if(usbspk_buf) {
        os_free(usbspk_buf); 
        os_printf("usbspk_room_del\n");
		usbspk_buf = NULL;
        check_usbspk_room = 0;
    }
    if(empty_buf) {
        os_free(empty_buf);
        os_printf("empty buf free\n");
        empty_buf = NULL;
    }
}
UAC_MANAGE *get_usbmic_new_frame(uint8 grab)
{
    UAC_MANAGE *uac_manage = NULL;
    if(grab == 0)
    {
        if(list_empty((const struct list_head *)&usbmic_free_list)){
//            os_printf("Not usbmic free frame\n");
            return 0;
        }  
    }
    else if(grab == 1)
    {
        if(list_empty((const struct list_head *)&usbmic_free_list)){
            list_move_tail((struct list_head *)usbmic_use_list.prev, (struct list_head *)&usbmic_free_list); 
        }
    } 
    uac_manage = list_entry((struct list_head *)usbmic_free_list.next, UAC_MANAGE, list); 
	uac_manage->data_len = 0;
	uac_manage->respace_len = UAC_FRAME_LEN;
	uac_manage->offset = 0;
    set_uac_frame_sta(uac_manage, 1); 
    return  uac_manage;
}
UAC_MANAGE *get_usbmic_frame(void)
{
    UAC_MANAGE *uac_manage = NULL;
	if(list_empty((const struct list_head *)&usbmic_use_list)){
//		os_printf("Not usbmic use frame\n");
		return 0;
	} 
    uac_manage = list_entry((struct list_head *)usbmic_use_list.next, UAC_MANAGE, list); 
    set_uac_frame_sta(uac_manage, 3);  
    return uac_manage;    
}
void put_usbmic_frame_to_use(UAC_MANAGE *uac_manage)
{
    set_uac_frame_sta(uac_manage, 2); 
    list_move_tail((struct list_head *)&uac_manage->list, (struct list_head *)&usbmic_use_list);  
}
void del_usbmic_frame(UAC_MANAGE *uac_manage)
{
    set_uac_frame_sta(uac_manage, 0); 
    list_move_tail((struct list_head *)&uac_manage->list, (struct list_head *)&usbmic_free_list); 
}

UAC_MANAGE *get_usbspk_new_frame(uint8 grab)
{
    UAC_MANAGE *uac_manage = NULL;
    if(grab == 0)
    {
        if(list_empty((const struct list_head *)&usbspk_free_list)){
//            os_printf("Not usbspk free frame\n");
            return 0;
        }  
    }
    else if(grab == 1)
    {
        if(list_empty((const struct list_head *)&usbspk_free_list)){
            list_move_tail((struct list_head *)usbspk_use_list.prev, (struct list_head *)&usbspk_free_list); 
        }
    }
    uac_manage = list_entry((struct list_head *)usbspk_free_list.next, UAC_MANAGE, list); 
	uac_manage->data_len = 0;
	uac_manage->respace_len = UAC_FRAME_LEN;
	uac_manage->offset = 0;
    set_uac_frame_sta(uac_manage, 1); 
    return  uac_manage;
}
UAC_MANAGE *get_usbspk_frame(void)
{
    UAC_MANAGE *uac_manage = NULL;
	if(list_empty((const struct list_head *)&usbspk_use_list)){
//		os_printf("Not usbspk use frame\n");
		return 0;
	} 
    uac_manage = list_entry((struct list_head *)usbspk_use_list.next, UAC_MANAGE, list); 
    set_uac_frame_sta(uac_manage, 3);  
    return uac_manage;    
}
void put_usbspk_frame_to_use(UAC_MANAGE *uac_manage)
{
    set_uac_frame_sta(uac_manage, 2); 
    list_move_tail((struct list_head *)&uac_manage->list, (struct list_head *)&usbspk_use_list);  
}
void del_usbspk_frame(UAC_MANAGE *uac_manage)
{
    set_uac_frame_sta(uac_manage, 0); 
    list_move_tail((struct list_head *)&uac_manage->list, (struct list_head *)&usbspk_free_list); 
}

void force_reset_usbmic_frame()
{
    uint8 frame_num = 0; 
    if(check_usbmic_room) {
        for(frame_num=0; frame_num<UAC_FRAME_NUM; frame_num++)
        {
            del_usbmic_frame((UAC_MANAGE *)&usbmic_manage[frame_num]);
        }
    }
}
void force_reset_usbspk_frame()
{
    uint8 frame_num = 0; 
    if(check_usbspk_room) {
        for(frame_num=0; frame_num<UAC_FRAME_NUM; frame_num++)
        {
            del_usbspk_frame((UAC_MANAGE *)&usbspk_manage[frame_num]);
        }
    }
}
static UAC_MANAGE *mic_frame = NULL;
static UAC_MANAGE *spk_frame = NULL;

int usbmic_deal(struct usb_device *p_dev, uint8_t* rx_buff)
{	
    if(!mic_frame){
	    mic_frame = get_usbmic_new_frame(1);
	}
    if(!mic_frame) {
	    return 0;
    }
    uint8 *addr = get_uac_frame_data(mic_frame);
	os_memcpy(addr+(mic_frame->offset), rx_buff, usbmic_packet_len);
    mic_frame->offset += usbmic_packet_len;
    mic_frame->respace_len -= usbmic_packet_len;
    mic_frame->data_len += usbmic_packet_len;
    if(mic_frame->respace_len < usbmic_packet_len)
    {
        put_usbmic_frame_to_use(mic_frame);
        mic_frame = NULL;
    }
	return 1;
}


void usbspk_tx(struct usb_device *p_dev, uint8_t ep, uint32_t len, usbh_audio_spk_tx func)
{
    uint32 packetlen = len;
    static uint32_t send_empty_cnt = 0;
    uint8 *audio_addr = NULL;

    if(send_empty_cnt > 0) {
		func(p_dev, ep, (uint8_t *)empty_buf, packetlen);
		send_empty_cnt--;
        return;
    }

	if(!spk_frame) {
		spk_frame = get_usbspk_frame();
	}
	if(!spk_frame)
	{
		func(p_dev, ep, (uint8_t *)empty_buf, packetlen);
		send_empty_cnt = UAC_FRAME_LEN/packetlen - 1;
	}
	else {
		audio_addr = get_uac_frame_data(spk_frame);
		func(p_dev, ep, (uint8_t *)(audio_addr+(spk_frame->offset)), packetlen);
		spk_frame->offset += packetlen;
		if(spk_frame->offset >= get_uac_frame_datalen(spk_frame))
		{
			del_usbspk_frame(spk_frame);
			spk_frame = NULL;
		}		
	}
}


void uac_stop(void)
{
	if(uac_open) {
		uac_run = 0;
		os_sleep_ms(10);
		if(check_usbmic_room) {
			os_memset(usbmic_buf, 0, UAC_FRAME_LEN*UAC_FRAME_NUM);
        }
		if(check_usbspk_room) {
			os_memset(usbspk_buf, 0, UAC_FRAME_LEN*UAC_FRAME_NUM);
        }
	}
}


