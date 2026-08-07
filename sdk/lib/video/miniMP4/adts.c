/**
 * @file adts.c
 * @brief 
 * @author licaibiao
 * @version 1.0
 * @date 2023-12-06
 * 
 * @copyright Copyright (c) 2023-2030 liwen01 Technology Co., Ltd
 * 
 */
#include "adts.h"
#include <math.h>

void set_fixed_header(adts_fixed_header *header) {
    header->syncword                 = 0xFF;// 代表着一个帧的开始
    header->id                       = 1;// MPEG Version: 0 for MPEG-4, 1 for MPEG-2
    header->layer                    = 0;// always 00
    header->protection_absent        = 1;// 表示是否误码校验
    header->profile                  = 1;// 表示使用哪个级别的AAC, 有些芯片只支持AAC LC.
    header->sampling_frequency_index = 0xb;// 表示使用的采样率的下标
    header->private_bit              = 0;
    header->channel_configuration    = 1;// 表示声道数
    header->original_copy            = 0;
    header->home                     = 0;
}

void set_variable_header(adts_variable_header *header, const int aac_raw_data_length) {
    header->copyright_identification_bit       = 0;
    header->copyright_identification_start     = 0;
    header->aac_frame_length                   = aac_raw_data_length + 7;
    header->adts_buffer_fullness               = 0x7f;
    header->number_of_raw_data_blocks_in_frame = 0;
}

void get_fixed_header(const unsigned char buff[7], adts_fixed_header *header) {
    unsigned long long adts = 0;
    const unsigned char *p = buff;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++;
    
    
    header->syncword                 = (adts >> 44);
    header->id                       = (adts >> 43) & 0x01;
    header->layer                    = (adts >> 41) & 0x03;
    header->protection_absent        = (adts >> 40) & 0x01;
    header->profile                  = (adts >> 38) & 0x03;
    header->sampling_frequency_index = (adts >> 34) & 0x0f;
    header->private_bit              = (adts >> 33) & 0x01;
    header->channel_configuration    = (adts >> 30) & 0x07;
    header->original_copy            = (adts >> 29) & 0x01;
    header->home                     = (adts >> 28) & 0x01;
}

void get_variable_header(const unsigned char buff[7], adts_variable_header *header) {
    unsigned long long adts = 0;
    adts  = buff[0]; adts <<= 8;
    adts |= buff[1]; adts <<= 8;
    adts |= buff[2]; adts <<= 8;
    adts |= buff[3]; adts <<= 8;
    adts |= buff[4]; adts <<= 8;
    adts |= buff[5]; adts <<= 8;
    adts |= buff[6];
    
    header->copyright_identification_bit = (adts >> 27) & 0x01;
    header->copyright_identification_start = (adts >> 26) & 0x01;
    header->aac_frame_length = (adts >> 13) & ((int)pow(2, 14) - 1);
    header->adts_buffer_fullness = (adts >> 2) & ((int)pow(2, 11) - 1);
    header->number_of_raw_data_blocks_in_frame = adts & 0x03;
}

void convert_adts_header2int64(const adts_fixed_header *fixed_header, const adts_variable_header *variable_header, uint64_t *header) {
    uint64_t ret_value = 0;
    ret_value = ((fixed_header->syncword<<4) | 0x0F);
    ret_value <<= 1;
    ret_value |= fixed_header->id;
    ret_value <<= 2;
    ret_value |= fixed_header->layer;
    ret_value <<= 1;
    ret_value |= (fixed_header->protection_absent) & 0x01;
    ret_value <<= 2;
    ret_value |= fixed_header->profile;
    ret_value <<= 4;
    ret_value |= (fixed_header->sampling_frequency_index) & 0x0f;
    ret_value <<= 1;
    ret_value |= (fixed_header->private_bit) & 0x01;
    ret_value <<= 3;
    ret_value |= (fixed_header->channel_configuration) & 0x07;
    ret_value <<= 1;
    ret_value |= (fixed_header->original_copy) & 0x01;
    ret_value <<= 1;
    ret_value |= (fixed_header->home) & 0x01;
    
    ret_value <<= 1;
    ret_value |= (variable_header->copyright_identification_bit) & 0x01;
    ret_value <<= 1;
    ret_value |= (variable_header->copyright_identification_start) & 0x01;
    ret_value <<= 13;
    ret_value |= (variable_header->aac_frame_length) & ((int)pow(2, 13) - 1);
    ret_value <<= 11;
    ret_value |= ((variable_header->adts_buffer_fullness<<4) | 0x0F ) & ((int)pow(2, 11) - 1);
    ret_value <<= 2;
    ret_value |= (variable_header->number_of_raw_data_blocks_in_frame) & ((int)pow(2, 2) - 1);
    
    *header = ret_value;
}

void convert_adts_header2char(const adts_fixed_header *fixed_header, const adts_variable_header *variable_header, unsigned char buffer[7]) {
    uint64_t value = 0;
    convert_adts_header2int64(fixed_header, variable_header, &value);
    buffer[0] = (value >> 48) & 0xff;
    buffer[1] = (value >> 40) & 0xff;
    buffer[2] = (value >> 32) & 0xff;
    buffer[3] = (value >> 24) & 0xff;
    buffer[4] = (value >> 16) & 0xff;
    buffer[5] = (value >> 8) & 0xff;
    buffer[6] = (value) & 0xff;
}

void aac_dsi_to_adts(uint8_t *dsi, uint8_t *adts, uint32_t aac_data_length)
{
    uint8_t audio_object_type = (dsi[0] >> 3) & 0x1F;
    uint8_t profile = audio_object_type - 1;
    uint8_t samplerate_index = ((dsi[0] & 0x07) << 1) | (dsi[1] >> 7);
    uint8_t channels = (dsi[1] >> 3) & 0x0F; 
	;uint32_t frame_length = aac_data_length + 7;

    adts[0] = 0xFF;                         //syncword hight 8 bits
    adts[1] = 0xF0;                         //syncword low 4 bits
    adts[1] |= (0x01 << 3);                 //ID 0:MPEG-4,1:MPEG-2
    adts[1] |= (0x00 << 1);                 //layer:00
    adts[1] |= (0x01 & 0x1);                //1:no CRC,0:is CRC
    adts[2] = (profile << 6);               //profile 0x01:AAC-LC
    adts[2] |= (samplerate_index << 2);
    adts[2] |= (0x00 << 1);                 //private_bit (0)
    adts[2] |= (channels & 0x4) >> 2;
    adts[3] = (channels & 0x3) << 6;
    adts[3] |= (frame_length >> 11);      	//frame_length bits12-bits13
    adts[4] = (frame_length >> 3) & 0xFF; 	//frame_length bits4-bits11
    adts[5] = (frame_length & 0x07) << 5; 	//frame_length low 3 bits first
    adts[5] |= (0x7FF >> 6) & 0x1F;         //adts_buffer_fullness high 5 bits
    adts[6] = (0x7FF & 0x3F) << 2;          //adts_buffer_fullness low 6 bits
    adts[6] |= (0x00);                      //number_of_raw_data_blocks_in_frame (0)
}