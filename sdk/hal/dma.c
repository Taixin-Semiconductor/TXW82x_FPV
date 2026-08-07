#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/dma.h"
#include "osal/string.h"

int32 dma_pause(struct dma_device *dma, uint32 ch)
{
    if (dma && ((const struct dma_hal_ops *)dma->dev.ops)->pause) {
        return ((const struct dma_hal_ops *)dma->dev.ops)->pause(dma, ch);
    }
    return RET_ERR;
}

int32 dma_resume(struct dma_device *dma, uint32 ch)
{
    if (dma && ((const struct dma_hal_ops *)dma->dev.ops)->resume) {
        return ((const struct dma_hal_ops *)dma->dev.ops)->resume(dma, ch);
    }
    return RET_ERR;
}

#pragma GCC push_options
#pragma GCC optimize ("O3")
void cache_deal_invalid(void *dst,uint32 n){
	uint8 *d1;
	uint8 *d2;
	uint8 st_head,end_head;
	d1 = (uint8 *)dst;
	st_head = (uint32)d1%32;
	end_head = 32 - ((uint32)d1+n)%32;
	n = n+st_head+end_head;
	d1 = d1 - st_head;
	d2 = (uint8 *)((uint32)d1&0x0fffffff);
	memcpy(d2,d1,32);
	memcpy(d2+n-32,d1+n-32,32);
	
	sys_dcache_invalid_range((void *)d1, n);
}

void cache_deal_writeback(void *dst,uint32 n){
	uint8 *d1;
	uint8 *d2;
	uint8 st_head,end_head;
	d1 = (uint8 *)dst;
	st_head = ((uint32)d1 % 32);
	end_head = 32 - ((uint32)d1+n)%32;
	n = n+st_head+end_head;
	d1 = d1 - st_head;
	d2 = (uint8 *)((uint32)d1&0x0fffffff);
	memcpy(d2,d1,32);
	memcpy(d2+n-32,d1+n-32,32);
	
	sys_dcache_clean_range((void *)d1, n);

}
void dma_memcpy(struct dma_device *dma, void *dst, const void *src, uint32 n)
{
	uint8  dma_buf[64];
	uint8  *s,*d;
	uint8  *s1,*d1,*s2,*d2;
	uint32 i = 0;
	uint32 addr;
    ASSERT(dma && ((const struct dma_hal_ops *)dma->dev.ops)->xfer);
	
    struct dma_xfer_data xfer_data;
	if(n <= 64){
		s1 = (uint8 *)src;
		d1 = (uint8 *)dst;
		for(i = 0;i < n;i++){
			d1[i] = s1[i];
		}
		return;
	}

	s1 = (uint8 *)src;
	d1 = (uint8 *)dst;

//	for(i = 0;i < 16;i++){        //处理头
//		dma_buf[i] = s1[i];
//	}
    memcpy(dma_buf, s1, 32);

	s2 = (uint8 *)src+n-32;
	d2 = (uint8 *)dst+n-32;
//	for(i = 0;i < 16;i++){        //处理头
//		dma_buf[i+16] = s2[i];
//	}
	memcpy(dma_buf+32, s2, 32);
	
	n = n - 64;
	s = (uint8 *)src+32;
	d = (uint8 *)dst+32;


    xfer_data.dest              = (uint32)d;
    xfer_data.src               = (uint32)s;
    xfer_data.element_per_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
    xfer_data.element_num       = n;
    xfer_data.dir               = ((((uint32)dst) < SRAM_BASE) || (((uint32)src) < SRAM_BASE)) ? DMA_XFER_DIR_M2D : DMA_XFER_DIR_M2M;
    xfer_data.src_addr_mode     = DMA_XFER_MODE_INCREASE;
    xfer_data.dst_addr_mode     = DMA_XFER_MODE_INCREASE;
    xfer_data.dst_id            = 0;
    xfer_data.src_id            = 0;
    xfer_data.irq_hdl           = NULL;
    xfer_data.endian            = 0;
	addr = (uint32)s;

	
	addr = addr&((0x7FFFFFFUL << 5));
	s = (uint8*)addr;
	addr = (uint32)d;
	addr = addr&((0x7FFFFFFUL << 5));
	d = (uint8*)addr;//&CACHE_CIR_INV_ADDR_Msk;
    sys_dcache_clean_range((void *)s, (int32_t)s2-(int32_t)s);          //cache->psram
	sys_dcache_clean_invalid_range((void *)d, (int32_t)d2-(int32_t)d);	//psram cache invalid  //最后一行可能没有无效化	
//	printf("===============s:%08x  d:%08x\r\n\r\n",s,d);
//    xfer_data.irq_data          = 0;
    ((const struct dma_hal_ops *)dma->dev.ops)->xfer(dma, &xfer_data);

//    sys_dcache_invalid_range((void *)d, (int32_t)d2-(int32_t)d);

//	for(i = 0;i < 16;i++){        //处理头
//		d1[i] = dma_buf[i];
//	}
    memcpy(d1, dma_buf, 32);
//	for(i = 0;i < 16;i++){        //处理尾
//		d2[i] = dma_buf[i+16];
//	}
    memcpy(d2, dma_buf+32, 32);
}

