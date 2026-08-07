/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 * @file     gcc_csky.ld
 * @brief    csky linker file
 * @version  V1.0
 * @date     2025.4.12
 * SRAM0-0: 0x20000000 ~ 0x20003FFF 16KB  
 * SRAM0-1: 0x20004000 ~ 0x2001FFFF 112KB 
 * SRAM1:   0x20020000 ~ 0x2003FFFF 128KB 
 * SRAM2-0: 0x20040000 ~ 0x20057FFF 96KB  
 * SRAM2-1: 0x20058000 ~ 0x2005FFFF 32KB (DCAHCE) 
 * SRAM2-2: 0x20060000 ~ 0x20067FFF 32KB (CPU1 I/D CACHE) 
 * SRAM2-3: 0x20068000 ~ 0x2006BF00 16KB (末尾256byte为CoreSetting数据)
 * SRAM3:   0x20080000 ~ 0x2009FFFF 128KB (H264)   
 ******************************************************************************/
MEMORY
{
    ISRAM  : ORIGIN = 0x04001000 , LENGTH = 0x2a000   
    SRAM   : ORIGIN = 0x200065fc , LENGTH = 0x40000  /*起始地址为CoreCode BSS结束地址 */ 
    SRAM2  : ORIGIN = 0x20068000 , LENGTH = 0x3A00    /*末尾预留256byte做为CoreSetting数据*/
    FLASH  : ORIGIN = 0x10000000 , LENGTH = 0x200000 
    PSRAM  : ORIGIN = 0x28000000 , LENGTH = 0x800000 
    SRAM0   : ORIGIN = 0x20001100 , LENGTH = 0x2F00   /*SRAM0 16 - 4KB for sleep*/ 
    ALUSER-SRAM : ORIGIN = 0x2006BA00 , LENGTH = 0x400         /* 二次loader的固件参数区 ALUSER-SRAM  1KB */
	FW_INFO : ORIGIN = 0x2006BE00 , LENGTH = 0x100
}

__heap_size = 0x40000;

PROVIDE (__ram_end  = 0x20058000);
PROVIDE (__heap_end = 0x20058000);
PROVIDE (__sleep_data_start = 0x20000200);
PROVIDE (__sleep_data_stop  = 0x20001000);
PROVIDE (__psram_heap_end = 0x29000000);
PROVIDE (_data0_lma_start = 0);
PROVIDE (__data0_start__ = 0);
PROVIDE (__data0_end__ = 0);

REGION_ALIAS("REGION_TEXT",    FLASH);
REGION_ALIAS("REGION_RODATA",  FLASH);
REGION_ALIAS("REGION_DATA",    SRAM);
REGION_ALIAS("REGION_BSS",     SRAM);
REGION_ALIAS("REGION_INIT",    SRAM);
REGION_ALIAS("REGION_PSRAM_HEAP",    PSRAM);
REGION_ALIAS("REGION_NOLOAD_PSRAM",    PSRAM);
REGION_ALIAS("REGION_DATA2",    SRAM2);
REGION_ALIAS("REGION_OTA_VMA",    SRAM);
REGION_ALIAS("REGION_TEXT2",    SRAM2);
REGION_ALIAS("REGION_DSLEEP",    SRAM0);
REGION_ALIAS("REGION_ALUSER",    ALUSER-SRAM);


