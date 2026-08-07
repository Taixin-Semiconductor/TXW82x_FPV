#ifndef BIQUAD_FILTER_H
#define BIQUAD_FILTER_H


typedef int						s32, S32;
typedef unsigned int			u32, U32, uint;
typedef long long int			s64, S64;
typedef unsigned long long		u64, U64;
typedef short int               s16, S16;
typedef unsigned short int      u16, U16;
typedef signed char             s8, S8;
typedef unsigned char           u8, U8;

//模拟64bit芯片运算
int CLIP_INT24(int in);
int CLIP_INT16(int in);
void ML0(s32 x, s32 y);
void MLA(s32 x, s32 y);
void MSB(s32 x, s32 y);
s32 MLZ(s32 n);
s32 MLZ_QUICK(s32 n);
s32 MLZ_FIX(s32 n);
u32 get_acc_64_high_32(void);
u32 get_acc_64_low_32(void);



typedef struct {
    int b[3];
    int a[2];
    int x[2];
    int y[2];
    int fixed_point_num;
} TYPE_BIQUAD_FILTER;

int biquad_filter_calc(TYPE_BIQUAD_FILTER *filter_ctl, int x);
//void biquad_filter_ctl_init(TYPE_BIQUAD_FILTER *filter_ctl, int fixed_point_num);

#if 0
void eq_lpf_coeff_calc(TYPE_BIQUAD_FILTER *filter_ctl,
                         double fs, double f0, double q);
void eq_hpf_coeff_calc(TYPE_BIQUAD_FILTER *filter_ctl,
                         double fs, double f0, double q);
void eq_bpf_coeff_calc(TYPE_BIQUAD_FILTER *filter_ctl,
                         double fs, double f0, double q);
void eq_notch_coeff_calc(TYPE_BIQUAD_FILTER *filter_ctl,
                           double fs, double f0, double q);
void eq_apf_coeff_calc(TYPE_BIQUAD_FILTER *filter_ctl,
                         double fs, double f0, double q);
void eq_peaking_coeff_calc(TYPE_BIQUAD_FILTER *filter_ctl,
                             double fs, double f0, double gain, double q);
void eq_low_shelf_coeff_calc(TYPE_BIQUAD_FILTER *filter_ctl,
                               double fs, double f0, double gain, double q);
void eq_high_shelf_coeff_calc(TYPE_BIQUAD_FILTER *filter_ctl,
                                double fs, double f0, double gain, double q);
#endif

#endif /* BIQUAD_FILTER_H */

