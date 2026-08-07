#include "basic_include.h"
#include "lwip/sockets.h"
#include "netif/ethernetif.h"
#include "lib/net/eloop/eloop.h"
#include "syscfg.h"
#include "walkie_talkie_calling.h"

#define CALLING_TIMEOUT          100
#define WAITACCEPT_TIMEOUT       20
#define WAITCONNECT_TIMEOUT      300

typedef void (*walkie_talkie_calling_cb)(uint32 local_status);

typedef struct {
    struct os_msgqueue recv_msgq;
    struct os_msgqueue user_msgq;
    struct os_mutex mutex;
    walkie_talkie_calling_cb cb_func;
    void *task_hdl;
    struct sockaddr_in remote_addr;
    uint8 start;
    uint32 remote_status;
    uint32 local_status;
    uint32 user_status;
    uint32 recv_info;
    uint32 send_info;
    uint32 cur_seq;
    int32 heartbeat_fd;
    uint32 timeout_cnt;
    uint32 timeout;
} CALLING_STRUCT;

CALLING_STRUCT calling_s;

void walkie_talkie_heartbeat_recv(void *d)
{	
    int32 read_len = 0;
	socklen_t addrlen = sizeof(struct sockaddr_in);
    uint32 recv_info = 0;
	
	while(1) {
		recv_info = 0;
		read_len = recvfrom(calling_s.heartbeat_fd, &recv_info, 4, 0, (struct sockaddr*)&(calling_s.remote_addr), (socklen_t*)&addrlen);
        os_mutex_lock(&calling_s.mutex, osWaitForever);
		if((read_len <= 0 && calling_s.local_status != status_none) || ((recv_info & 0xFFFF) == 0) ||
			(calling_s.cur_seq && ((recv_info & 0xFFFF) != calling_s.cur_seq))) {
			calling_s.timeout_cnt++;
			if(calling_s.local_status == wait_connect || calling_s.local_status == status_none) {
				calling_s.timeout = WAITCONNECT_TIMEOUT;
			}
			else if(calling_s.local_status == wait_accept_connect) {
				calling_s.timeout = WAITACCEPT_TIMEOUT;
			}
			if(calling_s.timeout_cnt >= calling_s.timeout) {
				calling_s.timeout_cnt = calling_s.timeout;
				calling_s.remote_status = disconnecting;  
			}
			os_mutex_unlock(&calling_s.mutex);
			continue;
		}
        os_mutex_unlock(&calling_s.mutex);
        if(read_len > 0) {
            calling_s.timeout_cnt = 0;
		    calling_s.remote_status = connecting;
            if(calling_s.recv_info != recv_info) {
                calling_s.recv_info = recv_info;
                os_msgq_put(&calling_s.recv_msgq, calling_s.recv_info, osWaitForever);
            }
        }
	}
}

