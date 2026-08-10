#include "mux_file.h"
#include "lib/heap/av_psram_heap.h"
#include "osal/string.h"

// data 申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE   os_free
#define STREAM_LIBC_ZALLOC os_zalloc

typedef struct
{
    F_FILE  *fp;
    uint8_t  align_en;

    uint8_t *buf;
    uint32_t buf_size;
    uint32_t buf_len;

    uint32_t write_offset;
    uint32_t logical_offset;
    uint32_t prealloc_size;
} mux_file_t;

static int mux_file_direct_write(F_FILE *fp, const void *buf, uint32_t len)
{
    uint32_t written;

    if (!fp || (!buf && len))
    {
        return -1;
    }

    if (!len)
    {
        return 0;
    }

    written = osal_fwrite((void *) buf, 1, len, fp);
    return written == len ? 0 : -1;
}

static int mux_file_write_zero_direct(F_FILE *fp, uint32_t len)
{
    uint8_t  zero[64] = {0};
    uint32_t write_len;

    if (!fp)
    {
        return -1;
    }

    while (len)
    {
        write_len = len > sizeof(zero) ? sizeof(zero) : len;
        if (mux_file_direct_write(fp, zero, write_len))
        {
            return -1;
        }
        len -= write_len;
    }

    return 0;
}

static int mux_file_is_valid(const mux_file_t *mf)
{
    if (!mf || !mf->fp)
    {
        return 0;
    }

    if (!mf->align_en)
    {
        return 1;
    }

    if (!mf->buf || mf->buf_size != MUX_FILE_ALIGN_SIZE || mf->buf_len > mf->buf_size)
    {
        return 0;
    }

    return mf->logical_offset == mf->write_offset + mf->buf_len;
}

static int mux_file_flush_block(mux_file_t *mf)
{
    if (!mux_file_is_valid(mf) || !mf->align_en || mf->buf_len != mf->buf_size)
    {
        return -1;
    }

    osal_fseek(mf->fp, mf->write_offset);
    if (mux_file_direct_write(mf->fp, mf->buf, mf->buf_size))
    {
        return -1;
    }

    mf->write_offset += mf->buf_size;
    mf->buf_len = 0;
    return 0;
}

static int mux_file_flush_full_blocks(mux_file_t *mf)
{
    if (!mux_file_is_valid(mf))
    {
        return -1;
    }

    if (!mf->align_en)
    {
        return 0;
    }

    if (mf->buf_len == mf->buf_size)
    {
        return mux_file_flush_block(mf);
    }

    return 0;
}

void *mux_file_open(F_FILE *fp, uint32_t prealloc_size, uint8_t align_en)
{
    mux_file_t *mf;

    if (!fp)
    {
        return NULL;
    }

    mf = (mux_file_t *) STREAM_LIBC_ZALLOC(sizeof(mux_file_t));
    if (!mf)
    {
        return NULL;
    }

    mf->fp            = fp;
    mf->align_en      = align_en ? 1 : 0;
    mf->prealloc_size = (mf->align_en && prealloc_size) ? muxer_file_align_up(prealloc_size) : prealloc_size;
    mf->write_offset  = 0;
    mf->logical_offset = mf->write_offset;

    if (mf->align_en)
    {
        mf->buf = (uint8_t *) STREAM_MALLOC(MUX_FILE_ALIGN_SIZE);
        if (!mf->buf)
        {
            STREAM_LIBC_FREE(mf);
            return NULL;
        }
        mf->buf_size = MUX_FILE_ALIGN_SIZE;
    }

    if (mf->prealloc_size)
    {
        uint32_t filesize = osal_fsize(fp);
        if (filesize != mf->prealloc_size)
        {
            osal_fseek(fp, mf->prealloc_size);
            osal_ftruncate(fp);
            osal_fseek(fp, mf->logical_offset);
        }
    }

    return mf;
}

int mux_file_close(void *file)
{
    mux_file_t *mf = (mux_file_t *) file;

    if (!mf)
    {
        return 0;
    }

    if (mf->buf)
    {
        STREAM_FREE(mf->buf);
        mf->buf = NULL;
    }
    STREAM_LIBC_FREE(mf);
    return 0;
}

