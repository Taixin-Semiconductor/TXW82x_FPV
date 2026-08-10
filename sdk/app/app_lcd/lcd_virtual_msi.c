#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lcd_core.h"
#include "user_work/user_work.h"
#include "hal/vdd.h"
#include "stream_define.h"
#include "lib/multimedia/msi_names.h"
#define DELETE_NAME "_del_virtual"

typedef struct
{
    struct os_work     work;
    struct msi        *msi;
    void              *chan;
    struct vdd_device *dev;
    uint16_t           w, h;
    uint16_t           x, y;
    char               msi_name[32];
} lcd_virtual_chan_s;

static int32_t lcd_virtual_msi_sub_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t             ret          = RET_OK;
    lcd_virtual_chan_s *virtual_chan = msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            msi->name = DELETE_NAME;
            vdd_close(virtual_chan->dev, virtual_chan->chan);
            os_free(msi->priv);
        }
        break;

        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&virtual_chan->work, 1);
            break;
        }
        // 帮忙转发,但是永远不需要放在队列中
        case MSI_CMD_TRANS_FB:
        {
        }
        break;

        case MSI_CMD_STOP:
        {
            vdd_ioctl(virtual_chan->dev, virtual_chan->chan, 0, 0);
        }

        break;
        case MSI_CMD_TRANS_FB_END:
        {
            os_run_work(&virtual_chan->work);
        }
        break;

        case MSI_CMD_SET_VDD_RECT:
        {
            struct vdd_rect *vdd = (struct vdd_rect *) param1;
            virtual_chan->w      = vdd->width;
            virtual_chan->h      = vdd->height;
            virtual_chan->x      = vdd->x;
            virtual_chan->y      = vdd->y;
            break;
        }

        case MSI_CMD_GET_VDD_RECT:
        {
            struct vdd_rect *vdd = (struct vdd_rect *) param1;
            vdd->width           = virtual_chan->w;
            vdd->height          = virtual_chan->h;
            vdd->x               = virtual_chan->x;
            vdd->y               = virtual_chan->y;
            break;
        }

        default:
            break;
    }
    return ret;
}

int32_t lcd_virtual_msi_work(struct os_work *work)
{
    lcd_virtual_chan_s *virtual_chan = (lcd_virtual_chan_s *) work;
    struct framebuff   *fb           = NULL;
    fb                               = msi_get_fb(virtual_chan->msi, 0);
    if (fb)
    {
        vdd_display(virtual_chan->dev, virtual_chan->chan, fb);
        msi_delete_fb(NULL, fb);
    }
    return 0;
}

struct msi *lcd_virtual_msi_sub_init(void)
{
    struct msi         *m            = NULL;
    lcd_virtual_chan_s *virtual_chan = NULL;
    void               *chan         = NULL;

    virtual_chan = (lcd_virtual_chan_s *) os_zalloc(sizeof(lcd_virtual_chan_s));
    if (!virtual_chan)
    {
        goto init_end;
    }

    virtual_chan->dev = (struct vdd_device *) dev_get(HG_LCD_VIRTUAL_DEVID);
    chan              = vdd_open(virtual_chan->dev);
    if (!chan)
    {
        return NULL;
    }
    virtual_chan->chan = chan;
    os_sprintf(virtual_chan->msi_name, "virtual_%08X_%08X", (uint32_t) chan, (uint32_t) os_jiffies());
    m = msi_new(virtual_chan->msi_name, 2, NULL);
    if (m)
    {
        m->priv           = virtual_chan;
        virtual_chan->msi = m;
        m->action         = lcd_virtual_msi_sub_action;
        // 使能work
        OS_WORK_INIT(&virtual_chan->work, lcd_virtual_msi_work, 0);
        m->enable = 1;
    }

init_end:
    if (!m)
    {
        if (chan)
        {
            vdd_close(virtual_chan->dev, chan);
        }
        if (virtual_chan)
        {
            os_free(virtual_chan);
        }
    }

    return m;
}

static int32_t lcd_virtual_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    switch (cmd_id)
    {
        case MSI_CMD_NEW_CHANNEL:
            if (msi->chan_mgr)
            {
                struct msi *m           = lcd_virtual_msi_sub_init();
                *(struct msi **) param2 = m;
                return m ? RET_OK : -ENOMEM;
            }
            break;
        default:
            break;
    }
    return ret;
}
void lcd_virtual_msi_init(void)
{
    struct msi *m = msi_new(VDD_MSI, 0, NULL);
    // 这个参数没有用
    m->priv       = (void *) m;
    m->chan_mgr   = 1;
    m->action     = lcd_virtual_msi_action;
    m->enable     = 1;
    return;
}
