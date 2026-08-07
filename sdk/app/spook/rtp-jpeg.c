/*
 * Copyright (C) 2004 Nathan Lutchansky <lutchann@litech.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include <stdio.h>
#include "lwip\sockets.h"
#include "lwip\netif.h"
#include "lwip\dns.h"

#include "lwip\api.h"
#include "lwip\tcp.h"

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <rtp.h>
#include <rtp_media.h>
#include <spook_config.h>
#include "list.h"
#include "jpgdef.h"
#include "osal/sleep.h"
#include "session.h"
//#include "common.h"
#include "csi_kernel.h"
#include "osal/string.h"
#include "utlist.h"


#ifdef USB_EN
#include "dev/usb/uvc_host.h"
#endif

#define EXTHDR_LEN 6
#define EXTHDROFF_MAGIC 0
#define EXTHDROFF_VER   1
#define EXTHDROFF_DRI   2
#define EXTHDROFF_HUFF  4

struct rtp_jpeg {
	unsigned char *d;//链表第一帧数据
	int type;
	int q;
	int width;
	int height;
	int luma_table;
	int chroma_table;
	//unsigned char *quant[16];
	int quant[16];
	int huffoff;
	int hufflen;
	char rtp_quant[128];//用于临时保存量化表
	char huff_quant[512];
	unsigned char exthdr[EXTHDR_LEN];
	unsigned char *scan_data;
	int scan_data_len;
	int offset;
	int init_done;
	int ts_incr;
	unsigned int timestamp;	
	int reset_interval;
	int dri_num;
	int max_send_size;
	unsigned short int dri_len[100];
	//为了获取到frame,因为使用了链表形式
	struct frame *f;
};

static int parse_DQT( struct rtp_jpeg *out, unsigned char *d, int len )
{
	int i;

	for( i = 0; i < len; i += 65 )
	{
		if( ( d[i] & 0xF0 ) != 0 )
		{
			_os_printf( "Unsupported quant table precision 0x%X!\n",
					d[i] & 0xF0 );
			return -1;
		}
		//out->quant[d[i] & 0xF] = d + i + 1;
		out->quant[d[i] & 0xF] = d-out->d+i+1;
		//out->quant[d[i] & 0xF] = out->scan_data - (d + i + 1);
	}
	return 0;
}

static int parse_SOF( struct rtp_jpeg *out, unsigned char *d, int len )
{
	int c;

	out->chroma_table = -1;
	if( d[0] != 8 )
	{
		_os_printf( "Invalid precision %d in SOF0\n", d[0] );
		return -1;
	}
	out->height = GET_16( d + 1 );
	out->width = GET_16( d + 3 );

	
	if( ( out->height & 0x7 ) || ( out->width & 0x7 ) )
	{
		os_printf(KERN_ERR"Width/height not divisible by 8!\n" );
		os_printf(KERN_ERR"w:%d\th:%d\n",out->width,out->height);
		return -1;
	}
	out->width >>= 3;
	out->height >>= 3;
	if( d[5] != 3 )
	{
		_os_printf( "Number of components is %d, not 3!\n", d[5] );
		return -1;
	}
	/* Loop over the parameters for each component */
	for( c = 6; c < 6 + 3 * 3; c += 3 )
	{
//		if( d[c + 2] >= 16 || ! out->quant[d[c + 2]] )
//		{
//			_os_printf( "Component %d specified undefined quant table %d!\n", d[c], d[c + 2] );
//			return -1;
//		}
		switch( d[c] ) /* d[c] contains the component ID */
		{
		case 1: /* Y */
			/*
			if( d[c + 1] == 0x11 ) out->type = 0;
			else if( d[c + 1] == 0x22 ) out->type = 1;
			*/			
			if( d[c + 1] == 0x21 ) out->type = out->type & ~1;   //YUV422
			else if( d[c + 1] == 0x22 ) out->type = (out->type & ~1)|1;  //YUV420
			else
			{
				_os_printf( "Invalid sampling factor 0x%02X in Y component!\n", d[c + 1] );
				return -1;
			}
			out->luma_table = d[c + 2];
			break;
		case 2: /* Cb */
		case 3: /* Cr */
			if( d[c + 1] != 0x11 )
			{
				_os_printf( "Invalid sampling factor 0x%02X in %s component!\n", d[c + 1], d[c] == 2 ? "Cb" : "Cr" );
				return -1;
			}
			if( out->chroma_table < 0 )
				out->chroma_table = d[c + 2];
			else if( out->chroma_table != d[c + 2] )
			{
				_os_printf( "Cb and Cr components do not share a quantization table!\n" );
				return -1;
			}
			break;
		default:
			_os_printf( "Invalid component %d in SOF!\n", d[c] );
			return -1;
		}
	}
	return 0;
}

