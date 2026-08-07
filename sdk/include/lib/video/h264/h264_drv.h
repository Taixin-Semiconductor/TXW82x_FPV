#ifndef _H264_MODE_DEFINE_
#define _H264_MODE_DEFINE_


struct  h264_cfg_t {
	uint8_t    enc_mode      ; //0:dec, 1:enc
	uint8_t		enc_bs_buf_size;//0:1KB; 1:2KB; 2:4KB; 3:8KB; 4:16KB
	uint16_t   frm_width     ;
	uint16_t   frm_height    ;
	uint16_t   wrap_width;
	uint16_t   wrap_height;
	uint16_t   enc_bps       ; //Kbit pre second
	uint16_t   frm_rate      ; //fps
	uint16_t   frm_gop       ; //IPPPP frame number of a gop
	uint8_t    rc_en         ; //enc rate control enable
	uint8_t    rc_grp        ; //mb line number when RC change qp
	uint8_t    rc_effort     ; //0:low; 1: high; 2:relax;
	uint8_t    frm_ip_rate   ; //initial (Intra MB line)/(Inter MB line) target bit rate times; not critical parameter.
	uint8_t    frm_ip_rate_max;
	uint8_t    frm_ip_rate_min;
	uint8_t    flt3d_en      ; //3D filter enable
    uint8_t    flt3d_en_last ;
	uint8_t    flt_noise_lev ; //0: very low nosie; 1: low; 2: medium; 3: high; 4: very high; naomal usage is: 2
    uint8_t    flt_noise_lev_last;
	uint8_t    ini_qp        ; //initial QP for this sequence, not critical
	uint8_t    roi_qp        ; //QP used in ROI windows
	uint8_t    src_from      ;
	uint8_t    count         ;
	uint32_t   r3dnr         ;
	uint8_t    gop_rst_3dnr  ;
	uint8_t    run_gop_3dnr  ;
	uint8_t    auto_ctl_3dnr ;  //need open vpp det
	uint32_t    last_vpp_md;
	uint32_t    md_3dnr_en;
	uint8_t    md_3dnr_timer;
	uint8_t    md_set_3dnr;
	uint8_t    enc_runing;

};

struct  h264_ctl_t {
	uint8_t    enc_mode      ;   //0:dec, 1:enc
	uint8_t		bs_buf_id			;		//0:use buf0; 1:use buf1
	uint32_t		bs_base0			;		//bitream buffer base addr, 1KB aligned
	uint32_t		bs_base1			;		//bitream buffer base addr, 1KB aligned
	uint32_t   ref_lu_base   ;   //ref luma buffer base address, 4KB aligned
	uint32_t   ref_lu_end    ;   //ref luma buffer end address, 4KB aligned
	uint32_t   ref_ch_base   ;   //ref chroma buffer base address, 4KB aligned
	uint32_t   ref_ch_end    ;   //ref chroma buffer end address, 4KB aligned
	uint8_t    frm_type      ;   //0:P frame; 2:I frame
	uint8_t    frm_qp        ;   //qp for this frame
	uint16_t   frm_width     ;
	uint16_t   frm_height    ;
	uint16_t   frm_cnt       ;   //encoded frame numebr cnt
	uint8_t    gop_frm_cnt   ;   //0~N cnt within a gop
	uint8_t    idrid         ;
	uint32_t   target_gop    ;   //target bs byte of a gop
	uint32_t   frm_target    ;   //rc target byte of this frame
	uint32_t   last_lpf_lu_base; //last frame lpf luma write base
	uint32_t   last_lpf_ch_base; //last frame lpf chroma write base
	uint16_t   frm_mblines   ;   //frm_height/16
	uint32_t   frm_luma_size ;   //frm_width*frm_height
	uint32_t   timeout_limit ;   //timeout cycle for a frame
	uint32_t   bs_byte_limit ;   //max enc bitstream length
	uint8_t    enc_first_frm ;   //1: first frame; 0: not first frame
	uint32_t   enc_end_flags ;   //flags after end enc a frame
	uint32_t   enc_bs_byte   ;   //enc bitstream byte length of a frame
	uint8_t    hw_cal_qp     ;   //when RC enable, HW cal qp for next frame
	uint32_t   base_bs       ;
};