void dma_memcpy_no_cache(struct dma_device *dma, void *dst, const void *src, uint32 n)
{
	//uint8  dma_buf[64];
	uint8  *s,*d;
	//uint8  *s1,*d1,*s2,*d2;
	//uint32 i = 0;
	uint32 addr;
    ASSERT(dma && ((const struct dma_hal_ops *)dma->dev.ops)->xfer);
	
    struct dma_xfer_data xfer_data;

	s = (uint8 *)src;
	d = (uint8 *)dst;


    xfer_data.dest              = (uint32)d;
    xfer_data.src               = (uint32)s;
    xfer_data.element_per_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
    xfer_data.element_num       = n;
    xfer_data.dir               = ((((uint32)dst) < SRAM_BASE) || (((uint32)src) < SRAM_BASE)) ? DMA_XFER_DIR_M2D : DMA_XFER_DIR_M2M;
    xfer_data.src_addr_mode     = DMA_XFER_MODE_INCREASE;
    xfer_data.dst_addr_mode     = DMA_XFER_MODE_INCREASE;
    xfer_data.dst_id            = 0;
    xfer_data.src_id            = 0;
    xfer_data.irq_hdl           = NULL;
    xfer_data.endian            = 0;
	addr = (uint32)s;

    ((const struct dma_hal_ops *)dma->dev.ops)->only_hw_xfer(dma, &xfer_data);
}