static int parse_DHT( struct rtp_jpeg *out, unsigned char *d, int len )
{
	/* We should verify that this coder uses the standard Huffman tables */
  /* Kaifan:
   * Some USB-Sensor do not use standard Huffman tables.
   * So I add Huffman following quant table.
   * It's compatible to the old version of App,
   * It just looks like some dummy quant tables exist.
   * But the dummy size can not be too long, because the old rtpdec-jpeg.c uses hdr[1024]
   */
  if(0 == out->hufflen)
  {
		//out->huffoff = out->scan_data - (d - 4);
  	out->huffoff = (int)(d - 4);
  }
  out->hufflen += len + 4;
  PUT_16(out->exthdr+EXTHDROFF_HUFF, out->hufflen);
	return 0;
}

static void parse_DRI (struct rtp_jpeg *out, unsigned char *d)
{
	if (GET_16( d )) {
		out->type |= 0x40;
		out->reset_interval = GET_16( d );
		/* Kaifan:
		 * Some USB-Sensor's reset-interval is not width/16.
		 * But the origin room for reset-interval is used by res_len.
		 * So Save the reset-interval before the Huffman table
		 */
		PUT_16(out->exthdr+EXTHDROFF_DRI, out->reset_interval);
	}
}

extern volatile uint8_t framerate_c;

void clear_init_done(void *d)
{
	_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	struct rtp_jpeg *out = (struct rtp_jpeg *)d;
	out->init_done = 0;
}