int mux_file_write(void *file, const void *buf, uint32_t len)
{
    mux_file_t     *mf  = (mux_file_t *) file;
    const uint8_t  *src = (const uint8_t *) buf;

    if (!mux_file_is_valid(mf) || (!src && len))
    {
        return -1;
    }

    if (!len)
    {
        return 0;
    }

    if (!mf->align_en)
    {
        osal_fseek(mf->fp, mf->logical_offset);
        if (mux_file_direct_write(mf->fp, src, len))
        {
            return -1;
        }
        mf->logical_offset += len;
        mf->write_offset = mf->logical_offset;
        return 0;
    }

    while (len)
    {
        uint32_t copy_len = mf->buf_size - mf->buf_len;
        if (copy_len > len)
        {
            copy_len = len;
        }

        os_memcpy(mf->buf + mf->buf_len, src, copy_len);
        mf->buf_len += copy_len;
        mf->logical_offset += copy_len;
        src += copy_len;
        len -= copy_len;

        if (mf->buf_len == mf->buf_size && mux_file_flush_block(mf))
        {
            return -1;
        }
    }

    return 0;
}

int mux_file_skip(void *file, uint32_t len)
{
    mux_file_t *mf = (mux_file_t *) file;

    if (!mux_file_is_valid(mf))
    {
        return -1;
    }

    if (!len)
    {
        return 0;
    }

    if (!mf->align_en)
    {
        mf->logical_offset += len;
        mf->write_offset = mf->logical_offset;
        osal_fseek(mf->fp, mf->logical_offset);
        return 0;
    }

    if (mf->buf_len)
    {
        uint32_t pad_len = mf->buf_size - mf->buf_len;
        if (pad_len > len)
        {
            pad_len = len;
        }

        os_memset(mf->buf + mf->buf_len, 0, pad_len);
        mf->buf_len += pad_len;
        mf->logical_offset += pad_len;
        len -= pad_len;

        if (mf->buf_len == mf->buf_size && mux_file_flush_block(mf))
        {
            return -1;
        }
    }

    if (len >= MUX_FILE_ALIGN_SIZE)
    {
        uint32_t skip_len = len & ~(MUX_FILE_ALIGN_SIZE - 1U);
        uint32_t done_len = skip_len;
        if (mf->prealloc_size && mf->write_offset <= mf->prealloc_size &&
            skip_len <= mf->prealloc_size - mf->write_offset)
        {
            mf->write_offset += skip_len;
            mf->logical_offset += skip_len;
        }
        else
        {
            os_memset(mf->buf, 0, mf->buf_size);
            while (skip_len)
            {
                osal_fseek(mf->fp, mf->write_offset);
                if (mux_file_direct_write(mf->fp, mf->buf, mf->buf_size))
                {
                    return -1;
                }
                mf->write_offset += mf->buf_size;
                mf->logical_offset += mf->buf_size;
                skip_len -= mf->buf_size;
            }
        }
        len -= done_len;
    }

    if (len)
    {
        os_memset(mf->buf, 0, len);
        mf->buf_len = len;
        mf->logical_offset += len;
    }

    return 0;
}

uint32_t mux_file_tell(void *file)
{
    mux_file_t *mf = (mux_file_t *) file;
    return mf ? mf->logical_offset : 0;
}

int mux_file_write_at(void *file, uint32_t offset, const void *buf, uint32_t len)
{
    mux_file_t    *mf = (mux_file_t *) file;
    const uint8_t *src = (const uint8_t *) buf;
    uint32_t       restore_offset;

    if (!mux_file_is_valid(mf) || (!src && len))
    {
        return -1;
    }

    if (mux_file_flush_full_blocks(mf))
    {
        return -1;
    }

    restore_offset = mf->align_en ? mf->write_offset : mf->logical_offset;
    if (!mf->align_en)
    {
        osal_fseek(mf->fp, offset);
        if (mux_file_direct_write(mf->fp, src, len))
        {
            return -1;
        }
        osal_fseek(mf->fp, restore_offset);
        return 0;
    }

    while (len)
    {
        uint32_t pending_start = mf->write_offset;
        uint32_t pending_end   = mf->write_offset + mf->buf_len;

        if (offset >= pending_start && offset < pending_end)
        {
            uint32_t copy_len = pending_end - offset;
            if (copy_len > len)
            {
                copy_len = len;
            }

            os_memcpy(mf->buf + (offset - pending_start), src, copy_len);
            offset += copy_len;
            src += copy_len;
            len -= copy_len;
            continue;
        }

        {
            uint32_t write_len = len;
            if (offset < pending_start && offset + write_len > pending_start)
            {
                write_len = pending_start - offset;
            }

            osal_fseek(mf->fp, offset);
            if (mux_file_direct_write(mf->fp, src, write_len))
            {
                return -1;
            }
            offset += write_len;
            src += write_len;
            len -= write_len;
        }
    }

    osal_fseek(mf->fp, restore_offset);
    return 0;
}

int mux_file_flush(void *file)
{
    mux_file_t *mf = (mux_file_t *) file;

    if (!mux_file_is_valid(mf))
    {
        return -1;
    }

    if (mux_file_flush_full_blocks(mf))
    {
        return -1;
    }

    return osal_fsync(mf->fp) == 0 ? 0 : -1;
}

