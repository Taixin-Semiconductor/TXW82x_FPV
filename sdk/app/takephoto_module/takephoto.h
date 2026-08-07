#ifndef __TAKEPHOTO_H__
#define __TAKEPHOTO_H__
void common_takephoto_over_api(uint16_t over_w, uint16_t over_h, uint16_t thumb_w, uint16_t thumb_h,uint8_t takephoto_num);
void common_takephoto_over_dpi_init(uint8_t jpg_num);
void common_takephoto_normal_init(const char *thumb_msi_name);
void common_takephoto_noraml_api_with_path(struct msi *jpg_normal_msi,uint8_t takephoto_num,const char *path);
#endif