static int jpeg_process_frame( struct frame *f, void *d )
{
	struct rtp_media *rtp = (struct rtp_media *)d;
	struct rtp_jpeg *out = (struct rtp_jpeg *)rtp->private;
	out->f = f;
	int i, blen;
	uint32_t per_ms_incr = (25 * out->ts_incr)/1000;
	//static uint32_t last_time = 0;
	//uint32_t sys_time = 0;
	out->timestamp = per_ms_incr*f->timestamp;

	//out->scan_data = j->scan_data;
	//out->scan_data_len = j->scan_data_len;
	//out->dri_num = j->dri_num;
	//memcpy(out->dri_len, j->dri_len, sizeof(j->dri_len));
		
	/* note by jornny 
	* just parse once in order to reduce time of processing frame
	*/
	//uvc 每一次都要扫描
	#if USB_EN == 1 
			out->init_done = 0;
	#endif	

    if(out->init_done == 0 ) {
		out->init_done =0; 
		for( i = 0; i < 16; ++i ) out->quant[i] = 0;	
		out->type = 0;
		out->reset_interval = 0;
		/* Kaifan:
		 * exthdr[0] = 0 let the App knows the extend protocol
		 * exthdr[1] = 0 extend protocal version 0
		 */
		out->hufflen = 0;
		out->exthdr[EXTHDROFF_MAGIC] = 0;
		out->exthdr[EXTHDROFF_VER]   = 0;

	} else {
		return out->init_done;
	}
	//out->dri_num = 0;
	
	out->d = f->d;

	//增加一个前置空数的地方
	for( i = 0; i < f->first_length; i += blen + 2 )
	{
		if( f->d[i] != 0xFF ) 
		{
			_os_printf( "Found %02X at %d, expecting FF\n", f->d[i], i );
			out->scan_data_len = 0;
			return 0;
		}
		while(f->d[i+1] == 0xFF) ++i;

		/* SOI (FF D8) is a bare marker with no length field */
		if( f->d[i + 1] == 0xD8 ) blen = 0;
		else blen = GET_16( f->d + i + 2 );

		switch( f->d[i + 1] )
		{
		case 0xDB: /* Quantization Table */
			if( out->init_done ) break;
			if( parse_DQT( out, f->d + i + 4, blen - 2 ) < 0 )
			{
				out->scan_data_len = 0;
                _os_printf( "jpeg_process_frame Quantization Table err!\n" );
				return 0;
			}
			break;
		case 0xC0: /* Start of Frame */
			if( out->init_done ) break;
			if( parse_SOF( out, f->d + i + 4, blen - 2 ) < 0 )
			{
				out->scan_data_len = 0;
                _os_printf( "jpeg_process_frame Start of Frame err!\n" );
				return 0;
			}
			break;
		case 0xC4: /* Huffman Table */
//			if( out->init_done ) break; /* only parse DHT once */ /* Kaifan: No! Maybe has 4 DHTs! */
			if( parse_DHT( out, f->d + i + 4, blen - 2 ) < 0 )
			{
				out->scan_data_len = 0;
                _os_printf( "jpeg_process_frame Huffman Table err!\n" );
				return 0;
			}
//			out->init_done = 1; /* Kaifan: Maybe has 4 DHTs! So can't done here */
			break;		
		case 0xDD:	/* DRI */
			if( out->init_done ) break;
			parse_DRI (out, f->d + i + 4);
			break;
		case 0xDA: /* Start of Scan */
			out->init_done = 1; /* Kaifan: Now can done here */
			out->scan_data = f->d + i + 14;
			out->scan_data_len = f->length - (out->scan_data - f->d);
			out->offset = out->scan_data-f->d;
			//_os_printf("scan_data_len:%d\t%d\n",f->length,(out->scan_data - f->d));
		
			return out->init_done;
			/*out->scan_data = f->d + i + 2 + blen;
			out->scan_data_len = f->length - i - 2 - blen;
			out->timestamp += out->ts_incr;
			if(scan_DRI(out))
				return 0;
			else 
				return out->init_done;*/
		}
	}

	_os_printf( "Found no scan data!\n" );
	uint32_t xi=0;
	for(xi=0;xi<0x290;xi++)
	{
		_os_printf(" %02x",f->d[xi]);	
	}
    _os_printf(" \n");
	out->scan_data_len = 0;
	return 0;
}

static int jpeg_get_sdp( char *dest, int len, int payload, int port, void *d )
{
	return snprintf( dest, len, "m=video %d RTP/AVP 26\r\n", port );
}

static int jpeg_get_payload( int payload, void *d )
{
	return 26;
}

static int cal_data_len(struct rtp_jpeg *out, int *dri_index, int *res_len, int max_size)
{
	/* not by jorrny
	* At the beginning, we try to send the packet aligned to dri.
	* But we found it inefficient when the dri length is almost a half of the max_size
	* So we decide to send the packet with the max_size always to improve efficiency.
	* In the same way, we try to send the packet within max_size in order to prevent fragment in ip layer.
	*/

	int len = 0;

	do
	{
		if(*res_len) {
			len = *res_len;
		} else {
			len += out->dri_len[*dri_index];
		}
		*res_len = 0;
		if(len > max_size) {
			*res_len = len - max_size;
			len = max_size;
			break;
		} else if (len == max_size) {
			(*dri_index)++;
			break;
		}
		if(++(*dri_index) > out->dri_num)
			break;
	} while(1);

	return len;
}

//版本已经移除
static int jpeg_send( struct rtp_endpoint *ep, void *d )
{
	return 0;
}

