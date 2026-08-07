#ifndef __UI_VPP_IPF_CTRL_H_
#define __UI_VPP_IPF_CTRL_H_


void ui_vpp_ipf_update_resource(void* img_data);
int32_t ui_vpp_ipf_flush_irq(uint32 dev);
void ui_vpp_ipf_ctrl_init();
void ui_vpp_ipf_ctrl_deinit();

#endif