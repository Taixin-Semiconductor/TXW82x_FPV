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
	int quant[16];
	int huffoff;
	int hufflen;
	unsigned char exthdr[EXTHDR_LEN];
	unsigned char *scan_data;
	int scan_data_len;
	int offset;
	int init_done;
	int ts_incr;
	unsigned int timestamp;	
	int reset_interval;
	int dri_num;
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
  	out->huffoff = (int)( ( d - 4 ) - out->d );
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

static void jpeg_send_frame_to_endpoint( struct rtp_endpoint *ep, struct rtp_jpeg *out, struct framebuff *fb )
{
	uint8_t *jpeg_buf_addr = (uint8_t *)fb->data;
	int i = 0, plen, vcnt, hdr_len;
	uint32_t node_offset = out->offset;
	struct iovec v[8];
	uint8_t vhdr[12], qhdr[4];
	int res_len = 1, max_data_size;
	uint32_t node_len = out->f->node_len;

	if( !ep )
	{
		return;
	}
	if( !ep->sendEnable )
	{
		ep->sendEnable = 1;
		return;
	}

	vhdr[0] = 0;
	vhdr[4] = out->type;
	vhdr[5] = 255;
	vhdr[6] = out->width;
	vhdr[7] = out->height;
	v[1].iov_base = vhdr;
	v[1].iov_len = ( out->type & 0x40 ) ? 12 : 8;

	qhdr[0] = 0;
	qhdr[1] = 0;
#ifdef USE_EXTHDR
	PUT_16( qhdr + 2, 2 * 64 + out->hufflen + EXTHDR_LEN );
#else
	PUT_16( qhdr + 2, 2 * 64 );
#endif
	v[2].iov_base = qhdr;
	v[2].iov_len = 4;
	v[3].iov_base = jpeg_buf_addr + out->quant[out->luma_table];
	v[3].iov_len = 64;
	v[4].iov_base = jpeg_buf_addr + out->quant[out->chroma_table];
	v[4].iov_len = 64;
	hdr_len = 132 + v[1].iov_len;
#ifdef USE_EXTHDR
	vcnt = 7;
	v[5].iov_base = out->exthdr;
	v[5].iov_len = EXTHDR_LEN;
	/* The DHT block stays inside the current JPEG frame, so it can be sent directly. */
	v[6].iov_base = jpeg_buf_addr + out->huffoff;
	v[6].iov_len = out->hufflen;
	hdr_len += v[5].iov_len + v[6].iov_len;
#else
	vcnt = 5;
#endif

	while( i < out->scan_data_len )
	{
		if( node_offset >= node_len ) 
		{
			node_offset = 0;
		}

		max_data_size = rtp_get_payload_size_limit( ep, RTP_HEADER_SIZE + hdr_len );
		if( max_data_size <= 0 )
		{
			ep->sendEnable = 0;
			break;
		}
		if( out->type & 0x40 )
		{
			if( ( out->scan_data_len - i ) > max_data_size )
			{
				plen = max_data_size;
			}
			else
			{
				plen = out->scan_data_len - i;
			}
			
			if( res_len )
			{
				vhdr[8] = out->exthdr[EXTHDROFF_DRI];
				vhdr[9] = out->exthdr[EXTHDROFF_DRI + 1];
			}
			else
			{
				PUT_16( vhdr + 8, 0x28 );
			}
			PUT_16( vhdr + 10, 0xffff );
		}
		else
		{
			plen = out->scan_data_len - i;
			if( plen > max_data_size )
			{
				plen = max_data_size;
			}
			
		}

		vhdr[1] = i >> 16;
		vhdr[2] = ( i >> 8 ) & 0xff;
		vhdr[3] = i & 0xff;
		v[vcnt].iov_base = jpeg_buf_addr + node_offset;

		if( node_offset + plen >= node_len )
		{
			plen = node_len - node_offset;
		}
		else
		{
			uint32_t align_32 = ( node_offset + plen ) & ( ~0x1F );
			plen = align_32 - node_offset;
		}

		if( i + plen >= out->scan_data_len ) plen = out->scan_data_len - i;
		node_offset += plen;
		v[vcnt].iov_len = plen;
		if( rtp_sendmsg( ep, v, vcnt + 1, out->timestamp, plen + i == out->scan_data_len, 30 ) < 0 )
		{
			ep->sendEnable = 0;
			break;
		}

		vcnt = 2;
		hdr_len = v[1].iov_len;
		i += plen;
	}

	ep->sendEnable = 1;
}

static int jpeg_send_more( rtp_loop_search_ep search,void *ls,void *track, void *d )
{
	struct rtp_jpeg *out = (struct rtp_jpeg *)d;
	struct framebuff *fb = (struct framebuff *)out->f->get_f;
	struct rtp_endpoint *ep;
	void *head;

	if( !fb )
	{
		return 0;
	}
	out->scan_data_len = out->f->length - out->offset;

	head = ls;
	while( head )
	{
		head = search( head, track, (void **)&ep );
		if( ep )
		{
			jpeg_send_frame_to_endpoint( ep, out, fb );
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
	out->ts_incr = 90000 * fincr / fbase;
	m = new_rtp_rtcp_media( jpeg_get_sdp, jpeg_get_payload, jpeg_process_frame, jpeg_send, new_rtcp_send, out );
	if(m)
	{
		m->type = 0;
		m->send_more = jpeg_send_more;
		m->per_ms_incr = (25 * out->ts_incr)/1000;
	}
	return m;

}

void del_rtp_media_jpeg_stream( struct rtp_media *m )
{
	struct rtp_jpeg *out = (struct rtp_jpeg *)m->private;
	if(out)
	{
		free(out);
	}

	if(m)
	{
		free(m);
	}
}