//#define USE_EXTHDR
static int jpeg_send_more( rtp_loop_search_ep search,void *ls,void *track, void *d,void *cache_buf,int cache_buf_len )
{
	 unsigned char *tmp_buf ;
	 //_os_printf("@");
	 struct rtp_jpeg *out = (struct rtp_jpeg *)d;
	 //获取链表
	 struct framebuff *fb = (struct framebuff *)out->f->get_f;
	 uint8_t *jpeg_buf_addr ;
	 int i, plen, vcnt, hdr_len,j;
	 unsigned int node_offset;
	 struct iovec v[8];
	 unsigned char vhdr[12], qhdr[4];/*, dhdr[4]*/
	 int res_len = 1, max_data_size;
	 int total_len;
	 struct rtp_endpoint *ep;
	 void *head;
	 unsigned char *send_buf;
	 int send_total_len;
 
	 int node_len = out->f->node_len;
	 out->scan_data_len = out->f->length-out->offset;
 
 
 /* There's at least 8-byte video-specific header  
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | Type-specific |              Fragment Offset                  		|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |      Type     	  |       Q       	|     Width     	|     Height    		|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  
 */	
 
	 /* Main JPEG header */
	 vhdr[0] = 0; /* type-specific, value 0 -> non-interlaced frame */
	 /* vhdr[1..3] is the fragment offset */
	 vhdr[4] = out->type; /* type */
	 vhdr[5] = 255; /* Q, value 255 -> dynamic quant tables */
	 vhdr[6] = out->width;
	 vhdr[7] = out->height;
	 
 /* Restart Marker header present
 0 				  1 				  2 				  3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |	   Restart Interval 	   	  |F|L|	   Restart Count	   |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+	 
 */
	 //PUT_16 (vhdr+8,  out->reset_interval);
	 //_os_printf("rtp send\n");
	 /* PUT_16 (vhdr+10, 0xFFFF); */
	 v[1].iov_base = vhdr;
 
	 if (out->type & 0x40)
		 v[1].iov_len = 12;
	 else
		 v[1].iov_len = 8;
 
	 /* Quantization table header */
	 qhdr[0] = 0; /* MBZ */
	 qhdr[1] = 0; /* precision */
	 jpeg_buf_addr =fb->data;
	 #ifdef PSRAM_HEAP
		 tmp_buf = (unsigned char*)cache_buf;
		 //拷贝到缓冲区,主要为了解决psram中cache的问题
		 hw_memcpy0(tmp_buf,jpeg_buf_addr,cache_buf_len);
	 #else
		 tmp_buf = jpeg_buf_addr;
	 #endif
 
 
 
 #ifdef USE_EXTHDR
	 PUT_16( qhdr + 2, 2 * 64 + out->hufflen + EXTHDR_LEN); /* length */
 #else
	 PUT_16( qhdr + 2, 2 * 64); /* length */
 #endif
	 v[2].iov_base = qhdr;
	 v[2].iov_len = 4;
 
	 /* Luma quant table */
	 /* Kaifan:
	  * jpeg_process_frame called Only Once by spook, It can locate at any frame in the pool.
	  * If the frame align at 4KB boundrary like AX3268, The address out->quant set by parse_DQT may error.
	  * In fix header enviroment, all frame's DQT-distance-from-scan-data is the same.
	  */
	 /* Kaifan 20170913:
	  * - Jonny added the init_done which causes jpeg_process_frame called only once.
	  * - But jpeg-directly-output usb-sensors let SCAN_DATA_OFFSET & 0x19 & 0x5A non-constant.
	  * It needs to change the struct rtp_jpeg out->quant to record the quant offset instead of address.
	  */
	 hw_memcpy0(out->rtp_quant,tmp_buf+out->quant[out->luma_table],64);
	 v[3].iov_base = out->rtp_quant;
	 v[3].iov_len = 64;
 
 
	 /* Chroma quant table */
	 hw_memcpy0(out->rtp_quant+64,tmp_buf+out->quant[out->chroma_table],64);
	 v[4].iov_base = out->rtp_quant+64;
	 v[4].iov_len = 64;
 ///////// PORTA.8 print
 
	 hdr_len = 132 + v[1].iov_len;
 #ifdef USE_EXTHDR 
	 vcnt = 7;					
	 hw_memcpy0(out->huff_quant,out->huffoff,out->hufflen);
	 v[5].iov_base = out->exthdr;   
	 v[5].iov_len = EXTHDR_LEN;
	 v[6].iov_base = out->huff_quant;//out->huffoff;
	 //_os_printf("scan_data:%X\thuffoff:%X\t%d\n",out->scan_data,out->huffoff,out->hufflen);
	 v[6].iov_len = out->hufflen;
	 hdr_len += (v[5].iov_len+v[6].iov_len);
 #else
	 vcnt = 5;
 #endif
	 //获取首个节点
	 node_offset = out->offset;//out->scan_data-jpeg_buf_addr;

	 //循环查找链表的buf
 
	 //uint32_t flags;
	 //uint32_t ref;
	 i = 0;
	 //这个通过链表轮询,需要源头使用这种结构才行,因为这里知道jpg是使用这种结构发送数据,因此这里直接使用,存在一定耦合性
 
 
		 //根据实际结构获取buf
		 jpeg_buf_addr = (uint8_t*)fb->data;
		 //os_printf("node_len:%d\tjpeg_buf_addr:%X\n",node_len,jpeg_buf_addr);
 
 //改节点还没有完成,继续发送
 continue_send:
		 if(i >= out->scan_data_len)
		 {
			 goto jpeg_send_more_msi_end;
		 }
 
 
		 //因为有预留,所以这里size就要减少(注意框架不应该进去,如果进去,就是异常,可以设置FREE_JPG_NODE打印错误异常)
		 if(node_offset >= node_len)
		 {
			 node_offset = 0;
		 }
 
		 max_data_size = out->max_send_size-hdr_len;
 
		 if(out->type & 0x40) 
		 {
			 if((out->scan_data_len - i) > max_data_size)
			 {
				 plen = max_data_size;
			 }
			 else
			 {
				 plen = out->scan_data_len - i;
			 }
		 
			 if(res_len) 
			 {
				 memcpy(vhdr+8,out->exthdr+EXTHDROFF_DRI,2);
 
			 } 
			 else 
			 {
				 PUT_16 (vhdr+8,  0x28);
			 }
			 PUT_16 (vhdr+10, 0xffff);
 
		 } 
		 else 
		 {
			 plen = out->scan_data_len - i;
			 if(plen > max_data_size) 
			 {
				 plen = max_data_size;
			 }
		 }
		 vhdr[1] = i >> 16;
		 vhdr[2] = ( i >> 8 ) & 0xff;
		 vhdr[3] = i & 0xff;
 
		 //如果发送使用原来的buf,记录需要发送的起始地址,v[0].iov_len=12(固定)
		 //所以这里统计之前iov_len,然后其实地址就是jpeg_buf_addr+node_offset-total_len
		 //空出前面的位置,填充rtp的头,所以发送数据就是rtp+data
 
 
		 total_len = 12;
		 for(j=1;j<vcnt;j++)
		 {
			 total_len += v[j].iov_len;
		 }
 
		 v[vcnt].iov_base = jpeg_buf_addr+node_offset-total_len;
 
		 //判断节点偏移,如果偏移到结尾,则修改pln
		 if(node_offset+plen>=node_len  )
		 {
			 plen = node_len - node_offset;
		 }
		 //32byte对齐,默认都是psram
		 else
		 {
			uint32_t align_32 = (node_offset+plen)&(~0x1F);
			plen = align_32 - node_offset;
		 }
 
 
		 if(i+plen >=out->scan_data_len)
		 {
			 plen = out->scan_data_len - i;
		 }
		 node_offset+=plen;
		 v[vcnt].iov_len = plen;
		 //获取数据内容
		 send_buf = get_send_rtp_packet_head(v ,vcnt + 1,cache_buf);
		 //重新赋值ls的头
		 head = ls;
		 while(head)
		 {
			 //获取ep
			 head = search(head,track,(void*)&ep);
			 if(ep)
			 {
				 //数据内容是一样的,修改头部就可以了
				 //_os_printf("real data:%X\tsend_data:%X\n",jpeg_buf_addr,send_buf);
				 send_total_len = set_send_rtp_packet_head(ep, v, vcnt + 1,out->timestamp, plen + i == out->scan_data_len,send_buf );
				 if(ep->sendEnable)
				 {
					 send_rtp_packet_more(ep, send_buf, send_total_len,30 );
				 }
			 }
 
		 }
 
		 /* Done with all hea	ders except main JPEG header */
		 vcnt = 2;
		 hdr_len = v[1].iov_len;		
 
		 i += plen;
 
		 //os_printf("node_offset:%d\tscan_data_len:%d\ti:%d\tnode_len:%d\n",node_offset,out->scan_data_len,i,node_len);
		 if(node_offset < node_len && i < out->scan_data_len)
		 {
			 //节点没有发送完成,继续回去发送
			 goto continue_send;
		 }
 
 jpeg_send_more_msi_end:
	 
 
	 //重新赋值ls的头
	 head = ls;
	 while(head)
	 {
		 //获取ep
		 head = search(head,track,(void*)&ep);
		 if(ep)
		 {
			 ep->sendEnable = 1;
		 }
	 }
 
	 return 0;
}




