#ifndef _HAL_PDM_H_
#define _HAL_PDM_H_
#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief pdm sample frequency type
  */
enum pdm_sample_freq {
    /*! Compatible version: V0
     *@Describe:
     * pdm sample frequency: 8K
     */
    PDM_SAMPLE_FREQ_8K,
    /*! Compatible version: V0
     *@Describe:
     * pdm sample frequency: 16K
     */
    PDM_SAMPLE_FREQ_16K,
    /*! Compatible version: V0
     *@Describe:
     * pdm sample frequency: 32K
     */
    PDM_SAMPLE_FREQ_32K,
    /*! Compatible version: V0
     *@Describe:
     * pdm sample frequency: 48K
     */
    PDM_SAMPLE_FREQ_48K,
};

/**
  * @brief pdm channel of working mode type
  */
enum pdm_channel {
    /*! Compatible version: V0
     *@Describe:
     * channel of working mode: left channel
     */
    PDM_CHANNEL_LEFT,
    /*! Compatible version: V0
     *@Describe:
     * channel of working mode: right channel
     */
    PDM_CHANNEL_RIGHT,
    /*! Compatible version: V0
     *@Describe:
     * channel of working mode: stereo channel
     */
    PDM_CHANNEL_STEREO,
};

/**
  * @brief pdm interrupt type
  */
enum pdm_irq_flag {
    /*! Compatible version: V0
     *@Describe:
     * pdm module receive data half of buf len
     */
    PDM_IRQ_FLAG_DMA_HF = BIT(0),
    /*! Compatible version: V0
     *@Describe:
     * pdm module receive data full of buf len
     */
    PDM_IRQ_FLAG_DMA_OV = BIT(1),
};

/**
  * @brief pdm ioctl cmd
  */
enum pdm_ioctl_cmd {
    /*! Compatible version: V0
     *@Describe:
     * Left and right channels interchange
     */
    PDM_IOCTL_CMD_LR_CHANNEL_INTERCHANGE,
};


/* User interrupt handle */
typedef void (*pdm_irq_hdl)(uint32 irq, uint32 irq_data);

/* PDM api for user */
struct pdm_device {
    struct dev_obj dev;
};

struct pdm_hal_ops{
    struct devobj_ops ops;
    int32 (*open       )(struct pdm_device *pdm, enum pdm_sample_freq freq, enum pdm_channel channel);
    int32 (*read       )(struct pdm_device *pdm, void *buf, uint32 len);
    int32 (*close      )(struct pdm_device *pdm);
    int32 (*request_irq)(struct pdm_device *pdm, enum pdm_irq_flag, pdm_irq_hdl irq_hdl, uint32 data);
    int32 (*release_irq)(struct pdm_device *pdm, enum pdm_irq_flag irq_flag);
    int32 (*ioctl      )(struct pdm_device *pdm, enum pdm_ioctl_cmd cmd, uint32 param);
};

/*  PDM API functions */

/**
 * @brief pdm_open
 *
 * @Compatible version: V0
 *
 * @note  This function must be called before any pdm driver operations.
 *
 * @param pdm       The address of pdm device in the RTOS.
 * @param freq      PDM sample frequency setting        - see "enum pdm_sample_freq".
 * @param channel   PDM channel of working mode setting - see "enum pdm_channel".
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 pdm_open(struct pdm_device *pdm, enum pdm_sample_freq freq, enum pdm_channel channel);

/**
 * @brief pdm_read
 *
 * @Compatible version: V0
 *
 * @note Turn PDM module on.
 *
 * @param pdm       The address of pdm device in the RTOS.
 * @param buf       The pointer of receive buf.
 * @param channel   PDM module receive length.
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 pdm_read(struct pdm_device *pdm, void *buf, uint32 len);

/**
 * @brief pdm_close
 *
 * @Compatible version: V0
 *
 * @note Turn PDM module off.
 *
 * @param pdm       The address of pdm device in the RTOS.
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 pdm_close(struct pdm_device *pdm);

/**
 * @brief pdm_request_irq.
 *
 * @Compatible version: V0
 *
 * @param pdm       The address of PDM device in the RTOS.
 * @param irq_flag  The type of interrupt - see"enum pdm_irq_flag".
 * @param irqhdl    The interrupt function.
 * @param irq_data  The parameters of the interrupt function
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 pdm_request_irq(struct pdm_device *pdm, enum pdm_irq_flag flag, pdm_irq_hdl irq_hdl, uint32 data);

/**
 * @brief pdm_release_irq.
 *
 * @Compatible version: V0
 *
 * @param pdm       The address of PDM device in the RTOS.
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 pdm_release_irq(struct pdm_device *pdm, enum pdm_irq_flag irq_flag);

/**
 * @brief pdm_ioctl.
 *
 * @Compatible version: V0
 *
 * @param pdm       The address of PDM device in the RTOS.
 * @param cmd       Control command - see"enum pdm_ioctl_cmd".
 * @param param     Parameters required to execute the current Control command.
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 pdm_ioctl(struct pdm_device *pdm, enum pdm_ioctl_cmd cmd, uint32 param);

#ifdef __cplusplus
}
#endif
#endif