void dma_memset_word(struct dma_device *dma, void *dst, uint32 c, uint32 n)
{
    uint32 val = c;
	uint8 *d, *d1, *d2;
	uint32 i = 0;
    uint32 size  = 0;
	uint32 addr;

	if(n <= 64){
        d1   = (uint8 *)dst;
        i    = (uint32)dst % 4;
        size = i + n;
        for(;i < (size);i++){
            *d1++ = (val >> ((i % 4)<<3));
        }
		return;
	}	
	
	d1 = (uint8 *)(dst);
	d2 = (uint8 *)(dst+n-32);
	n  = n - 64;
	d  = (uint8 *)(dst+32);

	ASSERT(dma && ((const struct dma_hal_ops *)dma->dev.ops)->xfer);
	struct dma_xfer_data xfer_data;
	xfer_data.dest				= (uint32)d;
	xfer_data.src				= (uint32)&val;
	xfer_data.element_per_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	xfer_data.element_num		= n;
	xfer_data.dir				= (((uint32)dst) < SRAM_BASE) ? DMA_XFER_DIR_M2D : DMA_XFER_DIR_M2M;
	xfer_data.src_addr_mode 	= DMA_XFER_MODE_RECYCLE;
	xfer_data.dst_addr_mode 	= DMA_XFER_MODE_INCREASE;
	xfer_data.dst_id			= 0;
	xfer_data.src_id			= 0;
	xfer_data.irq_hdl			= NULL;
	xfer_data.irq_data		    = 0;
    xfer_data.endian            = 0;

	addr = (uint32)d;
	addr = addr&((0x7FFFFFFUL << 5));
	d    = (uint8 *)addr;//&CACHE_CIR_INV_ADDR_Msk;		
	sys_dcache_clean_invalid_range((void *)d, (int32_t)d2-(int32_t)d);

    ((const struct dma_hal_ops *)dma->dev.ops)->xfer(dma, &xfer_data);
    
    sys_dcache_invalid_range((void *)d, (int32_t)d2-(int32_t)d);
    
    i    = (uint32)dst % 4;
    size = i + 32;
	for(;i < (size);i++){        //处理头
		*d1++ = (val >> ((i % 4)<<3));
	}
    i    = (uint32)d2 % 4;
    size = i + 32;
    for(;i < (size);i++){            //处理尾
        *d2++ = (val >> ((i % 4)<<3));
    }
}

void dma_memset(struct dma_device *dma, void *dst, uint8 c, uint32 n)
{
    uint32 val = c | (c<<8) | (c<<16) | (c<<24);
    dma_memset_word(dma, dst, val, n);
}