struct rtp_media *new_rtp_media_jpeg_stream( struct stream *stream )
{
	struct rtp_jpeg *out;
	int fincr, fbase;
	struct rtp_media *m;

	stream->get_framerate( stream, &fincr, &fbase );
	out = (struct rtp_jpeg *)malloc( sizeof( struct rtp_jpeg ) );
	out->init_done = 0;
	out->timestamp = 0;
	out->scan_data = NULL;
	out->scan_data_len = 0;
	out->max_send_size = MAX_DATA_PACKET_SIZE;
	out->ts_incr = 90000 * fincr / fbase;
	memset(out->rtp_quant,0,sizeof(out->rtp_quant));

	m = new_rtp_rtcp_media( jpeg_get_sdp, jpeg_get_payload,
					jpeg_process_frame, jpeg_send,new_rtcp_send, out );
	if(m)
	{
		m->type = 0;
		m->send_more = jpeg_send_more;
		m->per_ms_incr = (25 * out->ts_incr)/1000;
	}
	return m;

}
 
#if (JPG_EN == 1 || USB_EN == 1)

//侧重与无框架无psram方式,同时数据包头需要留24byte的包头

int sendmsg2(int fd, struct msghdr *msg, unsigned flags)
{	
	int total_len = 0;
	unsigned int i = 0;
	int size = -1;
	int timeouts = 0;
	struct sockaddr_in rtpaddr;
	unsigned int namelen = sizeof( rtpaddr );
	unsigned char *send_start = NULL;

	if( getsockname( fd, (struct sockaddr *)&rtpaddr, &namelen ) < 0 ) {
		spook_log( SL_ERR, "sendmsg getsockname error");
	}
	
	
	//这里到时候整理，将buf数据引导到jpeg的BUF中，减少cpy动作的同时，减少缓存空间
	send_start = msg->msg_iov[msg->msg_iovlen-1].iov_base;
	while(i < msg->msg_iovlen-1) {
		memcpy(send_start + total_len, msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len);
		total_len += msg->msg_iov[i].iov_len;
		i++;
	}
	//不需要copy的位置,所以发送长度要增加
	total_len += msg->msg_iov[msg->msg_iovlen-1].iov_len;

	while(size < 0 )
	{
		size = sendto(fd, send_start, total_len, MSG_DONTWAIT, (struct sockaddr *)&rtpaddr, namelen);
		timeouts++;


		if(timeouts>10)
		{

			break;
		}
		if(timeouts%3 == 0)
		{
			os_sleep_ms(1);
		}

		
	}
	//_os_printf("size:%d\n",size);
	if(timeouts>1)
	{
		_os_printf("timeouts:%d\n",timeouts);
	}
	//_os_printf("%s:%d\tsize:%d\t%d\n",__FUNCTION__,__LINE__,size,total_len);	
	if(size <= 0) {
		_os_printf("%s size err :%d\n",__FUNCTION__,size);
		return -1;
	} else {
		return 0;
	}
}


