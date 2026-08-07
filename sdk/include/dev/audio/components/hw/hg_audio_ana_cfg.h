#ifndef __HG_AUDIO_ANA_CFG_H
#define __HG_AUDIO_ANA_CFG_H

#ifdef __cplusplus
extern "C" {
#endif 


//DAC+PGA
#define DAC_DRIVER_VERSION0                     (0x0)
//1bit PWM
#define DAC_DRIVER_VERSION1                     (0x1)
//DAC
#define DAC_DRIVER_VERSION2                     (0x2)


#define __AUDIO_ADC_ANA_CFG_SAMPLE_RATE_44_1K   (1)
#define __AUDIO_ADC_ANA_CFG_SAMPLE_RATE_48K     (2)
#define __AUDIO_ADC_ANA_CFG_SAMPLE_RATE_32K     (3)
#define __AUDIO_ADC_ANA_CFG_SAMPLE_RATE_16K     (4)
#define __AUDIO_ADC_ANA_CFG_SAMPLE_RATE_8K      (5)
#define __AUDIO_ADC_ANA_CFG_SAMPLE_RATE_11_025K (6)
#define __AUDIO_ADC_ANA_CFG_SAMPLE_RATE_22_05K  (7)
#define __AUDIO_ADC_ANA_CFG_SAMPLE_RATE_24K     (8)
#define __AUDIO_ADC_ANA_CFG_SAMPLE_RATE_12K     (10)

/*!
 * mic type: difference
 */
#define __AUDIO_ADC_ANA_CFG_MIC_TYPE_DIFF       (1)

/*!
 * mic type: single ended
 */
#define __AUDIO_ADC_ANA_CFG_MIC_TYPE_SINGE      (2)


struct __audio_adc_ana_cfg {
    uint16 sample_rate;
    uint16 mic_type;
};

struct __audio_dac_ana_cfg {
    uint16 driver_version;
};

struct __audio_comm_ana_cfg {
	union {
		uint32 comm;
		struct {
			uint32 audio_ref1 :6,
				   audio_ref2 :6,
				   audio_c2   :6;
		}comm_bits;
	};
};

struct __audio_dac_digital_cfg {
   uint32 target_vol;
};
struct __audio_dac_digital_cfg  dac_digital_cfg;

struct __audio_ana_cfg {
    struct __audio_adc_ana_cfg  adc;
    struct __audio_dac_ana_cfg  dac;
    struct __audio_comm_ana_cfg comm;
};


void audio_ana_config_power_on(void *cfg);
void audio_ana_config_power_off(void *cfg);
void audio_ana_config_adc_on(void *cfg);
void audio_ana_config_adc_off(void *cfg);
void audio_ana_config_dac_on(void *cfg);
void audio_ana_config_dac_off(void *cfg);
void audio_ana_config_dac_voice_off(void *cfg);
void audio_ana_config_dac_voice_on(void *cfg);
void audio_ana_config_rc_cali(void *cfg);



#ifdef __cplusplus
}
#endif


#endif