void walkie_talkie_calling_thread(void *d)
{
    int32 err = -1;
    int32 time_out = 100;
    int32 recv_msgq_ret = RET_ERR;
    int32 user_msgq_ret = RET_ERR;
    uint32 recv_info = 0;
    uint32 recv_status = status_none;
    uint32 user_ctrl = calling_none;
    uint32 ipaddr = 0;
    struct sockaddr_in remote_addr;
    struct sockaddr_in local_addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);

	if(sys_cfgs.wifi_mode == WIFI_MODE_STA) {
		do{
			if(sys_cfgs.wifi_mode == WIFI_MODE_AP)
				break;
			ipaddr = lwip_netif_get_ip2("w0").addr;
			os_sleep_ms(100);
		}while((ipaddr&0xff000000) == 0x1000000);		
	} 
    calling_s.heartbeat_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(calling_s.heartbeat_fd < 0) {
		goto walkie_talkie_calling_thread_exit;
	} 
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(5010);
    local_addr.sin_addr.s_addr = 0;	
    err = bind(calling_s.heartbeat_fd, (struct sockaddr*)&local_addr, addrlen);
	if(err == -1) {
		goto walkie_talkie_calling_thread_exit;
	}	
	setsockopt(calling_s.heartbeat_fd, SOL_SOCKET, SO_RCVTIMEO, &time_out, sizeof(int32_t));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(5010);
    if(sys_cfgs.wifi_mode == WIFI_MODE_STA) {
        remote_addr.sin_addr.s_addr = inet_addr("192.168.169.1");
    }
    else {
        remote_addr.sin_addr.s_addr = inet_addr("192.168.169.100");
    }
    os_task_create("walkie_talkie_heartbeat_recv", walkie_talkie_heartbeat_recv, NULL, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
    calling_s.start = 1;
    while(1) {
        if(os_memcmp(&calling_s.remote_addr.sin_addr, &remote_addr.sin_addr, sizeof(struct in_addr))) {
            os_memcpy(&calling_s.remote_addr.sin_addr, &remote_addr.sin_addr, sizeof(struct in_addr));
        }
        recv_info = 0;
        user_ctrl = calling_none;
        recv_status = status_none;
        recv_info = os_msgq_get2(&calling_s.recv_msgq, 1, &recv_msgq_ret);
        user_ctrl = os_msgq_get2(&calling_s.user_msgq, 1, &user_msgq_ret);
        if(recv_msgq_ret == RET_OK) {
            recv_status = recv_info >> 16;
			// os_printf("\n\n****recv_status:%d %d****\n\n",recv_status,calling_s.local_status);
        } 
        os_mutex_lock(&calling_s.mutex, osWaitForever);
        switch(calling_s.local_status) {
            case status_none:
            {
                if(recv_status == wait_connect) {                      
                    calling_s.local_status = wait_accept_connect;
                    calling_s.user_status = wait_accept_connect;
                    calling_s.cur_seq = (recv_info & 0xFFFF);
                }
                else if(user_ctrl == calling_start) {
                    calling_s.local_status = wait_connect;
                    calling_s.timeout_cnt = 0;
                    calling_s.remote_status = connecting;
                    os_random_bytes((uint8_t*)(&calling_s.cur_seq), 4);
					calling_s.cur_seq &= 0xFFFF;
                }
                break;
            }
            case wait_connect:                                             
            {
                if(recv_status == wait_disconnect) {
                    calling_s.local_status = disconnecting;
                }
                else if(user_ctrl == calling_stop) {
                    calling_s.local_status = wait_disconnect;
                }
                else if(recv_status == accept_connect) {
                    calling_s.local_status = connecting;
                }
                break;
            }
            case wait_accept_connect:                                       
            {
                if(recv_status == wait_disconnect) {
                    calling_s.local_status = disconnecting;
                }
                else if(user_ctrl == calling_start) {
                    calling_s.local_status = accept_connect;
                    calling_s.user_status = accept_connect;
                }
                else if(user_ctrl == calling_stop) {
                    calling_s.local_status = wait_disconnect;
                }
                break;
            }
            case accept_connect:
            {
                if(recv_status == wait_disconnect) {
                    calling_s.local_status = disconnecting;
                }
                else if(user_ctrl == calling_stop) {
                    calling_s.local_status = wait_disconnect;
                }
                else if(recv_status == connecting) {
                    calling_s.local_status = connecting;
                }
                break;
            }
            case connecting:
            {
                if(recv_status == wait_disconnect) {
                    calling_s.local_status = disconnecting;
                }
                else if(user_ctrl == calling_stop) {
                    calling_s.local_status = wait_disconnect;
                }
                else {
                    calling_s.user_status = connecting;
                }
                break;
            }
            case wait_disconnect:
            {
				if(user_ctrl == calling_start) {
                    calling_s.local_status = wait_connect;
                    calling_s.timeout_cnt = 0;
                    calling_s.remote_status = connecting;
                    os_random_bytes((uint8_t*)(&calling_s.cur_seq), 4);
					calling_s.cur_seq &= 0xFFFF;
                }
                else if(recv_status == disconnecting) {
                    calling_s.local_status = disconnecting;
                }			
                break;
            }
            case disconnecting:
            {
                if(recv_status == wait_connect) {                      
                    calling_s.local_status = wait_accept_connect;
                    calling_s.user_status = wait_accept_connect;
                    calling_s.cur_seq = (recv_info & 0xFFFF);
                }
                else if(user_ctrl == calling_start) {
                    calling_s.local_status = wait_connect;
                    calling_s.timeout_cnt = 0;
                    calling_s.remote_status = connecting;
                    os_random_bytes((uint8_t*)(&calling_s.cur_seq), 4);
					calling_s.cur_seq &= 0xFFFF;
                }
                break;
            }
            default:
                break;
        }
        if(calling_s.remote_status == disconnecting) {
            calling_s.user_status = disconnecting;
            calling_s.cur_seq = 0;
            calling_s.local_status = status_none;
        }
		else if(calling_s.local_status == disconnecting) {
			calling_s.user_status = disconnecting;
			calling_s.cur_seq = 0;			
		}
		else if(calling_s.local_status == wait_disconnect) {
			calling_s.user_status = disconnecting;
		}
        os_mutex_unlock(&calling_s.mutex);
        if(calling_s.local_status != wait_accept_connect) {
            calling_s.send_info = calling_s.cur_seq | (calling_s.local_status << 16);
            sendto(calling_s.heartbeat_fd, &calling_s.send_info, 4, 0, (struct sockaddr*)&remote_addr, addrlen);
        }
        calling_s.cb_func(calling_s.local_status); 
        if(recv_msgq_ret & user_msgq_ret) {
            os_sleep_ms(100);
        } 
    }
walkie_talkie_calling_thread_exit:
    if(calling_s.heartbeat_fd >= 0) {
        close(calling_s.heartbeat_fd);
    }
}