#endif


//返回发送数据的buf地址
unsigned char *get_send_real_data(struct msghdr *msg,char *sendtobuf)
{	
	unsigned int i = 0;
	int total_len = 0;
	unsigned char *send_start = NULL;
	//实际这里没有用,后面会根据不同的连接设备进行修改
	while(i < msg->msg_iovlen-1) {
		//memcpy(sendtobuf + total_len, msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len);
		//hw_memcpy0(sendtobuf + total_len, msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len);
		total_len += msg->msg_iov[i].iov_len;
		i++;
	}
	#if 1
	//最后的数据补充回来,这里
	uint32_t cp_real_addr = (uint32_t)msg->msg_iov[i].iov_base + total_len;
	uint32_t cp_addr = cp_real_addr/0x10 * 0x10;
	uint32_t diff_len = cp_real_addr - cp_addr;
	if(diff_len)
	{
		//os_printf("diff_len:%d\t%d\t%X\n",diff_len,total_len,cp_addr);
	}
	#else
		uint32_t cp_real_addr = (uint32_t)msg->msg_iov[i].iov_base + total_len;
		uint32_t cp_addr = cp_real_addr;
		uint32_t diff_len = 0;
	#endif
	//这里主要是为了16byte对齐
	hw_memcpy0(sendtobuf + total_len - diff_len, (unsigned char *)cp_addr, msg->msg_iov[i].iov_len + diff_len);
	total_len += msg->msg_iov[i].iov_len;
	i++;
	send_start = (unsigned char *)sendtobuf;
	return send_start;
}



