#ifndef _HG_AUDIO_V0_HW_H
#define _HG_AUDIO_V0_HW_H

#ifdef __cplusplus
    extern "C" {
#endif

/*!
  *DIGI_PATH_CTL
  */
#define LL_AUDIO_DIGI_PATH_CTL_ADC_MODE(x)                  (((x)&0x3)      << (0x1e))
#define LL_AUDIO_DIGI_PATH_CTL_DAC_MODE(x)                  (((x)&0x3)      << (0x1c))
#define LL_AUDIO_DIGI_PATH_CTL_DRC_DIRECT_EN(x)             (((x)&0x1)      << (0x1b))
#define LL_AUDIO_DIGI_PATH_CTL_ASRC_DIRECT_EN(x)            (((x)&0x1)      << (0x1a))
#define LL_AUDIO_DIGI_PATH_CTL_EQ_DIRECT_EN(x)              (((x)&0x1)      << (0x19))
#define LL_AUDIO_DIGI_PATH_CTL_XU_SET(x)                    (((x)&0x7)      << (0xe ))
#define LL_AUDIO_DIGI_PATH_CTL_ALPHA_SEL(x)                 (((x)&0x3)      << (0xc ))
#define LL_AUDIO_DIGI_PATH_CTL_MXS_DWA_EN(x)                (((x)&0x1)      << (0xb ))
#define LL_AUDIO_DIGI_PATH_CTL_VOLEN(x)                     (((x)&0x1)      << (0x5 ))
#define LL_AUDIO_DIGI_PATH_CTL_AGCEN(x)                     (((x)&0x1)      << (0x4 ))
#define LL_AUDIO_DIGI_PATH_CTL_ADCHP(x)                     (((x)&0x1)      << (0x3 ))
#define LL_AUDIO_DIGI_PATH_CTL_DACM(x)                      (((x)&0x1)      << (0x2 ))
#define LL_AUDIO_DIGI_PATH_CTL_ADCEN(x)                     (((x)&0x1)      << (0x1 ))
#define LL_AUDIO_DIGI_PATH_CTL_DACEN(x)                     (((x)&0x1)      << (0x0 ))




/*!
  *DAC_DMA_CTL
  */
#define LL_AUDIO_DAC_DMA_CTL_TH1(x)                         (((x)&0xff)     << (0x18))
#define LL_AUDIO_DAC_DMA_CTL_TH0(x)                         (((x)&0xff)     << (0x10))
#define LL_AUDIO_DAC_DMA_CTL_SKIPDATA(x)                    (((x)&0x3)      << (0x6 ))
#define LL_AUDIO_DAC_DMA_CTL_RELOAD(x)                      (((x)&0x1)      << (0x4 ))
#define LL_AUDIO_DAC_DMA_CTL_DATAMODE(x)                    (((x)&0x3)      << (0x2 ))
#define LL_AUDIO_DAC_DMA_CTL_DMASTOP(x)                     (((x)&0x1)      << (0x1 ))
#define LL_AUDIO_DAC_DMA_CTL_DMASTART(x)                    (((x)&0x1)      << (0x0 ))




/*!
  *DAC_DMA_WCTL
  */
#define LL_AUDIO_DAC_DMA_WCTL_TH1(x)                        (((x)&0xff)     << (0x18))
#define LL_AUDIO_DAC_DMA_WCTL_TH0(x)                        (((x)&0xff)     << (0x10))
#define LL_AUDIO_DAC_DMA_WCTL_RELOAD(x)                     (((x)&0x1)      << (0x4 ))
#define LL_AUDIO_DAC_DMA_WCTL_DMASTOP(x)                    (((x)&0x1)      << (0x1 ))
#define LL_AUDIO_DAC_DMA_WCTL_DMASTART(x)                   (((x)&0x1)      << (0x0 ))




/*!
  *ADC_DMA_CTL
  */
#define LL_AUDIO_ADC_DMA_CTL_TH1(x)                         (((x)&0xff)     << (0x18))
#define LL_AUDIO_ADC_DMA_CTL_TH0(x)                         (((x)&0xff)     << (0x10))
#define LL_AUDIO_ADC_DMA_CTL_RELOAD(x)                      (((x)&0x1)      << (0x4 ))
#define LL_AUDIO_ADC_DMA_CTL_DMASTOP(x)                     (((x)&0x1)      << (0x1 ))
#define LL_AUDIO_ADC_DMA_CTL_DMASTART(x)                    (((x)&0x1)      << (0x0 ))




/*!
  *DAC_DMA_STADR
  */
#define LL_AUDIO_DAC_DMA_STADR_STADR(x)                     (((x)&0xffffffff)<< (0x0 ))




/*!
  *DAC_DMA_WSTADR
  */
#define LL_AUDIO_DAC_DMA_WSTADR_STADR(x)                    (((x)&0xffffffff)<< (0x0 ))




/*!
  *ADC_DMA_STADR
  */
#define LL_AUDIO_ADC_DMA_STADR_STADR(x)                     (((x)&0xffffffff)<< (0x0 ))




/*!
  *DAC_DMA_LEN
  */
#define LL_AUDIO_DAC_DMA_LEN_LEN(x)                         (((x)&0xffff)   << (0x0 ))




/*!
  *DAC_DMA_WLEN
  */
#define LL_AUDIO_DAC_DMA_WLEN_LEN(x)                        (((x)&0xffff)   << (0x0 ))




/*!
  *ADC_DMA_LEN
  */
#define LL_AUDIO_ADC_DMA_LEN_LEN(x)                         (((x)&0xffff)   << (0x0 ))




/*!
  *DAC_SAMP_IDX
  */
#define LL_AUDIO_DAC_SAMP_IDX_IDX(x)                        (((x)&0xffff)   << (0x0 ))




/*!
  *DAC_SAMP_WIDX
  */
#define LL_AUDIO_DAC_SAMP_WIDX_IDX(x)                       (((x)&0xffff)   << (0x0 ))




/*!
  *ADC_SAMP_IDX
  */
#define LL_AUDIO_ADC_SAMP_IDX_IDX(x)                        (((x)&0xffff)   << (0x0 ))




/*!
  *AUDIO_IP
  */
#define LL_AUDIO_AUDIO_IP_DAC_WR_TH1IP(x)                   (((x)&0x1)      << (0x15))
#define LL_AUDIO_AUDIO_IP_DAC_WR_TH0IP(x)                   (((x)&0x1)      << (0x14))
#define LL_AUDIO_AUDIO_IP_DAC_WR_DONEIP(x)                  (((x)&0x1)      << (0x13))
#define LL_AUDIO_AUDIO_IP_CORDIC_OVIP(x)                    (((x)&0x1)      << (0x12))
#define LL_AUDIO_AUDIO_IP_VOL_OVIP(x)                       (((x)&0x1)      << (0x11))
#define LL_AUDIO_AUDIO_IP_HS_DMAOVIP(x)                     (((x)&0x1)      << (0x10))
#define LL_AUDIO_AUDIO_IP_HS_DMAHFIP(x)                     (((x)&0x1)      << (0xf ))
#define LL_AUDIO_AUDIO_IP_CODEC_DMAOVIP(x)                  (((x)&0x1)      << (0xe ))
#define LL_AUDIO_AUDIO_IP_CODEC_DMAHFIP(x)                  (((x)&0x1)      << (0xd ))
#define LL_AUDIO_AUDIO_IP_VAD_WINOVIP(x)                    (((x)&0x1)      << (0xc ))
#define LL_AUDIO_AUDIO_IP_VAD_DMAOVIP(x)                    (((x)&0x1)      << (0xb ))
#define LL_AUDIO_AUDIO_IP_VAD_DMAHFIP(x)                    (((x)&0x1)      << (0xa ))
#define LL_AUDIO_AUDIO_IP_VAD_ZCRIP(x)                      (((x)&0x1)      << (0x9 ))
#define LL_AUDIO_AUDIO_IP_VAD_POWER1IP(x)                   (((x)&0x1)      << (0x8 ))
#define LL_AUDIO_AUDIO_IP_VAD_POWER0IP(x)                   (((x)&0x1)      << (0x7 ))
#define LL_AUDIO_AUDIO_IP_VADIP(x)                          (((x)&0x1)      << (0x6 ))
#define LL_AUDIO_AUDIO_IP_ADCTH1IP(x)                       (((x)&0x1)      << (0x5 ))
#define LL_AUDIO_AUDIO_IP_ADCTH0IP(x)                       (((x)&0x1)      << (0x4 ))
#define LL_AUDIO_AUDIO_IP_ADCDONEIP(x)                      (((x)&0x1)      << (0x3 ))
#define LL_AUDIO_AUDIO_IP_DACTH1IP(x)                       (((x)&0x1)      << (0x2 ))
#define LL_AUDIO_AUDIO_IP_DACTH0IP(x)                       (((x)&0x1)      << (0x1 ))
#define LL_AUDIO_AUDIO_IP_DACDONEIP(x)                      (((x)&0x1)      << (0x0 ))




/*!
  *AUDIO_IE
  */
#define LL_AUDIO_AUDIO_IE_CORDIC_OVIE(x)                    (((x)&0x1)      << (0x12))
#define LL_AUDIO_AUDIO_IE_HS_DMAOVIE(x)                     (((x)&0x1)      << (0x10))
#define LL_AUDIO_AUDIO_IE_HS_DMAHFIE(x)                     (((x)&0x1)      << (0xf ))
#define LL_AUDIO_AUDIO_IE_CODEC_DMAOVIE(x)                  (((x)&0x1)      << (0xe ))
#define LL_AUDIO_AUDIO_IE_CODEC_DMAHFIE(x)                  (((x)&0x1)      << (0xd ))
#define LL_AUDIO_AUDIO_IE_VAD_WINOVIE(x)                    (((x)&0x1)      << (0xc ))
#define LL_AUDIO_AUDIO_IE_VAD_DMAOVIE(x)                    (((x)&0x1)      << (0xb ))
#define LL_AUDIO_AUDIO_IE_VAD_DMAHFIE(x)                    (((x)&0x1)      << (0xa ))
#define LL_AUDIO_AUDIO_IE_VAD_ZCRIE(x)                      (((x)&0x1)      << (0x9 ))
#define LL_AUDIO_AUDIO_IE_VAD_POWER1IE(x)                   (((x)&0x1)      << (0x8 ))
#define LL_AUDIO_AUDIO_IE_VAD_POWER0IE(x)                   (((x)&0x1)      << (0x7 ))
#define LL_AUDIO_AUDIO_IE_VADIE(x)                          (((x)&0x1)      << (0x6 ))
#define LL_AUDIO_AUDIO_IE_ADCTH1IE(x)                       (((x)&0x1)      << (0x5 ))
#define LL_AUDIO_AUDIO_IE_ADCTH0IE(x)                       (((x)&0x1)      << (0x4 ))
#define LL_AUDIO_AUDIO_IE_ADCDONEIE(x)                      (((x)&0x1)      << (0x3 ))
#define LL_AUDIO_AUDIO_IE_DACTH1IE(x)                       (((x)&0x1)      << (0x2 ))
#define LL_AUDIO_AUDIO_IE_DACTH0IE(x)                       (((x)&0x1)      << (0x1 ))
#define LL_AUDIO_AUDIO_IE_DACDONEIE(x)                      (((x)&0x1)      << (0x0 ))




/*!
  *DAC_FLT_CTL
  */
#define LL_AUDIO_DAC_FLT_CTL_SDM_DC(x)                      (((x)&0xff)     << (0x18))
#define LL_AUDIO_DAC_FLT_CTL_SDM_GAIN(x)                    (((x)&0xff)     << (0x10))
#define LL_AUDIO_DAC_FLT_CTL_HK_SEL(x)                      (((x)&0x1)      << (0xf ))
#define LL_AUDIO_DAC_FLT_CTL_MASH_SEL(x)                    (((x)&0x1)      << (0xe ))
#define LL_AUDIO_DAC_FLT_CTL_ANA_CLK_SEL(x)                 (((x)&0x1)      << (0xd ))
#define LL_AUDIO_DAC_FLT_CTL_PDM_OUT_EN(x)                  (((x)&0x1)      << (0xc ))
#define LL_AUDIO_DAC_FLT_CTL_NOISE_WIDTH(x)                 (((x)&0x1f)     << (0x7 ))
#define LL_AUDIO_DAC_FLT_CTL_NOSIE_EN(x)                    (((x)&0x1)      << (0x6 ))
#define LL_AUDIO_DAC_FLT_CTL_OSR_SEL(x)                     (((x)&0x3)      << (0x2 ))
#define LL_AUDIO_DAC_FLT_CTL_BAND_SEL(x)                    (((x)&0x3)      << (0x0 ))




/*!
  *DAC_MIX_CTL
  */
#define LL_AUDIO_DAC_MIX_CTL_RGAIN(x)                       (((x)&0xff)     << (0x8 ))
#define LL_AUDIO_DAC_MIX_CTL_LGAIN(x)                       (((x)&0xff)     << (0x0 ))




/*!
  *DAC_VOL_CTL0
  */
#define LL_AUDIO_DAC_VOL_CTL0_D_VOL(x)                      (((x)&0x7fff)   << (0x11))
#define LL_AUDIO_DAC_VOL_CTL0_S_VOL(x)                      (((x)&0x7fff)   << (0x2 ))
#define LL_AUDIO_DAC_VOL_CTL0_VOL_DIR(x)                    (((x)&0x1)      << (0x1 ))
#define LL_AUDIO_DAC_VOL_CTL0_VOL_KICK(x)                   (((x)&0x1)      << (0x0 ))




/*!
  *DAC_VOL_CTL1
  */
#define LL_AUDIO_DAC_VOL_CTL1_DAC_VOL(x)                    (((x)&0x7fff)   << (0x4 ))
#define LL_AUDIO_DAC_VOL_CTL1_VOL_STEP(x)                   (((x)&0x3)      << (0x2 ))
#define LL_AUDIO_DAC_VOL_CTL1_VOL_SAMP(x)                   (((x)&0x3)      << (0x0 ))




/*!
  *ADC_FLT_CTL
  */
#define LL_AUDIO_ADC_FLT_CTL_ADCGAIN(x)                     (((x)&0xff)     << (0x18))
#define LL_AUDIO_ADC_FLT_CTL_XU_SET(x)                      (((x)&0x7)      << (0x15))
#define LL_AUDIO_ADC_FLT_CTL_SMP_CYC(x)                     (((x)&0x3)      << (0x13))
#define LL_AUDIO_ADC_FLT_CTL_ANA_CLK_SEL(x)                 (((x)&0x1)      << (0xc ))
#define LL_AUDIO_ADC_FLT_CTL_ALPHA_SET(x)                   (((x)&0x3)      << (0x9 ))
#define LL_AUDIO_ADC_FLT_CTL_MXS_DWA_EN(x)                  (((x)&0x1)      << (0x8 ))
#define LL_AUDIO_ADC_FLT_CTL_MXS_DWA_CLK_EN(x)              (((x)&0x1)      << (0x7 ))
#define LL_AUDIO_ADC_FLT_CTL_NOSIE_EN(x)                    (((x)&0x1)      << (0x6 ))
#define LL_AUDIO_ADC_FLT_CTL_X2_MODE_EN(x)                  (((x)&0x1)      << (0x4 ))
#define LL_AUDIO_ADC_FLT_CTL_OSR_SEL(x)                     (((x)&0x3)      << (0x2 ))
#define LL_AUDIO_ADC_FLT_CTL_BAND_SEL(x)                    (((x)&0x3)      << (0x0 ))




/*!
  *DAC_ANA_CTL
  */
#define LL_AUDIO_DAC_ANA_CTL_TESTEN(x)                      (((x)&0x1)      << (0x1f))
#define LL_AUDIO_DAC_ANA_CTL_SELVCM(x)                      (((x)&0x7f)     << (0x14))
#define LL_AUDIO_DAC_ANA_CTL_DAC_EN(x)                      (((x)&0x1)      << (0x13))
#define LL_AUDIO_DAC_ANA_CTL_DACOUTPD(x)                    (((x)&0x1)      << (0x12))
#define LL_AUDIO_DAC_ANA_CTL_EN_COREOP(x)                   (((x)&0x1)      << (0x11))
#define LL_AUDIO_DAC_ANA_CTL_EN_OUTPUTOP(x)                 (((x)&0x1)      << (0x10))
#define LL_AUDIO_DAC_ANA_CTL_EN_VCM(x)                      (((x)&0x1)      << (0xf ))
#define LL_AUDIO_DAC_ANA_CTL_MUTE(x)                        (((x)&0x1)      << (0xe ))
#define LL_AUDIO_DAC_ANA_CTL_VCMPD(x)                       (((x)&0x1)      << (0xd ))
#define LL_AUDIO_DAC_ANA_CTL_GSET(x)                        (((x)&0x3)      << (0xb ))
#define LL_AUDIO_DAC_ANA_CTL_GSEL(x)                        (((x)&0x7)      << (0x8 ))
#define LL_AUDIO_DAC_ANA_CTL_ISET(x)                        (((x)&0xff)     << (0x0 ))




/*!
  *ADC_ANA_CTL0
  */
#define LL_AUDIO_ADC_ANA_CTL0_SEL_REFSOURCE(x)              (((x)&0x3)      << (0x1e))
#define LL_AUDIO_ADC_ANA_CTL0_EN_VCMLPF(x)                  (((x)&0x1)      << (0x1d))
#define LL_AUDIO_ADC_ANA_CTL0_SW_VCM2PAD(x)                 (((x)&0x1)      << (0x1c))
#define LL_AUDIO_ADC_ANA_CTL0_EN_V2I(x)                     (((x)&0x1)      << (0x1b))
#define LL_AUDIO_ADC_ANA_CTL0_EN_LDO27(x)                   (((x)&0x1)      << (0x1a))
#define LL_AUDIO_ADC_ANA_CTL0_EN_BG(x)                      (((x)&0x1)      << (0x19))
#define LL_AUDIO_ADC_ANA_CTL0_SEL_VCC27AU(x)                (((x)&0x7)      << (0x16))
#define LL_AUDIO_ADC_ANA_CTL0_SEL_VREF100(x)                (((x)&0xf)      << (0x12))
#define LL_AUDIO_ADC_ANA_CTL0_SEL_VCMAU(x)                  (((x)&0xf)      << (0xe ))
#define LL_AUDIO_ADC_ANA_CTL0_TEST_KICK_P(x)                (((x)&0x1)      << (0xd ))
#define LL_AUDIO_ADC_ANA_CTL0_TEST_KICK_N(x)                (((x)&0x1)      << (0xc ))
#define LL_AUDIO_ADC_ANA_CTL0_SW_PGA2ADC(x)                 (((x)&0x1)      << (0xb ))
#define LL_AUDIO_ADC_ANA_CTL0_SW_PAD2ADC(x)                 (((x)&0x1)      << (0xa ))
#define LL_AUDIO_ADC_ANA_CTL0_EN_AUADC(x)                   (((x)&0x1)      << (0x9 ))
#define LL_AUDIO_ADC_ANA_CTL0_EN_AUADCBIAS(x)               (((x)&0x1)      << (0x8 ))
#define LL_AUDIO_ADC_ANA_CTL0_SEL_LDO11VOLT(x)              (((x)&0xf)      << (0x4 ))
#define LL_AUDIO_ADC_ANA_CTL0_SEL_SDDIV(x)                  (((x)&0x3)      << (0x2 ))
#define LL_AUDIO_ADC_ANA_CTL0_SEL_LDO11R1(x)                (((x)&0x3)      << (0x0 ))




/*!
  *ADC_ANA_CTL1
  */
#define LL_AUDIO_ADC_ANA_CTL1_TEST_OK_N(x)                  (((x)&0x1)      << (0x1f))
#define LL_AUDIO_ADC_ANA_CTL1_TEST_OK_P(x)                  (((x)&0x1)      << (0x1e))
#define LL_AUDIO_ADC_ANA_CTL1_TEST_EN(x)                    (((x)&0x1)      << (0x0 ))




/*!
  *ADC_ANA_CTL2
  */
#define LL_AUDIO_ADC_ANA_CTL2_RESERVED(x)                   (((x)&0xffffffff)<< (0x0 ))




/*!
  *MIC_ANA_CTL0
  */
#define LL_AUDIO_MIC_ANA_CTL0_EN_PGASEM(x)                  (((x)&0x1)      << (0x8 ))
#define LL_AUDIO_MIC_ANA_CTL0_EN_PGADFM(x)                  (((x)&0x1)      << (0x7 ))
#define LL_AUDIO_MIC_ANA_CTL0_SATOPGA(x)                    (((x)&0x7)      << (0x4 ))
#define LL_AUDIO_MIC_ANA_CTL0_SEL_PGA2RIN(x)                (((x)&0x3)      << (0x2 ))
#define LL_AUDIO_MIC_ANA_CTL0_SEL_PGA1RIN(x)                (((x)&0x3)      << (0x0 ))




/*!
  *MIC_ANA_CTL1
  */
#define LL_AUDIO_MIC_ANA_CTL1_RESERVED(x)                   (((x)&0xffffffff)<< (0x0 ))




/*!
  *VAD_CTL0
  */
#define LL_AUDIO_VAD_CTL0_SIGN_CALC_EN(x)                   (((x)&0x1)      << (0xf ))
#define LL_AUDIO_VAD_CTL0_AUTO_CLR_EN(x)                    (((x)&0x1)      << (0xe ))
#define LL_AUDIO_VAD_CTL0_WAIT_SPAC_TIME(x)                 (((x)&0xff)     << (0x6 ))
#define LL_AUDIO_VAD_CTL0_ZCR_OV0_EN(x)                     (((x)&0x1)      << (0x5 ))
#define LL_AUDIO_VAD_CTL0_POWER_OV1_EN(x)                   (((x)&0x1)      << (0x4 ))
#define LL_AUDIO_VAD_CTL0_POWER_OV0_EN(x)                   (((x)&0x1)      << (0x3 ))
#define LL_AUDIO_VAD_CTL0_AUTO_MODE_EN(x)                   (((x)&0x1)      << (0x2 ))
#define LL_AUDIO_VAD_CTL0_VAD_KICK(x)                       (((x)&0x1)      << (0x1 ))
#define LL_AUDIO_VAD_CTL0_VADEN(x)                          (((x)&0x1)      << (0x0 ))




/*!
  *VAD_CTL1
  */
#define LL_AUDIO_VAD_CTL1_POWER_TH1(x)                      (((x)&0xffff)   << (0x10))
#define LL_AUDIO_VAD_CTL1_POWER_TH0(x)                      (((x)&0xffff)   << (0x0 ))




/*!
  *VAD_CTL2
  */
#define LL_AUDIO_VAD_CTL2_ZCR_TH0(x)                        (((x)&0xff)     << (0x18))
#define LL_AUDIO_VAD_CTL2_WINDOW_NUM(x)                     (((x)&0xff)     << (0x10))
#define LL_AUDIO_VAD_CTL2_WINDOW_LEN(x)                     (((x)&0xffff)   << (0x0 ))




/*!
  *VAD_DMA_CTL
  */
#define LL_AUDIO_VAD_DMA_CTL_DMA_SHITF(x)                   (((x)&0xffff)   << (0x0 ))




/*!
  *VAD_DMA_STADR
  */
#define LL_AUDIO_VAD_DMA_STADR_DMA_STADR(x)                 (((x)&0xffffffff)<< (0x0 ))




/*!
  *VAD_DMA_CNT
  */
#define LL_AUDIO_VAD_DMA_CNT_ZCR_RES(x)                     (((x)&0xffff)   << (0x10))
#define LL_AUDIO_VAD_DMA_CNT_DMA_CNT(x)                     (((x)&0xffff)   << (0x0 ))




/*!
  *VAD_STA
  */
#define LL_AUDIO_VAD_STA_POWER_RES_DB(x)                    (((x)&0xffffffff)<< (0x0 ))




/*!
  *CORDIC_CTL0
  */
#define LL_AUDIO_CORDIC_CTL0_THETA(x)                       (((x)&0xffff)   << (0x1 ))
#define LL_AUDIO_CORDIC_CTL0_CORDIC_KICK(x)                 (((x)&0x1)      << (0x0 ))




/*!
  *CORDIC_STA
  */
#define LL_AUDIO_CORDIC_STA_COS_DATA(x)                     (((x)&0xffff)   << (0x10))
#define LL_AUDIO_CORDIC_STA_SIN_DATA(x)                     (((x)&0xffff)   << (0x0 ))




/*!
  *EQ_CON0
  */
#define LL_AUDIO_EQ_CON0_EQ_GAIN(x)                         (((x)&0x1fff)   << (0xd ))
#define LL_AUDIO_EQ_CON0_EQ_COEFF_SW(x)                     (((x)&0x1)      << (0xc ))
#define LL_AUDIO_EQ_CON0_EQ_ROUND_MODE(x)                   (((x)&0x1)      << (0xb ))
#define LL_AUDIO_EQ_CON0_EQ_DSM_EN(x)                       (((x)&0x3ff)    << (0x1 ))
#define LL_AUDIO_EQ_CON0_EQ_EN(x)                           (((x)&0x1)      << (0x0 ))




/*!
  *EQ_CON1
  */
#define LL_AUDIO_EQ_CON1_EQ_COEFF_RD(x)                     (((x)&0x1)      << (0x1f))
#define LL_AUDIO_EQ_CON1_EQ_COEFF_WR(x)                     (((x)&0x1)      << (0x1e))
#define LL_AUDIO_EQ_CON1_EQ_COEFF_WDATA(x)                  (((x)&0x3fffff) << (0x8 ))
#define LL_AUDIO_EQ_CON1_EQ_COEFF_SEL(x)                    (((x)&0x1)      << (0x7 ))
#define LL_AUDIO_EQ_CON1_EQ_ADDR(x)                         (((x)&0x7f)     << (0x0 ))




/*!
  *EQ_CON2
  */
#define LL_AUDIO_EQ_CON2_EQ_DONE(x)                         (((x)&0x1)      << (0x1f))
#define LL_AUDIO_EQ_CON2_EQ_SW_DONE(x)                      (((x)&0x1)      << (0x1e))
#define LL_AUDIO_EQ_CON2_EQ_COEFF_NUM(x)                    (((x)&0x1)      << (0x1d))
#define LL_AUDIO_EQ_CON2_EQ_COEFF_RDATA(x)                  (((x)&0x3fffff) << (0x0 ))




/*!
  *ASRC_CON0
  */
#define LL_AUDIO_ASRC_CON0_SAMPLE_OFS(x)                    (((x)&0x3fffff) << (0xa ))
#define LL_AUDIO_ASRC_CON0_SAMPLE_CNT(x)                    (((x)&0x1ff)    << (0x1 ))
#define LL_AUDIO_ASRC_CON0_ASRC_EN(x)                       (((x)&0x1)      << (0x0 ))




/*!
  *ASRC_STA
  */
#define LL_AUDIO_ASRC_STA_FIFO_FULL(x)                      (((x)&0x1)      << (0x1 ))
#define LL_AUDIO_ASRC_STA_FIFO_EMPTY(x)                     (((x)&0x1)      << (0x0 ))




/*!
  *DRC_CON0
  */
#define LL_AUDIO_DRC_CON0_KNEE1_OP(x)                       (((x)&0x7f)     << (0x19))
#define LL_AUDIO_DRC_CON0_KNEE1_IP(x)                       (((x)&0x1f)     << (0x14))
#define LL_AUDIO_DRC_CON0_NOISE_SLOPE(x)                    (((x)&0x3)      << (0x12))
#define LL_AUDIO_DRC_CON0_SLOPE1(x)                         (((x)&0x7)      << (0xf ))
#define LL_AUDIO_DRC_CON0_SLOPE0(x)                         (((x)&0x7)      << (0xc ))
#define LL_AUDIO_DRC_CON0_RC_SET(x)                         (((x)&0x1f)     << (0x7 ))
#define LL_AUDIO_DRC_CON0_ATK_SET(x)                        (((x)&0x1f)     << (0x2 ))
#define LL_AUDIO_DRC_CON0_DRC_EN(x)                         (((x)&0x1)      << (0x0 ))




/*!
  *DRC_CON1
  */
#define LL_AUDIO_DRC_CON1_NOISE_MIN_GAIN(x)                 (((x)&0x1f)     << (0xf ))
#define LL_AUDIO_DRC_CON1_MAX_GAIN(x)                       (((x)&0x3)      << (0xd ))
#define LL_AUDIO_DRC_CON1_MIN_GAIN(x)                       (((x)&0x7)      << (0xa ))
#define LL_AUDIO_DRC_CON1_KNEE2_OP(x)                       (((x)&0x1f)     << (0x5 ))
#define LL_AUDIO_DRC_CON1_KNEE2_IP(x)                       (((x)&0x1f)     << (0x0 ))



struct hg_audio_hw_v0 {
    __IO uint32 DIGI_PATH_CTL        ; // 0x0         
    __IO uint32 DAC_DMA_CTL          ; // 0x4         
    __IO uint32 DAC_DMA_STADR        ; // 0x8  
    __IO uint32 DAC_DMA_LEN          ; // 0xc         
    __IO uint32 DAC_SAMP_IDX         ; // 0x10 
    __IO uint32 DAC_DMA_WCTL         ; // 0x14        
                                      
    __IO uint32 DAC_DMA_WSTADR       ; // 0x18 
    __IO uint32 DAC_DMA_WLEN         ; // 0x1c        
    __IO uint32 DAC_SAMP_WIDX        ; // 0x20 
                                      
    __IO uint32 DAC_FLT_CTL          ; // 0x24        
    __IO uint32 DAC_MIX_CTL          ; // 0x28 
    __IO uint32 DAC_VOL_CTL0         ; // 0x2c 
    __IO uint32 DAC_VOL_CTL1         ; // 0x30 
    
    __IO uint32 reserve0[51]         ; // 0x34-0xfc
    
    __IO uint32 ADC_DMA_CTL          ; // 0x100       
    __IO uint32 ADC_DMA_STADR        ; // 0x104
    __IO uint32 ADC_DMA_LEN          ; // 0x108       
    __IO uint32 ADC_SAMP_IDX         ; // 0x10c
    __IO uint32 ADC_FLT_CTL          ; // 0x110       
    __IO uint32 AUDIO_IP             ; // 0x114       
    __IO uint32 AUDIO_IE             ; // 0x118       
    __IO uint32 ADC_RCCAL_CON        ; // 0x11c
    __IO uint32 ADC_RCCAL_STA        ; // 0x120
    
    __IO uint32 reserve1[55]         ; // 0x124-0x1fc
    
    __IO uint32 ADC_ANA_CTL0       ; // 0x200
    __IO uint32 ADC_ANA_CTL1       ; // 0x204
    __IO uint32 AMP_ANA_CTL0       ; // 0x208
    __IO uint32 AMP_ANA_CTL1       ; // 0x20c
    __IO uint32 DAC_ANA_CTL0       ; // 0x210
    __IO uint32 DAC_ANA_CTL1       ; // 0x214
    __IO uint32 PA_ANA_CTL0        ; // 0x218
    __IO uint32 PA_ANA_CTL1        ; // 0x21c
    __IO uint32 BIAS_ANA_CTL0      ; // 0x220
    __IO uint32 BIAS_ANA_CTL1      ; // 0x224
    
    __IO uint32 reserve2[54]         ; // 0x228-0x2fc
    
    __IO uint32 VAD_CTL0             ; // 0x300 
    __IO uint32 VAD_CTL1             ; // 0x304 
    __IO uint32 VAD_CTL2             ; // 0x308 
    __IO uint32 VAD_DMA_CTL          ; // 0x30c 
    __IO uint32 VAD_DMA_STADR        ; // 0x310 
    __IO uint32 VAD_DMA_CNT          ; // 0x314 
    __IO uint32 VAD_STA              ; // 0x318 
    
    __IO uint32 reserve3[281]        ; // 0x31c-0x77c
    
    __IO uint32 INCGAINTH3_0         ; // 0x780 
    __IO uint32 INCGAINTH5_4         ; // 0x784 
    __IO uint32 REDGAINTH3_0         ; // 0x788 
    __IO uint32 REDGAINTH5_4         ; // 0x78c 
    __IO uint32 GAINVAL3_0           ; // 0x790 
    __IO uint32 GAINVAL6_4           ; // 0x794 
    __IO uint32 AGC_CTL0             ; // 0x798 
    __IO uint32 AGC_CTL1             ; // 0x79c 
    __IO uint32 CODEC_CTL            ; // 0x7a0 
    __IO uint32 CODEC_DMA_STADR      ; // 0x7a4 
    __IO uint32 CODEC_DMA_DSTADR     ; // 0x7a8 
    __IO uint32 CODEC_DMA_LEN        ; // 0x7ac 
    __IO uint32 CODEC_DMA_CNT        ; // 0x7b0 
    __IO uint32 HS_CTL0              ; // 0x7b4  
    __IO uint32 HS_CTL1              ; // 0x7b8 
    __IO uint32 HS_DMA_STADR         ; // 0x7bc 
    __IO uint32 HS_DMA_DSTADR        ; // 0x7c0 
    __IO uint32 HS_DMA_LEN           ; // 0x7c4 
    __IO uint32 HS_DMA_CNT           ; // 0x7c8
};


struct hg_aueq_hw_v0 {
    __IO uint32 EQ_CON0;
    __IO uint32 EQ_CON1;
    __IO uint32 EQ_CON2;
};

struct hg_auasrc_hw_v0 {
    __IO uint32 ASRC_CON0;
    __IO uint32 ASRC_STA;
}; 

struct hg_audrc_hw_v0 {
    __IO uint32 DRC_CON0;
    __IO uint32 DRC_CON1;
};




#ifdef __cplusplus
}
#endif

#endif /* _HG_AUDIO_V0_HW_H */