int32 walkie_talkie_calling_init(void *cb_func)
{
    calling_s.heartbeat_fd = -1;
    if(os_msgq_init(&calling_s.recv_msgq, 1) != RET_OK) {
        goto walkie_talkie_calling_init_fail;
    }
    if(os_msgq_init(&calling_s.user_msgq, 1) != RET_OK) {
        goto walkie_talkie_calling_init_fail;
    }
    if(os_mutex_init(&calling_s.mutex) != RET_OK) {
        goto walkie_talkie_calling_init_fail;
    }
    calling_s.user_status = disconnecting;
	calling_s.cb_func = cb_func;
    calling_s.task_hdl = os_task_create("walkie_talkie_calling_thread", walkie_talkie_calling_thread, NULL, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
    if(calling_s.task_hdl == NULL) {
        goto walkie_talkie_calling_init_fail;
    }
    return RET_OK;
walkie_talkie_calling_init_fail:
    os_printf("walkie_talkie_calling_init fail\n");
    if(calling_s.recv_msgq.hdl) {
        os_msgq_del(&calling_s.recv_msgq);
    }
    if(calling_s.user_msgq.hdl) {
        os_msgq_del(&calling_s.user_msgq);
    }
    if(calling_s.mutex.hdl) {
        os_mutex_del(&calling_s.mutex);
    }
    return RET_ERR;
}

int32 walkie_talkie_calling_set(uint32 ctrl)
{   
    if(calling_s.user_msgq.hdl == NULL) {
        return RET_ERR;
    }
    os_mutex_lock(&calling_s.mutex, osWaitForever);
    if(calling_s.start == 0) {
        os_mutex_unlock(&calling_s.mutex);
        return RET_ERR;
    }
    if(ctrl == calling_stop) {
		if(calling_s.user_status == status_none || calling_s.user_status == wait_disconnect || calling_s.user_status == disconnecting) {
            os_mutex_unlock(&calling_s.mutex);
            return RET_ERR;
        }
        calling_s.user_status = wait_disconnect;
    }
    else {
		if(calling_s.user_status == wait_connect || calling_s.user_status == accept_connect || calling_s.user_status == connecting) {
            os_mutex_unlock(&calling_s.mutex);
            return RET_ERR;
        }
        calling_s.user_status = wait_connect;
    }
    os_mutex_unlock(&calling_s.mutex);
    os_msgq_put(&calling_s.user_msgq, ctrl, osWaitForever);
    return RET_OK;
}

int32 walkie_talkie_calling_get(void)
{
    return calling_s.user_status;
}