#pragma GCC pop_options
void dma_memcpy_endian(struct dma_device *dma, void *dst, const void *src, uint32 n, enum dma_endian_type type)
{
	uint32 dma_buf[16];
	uint8  *s,*d;
	uint8  *s1, *d1, *s2, *d2;
    uint16 *s3, *d3, *s4;
    uint32 *s5, *d5, *s6;
	uint32 i = 0;
	uint32 addr;
	uint32 ret = RET_OK;
    ASSERT(dma && ((const struct dma_hal_ops *)dma->dev.ops)->xfer);
	
    struct dma_xfer_data xfer_data;
    switch (type)
    {
        case DMA_ENDIAN_TYPE_NO_OVERTURN:
            s1 = (uint8 *)src;
            d1 = (uint8 *)dst;
            s2 = (uint8 *)dma_buf;
            if (n <= 64)
            {
                for(i = 0; i < (n >> type); i++){
                    *d1++ = *s1++;
                }
            } else {
                for(i = 0; i < (32 >> type); i++){  // 处理头
                    *s2++ = *s1++;
                }
                s1 = (uint8 *)((uint32)src+n-32);
                for(i = 0; i < (32 >> type); i++){  // 处理尾
                    *s2++ = *s1++;
                }
            }
            break;
        
        case DMA_ENDIAN_TYPE_16BIT_OVERTURN:
            s3 = (uint16 *)src;
            d3 = (uint16 *)dst;
            s4 = (uint16 *)dma_buf;
            ret = (n % 2 != 0);  
            if (!ret)
            {
                if (n <= 64)
                {
                    for(i = 0; i < (n >> type); i++){
                        *d3++ = __REVSH(*s3++);
                    }
                } else {
                    for(i = 0; i < (32 >> type); i++){  // 处理头
                        *s4++ = __REVSH(*s3++);
                    }
                    s3 = (uint16 *)((uint32)src+n-32);
                    for(i = 0; i < (32 >> type); i++){  // 处理尾
                        *s4++ = __REVSH(*s3++);
                    }
                }
            } else {
                os_printf("dma memcpy size : %d endian: %d err\r\n", n, type);
            }
            break;

        case DMA_ENDIAN_TYPE_32BIT_OVERTURN:
            s5 = (uint32 *)src;
            d5 = (uint32 *)dst;
            s6 = (uint32 *)dma_buf;
            ret = (n % 4 != 0);
            if (!ret)
            {
                if (n <= 64)
                {
                    for(i = 0; i < (n >> type); i++){
                        *d5++ = __REV(*s5++);
                    }
                } else {
                    for(i = 0; i < (32 >> type); i++){  // 处理头
                        *s6++ = __REV(*s5++);
                    }
                    s5 = (uint32 *)((uint32)src+n-32);
                    for(i = 0; i < (32 >> type); i++){  // 处理尾
                        *s6++ = __REV(*s5++);
                    }
                }
            } else {
                os_printf("dma memcpy size : %d endian: %d err\r\n", n, type);
            }
            break;
        
        default:
            os_printf("dma memcpy endian type err!");
            ret = RET_ERR;
            break;
    }
    if (ret || (n <= 64))   return;

    s1 = (uint8 *)src;
	d1 = (uint8 *)dst;
    s2 = (uint8 *)(src+n-32);
    d2 = (uint8 *)(dst+n-32);
    n  = n - 64;
    s  = (uint8 *)(src+32);
	d  = (uint8 *)(dst+32);
    xfer_data.dest              = (uint32)d;
    xfer_data.src               = (uint32)s;
    xfer_data.element_per_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
    xfer_data.element_num       = n;
    xfer_data.dir               = ((((uint32)dst) < SRAM_BASE) || (((uint32)src) < SRAM_BASE)) ? DMA_XFER_DIR_M2D : DMA_XFER_DIR_M2M;
    xfer_data.src_addr_mode     = DMA_XFER_MODE_INCREASE;
    xfer_data.dst_addr_mode     = DMA_XFER_MODE_INCREASE;
    xfer_data.dst_id            = 0;
    xfer_data.src_id            = 0;
    xfer_data.irq_hdl           = NULL;
    xfer_data.endian            = type;
    addr = (uint32)s;
	addr = addr&((0x7FFFFFFUL << 5));
	s = (uint8*)addr;
	addr = (uint32)d;
	addr = addr&((0x7FFFFFFUL << 5));
	d = (uint8*)addr;//&CACHE_CIR_INV_ADDR_Msk;
    sys_dcache_clean_range((void *)s, (int32_t)s2-(int32_t)s);          //cache->psram
	sys_dcache_clean_invalid_range((void *)d, (int32_t)d2-(int32_t)d);	//psram cache invalid  //最后一行可能没有无效化	

    ((const struct dma_hal_ops *)dma->dev.ops)->xfer(dma, &xfer_data);
    
    sys_dcache_invalid_range((void *)d, (int32_t)d2-(int32_t)d);
    memcpy(d1, dma_buf, 32);
    memcpy(d2, (void *)((uint32)dma_buf+32), 32);
}

int32 dma_xfer(struct dma_device *dma, struct dma_xfer_data *data)
{
    if (dma && ((const struct dma_hal_ops *)dma->dev.ops)->xfer) {
        return ((const struct dma_hal_ops *)dma->dev.ops)->xfer(dma, data);
    }
    return RET_ERR;
}

int32 dma_status(struct dma_device *dma, uint32 ch)
{
    if (dma && ((const struct dma_hal_ops *)dma->dev.ops)->get_status) {
        return ((const struct dma_hal_ops *)dma->dev.ops)->get_status(dma, ch);
    }
    return RET_ERR;
}

int32 dma_stop(struct dma_device *dma, uint32 ch)
{
    if (dma && ((const struct dma_hal_ops *)dma->dev.ops)->stop) {
        return ((const struct dma_hal_ops *)dma->dev.ops)->stop(dma, ch);
    }
    return RET_ERR;
}

int32 dma_ioctl(struct dma_device *dma, uint32 cmd, int32 param1, int32 param2)
{
    if (dma && ((const struct dma_hal_ops *)dma->dev.ops)->ioctl) {
        return ((const struct dma_hal_ops *)dma->dev.ops)->ioctl(dma, cmd, param1, param2);
    }
    return RET_ERR;
}
