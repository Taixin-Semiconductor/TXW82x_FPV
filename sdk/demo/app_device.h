#ifndef __APP_DEVICE_H
#define __APP_DEVICE_H

#include "basic_include.h"
#include "chip/txw82x/sysctrl.h"
#include "hal/i2c.h"
#include "dev/audio/components/hg_audio_v0.h"
#include "dev/csc/hgcsc.h"
#include "dev/csi/hgdvp.h"
#include "dev/dma2d/hg_dma2d_v0.h"
#include "dev/dual/hgdual_org.h"
#include "dev/emac/hg_gmac_eva_v2.h"
#include "dev/gen/hggen420.h"
#include "dev/gen/hggen422.h"
#include "dev/h264/hg264.h"
#include "dev/i2c/hgi2c_v1.h"
#include "dev/i2s/hgi2s_v0.h"
#include "dev/isp/hgisp_v0.h"
#include "dev/jpg/hgjpg.h"
#include "dev/lcdc/hgdsi.h"
#include "dev/lcdc/hglcdc.h"
#include "dev/mipi_csi/hgmipi_csi.h"
#include "dev/osd_enc/hgosd_enc.h"
#include "dev/para_in/hgpara_in.h"
#include "dev/pdm/hgpdm_v0.h"
#include "dev/prc/hgprc.h"
#include "dev/pwm/hgpwm_v0.h"
#include "dev/scale/hgscale.h"
#include "dev/usb/hgusb20_v1_dev_api.h"
#include "dev/usb/usb11_v0/hgusb11_v0_dev_api.h"
#include "dev/usb/usb11_v0/hgusb11_v0_host_api.h"
#include "dev/vpp/hgvpp.h"
#include "lib/audio/audio_code/audio_code.h"
#include "decode/decode.h"
#include "lib/net/ethphy/eth_mdio_bus.h"
#include "lib/net/ethphy/eth_phy.h"
#include "lib/sdhost/sdhost.h"
#include "dev/spi/hgspi_v3.h"
#include "audio/resample/resample.h"
#include "audio/audio_proc/audio_proc.h"

extern struct hgspi_v3            spi0;
extern struct hgi2c_v1            iic1;
extern struct hgi2c_v1            iic2;
extern struct hgisp_v0            isp;
extern struct hgdvp               dvp;
extern struct hgmipi_csi          csi0;
extern struct hgmipi_csi          csi1;
extern struct hgjpg               jpg0;
extern struct hgjpg               jpg1;
extern struct hg264               h264;
extern struct hglcdc              lcdc;
extern struct hgdsi               dsic;
extern struct hgdma2d_v0          dma2d;
extern struct hgosd               osdenc;
extern struct hgpwm_v0            pwm;
extern struct hgsdh               sdh;
extern struct hgusb20_dev         usb20_dev;
extern struct hgusb11_dev         usb11_dev;
extern struct hgusb11_host        usb11_host;
extern struct hgi2s_v0            i2s0;
extern struct hgi2s_v0            i2s1;
extern struct hgpdm_v0            pdm;
extern struct hg_audio_v0         auadc;
extern struct hg_audio_v0         audac;
extern struct hg_audio_v0         auasrc;
extern struct hg_audio_v0         aueq;
extern struct hg_audio_v0         aufade;
extern struct ethernet_mdio_bus   mdio_bus0;
extern struct ethernet_phy_device ethernet_phy0;
extern struct hg_gmac_eva_v2      gmac;
extern struct hgvpp               vpp;
extern struct hggen420            gen420;
extern struct hggen422            gen422;
extern struct hgprc               prc;
extern struct hgcsc               csc;
extern struct hgdual              dual;
extern struct hgscale             scale1;
extern struct hgscale             scale2;
extern struct hgscale             scale3;
extern struct hgpara_in           para_in;
extern struct hgacodec_v1         aacdec;
extern struct hgacodec_v1         aacenc;
extern struct hgacodec_v1         alawdec;
extern struct hgacodec_v1         alawenc;
extern struct hgacodec_v1         amrnbdec;
extern struct hgacodec_v1         amrwbdec;
extern struct hgacodec_v1         mp3dec;
extern struct hgacodec_v1         opusdec;
extern struct hgacodec_v1         opusenc;
extern struct hgacodec_v1         ulawdec;
extern struct hgacodec_v1         ulawenc;
extern struct hgacodec_v1         pcmdec;
extern struct hgaures_v1          aures;
extern struct hgauproc_v1         auproc;
extern struct hgauchange_v1       auchange;


#endif  /* __APP_DEVICE_H */