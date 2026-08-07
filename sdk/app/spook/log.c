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
#include <stdarg.h>
//#include <test_util.h>
#include <csi_config.h>

#include "lwip/api.h"

#if 0

//#include <portmacro.h>

//extern void Uart_PutStr(unsigned char *p_str);

#define LOG_BUFFER_SIZE		1024

static int minlevel;
void spook_log_init( int min )
{
	minlevel = min;
}

char string[ LOG_BUFFER_SIZE ];

void spook_debug( int level, char *fmt, ... )
{
	
	#if 1
	va_list ap; 
	//static char string[ LOG_BUFFER_SIZE ];
	//OS_CPU_SR  cpu_sr = 0u;

	if( level < minlevel )
	{
		va_end( ap );
		return;
	}

	//OS_ENTER_CRITICAL(); 
	//disable_irq();

	va_start( ap, fmt );
	vsprintf( string , fmt, ap );	 
	va_end( ap );	

	//Uart_PutStr(string);
	_os_printf("%s",string);
	//__enable_irq();
	#endif

	//OS_EXIT_CRITICAL();
}

#endif

