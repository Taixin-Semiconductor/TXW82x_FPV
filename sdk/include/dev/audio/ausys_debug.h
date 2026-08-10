#ifndef __AUSYS_DEBUG_H
#define __AUSYS_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif



#define AUSYS_DEBUG(fmt, args...)                   //_os_printf(fmt, ##args)
#define AUSYS_INFO(fmt, ...)                        \
											        do{                                             	\
											            os_printf("ausys info:"fmt,##__VA_ARGS__);   	\
											        }while(0);					
#define AUSYS_ERR(fmt, ...)                         \
											        do{                                             	\
											            os_printf("ausys err:"fmt,##__VA_ARGS__);   	\
											            os_printf("Func:%s Line:%d LR=0x%x\r\n",    	\
											            __func__,__LINE__, (uint32)RETURN_ADDR());  	\
											        }while(0);
#define AUSYS_WARNING(fmt, ...)                     \
											        do{                                                 \
											            os_printf("ausys warning:"fmt,##__VA_ARGS__);   \
											        }while(0);
#define AUSYS_ASSERT(expr, info)            	    \
											        do {                           						\
											            if (expr) {        								\
															AUSYS_ERR(info);							\
											                return RET_ERR;        						\
											            }                          						\
											        } while(0);
#define AUSYS_PRINT_ARRAY(buf, len)                 do{\
                                                        uint8 *p_print = (uint8 *)buf;\
                                                        _os_printf("\r\n");\
                                                        for (uint32 __ii=1; __ii<=len; __ii++) {\
                                                            _os_printf("%02x ", p_print[__ii-1]);\
                                                            if (((__ii)%16 == 0)) _os_printf("\r\n");\
                                                        }\
                                                        _os_printf("\r\n");\
                                                    }while(0);



#ifdef __cplusplus
}
#endif


#endif
