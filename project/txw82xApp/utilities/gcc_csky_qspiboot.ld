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
 * @date     02. June 2017
 ******************************************************************************/
MEMORY
{
    I-SRAM   : ORIGIN = 0x0        , LENGTH = 0x80000   /* I-SRAM  128KB */
    D-SRAM   : ORIGIN = 0x20000000 , LENGTH = 0x80000   /* D-SRAM  512KB */
    O-SRAM   : ORIGIN = 0x50000000 , LENGTH = 0x800000   /* off-chip SRAM 8MB */
    FLASH    : ORIGIN = 0x18000000 , LENGTH = 0x200000   /* MCP FLASH 2MB */
    SRAM     : ORIGIN = 0x20002000 , LENGTH = 0x46000   /* on-chip SRAM 288KB */
    LMAC_BUF : ORIGIN = 0x20051000 , LENGTH = 18*1024   /* on-chip SRAM 18KB */
}

__heap_size = 0x11000;

PROVIDE (__ram_end  = 0x20048000);
PROVIDE (__heap_end = 0x20048000);

REGION_ALIAS("REGION_TEXT",    FLASH);
REGION_ALIAS("REGION_RODATA",  FLASH);
REGION_ALIAS("REGION_DATA",    SRAM);
REGION_ALIAS("REGION_BSS",     SRAM);

ENTRY(Reset_Handler)
SECTIONS
{
    .text : AT(ADDR(.text)) {
        . = ALIGN(0x4) ;
        KEEP(*startup.o(.vectors))
        KEEP(*(SYS_PARAM))
        *(.dsleep_data)
        __stext = . ;
        *(.text)
        *(.text*)
        *(.text.*)
        *(.gnu.warning)
        *(.stub)
        *(.gnu.linkonce.t*)
        *(.glue_7t)
        *(.glue_7)
        *(.jcr)
        KEEP (*(.init))
        KEEP (*(.fini))
        . = ALIGN (4) ;
        PROVIDE(__ctbp = .);
        *(.call_table_data)
        *(.call_table_text)
        . = ALIGN(0x10) ;
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
        KEEP (*crtbegin.o(.ctors))
        KEEP (*crtbegin?.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
        KEEP (*crtbegin.o(.dtors))
        KEEP (*crtbegin?.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
        . = ALIGN(0x4) ;
        __erodata = .;
    } > REGION_RODATA
    
    .data : AT(LOADADDR(.rodata) + SIZEOF(.rodata)){
        . = ALIGN(0x4) ;
        __sdata = . ;
        __data_start__ = . ;
        data_start = . ;
        *(.got.plt)
        *(.got)
        *(.gnu.linkonce.r*)
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
        . = ALIGN(0x4) ;
        __edata = .;
        __data_end__ = .;
    } > REGION_DATA
    
    __dsleep_ro_start__ = LOADADDR(.rodata) + SIZEOF(.rodata) + SIZEOF(.data);
    .dsleep : AT(__dsleep_ro_start__) {
        __dsleep_code_start__ = .;
        KEEP(*(.dsleep_vector))
        *(.dsleep_txt)
        *(.lmac_dsleep_txt)
        *(.wphy_dsleep_txt)
        *(.rf_digicali_dsleep_txt)
        __dsleep_code_end__ = .;
    } > LMAC_BUF

    .dsleep_buf (NOLOAD) : {
        *(.dsleep_stack)
        *(.lmac_dsleep_data)
        *(.wphy_dsleep_data)
        *(.rf_digicali_dsleep_data)
    } > LMAC_BUF
    
    /* 当.dsleep的代码是放到sram，则在bss之前需要预留.dsleep加载代码的空间 */
    .fill_dsleep_code (NOLOAD) : {
        . = . + (((__dsleep_ro_start__ & 0x20000000) == 0x20000000) ? SIZEOF(.dsleep) : 0);
    } > REGION_DATA
    
    .no_init (NOLOAD) : {
        *(.stack);
        *(.no_init);
    } > REGION_BSS
    .bss : {
        . = ALIGN(0x4) ;
        __sbss = ALIGN(0x4) ;
        __bss_start__ = . ;
        *(.dynsbss)
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

    ._pdata_w 0x28800000 (NOLOAD): {
        *(.psram.src)
    } 
 /DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) }
}


/* ROM Functions */
/* ASSERT(DEFINED(ROM_FUNC_ENABLE), "please disable ROM Functions in ld file") */
/* ASSERT(ADDR(.text) < 0x20000068, "ROM Functions data area conflict!!!")*/

/* INPUT(romcode.ld) */