struct  h264_rc_ctl_t {
	uint8_t    i_qp            ;   //start QP for I frame
	uint8_t    p_qp            ;   //start QP for p frame
	uint16_t   p_qp_acc        ;   //accumulate QP for last 4 P frames of a gop
	uint16_t   p_acc_num       ;   //frame number of QP accumulate
	uint16_t   gop_remain_frame;   //remian frame number with in a GOP
	int32_t   gop_remain_byte ;   //remain bitstream byte size for frames in this GOP
	uint32_t   i_byte          ;   //target bitstream byte size for I frame
	uint32_t   i_frm_max       ;   //max target byte for I frame
	uint32_t   i_frm_min       ;   //min target byte for I frame
	uint32_t   p_avg           ;   //P frame average byte target in a GOP
	uint32_t   p_frm_max       ;   //max target byte for P frame
	uint32_t   p_frm_min       ;   //min target byte for P frame
	uint32_t   frm_rdcost      ;   //frame rdcost
};


//--- decoder related ---//
enum nal_type {
	pic_data= 1, 
	pic_idr = 5,
	sps     = 7,
	pps     = 8
};

struct str_info{
	uint8_t*   ptr       ;
	uint32_t   stream    ; //after each h264_read_bs(), valid bit>=25
	uint8_t    bit_len   ;
	uint32_t   used_bits ;
};

struct  h264_header {
	uint8_t    idr_flag      ;
	uint16_t   sps_log2_max_fnum_minus4;
	uint16_t   sps_log2_max_poc_lsb_minus4;
	uint16_t   sps_pic_width ; 
	uint16_t   sps_pic_height; 
	uint8_t    pps_qp_ini    ;
	uint8_t    pps_cavlc_mode; 
	int8_t    pps_chrom_qp_offset;
	uint8_t    pic_slice_type; 
	uint8_t    pic_qp        ; 
};

typedef struct 
{
	struct list_head list;				//h264_frame的节点头
	uint32 frame_len;						//帧长度
	uint8 usable;							//判断是否可用       0:空闲
											//			   1:节点补充中或正在使用中
											//			   2:可用
	uint8 h264_num;						//2~255
	//uint8 h264_type;					//1:I frame  2:P frame
	uint8 h264_type: 2, which: 3,srcID: 3;		//h264_type暂时是I帧  P帧(兼容旧版本),which是指源头之类(可能从vpp0编码,可能从gen420编码,或者是vpp0的第二个摄像头,都有可能,暂时给3bit,可以代表7种)
	uint8 h264_dev_id;	
	uint32_t timestamp;
	uint16_t w,h;
}h264_frame;

typedef struct
{     
	struct list_head list;
	uint8* buf_addr;
}h264_node;


struct stream_h264_data_s
{
	struct stream_h264_data_s *next;
	void *data;
	int ref;
};




#define IMAGE_W_H264      1280//1920//
#define IMAGE_H_H264	  720//1088//

#define H264_BS_SIZE      512*1024                    //512K

#define H264_NODE_LEN     16*1024                     //不要改
#define H264_NODE_NUM     32
#define H264_FRAME_NUM    4

#define H264_DEV_0_ID     0
#define H264_DEV_1_ID     1

void h264_drv_init();
int h264_enc(uint32_t drv1_from,uint32_t drv1_w,uint32_t drv1_h,uint32_t drv2_from,uint32_t drv2_w,uint32_t drv2_h);
uint32 get_h264_len(void *d);
void del_h264_frame(void *d);
uint32 get_h264_node_len_new(void *get_f);
void *get_h264_first_buf(void *d);
int del_h264_first_node(void *d);
void *get_h264_node_buf(void *d);
void del_h264_node(void *d);
uint8 get_h264_type(void *d);
uint8 get_h264_loop_num(void *d);
void h264_dec_src_264(uint8 *src_file,uint32 file_size,uint32 w,uint32 h,uint32_t devid);
void h264_dec_room_init(uint8_t devnum,uint16_t drv2_w,uint16_t drv2_h);
struct list_head* get_h264_frame();
uint32 get_h264_timestamp(void *d);
int h264_mem_init(uint8_t init,uint32_t drv1_w,uint32_t drv1_h,uint32_t drv2_w,uint32_t drv2_h);
uint32 get_h264_which(void *d);
void h264_recfg_bsp(uint8_t dev_num,uint16_t bps);
void h264_recfg_rate(uint8_t dev_num,uint16_t rate);
void h264_recfg_frm_gop(uint8_t dev_num,uint16_t gop);
void h264_recfg_ini_qp(uint8_t dev_num,uint8_t qp);
void h264_reflash_new_gop(uint8_t dev_num,uint8_t en);
void h264wq_queue_init(uint16_t w,uint16_t h);

uint8_t h264msg_queue_done(uint32 list);
int put_h264msg_to_queue(uint8_t type,uint32_t w,uint32_t h,uint32 data,uint32 len);
#endif
