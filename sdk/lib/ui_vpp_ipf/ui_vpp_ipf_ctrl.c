#include "basic_include.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "lib/video/vpp/vpp_dev.h"
#include "ui_vpp_ipf_resource.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

// data申请空间函数
#define STREAM_MALLOC                   av_psram_malloc
#define STREAM_FREE                     av_psram_free
#define STREAM_ZALLOC                   av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC              av_malloc
#define STREAM_LIBC_FREE                av_free
#define STREAM_LIBC_ZALLOC              av_zalloc

#define UI_VPP_IPF_RES_CACHE_NUM		2

struct ui_vpp_ipf_ctrl_priv
{
	struct vpp_device *vpp_dev;
	uint8_t *ui_vpp_ifp_flush_addr;
	uint8_t *ui_vpp_ifp_res_cache[UI_VPP_IPF_RES_CACHE_NUM];
	uint8_t  ui_vpp_ifp_res_cache_num;
	uint8_t  ui_vpp_ipf_flush_flag;
	uint8_t  ui_vpp_ipf_ctrl_en;
};

static struct ui_vpp_ipf_ctrl_priv* g_ui_ipf_priv = NULL;

void ui_vpp_ipf_update_resource(void* img_data)
{
	if (g_ui_ipf_priv == NULL) {
		return ;
	}

	_ipf_img_resource_ *img_src = (_ipf_img_resource_*)img_data;

	if (img_src == NULL) {
		uint32_t flags = disable_irq();
		g_ui_ipf_priv->ui_vpp_ipf_ctrl_en    = 0;
		g_ui_ipf_priv->ui_vpp_ipf_flush_flag = 1;
		enable_irq(flags);
		return ;
	}

	uint16_t width 		= 0;
	uint16_t height 	= 0;
	get_vpp_w_h(&width, &height);

	uint32_t size = img_src->data_size;
	uint8_t *data = g_ui_ipf_priv->ui_vpp_ifp_res_cache[g_ui_ipf_priv->ui_vpp_ifp_res_cache_num];
	
	os_printf("####### ui_vpp_ipf_update_resource = %d \n", size);

	hw_memset(data, 0, (width * height + width * height / 2));
	os_memcpy(data, img_src->data, size);
    sys_dcache_clean_range((uint32_t*)data, size);

	uint32_t flags = disable_irq();
	g_ui_ipf_priv->ui_vpp_ifp_flush_addr 	 = data;
	g_ui_ipf_priv->ui_vpp_ifp_res_cache_num ^= 1;     
	g_ui_ipf_priv->ui_vpp_ipf_ctrl_en    	 = 1;
	g_ui_ipf_priv->ui_vpp_ipf_flush_flag 	 = 1;
	enable_irq(flags);
}

int32_t ui_vpp_ipf_flush_irq(uint32 dev)
{

	struct ui_vpp_ipf_ctrl_priv *ui_ipf_priv = (struct ui_vpp_ipf_ctrl_priv *)dev;

	if(ui_ipf_priv->ui_vpp_ipf_flush_flag)
	{
		ui_ipf_priv->ui_vpp_ipf_flush_flag = 0;

		if(ui_ipf_priv->ui_vpp_ipf_ctrl_en)
		{
			vpp_set_ifp_en(ui_ipf_priv->vpp_dev, 0);
			vpp_set_ifp_addr(ui_ipf_priv->vpp_dev, (uint32_t)ui_ipf_priv->ui_vpp_ifp_flush_addr);
			vpp_set_ifp_en(ui_ipf_priv->vpp_dev, 1);
		}
		else
		{
			vpp_set_ifp_en(ui_ipf_priv->vpp_dev, 0);
		}
	}

	return 0;
}

void ui_vpp_ipf_ctrl_init()
{
	uint16_t width 		= 0;
	uint16_t height 	= 0;

	struct vpp_device *vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);

	g_ui_ipf_priv = (struct ui_vpp_ipf_ctrl_priv*)STREAM_LIBC_ZALLOC(sizeof(struct ui_vpp_ipf_ctrl_priv));

	if (!g_ui_ipf_priv) {
		goto __err_exit;
	}

	get_vpp_w_h(&width, &height);

	for (int i = 0; i < UI_VPP_IPF_RES_CACHE_NUM; i++) {
		g_ui_ipf_priv->ui_vpp_ifp_res_cache[i] = STREAM_MALLOC(width * height + width * height / 2);
		if (g_ui_ipf_priv->ui_vpp_ifp_res_cache[i] == NULL) {
			goto __err_exit;
		}
	}

	g_ui_ipf_priv->vpp_dev = vpp_dev;
	g_ui_ipf_priv->ui_vpp_ifp_res_cache_num = 0;

	vppdone_func_register(VPP_IFP_EN_CTRL, ui_vpp_ipf_flush_irq, (uint32_t)g_ui_ipf_priv);

	os_printf("%s %d\n", __FUNCTION__, __LINE__);

	return ;

__err_exit:

	if (g_ui_ipf_priv) {
		for (int i = 0; i < UI_VPP_IPF_RES_CACHE_NUM; i++) {
			if (g_ui_ipf_priv->ui_vpp_ifp_res_cache[i]) {
				STREAM_FREE(g_ui_ipf_priv->ui_vpp_ifp_res_cache[i]);
				g_ui_ipf_priv->ui_vpp_ifp_res_cache[i] = NULL;
			}
		}	

		STREAM_LIBC_FREE(g_ui_ipf_priv);
		g_ui_ipf_priv = NULL;
	}

	return ;
}

void ui_vpp_ipf_ctrl_deinit()
{
	vppdone_func_unregister(VPP_IFP_EN_CTRL);

	if (g_ui_ipf_priv) {
		for (int i = 0; i < UI_VPP_IPF_RES_CACHE_NUM; i++) {
			if (g_ui_ipf_priv->ui_vpp_ifp_res_cache[i]) {
				STREAM_FREE(g_ui_ipf_priv->ui_vpp_ifp_res_cache[i]);
				g_ui_ipf_priv->ui_vpp_ifp_res_cache[i] = NULL;
			}
		}	

		STREAM_LIBC_FREE(g_ui_ipf_priv);
		g_ui_ipf_priv = NULL;
	}
}


