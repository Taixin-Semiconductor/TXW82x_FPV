#ifndef __SPOOK_H__
#define __SPOOK_H__
#include "spook_config.h"
int config_port( int port );
int live_init(const rtp_name *jpg_name);
void spook_init(void);
void * jpeg_init(const rtp_name *jpg_name);

#endif 