ENTRY(Reset_Handler)
SECTIONS
{
    .text : AT(ADDR(.text)) {
        . = ALIGN(0x4) ;
        __code_start = .;
        KEEP(*startup.o(.vectors))
		. = ALIGN(0x10) ;
        KEEP(*(SYS_PARAM))     /*如果预留参数区的Size 大于4K时 不能再放在此位置*/
        . = ALIGN(0x1000);
        . = . + 0x5f6d8; /*预留CodeCode Size大小区间*/
        . = ALIGN(0x4);
        __stext = . ;
        *(.text)
        *(.text*)
        *(.text.*)
        *(INIT.TXT)
        *(INIT.TXT.*)
        *(.gnu.warning)
        *(.stub)
        *(.gnu.linkonce.t*)
        *(.glue_7t)
        *(.glue_7)
        *(.jcr)
        KEEP (*(.init*))
        KEEP (*(.fini))
        . = ALIGN (4) ;
        PROVIDE(__ctbp = .);
        *(.call_table_data)
        *(.call_table_text)
        . = ALIGN(4) ;
        __etext = . ;
    } > REGION_TEXT

    .rodata : {
        . = ALIGN(0x4) ;
        __srodata = .;
        *(.rdata)
        *(.rdata*)
        *(.rdata1)
        *(.rdata.*)
        *(.rodata)
        *(.rodata1)
        *(.rodata*)
        *(.rodata.*)
        *(.rodata.str1.4)
        *(INIT.RO)      
        *(INIT.RO.*)        
        . = ALIGN(0x4) ;
        
        __modver_start = .;
        KEEP(*(.modver))
        __modver_end = .;
        
        . = ALIGN(0x4) ;
        __CTOR_LIST__ = . ;
        LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
        KEEP(*(SORT(.ctors)))
        LONG(0)
        __CTOR_END__ = . ;
        __DTOR_LIST__ = . ;
        LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
        KEEP(*(SORT(.dtors)))
        LONG(0)
        __DTOR_END__ = . ;

        . = ALIGN(0x4) ;
    } > REGION_RODATA
   
    .dsleep : {
    	__data0_start__ = . ;
        __dsleep_start = .;
 		*(.dsleep.*)
		. = ALIGN(0x4) ;
	    __dsleep_end = .;
        __data0_end__ = . ;
    } > REGION_DSLEEP AT>REGION_RODATA
  _data0_lma_start = LOADADDR(.dsleep);
	_data0_lma_end = LOADADDR(.dsleep) + SIZEOF(.dsleep);
    
    .data : {
        . = ALIGN(4) ;
        __sdata = . ;
        __data_start__ = . ;
        data_start = . ;
        
  
        *(INIT.DAT)
        *(INIT.DAT.*)
	    
        *(.got.plt)
        *(.got)
        *(.gnu.linkonce.r*)
        *(.bobj)
        *(.data)
        *(.data*)
        *(.data1)
        *(.data.*)
        *(.gnu.linkonce.d*)
        *(.data1)
        *(.gcc_except_table)
        *(.gcc_except_table*)
        __start_init_call = .;
        *(.initcall.init)
        __stop_init_call = .;
        __start_cmd = .;
        *(.bootloaddata.cmd)
        . = ALIGN(4) ;
        __stop_cmd = .;
        *(.sdata)
        *(.sdata.*)
        *(.gnu.linkonce.s.*)
        *(__libc_atexit)
        *(__libc_subinit)
        *(__libc_subfreeres)
        *(.note.ABI-tag)
        *(.ram.text)
        . = ALIGN(0x4) ;
        __edata = .;
        __data_end__ = .;
    } > REGION_DATA AT>REGION_RODATA
    _data_lma_start = LOADADDR(.data);
    _data_lma_end = LOADADDR(.data) + SIZEOF(.data);

	.fw_info : {
		__fw_info_start = .;
		. += 160;
		__fw_info_end = .;
	} > FW_INFO

    .no_init (NOLOAD) : {
        *(.irq_stack);
        *(.trap_stack);
        *(.no_init);
    } > REGION_BSS
    
    .ota : {
        . = ALIGN(0x4) ;
        _ota_vma_start = . ;
        KEEP (*(.otacode))
        _ota_vma_end = . ;
    } > REGION_OTA_VMA AT > REGION_RODATA
    _ota_lma_start = LOADADDR(.ota);
    _ota_lma_end = LOADADDR(.ota) + SIZEOF(.ota);
    
    
    .iis : AT(LOADADDR(.ota) + SIZEOF(.ota)) {
        . = ALIGN(0x4) ;
        iis_start = .;
        KEEP (*(.iiscode))
    } > REGION_OTA_VMA
    _iis_start = LOADADDR(.iis);
    _iis_end = LOADADDR(.iis) + SIZEOF(.iis);
    __code_end = LOADADDR(.iis) + SIZEOF(.iis);

    ._al_user_sram (NOLOAD): {
        . = ALIGN(0x4) ;
        __al_user_sram_start = .;
        *(.al_user_data)
        __al_user_sram_current = .;
    } > REGION_ALUSER AT > REGION_BSS  

    .bss : {
        . = ALIGN(0x4) ;
        __sbss = ALIGN(0x4) ;
        __bss_start__ = . ;
        *(.dynsbss)
        *(.sbss.ota)
        *(.sbss)
        *(.sbss.*)
        *(.scommon)
        *(.dynbss)
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(0x4) ;
        __ebss = . ;
        __end = . ;
        end = . ;
        __bss_end__ = .;
    } > REGION_BSS
    
    ._user_heap : {
        . = ALIGN(0x4) ;
        __heap_start = .;
    } > REGION_BSS
  
    ._psram_data (NOLOAD): {
        *(.psram.src)
        *(.psram.data)
    } > REGION_NOLOAD_PSRAM
    
    .psram_heap : {
        . = ALIGN(4) ;
        __psram_heap_start = .;
    } > REGION_PSRAM_HEAP


    .eh_frame : ONLY_IF_RO { KEEP (*(.eh_frame)) } > REGION_BSS
    .gcc_except_table : ONLY_IF_RO { *(.gcc_except_table .gcc_except_table.*) } > REGION_BSS
    .eh_frame : ONLY_IF_RW { KEEP (*(.eh_frame)) }
    .gcc_except_table : ONLY_IF_RW { *(.gcc_except_table .gcc_except_table.*) }
    .eh_frame_hdr : { *(.eh_frame_hdr) }
    
    .preinit_array :{
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    }
    
    .init_array :{
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);
    }
    
    .fini_array :{
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        PROVIDE_HIDDEN (__fini_array_end = .);
    }
    
    .junk 0 : { *(.rel*) *(.rela*) }
    .stab 0 : { *(.stab) }
    .stabstr 0 : { *(.stabstr) }
    .stab.excl 0 : { *(.stab.excl) }
    .stab.exclstr 0 : { *(.stab.exclstr) }
    .stab.index 0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment 0 : { *(.comment) }
    .debug 0 : { *(.debug) }
    .line 0 : { *(.line) }
    .debug_srcinfo 0 : { *(.debug_srcinfo) }
    .debug_sfnames 0 : { *(.debug_sfnames) }
    .debug_aranges 0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    .debug_info 0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_line 0 : { *(.debug_line) }
    .debug_frame 0 : { *(.debug_frame) }
    .debug_str 0 : { *(.debug_str) }
    .debug_loc 0 : { *(.debug_loc) }
    .debug_macinfo 0 : { *(.debug_macinfo) }
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames 0 : { *(.debug_varnames) }
    .debug_pubtypes 0 : { *(.debug_pubtypes) }
    .debug_ranges 0 : { *(.debug_ranges) }
    .gnu.attributes 0 : { KEEP (*(.gnu.attributes)) }
    /DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) }
}


/* ROM Functions */
/* ASSERT(DEFINED(ROM_FUNC_ENABLE), "please disable ROM Functions in ld file") */
/* ASSERT(ADDR(.text) < 0x20000068, "ROM Functions data area conflict!!!")*/

/* INPUT(romcode.ld) */ 
  memset                              = 0x00000568; 
  memcpy                              = 0x00000608; 
  memcmp                              = 0x00000688; 
