#ifndef _HG_AUDIO_V0_H_
#define _HG_AUDIO_V0_H_

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Type of device
 */
#define AUDIO_TYPE_AUADC    BIT(0)
#define AUDIO_TYPE_AUDAC    BIT(1)
#define AUDIO_TYPE_AUHS     BIT(2)
#define AUDIO_TYPE_AUVAD    BIT(3)
#define AUDIO_TYPE_AUALAW   BIT(4)
#define AUDIO_TYPE_AUFADE   BIT(5)
#define AUDIO_TYPE_AUEQ     BIT(6)
#define AUDIO_TYPE_AUASRC   BIT(7)
#define AUDIO_TYPE_AUDRC    BIT(8)



/** @brief AUDIO operating handle structure
  * @{
  */


/*!
 * Common dat areas struct
 */
union __comm_dat{
    uint32 comm;
    struct {
        uint32  adc_en              :1,
                dac_en              :1,
                hs_en               :1,
                alaw_en             :1,
                vad_en              :1,
                aufade_en           :1,
                eq_en               :1,
                asrc_en             :1,
                drc_en              :1,
                ana_power_on        :1,
                ana_power_adc_on    :1,
                ana_power_dac_on    :1,
                ana_rc_cali_ok      :1,
                ana_driver_version  :2;
    } comm_bits;
};

struct hg_audio_v0 {
    /* Pointer of device */
    struct dev_obj      dev;
    
    uint32              hw;

    /* common areas */
    union __comm_dat    comm_dat;
    /* Pointer of common areas */
    void               *p_comm; 

    void               *irq_hdl;
    uint32              irq_data;
    uint16              irq_num;

    /* Type of device */
    uint16              dev_type;
};


int32 hg_audio_v0_attach(uint32 dev_id, struct hg_audio_v0 *audio);



#ifdef __cplusplus
}
#endif

#endif /* _HGI2S_V0_H_ */
