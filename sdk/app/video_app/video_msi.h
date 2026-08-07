#ifndef __VIDEO_MSI_H
#define __VIDEO_MSI_H
typedef uint8_t (*scale1_filter_fn)(void* f, uint8_t stype);
typedef uint8_t (*gen420_filter_fn)(void *fb,uint8_t src_id);
struct msi *auto_jpg_msi_init(const char *auto_jpg_name, uint8_t which_jpg, uint8_t src_from);
struct msi *auto_h264_msi_init(const char *auto_h264_name, uint8_t src_from0, uint16_t w0, uint16_t h0, uint8_t src_from1, uint16_t w1, uint16_t h1);
struct msi *thumb_over_dpi_msi_init(const char *msi_name, uint8_t jpg_type, uint8_t stype, uint32_t magic);
struct msi *jpg_concat_msi_init_start(uint32_t jpgid, uint16_t w, uint16_t h, uint16_t *filter_type, uint8_t src_from, uint8_t run);
struct msi *mp4_thumb_msi_init(const char *filename, uint8_t srcID, uint8_t filter);
void usb_to_recode_init();
struct msi *h264_msi_init_with_mode(uint32_t drv1_from, uint16_t drv1_w, uint16_t drv1_h, uint16_t drv2_from, uint16_t drv2_w, uint16_t drv2_h);
struct msi *gen420_jpg_msi_init(const char *msi_name, uint8_t which_jpg, uint8_t recv_type, uint8_t lock_value, uint8_t queue_value, uint16_t *filter_type, gen420_filter_fn fn);
struct msi *scale1_jpg_msi_init(const char *msi_name, uint8_t which_jpg, uint8_t recv_type, uint8_t lock_value, uint16_t force_type, scale1_filter_fn fn);
#endif