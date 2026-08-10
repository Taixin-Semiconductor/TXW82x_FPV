#include "basic_include.h"
#include <stdio.h>
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/multimedia/video.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

#include "general_msi/output_other_msi.h"
#include "encode/jpg_encode.h"
#include "decode/jpg_decode.h"

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc
#define NAME_LEN           64

typedef struct
{
    struct msi *msi;
    struct msi *decode;
    struct msi *encode;
    struct msi *output;
    char        decode_name[NAME_LEN];
    char        encode_name[NAME_LEN];
    char        output_name[NAME_LEN];
} jpg_recode_s;

static int32 jpg_recode_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    jpg_recode_s *jpg_recode = (jpg_recode_s *) msi->priv;
    int32         ret       = RET_OK;
    switch (cmd_id)
    {
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            if (fb)
            {
                msi_recv_fb(jpg_recode->decode, fb);
            }
            break;
        }
        case MSI_CMD_PRE_DESTROY:
        {
            if (jpg_recode->decode)
            {
                msi_destroy(jpg_recode->decode);
            }
            if (jpg_recode->encode)
            {
                msi_destroy(jpg_recode->encode);
            }
            if (jpg_recode->output)
            {
                msi_destroy(jpg_recode->output);
            }
        }
        break;
        case MSI_CMD_POST_DESTROY:
        {
            STREAM_FREE(jpg_recode);
            break;
        }
        default:
            break;
    }
    return ret;
}

struct msi *new_jpg_recode_msi(const char *name, uint16_t w, uint16_t h)
{
    struct msi   *msi       = NULL;
    uint8_t       isnew     = 0;
    jpg_recode_s *jpg_recode = NULL;

    if (!name)
    {
        return NULL;
    }

    msi = msi_new(name, 0, &isnew);
    if (!msi)
    {
        return NULL;
    }
    if (!isnew)
    {
        return msi;
    }

    jpg_recode = (jpg_recode_s *) STREAM_ZALLOC(sizeof(jpg_recode_s));
    if (!jpg_recode)
    {
        goto fail;
    }

    snprintf(jpg_recode->decode_name, NAME_LEN, "%s.dec", name);
    snprintf(jpg_recode->encode_name, NAME_LEN, "%s.enc", name);
    snprintf(jpg_recode->output_name, NAME_LEN, "%s.out", name);

    jpg_recode->decode = new_jpg_decode_msi(jpg_recode->decode_name, w, h);
    jpg_recode->encode = new_jpg_encode_msi(jpg_recode->encode_name);
    jpg_recode->output = new_output_other_msi(jpg_recode->output_name, msi);
    if (!jpg_recode->decode || !jpg_recode->encode || !jpg_recode->output)
    {
        goto fail;
    }

    msi_add_output(jpg_recode->decode, NULL, jpg_recode->encode, NULL);

    msi_add_output(jpg_recode->encode, NULL, jpg_recode->output, NULL);

    jpg_recode->msi = msi;
    msi->priv      = (void *) jpg_recode;
    msi->action    = jpg_recode_action;
    msi->enable    = 1;
    return msi;

fail:
    if (jpg_recode)
    {
        if (jpg_recode->decode)
        {
            msi_destroy(jpg_recode->decode);

        }
        if (jpg_recode->encode)
        {
            msi_destroy(jpg_recode->encode);
        }
        if (jpg_recode->output)
        {
            msi_destroy(jpg_recode->output);
        }
        STREAM_FREE(jpg_recode);
    }
    msi_destroy(msi);
    return NULL;
}