int mux_file_finish(void *file)
{
    mux_file_t *mf = (mux_file_t *) file;
    uint32_t    pad_len;

    if (!mux_file_is_valid(mf))
    {
        return -1;
    }

    if (!mf->align_en)
    {
        osal_fseek(mf->fp, mf->logical_offset);
        return osal_fsync(mf->fp) == 0 ? 0 : -1;
    }

    if (mf->buf_len)
    {
        pad_len = mf->buf_size - mf->buf_len;
        os_memset(mf->buf + mf->buf_len, 0, pad_len);
        mf->buf_len = mf->buf_size;
        mf->logical_offset += pad_len;
        if (mux_file_flush_block(mf))
        {
            return -1;
        }
    }

    return osal_fsync(mf->fp) == 0 ? 0 : -1;
}

int mux_file_tail(void *file, const void *tail, uint32_t tail_len, uint32_t logical_end, uint32_t reserved_end)
{
    mux_file_t    *mf = (mux_file_t *) file;
    const uint8_t *tail_buf = (const uint8_t *) tail;
    uint8_t       *snapshot;
    uint32_t       remain;
    uint32_t       pending;
    uint32_t       block_remain;
    uint32_t       write_len;
    uint32_t       copy_len;

    if (!mux_file_is_valid(mf) || (!tail_buf && tail_len))
    {
        return -1;
    }

    if (reserved_end <= logical_end)
    {
        return 0;
    }

    remain = reserved_end - logical_end;
    if (tail_len > remain)
    {
        return -1;
    }

    if (!mf->align_en)
    {
        uint32_t restore_offset = mf->logical_offset;
        osal_fseek(mf->fp, logical_end);
        if (mux_file_direct_write(mf->fp, tail_buf, tail_len) ||
            mux_file_write_zero_direct(mf->fp, remain - tail_len))
        {
            return -1;
        }
        osal_fseek(mf->fp, restore_offset);
        return 0;
    }

    if (mf->logical_offset != logical_end || !mf->buf)
    {
        return -1;
    }

    pending = mf->buf_len;
    if (pending > mf->buf_size)
    {
        return -1;
    }

    snapshot = (uint8_t *) STREAM_MALLOC(MUX_FILE_ALIGN_SIZE);
    if (!snapshot)
    {
        return -1;
    }

    if (pending)
    {
        os_memcpy(snapshot, mf->buf, pending);
    }

    block_remain = mf->buf_size - pending;
    copy_len = tail_len < block_remain ? tail_len : block_remain;
    if (copy_len)
    {
        os_memcpy(snapshot + pending, tail_buf, copy_len);
    }

    if (block_remain > copy_len)
    {
        uint32_t zero_len = block_remain - copy_len;
        uint32_t max_zero = remain > tail_len ? remain - tail_len : 0;
        if (zero_len > max_zero)
        {
            zero_len = max_zero;
        }
        os_memset(snapshot + pending + copy_len, 0, zero_len);
    }

    write_len = pending + remain;
    if (write_len > mf->buf_size)
    {
        write_len = mf->buf_size;
    }

    osal_fseek(mf->fp, mf->write_offset);
    if (mux_file_direct_write(mf->fp, snapshot, write_len))
    {
        STREAM_FREE(snapshot);
        return -1;
    }

    if (tail_len > copy_len)
    {
        uint32_t rest_len    = tail_len - copy_len;
        uint32_t rest_offset = copy_len;

        osal_fseek(mf->fp, mf->write_offset + mf->buf_size);

        while (rest_len > 0)
        {
            uint32_t chunk_copy = rest_len < mf->buf_size ? rest_len : mf->buf_size;
            os_memcpy(snapshot, tail_buf + rest_offset, chunk_copy);
            rest_offset += chunk_copy;
            rest_len -= chunk_copy;

            if (chunk_copy < mf->buf_size)
            {
                os_memset(snapshot + chunk_copy, 0, mf->buf_size - chunk_copy);
            }

            if (mux_file_direct_write(mf->fp, snapshot, mf->buf_size))
            {
                STREAM_FREE(snapshot);
                return -1;
            }
        }
    }

    STREAM_FREE(snapshot);

    return 0;
}

void mux_file_get_ops(void *file, file_ops_t *ops)
{
    if (!ops)
    {
        return;
    }

    os_memset(ops, 0, sizeof(*ops));
    ops->file     = file;
    ops->write    = mux_file_write;
    ops->skip     = mux_file_skip;
    ops->tell     = mux_file_tell;
    ops->write_at = mux_file_write_at;
    ops->flush    = mux_file_flush;
    ops->finish   = mux_file_finish;
    ops->tail     = mux_file_tail;
}
