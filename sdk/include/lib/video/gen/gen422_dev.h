#ifndef GEN422_DEV_H
#define GEN422_DEV_H

void gen422_init(uint32_t w,uint32_t h,uint8_t sigle_frm);
void gen422_set_frame(uint32_t psram0,uint32_t psram1,uint32_t w,uint32_t h);
void gen422_run();

#endif