//返回需要发送的长度,配置数据头部数据
int set_send_data_head(unsigned char *send_buf,struct msghdr *msg)
{
	int total_len = 0;
	int i = 0;
	while(i < msg->msg_iovlen-1) {
		hw_memcpy0(send_buf + total_len, msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len);
		total_len += msg->msg_iov[i].iov_len;
		i++;
	}
	total_len += msg->msg_iov[msg->msg_iovlen-1].iov_len;
	return total_len;
}

//端口发送数据,
int fd_send_data(int fd,unsigned char *sendbuf,int sendLen,int times)
{
	int size = -1;
	int timeouts = 0;
	struct sockaddr_in rtpaddr;
	unsigned int namelen = sizeof( rtpaddr );
	if( getsockname( fd, (struct sockaddr *)&rtpaddr, &namelen ) < 0 ) {
		spook_log( SL_ERR, "sendmsg getsockname error");
	}

	
	while(size < 0 )
	{
		//size = sendto(fd, sendtobuf, total_len, MSG_DONTWAIT, (struct sockaddr *)&rtpaddr, namelen);
		size = sendto(fd, sendbuf, sendLen, MSG_DONTWAIT, (struct sockaddr *)&rtpaddr, namelen);
		//_os_printf("P:%d ",size);
		timeouts++;
		if(timeouts>times)
		{

			break;
		}
		if(size < 0)
		{
			os_sleep_ms(2);
		}		
	}
	if(size<0)
	{
		_os_printf(KERN_NOTICE"%s err size:%d\n",__FUNCTION__,size);
		return -1;

	}
	else
	{
		return 0;
	}
}

